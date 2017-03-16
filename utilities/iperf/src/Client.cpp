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
 * Client.cpp
 * by Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * A client thread initiates a connect to the server and handles
 * sending and receiving data, then closes the socket.
 * ------------------------------------------------------------------- */

#include <time.h>
#include "headers.h"
#include "Client.hpp"
#include "Thread.h"
#include "SocketAddr.h"
#include "PerfSocket.hpp"
#include "Extractor.h"
#include "delay.h"
#include "util.h"
#include "Locale.h"
#ifdef HAVE_SCHED_SETSCHEDULER
#include <sched.h>
#endif
#ifdef HAVE_MLOCKALL
#include <sys/mman.h>
#endif

/* -------------------------------------------------------------------
 * Store server hostname, optionally local hostname, and socket info.
 * ------------------------------------------------------------------- */

Client::Client( thread_Settings *inSettings ) {
    mSettings = inSettings;
    mBuf = NULL;

    // initialize buffer
    mBuf = new char[ mSettings->mBufLen ];
    pattern( mBuf, mSettings->mBufLen );
    if ( isFileInput( mSettings ) ) {
        if ( !isSTDIN( mSettings ) )
            Extractor_Initialize( mSettings->mFileName, mSettings->mBufLen, mSettings );
        else
            Extractor_InitializeFile( stdin, mSettings->mBufLen, mSettings );

        if ( !Extractor_canRead( mSettings ) ) {
            unsetFileInput( mSettings );
        }
    }

    // connect
    Connect( );

    if ( isReport( inSettings ) ) {
        ReportSettings( inSettings );
        if ( mSettings->multihdr && isMultipleReport( inSettings ) ) {
            mSettings->multihdr->report->connection.peer = mSettings->peer;
            mSettings->multihdr->report->connection.size_peer = mSettings->size_peer;
            mSettings->multihdr->report->connection.local = mSettings->local;
            SockAddr_setPortAny( &mSettings->multihdr->report->connection.local );
            mSettings->multihdr->report->connection.size_local = mSettings->size_local;
        }
    }

} // end Client

/* -------------------------------------------------------------------
 * Delete memory (hostname strings).
 * ------------------------------------------------------------------- */

Client::~Client() {
    if ( mSettings->mSock != INVALID_SOCKET ) {
        int rc = close( mSettings->mSock );
        WARN_errno( rc == SOCKET_ERROR, "close" );
        mSettings->mSock = INVALID_SOCKET;
    }
    DELETE_ARRAY( mBuf );
} // end ~Client

// const double kSecs_to_usecs = 1e6; 
const double kSecs_to_nsecs = 1e9; 
const int    kBytes_to_Bits = 8; 

// A version of the transmit loop that
// supports TCP rate limiting using a token bucket
void Client::RunRateLimitedTCP ( void ) {
    int currLen = 0;
#ifdef HAVE_SETITIMER
    struct itimerval it;
#endif
    max_size_t totLen = 0;
    double time1, time2 = 0, tokens;
    tokens=0;

#ifdef HAVE_CLOCK_GETTIME
    struct timespec t1; 
    clock_gettime(CLOCK_REALTIME, &t1);
    time1 = t1.tv_sec + (t1.tv_nsec / 1000000000.0);
#else 
    struct timeval t1;
    gettimeofday( &t1, NULL );
    time1 = t1.tv_sec + (t1.tv_usec / 1000000.0);
#endif    

    char* readAt = mBuf;

    // Indicates if the stream is readable 
    bool canRead = true, mMode_Time = isModeTime( mSettings ); 

    ReportStruct *reportstruct = NULL;

    // InitReport handles Barrier for multiple Streams
    mSettings->reporthdr = InitReport( mSettings );
    reportstruct = new ReportStruct;
    reportstruct->packetID = 0;
    reportstruct->emptyreport=0;
    reportstruct->socket = mSettings->mSock;
    
    lastPacketTime.setnow();
    if ( mMode_Time ) {
#ifdef HAVE_SETITIMER
        int err;
	memset (&it, 0, sizeof (it));
	it.it_value.tv_sec = (int) (mSettings->mAmount / 100.0);
	it.it_value.tv_usec = (int) (10000 * (mSettings->mAmount -
                                     it.it_value.tv_sec * 100.0));
	err = setitimer( ITIMER_REAL, &it, NULL );
	FAIL_errno( err != 0, "setitimer", mSettings);
#else 
        mEndTime.setnow();
	mEndTime.add( mSettings->mAmount / 100.0 );
#endif 
    }
    while (1) {
        // Read the next data block from 
        // the file if it's file input 
        if ( isFileInput( mSettings ) ) {
            Extractor_getNextDataBlock( readAt, mSettings ); 
            canRead = Extractor_canRead( mSettings ) != 0; 
        } else
            canRead = true; 
	// Add tokens per the loop time
	// clock_gettime is much cheaper than gettimeofday() so 
	// use it if possible. 
#ifdef HAVE_CLOCK_GETTIME
	clock_gettime(CLOCK_REALTIME, &t1);
	time2 = t1.tv_sec + (t1.tv_nsec / 1000000000.0);
	tokens += (time2 - time1) * (mSettings->mUDPRate / 8.0);
	time1 = time2;
#else 
	if (!time2) 
	    gettimeofday( &t1, NULL );
	time2 = t1.tv_sec + (t1.tv_usec / 1000000.0);
	tokens += (time2 - time1) * (mSettings->mUDPRate / 8.0);
	time1 = time2;
	time2 = 0;
#endif    
	if (tokens >= 0) { 
	    // perform write 
	    reportstruct->errwrite=0;
	    currLen = write( mSettings->mSock, mBuf, mSettings->mBufLen );
	    if ( currLen < 0 ) {
		reportstruct->errwrite=1;
		currLen = 0;
		if (
#ifdef WIN32
		    (errno = WSAGetLastError()) != WSAETIMEDOUT
#else
		    errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR
#endif
		    ) {
		    WARN_errno( 1 , "write");
		    break;
		}
	    }
	    // Consume tokens per the transmit
	    tokens -= currLen;
	    totLen += currLen;

#ifndef HAVE_SETITIMER
	    // Get the time for so the loop can 
	    // end if the running time exceeds the requested
	    gettimeofday( &(reportstruct->packetTime), NULL );
# ifndef HAVE_CLOCK_GETTIME
	    // leverage the packet gettimeofday reducing
	    // these sys calls (which can be expensive)
	    // time2 is used for the token bucket adjust
	    time2 = reportstruct->packetTime.tv_sec + (reportstruct->packetTime.tv_usec / 1000000.0);
# endif
#endif
	    if(mSettings->mInterval > 0) {
#ifdef HAVE_SETITIMER
		gettimeofday( &(reportstruct->packetTime), NULL );
# ifndef HAVE_CLOCK_GETTIME
		// leverage the packet gettimeofday reducing
		// these sys calls (which can be expensive)
		time2 = reportstruct->packetTime.tv_sec + (reportstruct->packetTime.tv_usec / 1000000.0);
# endif
#endif
		reportstruct->packetLen = currLen;
		ReportPacket( mSettings->reporthdr, reportstruct );
	    }	

	    if ( !mMode_Time ) {
		/* mAmount may be unsigned, so don't let it underflow! */
		if( mSettings->mAmount >= (unsigned long) currLen ) {
		    mSettings->mAmount -= (unsigned long) currLen;
		} else {
		    mSettings->mAmount = 0;
		}
	    }
        } else {
	    // Use a 4 usec delay to fill tokens
	    delay_loop(4);
	}
#ifdef HAVE_SETITIMER
	if (sInterupted || 
	    (!mMode_Time  && (mSettings->mAmount <= 0 || !canRead)))
	    break;
#else 
	if (sInterupted || 
	    (mMode_Time   &&  mEndTime.before(reportstruct->packetTime))  || 
	    (!mMode_Time  && (mSettings->mAmount <= 0 || !canRead)))
	    break;
#endif	
    }

    // stop timing
    gettimeofday( &(reportstruct->packetTime), NULL );

    // if we're not doing interval reporting, report the entire transfer as one big packet
    if(0.0 == mSettings->mInterval) {
        reportstruct->packetLen = totLen;
        ReportPacket( mSettings->reporthdr, reportstruct );
    }
    CloseReport( mSettings->reporthdr, reportstruct );

    DELETE_PTR( reportstruct );
    EndReport( mSettings->reporthdr );
}

void Client::RunTCP( void ) {
    int currLen = 0;
    max_size_t totLen = 0;

    char* readAt = mBuf;

    // Indicates if the stream is readable 
    bool canRead = true, mMode_Time = isModeTime( mSettings ); 

    ReportStruct *reportstruct = NULL;

    // InitReport handles Barrier for multiple Streams
    mSettings->reporthdr = InitReport( mSettings );
    reportstruct = new ReportStruct;
    reportstruct->packetID = 0;
    reportstruct->emptyreport=0;
    reportstruct->socket = mSettings->mSock;

    lastPacketTime.setnow();

    /*
     * Terminate the thread by setitimer's alarm (if possible)
     * as the alarm will break a blocked syscall (i.e. the write)
     * and provide for accurate timing. Otherwise the thread cannot 
     * terminate until the write completes and since this is 
     * a blocking write the time may not be exact to the request. 
     *
     * In this case of no setitimer we're just using the gettimeofday
     * calls to determine if the loop time exceeds the request time
     * and the blocking writes will affect timing.  The socket has set 
     * SO_SNDTIMEO to 1/2 the overall time (which should help limit 
     * gross error) or 1/2 the report interval time (better precision)
     *
     * Side note: The advantage of not using interval reports is that
     * the code path won't make any gettimeofday calls in the main loop
     * which are expensive syscalls.
     */ 
    if ( mMode_Time ) {
#ifdef HAVE_SETITIMER
        int err;
        struct itimerval it;
	memset (&it, 0, sizeof (it));
	it.it_value.tv_sec = (int) (mSettings->mAmount / 100.0);
	it.it_value.tv_usec = (int) (10000 * (mSettings->mAmount -
					      it.it_value.tv_sec * 100.0));
	err = setitimer( ITIMER_REAL, &it, NULL );
	FAIL_errno( err != 0, "setitimer", mSettings ); 
#else
        mEndTime.setnow();
        mEndTime.add( mSettings->mAmount / 100.0 );
#endif
    }
    while (1) {
        // Read the next data block from 
        // the file if it's file input 
        if ( isFileInput( mSettings ) ) {
	    Extractor_getNextDataBlock( readAt, mSettings ); 
            canRead = Extractor_canRead( mSettings ) != 0; 
        } else
            canRead = true; 

        // perform write 
	reportstruct->errwrite=0;
        currLen = write( mSettings->mSock, mBuf, mSettings->mBufLen );
        if ( currLen < 0 ) {
	    reportstruct->errwrite=1;
	    currLen = 0;
	    if (
#ifdef WIN32
		(errno = WSAGetLastError()) != WSAETIMEDOUT
#else
		errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR
#endif
		) {
	        WARN_errno( 1, "write" );
	        break;
	    }
        }

	totLen += currLen;
#ifndef HAVE_SETITIMER
	gettimeofday( &(reportstruct->packetTime), NULL );
#endif
	if(mSettings->mInterval > 0) {
#ifdef HAVE_SETITIMER
    	    gettimeofday( &(reportstruct->packetTime), NULL );
#endif
            reportstruct->packetLen = currLen;
            ReportPacket( mSettings->reporthdr, reportstruct );
        }	

        if ( !mMode_Time ) {
            /* mAmount may be unsigned, so don't let it underflow! */
            if( mSettings->mAmount >= (unsigned long) currLen ) {
                mSettings->mAmount -= (unsigned long) currLen;
            } else {
                mSettings->mAmount = 0;
            }
        }
#ifdef HAVE_SETITIMER
	if (sInterupted || 
	    (!mMode_Time  && (mSettings->mAmount <= 0 || !canRead)))
	    break;
#else 
	if (sInterupted || 
	    (mMode_Time   &&  mEndTime.before(reportstruct->packetTime))  || 
	    (!mMode_Time  && (mSettings->mAmount <= 0 || !canRead)))
	    break;
#endif	
    }
		  

    // stop timing
    gettimeofday( &(reportstruct->packetTime), NULL );

    // if we're not doing interval reporting, report the entire transfer as one big packet
    if(0.0 == mSettings->mInterval) {
        reportstruct->packetLen = totLen;
        ReportPacket( mSettings->reporthdr, reportstruct );
    }
    CloseReport( mSettings->reporthdr, reportstruct );

    DELETE_PTR( reportstruct );
    EndReport( mSettings->reporthdr );
}

/* ------------------------------------------------------------------- 
 * Send data using the connected UDP/TCP socket, 
 * until a termination flag is reached. 
 * Does not close the socket. 
 * ------------------------------------------------------------------- */ 

void Client::Run( void ) {
    struct UDP_datagram* mBuf_UDP = (struct UDP_datagram*) mBuf; 
    int currLen; 

    double delay_target = 0; 
    double delay = 0; 
    double adjust = 0;
    double delay_lower_bounds;

    char* readAt = mBuf;

    //  Enable socket write timeouts for responsive reporting
    //  Do this after the connection establishment
    //  and after Client::InitiateServer as during thes
    //  default socket timeouts are preferred.
    {
	int sosndtimer = 0;
	// sosndtimer units microseconds
	if (mSettings->mInterval) {
	    sosndtimer = (int) (mSettings->mInterval * 1000000) / 2;
	} else if (isModeTime(mSettings)) {
	    sosndtimer = (mSettings->mAmount * 10000) / 2;
	} 
        // units nanoseconds for delay bounds
	delay_lower_bounds = (double) sosndtimer * -1e3;

	if (sosndtimer > 0) {
#ifdef WIN32
            // Windows SO_RCVTIMEO uses ms
	    DWORD timeout = (double) sosndtimer / 1e3;
#else
	    struct timeval timeout;
	    timeout.tv_sec = sosndtimer / 1000000;
	    timeout.tv_usec = sosndtimer % 1000000;
#endif
	    if (setsockopt( mSettings->mSock, SOL_SOCKET, SO_SNDTIMEO, (char *)&timeout, sizeof(timeout)) < 0 ) {
		WARN_errno( mSettings->mSock == SO_SNDTIMEO, "socket" );
	    }
	}
    }

#if HAVE_THREAD
    if ( !isUDP( mSettings ) ) {
	if (mSettings->mUDPRate > 0)
	    RunRateLimitedTCP();
	else 
	    RunTCP();
	return;
    }
#endif
    
    // Indicates if the stream is readable 
    bool canRead = true, mMode_Time = isModeTime( mSettings ); 

    // setup termination variables
    if ( mMode_Time ) {
        mEndTime.setnow();
        mEndTime.add( mSettings->mAmount / 100.0 );
    }

    if ( isUDP( mSettings ) ) {
#ifdef HAVE_SCHED_SETSCHEDULER
	if ( isRealtime( mSettings ) ) {
	    // Thread settings to support realtime operations
	    // SCHED_OTHER, SCHED_FIFO, SCHED_RR
	    struct sched_param sp;
	    sp.sched_priority = sched_get_priority_max(SCHED_RR); 

	    if (sched_setscheduler(0, SCHED_RR, &sp) < 0) {
                WARN_errno( 1, "Client set scheduler" );
#ifdef HAVE_MLOCKALL
	    } else if (mlockall(MCL_CURRENT | MCL_FUTURE) != 0) { 
		// lock the threads memory
		WARN_errno( 1, "mlockall");
#endif
	    }
	}
#endif
	// compute delay target in units of nanoseconds
	if (mSettings->mUDPRateUnits == kRate_BW) { 
	    // compute delay for bandwidth restriction, constrained to [0,1] seconds 
	    delay_target = (double) ( mSettings->mBufLen * ((kSecs_to_nsecs * kBytes_to_Bits) 
							    / mSettings->mUDPRate) );
	} else {
	    delay_target = 1e9 / mSettings->mUDPRate;
	}
	if ( delay_target < 0  || 
             delay_target > 1.0 * kSecs_to_nsecs ) {
            fprintf( stderr, warn_delay_large, delay_target / kSecs_to_nsecs ); 
            delay_target = 1.0 * kSecs_to_nsecs; 
        }
        if ( isFileInput( mSettings ) ) {
	    // Due to the UDP timestamps etc, included 
	    // reduce the read size by an amount 
	    // equal to the header size
            if ( isCompat( mSettings ) ) {
                Extractor_reduceReadSize( sizeof(struct UDP_datagram), mSettings );
                readAt += sizeof(struct UDP_datagram);
            } else {
                Extractor_reduceReadSize( sizeof(struct UDP_datagram) +
                                          sizeof(struct client_hdr), mSettings );
                readAt += sizeof(struct UDP_datagram) +
		    sizeof(struct client_hdr);
            }
        }
    }

    ReportStruct *reportstruct = NULL;

    // InitReport handles Barrier for multiple Streams
    mSettings->reporthdr = InitReport( mSettings );
    reportstruct = new ReportStruct;
    reportstruct->packetID = 0;
    reportstruct->emptyreport=0;
    reportstruct->errwrite=0;
    reportstruct->socket = mSettings->mSock;

    lastPacketTime.setnow();
    // Set this to > 0 so first loop iteration will delay the IPG
    currLen = 1;

    do {

        // Test case: drop 17 packets and send 2 out-of-order: 
        // sequence 51, 52, 70, 53, 54, 71, 72 
        //switch( datagramID ) { 
        //  case 53: datagramID = 70; break; 
        //  case 71: datagramID = 53; break; 
        //  case 55: datagramID = 71; break; 
        //  default: break; 
        //} 
#ifdef HAVE_CLOCK_GETTIME
	struct timespec t1; 
	clock_gettime(CLOCK_REALTIME, &t1);
	reportstruct->packetTime.tv_sec = t1.tv_sec;
	reportstruct->packetTime.tv_usec = (t1.tv_nsec + 500) / 1000L;
#else 
        gettimeofday( &(reportstruct->packetTime), NULL );
#endif    
        if ( isUDP( mSettings ) ) {
            // store datagram ID into buffer 
            mBuf_UDP->id      = htonl( (reportstruct->packetID)++ ); 
            mBuf_UDP->tv_sec  = htonl( reportstruct->packetTime.tv_sec ); 
            mBuf_UDP->tv_usec = htonl( reportstruct->packetTime.tv_usec );

            // Adjustment for the running delay
	    // o measure how long the last loop iteration took
	    // o calculate the delay adjust
	    //   - If write succeeded, adjust = target IPG - the loop time
	    //   - If write failed, adjust = the loop time
	    // o then adjust the overall running delay
	    // Note: adjust units are nanoseconds, 
	    //       packet timestamps are microseconds
	    if (currLen > 0) 
	      adjust = delay_target + \
		       (1000.0 * lastPacketTime.subUsec( reportstruct->packetTime )); 
	    else 
	      adjust = 1000.0 * lastPacketTime.subUsec( reportstruct->packetTime );

            lastPacketTime.set( reportstruct->packetTime.tv_sec, 
				reportstruct->packetTime.tv_usec );
	    // Since linux nanosleep/busyloop can exceed delay 
	    // there are two possible equilibriums
	    //  1)  Try to perserve inter packet gap 
	    //  2)  Try to perserve requested transmit rate
	    // The latter seems preferred, hence use a running delay 
	    // that spans the life of the thread and constantly adjust.
	    // A negative delay means the iperf app is behind.
	    delay += adjust;
	    // Don't let delay grow unbounded
	    if (delay < delay_lower_bounds) {
		delay = delay_target;
	    }

        }

        // Read the next data block from 
        // the file if it's file input 
        if ( isFileInput( mSettings ) ) {
            Extractor_getNextDataBlock( readAt, mSettings ); 
            canRead = Extractor_canRead( mSettings ) != 0; 
        } else
            canRead = true; 

        // perform write 
        currLen = write( mSettings->mSock, mBuf, mSettings->mBufLen );
        if ( currLen < 0 ) {
	    reportstruct->errwrite = 1; 
	    reportstruct->packetID--;
	    reportstruct->emptyreport=1; 
	    currLen = 0;
	    if (
#ifdef WIN32
		(errno = WSAGetLastError()) != WSAETIMEDOUT &&
		errno != WSAECONNREFUSED
#else
		errno != EAGAIN && errno != EWOULDBLOCK &&
		errno != EINTR  && errno != ECONNREFUSED &&
		errno != ENOBUFS
#endif
		) {
	        WARN_errno( 1, "write" );
	        break;
	    }
	}

        // report packets 
        reportstruct->packetLen = (unsigned long) currLen;
        ReportPacket( mSettings->reporthdr, reportstruct );

	// Insert delay here only if the running delay is greater than 1 usec, 
        // otherwise don't delay and immediately continue with the next tx.  
        if ( delay >= 1000 ) {
	    // Convert from nanoseconds to microseconds
	    // and invoke the microsecond delay
	    delay_loop((unsigned long) (delay / 1000)); 
        }
        if ( !mMode_Time ) {
            /* mAmount may be unsigned, so don't let it underflow! */
            if( mSettings->mAmount >= (unsigned long) currLen ) {
                mSettings->mAmount -= (unsigned long) currLen;
            } else {
                mSettings->mAmount = 0;
            }
        }

    } while ( ! (sInterupted  || 
                 (mMode_Time   &&  mEndTime.before( reportstruct->packetTime ))  || 
                 (!mMode_Time  &&  0 >= mSettings->mAmount)) && canRead ); 

    // stop timing
    gettimeofday( &(reportstruct->packetTime), NULL );
    CloseReport( mSettings->reporthdr, reportstruct );

    if ( isUDP( mSettings ) ) {
        // send a final terminating datagram 
        // Don't count in the mTotalLen. The server counts this one, 
        // but didn't count our first datagram, so we're even now. 
        // The negative datagram ID signifies termination to the server. 
    
        // store datagram ID into buffer 
        mBuf_UDP->id      = htonl( -(reportstruct->packetID)  ); 
        mBuf_UDP->tv_sec  = htonl( reportstruct->packetTime.tv_sec ); 
        mBuf_UDP->tv_usec = htonl( reportstruct->packetTime.tv_usec ); 

        if ( isMulticast( mSettings ) ) {
            write( mSettings->mSock, mBuf, mSettings->mBufLen ); 
        } else {
            write_UDP_FIN( ); 
        }
    }
    DELETE_PTR( reportstruct );
    EndReport( mSettings->reporthdr );
} 
// end Run

void Client::InitiateServer() {
    if ( !isCompat( mSettings ) ) {
        int currLen;
        client_hdr* temp_hdr;
        if ( isUDP( mSettings ) ) {
            UDP_datagram *UDPhdr = (UDP_datagram *)mBuf;
            temp_hdr = (client_hdr*)(UDPhdr + 1);
        } else {
            temp_hdr = (client_hdr*)mBuf;
        }
        Settings_GenerateClientHdr( mSettings, temp_hdr );
        if ( !isUDP( mSettings ) ) {
            currLen = send( mSettings->mSock, mBuf, sizeof(client_hdr), 0 );
            if ( currLen < 0 ) {
                WARN_errno( currLen < 0, "write1" );
            }
        }
    }
}

/* -------------------------------------------------------------------
 * Setup a socket connected to a server.
 * If inLocalhost is not null, bind to that address, specifying
 * which outgoing interface to use.
 * ------------------------------------------------------------------- */

void Client::Connect( ) {
    int rc;
    SockAddr_remoteAddr( mSettings );

    assert( mSettings->inHostname != NULL );

    // create an internet socket
    int type = ( isUDP( mSettings )  ?  SOCK_DGRAM : SOCK_STREAM);

    int domain = (SockAddr_isIPv6( &mSettings->peer ) ? 
#ifdef HAVE_IPV6
                  AF_INET6
#else
                  AF_INET
#endif
                  : AF_INET);

    mSettings->mSock = socket( domain, type, 0 );
    WARN_errno( mSettings->mSock == INVALID_SOCKET, "socket" );

    SetSocketOptions( mSettings );

    SockAddr_localAddr( mSettings );
    if ( mSettings->mLocalhost != NULL ) {
        // bind socket to local address
        rc = bind( mSettings->mSock, (sockaddr*) &mSettings->local, 
                   SockAddr_get_sizeof_sockaddr( &mSettings->local ) );
        WARN_errno( rc == SOCKET_ERROR, "bind" );
    }
    // connect socket
    rc = connect( mSettings->mSock, (sockaddr*) &mSettings->peer, 
                  SockAddr_get_sizeof_sockaddr( &mSettings->peer ));
    FAIL_errno( rc == SOCKET_ERROR, "connect", mSettings );

    getsockname( mSettings->mSock, (sockaddr*) &mSettings->local, 
                 &mSettings->size_local );
    getpeername( mSettings->mSock, (sockaddr*) &mSettings->peer,
                 &mSettings->size_peer );
} // end Connect

/* ------------------------------------------------------------------- 
 * Send a datagram on the socket. The datagram's contents should signify 
 * a FIN to the application. Keep re-transmitting until an 
 * acknowledgement datagram is received. 
 * ------------------------------------------------------------------- */ 

void Client::write_UDP_FIN( ) {
    int rc; 
    fd_set readSet; 
    struct timeval timeout; 
    struct UDP_datagram* mBuf_UDP = (struct UDP_datagram*) mBuf; 

    int count = 0; 
    int packetid;
    while ( count < 10 ) {
        count++; 

        // write data 
        write( mSettings->mSock, mBuf, mSettings->mBufLen ); 
	// decrement the packet count
	packetid = ntohl(mBuf_UDP->id);
        mBuf_UDP->id = htonl(--packetid); 

        // wait until the socket is readable, or our timeout expires 
        FD_ZERO( &readSet ); 
        FD_SET( mSettings->mSock, &readSet ); 
        timeout.tv_sec  = 0; 
        timeout.tv_usec = 250000; // quarter second, 250 ms 

        rc = select( mSettings->mSock+1, &readSet, NULL, NULL, &timeout ); 
        FAIL_errno( rc == SOCKET_ERROR, "select", mSettings ); 

        if ( rc == 0 ) {
            // select timed out 
            continue; 
        } else {
            // socket ready to read 
            rc = read( mSettings->mSock, mBuf, mSettings->mBufLen ); 
            WARN_errno( rc < 0, "read" );
    	    if ( rc < 0 ) {
                break;
            } else if ( rc >= (int) (sizeof(UDP_datagram) + sizeof(server_hdr)) ) {
                ReportServerUDP( mSettings, (server_hdr*) ((UDP_datagram*)mBuf + 1) );
            }

            return; 
        } 
    } 

    fprintf( stderr, warn_no_ack, mSettings->mSock, count ); 
} 
// end write_UDP_FIN 
