/*---------------------------------------------------------------
 * Copyright (c) 1999,2000,2001,2002,2003
 * The Board of Trustees of the University of Illinois
 * All Rights Reserved.
 *---------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software (Iperf) and associated
 * documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
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
 * Neither the names of the University of Illinois, NCSA,
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
 * National Laboratory for Applied Network Research
 * National Center for Supercomputing Applications
 * University of Illinois at Urbana-Champaign
 * http://www.ncsa.uiuc.edu
 * ________________________________________________________________
 *
 * Reporter.c
 * by Kevin Gibbs <kgibbs@nlanr.net>
 *
 * Major rewrite by Robert McMahon (Sept 2020, ver 2.0.14)
 * ________________________________________________________________ */

#include <math.h>
#include "headers.h"
#include "Settings.hpp"
#include "util.h"
#include "Reporter.h"
#include "Thread.h"
#include "Locale.h"
#include "PerfSocket.hpp"
#include "SocketAddr.h"
#include "histogram.h"
#include "delay.h"
#include "packet_ring.h"
#include "payloads.h"
#include "iperf_formattime.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef INITIAL_PACKETID
# define INITIAL_PACKETID 0
#endif

struct ReportHeader *ReportRoot = NULL;
struct ReportHeader *ReportPendingHead = NULL;
struct ReportHeader *ReportPendingTail = NULL;

// Reporter's reset of stats after a print occurs
static void reporter_reset_transfer_stats_client_tcp(struct TransferInfo *stats);
static void reporter_reset_transfer_stats_client_udp(struct TransferInfo *stats);
static void reporter_reset_transfer_stats_server_udp(struct TransferInfo *stats);
static void reporter_reset_transfer_stats_server_tcp(struct TransferInfo *stats);
static void reporter_reset_transfer_stats_sum(struct TransferInfo *stats);

// code for welfornd's algorithm to produce running mean/min/max/var
static void reporter_update_mmm (struct MeanMinMaxStats *stats, double value);
static void reporter_reset_mmm (struct MeanMinMaxStats *stats);
static void reporter_update_mmm_sum (struct MeanMinMaxStats *sumstats, struct MeanMinMaxStats *stats);

// one way delay (OWD) calculations
static void reporter_handle_packet_oneway_transit(struct TransferInfo *stats, struct ReportStruct *packet);
static void reporter_handle_isoch_oneway_transit_tcp(struct TransferInfo *stats, struct ReportStruct *packet);
static void reporter_handle_isoch_oneway_transit_udp(struct TransferInfo *stats, struct ReportStruct *packet);
static void reporter_handle_frame_isoch_oneway_transit(struct TransferInfo *stats, struct ReportStruct *packet);
static void reporter_handle_txmsg_oneway_transit(struct TransferInfo *stats, struct ReportStruct *packet);
static void reporter_handle_rxmsg_oneway_transit(struct TransferInfo *stats, struct ReportStruct *packet);

static inline void reporter_compute_packet_pps (struct TransferInfo *stats, struct ReportStruct *packet);

#if HAVE_TCP_STATS
static inline void reporter_handle_packet_tcpistats(struct ReporterData *data, struct ReportStruct *packet);
#endif
static struct ConnectionInfo *myConnectionReport;

void PostReport (struct ReportHeader *reporthdr) {
#ifdef HAVE_THREAD_DEBUG
    char rs[REPORTTXTMAX];
    reporttype_text(reporthdr, &rs[0]);
    thread_debug("Jobq *POST* report %p (%s)", reporthdr, &rs[0]);
#endif
    if (reporthdr) {
#ifdef HAVE_THREAD
	/*
	 * Update the ReportRoot to include this report.
	 */
	Condition_Lock(ReportCond);
	reporthdr->next = NULL;
	if (!ReportPendingHead) {
	  ReportPendingHead = reporthdr;
	  ReportPendingTail = reporthdr;
	} else {
	  ReportPendingTail->next = reporthdr;
	  ReportPendingTail = reporthdr;
	}
	Condition_Unlock(ReportCond);
	// wake up the reporter thread
	Condition_Signal(&ReportCond);
#else
	/*
	 * Process the report in this thread
	 */
	reporthdr->next = NULL;
	reporter_process_report(reporthdr);
#endif
    }
}
/*
 * ReportPacket is called by a transfer agent to record
 * the arrival or departure of a "packet" (for TCP it
 * will actually represent many packets). This needs to
 * be as simple and fast as possible as it gets called for
 * every "packet".
 *
 * Returns true when the tcpinfo was sampled, false ohterwise
 */
void ReportPacket (struct ReporterData* data, struct ReportStruct *packet) {
    assert(data != NULL);
#ifdef HAVE_THREAD_DEBUG
    if (packet->packetID < 0) {
	thread_debug("Reporting last packet for %p  qdepth=%d sock=%d", (void *) data, packetring_getcount(data->packetring), data->info.common->socket);
    }
#endif
    packet->tcpstats.needTcpInfoSample = false;
#if HAVE_TCP_STATS
    struct TransferInfo *stats = &data->info;
    if (stats->isEnableTcpInfo && !TimeZero(stats->ts.nextTCPSampleTime)) {
	double td = TimeDifference(stats->ts.nextTCPSampleTime, packet->packetTime);
	if (td < (TimeDouble(stats->ts.intervalTime) / 2)) {
	    TimeAdd(stats->ts.nextTCPSampleTime, stats->ts.intervalTime);
	    packet->tcpstats.needTcpInfoSample = true;
	} else if ((td < 0) && packet->tcpstats.needTcpInfoSample) {
	    gettcpinfo(data->info.common->socket, &packet->tcpstats);
	    packet->tcpstats.needTcpInfoSample = false;
	}
    }
#endif
    // Note for threaded operation all that needs
    // to be done is to enqueue the packet data
    // into the ring.
    packetring_enqueue(data->packetring, packet);
    // The traffic thread calls the reporting process
    // directly forr non-threaded operation
    // These defeats the puropse of separating
    // traffic i/o from user i/o and really
    // should be avoided.
#ifdef HAVE_THREAD
    // bypass the reporter thread here for single UDP
    if (isSingleUDP(data->info.common))
        reporter_process_transfer_report(data);
#else
    /*
     * Process the report in this thread
     */
    reporter_process_transfer_report(data);
#endif
}

/*
 * EndJob is called by a traffic thread to inform the reporter
 * thread to print a final report and to remove the data report from its jobq.
 * It also handles the freeing reports and other closing actions
 */
bool EndJob (struct ReportHeader *reporthdr, struct ReportStruct *finalpacket) {
    assert(reporthdr!=NULL);
    assert(finalpacket!=NULL);
    struct ReporterData *report = (struct ReporterData *) reporthdr->this_report;
    struct ReportStruct packet;

    memset(&packet, 0, sizeof(struct ReportStruct));
    bool do_close = true;
    /*
     * Using PacketID of -1 ends reporting
     * It pushes a "special packet" through
     * the packet ring which will be detected
     * by the reporter thread as and end of traffic
     * event
     */
#if HAVE_TCP_STATS
    // tcpi stats are sampled on a final packet
    struct TransferInfo *stats = &report->info;
    if (stats->isEnableTcpInfo) {
	gettcpinfo(report->info.common->socket, &finalpacket->tcpstats);
    }
#endif
    // clear the reporter done predicate
    report->packetring->consumerdone = false;
    // the negative packetID is used to inform the report thread this traffic thread is done
    packet.packetID = -1;
    packet.packetLen = finalpacket->packetLen;
    packet.packetTime = finalpacket->packetTime;
    packet.err_readwrite = NullEvent; // this is not a real event
    if (isSingleUDP(report->info.common)) {
	packetring_enqueue(report->packetring, &packet);
	reporter_process_transfer_report(report);
    } else {
	ReportPacket(report, &packet);
	Condition_Lock((*(report->packetring->awake_producer)));
	while (!report->packetring->consumerdone) {
	    // This wait time is the lag between the reporter thread
	    // and the traffic thread, a reporter thread with lots of
	    // reports (e.g. fastsampling) can lag per the i/o
#ifdef HAVE_THREAD_DEBUG
	    thread_debug( "Traffic thread awaiting reporter to be done with %p and cond %p", (void *)report, (void *) report->packetring->awake_producer);
#endif
	    Condition_TimedWait(report->packetring->awake_producer, 1);
	    // printf("Consumer done may be stuck\n");
	}
	Condition_Unlock((*(report->packetring->awake_producer)));
    }
    if (report->FullDuplexReport && isFullDuplex(report->FullDuplexReport->info.common)) {
	if (fullduplex_stop_barrier(&report->FullDuplexReport->fullduplex_barrier)) {
	    struct Condition *tmp = &report->FullDuplexReport->fullduplex_barrier.await;
	    Condition_Destroy(tmp);
#if HAVE_THREAD_DEBUG
	    thread_debug("Socket fullduplex close sock=%d", report->FullDuplexReport->info.common->socket);
#endif
	    FreeSumReport(report->FullDuplexReport);
	} else {
	    do_close = false;
	}
    }
    return do_close;
}

//  This is used to determine the packet/cpu load into the reporter thread
//  If the overall reporter load is too low, add some yield
//  or delay so the traffic threads can fill the packet rings
#define MINPACKETDEPTH 10
#define MINPERQUEUEDEPTH 20
#define REPORTERDELAY_DURATION 16000 // units is microseconds
struct ConsumptionDetectorType {
    int accounted_packets;
    int accounted_packet_threads;
    int reporter_thread_suspends ;
};
struct ConsumptionDetectorType consumption_detector = \
  {.accounted_packets = 0, .accounted_packet_threads = 0, .reporter_thread_suspends = 0};

static inline void reset_consumption_detector (void) {
    consumption_detector.accounted_packet_threads = thread_numtrafficthreads();
    if ((consumption_detector.accounted_packets = thread_numtrafficthreads() * MINPERQUEUEDEPTH) <= MINPACKETDEPTH) {
	consumption_detector.accounted_packets = MINPACKETDEPTH;
    }
}
static inline void apply_consumption_detector (void) {
    if (--consumption_detector.accounted_packet_threads <= 0) {
	// All active threads have been processed for the loop,
	// reset the thread counter and check the consumption rate
	// If the rate is too low add some delay to the reporter
	consumption_detector.accounted_packet_threads = thread_numtrafficthreads();
	// Check to see if we need to suspend the reporter
	if (consumption_detector.accounted_packets > 0) {
	    /*
	     * Suspend the reporter thread for some (e.g. 4) milliseconds
	     *
	     * This allows the thread to receive client or server threads'
	     * packet events in "aggregates."  This can reduce context
	     * switching allowing for better CPU utilization,
	     * which is very noticble on CPU constrained systems.
	     */
	    delay_loop(REPORTERDELAY_DURATION);
	    consumption_detector.reporter_thread_suspends++;
	    // printf("DEBUG: forced reporter suspend, accounted=%d,  queueue depth after = %d\n", accounted_packets, getcount_packetring(reporthdr));
	} else {
	    // printf("DEBUG: no suspend, accounted=%d,  queueue depth after = %d\n", accounted_packets, getcount_packetring(reporthdr));
	}
	reset_consumption_detector();
    }
}

#ifdef HAVE_THREAD_DEBUG
static void reporter_jobq_dump(void) {
  thread_debug("reporter thread job queue request lock");
  Condition_Lock(ReportCond);
  struct ReportHeader *itr = ReportRoot;
  while (itr) {
    thread_debug("Job in queue %p",(void *) itr);
    itr = itr->next;
  }
  Condition_Unlock(ReportCond);
  thread_debug("reporter thread job queue unlock");
}
#endif


/* Concatenate pending reports and return the head */
static inline struct ReportHeader *reporter_jobq_set_root (struct thread_Settings *inSettings) {
    struct ReportHeader *root = NULL;
    Condition_Lock(ReportCond);
    // check the jobq for empty
    if (ReportRoot == NULL) {
	sInterupted = 0; // reset flags in reporter thread emtpy context
	// The reporter is starting from an empty state
	// so set the load detect to trigger an initial delay
        if (!isSingleUDP(inSettings)) {
	    reset_consumption_detector();
	    reporter_default_heading_flags((inSettings->mReportMode == kReport_CSV));
        }
	// Only hang the timed wait if more than this thread is active
	if (!ReportPendingHead && (thread_numuserthreads() > 1)) {
	    Condition_TimedWait(&ReportCond, 1);
#ifdef HAVE_THREAD_DEBUG
	    thread_debug( "Jobq *WAIT* exit  %p/%p cond=%p threads u/t=%d/%d", \
			  (void *) ReportRoot, (void *) ReportPendingHead, \
			  (void *) &ReportCond, thread_numuserthreads(), thread_numtrafficthreads());
#endif
	}
    }
    // update the jobq per pending reports
    if (ReportPendingHead) {
	ReportPendingTail->next = ReportRoot;
	ReportRoot = ReportPendingHead;
#ifdef HAVE_THREAD_DEBUG
	thread_debug( "Jobq *ROOT* %p (last=%p)", \
		      (void *) ReportRoot, (void * ) ReportPendingTail->next);
#endif
	ReportPendingHead = NULL;
	ReportPendingTail = NULL;
    }
    root = ReportRoot;
    Condition_Unlock(ReportCond);
    return root;
}
/*
 * Welford's online algorithm
 *
 * # For a new value newValue, compute the new count, new mean, the new M2.
 * # mean accumulates the mean of the entire dataset
 * # M2 aggregates the squared distance from the mean
 * # count aggregates the number of samples seen so far
 * def update(existingAggregate, newValue):
 *   (count, mean, M2) = existingAggregate
 *   count += 1
 *   delta = newValue - mean
 *   mean += delta / count
 *   delta2 = newValue - mean
 *   M2 += delta * delta2
 *   return (count, mean, M2)
 *
 * # Retrieve the mean, variance and sample variance from an aggregate
 * def finalize(existingAggregate):
 *   (count, mean, M2) = existingAggregate
 *   if count < 2:
 *       return float("nan")
 *   else:
 *       (mean, variance, sampleVariance) = (mean, M2 / count, M2 / (count - 1))
 *       return (mean, variance, sampleVariance)
 *
 */
static void reporter_update_mmm (struct MeanMinMaxStats *stats, double value) {
    assert(stats != NULL);
    stats->cnt++;
    if (stats->cnt == 1) {
	// Very first entry
	stats->min = value;
	stats->max = value;
	stats->sum = value;
	stats->vd = value;
	stats->mean = value;
	stats->m2 = 0;
	stats->sum = value;
    } else {
	stats->sum += value;
	stats->vd = value - stats->mean;
	stats->mean += (stats->vd / stats->cnt);
	stats->m2 += stats->vd * (value - stats->mean);
	// mean min max tests
	if (value < stats->min)
	    stats->min = value;
	if (value > stats->max)
	    stats->max = value;
    }
    // fprintf(stderr,"**** mmm(%d) val/sum=%f/%f mmm=%f/%f/%f/%f\n", stats->cnt, value, stats->sum, stats->mean, stats->min, stats->max, stats->m2);
}
static void reporter_reset_mmm (struct MeanMinMaxStats *stats) {
    stats->min = FLT_MAX;
    stats->max = FLT_MIN;
    stats->sum = 0;
    stats->vd = 0;
    stats->mean = 0;
    stats->m2 = 0;
    stats->cnt = 0;
};

static void reporter_update_mmm_sum (struct MeanMinMaxStats *sumstats, struct MeanMinMaxStats *stats) {
    assert(stats != NULL);
    stats->cnt++;
    if (sumstats->cnt == 0) {
	// Very first entry
	sumstats->min = stats->min;
	sumstats->max = stats->max;
	sumstats->sum = stats->sum;
	sumstats->cnt = stats->cnt;
    } else {
	if (stats->min < sumstats->min)
	    sumstats->min = stats->min;
	if (stats->max > sumstats->max)
	    sumstats->max = stats->max;
	sumstats->cnt += stats->cnt;
	sumstats->sum += stats->sum;
    }
    // fprintf(stderr,"**** mmm(%ld) sum/min/max=%f/%f/%f\n", sumstats->cnt, sumstats->sum, sumstats->min, sumstats->max);
}

/*
 * This function is the loop that the reporter thread processes
 */
void reporter_spawn (struct thread_Settings *thread) {
#ifdef HAVE_THREAD_DEBUG
    thread_debug( "Reporter thread started");
#endif
    if (isConnectOnly(thread)) {
	myConnectionReport = InitConnectOnlyReport(thread);
    }
    /*
     * reporter main loop needs to wait on all threads being started
     */
    Condition_Lock(threads_start.await);
    while (!threads_start.ready) {
	Condition_TimedWait(&threads_start.await, 1);
    }
    Condition_Unlock(threads_start.await);
#ifdef HAVE_THREAD_DEBUG
    thread_debug( "Reporter await done");
#endif

    //
    // Signal to other (client) threads that the
    // reporter is now running.
    //
    Condition_Lock(reporter_state.await);
    reporter_state.ready = 1;
    Condition_Unlock(reporter_state.await);
    Condition_Broadcast(&reporter_state.await);
#if HAVE_SCHED_SETSCHEDULER
    // set reporter thread to realtime if requested
    thread_setscheduler(thread);
#endif
    /*
     * Keep the reporter thread alive under the following conditions
     *
     * o) There are more reports to output, ReportRoot has a report
     * o) The number of threads is greater than one which indicates
     *    either traffic threads are still running or a Listener thread
     *    is running. If equal to 1 then only the reporter thread is alive
     */
    while ((reporter_jobq_set_root(thread) != NULL) || (thread_numuserthreads() > 1)){
#ifdef HAVE_THREAD_DEBUG
	// thread_debug( "Jobq *HEAD* %p (%d)", (void *) ReportRoot, thread_numuserthreads());
#endif
	if (ReportRoot) {
	    // https://blog.kloetzl.info/beautiful-code/
	    // Linked list removal/processing is derived from:
	    //
	    // remove_list_entry(entry) {
	    //     indirect = &head;
	    //     while ((*indirect) != entry) {
	    //	       indirect = &(*indirect)->next;
	    //     }
	    //     *indirect = entry->next
	    // }
	    struct ReportHeader **work_item = &ReportRoot;
	    while (*work_item) {
#ifdef HAVE_THREAD_DEBUG
		// thread_debug( "Jobq *NEXT* %p", (void *) *work_item);
#endif
		// Report process report returns true
		// when a report needs to be removed
		// from the jobq.  Also, work item might
		// be free as part of its processing
		// Store a cached pointer tmp
		// for the next work item
		struct ReportHeader *tmp = (*work_item)->next;
	        if (reporter_process_report(*work_item)) {
#ifdef HAVE_THREAD_DEBUG
		  thread_debug("Jobq *REMOVE* %p", (void *) (*work_item));
#endif
		    // memory for *work_item is gone by now
		    *work_item = tmp;
		    if (!tmp)
			break;
		}
		work_item = &(*work_item)->next;
	    }
	}
    }
    if (myConnectionReport) {
	if (myConnectionReport->connect_times.cnt > 1) {
	    reporter_connect_printf_tcp_final(myConnectionReport);
	}
	FreeConnectionReport(myConnectionReport);
    }
#ifdef HAVE_THREAD_DEBUG
    if (sInterupted)
        reporter_jobq_dump();
    thread_debug("Reporter thread finished user/traffic %d/%d", thread_numuserthreads(), thread_numtrafficthreads());
#endif
}

// The Transfer or Data report is by far the most complicated report
bool reporter_process_transfer_report (struct ReporterData *this_ireport) {
    assert(this_ireport != NULL);
    struct TransferInfo *sumstats = (this_ireport->GroupSumReport ? &this_ireport->GroupSumReport->info : NULL);
    struct TransferInfo *fullduplexstats = (this_ireport->FullDuplexReport ? &this_ireport->FullDuplexReport->info : NULL);
    bool need_free = false;
    // The consumption detector applies delay to the reporter
    // thread when its consumption rate is too low.   This allows
    // the traffic threads to send aggregates vs thrash
    // the packet rings.  The dissimilarity between the thread
    // speeds is due to the performance differences between i/o
    // bound threads vs cpu bound ones, and it's expected
    // that reporter thread being CPU limited should be much
    // faster than the traffic threads, even in aggregate.
    // Note: If this detection is not going off it means
    // the system is likely CPU bound and iperf is now likely
    // becoming a CPU bound test vs a network i/o bound test
    if (!isSingleUDP(this_ireport->info.common))
	apply_consumption_detector();
    // If there are more packets to process then handle them
    struct ReportStruct *packet = NULL;
    bool advance_jobq = false;
    while (!advance_jobq && (packet = packetring_dequeue(this_ireport->packetring))) {
	// Increment the total packet count processed by this thread
	// this will be used to make decisions on if the reporter
	// thread should add some delay to eliminate cpu thread
	// thrashing,
	consumption_detector.accounted_packets--;
	// Check against a final packet event on this packet ring
#if HAVE_TCP_STATS
	if (this_ireport->info.isEnableTcpInfo && packet->tcpstats.isValid) {
	    reporter_handle_packet_tcpistats(this_ireport, packet);
	}
#endif
#if (HAVE_DECL_SO_MAX_PACING_RATE)
	if (isFQPacing(this_ireport->info.common))
	    this_ireport->info.FQPacingRateCurrent = packet->FQPacingRate;
#endif
	if (this_ireport->transfer_interval_handler) {
	    if (sumstats && (this_ireport->packetring->uplevel != sumstats->uplevel) \
		&& (TimeDifference(sumstats->ts.nextTime, packet->packetTime) >= 0)) {
		sumstats->slot_thread_upcount++;
#if HAVE_SUMMING_DEBUG
		printf("**** %s upcnt   (%p) pkt=%ld.%06ld (up/down)=%d/%d uplevel (sum/pkt)=%d/%d\n", this_ireport->info.common->transferIDStr, (void *)this_ireport->packetring, \
		       (long) packet->packetTime.tv_sec, (long) packet->packetTime.tv_usec, sumstats->slot_thread_upcount, sumstats->slot_thread_downcount, \
		       sumstats->uplevel, this_ireport->packetring->uplevel);
#endif
		this_ireport->packetring->uplevel = toggleLevel(this_ireport->packetring->uplevel);
	    }
	}
	if (!(packet->packetID < 0)) {
	    // Check to output any interval reports,
	    // bursts need to report the packet first
	    if (this_ireport->packet_handler_pre_report) {
		(*this_ireport->packet_handler_pre_report)(this_ireport, packet);
	    }
	    if (this_ireport->transfer_interval_handler) {
		advance_jobq = (*this_ireport->transfer_interval_handler)(this_ireport, packet);
	    }
	    if (this_ireport->packet_handler_post_report) {
		(*this_ireport->packet_handler_post_report)(this_ireport, packet);
	    }
	    // Sum reports update the report header's last
	    // packet time after the handler. This means
	    // the report header's packet time will be
	    // the previous time before the interval
	    if (sumstats)
		sumstats->ts.packetTime = packet->packetTime;
	    if (fullduplexstats)
		fullduplexstats->ts.packetTime = packet->packetTime;
	} else {
	    need_free = true;
	    advance_jobq = true;
	    // A last packet event was detected
	    // printf("last packet event detected\n"); fflush(stdout);
	    this_ireport->reporter_thread_suspends = consumption_detector.reporter_thread_suspends;
	    if (this_ireport->packet_handler_pre_report) {
		(*this_ireport->packet_handler_pre_report)(this_ireport, packet);
	    }
	    if (this_ireport->packet_handler_post_report) {
		(*this_ireport->packet_handler_post_report)(this_ireport, packet);
	    }
	    this_ireport->info.ts.packetTime = packet->packetTime;
	    assert(this_ireport->transfer_protocol_handler != NULL);
	    (*this_ireport->transfer_protocol_handler)(this_ireport, true);
	    // This is a final report so set the sum report header's packet time
	    // Note, the thread with the max value will set this
	    if (fullduplexstats && isEnhanced(this_ireport->info.common)) {
		// The largest packet timestamp sets the sum report final time
		if (TimeDifference(fullduplexstats->ts.packetTime, packet->packetTime) > 0) {
		    fullduplexstats->ts.packetTime = packet->packetTime;
		}
		if (DecrSumReportRefCounter(this_ireport->FullDuplexReport) == 0) {
		    if (this_ireport->FullDuplexReport->transfer_protocol_sum_handler) {
			(*this_ireport->FullDuplexReport->transfer_protocol_sum_handler)(fullduplexstats, true);
		    }
		    // FullDuplex report gets freed by a traffic thread (per its barrier)
		}
	    }
	    if (sumstats) {
		if (TimeDifference(sumstats->ts.packetTime, packet->packetTime) > 0) {
		    sumstats->ts.packetTime = packet->packetTime;
		}
		if (this_ireport->GroupSumReport->transfer_protocol_sum_handler) {
		    Mutex_Lock(&this_ireport->GroupSumReport->reference.lock);
		    if ((++this_ireport->GroupSumReport->final_thread_upcount == this_ireport->GroupSumReport->reference.maxcount) && \
			((this_ireport->GroupSumReport->reference.maxcount > 1) || isSumOnly(this_ireport->info.common))) {
			(*this_ireport->GroupSumReport->transfer_protocol_sum_handler)(&this_ireport->GroupSumReport->info, true);
		    }
		    Mutex_Unlock(&this_ireport->GroupSumReport->reference.lock);
		}
	    }
	}
    }
    return need_free;
}
/*
 * Process reports
 *
 * Make notice here, the reporter thread is freeing most reports, traffic threads
 * can't use them anymore (except for the DATA REPORT);
 *
 */
inline bool reporter_process_report (struct ReportHeader *reporthdr) {
    assert(reporthdr != NULL);
    bool done = true;
    switch (reporthdr->type) {
    case DATA_REPORT:
	done = reporter_process_transfer_report((struct ReporterData *)reporthdr->this_report);
	if (done) {
	    struct ReporterData *tmp = (struct ReporterData *)reporthdr->this_report;
	    struct PacketRing *pr = tmp->packetring;
	    pr->consumerdone = true;
# ifdef HAVE_THREAD_DEBUG
	    struct ReporterData *report = (struct ReporterData *) reporthdr->this_report;
	    thread_debug( "Reporter thread signal traffic thread %p %p", (void *)report, (void *) report->packetring->awake_producer);
#endif
	    // Data Reports are special because the traffic thread needs to free them, just signal
	    Condition_Signal(pr->awake_producer);
	}
	break;
    case CONNECTION_REPORT:
    {
	struct ConnectionInfo *creport = (struct ConnectionInfo *)reporthdr->this_report;
	assert(creport!=NULL);
	if (!isCompat(creport->common) && (creport->common->ThreadMode == kMode_Client) && myConnectionReport) {
	    // Clients' connect times will be inputs to the overall connect stats
	    if (creport->tcpinitstats.connecttime > 0.0) {
		reporter_update_mmm(&myConnectionReport->connect_times, creport->tcpinitstats.connecttime);
	    } else {
		myConnectionReport->connect_times.err++;
	    }
	}
	reporter_print_connection_report(creport);
	FreeReport(reporthdr);
    }
	break;
    case SETTINGS_REPORT:
	reporter_print_settings_report((struct ReportSettings *)reporthdr->this_report);
	FreeReport(reporthdr);
	break;
    case SERVER_RELAY_REPORT:
	reporter_print_server_relay_report((struct ServerRelay *)reporthdr->this_report);
	FreeReport(reporthdr);
	break;
    case STRING_REPORT:
        if (reporthdr->this_report) {
	    printf("%s\n", (char *)reporthdr->this_report);
	    free((char *)reporthdr->this_report);
	}
	break;
    case ERROR_REPORT:
        if (reporthdr->this_report) {
	    fprintf(stderr, "%s\n", (char *)reporthdr->this_report);
	    free((char *)reporthdr->this_report);
	}
	break;
    default:
	fprintf(stderr,"Invalid report type in process report %p\n", reporthdr->this_report);
	assert(0);
	break;
    }
#ifdef HAVE_THREAD_DEBUG
    // thread_debug("Processed report %p type=%d", (void *)reporthdr, reporthdr->report.type);
#endif
    return done;
}

/*
 * Updates connection stats
 */
#define L2DROPFILTERCOUNTER 100

// Reporter private routines
void reporter_handle_packet_null (struct ReporterData *data, struct ReportStruct *packet) {
}
void reporter_transfer_protocol_null (struct ReporterData *data, bool final){
}

#define DEBUG_PPS 0
static inline void reporter_compute_packet_pps (struct TransferInfo *stats, struct ReportStruct *packet) {
    if (!packet->emptyreport) {
        stats->total.Datagrams.current++;
        stats->total.IPG.current++;
    }
    if ((stats->IPGsum == 0) && !TimeZero(stats->ts.prevTime)) {
	// printf("*** cross interval %f\n", TimeDifference(packet->packetTime, stats->ts.prevTime));
	stats->IPGsum += TimeDifference(packet->packetTime, stats->ts.prevTime);
	stats->ts.prevTime = stats->ts.nextTime;
    } else if (TimeZero(stats->ts.prevTime)) {
	// printf("*** start %f\n", TimeDifference(packet->packetTime, stats->ts.startTime));
	stats->IPGsum += TimeDifference(packet->packetTime, stats->ts.startTime);
	if (TimeDifference(packet->packetTime, stats->ts.startTime) > 0) {
	    stats->ts.prevTime = stats->ts.startTime;
	} else {
	    stats->ts.prevTime = packet->packetTime;
	}
    } else {
	// printf("*** within interval %f\n",TimeDifference(packet->packetTime, packet->prevPacketTime));
	stats->IPGsum += TimeDifference(packet->packetTime, packet->prevPacketTime);
    }
    stats->IPGsumcarry = TimeDifference(stats->ts.nextTime, packet->packetTime);
#if DEBUG_PPS
    printf("*** IPGsum = %f cnt=%ld ipg=%ld.%06ld pkt=%ld.%06ld id=%ld empty=%d transit=%f prev=%ld.%06ld carry %f\n", stats->IPGsum, stats->total.IPG.current, stats->ts.IPGstart.tv_sec, stats->ts.IPGstart.tv_usec, packet->packetTime.tv_sec, packet->packetTime.tv_usec, packet->packetID, packet->emptyreport, TimeDifference(packet->packetTime, packet->prevPacketTime), packet->prevPacketTime.tv_sec, packet->prevPacketTime.tv_usec, stats->IPGsumcarry);
#endif
}

static void reporter_handle_packet_oneway_transit (struct TransferInfo *stats, struct ReportStruct *packet) {
    // Transit or latency updates done inline below
    double transit = TimeDifference(packet->packetTime, packet->sentTime);
    if (stats->latency_histogram) {
        histogram_insert(stats->latency_histogram, transit, &packet->packetTime);
    }
    double deltaTransit;
    deltaTransit = transit - stats->transit.current.last;
    stats->transit.current.last = transit; // shift transit for next time
    if (deltaTransit < 0.0) {
	deltaTransit = -deltaTransit;
    }
    // Compute end/end delay stats
    reporter_update_mmm(&stats->transit.total, transit);
    reporter_update_mmm(&stats->transit.current, transit);
    //
    // Compute jitter though filter the case of isoch and between isoch frames
    // or, in other words, only calculate jitter for packets within the same isoch frame
    //
    // Taken from RFC 1889, Real Time Protocol (RTP)
    // J = J + ( | D(i-1,i) | - J ) /
    //
    // interarrival jitter:
    //
    // An estimate of the statistical variance of the RTP data packet
    // interarrival time, measured in timestamp units and expressed as
    // an unsigned integer. The interarrival jitter J is defined to be
    // the mean deviation (smoothed absolute value) of the difference D
    // in packet spacing at the receiver compared to the sender for a
    // pair of packets. As shown in the equation below, this is
    // equivalent to the difference in the "relative transit time" for
    // the two packets; the relative transit time is the difference
    // between a packet's RTP timestamp and the receiver's clock at the
    // time of arrival, measured in the same units.
    //
    // If Si is the RTP timestamp from packet i, and Ri is the time of
    // arrival in RTP timestamp units for packet i, then for two packets i
    // and j, D may be expressed as
    //
    //             D(i,j)=(Rj-Ri)-(Sj-Si)=(Rj-Sj)-(Ri-Si)
    //
    // The interarrival jitter is calculated continuously as each data
    // packet i is received from source SSRC_n, using this difference D for
    // that packet and the previous packet i-1 in order of arrival (not
    // necessarily in sequence), according to the formula
    //
    //              J=J+(|D(i-1,i)|-J)/16
    //
    // Whenever a reception report is issued, the current value of J is
    // sampled. This algorithm is the optimal first-order estimator and
    // the gain parameter 1/16 gives a good noise reduction ratio while /
    // maintaining a reasonable rate of convergence
    //
    if (isIsochronous(stats->common) && stats->isochstats.newburst) {
        --stats->isochstats.newburst; // decr the burst counter, need for RTP estimator w/isoch
	//	printf("**** skip value %f per frame change packet %d expected %d max = %f %d\n", deltaTransit, packet->frameID, stats->isochstats.frameID, stats->inline_jitter.total.max, stats->isochstats.newburst);
    } else if (stats->transit.total.cnt > 1) {
	stats->jitter += (deltaTransit - stats->jitter) / (16.0);
	reporter_update_mmm(&stats->inline_jitter.total, stats->jitter);
	reporter_update_mmm(&stats->inline_jitter.current, stats->jitter);
	if (stats->jitter_histogram) {
	    histogram_insert(stats->jitter_histogram, deltaTransit, NULL);
	}
    }
}

static void reporter_handle_isoch_oneway_transit_tcp (struct TransferInfo *stats, struct ReportStruct *packet) {
    // printf("fid=%lu bs=%lu remain=%lu\n", packet->frameID, packet->burstsize, packet->remaining);
    if (packet->frameID && packet->transit_ready) {
	int framedelta = 0;
	double frametransit = 0;
	// very first isochronous frame
	if (!stats->isochstats.frameID) {
	    stats->isochstats.framecnt.current=packet->frameID;
	    stats->isochstats.newburst = 0; // no packet/read filtering of early samples for TCP
	}
	// perform client and server frame based accounting
	if ((framedelta = (packet->frameID - stats->isochstats.frameID))) {
	    stats->isochstats.framecnt.current++;
	    if (framedelta > 1) {
		stats->isochstats.framelostcnt.current += (framedelta-1);
		stats->isochstats.slipcnt.current++;
	    } else if (stats->common->ThreadMode == kMode_Server) {
		// Triptimes use the frame start time in passed in the frame header while
		// it's calculated from the very first start time and frame id w/o trip timees
		if (isTripTime(stats->common)) {
		    frametransit = TimeDifference(packet->packetTime, packet->isochStartTime);
		} else {
		    frametransit = TimeDifference(packet->packetTime, packet->isochStartTime) \
			- ((packet->burstperiod * (packet->frameID - 1)) / 1e6);
		}
		reporter_update_mmm(&stats->isochstats.transit.total, frametransit);
		reporter_update_mmm(&stats->isochstats.transit.current, frametransit);
		if (stats->framelatency_histogram) {
		    histogram_insert(stats->framelatency_histogram, frametransit, &packet->packetTime);
		}
	    }
	}
	stats->isochstats.frameID = packet->frameID;
    }
}

static void reporter_handle_isoch_oneway_transit_udp (struct TransferInfo *stats, struct ReportStruct *packet) {
    // printf("fid=%lu bs=%lu remain=%lu\n", packet->frameID, packet->burstsize, packet->remaining);
    if (packet->frameID && packet->transit_ready) {
	int framedelta = 0;
	double frametransit = 0;
	// very first isochronous frame
	if (!stats->isochstats.frameID) {
	    stats->isochstats.framecnt.current=1;
	}
	// perform client and server frame based accounting
	framedelta = (packet->frameID - stats->isochstats.frameID);
	stats->isochstats.framecnt.current++;
//      stats->matchframeID = packet->frameID + 1;
	if (framedelta == 1) {
	    stats->isochstats.newburst = 2; // set to two per RTP's pair calculation
	    // Triptimes use the frame start time in passed in the frame header while
	    // it's calculated from the very first start time and frame id w/o trip timees
	    frametransit = TimeDifference(packet->packetTime, packet->isochStartTime) \
		- ((packet->burstperiod * (packet->frameID - 1)) / 1e6);
	    reporter_update_mmm(&stats->isochstats.transit.total, frametransit);
	    reporter_update_mmm(&stats->isochstats.transit.current, frametransit);
	    if (stats->framelatency_histogram) {
		histogram_insert(stats->framelatency_histogram, frametransit, &packet->packetTime);
	    }
	} else if (framedelta > 1) {
	    stats->isochstats.newburst = 2; // set to two per RTP's pair calculation
	    if (stats->common->ThreadMode == kMode_Server) {
		stats->isochstats.framelostcnt.current += framedelta;
	    } else {
		stats->isochstats.framelostcnt.current += framedelta;
		stats->isochstats.slipcnt.current++;
	    }
	}
	stats->isochstats.frameID = packet->frameID;
    }
}


static void reporter_handle_rxmsg_oneway_transit (struct TransferInfo *stats, struct ReportStruct *packet) {
    // very first burst
    if (!stats->isochstats.frameID) {
	stats->isochstats.frameID = packet->frameID;
    }
    if (packet->frameID && packet->transit_ready) {
	double transit = TimeDifference(packet->packetTime, packet->sentTime);
//	printf("**** r pt %ld.%06ld st %ld.%06ld %f\n", packet->packetTime.tv_sec, packet->packetTime.tv_usec, packet->sentTime.tv_sec, packet->sentTime.tv_usec, transit);
	reporter_update_mmm(&stats->transit.total, transit);
	reporter_update_mmm(&stats->transit.current, transit);
	if (stats->framelatency_histogram) {
	    histogram_insert(stats->framelatency_histogram, transit, &packet->sentTime);
	}
	if (!TimeZero(stats->ts.prevpacketTime)) {
	    double delta = TimeDifference(packet->sentTime, stats->ts.prevpacketTime);
	    stats->IPGsum += delta;
	}
	stats->ts.prevpacketTime = packet->sentTime;
	stats->isochstats.frameID++;  // RJM fix this overload
	stats->burstid_transition = true;
    } else if (stats->burstid_transition && packet->frameID && (packet->frameID != stats->isochstats.frameID)) {
	stats->burstid_transition = false;
	fprintf(stderr,"%sError: expected burst id %u but got %d\n", \
		stats->common->transferIDStr, stats->isochstats.frameID + 1, packet->frameID);
	stats->isochstats.frameID = packet->frameID;
    }
}

static inline void reporter_handle_txmsg_oneway_transit (struct TransferInfo *stats, struct ReportStruct *packet) {
    // very first burst
    if (!stats->isochstats.frameID) {
	stats->isochstats.frameID = packet->frameID;
    }
    if (!TimeZero(stats->ts.prevpacketTime)) {
	double delta = TimeDifference(packet->sentTime, stats->ts.prevpacketTime);
	stats->IPGsum += delta;
    }
    if (packet->transit_ready) {
        reporter_handle_packet_oneway_transit(stats, packet);
	// printf("***Burst id = %ld, transit = %f\n", packet->frameID, stats->transit.lastTransit);
	if (isIsochronous(stats->common)) {
	    if (packet->frameID && (packet->frameID != (stats->isochstats.frameID + 1))) {
		fprintf(stderr,"%sError: expected burst id %u but got %d\n", \
			stats->common->transferIDStr, stats->isochstats.frameID + 1, packet->frameID);
	    }
	    stats->isochstats.frameID = packet->frameID;
	}
    }
}

static void reporter_handle_frame_isoch_oneway_transit (struct TransferInfo *stats, struct ReportStruct *packet) {
    // printf("fid=%lu bs=%lu remain=%lu\n", packet->frameID, packet->burstsize, packet->remaining);
    if (packet->scheduled) {
	reporter_update_mmm(&stats->schedule_error, (double)(packet->sched_err));
    }
    if (packet->frameID && packet->transit_ready) {
	int framedelta=0;
	// very first isochronous frame
	if (!stats->isochstats.frameID) {
	    stats->isochstats.framecnt.current=packet->frameID;
	}
	// perform frame based accounting
	if ((framedelta = (packet->frameID - stats->isochstats.frameID))) {
	    stats->isochstats.framecnt.current++;
	    if (framedelta > 1) {
		stats->isochstats.framelostcnt.current += (framedelta-1);
		stats->isochstats.slipcnt.current++;
	    }
	}
	stats->isochstats.frameID = packet->frameID;
    }
}

// This is done in reporter thread context
void reporter_handle_packet_client (struct ReporterData *data, struct ReportStruct *packet) {
    struct TransferInfo *stats = &data->info;
    stats->ts.packetTime = packet->packetTime;
    switch (packet->err_readwrite) {
    case WriteSelectRetry :
	stats->sock_callstats.write.WriteErr++;
	stats->sock_callstats.write.totWriteErr++;
    case WriteSuccess :
	stats->total.Bytes.current += packet->packetLen;
	// These are valid packets that need standard iperf accounting
	stats->sock_callstats.write.WriteCnt += packet->writecnt;
	stats->sock_callstats.write.totWriteCnt += packet->writecnt;
	if (isIsochronous(stats->common)) {
	    reporter_handle_frame_isoch_oneway_transit(stats, packet);
	} else if (isPeriodicBurst(stats->common)) {
	    reporter_handle_txmsg_oneway_transit(stats, packet);
	}
	if (isTcpWriteTimes(stats->common) && !isUDP(stats->common) && (packet->write_time > 0)) {
	    reporter_update_mmm(&stats->write_mmm.current, ((double) packet->write_time));
	    reporter_update_mmm(&stats->write_mmm.total, ((double) packet->write_time));
	    if (stats->write_histogram ) {
		histogram_insert(stats->write_histogram, (1e-6 * packet->write_time), NULL);
	    }
	}
	break;
    case WriteTimeo:
	stats->sock_callstats.write.WriteTimeo++;
	stats->sock_callstats.write.totWriteTimeo++;
    case WriteErrAccount :
	stats->sock_callstats.write.WriteErr++;
	stats->sock_callstats.write.totWriteErr++;
    case WriteNoAccount:
    case NullEvent:
	break;
    default :
	fprintf(stderr, "Program error: invalid client packet->err_readwrite %d\n", packet->err_readwrite);
    }

    if (isUDP(stats->common)) {
	stats->PacketID = packet->packetID;
	reporter_compute_packet_pps(stats, packet);
    }
}

#define DEBUG_BB_TIMESTAMPS 0
void reporter_handle_packet_bb_client (struct ReporterData *data, struct ReportStruct *packet) {
    struct TransferInfo *stats = &data->info;
    if (packet->scheduled) {
	reporter_update_mmm(&stats->schedule_error, (double)(packet->sched_err));
    }
    if (!packet->emptyreport && (packet->packetLen > 0)) {
	stats->total.Bytes.current += packet->packetLen;
	stats->total.TxBytes.current += packet->writeLen;
	stats->total.RxBytes.current += packet->recvLen;
	double bbrtt = TimeDifference(packet->packetTime, packet->sentTime);
	stats->iBBrunning += bbrtt;
	stats->fBBrunning += bbrtt;
	reporter_update_mmm(&stats->bbrtt.current, bbrtt);
	reporter_update_mmm(&stats->bbrtt.total, bbrtt);
	if (stats->bbrtt_histogram) {
	    histogram_insert(stats->bbrtt_histogram, bbrtt, NULL);
	}
	if (isTripTime(stats->common)) {
	    double bbowdto = TimeDifference(packet->sentTimeRX, packet->sentTime);
	    double bbowdfro = TimeDifference(packet->packetTime, packet->sentTimeTX);
	    double asym = bbowdfro - bbowdto;
	    double bbturnaround = TimeDifference(packet->sentTimeTX, packet->sentTimeRX);
	    double bbadj = TimeDifference(packet->packetTime, packet->sentTimeRX);
	    // If you measure RTT, you can detect when clock are unsync.
	    // If you have the sent-time, rcv-time and return-time, you can check that
	    // sent-time < rcv-time < return-time. As sent-time and return-time use
	    // the same clock, you can detect any drift bigger than RTT. JT
	    //
	    // Adjust this clock A write < clock B read < Clock A read - (clock B write - clock B read)
	    if ((bbowdto < 0) || ((bbadj - bbturnaround) < 0)) {
		stats->bb_clocksync_error++;
	    }
	    reporter_update_mmm(&stats->bbowdto.total, bbowdto);
	    reporter_update_mmm(&stats->bbowdfro.total, bbowdfro);
	    reporter_update_mmm(&stats->bbasym.total, fabs(asym));
	    reporter_update_mmm(&stats->bbowdto.current, bbowdto);
	    reporter_update_mmm(&stats->bbowdfro.current, bbowdfro);
	    reporter_update_mmm(&stats->bbasym.current, fabs(asym));
	    if (stats->bbowdto_histogram) {
		histogram_insert(stats->bbowdto_histogram, bbowdto, NULL);
	    }
	    if (stats->bbowdfro_histogram) {
		histogram_insert(stats->bbowdfro_histogram, bbowdfro, NULL);
	    }
	}
	stats->ts.prevpacketTime = packet->packetTime;
#if DEBUG_BB_TIMESTAMPS
	fprintf(stderr, "BB Debug: ctx=%lx.%lx srx=%lx.%lx stx=%lx.%lx crx=%lx.%lx  (hex)\n", packet->sentTime.tv_sec, packet->sentTime.tv_usec, packet->sentTimeRX.tv_sec, packet->sentTimeRX.tv_usec, packet->sentTimeTX.tv_sec, packet->sentTimeTX.tv_usec, packet->packetTime.tv_sec, packet->packetTime.tv_usec);
	fprintf(stderr, "BB Debug: ctx=%ld.%06ld srx=%ld.%06ld stx=%ld.%06ld crx=%ld.%06ld\n", packet->sentTime.tv_sec, packet->sentTime.tv_usec, packet->sentTimeRX.tv_sec, packet->sentTimeRX.tv_usec, packet->sentTimeTX.tv_sec, packet->sentTimeTX.tv_usec, packet->packetTime.tv_sec, packet->packetTime.tv_usec);
	fprintf(stderr, "BB RTT=%f OWDTo=%f OWDFro=%f Asym=%f\n", bbrtt, bbowdto, bbowdfro, asym);
#endif
    }
}

inline void reporter_handle_packet_server_tcp (struct ReporterData *data, struct ReportStruct *packet) {
    struct TransferInfo *stats = &data->info;
    if (packet->packetLen > 0) {
	int bin;
	stats->total.Bytes.current += packet->packetLen;
	// mean min max tests
	stats->sock_callstats.read.ReadCnt.current++;
	bin = (int)floor((packet->packetLen -1)/stats->sock_callstats.read.binsize);
	if (bin < TCPREADBINCOUNT) {
	    stats->sock_callstats.read.bins[bin]++;
	    stats->sock_callstats.read.totbins[bin]++;
	}
	if (packet->transit_ready) {
	    if (isIsochronous(stats->common) && packet->frameID) {
		reporter_handle_isoch_oneway_transit_tcp(stats, packet);
	    } else if (isPeriodicBurst(stats->common) || isTripTime(stats->common)) {
		reporter_handle_rxmsg_oneway_transit(stats, packet);
	    }
	}
    }
}

inline void reporter_handle_packet_server_udp (struct ReporterData *data, struct ReportStruct *packet) {
    struct TransferInfo *stats = &data->info;
    stats->ts.packetTime = packet->packetTime;
    if (packet->emptyreport && (stats->transit.current.cnt == 0)) {
	// This is the case when empty reports
	// cross the report interval boundary
	// Hence, set the per interval min to infinity
	// and the per interval max and sum to zero
	reporter_reset_mmm(&stats->transit.current);
    } else if (!packet->emptyreport && (packet->packetID > 0)) {
	bool ooo_packet = false;
	// packet loss occured if the datagram numbers aren't sequential
	if (packet->packetID != stats->PacketID + 1) {
	    if (packet->packetID < stats->PacketID + 1) {
		stats->total.OutofOrder.current++;
		ooo_packet = true;
	    } else {
		stats->total.Lost.current += packet->packetID - stats->PacketID - 1;
	    }
	}
	// never decrease datagramID (e.g. if we get an out-of-order packet)
	if (packet->packetID > stats->PacketID) {
	    stats->PacketID = packet->packetID;
	}
	// These are valid packets that need standard iperf accounting
	// Do L2 accounting first (if needed)
	if (packet->l2errors && (stats->total.Datagrams.current > L2DROPFILTERCOUNTER)) {
	    stats->l2counts.cnt++;
	    stats->l2counts.tot_cnt++;
	    if (packet->l2errors & L2UNKNOWN) {
		stats->l2counts.unknown++;
		stats->l2counts.tot_unknown++;
	    }
	    if (packet->l2errors & L2LENERR) {
		stats->l2counts.lengtherr++;
		stats->l2counts.tot_lengtherr++;
	    }
	    if (packet->l2errors & L2CSUMERR) {
		stats->l2counts.udpcsumerr++;
		stats->l2counts.tot_udpcsumerr++;
	    }
	}
	if (packet->err_readwrite == ReadErrLen) {
	    stats->sock_callstats.read.ReadErrLenCnt.current++;
	}
	if (!ooo_packet && \
	    ((packet->err_readwrite == ReadSuccess) ||
	     ((packet->err_readwrite == ReadErrLen) && (packet->packetLen >= sizeof(struct UDP_datagram))))) {
	    reporter_handle_packet_oneway_transit(stats, packet);
	}
	stats->total.Bytes.current += packet->packetLen;
	reporter_compute_packet_pps(stats, packet);

	if (packet->transit_ready) {
	    if (isIsochronous(stats->common)) {
	        reporter_handle_isoch_oneway_transit_udp(stats, packet);
	    } else if (isPeriodicBurst(stats->common)) {
	        reporter_handle_txmsg_oneway_transit(stats, packet);
	    }
	}
    }
    if (packet->err_readwrite != ReadNoAccount) {
	if (packet->emptyreport) {
	    stats->sock_callstats.read.ReadTimeoCnt.current++;
	} else {
	    stats->sock_callstats.read.ReadCnt.current++;
	}
    }
}
#if HAVE_TCP_STATS
static inline void reporter_handle_packet_tcpistats (struct ReporterData *data, struct ReportStruct *packet) {
    assert(data!=NULL);
    struct TransferInfo *stats = &data->info;
    stats->sock_callstats.write.tcpstats.retry += (packet->tcpstats.retry_tot - stats->sock_callstats.write.tcpstats.retry_prev);
    stats->sock_callstats.write.tcpstats.retry_prev = packet->tcpstats.retry_tot;
    stats->sock_callstats.write.tcpstats.retry_tot = packet->tcpstats.retry_tot;
    stats->sock_callstats.write.tcpstats.cwnd = packet->tcpstats.cwnd;
    stats->sock_callstats.write.tcpstats.cwnd_packets = packet->tcpstats.cwnd_packets;
    stats->sock_callstats.write.tcpstats.rtt = packet->tcpstats.rtt;
    stats->sock_callstats.write.tcpstats.rttvar = packet->tcpstats.rttvar;
#if HAVE_TCP_INFLIGHT
    stats->sock_callstats.write.tcpstats.packets_in_flight = packet->tcpstats.packets_in_flight;
    stats->sock_callstats.write.tcpstats.bytes_in_flight = packet->tcpstats.bytes_in_flight;
#else
    stats->sock_callstats.write.tcpstats.bytes_in_flight = -1;
    stats->sock_callstats.write.tcpstats.packets_in_flight = -1;
#endif
}
#endif

/*
 * Report printing routines below
 */
static inline void reporter_set_timestamps_time (struct TransferInfo *stats, enum TimeStampType tstype) {
    // There is a corner case when the first packet is also the last where the start time (which comes
    // from app level syscall) is greater than the packetTime (which come for kernel level SO_TIMESTAMP)
    // For this case set the start and end time to both zero.
    struct ReportTimeStamps *times = &stats->ts;
    if (TimeDifference(times->packetTime, times->startTime) < 0) {
	times->iEnd = 0;
	times->iStart = 0;
    } else {
	switch (tstype) {
	case INTERVAL:
	    times->iStart = times->iEnd;
	    times->iEnd = TimeDifference(times->nextTime, times->startTime);
	    TimeAdd(times->nextTime, times->intervalTime);
	    stats->final = false;
	    break;
	case TOTAL:
	    times->iStart = 0;
	    times->iEnd = TimeDifference(times->packetTime, times->startTime);
	    stats->final = true;
	    break;
	case FINALPARTIAL:
	    times->iStart = times->iEnd;
	    times->iEnd = TimeDifference(times->packetTime, times->startTime);
	    stats->final = false;
	    break;
	case INTERVALPARTIAL:
	    if ((times->iStart = TimeDifference(times->prevpacketTime, times->startTime)) < 0)
		times->iStart = 0.0;
	    times->iEnd = TimeDifference(times->packetTime, times->startTime);
	    stats->final = false;
	    break;
	default:
	    times->iEnd = -1;
	    times->iStart = -1;
	    stats->final = false;
	    break;
	}
    }
}

#if HAVE_SUMMING_DEBUG
static void reporter_dump_timestamps (struct ReportStruct *packet, struct TransferInfo *stats, struct TransferInfo *sumstats, bool final) {
    if (packet)
	printf("**** %s pkt      =%ld.%06ld (up/down)=%d/%d ibytes=%ld/%ld sbytes=%ld/%ld (final=%d)\n", stats->common->transferIDStr, (long) packet->packetTime.tv_sec, \
	       (long) packet->packetTime.tv_usec, sumstats->slot_thread_upcount, sumstats->slot_thread_downcount, stats->total.Bytes.prev, stats->total.Bytes.current, \
	       sumstats->total.Bytes.prev, sumstats->total.Bytes.current, final);
    else {
	printf("**** %s pkt ts   =%ld.%06ld prev=%ld.%06ld (up/down)=%d/%d ibytes=%ld/%ld sbytes=%ld/%ld (final=%d)\n", stats->common->transferIDStr, (long) stats->ts.packetTime.tv_sec, \
	       (long) stats->ts.packetTime.tv_usec, (long) stats->ts.prevpacketTime.tv_sec, (long) stats->ts.prevpacketTime.tv_usec, \
	       sumstats->slot_thread_upcount, sumstats->slot_thread_downcount, stats->total.Bytes.prev, stats->total.Bytes.current, \
	       sumstats->total.Bytes.prev, sumstats->total.Bytes.current, final);
    }
    printf("**** %s stats    =%ld.%06ld next=%ld.%06ld prev=%ld.%06ld\n", stats->common->transferIDStr, (long) stats->ts.packetTime.tv_sec, \
	   (long) stats->ts.packetTime.tv_usec, (long) stats->ts.nextTime.tv_sec, (long) stats->ts.nextTime.tv_usec, (long) stats->ts.prevpacketTime.tv_sec, (long) stats->ts.prevpacketTime.tv_usec);
    printf("**** %s sum stats=%ld.%06ld next=%ld.%06ld prev=%ld.%06ld \n", stats->common->transferIDStr, (long) sumstats->ts.packetTime.tv_sec, \
	   (long) sumstats->ts.packetTime.tv_usec, (long) sumstats->ts.nextTime.tv_sec, (long) sumstats->ts.nextTime.tv_usec, (long) sumstats->ts.prevTime.tv_sec, (long) sumstats->ts.prevTime.tv_usec);
}
#endif

// If reports were missed, catch up now
static inline void reporter_transfer_protocol_missed_reports (struct TransferInfo *stats, struct ReportStruct *packet) {
    while (TimeDifference(packet->packetTime, stats->ts.nextTime) > TimeDouble(stats->ts.intervalTime)) {
//	printf("**** cmp=%f/%f next %ld.%06ld packet %ld.%06ld id=%ld\n", TimeDifference(packet->packetTime, stats->ts.nextTime), TimeDouble(stats->ts.intervalTime), stats->ts.nextTime.tv_sec, stats->ts.nextTime.tv_usec, packet->packetTime.tv_sec, packet->packetTime.tv_usec, packet->packetID);
	reporter_set_timestamps_time(stats, INTERVAL);
	struct TransferInfo emptystats;
	memset(&emptystats, 0, sizeof(struct TransferInfo));
	emptystats.ts.iStart = stats->ts.iStart;
	emptystats.ts.iEnd = stats->ts.iEnd;
	emptystats.common = stats->common;
	if ((stats->output_handler) && !(stats->isMaskOutput))
	    (*stats->output_handler)(&emptystats);
    }
}

static inline void reporter_reset_transfer_stats_sum (struct TransferInfo *sumstats) {
#if HAVE_SUMMING_DEBUG
    printf("***** [SUM] RESET %ld.%06ld nxt %ld.%06ld down=%d up=%d\n", (long) sumstats->ts.prevTime.tv_sec, (long) sumstats->ts.prevTime.tv_usec, \
	   (long) sumstats->ts.nextTime.tv_sec, (long) sumstats->ts.nextTime.tv_usec, sumstats->slot_thread_downcount, sumstats->slot_thread_upcount);
#endif
    sumstats->slot_thread_upcount -= sumstats->slot_thread_downcount;
    sumstats->slot_thread_downcount = 0;
    sumstats->ts.prevTime = sumstats->ts.nextTime;
    sumstats->iInP = 0;
    sumstats->uplevel = toggleLevel(sumstats->uplevel);
    sumstats->downlevel = toggleLevel(sumstats->downlevel);
    sumstats->total.Bytes.prev = sumstats->total.Bytes.current;
}

static inline void reporter_reset_transfer_stats_client_tcp (struct TransferInfo *stats) {
    stats->total.Bytes.prev = stats->total.Bytes.current;
    stats->sock_callstats.write.WriteCnt = 0;
    stats->sock_callstats.write.WriteErr = 0;
    stats->sock_callstats.write.WriteTimeo = 0;
    stats->isochstats.framecnt.prev = stats->isochstats.framecnt.current;
    stats->isochstats.framelostcnt.prev = stats->isochstats.framelostcnt.current;
    stats->isochstats.slipcnt.prev = stats->isochstats.slipcnt.current;
#if HAVE_TCP_STATS
    // set the interval retry counter to zero
    stats->sock_callstats.write.tcpstats.retry = 0;
#endif
    if (isBounceBack(stats->common)) {
	stats->iBBrunning = 0;
	reporter_reset_mmm(&stats->bbrtt.current);
	reporter_reset_mmm(&stats->bbowdto.current);
	reporter_reset_mmm(&stats->bbowdfro.current);
	reporter_reset_mmm(&stats->bbasym.current);
	stats->total.RxBytes.prev = stats->total.RxBytes.current;
	stats->total.TxBytes.prev = stats->total.TxBytes.current;
    }
    if (isTcpWriteTimes(stats->common)) {
	reporter_reset_mmm(&stats->write_mmm.current);
    }
}

static inline void reporter_reset_transfer_stats_client_udp (struct TransferInfo *stats) {
    if (stats->cntError < 0) {
	stats->cntError = 0;
    }
    stats->total.Lost.prev = stats->total.Lost.current;
    stats->total.Datagrams.prev = stats->total.Datagrams.current;
    stats->total.Bytes.prev = stats->total.Bytes.current;
    stats->total.IPG.prev = stats->total.IPG.current;
    stats->sock_callstats.write.WriteCnt = 0;
    stats->sock_callstats.write.WriteErr = 0;
    stats->sock_callstats.write.WriteTimeo = 0;
    stats->isochstats.framecnt.prev = stats->isochstats.framecnt.current;
    stats->isochstats.framelostcnt.prev = stats->isochstats.framelostcnt.current;
    stats->isochstats.slipcnt.prev = stats->isochstats.slipcnt.current;
    if (stats->cntDatagrams)
	stats->IPGsum = 0;
}

static inline void reporter_reset_transfer_stats_server_tcp (struct TransferInfo *stats) {
    int ix;
    stats->total.Bytes.prev = stats->total.Bytes.current;
    stats->sock_callstats.read.ReadCnt.prev = stats->sock_callstats.read.ReadCnt.current;
    for (ix = 0; ix < 8; ix++) {
	stats->sock_callstats.read.bins[ix] = 0;
    }
    reporter_reset_mmm(&stats->transit.current);
    reporter_reset_mmm(&stats->isochstats.transit.current);
    stats->IPGsum = 0;
}

static inline void reporter_reset_transfer_stats_server_udp (struct TransferInfo *stats) {
    // Reset the enhanced stats for the next report interval
    stats->total.Bytes.prev = stats->total.Bytes.current;
    stats->total.Datagrams.prev = stats->PacketID;
    stats->total.OutofOrder.prev = stats->total.OutofOrder.current;
    stats->total.Lost.prev = stats->total.Lost.current;
    stats->total.IPG.prev = stats->total.IPG.current;
    reporter_reset_mmm(&stats->transit.current);
    stats->isochstats.framecnt.prev = stats->isochstats.framecnt.current;
    stats->isochstats.framelostcnt.prev = stats->isochstats.framelostcnt.current;
    stats->isochstats.slipcnt.prev = stats->isochstats.slipcnt.current;
    stats->l2counts.cnt = 0;
    stats->l2counts.unknown = 0;
    stats->l2counts.udpcsumerr = 0;
    stats->l2counts.lengtherr = 0;
    stats->iInP = 0;
    stats->sock_callstats.read.ReadCnt.prev = stats->sock_callstats.read.ReadCnt.current;
    stats->sock_callstats.read.ReadTimeoCnt.prev = stats->sock_callstats.read.ReadTimeoCnt.current;
    stats->sock_callstats.read.ReadErrLenCnt.prev = stats->sock_callstats.read.ReadErrLenCnt.current;
    if (stats->cntDatagrams)
	stats->IPGsum = 0;
}
// These do the following
//
// o) set the TransferInfo struct and then calls the individual report output handler
// o) updates the sum and fullduplex reports
//
void reporter_transfer_protocol_server_udp (struct ReporterData *data, bool final) {
    struct TransferInfo *stats = &data->info;
    struct TransferInfo *sumstats = (data->GroupSumReport != NULL) ? &data->GroupSumReport->info : NULL;
    struct TransferInfo *fullduplexstats = (data->FullDuplexReport != NULL) ? &data->FullDuplexReport->info : NULL;
    // print a interval report and possibly a partial interval report if this a final
    stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
    stats->cntOutofOrder = stats->total.OutofOrder.current - stats->total.OutofOrder.prev;
    // assume most of the  time out-of-order packets are
    // duplicate packets, so conditionally subtract them from the lost packets.
    stats->cntError = stats->total.Lost.current - stats->total.Lost.prev - stats->cntOutofOrder;
    if (stats->cntError < 0)
	stats->cntError = 0;
    stats->cntDatagrams = stats->PacketID - stats->total.Datagrams.prev;
    stats->cntIPG = stats->total.IPG.current - stats->total.IPG.prev;
    stats->sock_callstats.read.cntRead = stats->sock_callstats.read.ReadCnt.current - stats->sock_callstats.read.ReadCnt.prev;
    stats->sock_callstats.read.cntReadTimeo = stats->sock_callstats.read.ReadTimeoCnt.current - stats->sock_callstats.read.ReadTimeoCnt.prev;
    stats->sock_callstats.read.cntReadErrLen = stats->sock_callstats.read.ReadErrLenCnt.current - stats->sock_callstats.read.ReadErrLenCnt.prev;

    if (stats->latency_histogram) {
        stats->latency_histogram->final = final;
    }
    if (stats->jitter_histogram) {
        stats->jitter_histogram->final = final;
    }
    if (isIsochronous(stats->common)) {
	stats->isochstats.cntFrames = stats->isochstats.framecnt.current - stats->isochstats.framecnt.prev;
	stats->isochstats.cntFramesMissed = stats->isochstats.framelostcnt.current - stats->isochstats.framelostcnt.prev;
	stats->isochstats.cntSlips = stats->isochstats.slipcnt.current - stats->isochstats.slipcnt.prev;
	if (stats->framelatency_histogram) {
	    stats->framelatency_histogram->final = final;
	}
    }
    if (stats->total.Datagrams.current == 1)
	stats->jitter = 0;
    if (isTripTime(stats->common) && !final) {
	double lambda =  ((stats->IPGsum > 0.0) ? (round (stats->cntIPG / stats->IPGsum)) : 0.0);
	double meantransit = (double) ((stats->transit.current.cnt > 0) ? (stats->transit.current.sum / stats->transit.current.cnt) : 0.0);
	double variance = (stats->transit.current.cnt < 2) ? 0 : \
	    (sqrt(stats->transit.current.m2 / (stats->transit.current.cnt - 1)));
	stats->iInP = (double) lambda * meantransit;
	stats->iInPVar = (double) lambda * variance;
    }
    if (sumstats) {
	sumstats->total.OutofOrder.current += stats->total.OutofOrder.current - stats->total.OutofOrder.prev;
	// assume most of the  time out-of-order packets are not
	// duplicate packets, so conditionally subtract them from the lost packets.
	sumstats->total.Lost.current += stats->total.Lost.current - stats->total.Lost.prev;
	sumstats->total.Datagrams.current += stats->PacketID - stats->total.Datagrams.prev;
	sumstats->total.Bytes.current += stats->cntBytes;
	sumstats->total.IPG.current += stats->cntIPG;
	if (sumstats->IPGsum < stats->IPGsum)
	    sumstats->IPGsum = stats->IPGsum;
	sumstats->iInP += stats->iInP;
	sumstats->sock_callstats.read.cntRead += stats->sock_callstats.read.cntRead;
	sumstats->sock_callstats.read.cntReadTimeo += stats->sock_callstats.read.cntReadTimeo;
	sumstats->sock_callstats.read.cntReadErrLen += stats->sock_callstats.read.cntReadErrLen;
	if (isEnhanced(stats->common)) {
	    reporter_update_mmm_sum(&sumstats->transit.current, &stats->transit.current);
	    reporter_update_mmm_sum(&sumstats->transit.total, &stats->transit.total);
	}
	if (final) {
	    sumstats->threadcnt_final++;
	    if (data->packetring->downlevel != sumstats->downlevel) {
		sumstats->slot_thread_downcount++;
	    }
	    if (data->packetring->uplevel != sumstats->uplevel){
		sumstats->slot_thread_upcount++;
	    }
#if HAVE_SUMMING_DEBUG
	    printf("**** %s downcnt (%p) (up/down)=%d/%d final true uplevel (sum/pkt)=%d/%d\n", stats->common->transferIDStr, (void *)data->packetring, \
		   sumstats->slot_thread_upcount, sumstats->slot_thread_downcount, \
		   sumstats->uplevel, data->packetring->uplevel);
#endif
	}
    }
    if (fullduplexstats) {
	fullduplexstats->total.Bytes.current += stats->cntBytes;
	fullduplexstats->total.IPG.current += stats->cntIPG;
	fullduplexstats->total.Datagrams.current += (stats->total.Datagrams.current - stats->total.Datagrams.prev);
	if (fullduplexstats->IPGsum < stats->IPGsum)
	    fullduplexstats->IPGsum = stats->IPGsum;
    }
    if (final) {
	if ((stats->cntBytes > 0) && !TimeZero(stats->ts.intervalTime)) {
	    stats->cntOutofOrder = stats->total.OutofOrder.current - stats->total.OutofOrder.prev;
	    // assume most of the  time out-of-order packets are not
	    // duplicate packets, so conditionally subtract them from the lost packets.
	    stats->cntError = stats->total.Lost.current - stats->total.Lost.prev;
	    stats->cntError -= stats->cntOutofOrder;
	    if (stats->cntError < 0)
		stats->cntError = 0;
	    stats->cntDatagrams = stats->PacketID - stats->total.Datagrams.prev;
	    if ((stats->output_handler) && !(stats->isMaskOutput)) {
		reporter_set_timestamps_time(stats, FINALPARTIAL);
		if ((stats->ts.iEnd - stats->ts.iStart) > stats->ts.significant_partial)
		    (*stats->output_handler)(stats);
	    }
	}
	reporter_set_timestamps_time(stats, TOTAL);
	stats->IPGsum = TimeDifference(stats->ts.packetTime, stats->ts.startTime);
	stats->cntOutofOrder = stats->total.OutofOrder.current;
	// assume most of the  time out-of-order packets are not
	// duplicate packets, so conditionally subtract them from the lost packets.
	stats->cntError = stats->total.Lost.current;
	stats->cntError -= stats->cntOutofOrder;
	if (stats->cntError < 0)
	    stats->cntError = 0;
	stats->cntDatagrams = stats->PacketID;
	stats->cntIPG = stats->total.IPG.current;
	stats->IPGsum = TimeDifference(stats->ts.packetTime, stats->ts.startTime);
	stats->cntBytes = stats->total.Bytes.current;
	stats->l2counts.cnt = stats->l2counts.tot_cnt;
	stats->l2counts.unknown = stats->l2counts.tot_unknown;
	stats->l2counts.udpcsumerr = stats->l2counts.tot_udpcsumerr;
	stats->l2counts.lengtherr = stats->l2counts.tot_lengtherr;
	stats->transit.current = stats->transit.total;
	if (isTripTime(stats->common)) {
	    double lambda =  ((stats->IPGsum > 0.0) ? (round (stats->cntIPG / stats->IPGsum)) : 0.0);
	    double meantransit = (double) ((stats->transit.total.cnt > 0) ? (stats->transit.total.sum / stats->transit.total.cnt) : 0.0);
	    double variance = (stats->transit.total.cnt < 2) ? 0 :	\
		(sqrt(stats->transit.total.m2 / (stats->transit.total.cnt - 1)));
	    stats->fInP = (double) lambda * meantransit;
	    stats->fInPVar = (double) lambda * variance;
	    if (sumstats) {
		sumstats->fInP += stats->fInP;
	    }
	}
	if (isIsochronous(stats->common)) {
	    stats->isochstats.cntFrames = stats->isochstats.framecnt.current;
	    stats->isochstats.cntFramesMissed = stats->isochstats.framelostcnt.current;
	    stats->isochstats.cntSlips = stats->isochstats.slipcnt.current;
	}
	if (stats->latency_histogram) {
	    if (sumstats && sumstats->latency_histogram) {
	        histogram_add(sumstats->latency_histogram, stats->latency_histogram);
		sumstats->latency_histogram->final = true;
	    }
	    stats->latency_histogram->final = true;
	}
	if (stats->jitter_histogram) {
	    if (sumstats && sumstats->jitter_histogram) {
	        histogram_add(sumstats->jitter_histogram, stats->jitter_histogram);
		sumstats->jitter_histogram->final = true;
	    }
	    stats->jitter_histogram->final = true;
	}
	if (stats->framelatency_histogram) {
	    stats->framelatency_histogram->final = true;
	}
	stats->sock_callstats.read.cntRead = stats->sock_callstats.read.ReadCnt.current;
	stats->sock_callstats.read.cntReadTimeo = stats->sock_callstats.read.ReadTimeoCnt.current;
	stats->sock_callstats.read.cntReadErrLen = stats->sock_callstats.read.ReadErrLenCnt.current;
    }
    if ((stats->output_handler) && !(stats->isMaskOutput))
	(*stats->output_handler)(stats);
    if (!final)
	reporter_reset_transfer_stats_server_udp(stats);
}

void reporter_transfer_protocol_sum_server_udp (struct TransferInfo *stats, bool final) {
    if (final) {
	reporter_set_timestamps_time(stats, TOTAL);
	stats->cntOutofOrder = stats->total.OutofOrder.current;
	// assume most of the  time out-of-order packets are not
	// duplicate packets, so conditionally subtract them from the lost packets.
	stats->cntError = stats->total.Lost.current;
	stats->cntError -= stats->cntOutofOrder;
	if (stats->cntError < 0)
	    stats->cntError = 0;
	stats->cntDatagrams = stats->total.Datagrams.current;
	stats->cntBytes = stats->total.Bytes.current;
	stats->IPGsum = TimeDifference(stats->ts.packetTime, stats->ts.startTime);
	stats->cntIPG = stats->total.IPG.current;
    } else {
	stats->cntOutofOrder = stats->total.OutofOrder.current - stats->total.OutofOrder.prev;
	// assume most of the  time out-of-order packets are not
	// duplicate packets, so conditionally subtract them from the lost packets.
	stats->cntError = stats->total.Lost.current - stats->total.Lost.prev;
	stats->cntError -= stats->cntOutofOrder;
	if (stats->cntError < 0)
	    stats->cntError = 0;
	stats->cntDatagrams = stats->total.Datagrams.current - stats->total.Datagrams.prev;
	stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
	stats->cntIPG = stats->total.IPG.current - stats->total.IPG.prev;
    }
    if ((stats->output_handler) && !(stats->isMaskOutput)) {
	(*stats->output_handler)(stats);
	reporter_reset_transfer_stats_sum(stats);
    }
    if (!final) {
	// there is no packet ID for sum server reports, set it to total cnt for calculation
	stats->PacketID = stats->total.Datagrams.current;
	reporter_reset_transfer_stats_server_udp(stats);
    }
}
void reporter_transfer_protocol_sum_client_udp (struct TransferInfo *stats, bool final) {
    if (final) {
	reporter_set_timestamps_time(stats, TOTAL);
	stats->sock_callstats.write.WriteErr = stats->sock_callstats.write.totWriteErr;
	stats->sock_callstats.write.WriteCnt = stats->sock_callstats.write.totWriteCnt;
	stats->cntDatagrams = stats->total.Datagrams.current;
	stats->cntBytes = stats->total.Bytes.current;
	stats->IPGsum = TimeDifference(stats->ts.packetTime, stats->ts.startTime);
	stats->cntIPG = stats->total.IPG.current;
    } else {
	stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
	stats->cntIPG = stats->total.IPG.current - stats->total.IPG.prev;
	stats->cntDatagrams = stats->total.Datagrams.current - stats->total.Datagrams.prev;
    }
    if ((stats->output_handler) && !(stats->isMaskOutput)) {
	(*stats->output_handler)(stats);
	reporter_reset_transfer_stats_sum(stats);
    }
    if (!final) {
	reporter_reset_transfer_stats_client_udp(stats);
    } else if ((stats->common->ReportMode != kReport_CSV) && !(stats->isMaskOutput)) {
	printf(report_sumcnt_datagrams, stats->threadcnt_final, stats->total.Datagrams.current);
	fflush(stdout);
    }
}

void reporter_transfer_protocol_client_udp (struct ReporterData *data, bool final) {
    struct TransferInfo *stats = &data->info;
    struct TransferInfo *sumstats = (data->GroupSumReport != NULL) ? &data->GroupSumReport->info : NULL;
    struct TransferInfo *fullduplexstats = (data->FullDuplexReport != NULL) ? &data->FullDuplexReport->info : NULL;
    stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
    stats->cntDatagrams = stats->total.Datagrams.current - stats->total.Datagrams.prev;
    stats->cntIPG = stats->total.IPG.current - stats->total.IPG.prev;
    if (isIsochronous(stats->common)) {
	stats->isochstats.cntFrames = stats->isochstats.framecnt.current - stats->isochstats.framecnt.prev;
	stats->isochstats.cntFramesMissed = stats->isochstats.framelostcnt.current - stats->isochstats.framelostcnt.prev;
	stats->isochstats.cntSlips = stats->isochstats.slipcnt.current - stats->isochstats.slipcnt.prev;
    }
    if (sumstats) {
	sumstats->total.Bytes.current += stats->cntBytes;
	sumstats->sock_callstats.write.WriteErr += stats->sock_callstats.write.WriteErr;
	sumstats->sock_callstats.write.WriteCnt += stats->sock_callstats.write.WriteCnt;
	sumstats->sock_callstats.write.WriteTimeo += stats->sock_callstats.write.WriteTimeo;
	sumstats->sock_callstats.write.totWriteErr += stats->sock_callstats.write.WriteErr;
	sumstats->sock_callstats.write.totWriteCnt += stats->sock_callstats.write.WriteCnt;
	sumstats->sock_callstats.write.totWriteTimeo += stats->sock_callstats.write.WriteTimeo;
	sumstats->total.Datagrams.current += stats->cntDatagrams;
	if (sumstats->IPGsum < stats->IPGsum)
	    sumstats->IPGsum = stats->IPGsum;
	sumstats->total.IPG.current += stats->cntIPG;
	if (final) {
	    sumstats->threadcnt_final++;
	    if (data->packetring->downlevel != sumstats->downlevel) {
		sumstats->slot_thread_downcount++;
	    }
	    if (data->packetring->uplevel != sumstats->uplevel){
		sumstats->slot_thread_upcount++;
	    }
#if HAVE_SUMMING_DEBUG
	    printf("**** %s downcnt (%p) (up/down)=%d/%d final true level (sum/pkt)=%d/%d\n", stats->common->transferIDStr, (void *)data->packetring, \
		   sumstats->slot_thread_upcount, sumstats->slot_thread_downcount, \
		   sumstats->uplevel, data->packetring->uplevel);
#endif
	}
    }
    if (fullduplexstats) {
	fullduplexstats->total.Bytes.current += stats->cntBytes;
	fullduplexstats->total.IPG.current += stats->cntIPG;
	fullduplexstats->total.Datagrams.current += stats->cntDatagrams;
	if (fullduplexstats->IPGsum < stats->IPGsum)
	    fullduplexstats->IPGsum = stats->IPGsum;
    }
    if (final) {
	reporter_set_timestamps_time(stats, TOTAL);
	stats->cntBytes = stats->total.Bytes.current;
	stats->sock_callstats.write.WriteErr = stats->sock_callstats.write.totWriteErr;
	stats->sock_callstats.write.WriteCnt = stats->sock_callstats.write.totWriteCnt;
	stats->sock_callstats.write.WriteTimeo = stats->sock_callstats.write.totWriteTimeo;
	stats->cntIPG = stats->total.IPG.current;
	stats->cntDatagrams = stats->PacketID;
	stats->IPGsum = TimeDifference(stats->ts.packetTime, stats->ts.startTime);
	if (isIsochronous(stats->common)) {
	    stats->isochstats.cntFrames = stats->isochstats.framecnt.current;
	    stats->isochstats.cntFramesMissed = stats->isochstats.framelostcnt.current;
	    stats->isochstats.cntSlips = stats->isochstats.slipcnt.current;
	}
    } else {
	if (stats->ts.iEnd > 0) {
	    stats->cntIPG = (stats->total.IPG.current - stats->total.IPG.prev);
	} else {
	    stats->cntIPG = 0;
	}
    }
    if ((stats->output_handler) && !(stats->isMaskOutput)) {
	(*stats->output_handler)(stats);
	if (final && (stats->common->ReportMode != kReport_CSV)) {
	    printf(report_datagrams, stats->common->transferID, stats->total.Datagrams.current);
	    fflush(stdout);
	}
    }
    reporter_reset_transfer_stats_client_udp(stats);
}

void reporter_transfer_protocol_server_tcp (struct ReporterData *data, bool final) {
    struct TransferInfo *stats = &data->info;
    struct TransferInfo *sumstats = (data->GroupSumReport != NULL) ? &data->GroupSumReport->info : NULL;
    struct TransferInfo *fullduplexstats = (data->FullDuplexReport != NULL) ? &data->FullDuplexReport->info : NULL;
    stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
    int ix;
    if (stats->framelatency_histogram) {
        stats->framelatency_histogram->final = false;
    }
    double thisInP;
    if (!final) {
	double bytecnt = (double) (stats->total.Bytes.current - stats->total.Bytes.prev);
	double lambda = (stats->IPGsum > 0.0) ? (bytecnt / stats->IPGsum) : 0.0;
	double meantransit = (double) ((stats->transit.current.cnt > 0) ? (stats->transit.current.sum / stats->transit.current.cnt) : 0.0);
	thisInP  = lambda * meantransit;
	stats->iInP = thisInP;
        stats->sock_callstats.read.cntRead = stats->sock_callstats.read.ReadCnt.current - stats->sock_callstats.read.ReadCnt.prev;
    } else {
	double bytecnt = (double) stats->cntBytes;
	double lambda = (stats->IPGsum > 0.0) ? (bytecnt / stats->IPGsum) : 0.0;
	double meantransit = (double) ((stats->transit.total.cnt > 0) ? (stats->transit.total.sum / stats->transit.total.cnt) : 0.0);
	thisInP  = lambda * meantransit;
	stats->fInP = thisInP;
        stats->sock_callstats.read.cntRead = stats->sock_callstats.read.ReadCnt.current;
    }
    if (sumstats) {
	sumstats->total.Bytes.current += stats->cntBytes;
        sumstats->sock_callstats.read.ReadCnt.current += stats->sock_callstats.read.cntRead;
        for (ix = 0; ix < TCPREADBINCOUNT; ix++) {
	    sumstats->sock_callstats.read.bins[ix] += stats->sock_callstats.read.bins[ix];
	    sumstats->sock_callstats.read.totbins[ix] += stats->sock_callstats.read.bins[ix];
        }
	if (!final) {
	    sumstats->iInP += thisInP;
	} else {
	    sumstats->fInP += thisInP;
	    sumstats->threadcnt_final++;
	    if (data->packetring->downlevel != sumstats->downlevel) {
		sumstats->slot_thread_downcount++;
	    }
	    if (data->packetring->uplevel != sumstats->uplevel){
		sumstats->slot_thread_upcount++;
	    }
#if HAVE_SUMMING_DEBUG
	    printf("**** %s downcnt (%p) (up/down)=%d/%d final true level (sum/pkt)=%d/%d\n", stats->common->transferIDStr, (void *)data->packetring, \
		   sumstats->slot_thread_upcount, sumstats->slot_thread_downcount, \
		   sumstats->uplevel, data->packetring->uplevel);
#endif
	}
    }
    if (fullduplexstats) {
	fullduplexstats->total.Bytes.current += stats->cntBytes;
    }
    if (final) {
	if ((stats->cntBytes > 0) && stats->output_handler && !TimeZero(stats->ts.intervalTime)) {
	    // print a partial interval report if enable and this a final
	    if ((stats->output_handler) && !(stats->isMaskOutput)) {
		if (isIsochronous(stats->common)) {
		    stats->isochstats.cntFrames = stats->isochstats.framecnt.current - stats->isochstats.framecnt.prev;
		    stats->isochstats.cntFramesMissed = stats->isochstats.framelostcnt.current - stats->isochstats.framelostcnt.prev;
		    stats->isochstats.cntSlips = stats->isochstats.slipcnt.current - stats->isochstats.slipcnt.prev;
		}
		reporter_set_timestamps_time(stats, FINALPARTIAL);
		if ((stats->ts.iEnd - stats->ts.iStart) > stats->ts.significant_partial)
		    (*stats->output_handler)(stats);
		reporter_reset_transfer_stats_server_tcp(stats);
	    }
        }
        if (stats->framelatency_histogram) {
	    stats->framelatency_histogram->final = true;
	}
	reporter_set_timestamps_time(stats, TOTAL);
        stats->cntBytes = stats->total.Bytes.current;
	stats->IPGsum = stats->ts.iEnd;
        stats->sock_callstats.read.cntRead = stats->sock_callstats.read.ReadCnt.current;
        for (ix = 0; ix < TCPREADBINCOUNT; ix++) {
	    stats->sock_callstats.read.bins[ix] = stats->sock_callstats.read.totbins[ix];
        }
	if (isIsochronous(stats->common)) {
	    stats->isochstats.cntFrames = stats->isochstats.framecnt.current;
	    stats->isochstats.cntFramesMissed = stats->isochstats.framelostcnt.current;
	    stats->isochstats.cntSlips = stats->isochstats.slipcnt.current;
	}
	stats->transit.current = stats->transit.total;
	if (stats->framelatency_histogram) {
	    if (sumstats && sumstats->framelatency_histogram) {
	        histogram_add(sumstats->framelatency_histogram, stats->framelatency_histogram);
		sumstats->framelatency_histogram->final = true;
	    }
	    stats->framelatency_histogram->final = true;
	}
    } else if (isIsochronous(stats->common)) {
	stats->isochstats.cntFrames = stats->isochstats.framecnt.current - stats->isochstats.framecnt.prev;
	stats->isochstats.cntFramesMissed = stats->isochstats.framelostcnt.current - stats->isochstats.framelostcnt.prev;
	stats->isochstats.cntSlips = stats->isochstats.slipcnt.current - stats->isochstats.slipcnt.prev;
    }
    if ((stats->output_handler) && !stats->isMaskOutput) {
	(*stats->output_handler)(stats);
	if (isFrameInterval(stats->common) && stats->framelatency_histogram) {
	    histogram_print(stats->framelatency_histogram, stats->ts.iStart, stats->ts.iEnd);
	}
    }
    if (!final)
	reporter_reset_transfer_stats_server_tcp(stats);
}

void reporter_transfer_protocol_client_tcp (struct ReporterData *data, bool final) {
    struct TransferInfo *stats = &data->info;
    struct TransferInfo *sumstats = (data->GroupSumReport != NULL) ? &data->GroupSumReport->info : NULL;
    struct TransferInfo *fullduplexstats = (data->FullDuplexReport != NULL) ? &data->FullDuplexReport->info : NULL;
    stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
    if (stats->latency_histogram) {
        stats->latency_histogram->final = final;
    }
    if (stats->write_histogram) {
        stats->write_histogram->final = final;
    }
    if (isIsochronous(stats->common)) {
	if (final) {
	    stats->isochstats.cntFrames = stats->isochstats.framecnt.current;
	    stats->isochstats.cntFramesMissed = stats->isochstats.framelostcnt.current;
	    stats->isochstats.cntSlips = stats->isochstats.slipcnt.current;
	} else {
	    stats->isochstats.cntFrames = stats->isochstats.framecnt.current - stats->isochstats.framecnt.prev;
	    stats->isochstats.cntFramesMissed = stats->isochstats.framelostcnt.current - stats->isochstats.framelostcnt.prev;
	    stats->isochstats.cntSlips = stats->isochstats.slipcnt.current - stats->isochstats.slipcnt.prev;
	}
    }
    if (sumstats) {
	sumstats->total.Bytes.current += stats->cntBytes;
	sumstats->sock_callstats.write.WriteErr += stats->sock_callstats.write.WriteErr;
	sumstats->sock_callstats.write.WriteCnt += stats->sock_callstats.write.WriteCnt;
	sumstats->sock_callstats.write.totWriteErr += stats->sock_callstats.write.WriteErr;
	sumstats->sock_callstats.write.totWriteCnt += stats->sock_callstats.write.WriteCnt;
	if (final) {
	    sumstats->threadcnt_final++;
	    if (data->packetring->downlevel != sumstats->downlevel) {
		sumstats->slot_thread_downcount++;
	    }
	    if (data->packetring->uplevel != sumstats->uplevel){
		sumstats->slot_thread_upcount++;
	    }
#if HAVE_SUMMING_DEBUG
	    printf("**** %s downcnt (%p) (up/down)=%d/%d final true level (sum/pkt)=%d/%d\n", stats->common->transferIDStr, (void *)data->packetring, \
		   sumstats->slot_thread_upcount, sumstats->slot_thread_downcount, \
		   sumstats->uplevel, data->packetring->uplevel);
#endif
	}
#if HAVE_SUMMING_DEBUG
	reporter_dump_timestamps(NULL, stats, sumstats, final);
#endif

#if HAVE_TCP_STATS
	sumstats->sock_callstats.write.tcpstats.retry += stats->sock_callstats.write.tcpstats.retry;
	sumstats->sock_callstats.write.tcpstats.retry_tot += stats->sock_callstats.write.tcpstats.retry;
#endif
    }
    if (fullduplexstats) {
	fullduplexstats->total.Bytes.current += stats->cntBytes;
    }
    if (final) {
	if (stats->latency_histogram) {
	    stats->latency_histogram->final = true;
	}
	if (stats->write_histogram) {
	    stats->write_histogram->final = true;
	}
	if ((stats->cntBytes > 0) && stats->output_handler && !TimeZero(stats->ts.intervalTime)) {
	    // print a partial interval report if enable and this a final
	    if ((stats->output_handler) && !(stats->isMaskOutput)) {
		if (isIsochronous(stats->common)) {
		    stats->isochstats.cntFrames = stats->isochstats.framecnt.current - stats->isochstats.framecnt.prev;
		    stats->isochstats.cntFramesMissed = stats->isochstats.framelostcnt.current - stats->isochstats.framelostcnt.prev;
		    stats->isochstats.cntSlips = stats->isochstats.slipcnt.current - stats->isochstats.slipcnt.prev;
		}
		reporter_set_timestamps_time(stats, FINALPARTIAL);
		if ((stats->ts.iEnd - stats->ts.iStart) > stats->ts.significant_partial)
		    (*stats->output_handler)(stats);
		reporter_reset_transfer_stats_client_tcp(stats);
	    }
        }
	if (isIsochronous(stats->common)) {
	    stats->isochstats.cntFrames = stats->isochstats.framecnt.current;
	    stats->isochstats.cntFramesMissed = stats->isochstats.framelostcnt.current;
	    stats->isochstats.cntSlips = stats->isochstats.slipcnt.current;
	}
	stats->sock_callstats.write.WriteErr = stats->sock_callstats.write.totWriteErr;
	stats->sock_callstats.write.WriteCnt = stats->sock_callstats.write.totWriteCnt;
#if HAVE_TCP_STATS
	stats->sock_callstats.write.tcpstats.retry = stats->sock_callstats.write.tcpstats.retry_tot;
#endif
	if (stats->framelatency_histogram) {
	    stats->framelatency_histogram->final = true;
	}
	stats->cntBytes = stats->total.Bytes.current;
	stats->write_mmm.current = stats->write_mmm.total;
	reporter_set_timestamps_time(stats, TOTAL);
    } else if (isIsochronous(stats->common)) {
	stats->isochstats.cntFrames = stats->isochstats.framecnt.current - stats->isochstats.framecnt.prev;
	stats->isochstats.cntFramesMissed = stats->isochstats.framelostcnt.current - stats->isochstats.framelostcnt.prev;
	stats->isochstats.cntSlips = stats->isochstats.slipcnt.current - stats->isochstats.slipcnt.prev;
    }
    if ((stats->output_handler) && !(stats->isMaskOutput)) {
	(*stats->output_handler)(stats);
    }
    if (!final)
	reporter_reset_transfer_stats_client_tcp(stats);
}

/*
 * Handles summing of threads
 */
void reporter_transfer_protocol_sum_client_tcp (struct TransferInfo *stats, bool final) {
    if (!final || (final && (stats->cntBytes > 0) && !TimeZero(stats->ts.intervalTime))) {
	stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
	if (final) {
	    if ((stats->output_handler) && !(stats->isMaskOutput)) {
		reporter_set_timestamps_time(stats, FINALPARTIAL);
		if ((stats->ts.iEnd - stats->ts.iStart) > stats->ts.significant_partial) {
		    (*stats->output_handler)(stats);
		}
		reporter_reset_transfer_stats_client_tcp(stats);
	    }
	} else if ((stats->output_handler) && !(stats->isMaskOutput)) {
	    (*stats->output_handler)(stats);
	    reporter_reset_transfer_stats_sum(stats);
	}
	reporter_reset_transfer_stats_client_tcp(stats);
    }
    if (final) {
	stats->sock_callstats.write.WriteErr = stats->sock_callstats.write.totWriteErr;
	stats->sock_callstats.write.WriteCnt = stats->sock_callstats.write.totWriteCnt;
#if HAVE_TCP_STATS
	stats->sock_callstats.write.tcpstats.retry = stats->sock_callstats.write.tcpstats.retry_tot;
#endif
	stats->cntBytes = stats->total.Bytes.current;
	reporter_set_timestamps_time(stats, TOTAL);
	if ((stats->output_handler) && !(stats->isMaskOutput))
	    (*stats->output_handler)(stats);
    }
}

void reporter_transfer_protocol_client_bb_tcp (struct ReporterData *data, bool final) {
    struct TransferInfo *stats = &data->info;
    struct TransferInfo *sumstats = (data->GroupSumReport != NULL) ? &data->GroupSumReport->info : NULL;
    if (final) {
	if ((stats->cntBytes > 0) && stats->output_handler && !TimeZero(stats->ts.intervalTime)) {
	    // print a partial interval report if enable and this a final
	    if ((stats->output_handler) && !(stats->isMaskOutput)) {
		reporter_set_timestamps_time(stats, FINALPARTIAL);
		if ((stats->ts.iEnd - stats->ts.iStart) > stats->ts.significant_partial)
		    (*stats->output_handler)(stats);
		reporter_reset_transfer_stats_client_tcp(stats);
	    }
        }
#if HAVE_TCP_STATS
	stats->sock_callstats.write.tcpstats.retry = stats->sock_callstats.write.tcpstats.retry_tot;
#endif
	stats->cntBytes = stats->total.Bytes.current;
	stats->cntTxBytes = stats->total.TxBytes.current;
	stats->cntRxBytes = stats->total.RxBytes.current;
	reporter_set_timestamps_time(stats, TOTAL);
    } else {
	stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
	stats->cntRxBytes = stats->total.RxBytes.current - stats->total.RxBytes.prev;
	stats->cntTxBytes = stats->total.TxBytes.current - stats->total.TxBytes.prev;
    }
    if (sumstats) {
	if (!final) {
	    sumstats->total.Bytes.current += stats->cntBytes;
	    sumstats->sock_callstats.write.WriteErr += stats->sock_callstats.write.WriteErr;
	    sumstats->sock_callstats.write.WriteCnt += stats->sock_callstats.write.WriteCnt;
	    sumstats->sock_callstats.write.totWriteErr += stats->sock_callstats.write.WriteErr;
	    sumstats->sock_callstats.write.totWriteCnt += stats->sock_callstats.write.WriteCnt;
#if HAVE_TCP_STATS
	    sumstats->sock_callstats.write.tcpstats.retry += stats->sock_callstats.write.tcpstats.retry;
	    sumstats->sock_callstats.write.tcpstats.retry_tot += stats->sock_callstats.write.tcpstats.retry;
#endif
	} else {
	    sumstats->threadcnt_final++;
	    if (data->packetring->downlevel != sumstats->downlevel) {
		sumstats->slot_thread_downcount++;
	    }
	    if (data->packetring->uplevel != sumstats->uplevel){
		sumstats->slot_thread_upcount++;
	    }
#if HAVE_SUMMING_DEBUG
	    printf("**** %s downcnt (%p) (up/down)=%d/%d final true level (sum/pkt)=%d/%d\n", stats->common->transferIDStr, (void *)data->packetring, \
		   sumstats->slot_thread_upcount, sumstats->slot_thread_downcount, \
		   sumstats->uplevel, data->packetring->uplevel);
#endif
	}
#if HAVE_SUMMING_DEBUG
	reporter_dump_timestamps(NULL, stats, sumstats, final);
#endif
    }

    if ((stats->output_handler) && !(stats->isMaskOutput))
        (*stats->output_handler)(stats);
    if (!final) {
	reporter_reset_transfer_stats_client_tcp(stats);
    }
}

void reporter_transfer_protocol_sum_server_tcp (struct TransferInfo *stats, bool final) {
    if (!final || (final && (stats->cntBytes > 0) && !TimeZero(stats->ts.intervalTime))) {
	stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
	stats->sock_callstats.read.cntRead = stats->sock_callstats.read.ReadCnt.current - stats->sock_callstats.read.ReadCnt.prev;
	if (final) {
	    if ((stats->output_handler) && !(stats->isMaskOutput)) {
		reporter_set_timestamps_time(stats, FINALPARTIAL);
		if ((stats->ts.iEnd - stats->ts.iStart) > stats->ts.significant_partial) {
		    (*stats->output_handler)(stats);
		}
	    }
	} else if ((stats->output_handler) && !(stats->isMaskOutput)) {
	    (*stats->output_handler)(stats);
	    reporter_reset_transfer_stats_sum(stats);
	}
	reporter_reset_transfer_stats_server_tcp(stats);
    }
    if (final) {
	int ix;
	stats->cntBytes = stats->total.Bytes.current;
	stats->sock_callstats.read.cntRead = stats->sock_callstats.read.ReadCnt.current;
	for (ix = 0; ix < TCPREADBINCOUNT; ix++) {
	    stats->sock_callstats.read.bins[ix] = stats->sock_callstats.read.totbins[ix];
	}
	stats->cntBytes = stats->total.Bytes.current;
	reporter_set_timestamps_time(stats, TOTAL);
	if ((stats->output_handler) && !(stats->isMaskOutput))
	    (*stats->output_handler)(stats);
    }
}
void reporter_transfer_protocol_fullduplex_tcp (struct TransferInfo *stats, bool final) {
    if (!final || (final && (stats->cntBytes > 0) && !TimeZero(stats->ts.intervalTime))) {
	stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
	if (final) {
	    if ((stats->output_handler) && !(stats->isMaskOutput)) {
		reporter_set_timestamps_time(stats, FINALPARTIAL);
		if ((stats->ts.iEnd - stats->ts.iStart) > stats->ts.significant_partial)
		    (*stats->output_handler)(stats);
	    }
	}
	stats->total.Bytes.prev = stats->total.Bytes.current;
    }
    if (final) {
	stats->cntBytes = stats->total.Bytes.current;
	reporter_set_timestamps_time(stats, TOTAL);
    } else {
	reporter_set_timestamps_time(stats, INTERVAL);
    }
    if ((stats->output_handler) && !(stats->isMaskOutput))
	(*stats->output_handler)(stats);
}

void reporter_transfer_protocol_fullduplex_udp (struct TransferInfo *stats, bool final) {
    if (!final || (final && (stats->cntBytes > 0) && !TimeZero(stats->ts.intervalTime))) {
	stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
	stats->cntDatagrams = stats->total.Datagrams.current - stats->total.Datagrams.prev;
	stats->cntIPG = stats->total.IPG.current - stats->total.IPG.prev;
	if (final) {
	    if ((stats->output_handler) && !(stats->isMaskOutput)) {
		reporter_set_timestamps_time(stats, FINALPARTIAL);
		if ((stats->ts.iEnd - stats->ts.iStart) > stats->ts.significant_partial)
		    (*stats->output_handler)(stats);
	    }
	}
	stats->total.Bytes.prev = stats->total.Bytes.current;
	stats->total.IPG.prev = stats->total.IPG.current;
	stats->total.Datagrams.prev = stats->total.Datagrams.current;
    }
    if (final) {
	stats->cntBytes = stats->total.Bytes.current;
	stats->cntBytes = stats->total.Bytes.current;
	stats->cntDatagrams = stats->total.Datagrams.current ;
	stats->cntIPG = stats->total.IPG.current;
	stats->IPGsum = TimeDifference(stats->ts.packetTime, stats->ts.startTime);
	reporter_set_timestamps_time(stats, TOTAL);
    } else {
	reporter_set_timestamps_time(stats, INTERVAL);
    }
    if ((stats->output_handler) && !(stats->isMaskOutput))
	(*stats->output_handler)(stats);
    if (stats->cntDatagrams)
        stats->IPGsum = 0.0;
}

// Conditional print based on time
bool reporter_condprint_time_interval_report (struct ReporterData *data, struct ReportStruct *packet) {
    struct TransferInfo *stats = &data->info;
    assert(stats!=NULL);
    //   printf("***sum handler = %p\n", (void *) data->GroupSumReport->transfer_protocol_sum_handler);
    bool advance_jobq = false;
    // Print a report if packet time exceeds the next report interval time,
    // Also signal to the caller to move to the next report (or packet ring)
    // if there was output. This will allow for more precise interval sum accounting.
    // printf("***** pt = %ld.%06ld next = %ld.%06ld\n", packet->packetTime.tv_sec, packet->packetTime.tv_usec, stats->ts.nextTime.tv_sec, stats->ts.nextTime.tv_usec);
    // printf("***** nt %ld.%06ld pt %ld.%06ld pid=%lld empty=%d\n", stats->ts.nextTime.tv_sec, stats->ts.nextTime.tv_usec, packet->packetTime.tv_sec, packet->packetTime.tv_usec, packet->packetID, packet->emptyreport);
    if (TimeDifference(stats->ts.nextTime, packet->packetTime) < 0) {
	assert(data->transfer_protocol_handler!=NULL);
	advance_jobq = true;
	struct TransferInfo *sumstats = (data->GroupSumReport ? &data->GroupSumReport->info : NULL);
	struct TransferInfo *fullduplexstats = (data->FullDuplexReport ? &data->FullDuplexReport->info : NULL);
	stats->ts.packetTime = packet->packetTime;
#if DEBUG_PPS
	printf("*** packetID TRIGGER = %ld pt=%ld.%06ld empty=%d nt=%ld.%06ld carry %f\n",packet->packetID, packet->packetTime.tv_sec, packet->packetTime.tv_usec, packet->emptyreport, stats->ts.nextTime.tv_sec, stats->ts.nextTime.tv_usec, stats->IPGsumcarry);
#endif
	reporter_set_timestamps_time(stats, INTERVAL);
	(*data->transfer_protocol_handler)(data, 0);
	if (fullduplexstats && ((++data->FullDuplexReport->threads) == 2) && isEnhanced(stats->common)) {
	    data->FullDuplexReport->threads = 0;
	    assert(data->FullDuplexReport->transfer_protocol_sum_handler != NULL);
	    (*data->FullDuplexReport->transfer_protocol_sum_handler)(fullduplexstats, false);
	}
	if (sumstats) {
	    if (data->packetring->downlevel != sumstats->downlevel)	{
		sumstats->slot_thread_downcount++;
		data->packetring->downlevel = toggleLevel(data->packetring->downlevel);
#if HAVE_SUMMING_DEBUG
		printf("**** %s downcnt (%p) pkt=%ld.%06ld (up/down)=%d/%d final false level (sum/pkt)=%d/%d\n", stats->common->transferIDStr, (void *)data->packetring, \
		       (long) packet->packetTime.tv_sec, (long) packet->packetTime.tv_usec, sumstats->slot_thread_upcount, sumstats->slot_thread_downcount, \
		       sumstats->uplevel, data->packetring->uplevel);
#endif
	    }
	    if ((sumstats->slot_thread_downcount) == sumstats->slot_thread_upcount) {
		data->GroupSumReport->threads = 0;
		if ((data->GroupSumReport->reference.count > (fullduplexstats ? 2 : 1)) || \
		    isSumOnly(data->info.common)) {
		    sumstats->isMaskOutput = false;
		} else {
		    sumstats->isMaskOutput = true;
		}
#if HAVE_SUMMING_DEBUG
		reporter_dump_timestamps(packet, stats, sumstats, sumstats->final);
#endif
		reporter_set_timestamps_time(sumstats, INTERVAL);
		assert(data->GroupSumReport->transfer_protocol_sum_handler != NULL);
		(*data->GroupSumReport->transfer_protocol_sum_handler)(sumstats, false);
	    }
	}
        // In the (hopefully unlikely event) the reporter fell behind
        // output the missed reports to catch up
	if ((stats->output_handler) && !(stats->isMaskOutput))
	    reporter_transfer_protocol_missed_reports(stats, packet);
    }
    return advance_jobq;
}

// Conditional print based on bursts or frames
bool reporter_condprint_frame_interval_report_server_udp (struct ReporterData *data, struct ReportStruct *packet) {
    struct TransferInfo *stats = &data->info;
    bool advance_jobq = false;
    // first packet of a burst and not a duplicate
    if ((packet->burstsize == (packet->remaining + packet->packetLen)) && (stats->matchframeID != packet->frameID)) {
	stats->matchframeID=packet->frameID;
    }
    if ((packet->packetLen == packet->remaining) && (packet->frameID == stats->matchframeID)) {
	if ((stats->ts.iStart = TimeDifference(stats->ts.nextTime, stats->ts.startTime)) < 0)
	    stats->ts.iStart = 0.0;
	stats->frameID = packet->frameID;
	stats->ts.iEnd = TimeDifference(packet->packetTime, stats->ts.startTime);
	stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
	stats->cntOutofOrder = stats->total.OutofOrder.current - stats->total.OutofOrder.prev;
	// assume most of the  time out-of-order packets are not
	// duplicate packets, so conditionally subtract them from the lost packets.
	stats->cntError = stats->total.Lost.current - stats->total.Lost.prev;
	stats->cntError -= stats->cntOutofOrder;
	if (stats->cntError < 0)
	    stats->cntError = 0;
	stats->cntDatagrams = stats->PacketID - stats->total.Datagrams.prev;
	if ((stats->output_handler) && !(stats->isMaskOutput))
	    (*stats->output_handler)(stats);
	reporter_reset_transfer_stats_server_udp(stats);
	advance_jobq = true;
    }
    return advance_jobq;
}

bool reporter_condprint_frame_interval_report_server_tcp (struct ReporterData *data, struct ReportStruct *packet) {
    fprintf(stderr, "FIX ME\n");
    return true;
}

bool reporter_condprint_burst_interval_report_server_tcp (struct ReporterData *data, struct ReportStruct *packet) {
    struct TransferInfo *stats = &data->info;
    int advance_jobq = false;
    if (packet->transit_ready) {
	stats->ts.prevpacketTime = packet->sentTime;
	stats->ts.packetTime = packet->packetTime;
	reporter_set_timestamps_time(stats, INTERVALPARTIAL);
	stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
	stats->sock_callstats.read.cntRead = stats->sock_callstats.read.ReadCnt.current - stats->sock_callstats.read.ReadCnt.prev;
	if ((stats->output_handler) && !(stats->isMaskOutput))
	    (*stats->output_handler)(stats);
	reporter_reset_transfer_stats_server_tcp(stats);
	advance_jobq = true;
    }
    return advance_jobq;
}

bool reporter_condprint_burst_interval_report_client_tcp (struct ReporterData *data, struct ReportStruct *packet) {
    struct TransferInfo *stats = &data->info;
    int advance_jobq = false;
    // first packet of a burst and not a duplicate
    if (packet->transit_ready) {
        reporter_handle_packet_oneway_transit(stats, packet);
//	printf("****sndpkt=%ld.%06ld rxpkt=%ld.%06ld\n", packet->sentTime.tv_sec, packet->sentTime.tv_usec, packet->packetTime.tv_sec,packet->packetTime.tv_usec);
	stats->ts.prevpacketTime = packet->prevSentTime;
	stats->ts.packetTime = packet->packetTime;
	reporter_set_timestamps_time(stats, INTERVALPARTIAL);
	stats->cntBytes = stats->total.Bytes.current - stats->total.Bytes.prev;
	if ((stats->output_handler) && !(stats->isMaskOutput))
	    (*stats->output_handler)(stats);
	reporter_reset_transfer_stats_client_tcp(stats);
	advance_jobq = true;
    }
    return advance_jobq;
}

int post_connection_error (struct thread_Settings *inSettings, struct timeval t) {
    char timestr[120];
    char tmpaddr[200];
    char errtext[50];
    int connect_errno = errno_decode(errtext, sizeof(errtext));
    unsigned short port = SockAddr_getPort(&inSettings->peer);
    SockAddr_getHostAddress(&inSettings->peer, tmpaddr, sizeof(tmpaddr));
    iperf_formattime(timestr, sizeof(timestr), t, isEnhanced(inSettings), isUTC(inSettings), YearThruSecTZ);
    int slen = snprintf(NULL, 0, "%stcp connect to %s port %" PRIu16 " failed (%s) on %s", \
			inSettings->mTransferIDStr, tmpaddr, port, errtext, timestr);
    char *text = (char *) calloc((slen+1), sizeof(char));
    if (text) {
	snprintf(text, (slen+1), "%stcp connect to %s port %" PRIu16 " failed (%s) on %s", \
		 inSettings->mTransferIDStr, tmpaddr, port, errtext, timestr);
	PostReport(InitErrorReport(text));
	FREE_ARRAY(text);
    }
    return connect_errno;
}
void post_connect_timer_expired (struct thread_Settings *inSettings, struct timeval t) {
    char timestr[120];
    iperf_formattime(timestr, sizeof(timestr), t, isEnhanced(inSettings), isUTC(inSettings), YearThruSecTZ);
    int len = snprintf(NULL, 0, "%stcp connect attempt timer expired on %s\n", \
		       inSettings->mTransferIDStr, timestr);
    char *text = (char *) calloc(len+1, sizeof(char));
    if (text) {
	snprintf(text, len, "%stcp connect attempt timer expired on %s\n", \
		 inSettings->mTransferIDStr, timestr);
	PostReport(InitErrorReport(text));
	FREE_ARRAY(text);
    }
}

#ifdef __cplusplus
} /* end extern "C" */
#endif
