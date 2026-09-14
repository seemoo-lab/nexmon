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
 * Client.hpp
 * by Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * A client thread initiates a connect to the server and handles
 * sending and receiving data, then closes the socket.
 * -------------------------------------------------------------------
 */

#ifndef CLIENT_H
#define CLIENT_H

#include "Settings.hpp"
#include "Timestamp.hpp"
#include "isochronous.hpp"
#include "iperf_multicast_api.h"
#include "Mutex.h"
#if HAVE_UDP_L4S
#include "prague_cc.h"
#endif
/* ------------------------------------------------------------------- */
class Client {
public:
    // stores server hostname, port, UDP/TCP mode, and UDP rate
    Client( thread_Settings *inSettings );

    // destroy the client object
    ~Client();

    // Set up the traffic thread and invokes
    // appropriate traffic loop per the protocol
    // and type of traffic
    void Run( void );

    // For things like dual tests a server needs to be started by the client,
    // The code in src/launch.cpp will invoke this
    int StartSynch(void);
    void TxDelay(void);
    void ConnectPeriodic(void);
    bool my_connect(bool close_on_fail);
    void mySockInit(void);
    bool isConnected(void) const;
    int SendFirstPayload(void);
    bool BarrierClient(struct BarrierMutex *);
    void RunBounceBackTCP(void);
    struct ReportHeader *myJob;

private:
    inline void WritePacketID(intmax_t);
    inline void WriteTcpTxHdr(struct ReportStruct *, int, int);
    inline void WriteTcpTxBBHdr(struct ReportStruct *, uint32_t, int);
    inline int myWrite(int inSock, const void *inBuf, int inLen);
    inline int myWriten(int inSock, const void *inBuf, int inLen, int *count);
#if HAVE_TCP_STATS
    inline void mygetTcpInfo(void);
#endif
    inline double get_delay_target(void);
    void InitTrafficLoop(void);
    void SetReportStartTime(void);
    inline void SetFullDuplexReportStartTime(void);
    void FinishTrafficActions(void);
    void AwaitServerFinPacket(void);
    bool InProgress(void);
    void PostNullEvent(bool isFirst, bool select_retry);
    void AwaitServerCloseEvent(void);
    inline void tcp_shutdown(void);
    bool connected;
    ReportStruct scratchpad;
    ReportStruct *reportstruct;
    double delay_lower_bounds;
    intmax_t totLen;
    bool one_report;
    bool apply_first_udppkt_delay;
    int udp_payload_minimum;
    void myReportPacket(void);
    void myReportPacket(struct ReportStruct *);
    // TCP plain
    void RunTCP(void);
    // TCP version which supports rate limiting per -b
    void RunRateLimitedTCP(void);
    void RunNearCongestionTCP(void);
    bool AwaitSelectWrite(void);
#if HAVE_DECL_TCP_NOTSENT_LOWAT
    void RunWriteEventsTCP(void);
#endif
    // UDP traffic with isochronous and vbr support
    void RunUDPIsochronous(void);
    // UDP plain
    void RunUDP(void);
    void RunUDPBurst(void);
#if HAVE_UDP_L4S
    void RunUDPL4S(void);
    int ack_poll (time_tp ack_timeout);
#endif
    // client connect
    void PeerXchange(void);
    thread_Settings *mSettings;
#if WIN32
    SOCKET mySocket;
#else
    int mySocket;
#endif
    struct ReporterData *myReport;
    Timestamp mEndTime;
    Timestamp lastPacketTime;
    Timestamp now;
    char* readAt;
    Timestamp connect_done, connect_start;
    Isochronous::FrameCounter *framecounter;
    bool isburst;
    bool mysock_init_done;
    bool peerclose;
    Timestamp write_start;
#if HAVE_DECL_SO_MAX_PACING_RATE
    Timestamp PacingStepTime;
#endif
    struct udp_l4s_ack UDPAckBuf;
}; // end class Client

#endif // CLIENT_H
