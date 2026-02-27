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
 * ReportDefault.c
 * by Kevin Gibbs <kgibbs@nlanr.net>
 *
 * ________________________________________________________________ */

#include <math.h>
#include "headers.h"
#include "Settings.hpp"
#include "util.h"
#include "Reporter.h"
#include "report_default.h"
#include "Thread.h"
#include "Locale.h"
#include "PerfSocket.hpp"
#include "SocketAddr.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Prints transfer reports in default style
 */
void reporter_printstats( Transfer_Info *stats ) {
    static char header_printed = 0;

    byte_snprintf( buffer, sizeof(buffer)/2, (double) stats->TotalLen,
                   toupper( (int)stats->mFormat));
    byte_snprintf( &buffer[sizeof(buffer)/2], sizeof(buffer)/2,
                   (stats->TotalLen > 0) ? 
		   stats->TotalLen / (stats->endTime - stats->startTime) : 0, 
                   stats->mFormat);
    // TCP reports
    if (!stats->mUDP) {
	if (!stats->mEnhanced) {
	    if( !header_printed ) {
		printf( "%s", report_bw_header);
		header_printed = 1;
	    }
	    printf(report_bw_format, stats->transferID, 
		   stats->startTime, stats->endTime, 
		   buffer, &buffer[sizeof(buffer)/2]);
	} else {
	    if( !header_printed ) {
		printf((stats->mTCP == (char)kMode_Server ? report_bw_read_enhanced_header : report_bw_write_enhanced_header), (stats->tcp.read.binsize/1024.0));
		header_printed = 1;
	    }
	    if (stats->mTCP == (char)kMode_Server) {
		printf(report_bw_read_enhanced_format, 
		       stats->transferID, stats->startTime, stats->endTime, 
		       buffer, &buffer[sizeof(buffer)/2],
		       stats->tcp.read.cntRead,
		       stats->tcp.read.bins[0],
		       stats->tcp.read.bins[1],
		       stats->tcp.read.bins[2],
		       stats->tcp.read.bins[3],
		       stats->tcp.read.bins[4],
		       stats->tcp.read.bins[5],
		       stats->tcp.read.bins[6],
		       stats->tcp.read.bins[7]);
	    } else {
		printf(report_bw_write_enhanced_format, 
		       stats->transferID, stats->startTime, stats->endTime, 
		       buffer, &buffer[sizeof(buffer)/2],
		       stats->tcp.write.WriteCnt,
		       stats->tcp.write.WriteErr,
		       stats->tcp.write.TCPretry,
		       stats->tcp.write.cwnd,
		       stats->tcp.write.rtt);
	    }
	}	
    } else if ( stats->mUDP == (char)kMode_Client ) {
	// UDP Client reporting
	if( !header_printed ) {
	    printf( "%s", (stats->mEnhanced ? report_bw_pps_enhanced_header : report_bw_header));
	    header_printed = 1;
	}
	printf( stats->mEnhanced ? report_bw_pps_enhanced_format : report_bw_format, stats->transferID, 
		stats->startTime, stats->endTime, 
		buffer, &buffer[sizeof(buffer)/2],
		(stats->IPGcnt ? (stats->IPGcnt / stats->IPGsum) : 0.0));
    } else {
        // UDP Server Reporting
        if( !header_printed ) {
	    printf( "%s", (stats->mEnhanced ? report_bw_jitter_loss_enhanced_header : report_bw_jitter_loss_header));
            header_printed = 1;
        }
	if (stats->IPGcnt) {
	    if (stats->mEnhanced) {
		// If the min latency is out of bounds of a realistic value 
		// assume the clocks are not synched and suppress the 
		// latency output
		if ((stats->transit.minTransit > UNREALISTIC_LATENCYMINMAX) ||
		    (stats->transit.minTransit < UNREALISTIC_LATENCYMINMIN)) {
		    printf( report_bw_jitter_loss_suppress_enhanced_format, stats->transferID,
			    stats->startTime, stats->endTime, 
			    buffer, &buffer[sizeof(buffer)/2],
			    stats->jitter*1000.0, stats->cntError, stats->cntDatagrams,
			    (100.0 * stats->cntError) / stats->cntDatagrams,
			    (stats->IPGcnt / stats->IPGsum));
		} else { 
		    printf( report_bw_jitter_loss_enhanced_format, stats->transferID, 
			    stats->startTime, stats->endTime, 
			    buffer, &buffer[sizeof(buffer)/2],
			    stats->jitter*1000.0, stats->cntError, stats->cntDatagrams,
			    (100.0 * stats->cntError) / stats->cntDatagrams,
			    (stats->transit.sumTransit / stats->transit.cntTransit)*1000.0,
			    stats->transit.minTransit*1000.0,
			    stats->transit.maxTransit*1000.0,
			    (stats->transit.cntTransit < 2) ? 0 : sqrt(stats->transit.m2Transit / (stats->transit.cntTransit - 1)) / 1000,
			    (stats->IPGcnt / stats->IPGsum));
		}
	    } else {
		printf( report_bw_jitter_loss_format, stats->transferID, 
			stats->startTime, stats->endTime, 
			buffer, &buffer[sizeof(buffer)/2],
			stats->jitter*1000.0, stats->cntError, stats->cntDatagrams,
			(100.0 * stats->cntError) / stats->cntDatagrams);
	    }
	} else {
	    printf( stats->mEnhanced ? report_bw_jitter_loss_suppress_enhanced_format : report_bw_jitter_loss_format , stats->transferID, 
		    stats->startTime, stats->endTime, 
		    buffer, &buffer[sizeof(buffer)/2],
		    0.0, stats->cntError, 
		    stats->cntDatagrams, 
		    0.0,0.0,0.0,0.0,0.0,0.0);
	}
	if ( stats->cntOutofOrder > 0 ) {
	    printf( report_outoforder,
		    stats->transferID, stats->startTime, 
		    stats->endTime, stats->cntOutofOrder );
	}
    }
    // Reset the enhanced stats for the next report interval 
    if (stats->mEnhanced) {
	if (stats->mUDP) {
	    stats->transit.minTransit=stats->transit.lastTransit;
	    stats->transit.maxTransit=stats->transit.lastTransit;
	    stats->transit.sumTransit = stats->transit.lastTransit;
	    stats->transit.cntTransit = 0;
	    stats->transit.vdTransit = 0;
	    stats->transit.meanTransit = 0;
	    stats->transit.m2Transit = 0;
	}
    }
 
    if ( stats->free == 1 && stats->mUDP == (char)kMode_Client ) {
        printf( report_datagrams, stats->transferID, stats->cntDatagrams ); 
    }
}


/*
 * Prints multiple transfer reports in default style
 */
void reporter_multistats( Transfer_Info *stats ) {

    byte_snprintf( buffer, sizeof(buffer)/2, (double) stats->TotalLen,
                   toupper( (int)stats->mFormat));
    byte_snprintf( &buffer[sizeof(buffer)/2], sizeof(buffer)/2,
                   stats->TotalLen / (stats->endTime - stats->startTime), 
                   stats->mFormat);

    if (!stats->mEnhanced) {
	if (stats->mUDP == (char)kMode_Server) {
        // UDP Reporting
	    printf(report_sum_bw_jitter_loss_format, 
                stats->startTime, stats->endTime, 
                buffer, &buffer[sizeof(buffer)/2],
                stats->jitter*1000.0, stats->cntError, stats->cntDatagrams,
		   (100.0 * stats->cntError) / stats->cntDatagrams);
	} else {
        // TCP Reporting
	    printf(report_sum_bw_format, 
		    stats->startTime, stats->endTime, 
		    buffer, &buffer[sizeof(buffer)/2]);
	}
    } else {
	if (stats->mUDP) {
	    // UDP Enhanced Reporting
	    printf( report_sum_bw_pps_enhanced_format, 
		    stats->startTime, stats->endTime, 
		    buffer, &buffer[sizeof(buffer)/2],
		    (stats->IPGcnt ? (stats->IPGcnt / stats->IPGsum) : 0.0));
	} else {
	    // TCP Enhanced Reporting
	    if (stats->mTCP == (char)kMode_Client) {
		printf( report_sum_bw_write_enhanced_format,
			stats->startTime, stats->endTime, 
			buffer, &buffer[sizeof(buffer)/2],
			stats->tcp.write.WriteCnt,
			stats->tcp.write.WriteErr,
			stats->tcp.write.TCPretry);
	    } else {
		printf( report_sum_bw_read_enhanced_format,
			stats->startTime, stats->endTime, 
			buffer, &buffer[sizeof(buffer)/2],
			stats->tcp.read.cntRead,
			stats->tcp.read.bins[0],
			stats->tcp.read.bins[1],
			stats->tcp.read.bins[2],
			stats->tcp.read.bins[3],
			stats->tcp.read.bins[4],
			stats->tcp.read.bins[5],
			stats->tcp.read.bins[6],
			stats->tcp.read.bins[7]);
	    }		    
	}
    }
    if ((stats->mUDP == kMode_Server) && stats->cntOutofOrder > 0 ) {
            printf( report_sum_outoforder,
                    stats->startTime, 
                    stats->endTime, stats->cntOutofOrder );
    }
    if ((stats->free == 1) && (stats->mUDP == (char)kMode_Client)) {
        printf( report_sum_datagrams, stats->cntDatagrams ); 
    }
}

/*
 * Prints server transfer reports in default style
 */
void reporter_serverstats( Connection_Info *nused, Transfer_Info *stats ) {
    printf( server_reporting, stats->transferID );
    reporter_printstats( stats );
}

/*
 * Report the client or listener Settings in default style
 */
void reporter_reportsettings( ReporterData *data ) {
    int win, win_requested;
    int pid =  (int)  getpid();

    win = getsock_tcp_windowsize( data->info.transferID,
                  (data->mThreadMode == kMode_Listener ? 0 : 1) );
    win_requested = data->mTCPWin;
    printf( "%s", separator_line );
    if ( data->mThreadMode == kMode_Listener ) {
        printf(isEnhanced(data) ? server_pid_port : server_port,
                (isUDP( data ) ? "UDP" : "TCP"),
                data->mPort, pid );
    } else {
        printf(isEnhanced(data) ? client_pid_port : client_port,
                data->mHost,
                (isUDP( data ) ? "UDP" : "TCP"),
                data->mPort, pid);
    }

    if ( data->mLocalhost != NULL ) {
        printf( bind_address, data->mLocalhost );
	if ((data->mThreadMode != kMode_Client) && \
	    SockAddr_isMulticast(&data->connection.local)) {
              printf( join_multicast, data->mLocalhost );
        }
    }

    if ( isUDP( data ) ) {

	if (data->mThreadMode != kMode_Listener) {
	    double delay_target;
	    if (data->mUDPRateUnits == kRate_BW) { 
		delay_target = (double) ( data->mBufLen * 8000000.0 / data->mUDPRate);
	    } else {
		delay_target = (1e6 / data->mUDPRate);
	    }
#ifdef HAVE_KALMAN
	    printf(client_datagram_size_kalman, data->mBufLen, delay_target);
#else 	
	    printf(client_datagram_size, data->mBufLen, delay_target);
#endif
	} else {
	    printf(server_datagram_size, data->mBufLen);
	}
        if ( SockAddr_isMulticast( &data->connection.peer ) ) {
            printf( multicast_ttl, data->info.mTTL);
        }
    }
    byte_snprintf( buffer, sizeof(buffer), win,
                   toupper( (int)data->info.mFormat));
    printf( "%s: %s", (isUDP( data ) ? 
                                udp_buffer_size : tcp_window_size), buffer );

    if ( win_requested == 0 ) {
        printf( " %s", window_default );
    } else if ( win != win_requested ) {
        byte_snprintf( buffer, sizeof(buffer), win_requested,
                       toupper( (int)data->info.mFormat));
        printf( warn_window_requested, buffer );
    }
    printf( "\n%s", separator_line );
 }

/*
 * Report a socket's peer IP address in default style
 */
void *reporter_reportpeer( Connection_Info *stats, int ID ) {
    if ( ID > 0 ) {
        // copy the inet_ntop into temp buffers, to avoid overwriting
        char local_addr[ REPORT_ADDRLEN ];
        char remote_addr[ REPORT_ADDRLEN ];
        struct sockaddr *local = ((struct sockaddr*)&stats->local);
        struct sockaddr *peer = ((struct sockaddr*)&stats->peer);
    
        if ( local->sa_family == AF_INET ) {
            inet_ntop( AF_INET, &((struct sockaddr_in*)local)->sin_addr, 
                       local_addr, REPORT_ADDRLEN);
        }
#ifdef HAVE_IPV6
          else {
            inet_ntop( AF_INET6, &((struct sockaddr_in6*)local)->sin6_addr, 
                       local_addr, REPORT_ADDRLEN);
        }
#endif
    
        if ( peer->sa_family == AF_INET ) {
            inet_ntop( AF_INET, &((struct sockaddr_in*)peer)->sin_addr, 
                       remote_addr, REPORT_ADDRLEN);
        }
#ifdef HAVE_IPV6
          else {
            inet_ntop( AF_INET6, &((struct sockaddr_in6*)peer)->sin6_addr, 
                       remote_addr, REPORT_ADDRLEN);
        }
#endif
    
        printf( report_peer,
                ID,
                local_addr,  ( local->sa_family == AF_INET ?
                              ntohs(((struct sockaddr_in*)local)->sin_port) :
#ifdef HAVE_IPV6
                              ntohs(((struct sockaddr_in6*)local)->sin6_port)),
#else
                              0),
#endif
                remote_addr, ( peer->sa_family == AF_INET ?
                              ntohs(((struct sockaddr_in*)peer)->sin_port) :
#ifdef HAVE_IPV6
                              ntohs(((struct sockaddr_in6*)peer)->sin6_port)));
#else
                              0));
#endif
    }
    return NULL;
}
// end ReportPeer

/* -------------------------------------------------------------------
 * Report the MSS and MTU, given the MSS (or a guess thereof)
 * ------------------------------------------------------------------- */

// compare the MSS against the (MTU - 40) to (MTU - 80) bytes.
// 40 byte IP header and somewhat arbitrarily, 40 more bytes of IP options.

#define checkMSS_MTU( inMSS, inMTU ) (inMTU-40) >= inMSS  &&  inMSS >= (inMTU-80)

void reporter_reportMSS( int inMSS, thread_Settings *inSettings ) {
    if ( inMSS <= 0 ) {
        printf( report_mss_unsupported, inSettings->mSock );
    } else {
        char* net;
        int mtu = 0;

        if ( checkMSS_MTU( inMSS, 1500 ) ) {
            net = "ethernet";
            mtu = 1500;
        } else if ( checkMSS_MTU( inMSS, 4352 ) ) {
            net = "FDDI";
            mtu = 4352;
        } else if ( checkMSS_MTU( inMSS, 9180 ) ) {
            net = "ATM";
            mtu = 9180;
        } else if ( checkMSS_MTU( inMSS, 65280 ) ) {
            net = "HIPPI";
            mtu = 65280;
        } else if ( checkMSS_MTU( inMSS, 576 ) ) {
            net = "minimum";
            mtu = 576;
            printf( "%s", warn_no_pathmtu );
        } else {
            mtu = inMSS + 40;
            net = "unknown interface";
        }

        printf( report_mss,
                inSettings->mSock, inMSS, mtu, net );
    }
}
// end ReportMSS

#ifdef __cplusplus
} /* end extern "C" */
#endif
