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
 * Locale.c
 * by Ajay Tirumala <tirumala@ncsa.uiuc.edu>
 * & Mark Gates <mgates@nlanr.net>
 * -------------------------------------------------------------------
 * Strings and other stuff that is locale specific.
 * ------------------------------------------------------------------- */

/* Format specifiers per inttypes.h
 *
 * macro	description	example
 * PRIxMAX	printf specifier for intmax_t	PRIiMAX is the equivalent of i (in "%i") for intmax_t values
 * PRIxN	printf specifier for intN_t	PRId16 is the equivalent of d (in "%d") for int16_t values
 * PRIxLEASTN	printf specifier for int_leastN_t	PRIuLEAST32 is the equivalent of u (in "%u") for uint32_t values
 * PRIxFASTN	printf specifier for int_fastN_t	PRIxFAST8 is the equivalent of x (in "%x") for uint8_t values
 * PRIxPTR	printf specifier for intptr_t	PRIuPTR is the equivalent of u (in "%u") for uintptr_t values
 *
 Where:
 * x is one of d, i, o,u or x (for the printf specifiers this can also be an uppercase X).*
 * N is 8, 16, 32, 64, or any other type width supported by the library in <cstdint>.
 * * The specifier assumes the type to be signed for i and d, and unsigned for o, u, x and X.
 */

#include "headers.h"
#include "version.h"
#include "gettcpinfo.h"

#ifdef __cplusplus
extern "C" {
#endif
/* -------------------------------------------------------------------
 * usage
 * ------------------------------------------------------------------- */

const char usage_short[] = "\
Usage: %s [-s|-c host] [options]\n\
Try `%s --help' for more information.\n";

const char usage_long1[] = "\
Usage: iperf [-s|-c host] [options]\n\
       iperf [-h|--help] [-v|--version]\n\
\n\
Client/Server:\n\
  -b, --bandwidth #[kmgKMG | pps]  bandwidth to read/send at in bits/sec or packets/sec\n\
  -e, --enhanced    use enhanced reporting giving more tcp/udp and traffic information\n\
  -f, --format    [kmgKMG]   format to report: Kbits, Mbits, KBytes, MBytes\n\
      --hide-ips           hide ip addresses and host names within outputs\n\
      --histograms         enable histograms (see client or server for more)\n\
  -i, --interval  #        seconds between periodic bandwidth reports\n\
  -l, --len       #[kmKM]    length of buffer in bytes to read or write (Defaults: TCP=128K, v4 UDP=1470, v6 UDP=1450)\n\
  -m, --print_mss          print TCP maximum segment size\n\
  -o, --output    <filename> output the report or error message to this specified file\n\
  -p, --port      #        client/server port to listen/send on and to connect\n\
      --permit-key         permit key to be used to verify client and server (TCP only)\n\
      --tcp-tx-delay       set socket option of TCP_TX_DELAY (units is milliseconds)\n\
      --sum-only           output sum only reports\n\
  -u, --udp                use UDP rather than TCP\n\
      --utc                use coordinated universal time (UTC) with time output\n\
  -w, --window    #[KM]    TCP window size (socket buffer size)\n"
#ifdef HAVE_SCHED_SETSCHEDULER
"  -z, --realtime           request realtime scheduler\n"
#endif
"  -B, --bind <host>[:<port>][%<dev>] bind to <host>, ip addr (including multicast address) and optional port and device\n\
  -C, --compatibility      for use with older versions does not sent extra msgs\n\
      --NUM_REPORT_STRUCTS increase the shared memory between the traffic threads and the reporter thread (default is 10,000 entries)\n\
  -M, --mss       #        set TCP maximum segment size using TCP_MAXSEG\n\
  -N, --nodelay            set TCP no delay, disabling Nagle's Algorithm\n\
  -S, --tos       #        set the socket's IP_TOS (byte) field\n\
  -Z, --tcp-congestion <algo>  set TCP congestion control algorithm (Linux only)\n\
\n\
Server specific:\n\
  -p, --port      #[-#]    server port(s) to listen on/connect to\n\
  -s, --server             run in server mode\n\
  -1, --singleclient       run one server at a time\n\
      --histograms         enable latency histograms\n\
      --jitter-histograms  enable jitter histograms\n\
      --permit-key-timeout set the timeout for a permit key in seconds\n\
      --skip-rx-copy       set MSG_TRUNC to avoid kernel to application spaced copy of data\n\
      --tcp-rx-window-clamp set the TCP receive window clamp size in bytes\n\
      --test-exchange-timeout set the timeout on the test exchange, use 0 for no timeout\n\
      --tap-dev   #[<dev>] use TAP device to receive at L2 layer\n\
  -t, --time      #        time in seconds to listen for new connections as well as to receive traffic (default not set)\n\
  -B, --bind <ip>[%<dev>]  bind to multicast address and optional device\n\
  -U, --single_udp         run in single threaded UDP mode\n\
      --sum-dstip          sum traffic threads based upon destination ip address (default is src ip)\n\
  -D, --daemon             run the server as a daemon\n"
#ifdef WIN32
"  -R, --remove             remove service in win32\n"
#endif
"  -V, --ipv6_domain        Enable IPv6 reception by setting the domain and socket to AF_INET6 (Can receive on both IPv4 and IPv6)\n"
;

const char usage_long2[] = "\
\n\
Client specific:\n\
      --bounceback         request a bounceback test (use -l for size, defaults to 100 bytes)\n\
      --bounceback-hold    request the server to insert a delay of n milliseconds between its read and write\n\
      --bounceback-no-quickack request the server not set the TCP_QUICKACK socket option (disabling TCP ACK delays) during a bounceback test\n\
      --bounceback-period  request the client schedule a send every n milliseconds\n \
      --bounceback-reply   set the bounceback reply message size (defaults to symmetric)\n \
      --bounceback-txdelay  request the bounceback server delay n seconds between the request and the reply\n \
  -c, --client    <host>   run in client mode, connecting to <host>\n\
      --connect-only       run a connect only test\n\
      --connect-retry-timer minimum time interval in seconds between application level connect retries\n\
      --connect-retry-time time interval in seconds to attempt application level connect retries \n\
      --dscp               set the DSCP field (masking ECN bits) in TOS byte\n\
  -d, --dualtest           Do a bidirectional test simultaneously (multiple sockets)\n\
      --fq-rate #[kmgKMG]  bandwidth to socket pacing\n\
      --fq-rate-step #[kmgKMG]  step value to socket pacing\n\
      --fq-rate-step-interval #[kmgKMG]  step interval (in seconds) for socket pacing\n\
      --full-duplex        run full duplex test using same socket\n\
      --histograms         enable histograms for --tcp-write-times\n\
      --ignore-shutdown    don't wait on the TCP shutdown or close (fin & finack) rather use the final write as the ending event\n\
      --incr-dstip         Increment the destination ip with parallel (-P) traffic threads\n\
      --incr-dstport       Increment the destination port with parallel (-P) traffic threads\n\
      --incr-srcip         Increment the source ip with parallel (-P) traffic threads\n\
      --incr-srcport       Increment the source port with parallel (-P) traffic threads\n\
      --ipg                set the the interpacket gap (milliseconds) for packets within an isochronous frame\n\
      --isochronous <frames-per-second>:<mean>,<stddev> send traffic in bursts (frames - emulate video traffic)\n\
      --local-only         Set don't route on socket\n\
      --near-congestion=[w] Use a weighted write delay per the sampled TCP RTT (experimental)\n\
      --no-connect-sync    No sychronization after connect when -P or parallel traffic threads\n\
      --no-udp-fin         No final server to client stats at end of UDP test\n\
  -n, --num       #[kmgKMG]    number of bytes to transmit (instead of -t)\n\
      --sync-transfer-id   pass the clients' transfer id(s) to the server so both will use the same id in their respective outputs\n\
  -r, --tradeoff           Do a fullduplexectional test individually\n\
      --tcp-quickack       set the socket's TCP_QUICKACK option (off by default)\n\
      --tcp-write-prefetch set the socket's TCP_NOTSENT_LOWAT value in bytes and use event based writes\n\
      --tcp-write-times    measure the socket write times at the application level\n\
  -t, --time      #        time in seconds to transmit for (default 10 secs)\n\
      --trip-times         enable end to end measurements (requires client and server clock sync)\n\
      --txdelay-time       time in seconds to hold back after connect and before first write\n\
      --txstart-time       unix epoch time to schedule first write and start traffic\n\
      --udp-l4s            run a UDP L4S flow\n\
      --udp-l4s-video      run a UDP L4S video flow\n\
  -B, --bind [<ip> | <ip:port>] bind ip (and optional port) from which to source traffic\n\
  -F, --fileinput <name>   input the data to be transmitted from a file\n\
  -H, --ssm-host <ip>      set the SSM source, use with -B for (S,G) \n\
  -I, --stdin              input the data to be transmitted from stdin\n\
  -L, --listenport #       port to receive fullduplexectional tests back on\n\
  -P, --parallel  #        number of parallel client threads to run\n"
#ifndef WIN32
"  -R, --reverse            reverse the test (client receives, server sends)\n"
#else
"  -R                       Remove the windows service\n"
"      --reverse            reverse the test (client receives, server sends)\n"
#endif
"  -S, --tos                IP DSCP or tos settings\n\
  -T, --ttl       #        time-to-live, for multicast (default 1)\n\
      --working-load request working load(s)\n\
      --working-load-cca set working load CCA\n\
  -V, --ipv6_domain        Set the domain to IPv6 (send packets over IPv6)\n\
  -X, --peer-detect        perform server version detection and version exchange\n\
\n\
Miscellaneous:\n\
  -x, --reportexclude [CDMSV]   exclude C(connection) D(data) M(multicast) S(settings) V(server) reports\n\
  -y, --reportstyle C      report as a Comma-Separated Values\n\
  -h, --help               print this message and quit\n\
  -v, --version            print version information and quit\n\
\n\
[kmgKMG] Indicates options that support a k,m,g,K,M or G suffix\n\
Lowercase format characters are 10^3 based and uppercase are 2^n based\n\
(e.g. 1k = 1000, 1K = 1024, 1m = 1,000,000 and 1M = 1,048,576)\n\
\n\
Accepted tos values are: af11, af12, af13, af21, af22, af23, af31, af32, af33, af41, af42, af43,\n\
cs0, cs1, cs2, cs3, cs4, cs5, cs6, cs7, ef, le, nqb, nqb2, ac_be, ac_bk, ac_vi, ac_vo, lowdelay, throughput,\n\
reliability, or a numeric value.\n\
\n\
The TCP window size option can be set by the environment variable\n\
TCP_WINDOW_SIZE. Most other options can be set by an environment variable\n\
IPERF_<long option name>, such as IPERF_BANDWIDTH.\n\
\n\
Source at <http://sourceforge.net/projects/iperf2/>\n\
Report bugs to <iperf-users@lists.sourceforge.net>\n";

// include a description of the threading in the version
#if   defined( HAVE_POSIX_THREAD )
    #define IPERF_THREADS "pthreads"
#elif defined( HAVE_WIN32_THREAD )
    #define IPERF_THREADS "win32 threads"
#else
    #define IPERF_THREADS "single threaded"
#endif

const char version[] =
"iperf version " IPERF_VERSION " (" IPERF_VERSION_DATE ") " IPERF_THREADS "\n";
const char branch_version[] =
"iperf (branch " IPERF_BRANCH ") (" IPERF_VERSION_DATE ") " IPERF_THREADS "\n";

/* -------------------------------------------------------------------
 * settings
 * ------------------------------------------------------------------- */

const char separator_line[] =
"------------------------------------------------------------\n";

const char server_port[] =
"Server listening on %s port %d\n";

const char client_port[] =
"Client connecting to %s, %s port %d\n";

const char server_pid_port[] =
"Server listening on %s port %d with pid %d\n";

const char server_working_load_port[] =
"Working load listening on %s port %d\n";

const char server_pid_portrange[] =
"Server listening on %s ports %d-%d with pid %d\n";

const char client_pid_port[] =
"Client connecting to %s, %s port %d with pid %d (%d/%d flows/load)\n";

const char client_pid_port_dev[] =
"Client connecting to %s, %s port %d with pid %d via %s (%d/%d flows/load)\n";

const char bind_address[] =
"Binding to local address %s\n";

const char bind_address_iface[] =
"Binding to local address %s and iface %s\n";

const char bind_address_iface_taptun[] =
"Using virtual iface %s (AF_PACKET & SOCK_RAW)\n";

const char multicast_ttl[] =
"Setting multicast TTL to %d\n";

const char join_multicast[] =
"Joining multicast group  %s\n";

const char join_multicast_sg[] =
"Joining multicast (S,G)=%s,%s\n";

const char join_multicast_starg_dev[] =
"Joining multicast (*,G)=*,%s w/iface %s\n";

const char join_multicast_sg_dev[] =
"Joining multicast (S,G)=%s,%s w/iface %s\n";

const char client_datagram_size[] =
"Sending %d byte datagrams, IPG target: %.2f us\n";

const char client_datagram_size_kalman[] =
"Sending %d byte datagrams, IPG target: %.2f us (kalman adjust)\n";

const char server_datagram_size[] =
"Receiving %d byte datagrams\n";

const char tcp_window_size[] =
"TCP window size";

const char udp_buffer_size[] =
"UDP buffer size";

const char window_default[] =
"(default)";

const char wait_server_threads[] =
"Waiting for server threads to complete. Interrupt again to force quit.\n";

const char client_isochronous[] =
"Isochronous: %0.2f frames/sec mean=%s/s, stddev=%s/s, Period/IPG=%0.2f/%.3f ms\n";

const char client_burstperiod[] =
"Bursting: %s every %0.3f second(s)\n";

const char client_burstperiodcount[] =
"Bursting: %s writes %d times every %0.3f second(s)\n";

const char client_bounceback[] =
"Bounceback test (req/reply size =%s/%s) (server hold req=%d usecs & tcp_quickack)\n";

const char client_bbburstperiodcount[] =
"Bursting request %d times every %0.2f second(s)\n";

const char client_bounceback_noqack[] =
"Bounceback test (req/reply size =%s/%s) (server hold req=%d usecs)\n";

const char server_burstperiod[] =
"Burst wait timeout set to (2 * %0.2f) seconds (use --burst-period=<n secs> to change)\n";

const char client_fq_pacing [] =
"fair-queue socket pacing set to %s/s\n";

const char client_fq_pacing_step [] =
"fair-queue socket pacing set to %s/s (stepping rate by %s/s)\n";

/* -------------------------------------------------------------------
 * Legacy reports
 * ------------------------------------------------------------------- */

const char report_bw_header[] =
"[ ID] Interval       Transfer     Bandwidth\n";

const char report_bw_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec%s\n";

const char report_sum_bw_format[] =
"[SUM] " IPERFTimeFrmt " sec  %ss  %ss/sec%s\n";

const char report_sumcnt_bw_format[] =
"[SUM-%d] " IPERFTimeFrmt " sec  %ss  %ss/sec%s\n";

const char report_bw_jitter_loss_header[] =
"[ ID] Interval       Transfer     Bandwidth        Jitter   Lost/Total Datagrams\n";

const char report_bw_jitter_loss_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %6.3f ms %" PRIdMAX "/%" PRIdMAX " (%.2g%%)%s\n";

const char report_sum_bw_jitter_loss_format[] =
"[SUM] " IPERFTimeFrmt " sec  %ss  %ss/sec   %" PRIdMAX "/%" PRIdMAX " (%.2g%%)%s\n";

const char report_sumcnt_bw_jitter_loss_header[] =
"[SUM-cnt] Interval     Transfer     Bandwidth       Lost/Total    PPS\n";

const char report_sumcnt_bw_jitter_loss_format[] =
"[SUM-%d] " IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX " (%.2g%%) %8.0f pps%s\n";

/* -------------------------------------------------------------------
 * Enhanced reports (per -e)
 * ------------------------------------------------------------------- */
const char report_sumcnt_bw_header[] =
"[SUM-cnt] Interval       Transfer     Bandwidth\n";

const char client_report_epoch_start_current[] =
"[%3d] Client traffic to start in %d seconds at %s current time is %s\n";

const char server_report_epoch_start_current[] =
"[%3d] Server receiving to start in %d seconds at %s\n";

const char client_write_size[] =
"Write buffer size";

const char server_read_size[] =
"Read buffer size";

const char report_bw_enhanced_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec%s\n";

const char report_sum_bw_enhanced_format[] =
"[SUM] " IPERFTimeFrmt " sec  %ss  %ss/sec%s\n";

const char report_bw_read_enhanced_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth       Reads=Dist\n";

const char report_bw_read_enhanced_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "=%d:%d:%d:%d:%d:%d:%d:%d%s\n";

const char report_sumcnt_bw_read_enhanced_header[] =
"[SUM-cnt] Interval" IPERFTimeSpace "Transfer    Bandwidth       Reads=Dist\n";

const char report_sumcnt_bw_read_enhanced_format[] =
"[SUM-%d] " IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "=%d:%d:%d:%d:%d:%d:%d:%d%s\n";

const char report_sumcnt_bw_read_triptime_header[] =
"[SUM-cnt] Interval" IPERFTimeSpace "Transfer    Bandwidth  InP  Reads=Dist\n";

const char report_sumcnt_bw_read_triptime_format[] =
"[SUM-%d] " IPERFTimeFrmt " sec  %ss  %ss/sec %s  %" PRIdMAX "=%d:%d:%d:%d:%d:%d:%d:%d%s\n";

const char report_bw_read_enhanced_netpwr_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth    Burst Latency avg/min/max/stdev (cnt/size) inP NetPwr  Reads=Dist\n";

const char report_bw_read_enhanced_netpwr_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %.3f/%.3f/%.3f/%.3f ms (%d/%d) %s %s  %" PRIdMAX "=%d:%d:%d:%d:%d:%d:%d:%d%s\n";

const char report_bw_isoch_enhanced_netpwr_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth    Burst Latency avg/min/max/stdev (cnt/size) NetPwr  Reads=Dist\n";

const char report_bw_isoch_enhanced_netpwr_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %.3f/%.3f/%.3f/%.3f ms (%d/%d) %s  %" PRIdMAX "=%d:%d:%d:%d:%d:%d:%d:%d%s\n";

const char report_sum_bw_read_enhanced_format[] =
"[SUM] " IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "=%d:%d:%d:%d:%d:%d:%d:%d%s\n";

const char report_triptime_enhanced_format[] =
"%s" IPERFTimeFrmt " trip-time (3WHS done->fin+finack) = %.4f sec%s\n";

#if HAVE_TCP_STATS
#if HAVE_TCP_INFLIGHT
const char report_bw_write_enhanced_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth       Write/Err  Rtry     InF(pkts)/Cwnd(pkts)/RTT(var)        NetPwr\n";

const char report_bw_write_enhanced_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%10" PRIdMAX " %8" PRIdMAX "K(%" PRIuLEAST32 ")/%" PRIdMAX "K(%" PRIuLEAST32 ")/%" PRIuLEAST32 "(%" PRIuLEAST32 ") us  %s\n";

const char report_write_enhanced_nocwnd_write_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX " %10" PRIdMAX "       NA/%" PRIuLEAST32 "(%" PRIuLEAST32 ") us  %s  %.3f/%.3f/%.3f/%.3f ms (%" PRIdMAX ")%s\n";

const char report_bw_write_enhanced_fq_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth       Write/Err  Rtry     InF(pkts)/Cwnd(pkts)/RTT(var)  fq-rate  NetPwr\n";

const char report_bw_write_enhanced_fq_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%10" PRIdMAX " %8" PRIdMAX "K(%" PRIuLEAST32 ")/%" PRIdMAX "K(%" PRIuLEAST32 ")/%" PRIuLEAST32 "(%" PRIuLEAST32 ") us %s/sec %s%s\n";

const char report_client_bb_bw_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth         BB cnt=avg/min/max/stdev         Rtry  Cwnd(pkts)/RTT(var)   Tx/Rx bytes  RPS(avg)\n";

const char report_client_bb_bw_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec    %" PRIdMAX "=%.3f/%.3f/%.3f/%.3f ms %4" PRIuLEAST32 "%4" PRIdMAX "K/(%" PRIuLEAST32 ")/%" PRIuLEAST32 "(%" PRIuLEAST32 ") us  Tx=%s Rx=%s  RPS=%s%s\n";

const char report_client_bb_bw_final_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec    %" PRIdMAX "=%.3f/%.3f/%.3f/%.3f ms %4" PRIuLEAST32 "   -(-)/-(-)    Tx=%s Rx=%s  RPS=%s%s\n";

const char report_write_enhanced_write_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth       Write/Err  Rtry    InF(pkts)/Cwnd(pkts)/RTT(var)   NetPwr  write-times avg/min/max/stdev (cnt)\n";

const char report_write_enhanced_write_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX " %8" PRIuLEAST32 "%7" PRIdMAX "K(%" PRIuLEAST32 ")/%" PRIdMAX "K(%" PRIuLEAST32 ")/%" PRIuLEAST32 "(%" PRIuLEAST32 ") us      %s  %.3f/%.3f/%.3f/%.3f ms (%" PRIdMAX ")%s\n";

#else // no inflight
const char report_bw_write_enhanced_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth       Write/Err  Rtry     Cwnd(pkts)/RTT(var)        NetPwr\n";

const char report_bw_write_enhanced_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%10" PRIuLEAST32 " %8" PRIdMAX "K(%" PRIuLEAST32 ")/%" PRIuLEAST32 "(%" PRIuLEAST32 ") us  %s\n";

const char report_bw_write_enhanced_fq_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth       Write/Err  Rtry     Cwnd(pkts)/RTT(var)  fq-rate  NetPwr\n";

const char report_bw_write_enhanced_fq_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%10" PRIuLEAST32 " %8" PRIdMAX "K(%" PRIuLEAST32 ")/%" PRIuLEAST32 "(%" PRIuLEAST32 ") us %s/sec %s%s\n";

const char report_client_bb_bw_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth         BB cnt=avg/min/max/stdev         Rtry  Cwnd(pkts)/RTT(var)   Tx/Rx bytes  RPS(avg)\n";

const char report_client_bb_bw_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec    %" PRIdMAX "=%.3f/%.3f/%.3f/%.3f ms %4" PRIuLEAST32 " %4" PRIdMAX "K(%" PRIuLEAST32 ")/%" PRIuLEAST32 "(%" PRIuLEAST32 ") us  Tx=%s Rx=%s  RPS=%s%s\n";

const char report_client_bb_bw_final_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec    %" PRIdMAX "=%.3f/%.3f/%.3f/%.3f ms %4" PRIuLEAST32 "   -(-)/-(-)  Tx=%s Rx=%s  RPS=%s%s\n";

const char report_write_enhanced_write_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth       Write/Err  Rtry     Cwnd(pkts)/RTT(var)        NetPwr  write-times avg/min/max/stdev (cnt)\n";

const char report_write_enhanced_write_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX " %10" PRIuLEAST32 "%8" PRIdMAX "K(%" PRIuLEAST32 ")/%" PRIuLEAST32 "(%" PRIuLEAST32 ") us  %s  %.3f/%.3f/%.3f/%.3f ms (%" PRIdMAX ")%s\n";

const char report_write_enhanced_nocwnd_write_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX " %10" PRIuLEAST32 "       NA/%" PRIuLEAST32 "(%" PRIuLEAST32 ") us  %s  %.3f/%.3f/%.3f/%.3f ms (%" PRIdMAX ")%s\n";

#endif //HAVE_TCP_INFLIGHT

const char report_sum_bw_write_enhanced_format[] =
"[SUM] " IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%10" PRIuLEAST32 " %s\n";

const char report_bw_write_enhanced_fq_final_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%10" PRIuLEAST32 " %8dK/%u(%u) us %s%s\n";

const char report_bw_write_enhanced_nocwnd_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX " %10" PRIuLEAST32 "       NA/%" PRIuLEAST32 "(%" PRIuLEAST32 ")us    %s\n";

const char report_write_enhanced_isoch_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer     Bandwidth      Write/Err  Rtry     Cwnd(pkts)/RTT(var)     isoch:tx/miss/slip  NetPwr\n";

const char report_write_enhanced_isoch_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX " %8" PRIuLEAST32 " %8" PRIdMAX "K(%" PRIuLEAST32 ")/%" PRIuLEAST32 "(%" PRIuLEAST32 ") us  %9" PRIuMAX "/%" PRIuMAX "/%" PRIuMAX "       %s%s\n";

const char report_write_enhanced_isoch_nocwnd_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX " %10" PRIuLEAST32 "    NA/%" PRIuLEAST32 "(%" PRIuLEAST32 ") us  %9" PRIuMAX "/%" PRIuMAX "/%" PRIuMAX "   %s%s\n";

const char report_sumcnt_bw_write_enhanced_header[] =
"[SUM-cnt] Interval" IPERFTimeSpace "Transfer    Bandwidth       Write/Err  Rtry\n";

const char report_sumcnt_bw_write_enhanced_format[] =
"[SUM-%d] " IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%10" PRIdMAX "%s\n";

#else //no tcpstat

const char report_client_bb_bw_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth         BB cnt=avg/min/max/stdev  Tx/Rx bytes  RPS(avg)\n";

const char report_client_bb_bw_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec    %" PRIdMAX "=%.3f/%.3f/%.3f/%.3f ms  Tx=%s Rx=%s  RPS=%s%s\n";

const char report_bw_write_enhanced_fq_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth       Write/Err fq-rate  NetPwr\n";

const char report_bw_write_enhanced_fq_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%s/sec %s%s\n";

const char report_bw_write_enhanced_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth       Write/Err\n";

const char report_bw_write_enhanced_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%s\n";

const char report_sum_bw_write_enhanced_format[] =
"[SUM] " IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%s\n";

const char report_write_enhanced_isoch_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer     Bandwidth      Write/Err  isoch:tx/miss/slip\n";

const char report_write_enhanced_isoch_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%9" PRIuMAX "/%" PRIuMAX "/%" PRIuMAX "%s\n";

const char report_write_enhanced_write_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth       Write/Err NetPwr  write-times avg/min/max/stdev (cnt)\n";

const char report_write_enhanced_write_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%s  %.3f/%.3f/%.3f/%.3f ms (%" PRIdMAX ")%s\n";

const char report_bw_write_fq_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer    Bandwidth       Write/Err  fq-rate\n";

const char report_bw_write_fq_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "    %s/sec%s\n";

const char report_sumcnt_bw_write_enhanced_header[] =
"[SUM-cnt] Interval" IPERFTimeSpace "Transfer    Bandwidth       Write/Err\n";

const char report_sumcnt_bw_write_enhanced_format[] =
"[SUM-%d] " IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%s\n";

#endif //no tcp stat

const char report_client_bb_bw_triptime_format[] =
"%s" IPERFTimeFrmt " sec  OWD (ms) Cnt=%" PRIdMAX " TX=%.3f/%.3f/%.3f/%.3f RX=%.3f/%.3f/%.3f/%.3f Asymmetry=%.3f/%.3f/%.3f/%.3f%s\n";

const char report_bw_pps_enhanced_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer     Bandwidth      Write/Err/Timeo  PPS\n";

const char report_bw_pps_enhanced_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "/%" PRIdMAX "%8.0f pps%s\n";

const char report_sumcnt_bw_pps_enhanced_header[] =
"[SUM-cnt] Interval" IPERFTimeSpace "Transfer     Bandwidth      Write/Err  PPS\n";

const char report_sumcnt_bw_pps_enhanced_format[] =
"[SUM-%d] " IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX " %8.0f pps%s\n";

const char report_sumcnt_udp_enhanced_header[] =
"[SUM-cnt] Interval" IPERFTimeSpace "Transfer     Bandwidth      Lost/Total  PPS\n";

const char report_sumcnt_udp_enhanced_format[] =
"[SUM-%d] " IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX " %8.0f pps%s\n";

const char report_sumcnt_udp_triptime_header[] =
"[SUM-cnt] Interval" IPERFTimeSpace "Transfer     Bandwidth      Lost/Total  Latency avg/min/max  Rx/inP PPS\n";

const char report_sumcnt_udp_triptime_format[] =
"[SUM-%d] " IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX " (%.2g%%)  %.3f/%.3f/%.3f  %" PRIdMAX "/%.0f  %8.0f pps%s\n";

const char report_bw_pps_enhanced_isoch_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer     Bandwidth      Write/Err  PPS  isoch:tx/miss/slip\n";

const char report_bw_pps_enhanced_isoch_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%8.0f pps  %3" PRIuMAX "/%" PRIuMAX "/%" PRIuMAX "%s\n";

const char report_sum_bw_pps_enhanced_format[] =
"[SUM] " IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX "%8.0f pps%s\n";

const char report_bw_jitter_loss_pps_header[] =
"[ ID] Interval       Transfer     Bandwidth        Jitter   Lost/Total Datagrams   PPS\n";

const char report_bw_jitter_loss_pps_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %6.3f ms %" PRIdMAX "/%" PRIdMAX " (%.2g%%) %8.0f pps%s\n";

#if HAVE_DECL_MSG_TRUNC
const char report_bw_jitter_loss_enhanced_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer     Bandwidth        Jitter   Lost/Total \
 Latency avg/min/max/stdev PPS Read/Timeo/NA NetPwr\n";
#else
const char report_bw_jitter_loss_enhanced_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer     Bandwidth        Jitter   Lost/Total \
 Latency avg/min/max/stdev PPS N/A(Read/Timeo/Trunc) NetPwr\n";
#endif
const char report_bw_jitter_loss_enhanced_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %6.3f ms %" PRIdMAX "/%" PRIdMAX " (%.2g%%) %.3f/%.3f/%.3f/%.3f ms %.0f pps %" PRIdMAX "/%" PRIdMAX "/%" PRIdMAX " %s%s\n";

const char report_bw_jitter_loss_enhanced_triptime_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer     Bandwidth        Jitter   Lost/Total \
 Latency avg/min/max/stdev PPS  Rx/inP  Read/Timeo/Trunc  NetPwr\n";

const char report_bw_jitter_loss_enhanced_triptime_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %6.3f ms %" PRIdMAX "/%" PRIdMAX " (%.2g%%) %.3f/%.3f/%.3f/%.3f ms %.0f pps %" PRIdMAX "/%s %" PRIdMAX "/%" PRIdMAX "/%" PRIdMAX " %s%s\n";
const char report_bw_jitter_loss_enhanced_isoch_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer     Bandwidth        Jitter   Lost/Total \
 Latency avg/min/max/stdev PPS  NetPwr  Isoch:rx/lost\n";

const char report_bw_jitter_loss_enhanced_isoch_triptime_header[] =
"[ ID] Interval" IPERFTimeSpace "Transfer     Bandwidth        Jitter   Lost/Total \
Latency avg/min/max/stdev  PPS  Frame:rx/lost  Frame-latency avg/min/max/stdev  NetPwr\n";

const char report_bw_jitter_loss_enhanced_isoch_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %6.3f ms %" PRIdMAX "/%" PRIdMAX " (%.2g%%) %.3f/%.3f/%.3f/%.3f ms %.0f pps  %" PRIdMAX "/%" PRIdMAX " %.3f/%.3f/%.3f/%.3f ms %s%s\n";

const char report_sum_bw_jitter_loss_enhanced_format[] =
"[SUM] " IPERFTimeFrmt " sec  %ss  %ss/sec  %6.3f ms %" PRIdMAX "/%" PRIdMAX " (%.2g%%)  %.0f pps%s\n";

const char report_sumcnt_bw_jitter_loss_enhanced_header[] =
"[SUM-cnt] Interval     Transfer     Bandwidth       Lost/Total  Latency avg/min/max  PPS\n";

const char report_sumcnt_bw_jitter_loss_enhanced_format[] =
"[SUM-%d] " IPERFTimeFrmt " sec  %ss  %ss/sec  %" PRIdMAX "/%" PRIdMAX " (%.2g%%)  %.3f/%.3f/%.3f  %.0f pps%s\n";

const char report_bw_jitter_loss_suppress_enhanced_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %6.3f ms %" PRIdMAX "/%" PRIdMAX " (%.2g%%) -/-/-/- ms %.0f pps %" PRIdMAX "/%" PRIdMAX "/%" PRIdMAX "%s\n";

const char report_bw_jitter_loss_suppress_isoch_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %6.3f ms %" PRIdMAX "/%" PRIdMAX " (%.2g%%) -/-/-/- ms %.0f pps  %" PRIdMAX "/%" PRIdMAX " -/-/-/- ms  -%s\n";

/*
 * Frame interval reports
 */
#define IPERFFTimeFrmt "%4.4f-%4.4f"
#define IPERFFTimeSpace "  "
const char report_frame_jitter_loss_enhanced_header[] =
"[ ID] Interval(f-transit)" IPERFFTimeSpace "Transfer     Bandwidth    FrameID   Jitter   Lost/Total \
 Latency avg/min/max/stdev PPS  inP NetPwr\n";

#if 0
const char report_frame_jitter_loss_enhanced_format[] =
"%s" IPERFFTimeFrmt "(%0.4f) sec %ss  %ss/sec %" PRIdMAX "   %6.3f ms %" PRIdMAX "/%" PRIdMAX " (%.2g%%) %.3f/%.3f/%.3f/%.3f ms %.0f pps %2.0f pkts %s%s\n";

const char report_frame_jitter_loss_suppress_enhanced_format[] =
"%s" IPERFTimeFrmt "(%0.4f) sec %" PRIdMAX " %ss  %ss/sec %" PRIdMAX "   %6.3f ms %" PRIdMAX "/%" PRIdMAX " (%.2g%%) -/-/-/- ms %.0f pps %s%s\n";

#endif // not yet supported in src/ReportOutput.c RJM fix later

const char report_frame_tcp_enhanced_header[] =
"[ ID] Interval(f-transit)" IPERFFTimeSpace "Transfer     Bandwidth    FrameID\n";

const char report_burst_read_tcp_header[] =
"[ ID] Burst (start-end)" IPERFFTimeSpace "Transfer     Bandwidth       XferTime  (DC%)     Reads=Dist          NetPwr\n";

const char report_burst_read_tcp_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %6.3f ms (%.2g%%)    %" PRIdMAX "=%d:%d:%d:%d:%d:%d:%d:%d %s%s\n";

const char report_burst_read_tcp_final_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %.3f/%.3f/%.3f/%.3f ms  %" PRIdMAX "=%d:%d:%d:%d:%d:%d:%d:%d\n";
#if HAVE_TCP_STATS
const char report_burst_write_tcp_header[] =
"[ ID] Burst (start-end)" IPERFFTimeSpace "Transfer     Bandwidth       XferTime  Write/Err  Rtry  Cwnd/RTT   NetPwr\n";

const char report_burst_write_tcp_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %6.3f ms %" PRIdMAX "/%" PRIdMAX "%10" PRIdMAX "%8dK/%u %s%s\n";

#else
const char report_burst_write_tcp_header[] =
"[ ID] Burst (start-end)" IPERFFTimeSpace "Transfer     Bandwidth       XferTime  Write/Err\n";

const char report_burst_write_tcp_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %6.3f ms %" PRIdMAX "/%" PRIdMAX"%s\n";
#endif

const char report_burst_write_tcp_nocwnd_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec  %6.3f ms %" PRIdMAX "/%" PRIdMAX "%10" PRIdMAX "NA/%"PRIuLEAST32 "(%" PRIuLEAST32 ")%s\n";

const char report_burst_write_tcp_final_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec               %" PRIdMAX "=%d:%d:%d:%d:%d:%d:%d:%d%s\n";

/* -------------------------------------------------------------------
 * Fullduplex reports
 * ------------------------------------------------------------------- */
const char report_bw_sum_fullduplex_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec%s\n";

const char report_bw_sum_fullduplex_enhanced_format[] =
"[FD%d] " IPERFTimeFrmt " sec  %ss  %ss/sec%s\n";

const char report_udp_fullduplex_header[] =
"[ ID] Interval       Transfer     Bandwidth    Datagrams   PPS\n";

const char report_udp_fullduplex_format[] =
"%s" IPERFTimeFrmt " sec  %ss  %ss/sec %" PRIdMAX "%8.0f pps%s\n";

const char report_udp_fullduplex_enhanced_format[] =
"[FD%d] " IPERFTimeFrmt " sec  %ss  %ss/sec %" PRIdMAX "%8.0f pps%s\n";

const char report_udp_fullduplex_sum_format[] =
"[SUM] " IPERFTimeFrmt " sec  %ss  %ss/sec %" PRIdMAX "%8.0f pps%s\n";

/* -------------------------------------------------------------------
 * Misc reports
 * ------------------------------------------------------------------- */
const char report_outoforder[] =
"%s" IPERFTimeFrmt " sec  %" PRIdMAX " datagrams received out-of-order%s\n";

const char report_sumcnt_outoforder[] =
"[SUM-%d] " IPERFTimeFrmt " sec  %" PRIdMAX " datagrams  received out-of-order%s\n";

const char report_l2statistics[] =
"%s" IPERFTimeFrmt " sec   L2 processing detected errors, total(length/checksum/unknown) = %" PRIdMAX "(%" PRIdMAX "/%" PRIdMAX "/%" PRIdMAX ")\n";

const char report_sum_outoforder[] =
"[SUM] " IPERFTimeFrmt " sec  %" PRIdMAX " datagrams received out-of-order%s\n";

const char report_peer [] =
"%slocal %s port %u connected with %s port %u%s\n";

const char report_peer_dev [] =
"%slocal %s%%%s port %u connected with %s port %u%s\n";

const char report_peer_fail [] =
"[drop] local %s port %u connection with %s port %u%s (permit key fail)\n";

const char report_mss_unsupported[] =
"%sMSS and MTU size unknown (TCP_MAXSEG not supported)\n";

const char report_default_mss[] =
"MSS size %d bytes\n";

const char report_mss[] =
"MSS req size %d bytes (per TCP_MAXSEG)\n";

const char report_datagrams[] =
"[%3d] Sent %" PRIdMAX " datagrams\n";

const char report_sumcnt_datagrams[] =
"[SUM-%d] Sent %" PRIdMAX " datagrams\n";

const char report_sum_datagrams[] =
"[SUM] Sent %" PRIdMAX " datagrams\n";

const char server_reporting[] =
"[%3d] Server Report:\n";

const char reportCSV_peer[] =
"%s,%u,%s,%u";

const char report_l2length_error[] =
"%s" IPERFTimeFrmt " sec  %" PRIdMAX " datagrams received out-of-order\n";

const char report_client_bb_triptime_clocksync_error[] ="%s" IPERFTimeFrmt " sec  Clock sync error count = %" PRIdMAX "\n";

/*-------------------------------------------------------------------
 * CSV outputs
 *------------------------------------------------------------------*/
const char reportCSV_bw_format[] =
"%s,%s,%d,%.1f-%.1f,%" PRIdMAX ",%" PRIdMAX "\n";

const char reportCSV_bw_tcp_read_enhanced_header[] =
"time,srcaddress,srcport,dstaddr,dstport,transferid,istart,iend,bytes,speed,readcnt,bin0,bin1,bin2,bin3,bin4,bin5,bin6,bin7\n";

const char reportCSV_bw_tcp_read_enhanced_format[] =
"%s,%s,%d,%.1f,%.1f,%" PRIdMAX ",%" PRIdMAX ",%" PRIdMAX ",%d,%d,%d,%d,%d,%d,%d,%d\n";

const char reportCSV_bw_tcp_read_triptime_header[] =
"time,srcaddress,srcport,dstaddr,dstport,transferid,istart,iend,bytes,speed,ttavg,ttmin,ttmax,ttsdev,ttcnt,ttsize,readcnt,bin0,bin1,bin2,bin3,bin4,bin5,bin6,bin7\n";

const char reportCSV_bw_tcp_read_triptime_format[] =
"%s,%s,%d,%.1f,%.1f,%" PRIdMAX ",%" PRIdMAX ",%.3f,%.3f,%.3f,%.3f,%d,%d,%" PRIdMAX ",%d,%d,%d,%d,%d,%d,%d,%d\n";

const char reportCSV_bw_tcp_read_isochronous_header[] =
"time,srcaddress,srcport,dstaddr,dstport,transferid,istart,iend,bytes,speed,itavg,itmin,itmax,itsdev,itcnt,itsize,readcnt,bin0,bin1,bin2,bin3,bin4,bin5,bin6,bin7\n";

const char reportCSV_bw_tcp_write_enhanced_header[] =
"time,srcaddress,srcport,dstaddr,dstport,transferid,istart,iend,bytes,speed,writecnt,writeerr,tcpretry,tcpcwnd,tcppcwnd,tcprtt,tcprttvar\n";

const char reportCSV_bw_tcp_write_enhanced_format[] =
"%s,%s,%d,%.1f,%.1f,%" PRIdMAX ",%" PRIdMAX ",%" PRIdMAX ",%" PRIdMAX ",%" PRIuLEAST32 ",%" PRIdMAX ",%" PRIuLEAST32 ",%" PRIuLEAST32 ",%" PRIuLEAST32 "\n";

const char reportCSV_bw_jitter_loss_format[] =
"%s,%s,%d,%.1f-%.1f,%" PRIdMAX ",%" PRIdMAX ",%.3f,%" PRIdMAX ",%" PRIdMAX ",%.3f,%" PRIdMAX "\n";

const char reportCSV_bw_udp_read_enhanced_header[] =
"time,srcaddress,srcport,dstaddr,dstport,transferid,istart,iend,bytes,speed,jitter,errors,datagrams,errpercent,outoforder,readcnt,readtimeo,readerrlen,pps\n";

const char reportCSV_bw_udp_read_enhanced_format[] =
"%s,%s,%d,%.1f,%.1f,%" PRIdMAX ",%" PRIdMAX ",%.3f,%" PRIdMAX ",%" PRIdMAX ",%.3f,%" PRIdMAX ",%" PRIdMAX ",%" PRIdMAX ",%" PRIdMAX ",%f\n";

const char reportCSV_bw_udp_read_triptime_header[] =
"time,srcaddress,srcport,dstaddr,dstport,transferid,istart,iend,bytes,speed,jitter,errors,datagrams,errpercent,outoforder,ttavg,ttmin,ttmax,ttsdev,ttcnt,ttsize,readcnt,readtimeo,readerrlen,pps\n";

const char reportCSV_bw_udp_read_triptime_format[] =
"%s,%s,%d,%.1f,%.1f,%" PRIdMAX ",%" PRIdMAX ",%.3f,%" PRIdMAX ",%" PRIdMAX ",%.3f,%" PRIdMAX ",%.3f,%.3f,%.3f,%.3f,%d,%d,%" PRIdMAX ",%" PRIdMAX ",%" PRIdMAX ",%f\n";

const char reportCSV_bw_udp_read_isochronous_header[] =
"time,srcaddress,srcport,dstaddr,dstport,transferid,istart,iend,bytes,speed,jitter,errors,datagrams,errpercent,outoforder,ttavg,ttmin,ttmax,ttsdev,itavg,itmin,itmax,itsdev,readcnt,readtimeo,readerrlen,pps\n";

const char reportCSV_bw_udp_read_isochronous_format[] =
"%s,%s,%d,%.1f,%.1f,%" PRIdMAX ",%" PRIdMAX ",%.3f,%" PRIdMAX ",%" PRIdMAX ",%.3f,%" PRIdMAX ",%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%" PRIdMAX ",%" PRIdMAX ",%" PRIdMAX ",%f\n";

const char reportCSV_bw_udp_write_enhanced_header[] =
"time,srcaddress,srcport,dstaddr,dstport,transferid,istart,iend,bytes,speed,jitter,errors,datagrams,errpercent,outoforder,writecnt,writeerr,writetimeo,pps\n";

const char reportCSV_bw_udp_write_enhanced_format[] =
"%s,%s,%d,%.1f,%.1f,%" PRIdMAX ",%" PRIdMAX ",%.3f,%" PRIdMAX ",%" PRIdMAX ",%.3f,%" PRIdMAX ",%" PRIdMAX ",%" PRIdMAX ",%" PRIdMAX ",%f\n";

const char reportCSV_client_bb_bw_tcp_header[] =
"time,srcaddress,srcport,dstaddr,dstport,transferid,istart,iend,bytes,speed,bbcnt,bbavg,bbmin,bbmax,bbstdev,tcpretry,tcpcwnd,tcppcwnd,tcprtt,tcprttvar,txbytes,rxbytes,rps\n";

const char reportCSV_client_bb_bw_tcp_format[] =
"%s,%s,%d,%.1f,%.1f,%" PRIdMAX ",%" PRIdMAX ",%" PRIdMAX ",%.3f,%.3f,%.3f,%.3f,%" PRIuLEAST32 ",%" PRIdMAX ",%" PRIuLEAST32 ",%" PRIuLEAST32 ",%" PRIuLEAST32 ",%" PRIdMAX ",%" PRIdMAX ",%0.1f\n";

/* -------------------------------------------------------------------
 * warnings
 * ------------------------------------------------------------------- */

const char warn_window_requested[] =
" (WARNING: requested %s)";

const char warn_window_small[] = "\
WARNING: TCP window size set to %d bytes. A small window size\n\
will give poor performance. See the Iperf documentation.\n";

const char warn_delay_large[] =
"WARNING: delay too large, reducing from %.1f to 1.0 seconds.\n";

const char warn_no_pathmtu[] =
"WARNING: Path MTU Discovery may not be enabled.\n";

const char warn_no_ack[]=
"[%3d] WARNING: did not receive ack of last datagram after %d tries.\n";

const char warn_ack_failed[]=
"[%3d] WARNING: ack of last datagram failed.\n";

const char warn_fileopen_failed[]=
"WARNING: Unable to open file stream for transfer\n\
Using default data stream. \n";

const char unable_to_change_win[]=
"WARNING: Unable to change the window size\n";

const char opt_estimate[]=
"Optimal Estimate\n";

#ifdef HAVE_FASTSAMPLING
const char report_interval_small[] =
"WARNING: interval too small, increasing to %3.4f milliseconds.\n";
#else
const char report_interval_small[] =
"WARNING: interval too small, increasing to %3.3f milliseconds.\n";
#endif
const char warn_invalid_server_option[] =
"WARNING: option -%c is not valid for server mode\n";

const char warn_invalid_client_option[] =
"WARNING: option -%c is not valid for client mode\n";

const char warn_invalid_compatibility_option[] =
"WARNING: option -%c is not valid in compatibility mode\n";

const char warn_implied_udp[] =
"WARNING: option -%c implies udp testing\n";

const char warn_implied_compatibility[] =
"WARNING: option -%c has implied compatibility mode\n";

const char warn_buffer_too_small[] =
"WARNING: %s socket buffer size (-l value) increased to %d bytes for proper operation\n";

const char warn_invalid_single_threaded[] =
"WARNING: option -%c is not valid in single threaded versions\n";

const char warn_invalid_report_style[] =
"WARNING: unknown reporting style \"%s\", switching to default\n";

const char warn_invalid_report[] =
"WARNING: unknown reporting type \"%c\", ignored\n valid options are:\n\t exclude: C(connection) D(data) M(multicast) S(settings) V(server) report\n\n";

const char warn_server_old[] =
"WARNING: iperf server version is likely very old\n";

const char warn_test_exchange_failed[] =
"WARNING: client/server version exchange failed\n";

const char warn_len_too_small_peer_exchange[] =
"WARNING: %s option -l value of %d is too small for peer exchange (suggested min value is %d bytes)\n";

const char warn_compat_and_peer_exchange[] =
"WARNING: Options of '-C' '--compatibility' AND '-X' '--peerdetect' are mutually exclusive, --peerdetect ignored\n";

const char warn_start_before_now[] =
"[%3d] WARNING: --txstart-time (%ld.%06ld) %s is before now %s\n";

const char error_starttime_exceeds[] =
"ERROR: --txstart-time %ld.%06ld %s exceeds max scheduled delay of %d secs\n";

const char error_delaytime_exceeds[] =
"ERROR: --txdelay-time of %d seconds is more than the supported delay of %d seconds\n";

const char report_omitted[] = "  (omitted)";

#ifdef __cplusplus
} /* end extern "C" */
#endif
