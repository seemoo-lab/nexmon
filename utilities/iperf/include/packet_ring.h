/*---------------------------------------------------------------
 * Copyright (c) 2019
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
 * packet_ring.h
 * Suppport for packet rings between threads
 *
 * by Robert J. McMahon (rjmcmahon@rjmcmahon.com, bob.mcmahon@broadcom.com)
 * -------------------------------------------------------------------
 */
#ifndef PACKETRINGC_H
#define PACKETRINGC_H

#include "Condition.h"
#include "gettcpinfo.h"

#ifdef __cplusplus
extern "C" {
#endif

#define ACKRING_DEFAULTSIZE 100

enum ReadWriteExtReturnVals {
    ReadSuccess  = 1,
    ReadTimeo,
    ReadTimeoFatal,
    ReadErrLen,
    ReadNoAccount,
    WriteSuccess,
    WriteSelectRetry,
    WriteErrAccount,
    WriteErrFatal,
    WriteTimeo,
    WriteNoAccount,
    NullEvent
};

enum edgeLevel {
    LOW = 0,
    HIGH = 1
};

struct ReportStruct {
    intmax_t packetID;
    intmax_t packetLen;
    struct timeval packetTime;
    struct timeval prevPacketTime;
    struct timeval sentTime;
    struct timeval prevSentTime;
    enum ReadWriteExtReturnVals err_readwrite;
    bool emptyreport;
    int l2errors;
    int l2len;
    int expected_l2len;
    u_char tos;
    // isochStartTime is overloaded: first write timestamp of the frame or burst w/trip-times or very first read w/o trip-times
    // reporter calculation will compute latency accordingly
    struct timeval isochStartTime;
    uint32_t prevframeID;
    uint32_t frameID;
    uint32_t burstsize;
    uint32_t burstperiod;
    uint32_t remaining;
    bool transit_ready;
    int writecnt;
    intmax_t writeLen;
    intmax_t recvLen;
    long write_time;
    bool scheduled;
    long sched_err;
    struct timeval sentTimeRX;
    struct timeval sentTimeTX;
    struct iperf_tcpstats tcpstats;
#if defined(HAVE_DECL_SO_MAX_PACING_RATE)
    intmax_t FQPacingRate;
#endif
};

struct PacketRing {
    // producer and consumer
    // must be an atomic type, e.g. int
    // otherwise reads/write can be torn
    int producer;
    int consumer;
    int maxcount;
    bool consumerdone;
    int awaitcounter;
    bool mutex_enable;
    int bytes;
    enum edgeLevel uplevel;
    enum edgeLevel downlevel;

    // Use a condition variables
    // o) awake_producer - producer waits for the consumer thread to
    //    make space or end (signaled by the consumer)
    // o) awake_consumer - signal the consumer thread to to run
    //    (signaled by the producer)
    struct Condition *awake_producer;
    struct Condition *awake_consumer;
    struct ReportStruct *data;
};

extern struct PacketRing * packetring_init(int count, struct Condition *awake_consumer, struct Condition *awake_producer);
extern void packetring_enqueue(struct PacketRing *pr, struct ReportStruct *metapacket);
extern struct ReportStruct *packetring_dequeue(struct PacketRing * pr);
extern void enqueue_ackring(struct PacketRing *pr, struct ReportStruct *metapacket);
extern struct ReportStruct *dequeue_ackring(struct PacketRing * pr);
extern void packetring_free(struct PacketRing *pr);
extern void free_ackring(struct PacketRing *pr);
extern enum edgeLevel toggleLevel(enum edgeLevel level);
#ifdef HAVE_THREAD_DEBUG
extern int packetring_getcount(struct PacketRing *pr);
#endif

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif // PACKETRINGC_H
