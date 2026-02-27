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
#include "List.h"
#include "Extractor.h"
#include "Reporter.h"
#include "Locale.h"
#ifdef HAVE_SCHED_SETSCHEDULER
#include <sched.h>
#endif
#ifdef HAVE_MLOCKALL
#include <sys/mman.h>
#endif

/* -------------------------------------------------------------------
 * Stores connected socket and socket info.
 * ------------------------------------------------------------------- */

Server::Server( thread_Settings *inSettings ) {
    mSettings = inSettings;
    mBuf = NULL;

    // initialize buffer
    mBuf = new char[ mSettings->mBufLen ];
    FAIL_errno( mBuf == NULL, "No memory for buffer\n", mSettings );
}

/* -------------------------------------------------------------------
 * Destructor close socket.
 * ------------------------------------------------------------------- */

Server::~Server() {
    if ( mSettings->mSock != INVALID_SOCKET ) {
        int rc = close( mSettings->mSock );
        WARN_errno( rc == SOCKET_ERROR, "close" );
        mSettings->mSock = INVALID_SOCKET;
    }
    DELETE_ARRAY( mBuf );
}

void Server::Sig_Int( int inSigno ) {
}
/* ------------------------------------------------------------------- 
 * Receive TCP data from the (connected) socket.
 * Sends termination flag several times at the end. 
 * Does not close the socket. 
 * ------------------------------------------------------------------- */ 
void Server::RunTCP( void ) {
    long currLen; 
    max_size_t totLen = 0;
    ReportStruct *reportstruct = NULL;
    int running;
    bool mMode_Time = isServerModeTime( mSettings );

    reportstruct = new ReportStruct;
    if ( reportstruct != NULL ) {
        reportstruct->packetID = 0;
        mSettings->reporthdr = InitReport( mSettings );
	running=1;
	int sorcvtimer = 0;
	// sorcvtimer units microseconds convert to that
	// minterval double, units seconds
	// mAmount integer, units 10 milliseconds
	// divide by two so timeout is 1/2 the interval
	if (mSettings->mInterval) {
	    sorcvtimer = (int) (mSettings->mInterval * 1e6) / 2;
	} else if (isModeTime(mSettings)) {
	    sorcvtimer = (mSettings->mAmount * 1000) / 2;
	}
	if (sorcvtimer > 0) {
#ifdef WIN32
            // Windows SO_RCVTIMEO uses ms
	    DWORD timeout = (double) sorcvtimer / 1e3;
#else
	    struct timeval timeout;
	    timeout.tv_sec = sorcvtimer / 1000000;
	    timeout.tv_usec = sorcvtimer % 1000000;
#endif // WIN32
	    if (setsockopt( mSettings->mSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0 ) {
		WARN_errno( mSettings->mSock == SO_RCVTIMEO, "socket" );
	    }
	}
#ifdef HAVE_SCHED_SETSCHEDULER
	if ( isRealtime( mSettings ) ) {
	    struct sched_param sp;
	    sp.sched_priority = sched_get_priority_max(SCHED_RR); 
	    // SCHED_OTHER, SCHED_FIFO, SCHED_RR
	    if (sched_setscheduler(0, SCHED_RR, &sp) < 0)  {
		perror("Client set scheduler");
#ifdef HAVE_MLOCKALL
	    } else if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) { 
		// lock the threads memory
		perror ("mlockall");
#endif // MLOCK
	    }
	}
#endif // SCHED
	// setup termination variables
	if ( mMode_Time ) {
	    mEndTime.setnow();
	    mEndTime.add( mSettings->mAmount / 100.0 );
	}
        do {
	    reportstruct->emptyreport=0;
	    // perform read 
	    currLen = recv( mSettings->mSock, mBuf, mSettings->mBufLen, 0 );
#ifdef HAVE_CLOCK_GETTIME
	    {
		struct timespec t1; 
		clock_gettime(CLOCK_REALTIME, &t1);
		reportstruct->packetTime.tv_sec = t1.tv_sec;
		reportstruct->packetTime.tv_usec = t1.tv_nsec / 1000;
	    }
#else 
	    gettimeofday( &(reportstruct->packetTime), NULL );
#endif  // GETTIME  
	    if (currLen <= 0) {
		reportstruct->emptyreport=1;
		// End loop on 0 read or socket error
		// except for socket read timeout
		if (currLen == 0 ||
#ifdef WIN32
		    (WSAGetLastError() != WSAEWOULDBLOCK)
#else
		    (errno != EAGAIN && errno != EWOULDBLOCK)
#endif // WIN32		     
		    ) {
		    running = 0;
		}
		currLen = 0;
	    }
	    totLen += currLen;
	    reportstruct->packetLen = currLen;
	    if (mMode_Time && mEndTime.before( reportstruct->packetTime)) {
		running = 0;
	    }
	    ReportPacket( mSettings->reporthdr, reportstruct );
        } while (running); 
                
        // stop timing 
#ifdef HAVE_CLOCK_GETTIME
       {
	   struct timespec t1; 
	   clock_gettime(CLOCK_REALTIME, &t1);
	   reportstruct->packetTime.tv_sec = t1.tv_sec;
	   reportstruct->packetTime.tv_usec = t1.tv_nsec / 1000;
       }
#else 
        gettimeofday( &(reportstruct->packetTime), NULL );
#endif // GETTIME           

	if(0.0 == mSettings->mInterval) {
	    reportstruct->packetLen = totLen;
        }
	ReportPacket( mSettings->reporthdr, reportstruct );
        CloseReport( mSettings->reporthdr, reportstruct );
    } else {
        FAIL(1, "Out of memory! Closing server thread\n", mSettings);
    }

    Mutex_Lock( &clients_mutex );     
    Iperf_delete( &(mSettings->peer), &clients ); 
    Mutex_Unlock( &clients_mutex );

    DELETE_PTR( reportstruct );
    EndReport( mSettings->reporthdr );
} 

/* ------------------------------------------------------------------- 
 * Receive UDP data from the (connected) socket.
 * Sends termination flag several times at the end. 
 * Does not close the socket. 
 * ------------------------------------------------------------------- */ 
void Server::RunUDP( void ) {
    long currLen; 
    max_size_t totLen = 0;
    struct UDP_datagram* mBuf_UDP  = (struct UDP_datagram*) mBuf; 
    ReportStruct *reportstruct = NULL;
    int running;
    bool mMode_Time = isServerModeTime( mSettings );

#if HAVE_DECL_SO_TIMESTAMP
    // Structures needed for recvmsg
    // Use to get kernel timestamps of packets
    struct sockaddr_storage srcaddr;
    struct iovec iov[1];
    iov[0].iov_base=mBuf;
    iov[0].iov_len=mSettings->mBufLen;
    struct msghdr message;
    message.msg_iov=iov;
    message.msg_iovlen=1;
    message.msg_name=&srcaddr;
    message.msg_namelen=sizeof(srcaddr);
    char ctrl[CMSG_SPACE(sizeof(struct timeval))];
    struct cmsghdr *cmsg = (struct cmsghdr *) &ctrl;
    message.msg_control = (char *) ctrl;
    message.msg_controllen = sizeof(ctrl);
#endif

    reportstruct = new ReportStruct;
    if ( reportstruct != NULL ) {
        reportstruct->packetID = 0;
        mSettings->reporthdr = InitReport( mSettings );
	running=1;
	int sorcvtimer = 0;
	// sorcvtimer units microseconds convert to that
	// minterval double, units seconds
	// mAmount integer, units 10 milliseconds
	// divide by two so timeout is 1/2 the interval
	if (mSettings->mInterval) {
	    sorcvtimer = (int) (mSettings->mInterval * 1e6) / 2;
	} else if (isModeTime(mSettings)) {
	    sorcvtimer = (mSettings->mAmount * 1000) / 2;
	}
	if (sorcvtimer > 0) {
#ifdef WIN32
            // Windows SO_RCVTIMEO uses ms
	    DWORD timeout = (double) sorcvtimer / 1e3;
#else
	    struct timeval timeout;
	    timeout.tv_sec = sorcvtimer / 1000000;
	    timeout.tv_usec = sorcvtimer % 1000000;
#endif
	    if (setsockopt( mSettings->mSock, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout)) < 0 ) {
		WARN_errno( mSettings->mSock == SO_RCVTIMEO, "socket" );
	    }
	}
#if HAVE_DECL_SO_TIMESTAMP
	int timestampOn = 1;
	if (setsockopt(mSettings->mSock, SOL_SOCKET, SO_TIMESTAMP, (int *) &timestampOn, sizeof(timestampOn)) < 0) {
	    WARN_errno( mSettings->mSock == SO_TIMESTAMP, "socket" );
	}
#endif
#ifdef HAVE_SCHED_SETSCHEDULER
	if ( isRealtime( mSettings ) ) {
	    struct sched_param sp;
	    sp.sched_priority = sched_get_priority_max(SCHED_RR); 
	    // SCHED_OTHER, SCHED_FIFO, SCHED_RR
	    if (sched_setscheduler(0, SCHED_RR, &sp) < 0)  {
		perror("Client set scheduler");
#ifdef HAVE_MLOCKALL
	    } else if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) { 
		// lock the threads memory
		perror ("mlockall");
#endif
	    }
	}
#endif
	// setup termination variables
	if ( mMode_Time ) {
	    mEndTime.setnow();
	    mEndTime.add( mSettings->mAmount / 100.0 );
	}
        do {
	    reportstruct->emptyreport=0;
#if HAVE_DECL_SO_TIMESTAMP
            // perform read 
            currLen = recvmsg( mSettings->mSock, &message, 0 );
	    if (currLen <= 0) {
		// Socket read timeout or read error
		reportstruct->emptyreport=1;
		gettimeofday( &(reportstruct->packetTime), NULL );
                // End loop on 0 read or socket error
		// except for socket read timeout
		if (currLen == 0 ||
#ifdef WIN32
		    (WSAGetLastError() != WSAEWOULDBLOCK)
#else
		    (errno != EAGAIN && errno != EWOULDBLOCK)
#endif		     
		    ) {
		    running = 0;
		}
		currLen= 0;
	    }

            if (!reportstruct->emptyreport) {
                // read the datagram ID and sentTime out of the buffer 
                reportstruct->packetID = ntohl( mBuf_UDP->id ); 
                reportstruct->sentTime.tv_sec = ntohl( mBuf_UDP->tv_sec  );
                reportstruct->sentTime.tv_usec = ntohl( mBuf_UDP->tv_usec ); 
		reportstruct->packetLen = currLen;
		if (cmsg->cmsg_level == SOL_SOCKET &&
		    cmsg->cmsg_type  == SCM_TIMESTAMP &&
		    cmsg->cmsg_len   == CMSG_LEN(sizeof(struct timeval))) {
			memcpy(&(reportstruct->packetTime), CMSG_DATA(cmsg), sizeof(struct timeval));
		} else {
		    gettimeofday( &(reportstruct->packetTime), NULL );
		}
            }
#else
            // perform read 
            currLen = recv( mSettings->mSock, mBuf, mSettings->mBufLen, 0 );
	    if (currLen <= 0) {
		reportstruct->emptyreport=1;
                // End loop on 0 read or socket error
		// except for socket read timeout
		if (currLen == 0 ||
#ifdef WIN32
		    (WSAGetLastError() != WSAEWOULDBLOCK)
#else
		    (errno != EAGAIN && errno != EWOULDBLOCK)
#endif		     
		    ) {
		    running = 0;
		}
		currLen = 0;
	    }
            if (!reportstruct->emptyreport) {
#ifdef HAVE_CLOCK_GETTIME
		{
		    struct timespec t1; 
		    clock_gettime(CLOCK_REALTIME, &t1);
		    reportstruct->packetTime.tv_sec = t1.tv_sec;
		    reportstruct->packetTime.tv_usec = t1.tv_nsec / 1000;
		}
#else 
		gettimeofday( &(reportstruct->packetTime), NULL );
#endif    
		reportstruct->packetLen = currLen;
                // read the datagram ID and sentTime out of the buffer 
		reportstruct->packetID = ntohl( mBuf_UDP->id ); 
		reportstruct->sentTime.tv_sec = ntohl( mBuf_UDP->tv_sec  );
		reportstruct->sentTime.tv_usec = ntohl( mBuf_UDP->tv_usec ); 
            }
#endif
	    totLen += currLen;
            // terminate when datagram begins with negative index 
            // the datagram ID should be correct, just negated 
            if ( reportstruct->packetID < 0 ) {
                reportstruct->packetID = -reportstruct->packetID;
                currLen = -1;
		running = 0; 
            }
	    if (mMode_Time && mEndTime.before( reportstruct->packetTime)) {
		running = 0;
	    }
	    ReportPacket( mSettings->reporthdr, reportstruct );
        } while (running); 
                
        // stop timing 
#ifdef HAVE_CLOCK_GETTIME
       {
	   struct timespec t1; 
	   clock_gettime(CLOCK_REALTIME, &t1);
	   reportstruct->packetTime.tv_sec = t1.tv_sec;
	   reportstruct->packetTime.tv_usec = t1.tv_nsec / 1000;
       }
#else 
        gettimeofday( &(reportstruct->packetTime), NULL );
#endif            
        CloseReport( mSettings->reporthdr, reportstruct );
        
        // send a acknowledgement back only if we're NOT receiving multicast 
        if (!isMulticast( mSettings ) ) {
            // send back an acknowledgement of the terminating datagram 
            write_UDP_AckFIN( ); 
        }
    } else {
        FAIL(1, "Out of memory! Closing server thread\n", mSettings);
    }

    Mutex_Lock( &clients_mutex );     
    Iperf_delete( &(mSettings->peer), &clients ); 
    Mutex_Unlock( &clients_mutex );

    DELETE_PTR( reportstruct );
    EndReport( mSettings->reporthdr );
} 
// end Recv 

/* ------------------------------------------------------------------- 
 * Send an AckFIN (a datagram acknowledging a FIN) on the socket, 
 * then select on the socket for some time. If additional datagrams 
 * come in, probably our AckFIN was lost and they are re-transmitted 
 * termination datagrams, so re-transmit our AckFIN. 
 * ------------------------------------------------------------------- */ 

void Server::write_UDP_AckFIN( ) {

    int rc; 

    fd_set readSet; 
    FD_ZERO( &readSet ); 

    struct timeval timeout; 

    int count = 0; 
    while ( count < 10 ) {
        count++; 

        UDP_datagram *UDP_Hdr;
        server_hdr *hdr;

        UDP_Hdr = (UDP_datagram*) mBuf;

        if ( mSettings->mBufLen > (int) ( sizeof( UDP_datagram )
                                          + sizeof( server_hdr ) ) ) {
            Transfer_Info *stats = GetReport( mSettings->reporthdr );
            hdr = (server_hdr*) (UDP_Hdr+1);

            hdr->flags        = htonl( HEADER_VERSION1 );
            hdr->total_len1   = htonl( (long) (stats->TotalLen >> 32) );
            hdr->total_len2   = htonl( (long) (stats->TotalLen & 0xFFFFFFFF) );
            hdr->stop_sec     = htonl( (long) stats->endTime );
            hdr->stop_usec    = htonl( (long)((stats->endTime - (long)stats->endTime) * rMillion));
            hdr->error_cnt    = htonl( stats->cntError );
            hdr->outorder_cnt = htonl( stats->cntOutofOrder );
            hdr->datagrams    = htonl( stats->cntDatagrams );
            hdr->jitter1      = htonl( (long) stats->jitter );
            hdr->jitter2      = htonl( (long) ((stats->jitter - (long)stats->jitter) * rMillion) );
            hdr->minTransit1  = htonl( (long) stats->transit.totminTransit );
            hdr->minTransit2  = htonl( (long) ((stats->transit.totminTransit - (long)stats->transit.totminTransit) * rMillion) );
            hdr->maxTransit1  = htonl( (long) stats->transit.totmaxTransit );
            hdr->maxTransit2  = htonl( (long) ((stats->transit.totmaxTransit - (long)stats->transit.totmaxTransit) * rMillion) );
            hdr->sumTransit1  = htonl( (long) stats->transit.totsumTransit );
            hdr->sumTransit2  = htonl( (long) ((stats->transit.totsumTransit - (long)stats->transit.totsumTransit) * rMillion) );
            hdr->meanTransit1  = htonl( (long) stats->transit.totmeanTransit );
            hdr->meanTransit2  = htonl( (long) ((stats->transit.totmeanTransit - (long)stats->transit.totmeanTransit) * rMillion) );
            hdr->m2Transit1  = htonl( (long) stats->transit.totm2Transit );
            hdr->m2Transit2  = htonl( (long) ((stats->transit.totm2Transit - (long)stats->transit.totm2Transit) * rMillion) );
            hdr->vdTransit1  = htonl( (long) stats->transit.totvdTransit );
            hdr->vdTransit2  = htonl( (long) ((stats->transit.totvdTransit - (long)stats->transit.totvdTransit) * rMillion) );
            hdr->cntTransit   = htonl( stats->transit.totcntTransit );
	    hdr->IPGcnt = htonl( (long) (stats->cntDatagrams / (stats->endTime - stats->startTime)));
	    hdr->IPGsum = htonl(1);
        }

        // write data 
        write( mSettings->mSock, mBuf, mSettings->mBufLen ); 

        // wait until the socket is readable, or our timeout expires 
        FD_SET( mSettings->mSock, &readSet ); 
        timeout.tv_sec  = 1; 
        timeout.tv_usec = 0; 

        rc = select( mSettings->mSock+1, &readSet, NULL, NULL, &timeout ); 
        FAIL_errno( rc == SOCKET_ERROR, "select", mSettings ); 

        if ( rc == 0 ) {
            // select timed out 
            return; 
        } else {
            // socket ready to read 
            rc = read( mSettings->mSock, mBuf, mSettings->mBufLen ); 
            WARN_errno( rc < 0, "read" );
            if ( rc <= 0 ) {
                // Connection closed or errored
                // Stop using it.
                return;
            }
        } 
    } 

    fprintf( stderr, warn_ack_failed, mSettings->mSock, count ); 
} 
// end write_UDP_AckFIN 

