/*---------------------------------------------------------------
 * Copyright (c) 2017-2021
 * Broadcom Corporation
 * All Rights Reserved.
 *---------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 *
 * Redistributions of source code must retain the above
 * copyright notice, this list of conditions and
 * the following disclaimers.
 *
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimers in the documentation and/or other materials
 * provided with the distribution.
 *
 *
 * Neither the name of Broadcom Coporation,
 * nor the names of its contributors may be used to endorse
 * or promote products derived from this Software without
 * specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTIBUTORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ________________________________________________________________
 *
 * isochronous.c
 * Suppport for isochonronous traffic testing
 *
 * by Robert J. McMahon (rjmcmahon@rjmcmahon.com, bob.mcmahon@broadcom.com)
 * -------------------------------------------------------------------
 */
#include "headers.h"
#include "Timestamp.hpp"
#include "isochronous.hpp"
#include "delay.h"

#define BILLION 1000000000

using namespace Isochronous;

FrameCounter::FrameCounter (double value, const Timestamp& start) : frequency(value) {
    period = static_cast<unsigned int>(1000000 / frequency);
    startTime = start;
    nextslotTime = start;
    lastcounter = 0;
    slot_counter = 0;
    slip = 0;
}
FrameCounter::FrameCounter (double value) : frequency(value) {
#ifdef WIN32
    /* Create timer */
    my_timer = CreateWaitableTimer(NULL, TRUE, NULL);
    if (!SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_TIME_CRITICAL))
	WARN_errno(1, "SetThreadPriority");
#endif
    startTime.setnow();
    nextslotTime = startTime;
    period = static_cast<unsigned int>(1000000 / frequency); // unit us
    lastcounter = 0;
    slot_counter = 0;
    slip = 0;
}


FrameCounter::~FrameCounter () {
#ifdef WIN32
    /* Clean resources */
    if (my_timer)
	CloseHandle(my_timer);
#endif
}

#ifdef WIN32
/* Windows sleep in 100ns units returns 0 on success as does clock_nanosleep*/
int FrameCounter::mySetWaitableTimer (long delay_time) {
    int rc = -1;
    if (!my_timer) {
	if ((my_timer = CreateWaitableTimer(NULL, TRUE, NULL))) {
	    /* Set timer properties */
	    delay.QuadPart = -delay_time;
	} else {
	    WARN_errno(1, "CreateWaitable");
	    my_timer = NULL;
	}
    }
    if (my_timer) {
	/* Set timer properties */
	/* negative values are relative, positive absolute UTC time */
	delay.QuadPart = -delay_time;
	if(!SetWaitableTimer(my_timer, &delay, 0, NULL, NULL, FALSE)) {
	    WARN_errno(1, "SetWaitableTimer");
	    CloseHandle(my_timer);
	    my_timer = NULL;
	} else {
	    // Wait for timer
	    if (WaitForSingleObject(my_timer, INFINITE)) {
		WARN_errno(1, "WaitForSingleObject");
	    } else {
		rc = 0;
	    }
	}
    }
    return rc;
}
#endif

#if HAVE_CLOCK_NANOSLEEP
unsigned int FrameCounter::wait_tick (long *sched_err, bool sync_strict) {
    Timestamp now;
    int rc = true;
    if (!slot_counter) {
	slot_counter = 1;
	nextslotTime = now;
	startTime = now;
    } else {
	nextslotTime.add(period);
	slot_counter++;
	while (now.subUsec(nextslotTime) > static_cast<long>(sync_strict ? 0 : period)) {
	    nextslotTime.add(period);
	    slot_counter++;
	}
        if (lastcounter && ((slot_counter - lastcounter) > 1)) {
	    slip++;
        }
    }
  #ifndef WIN32
    timespec txtime_ts;
    txtime_ts.tv_sec = nextslotTime.getSecs();
    txtime_ts.tv_nsec = nextslotTime.getUsecs() * 1000;
    rc = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &txtime_ts, NULL);
  #else
    long duration = nextslotTime.subUsec(now);
    rc = mySetWaitableTimer(10 * duration); // convert us to 100 ns
    //int rc = clock_nanosleep(0, TIMER_ABSTIME, &txtime_ts, NULL);
  #endif
    if (sched_err) {
        // delay_loop(2020);
	Timestamp actual;
	*sched_err = actual.subUsec(nextslotTime);
//	printf("**** sched err %ld\n", *sched_err);
	if (*sched_err < 0) {
	    *sched_err = -(*sched_err); // err is an absolute value
            // Per windows docs, this timer can go off early per:
            // APIs that deal with timers use various different hardware clocks. These clocks may have resolutions
	    // significantly different from what you expect: some may be measured in milliseconds (for those that
	    // use an RTC-based timer chip), to those measured in nanoseconds (for those that use ACPI or TSC counters).
	    // You can change the resolution of your API with a call to the timeBeginPeriod and timeEndPeriod functions.
	    // How precise you can change the resolution depends on which hardware clock the particular API uses.
	    // For more information, check your hardware documentation.
            //
	    // I noticed the timer going off up to 8 ms early on a Windows 11 cross compile - yikes.
	    // Do a WAR hack here to add delay if & when that occurs
#ifdef WIN32
	    if (*sched_err > 1000) {
		delay_loop(*sched_err);
	    }
#endif
	}
    }
    WARN_errno((rc!=0), "wait_tick failed");
  #ifdef HAVE_THREAD_DEBUG
    // thread_debug("Client tick occurred per %ld.%06ld", txtime_ts.tv_sec, txtime_ts.tv_nsec / 1000);
  #endif
    lastcounter = slot_counter;
    return(slot_counter);
}
#else
unsigned int FrameCounter::wait_tick (long *sched_err, bool sync_strict) {
    Timestamp now;
    if (!slot_counter) {
	slot_counter = 1;
	startTime = now;
	nextslotTime = now;
    } else {
        long remaining;
	nextslotTime.add(period);
	slot_counter++;
	while (now.subUsec(nextslotTime) > (sync_strict ? 0 : period)) {
	    nextslotTime.add(period);
	    slot_counter++;
	}
//	printf("**** sync strict %d now %ld.%06ld next %ld.%06ld\n", sync_strict, now.getSecs(), now.getUsecs(), nextslotTime.getSecs(), nextslotTime.getUsecs());
	if (now.before(nextslotTime)) {
	    struct timespec tv0={0,0}, tv1;
	    get(&remaining);
	    remaining *= 1000; // convert to nano seconds
	    tv0.tv_sec = (remaining / BILLION);
	    tv0.tv_nsec += (remaining % BILLION);
	    if (tv0.tv_nsec >= BILLION) {
	        tv0.tv_sec++;
		tv0.tv_nsec -= BILLION;
	    }
//	    printf("**** wait: nanos %ld remain %ld.%06ld\n", remaining, tv0.tv_sec, tv0.tv_nsec);
	    int rc = nanosleep(&tv0, &tv1);
	    if (sched_err) {
	        Timestamp actual;
		*sched_err = actual.subUsec(nextslotTime);
		//	printf("**** slot %ld.%06ld actual %ld.%06ld %ld\n", slotstart.getSecs(), slotstart.getUsecs(), actual.getSecs(), actual.getUsecs(), *sched_err);
	    }
	    WARN_errno((rc != 0), "nanosleep wait_tick");
	}
    }
    return(slot_counter);
}
#endif
inline unsigned int FrameCounter::get () const {
    Timestamp now;
    return slot_counter + 1;
}

inline unsigned int FrameCounter::get (const Timestamp& slot) const {
    return(slot_counter + 1); // Frame counter for packets starts at 1
}

unsigned int FrameCounter::get (long *ticks_remaining) {
    assert(ticks_remaining != NULL);
    Timestamp sampleTime;  // Constructor will initialize timestamp to now
    long usecs = -startTime.subUsec(sampleTime);
    unsigned int counter = static_cast<unsigned int>(usecs / period) + 1;
    // figure out how many usecs before the next frame counter tick
    // the caller can use this to delay until the next tick
    *ticks_remaining = (counter * period) - usecs;
    return(counter); // Frame counter for packets starts at 1
}

inline Timestamp FrameCounter::next_slot () {
    Timestamp next = startTime;
    slot_counter = get();
    // period unit is in microseconds, convert to seconds
    next.add(slot_counter * (period / 1e6));
    return next;
}

unsigned int FrameCounter::period_us () {
    return(period);
}

void FrameCounter::reset () {
    period = (1000000 / frequency);
    startTime.setnow();
}

unsigned int FrameCounter::wait_sync (long sec, long usec) {
    long remaining;
    unsigned int framecounter;
    startTime.set(sec, usec);
    framecounter = get(&remaining);
    delay_loop(remaining);
    reset();
    framecounter = 1;
    lastcounter = 1;
    return(framecounter);
}

long FrameCounter::getSecs () {
    return startTime.getSecs();
}

long FrameCounter::getUsecs () {
    return startTime.getUsecs();
}
