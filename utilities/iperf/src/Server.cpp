/*---------------------------------------------------------------
 * Copyright (c) 1999,2000,2001,2002,2003
 * The Board of Trustees of the University of Illinois
 * All Rights Reserved.
 *---------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software (Iperf) and associated
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
 * Server.cpp
 * by Mark Gates <mgates@nlanr.net>
 *     Ajay Tirumala (tirumala@ncsa.uiuc.edu>.
 * -------------------------------------------------------------------
 * A server thread is initiated for each connection accept() returns.
 * Handles sending and receiving data, and then closes socket.
 * Changes to this version : The server can be run as a daemon
 * ------------------------------------------------------------------- */

#define HEADERS()

#include "headers.h"
#include "Server.hpp"
#include "active_hosts.h"
#include "Extractor.h"
#include "Reporter.h"
#include "Locale.h"
#include "delay.h"
#include "PerfSocket.hpp"
#include "SocketAddr.h"
#include "payloads.h"
#include "prague_cc.h"
#include <cmath>
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
#include "checksums.h"
#endif

/* -------------------------------------------------------------------
 * Stores connected socket and socket info.
 * ------------------------------------------------------------------- */

Server::Server (thread_Settings *inSettings) {
#ifdef HAVE_THREAD_DEBUG
    thread_debug("Server constructor with thread=%p sum=%p (sock=%d)", (void *) inSettings, (void *)inSettings->mSumReport, inSettings->mSock);
#endif
    mSettings = inSettings;
    myJob = NULL;
    reportstruct = &scratchpad;
    memset(&scratchpad, 0, sizeof(struct ReportStruct));
    mySocket = inSettings->mSock;
    peerclose = false;
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
    myDropSocket = inSettings->mSockDrop;
    if (isL2LengthCheck(mSettings)) {
        // For L2 UDP make sure we can receive a full ethernet packet plus a bit more
        if (mSettings->mBufLen < (2 * ETHER_MAX_LEN)) {
            mSettings->mBufLen = (2 * ETHER_MAX_LEN);
        }
    }
#endif
#if (HAVE_DECL_SO_TIMESTAMP) && (HAVE_DECL_MSG_CTRUNC)
    ctrunc_warn_enable = true;
#endif
    // Enable kernel level timestamping if available
    InitKernelTimeStamping();
    int sorcvtimer = 0;
    // sorcvtimer units microseconds convert to that
    // minterval double, units seconds
    // mAmount integer, units 10 milliseconds
    // divide by two so timeout is 1/2 the interval
    if ((mSettings->mInterval > 0) && (mSettings->mIntervalMode == kInterval_Time)) {
        sorcvtimer = static_cast<int>(round(mSettings->mInterval / 2.0));
    } else if (isServerModeTime(mSettings)) {
        sorcvtimer = static_cast<int>(round(mSettings->mAmount * 10000) / 2);
    }
    isburst = (isIsochronous(mSettings) || isPeriodicBurst(mSettings) || (isTripTime(mSettings)&& !isUDP(mSettings)));
    if (isburst && (mSettings->mFPS > 0.0)) {
        sorcvtimer = static_cast<int>(round(2000000.0 / mSettings->mFPS));
    }
    if ((mSettings->mInterval > 0) && (mSettings->mIntervalMode == kInterval_Time)) {
        int interval_quarter = static_cast<int>(round(mSettings->mAmount * 10000) / 4);
        if (sorcvtimer > interval_quarter) {
            sorcvtimer = interval_quarter;
        }
        if (sorcvtimer < 1000) {
            sorcvtimer = 1000; // lower bound of 1 ms
        }
    }
    if (sorcvtimer > 0) {
        SetSocketOptionsReceiveTimeout(mSettings, sorcvtimer);
    }
}

/* -------------------------------------------------------------------
 * Destructor close socket.
 * ------------------------------------------------------------------- */
Server::~Server () {
#if HAVE_THREAD_DEBUG
    thread_debug("Server destructor sock=%d fullduplex=%s", mySocket, (isFullDuplex(mSettings) ? "true" : "false"));
#endif
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
    if (myDropSocket != INVALID_SOCKET) {
        int rc = close(myDropSocket);
        WARN_errno(rc == SOCKET_ERROR, "server close drop");
        myDropSocket = INVALID_SOCKET;
    }
#endif
}

inline bool Server::InProgress () {
    return !(sInterupted || peerclose ||
             ((isServerModeTime(mSettings) || (isModeTime(mSettings) && isReverse(mSettings))) && mEndTime.before(reportstruct->packetTime)));
}

/* -------------------------------------------------------------------
 * Receive TCP data from the (connected) socket.
 * Sends termination flag several times at the end.
 * Does not close the socket.
 * ------------------------------------------------------------------- */
void Server::RunTCP () {
    long currLen;
    intmax_t totLen = 0;
    struct TCP_burst_payload burst_info; // used to store burst header and report in last packet of burst
    Timestamp time1, time2;
    double tokens=0.000004;

    if (!InitTrafficLoop())
        return;
    myReport->info.ts.prevsendTime = myReport->info.ts.startTime;

    int burst_nleft = 0;
    burst_info.burst_id = 0;
    burst_info.burst_period_us = 0;
    burst_info.send_tt.write_tv_sec = 0;
    burst_info.send_tt.write_tv_usec = 0;
    now.setnow();
    reportstruct->packetTime.tv_sec = now.getSecs();
    reportstruct->packetTime.tv_usec = now.getUsecs();
    while (InProgress()) {
        //	printf("***** bid expect = %u\n", burstid_expect);
        reportstruct->emptyreport = true;
        currLen = 0;
        // perform read
        if (isBWSet(mSettings)) {
            time2.setnow();
            tokens += time2.subSec(time1) * (mSettings->mAppRate / 8.0);
            time1 = time2;
        }
        reportstruct->transit_ready = false;
        if (tokens >= 0.0) {
            int n = 0;
            int readLen = mSettings->mBufLen;
            if (burst_nleft > 0)
                readLen = (mSettings->mBufLen < burst_nleft) ? mSettings->mBufLen : burst_nleft;
            reportstruct->emptyreport = true;
#if HAVE_DECL_TCP_QUICKACK
            if (isTcpQuickAck(mSettings)) {
                int opt = 1;
                Socklen_t len = sizeof(opt);
                int rc = setsockopt(mySocket, IPPROTO_TCP, TCP_QUICKACK,
                                    reinterpret_cast<char*>(&opt), len);
                WARN_errno(rc == SOCKET_ERROR, "setsockopt TCP_QUICKACK");
            }
#endif
            if (isburst && (burst_nleft == 0)) {
                if ((n = recvn(mSettings->mSock, reinterpret_cast<char *>(&burst_info), sizeof(struct TCP_burst_payload), 0)) == sizeof(struct TCP_burst_payload)) {
                    // burst_info.typelen.type = ntohl(burst_info.typelen.type);
                    // burst_info.typelen.length = ntohl(burst_info.typelen.length);
                    // This is the first stamp of the burst
                    burst_info.flags = ntohl(burst_info.flags);
                    burst_info.burst_size = ntohl(burst_info.burst_size);
                    assert(burst_info.burst_size > 0);
                    reportstruct->burstsize = burst_info.burst_size;
                    burst_info.burst_id = ntohl(burst_info.burst_id);
                    reportstruct->frameID = burst_info.burst_id;
                    if (isTripTime(mSettings)) {
                        burst_info.send_tt.write_tv_sec = ntohl(burst_info.send_tt.write_tv_sec);
                        burst_info.send_tt.write_tv_usec = ntohl(burst_info.send_tt.write_tv_usec);
                    } else if (isIsochronous(mSettings)) {
                        burst_info.send_tt.write_tv_sec = (uint32_t)myReport->info.ts.startTime.tv_sec;
                        burst_info.send_tt.write_tv_usec = (uint32_t)myReport->info.ts.startTime.tv_usec;
                        burst_info.burst_period_us = ntohl(burst_info.burst_period_us);
                    } else {
                        now.setnow();
                        burst_info.send_tt.write_tv_sec = (uint32_t)now.getSecs();
                        burst_info.send_tt.write_tv_usec = (uint32_t)now.getUsecs();
                    }
                    reportstruct->sentTime.tv_sec = burst_info.send_tt.write_tv_sec;
                    reportstruct->sentTime.tv_usec = burst_info.send_tt.write_tv_usec;
                    myReport->info.ts.prevsendTime = reportstruct->sentTime;
                    burst_nleft = burst_info.burst_size - n;
                    if (burst_nleft == 0) {
                        reportstruct->prevSentTime = myReport->info.ts.prevsendTime;
                        reportstruct->transit_ready = true;
                        reportstruct->burstperiod = burst_info.burst_period_us;
                    }
                    currLen += n;
                    readLen = (mSettings->mBufLen < burst_nleft) ? mSettings->mBufLen : burst_nleft;
                    WARN(burst_nleft <= 0, "invalid burst read req size");
                    // thread_debug("***read burst header size %d id=%d", burst_info.burst_size, burst_info.burst_id);
                } else {
                    if (n > 0) {
                        WARN(1, "partial readn");
#ifdef HAVE_THREAD_DEBUG
                        thread_debug("TCP burst partial read of %d wanted %d", n, sizeof(struct TCP_burst_payload));
                    } else {
                        thread_debug("Detected peer close");
#endif
                    }
                    goto Done;
                }
            }
            if (!reportstruct->transit_ready) {
#if HAVE_DECL_MSG_TRUNC
                int recvflags = ((!isSkipRxCopy(mSettings) || (isburst && (burst_nleft > 0))) ? 0 : MSG_TRUNC);
#else
                int recvflags = 0;
#endif
                n = recv(mSettings->mSock, mSettings->mBuf, readLen, recvflags);
                if (n > 0) {
                    reportstruct->emptyreport = false;
                    if (isburst) {
                        burst_nleft -= n;
                        if (burst_nleft == 0) {
                            reportstruct->sentTime = myReport->info.ts.prevsendTime;
                            if (isTripTime(mSettings) || isIsochronous(mSettings)) {
                                reportstruct->isochStartTime.tv_sec = burst_info.send_tt.write_tv_sec;
                                reportstruct->isochStartTime.tv_usec = burst_info.send_tt.write_tv_usec;
                                reportstruct->burstperiod = burst_info.burst_period_us;
                            }
                            reportstruct->transit_ready = true;
                        }
                    }
                } else if (n == 0) {
                    peerclose = true;
#ifdef HAVE_THREAD_DEBUG
                    thread_debug("Server thread detected EOF on socket %d", mSettings->mSock);
#endif
                } else if ((n < 0) && (FATALTCPREADERR(errno))) {
                    peerclose = true;
                    n = 0;
                    now.setnow();
                    char warnbuf[WARNBUFSIZE];
                    snprintf(warnbuf, sizeof(warnbuf), "%stcp recv",\
                             mSettings->mTransferIDStr);
                    warnbuf[sizeof(warnbuf)-1] = '\0';
                    WARN_errno(1, warnbuf);
                }
                currLen += n;
            }
            now.setnow();
            reportstruct->packetTime.tv_sec = now.getSecs();
            reportstruct->packetTime.tv_usec = now.getUsecs();
            totLen += currLen;
            if (isBWSet(mSettings))
                tokens -= currLen;

            reportstruct->packetLen = currLen;
            ReportPacket(myReport, reportstruct);
            // Check for reverse and amount where
            // the server stops after receiving
            // the expected byte count
            if (isReverse(mSettings) && !isModeTime(mSettings) && (totLen >= static_cast<intmax_t>(mSettings->mAmount))) {
                break;
            }
        } else {
            // Use a 4 usec delay to fill tokens
            delay_loop(4);
        }
    }
 Done:
    disarm_itimer();
    // stop timing
    now.setnow();
    reportstruct->packetTime.tv_sec = now.getSecs();
    reportstruct->packetTime.tv_usec = now.getUsecs();
    reportstruct->packetLen = 0;
    if (EndJob(myJob, reportstruct)) {
#if HAVE_THREAD_DEBUG
        thread_debug("tcp close sock=%d", mySocket);
#endif
        int rc = close(mySocket);
        WARN_errno(rc == SOCKET_ERROR, "server close");
    }
    Iperf_remove_host(mSettings);
    FreeReport(myJob);
}

void Server::PostNullEvent () {
    assert(myReport!=NULL);
    // push a nonevent into the packet ring
    // this will cause the reporter to process
    // up to this event
    struct ReportStruct report_nopacket;
    memset(&report_nopacket, 0, sizeof(struct ReportStruct));
    now.setnow();
    report_nopacket.packetTime.tv_sec = now.getSecs();
    report_nopacket.packetTime.tv_usec = now.getUsecs();
    report_nopacket.emptyreport = true;
    report_nopacket.err_readwrite = WriteNoAccount;
    reportstruct->packetTime = report_nopacket.packetTime; // needed for the InProgress loop test
    ReportPacket(myReport, &report_nopacket);
}

inline bool Server::ReadBBWithRXTimestamp () {
    bool rc = false;
    int n;
    while (InProgress()) {
        int read_offset = 0;
    RETRY_READ :
        n = recvn(mySocket, (mSettings->mBuf + read_offset), (mSettings->mBounceBackBytes - read_offset), 0);
        if (n > 0) {
            read_offset += n;
            if (read_offset == mSettings->mBounceBackBytes) {
                struct bounceback_hdr *bbhdr = reinterpret_cast<struct bounceback_hdr *>(mSettings->mBuf);
                uint16_t bbflags = ntohs(bbhdr->bbflags);
                now.setnow();
                reportstruct->packetTime.tv_sec = now.getSecs();
                reportstruct->packetTime.tv_usec = now.getUsecs();
                reportstruct->emptyreport = false;
                reportstruct->packetLen = mSettings->mBounceBackBytes;
                // write the rx timestamp back into the payload
                bbhdr->bbserverRx_ts.sec = htonl(reportstruct->packetTime.tv_sec);
                bbhdr->bbserverRx_ts.usec = htonl(reportstruct->packetTime.tv_usec);
                ReportPacket(myReport, reportstruct);
                if (!(bbflags & HEADER_BBSTOP)) {
                    rc = true;
                } else {
                    // last BB write received from client, false return code stops this side
                }
                break;
            }
        } else if (n == 0) {
            peerclose = true;
        } else if (n == IPERF_SOCKET_ERROR_NONFATAL) {
            PostNullEvent();
            if (InProgress())
                goto RETRY_READ;
        } else {
            if (FATALTCPREADERR(errno)) {
                WARN_errno(1, "fatal bounceback read");
                peerclose = true;
                break;
            } else {
                WARN(1, "timeout: bounceback read");
                PostNullEvent();
                if (InProgress())
                    goto RETRY_READ;
            }
        }
    }
    return rc;
}

inline bool Server::WriteBB () {
    int n;
    bool rc = false;
    struct bounceback_hdr *bbhdr = reinterpret_cast<struct bounceback_hdr *>(mSettings->mBuf);
    now.setnow();
    bbhdr->bbserverTx_ts.sec = htonl(now.getSecs());
    bbhdr->bbserverTx_ts.usec = htonl(now.getUsecs());
    if (mSettings->mTOS) {
        bbhdr->tos = htons((uint16_t)(mSettings->mTOS & 0xFF));
    }
    int write_offset = 0;
    reportstruct->writecnt = 0;
    int writelen = mSettings->mBounceBackReplyBytes;
    while (InProgress()) {
        n = writen(mySocket, (mSettings->mBuf + write_offset), (writelen - write_offset), &reportstruct->writecnt);
        if (n < 0) {
            if (FATALTCPWRITERR(errno)) {
                reportstruct->err_readwrite=WriteErrFatal;
                FAIL_errno(1, "tcp bounceback writen", mSettings);
                peerclose = true;
                break;
            } else {
                PostNullEvent();
                continue;
            }
        }
        write_offset += n;
        if (write_offset < writelen) {
            WARN_errno(1, "tcp bounceback writen incomplete");
            PostNullEvent();
            continue;
        }
        reportstruct->emptyreport = false;
        reportstruct->err_readwrite=WriteSuccess;
        reportstruct->packetLen = writelen;
        return true;
    }
    return rc;
}

void Server::RunBounceBackTCP () {
    if (!InitTrafficLoop())
        return;
#if HAVE_DECL_TCP_NODELAY
    {
        int nodelay = 1;
        // set TCP nodelay option
        int rc = setsockopt(mySocket, IPPROTO_TCP, TCP_NODELAY,
                            reinterpret_cast<char*>(&nodelay), sizeof(nodelay));
        WARN_errno(rc == SOCKET_ERROR, "setsockopt BB TCP_NODELAY");
        setNoDelay(mSettings);
    }
#endif
    if (mSettings->mInterval && (mSettings->mIntervalMode == kInterval_Time)) {
        int sotimer = static_cast<int>(round(mSettings->mInterval / 2.0));
        SetSocketOptionsSendTimeout(mSettings, sotimer);
        SetSocketOptionsReceiveTimeout(mSettings, sotimer);
    } else if (isModeTime(mSettings)) {
        int sotimer = static_cast<int>(round(mSettings->mAmount * 10000) / 2);
        SetSocketOptionsSendTimeout(mSettings, sotimer);
        SetSocketOptionsReceiveTimeout(mSettings, sotimer);
    }
    myReport->info.ts.prevsendTime = myReport->info.ts.startTime;
    now.setnow();
    reportstruct->packetTime.tv_sec = now.getSecs();
    reportstruct->packetTime.tv_usec = now.getUsecs();
    reportstruct->packetLen = mSettings->mBounceBackBytes;
    reportstruct->emptyreport = false;
    ReportPacket(myReport, reportstruct);

    int rc;
    while (InProgress() && (rc = WriteBB())) {
        if (rc) {
            ReportPacket(myReport, reportstruct);
            if (ReadBBWithRXTimestamp())
                continue;
            else {
                break;
            }
        } else {
            break;
        }
    }
    disarm_itimer();
    // stop timing
    now.setnow();
    reportstruct->packetTime.tv_sec = now.getSecs();
    reportstruct->packetTime.tv_usec = now.getUsecs();
    reportstruct->packetLen = 0;
    if (EndJob(myJob, reportstruct)) {
#if HAVE_THREAD_DEBUG
        thread_debug("tcp close sock=%d", mySocket);
#endif
        int rc = close(mySocket);
        WARN_errno(rc == SOCKET_ERROR, "server close");
    }
    Iperf_remove_host(mSettings);
    FreeReport(myJob);
}

void Server::InitKernelTimeStamping () {
#if HAVE_DECL_SO_TIMESTAMP
    iov[0].iov_base=mSettings->mBuf;
    iov[0].iov_len=mSettings->mBufLen;

    message.msg_iov=iov;
    message.msg_iovlen=1;
    message.msg_name=&srcaddr;
    message.msg_namelen=sizeof(srcaddr);

    message.msg_control = (char *) ctrl;
    message.msg_controllen = sizeof(ctrl);

    int timestampOn = 1;
    if (setsockopt(mSettings->mSock, SOL_SOCKET, SO_TIMESTAMP, &timestampOn, sizeof(timestampOn)) < 0) {
        WARN_errno(mSettings->mSock == SO_TIMESTAMP, "socket");
    }
#endif
}

//
// Set the report start times and next report times, options
// are now, the accept time or the first write time
//
inline void Server::SetFullDuplexReportStartTime () {
    assert(myReport->FullDuplexReport != NULL);
    struct TransferInfo *fullduplexstats = &myReport->FullDuplexReport->info;
    assert(fullduplexstats != NULL);
    if (TimeZero(fullduplexstats->ts.startTime)) {
        fullduplexstats->ts.startTime = myReport->info.ts.startTime;
        if (isModeTime(mSettings)) {
            fullduplexstats->ts.nextTime = myReport->info.ts.nextTime;
        }
    }
#ifdef HAVE_THREAD_DEBUG
    thread_debug("Server fullduplex report start=%ld.%06ld next=%ld.%06ld", fullduplexstats->ts.startTime.tv_sec, fullduplexstats->ts.startTime.tv_usec, fullduplexstats->ts.nextTime.tv_sec, fullduplexstats->ts.nextTime.tv_usec);
#endif
}

inline void Server::SetReportStartTime () {
    if (TimeZero(myReport->info.ts.startTime)) {
        if (isTripTime(mSettings) && !TimeZero(mSettings->sent_time) && !isTxStartTime(mSettings)) {
            // Servers that aren't full duplex use the accept timestamp for start
            myReport->info.ts.startTime.tv_sec = mSettings->sent_time.tv_sec;
            myReport->info.ts.startTime.tv_usec = mSettings->sent_time.tv_usec;
        } else if (!TimeZero(mSettings->accept_time)) {
            // Servers that aren't full duplex use the accept timestamp for start
            myReport->info.ts.startTime.tv_sec = mSettings->accept_time.tv_sec;
            myReport->info.ts.startTime.tv_usec = mSettings->accept_time.tv_usec;
            // The client may have had a barrier between the connect and start of traffic, check and adjust
            if (mSettings->barrier_time) {
                now.setnow();
                if (now.subUsec(mSettings->accept_time) >= mSettings->barrier_time) {
                    TimeAddIntUsec(myReport->info.ts.startTime, mSettings->barrier_time);
                }
            }
        } else {
            now.setnow();
            myReport->info.ts.startTime.tv_sec = now.getSecs();
            myReport->info.ts.startTime.tv_usec = now.getUsecs();
        }
    }

    if (!TimeZero(myReport->info.ts.intervalTime)) {
        myReport->info.ts.nextTime = myReport->info.ts.startTime;
        TimeAdd(myReport->info.ts.nextTime, myReport->info.ts.intervalTime);
    }
    if (myReport->GroupSumReport) {
        struct TransferInfo *sumstats = &myReport->GroupSumReport->info;
        assert(sumstats != NULL);
        Mutex_Lock(&myReport->GroupSumReport->reference.lock);
        if (TimeZero(sumstats->ts.startTime)) {
            sumstats->ts.startTime = myReport->info.ts.startTime;
            if (mSettings->mIntervalMode == kInterval_Time) {
                sumstats->ts.nextTime = myReport->info.ts.nextTime;
            }
        }
        Mutex_Unlock(&myReport->GroupSumReport->reference.lock);
    }
#ifdef HAVE_THREAD_DEBUG
    thread_debug("Server(%d) report start=%ld.%06ld next=%ld.%06ld", mSettings->mSock, myReport->info.ts.startTime.tv_sec, myReport->info.ts.startTime.tv_usec, myReport->info.ts.nextTime.tv_sec, myReport->info.ts.nextTime.tv_usec);
#endif
}

void Server::ClientReverseFirstRead (void) {
    // Handle the case when the client spawns a server (no listener) and need the initial header
    // Case of --trip-times and --reverse or --fullduplex, listener handles normal case
    // Handle the case when the client spawns a server (no listener) and need the initial header
    // Case of --trip-times and --reverse or --fullduplex, listener handles normal case
    if (isReverse(mSettings) && (isTripTime(mSettings) || isPeriodicBurst(mSettings) || isIsochronous(mSettings))) {
        int nread = 0;
        uint32_t flags = 0;
        int readlen = 0;
        if (isUDP(mSettings)) {
            nread = recvn(mSettings->mSock, mSettings->mBuf, mSettings->mBufLen, 0);
            switch (nread) {
            case 0:
                //peer closed the socket, with no writes e.g. a connect-only test
                peerclose = true;
                break;
            case -1 :
                FAIL_errno(1, "recvn-reverse", mSettings);
                break;
            default :
                struct client_udp_testhdr *udp_pkt = reinterpret_cast<struct client_udp_testhdr *>(mSettings->mBuf);
                flags = ntohl(udp_pkt->base.flags);
                mSettings->sent_time.tv_sec = 0;
                if (isTripTime(mSettings)) {
                    mSettings->sent_time.tv_sec = ntohl(udp_pkt->start_fq.start_tv_sec);
                    mSettings->sent_time.tv_usec = ntohl(udp_pkt->start_fq.start_tv_usec);
                }
                if (!mSettings->sent_time.tv_sec) {
                    now.setnow();
                    mSettings->sent_time.tv_sec = now.getSecs();
                    mSettings->sent_time.tv_usec = now.getUsecs();
                }
                reportstruct->packetLen = nread;
                reportstruct->packetID = 1;
                break;
            }
        } else {
            nread = recvn(mSettings->mSock, mSettings->mBuf, sizeof(uint32_t), 0);
            if (nread == 0) {
                fprintf(stderr, "WARN: zero read on header flags\n");
                //peer closed the socket, with no writes e.g. a connect-only test
                peerclose = true;
            }
            FAIL_errno((nread < (int) sizeof(uint32_t)), "client read tcp flags", mSettings);
            reportstruct->packetID = 1;
            struct client_tcp_testhdr *tcp_pkt = reinterpret_cast<struct client_tcp_testhdr *>(mSettings->mBuf);
            flags = ntohl(tcp_pkt->base.flags);
            // figure out the length of the test header
            if ((readlen = Settings_ClientTestHdrLen(flags, mSettings)) > 0) {
                // read the test settings passed to the mSettings by the client
                int adj = (readlen - sizeof(uint32_t));
                nread = recvn(mSettings->mSock, (mSettings->mBuf + sizeof(uint32_t)), adj, 0);
                if (nread == 0) {
                    peerclose = true;
                }
                FAIL_errno((nread < adj), "client read tcp test info", mSettings);
                if (nread > 0) {
                    if (isTripTime(mSettings)) {
                        struct client_tcp_testhdr *tcp_pkt = reinterpret_cast<struct client_tcp_testhdr *>(mSettings->mBuf);
                        mSettings->sent_time.tv_sec = ntohl(tcp_pkt->start_fq.start_tv_sec);
                        mSettings->sent_time.tv_usec = ntohl(tcp_pkt->start_fq.start_tv_usec);
                    } else {
                        now.setnow();
                        mSettings->sent_time.tv_sec = now.getSecs();
                        mSettings->sent_time.tv_sec = now.getUsecs();
                    }
                }
                mSettings->firstreadbytes = readlen;
            }
        }
    }
}

bool Server::InitTrafficLoop (void) {
    bool UDPReady = true;
    if (isSyncTransferID(mSettings)) {
        if (mSettings->mPeerTransferID != mSettings->mTransferID) {
            int len = snprintf(NULL, 0, "%sTransfer ID %d remapped to %d\n", \
                               mSettings->mTransferIDStr, mSettings->mTransferID, mSettings->mPeerTransferID);
            char *text = (char *) calloc(len+1, sizeof(char));
            if (text) {
                snprintf(text, len, "%sTransfer ID %d remapped to %d\n", \
                         mSettings->mTransferIDStr, mSettings->mTransferID, mSettings->mPeerTransferID);
                PostReport(InitStringReport(text));
                FREE_ARRAY(text);
            }
            updateTransferIDPeer(mSettings);
        }
    }
    myJob = InitIndividualReport(mSettings);
    myReport = static_cast<struct ReporterData *>(myJob->this_report);
    assert(myJob != NULL);
    if (mSettings->mReportMode == kReport_CSV) {
        format_ips_port_string(&myReport->info, 0);
    }
    //  copy the thread drop socket to this object such
    //  that the destructor can close it if needed
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
    if (mSettings->mSockDrop > 0)
        myDropSocket = mSettings->mSockDrop;
#endif
    // Initialze the reportstruct scratchpad
    reportstruct = &scratchpad;
    reportstruct->packetID = 0;
    reportstruct->l2len = 0;
    reportstruct->l2errors = 0x0;

    int setfullduplexflag = 0;
    Timestamp now;

    if ((mSettings->txstart_epoch.tv_sec > 0) && (mSettings->txstart_epoch.tv_sec - now.getSecs()) > 1) {
        // Have the server thread wait on the client's epoch start
        // unblocking one second ahead
        struct timeval wait_until = mSettings->txstart_epoch;
        wait_until.tv_sec -= 1;
        clock_usleep_abstime(&wait_until);
    }
    if (isFullDuplex(mSettings) && !isServerReverse(mSettings)) {
        assert(mSettings->mFullDuplexReport != NULL);
        if ((setfullduplexflag = fullduplex_start_barrier(&mSettings->mFullDuplexReport->fullduplex_barrier)) < 0)
            exit(-1);
    }
    if (isReverse(mSettings)) {
        mSettings->accept_time.tv_sec = now.getSecs();
        mSettings->accept_time.tv_usec = now.getUsecs();
        ClientReverseFirstRead();
    }
    if (isTripTime(mSettings)) {
        int diff_tolerance;
        if (mSettings->mInterval && (mSettings->mIntervalMode == kInterval_Time)) {
            diff_tolerance = ceil(mSettings->mInterval / 1000000);
        } else {
            diff_tolerance = MAXDIFFTIMESTAMPSECS;
        }
        if (diff_tolerance < 2) {
            diff_tolerance = 2; // min is 2 seconds
        }
        if (mSettings->txstart_epoch.tv_sec > 0) {
            mSettings->accept_time.tv_sec = mSettings->txstart_epoch.tv_sec;
            mSettings->accept_time.tv_usec = mSettings->txstart_epoch.tv_usec;
            mSettings->sent_time = mSettings->accept_time; // the first sent time w/epoch starts uses now()
        } else if ((abs(now.getSecs() - mSettings->sent_time.tv_sec)) > diff_tolerance) {
            unsetTripTime(mSettings);
            fprintf(stdout,"WARN: ignore --trip-times because client didn't provide valid start timestamp within %d seconds of now\n", diff_tolerance);
            mSettings->accept_time.tv_sec = now.getSecs();
            mSettings->accept_time.tv_usec = now.getUsecs();
        }
    }
    SetReportStartTime();
    reportstruct->prevPacketTime = myReport->info.ts.startTime;

    if (setfullduplexflag)
        SetFullDuplexReportStartTime();

    if (isServerModeTime(mSettings) || \
        (isModeTime(mSettings) && (isBounceBack(mSettings) || isServerReverse(mSettings) || isFullDuplex(mSettings) \
                                   || isReverse(mSettings)))) {
        if (isServerReverse(mSettings) || isFullDuplex(mSettings) || isReverse(mSettings))
            mSettings->mAmount += (SLOPSECS * 100);  // add 2 sec for slop on reverse, units are 10 ms

        uintmax_t end_usecs  (mSettings->mAmount * 10000); //amount units is 10 ms
        if (int err = set_itimer(end_usecs))
            FAIL_errno(err != 0, "setitimer", mSettings);
        mEndTime.setnow();
        mEndTime.add(mSettings->mAmount / 100.0);
    }
    if (!isSingleUDP(mSettings))
        PostReport(myJob);
    // The first payload is different for TCP so read it and report it
    // before entering the main loop
    if (mSettings->firstreadbytes > 0) {
        reportstruct->frameID = 0;
        reportstruct->packetLen = mSettings->firstreadbytes;
        if (isUDP(mSettings)) {
            int offset = 0;
            reportstruct->packetTime = mSettings->accept_time;
            UDPReady = !ReadPacketID(offset);
        } else {
            reportstruct->sentTime.tv_sec = myReport->info.ts.startTime.tv_sec;
            reportstruct->sentTime.tv_usec = myReport->info.ts.startTime.tv_usec;
            reportstruct->packetTime = reportstruct->sentTime;
        }
        ReportPacket(myReport, reportstruct);
    }
    return UDPReady;
}

inline int Server::ReadWithRxTimestamp () {
    int currLen;
    int tsdone = false;

    reportstruct->err_readwrite = ReadSuccess;
#if (HAVE_DECL_SO_TIMESTAMP) && (HAVE_DECL_MSG_CTRUNC)
    cmsg = reinterpret_cast<struct cmsghdr *>(&ctrl);
    currLen = recvmsg(mSettings->mSock, &message, mSettings->recvflags);
    if (currLen > 0) {
#if HAVE_DECL_MSG_TRUNC
        if (message.msg_flags & MSG_TRUNC) {
            reportstruct->err_readwrite = ReadErrLen;
        }
#endif
        if (!(message.msg_flags & MSG_CTRUNC)) {
            for (cmsg = CMSG_FIRSTHDR(&message); cmsg != NULL;
                 cmsg = CMSG_NXTHDR(&message, cmsg)) {
                if (cmsg->cmsg_level == SOL_SOCKET &&
                    cmsg->cmsg_type  == SCM_TIMESTAMP &&
                    cmsg->cmsg_len   == CMSG_LEN(sizeof(struct timeval))) {
                    memcpy(&(reportstruct->packetTime), CMSG_DATA(cmsg), sizeof(struct timeval));
                    if (TimeZero(myReport->info.ts.prevpacketTime)) {
                        myReport->info.ts.prevpacketTime = reportstruct->packetTime;
                    }
                    tsdone = true;
                }
		if (cmsg->cmsg_level == IPPROTO_IP &&
                    cmsg->cmsg_type  == IP_TOS &&
                    cmsg->cmsg_len   == CMSG_LEN(sizeof(u_char))) {
                    memcpy(&(reportstruct->tos), CMSG_DATA(cmsg), sizeof(u_char));
		}
            }
        } else if (ctrunc_warn_enable && mSettings->mTransferIDStr) {
            fprintf(stderr, "%sWARN: recvmsg MSG_CTRUNC occured\n", mSettings->mTransferIDStr);
            ctrunc_warn_enable = false;
        }
    }
#else
    currLen = recv(mSettings->mSock, mSettings->mBuf, mSettings->mBufLen, mSettings->recvflags);
#endif
    // RJM clean up
    if (currLen <=0) {
        // Socket read timeout or read error
        reportstruct->emptyreport = true;
        if (currLen == 0) {
            peerclose = true;
        } else if (FATALUDPREADERR(errno)) {
            char warnbuf[WARNBUFSIZE];
            snprintf(warnbuf, sizeof(warnbuf), "%srecvmsg",\
                     mSettings->mTransferIDStr);
            warnbuf[sizeof(warnbuf)-1] = '\0';
            WARN_errno(1, warnbuf);
            currLen = 0;
            peerclose = true;
        } else {
            reportstruct->err_readwrite = ReadTimeo;
        }
    } else if (TimeZero(myReport->info.ts.prevpacketTime)) {
        myReport->info.ts.prevpacketTime = reportstruct->packetTime;
    }
    if (!tsdone) {
        now.setnow();
        reportstruct->packetTime.tv_sec = now.getSecs();
        reportstruct->packetTime.tv_usec = now.getUsecs();
    }
    return currLen;
}

// Returns true if the client has indicated this is the final packet
inline bool Server::ReadPacketID (int offset_adjust) {
    bool terminate = false;
    struct UDP_datagram* mBuf_UDP  = reinterpret_cast<struct UDP_datagram*>(mSettings->mBuf + offset_adjust);
    // terminate when datagram begins with negative index
    // the datagram ID should be correct, just negated

    // read the sent timestamp from the rx packet
    reportstruct->sentTime.tv_sec = ntohl(mBuf_UDP->tv_sec);
    reportstruct->sentTime.tv_usec = ntohl(mBuf_UDP->tv_usec);
    if (isSeqNo64b(mSettings)) {
        // New client - Signed PacketID packed into unsigned id2,id
        reportstruct->packetID = (static_cast<uint32_t>(ntohl(mBuf_UDP->id))) | (static_cast<uintmax_t>(ntohl(mBuf_UDP->id2)) << 32);

#ifdef HAVE_PACKET_DEBUG
        if (isTripTime(mSettings)) {
            int len = snprintf(NULL,0,"%sPacket id 0x%x, 0x%x -> %" PRIdMAX " (0x%" PRIxMAX ") Sent: %ld.%06ld6 Received: %ld.%06ld6 Delay: %f\n", \
                               mSettings->mTransferIDStr,ntohl(mBuf_UDP->id), ntohl(mBuf_UDP->id2), reportstruct->packetID, reportstruct->packetID, \
                               reportstruct->sentTime.tv_sec, reportstruct->sentTime.tv_usec, \
                               reportstruct->packetTime.tv_sec, reportstruct->packetTime.tv_usec, TimeDifference(reportstruct->packetTime, reportstruct->sentTime));
            char *text = (char *) calloc(len+1, sizeof(char));
            if (text) {
                snprintf(text, len,"%sPacket ID id 0x%x, 0x%x -> %" PRIdMAX " (0x%" PRIxMAX ") Sent: %ld.%06ld Received: %ld.%06ld Delay: %f\n", \
                         mSettings->mTransferIDStr,ntohl(mBuf_UDP->id), ntohl(mBuf_UDP->id2), reportstruct->packetID, reportstruct->packetID, \
                         reportstruct->sentTime.tv_sec, reportstruct->sentTime.tv_usec, \
                         reportstruct->packetTime.tv_sec, reportstruct->packetTime.tv_usec, TimeDifference(reportstruct->packetTime, reportstruct->sentTime));
                PostReport(InitStringReport(text));
                FREE_ARRAY(text);
            }
        } else {
            printf("id 0x%x, 0x%x -> %" PRIdMAX " (0x%" PRIxMAX ")\n",
                   ntohl(mBuf_UDP->id), ntohl(mBuf_UDP->id2), reportstruct->packetID, reportstruct->packetID);
        }
#endif
    } else {
        // Old client - Signed PacketID in Signed id
        reportstruct->packetID = static_cast<int32_t>(ntohl(mBuf_UDP->id));
#ifdef HAVE_PACKET_DEBUG
        printf("id 0x%x -> %" PRIdMAX " (0x%" PRIxMAX ")\n",
               ntohl(mBuf_UDP->id), reportstruct->packetID, reportstruct->packetID);
#endif
    }
    if (reportstruct->packetID < 0) {
        reportstruct->packetID = -reportstruct->packetID;
        terminate = true;
    }
    return terminate;
}

void Server::L2_processing () {
#if (HAVE_LINUX_FILTER_H) && (HAVE_AF_PACKET)
    eth_hdr = reinterpret_cast<struct ether_header *>(mSettings->mBuf);
    ip_hdr = reinterpret_cast<struct iphdr *>(mSettings->mBuf + sizeof(struct ether_header));
    // L4 offest is set by the listener and depends upon IPv4 or IPv6
    udp_hdr = reinterpret_cast<struct udphdr *>(mSettings->mBuf + mSettings->l4offset);
    // Read the packet to get the UDP length
    int udplen = ntohs(udp_hdr->len);
    //
    // in the event of an L2 error, double check the packet before passing it to the reporter,
    // i.e. no reason to run iperf accounting on a packet that has no reasonable L3 or L4 headers
    //
    reportstruct->packetLen = udplen - sizeof(struct udphdr);
    reportstruct->expected_l2len = reportstruct->packetLen + mSettings->l4offset + sizeof(struct udphdr);
    if (reportstruct->l2len != reportstruct->expected_l2len) {
        reportstruct->l2errors |= L2LENERR;
        if (L2_quintuple_filter() != 0) {
            reportstruct->l2errors |= L2UNKNOWN;
            reportstruct->l2errors |= L2CSUMERR;
            reportstruct->emptyreport = true;
        }
    }
    if (!(reportstruct->l2errors & L2UNKNOWN)) {
        // perform UDP checksum test, returns zero on success
        int rc;
        rc = udpchecksum((void *)ip_hdr, (void *)udp_hdr, udplen, (isIPV6(mSettings) ? 1 : 0));
        if (rc) {
            reportstruct->l2errors |= L2CSUMERR;
            if ((!(reportstruct->l2errors & L2LENERR)) && (L2_quintuple_filter() != 0)) {
                reportstruct->emptyreport = true;
                reportstruct->l2errors |= L2UNKNOWN;
            }
        }
    }
#endif // HAVE_AF_PACKET
}

// Run the L2 packet through a quintuple check, i.e. proto/ip src/ip dst/src port/src dst
// and return zero is there is a match, otherwize return nonzero
int Server::L2_quintuple_filter () {
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)

#define IPV4SRCOFFSET 12  // the ipv4 source address offset from the l3 pdu
#define IPV6SRCOFFSET 8 // the ipv6 source address offset

    // Get the expected values from the sockaddr structures
    // Note: it's expected the initiating socket has aready "connected"
    // and the sockaddr structs have been populated
    // 2nd Note:  sockaddr structs are in network byte order
    struct sockaddr *p = reinterpret_cast<sockaddr *>(&mSettings->peer);
    struct sockaddr *l = reinterpret_cast<sockaddr *>(&mSettings->local);
    // make sure sa_family is coherent for both src and dst
    if (!(((l->sa_family == AF_INET) && (p->sa_family == AF_INET)) || ((l->sa_family == AF_INET6) && (p->sa_family == AF_INET6)))) {
        return -1;
    }

    // check the L2 ethertype
    struct ether_header *l2hdr = reinterpret_cast<struct ether_header *>(mSettings->mBuf);

    if (!isIPV6(mSettings)) {
        if (ntohs(l2hdr->ether_type) != ETHERTYPE_IP)
            return -1;
    } else {
        if (ntohs(l2hdr->ether_type) != ETHERTYPE_IPV6)
            return -1;
    }
    // check the ip src/dst
    const uint32_t *data;
    udp_hdr = reinterpret_cast<struct udphdr *>(mSettings->mBuf + mSettings->l4offset);

    // Check plain old v4 using v4 addr structs
    if (l->sa_family == AF_INET) {
        data = reinterpret_cast<const uint32_t *>(mSettings->mBuf + sizeof(struct ether_header) + IPV4SRCOFFSET);
        if ((reinterpret_cast<struct sockaddr_in *>(p))->sin_addr.s_addr != *data++)
            return -1;
        if ((reinterpret_cast<struct sockaddr_in *>(l))->sin_addr.s_addr != *data)
            return -1;
        if (udp_hdr->source != (reinterpret_cast<struct sockaddr_in *>(p))->sin_port)
            return -1;
        if (udp_hdr->dest != (reinterpret_cast<struct sockaddr_in *>(l))->sin_port)
            return -1;
    } else {
        // Using the v6 addr structures
#  if HAVE_IPV6
        struct in6_addr *v6peer = SockAddr_get_in6_addr(&mSettings->peer);
        struct in6_addr *v6local = SockAddr_get_in6_addr(&mSettings->local);
        if (isIPV6(mSettings)) {
            int i;
            data = reinterpret_cast<const uint32_t *>(mSettings->mBuf + sizeof(struct ether_header) + IPV6SRCOFFSET);
            // check for v6 src/dst address match
            for (i = 0; i < 4; i++) {
                if (v6peer->s6_addr32[i] != *data++)
                    return -1;
            }
            for (i = 0; i < 4; i++) {
                if (v6local->s6_addr32[i] != *data++)
                    return -1;
            }
        } else { // v4 addr in v6 family struct
            data = reinterpret_cast<const uint32_t *>(mSettings->mBuf + sizeof(struct ether_header) + IPV4SRCOFFSET);
            if (v6peer->s6_addr32[3] != *data++)
                return -1;
            if (v6peer->s6_addr32[3] != *data)
                return -1;
        }
        // check udp ports
        if (udp_hdr->source != (reinterpret_cast<struct sockaddr_in6 *>(p))->sin6_port)
            return -1;
        if (udp_hdr->dest != (reinterpret_cast<struct sockaddr_in6 *>(l))->sin6_port)
            return -1;
#  endif // HAVE_IPV6
    }
#endif // HAVE_AF_PACKET
    // made it through all the checks
    return 0;
}

inline void Server::udp_isoch_processing (int rxlen) {
    reportstruct->transit_ready = false;
    // Ignore runt sized isoch packets
    if (rxlen < static_cast<int>(sizeof(struct UDP_datagram) +  sizeof(struct client_hdr_v1) + sizeof(struct client_hdrext) + sizeof(struct isoch_payload))) {
        reportstruct->burstsize = 0;
        reportstruct->remaining = 0;
        reportstruct->frameID = 0;
    } else {
        struct client_udp_testhdr *udp_pkt = reinterpret_cast<struct client_udp_testhdr *>(mSettings->mBuf);
        reportstruct->isochStartTime.tv_sec = ntohl(udp_pkt->isoch.start_tv_sec);
        reportstruct->isochStartTime.tv_usec = ntohl(udp_pkt->isoch.start_tv_usec);
        reportstruct->frameID = ntohl(udp_pkt->isoch.frameid);
        reportstruct->prevframeID = ntohl(udp_pkt->isoch.prevframeid);
        reportstruct->burstsize = ntohl(udp_pkt->isoch.burstsize);
        assert(reportstruct->burstsize > 0);
        reportstruct->burstperiod = ntohl(udp_pkt->isoch.burstperiod);
        reportstruct->remaining = ntohl(udp_pkt->isoch.remaining);
        if ((reportstruct->remaining == (uint32_t) rxlen) && ((reportstruct->frameID - reportstruct->prevframeID) == 1)) {
            reportstruct->transit_ready = true;
        }
    }
}

/* -------------------------------------------------------------------
 * Receive UDP data from the (connected) socket.
 * Sends termination flag several times at the end.
 * Does not close the socket.
 * ------------------------------------------------------------------- */
void Server::RunUDP () {
    int rxlen;
    bool isLastPacket = false;

    bool startReceiving = InitTrafficLoop();
    Condition_Signal(&mSettings->receiving); // signal the listener thread so it can hang a new recvfrom
    if (startReceiving) {
        // Exit loop on three conditions
        // 1) Fatal read error
        // 2) Last packet of traffic flow sent by client
        // 3) -t timer expires
        while (InProgress() && !isLastPacket) {
            // The emptyreport flag can be set
            // by any of the packet processing routines
            // If it's set the iperf reporter won't do
            // bandwidth accounting, basically it's indicating
            // that the reportstruct itself couldn't be
            // completely filled out.
            reportstruct->emptyreport = true;
            reportstruct->packetLen=0;
            // read the next packet with timestamp
            // will also set empty report or not
            rxlen=ReadWithRxTimestamp();
            if (!peerclose && (rxlen > 0)) {
                reportstruct->emptyreport = false;
                reportstruct->packetLen = rxlen;
                if (isL2LengthCheck(mSettings)) {
                    reportstruct->l2len = rxlen;
                    // L2 processing will set the reportstruct packet length with the length found in the udp header
                    // and also set the expected length in the report struct.  The reporter thread
                    // will do the compare and account and print l2 errors
                    reportstruct->l2errors = 0x0;
                    L2_processing();
                }
                if (!(reportstruct->l2errors & L2UNKNOWN)) {
                    // ReadPacketID returns true if this is the last UDP packet sent by the client
                    // also sets the packet rx time in the reportstruct
                    reportstruct->prevSentTime = myReport->info.ts.prevsendTime;
                    reportstruct->prevPacketTime = myReport->info.ts.prevpacketTime;
                    isLastPacket = ReadPacketID(mSettings->l4payloadoffset);
                    myReport->info.ts.prevsendTime = reportstruct->sentTime;
                    myReport->info.ts.prevpacketTime = reportstruct->packetTime;
                    if (isIsochronous(mSettings)) {
                        udp_isoch_processing(rxlen);
                    }
                }
            }
            ReportPacket(myReport, reportstruct);
        }
    }
    disarm_itimer();
    bool do_close = EndJob(myJob, reportstruct);
    if (!isMulticast(mSettings) && !isNoUDPfin(mSettings)) {
        // send a UDP acknowledgement back except when:
        // 1) we're NOT receiving multicast
        // 2) the user requested no final exchange
        // 3) this is a full duplex test
        write_UDP_AckFIN(&myReport->info, mSettings->mBufLen);
    }
    if (do_close) {
#if HAVE_THREAD_DEBUG
        thread_debug("udp close sock=%d", mySocket);
#endif
        int rc = close(mySocket);
        WARN_errno(rc == SOCKET_ERROR, "server close");
    }
    Iperf_remove_host(mSettings);
    FreeReport(myJob);
}

#if HAVE_UDP_L4S
void Server::RunUDPL4S () {
    int rxlen;
    bool isLastPacket = false;
    bool startReceiving = InitTrafficLoop();
    PragueCC l4s_pacer;
    Condition_Signal(&mSettings->receiving); // signal the listener thread so it can hang a new recvfrom
    if (startReceiving) {
#ifdef HAVE_THREAD_DEBUG
	thread_debug("Server running L4S with thread=%p sum=%p (sock=%d)", (void *) mSettings, (void *)mSettings->mSumReport, mSettings->mSock);
#endif
        // Exit loop on three conditions
        // 1) Fatal read error
        // 2) Last packet of traffic flow sent by client
        // 3) -t timer expires
        while (InProgress() && !isLastPacket) {
            // The emptyreport flag can be set
            // by any of the packet processing routines
            // If it's set the iperf reporter won't do
            // bandwidth accounting, basically it's indicating
            // that the reportstruct itself couldn't be
            // completely filled out.
            reportstruct->emptyreport = true;
            reportstruct->packetLen=0;
            // read the next packet with timestamp
            // will also set empty report or not
            rxlen=ReadWithRxTimestamp();
            if (!peerclose && (rxlen > 0)) {
                reportstruct->emptyreport = false;
                reportstruct->packetLen = rxlen;
		// ReadPacketID returns true if this is the last UDP packet sent by the client
		// also sets the packet rx time in the reportstruct
		reportstruct->prevSentTime = myReport->info.ts.prevsendTime;
		reportstruct->prevPacketTime = myReport->info.ts.prevpacketTime;
		isLastPacket = ReadPacketID(mSettings->l4payloadoffset);
		myReport->info.ts.prevsendTime = reportstruct->sentTime;
		myReport->info.ts.prevpacketTime = reportstruct->packetTime;
		// Read L4S fields from UDP payload, ECN bits came from earlier cmsg
		struct client_udp_l4s_fwd *udp_l4spkt =			\
		    reinterpret_cast<struct client_udp_l4s_fwd *>(mSettings->mBuf);
		l4s_pacer.PacketReceived(ntohl(udp_l4spkt->sender_ts),ntohl(udp_l4spkt->echoed_ts));
		l4s_pacer.DataReceivedSequence(ecn_tp(reportstruct->tos & 0x03), \
					       ntohl(udp_l4spkt->sender_seqno));
		ReportPacket(myReport, reportstruct);
		// Send l4s ack
		//
		time_tp timestamp;
		time_tp echoed_timestamp;
		ecn_tp ip_ecn;
		l4s_pacer.GetTimeInfo(timestamp, echoed_timestamp,ip_ecn);
                struct udp_l4s_ack udp_l4s_pkt_ack;
		udp_l4s_pkt_ack.rx_ts = htonl((int32_t) timestamp);
		udp_l4s_pkt_ack.echoed_ts = htonl((int32_t) echoed_timestamp);
		count_tp pkts_rx;
		count_tp pkts_ce;
		count_tp pkts_lost;
		bool l4s_err;
		l4s_pacer.GetACKInfo(pkts_rx, pkts_ce, pkts_lost, l4s_err);
		udp_l4s_pkt_ack.rx_cnt = htonl(pkts_rx);
		udp_l4s_pkt_ack.CE_cnt = htonl(pkts_ce);
		udp_l4s_pkt_ack.lost_cnt = htonl(pkts_lost);
		udp_l4s_pkt_ack.flags = (l4s_err ? htons(L4S_ECN_ERR) : 0);
		udp_l4s_pkt_ack.l4sreserved = 0;
		struct msghdr msg;
		struct iovec iov[1];
		unsigned char cmsg[CMSG_SPACE(sizeof(int))];
		struct cmsghdr *cmsgptr = NULL;

		memset(&iov, 0, sizeof(iov));
		memset(&cmsg, 0, sizeof(cmsg));
		memset(&msg, 0, sizeof (struct msghdr));

		iov[0].iov_base = reinterpret_cast<char *> (&(udp_l4s_pkt_ack));
		iov[0].iov_len = sizeof(struct udp_l4s_ack);
		msg.msg_iov = iov;
		msg.msg_iovlen = 1;
		msg.msg_control = cmsg;
		msg.msg_controllen = sizeof(cmsg);
		cmsgptr = CMSG_FIRSTHDR(&msg);
		cmsgptr->cmsg_level = IPPROTO_IP;
		cmsgptr->cmsg_type  = IP_TOS;
		cmsgptr->cmsg_len  = CMSG_LEN(sizeof(u_char));
		u_char tos = (mSettings->mTOS | ip_ecn);
		memcpy(CMSG_DATA(cmsgptr), (u_char*)&tos, sizeof(u_char));
		msg.msg_controllen = CMSG_SPACE(sizeof(u_char));
		int ackLen = sendmsg(mySocket, &msg, 0);
		if (ackLen <= 0) {
		    if (ackLen == 0) {
			WARN_errno(1, "write l4s ack timeout");
			reportstruct->err_readwrite = WriteTimeo;
		    } else if (FATALUDPWRITERR(errno)) {
			WARN_errno(1, "write l4s ack fatal");
		    }
		}
	    }
        }
    }
    disarm_itimer();
    bool do_close = EndJob(myJob, reportstruct);
    if (!isMulticast(mSettings) && !isNoUDPfin(mSettings)) {
        // send a UDP acknowledgement back except when:
        // 1) we're NOT receiving multicast
        // 2) the user requested no final exchange
        // 3) this is a full duplex test
        write_UDP_AckFIN(&myReport->info, mSettings->mBufLen);
    }
    if (do_close) {
#if HAVE_THREAD_DEBUG
        thread_debug("udp close sock=%d", mySocket);
#endif
        int rc = close(mySocket);
        WARN_errno(rc == SOCKET_ERROR, "server close");
    }
    Iperf_remove_host(mSettings);
    FreeReport(myJob);
}
#endif
// end Recv
