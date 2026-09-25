/*---------------------------------------------------------------
 * Copyright (c) 1999,2000,2001,2002,2003 The Board of Trustees of the
 * University of Illinois All Rights Reserved.
 * ---------------------------------------------------------------
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
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT
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
 * Listener.cpp
 * by Mark Gates <mgates@nlanr.net>
 * &  Ajay Tirumala <tirumala@ncsa.uiuc.edu>
 * rewritten by Robert McMahon
 * -------------------------------------------------------------------
 * Listener sets up a socket listening on the server host. For each
 * connected socket that accept() returns, this creates a Server
 * socket and spawns a thread for it.
 *
 * Changes to the latest version. Listener will run as a daemon
 * Multicast Server is now Multi-threaded
 * -------------------------------------------------------------------
 * headers
 * uses
 *   <stdlib.h>
 *   <stdio.h>
 *   <string.h>
 *   <errno.h>
 *
 *   <sys/types.h>
 *   <unistd.h>
 *
 *   <netdb.h>
 *   <netinet/in.h>
 *   <sys/socket.h>
 * ------------------------------------------------------------------- */
#define HEADERS()

#include "headers.h"
#include "Listener.hpp"
#include "SocketAddr.h"
#include "PerfSocket.hpp"
#include "active_hosts.h"
#include "util.h"
#include "version.h"
#include "Locale.h"
#include "SocketAddr.h"
#include "payloads.h"
#include "delay.h"

/* -------------------------------------------------------------------

 * Stores local hostname and socket info.
 * ------------------------------------------------------------------- */

Listener::Listener (thread_Settings *inSettings) {
    mClients = inSettings->mThreads;
    ListenSocket = INVALID_SOCKET;
    /*
     * These thread settings are stored in three places
     *
     * 1) Listener thread
     * 2) Reporter Thread (per the ReportSettings())
     * 3) Server thread
     */
    mSettings = inSettings;
} // end Listener

/* -------------------------------------------------------------------
 * Delete memory (buffer).
 * ------------------------------------------------------------------- */
Listener::~Listener () {
#if HAVE_THREAD_DEBUG
    thread_debug("Listener destructor close sock=%d", ListenSocket);
#endif
    if (ListenSocket != INVALID_SOCKET) {
        int rc = close(ListenSocket);
        WARN_errno(rc == SOCKET_ERROR, "listener close");
    }
} // end ~Listener

/* -------------------------------------------------------------------
 * This is the main Listener thread loop, listens and accept new
 * connections and starts traffic threads
 *
 * Flow is
 * o) suspend on traffic done for single client case
 * o) hang a select() then accept() on the listener socket
 * o) read or, more accurately, peak the socket for initial messages
 * o) determine and set server's settings flags
 * o) instantiate new settings for listener's clients if needed
 * o) instantiate and bind sum and bidir report objects as needed
 * o) start the threads needed
 *
 * ------------------------------------------------------------------- */
void Listener::Run () {
    // mCount is set True if -P was passed to the server
    int mCount = ((mSettings->mThreads != 0) ?  mSettings->mThreads : -1);

    // This is a listener launched by the client per -r or -d
    if (mSettings->clientListener) {
        SockAddr_remoteAddr(mSettings);
    }
    if (!isUDP(mSettings)) {
        // TCP needs just one listen
        if (!my_listen()) // This will set ListenSocket to a new sock fd
            return;
    }
    bool mMode_Time = isServerModeTime(mSettings) && !isDaemon(mSettings);
    if (mMode_Time) {
        mEndTime.setnow();
        mEndTime.add(mSettings->mAmount / 100.0);
    } else if (isPermitKey(mSettings) && (mSettings->mListenerTimeout > 0)) {
        mEndTime.setnow();
        mEndTime.add(mSettings->mListenerTimeout);
    }
    Timestamp now;
    bool need_listen = true;
#define SINGLECLIENTDELAY_DURATION 50000 // units is microseconds

    while (!sInterupted && mCount) {
#ifdef HAVE_THREAD_DEBUG
        thread_debug("Listener main loop port %d ", mSettings->mPort);
#endif
        now.setnow();
        if ((mMode_Time || (mSettings->mListenerTimeout > 0)) && mEndTime.before(now)) {
#ifdef HAVE_THREAD_DEBUG
            thread_debug("Listener port %d (loop timer expired)", mSettings->mPort);
#endif
            break;
        }
        // Serialize in the event the -1 option or --singleclient is set
        int tc = 0;
        if ((isSingleClient(mSettings) || (isUDP(mSettings) && isMulticast(mSettings))) && \
            mCount && (tc = (thread_numtrafficthreads()) > 0)) {
            // Start with a delay in the event some traffic
            // threads are pending to be scheduled and haven't
            // had a chance to update the traffic thread count.
            // An event system between listener thread and traffic threads
            // might better but also more complex. This delay
            // really should be good enough unless the os scheduler sucks
            delay_loop(SINGLECLIENTDELAY_DURATION);
#ifdef HAVE_THREAD_DEBUG
            thread_debug("Listener single client loop mc/t/mcast/sc %d/%d/%d/%d",mCount, tc, isMulticast(mSettings), isSingleClient(mSettings));
#endif
            continue;
        }
        // This will set ListenSocket to a new sock fd
        if (isUDP(mSettings) && need_listen) {
            // UDP needs a new listen per every new socket
            if (!my_listen()) {
                break;
            } else {
                need_listen = false;
            }
        }
        // Use a select() with a timeout if -t is set or if this is a v1 -r or -d test
        fd_set set;
        if (((mMode_Time || isCompat(mSettings)) && (!isPermitKey(mSettings) && (mSettings->mAmount != 0))) \
            || (isPermitKey(mSettings) && (mSettings->mListenerTimeout != 0))) {
            // Hang a select w/timeout on the listener socket
            struct timeval timeout;
            if (!isPermitKey(mSettings)) {
                timeout.tv_sec = mSettings->mAmount / 100;
                timeout.tv_usec = (mSettings->mAmount % 100) * 10000;
            } else {
                timeout.tv_sec = static_cast<long>(mSettings->mListenerTimeout);
                timeout.tv_usec = (static_cast<long>(mSettings->mListenerTimeout) * 1000000) % 1000000;
            }
            FD_ZERO(&set);
            FD_SET(ListenSocket, &set);
            if (!(select(ListenSocket + 1, &set, NULL, NULL, &timeout) > 0)) {
#ifdef HAVE_THREAD_DEBUG
                thread_debug("Listener select timeout");
#endif
                if (isCompat(mSettings)) {
                    fprintf(stderr, "ERROR: expected reverse connect did not occur\n");
                    break;
                } else
                    continue;
            }
        }
        if (!setsock_blocking(mSettings->mSock, true)) {
            WARN(1, "Failed setting socket to blocking mode");
        }
        // Instantiate another settings object to be used by the server thread
        Settings_Copy(mSettings, &server, DEEP_COPY);
        FAIL(!server, "Failed memory allocation for server settings", mSettings);
        server->mThreadMode = kMode_Server;
        if (!isDataReport(mSettings))
            setNoDataReport(server);

        // accept a new socket and assign it to the server thread
        int accept_sock = my_accept(server);
        if (!(accept_sock > 0)) {
            assert(server != mSettings);
#ifdef HAVE_THREAD_DEBUG
            thread_debug("Listener thread accept fail %d", accept_sock);
#endif
            Settings_Destroy(server);
            continue;
        }
        need_listen = true;
#ifdef HAVE_THREAD_DEBUG
        thread_debug("Listener thread accepted server sock=%d transferID", server->mSock, server->mTransferID);
#endif
        // Decrement the -P counter, commonly usd to kill the listener
        // after one test, i.e. -s -P 1
        if (mCount > 0) {
            mCount--;
        }
        // These are some exception cases where the accepted socket shouldn't have been
        // accepted but the accept() was first required to figure this out
        //
        // 1) When a client started the listener per -d or -r (but not --reverse.)
        //    What's done here is to see if the server peer opening the
        //    socket matches the expected peer per a compare of the ip addresses
        //    For the case of a *client Listener* the server and  host must match
        //    Note: it's a good idea to prefer --reverse and full duplex socket vs this
        //    -d,-r legacy approach. Still support it though in the name of legacy usage
        //
        // 2) The peer is using a V6 address but the listener/server didn't get -V (for v6) on
        //    it's command line
        //
        if ((mSettings->clientListener && SockAddr_Hostare_Equal(&mSettings->peer, &server->peer)) || \
            (!isIPV6(mSettings) && SockAddr_isIPv6(&server->peer))) {
            // Not allowed, reset things and restart the loop
            // Don't forget to delete the UDP entry (inserted in my_accept)
            Iperf_remove_host(server);
            if (!isUDP(server))
                close(server->mSock);
            assert(server != mSettings);
            Settings_Destroy(server);
            continue;
        }
        // isCompat is a version 1.7 test, basically it indicates there is nothing
        // in the first messages so don't try to process them. Later iperf versions use
        // the first message to convey test request and test settings information.  This flag
        // is also used for threads that are children so-to-speak, e.g. a -d or -r client,
        // which cannot have test flags otherwise there would be "test setup recursion"
        // Time to read the very first packet received (per UDP) or the test flags (TCP)
        // to get the client's requested test information.
        // Note 1: It's important to know that this will also populate mBuf with
        // enough information for the listener to perform test info exchange later in the code
        // Note 2: The mBuf read is a peek so the server's traffic thread started later
        // will also process the first message from an accounting perspective.
        // This is required for accurate traffic statistics
        if (!isCompat(server) && (isConnectOnly(server) || !apply_client_settings(server))) {
            if (isConnectionReport(server) && !isSumOnly(server)) {
                struct ReportHeader *reporthdr = InitConnectionReport(server);
                struct ConnectionInfo *cr = static_cast<struct ConnectionInfo *>(reporthdr->this_report);
                cr->connect_timestamp.tv_sec = server->accept_time.tv_sec;
                cr->connect_timestamp.tv_usec = server->accept_time.tv_usec;
                assert(reporthdr);
                PostReport(reporthdr);
            }
            Iperf_remove_host(server);
            close(server->mSock);
            assert(server != mSettings);
            Settings_Destroy(server);
            continue;
        }
        // server settings flags should now be set per the client's first message exchange
        // so the server setting's flags per the client can now be checked
        if (isUDP(server)){
            if (!isCompat(mSettings) && !isTapDev(mSettings) && (isL2LengthCheck(mSettings) || isL2LengthCheck(server)) && !L2_setup(server, server->mSock)) {
                // Requested L2 testing but L2 setup failed
                Iperf_remove_host(server);
                assert(server != mSettings);
                Settings_Destroy(server);
                continue;
            }
        }
        setTransferID(server, NORMAL);
        if ((mSettings->mReportMode == kReport_CSV) && server->mSumReport && !server->mSumReport->sum_reverse_set) {
            format_ips_port_string(&server->mSumReport->info, 1);
            server->mSumReport->sum_reverse_set = true;
        } else if (isTripTime(server) && server->mSumReport) {
            SetSumHandlers(server, server->mSumReport); // this needs to be done again after first read
        }

        // Read any more test settings and test values (not just the flags) and instantiate
        // any settings objects for client threads (e.g. bidir or full duplex)
        // This will set the listener_client_settings to NULL if
        // there is no need for the Listener to start a client
        //
        // Note: the packet payload pointer for this information has different
        // offsets per TCP or UDP. Basically, TCP starts at byte 0 but UDP
        // has to skip over the UDP seq no, etc.
        //
        if (!isCompat(server) && !isCompat(mSettings) && \
            (isFullDuplex(server) || isServerReverse(server) || (server->mMode != kTest_Normal))) {
            thread_Settings *listener_client_settings = NULL;
            // read client header for reverse settings
            Settings_GenerateClientSettings(server, &listener_client_settings, server->mBuf);
            if (listener_client_settings) {
                if (server->mMode != kTest_Normal)
                    listener_client_settings->mTransferID = 0;
                setTransferID(listener_client_settings, REVERSED);
                if (isFullDuplex(listener_client_settings) || isReverse(listener_client_settings))
                    Iperf_push_host(listener_client_settings);
                if (isFullDuplex(server)) {
                    assert(server->mSumReport != NULL);
                    if (!server->mSumReport->sum_fd_set) {
                        // Reset the sum output routine for the server sum report
                        // now that it's know to be full duplex. This wasn't known
                        // during accept()
                        SetSumHandlers(server, server->mSumReport);
                        server->mSumReport->sum_fd_set = true;
                    }
                    server->mFullDuplexReport = InitSumReport(server, server->mSock, true);
                    listener_client_settings->mFullDuplexReport = server->mFullDuplexReport;
#if HAVE_THREAD_DEBUG
                    thread_debug("FullDuplex report client=%p/%p server=%p/%p", (void *) listener_client_settings, (void *) listener_client_settings->mFullDuplexReport, (void *) server, (void *) server->mFullDuplexReport);
#endif
                    server->runNow =  listener_client_settings;
                } else if (server->mMode != kTest_Normal) {
#if HAVE_THREAD_DEBUG
                    thread_debug("V1 test (-d or -r) sum report client=%p/%p server=%p/%p", (void *) listener_client_settings, (void *) listener_client_settings->mFullDuplexReport, (void *) server, (void *) server->mFullDuplexReport);
#endif
                    if (listener_client_settings->mMode == kTest_DualTest) {
#ifdef HAVE_THREAD
                        server->runNow =  listener_client_settings;
#else
                        server->runNext = listener_client_settings;
#endif
                    } else {
                        server->runNext =  listener_client_settings;
                    }
                }
            }
        }
        setTransferID(server, NORMAL);
        if (isConnectionReport(server) && !isSumOnly(server)) {
            struct ReportHeader *reporthdr = InitConnectionReport(server);
            struct ConnectionInfo *cr = static_cast<struct ConnectionInfo *>(reporthdr->this_report);
            cr->connect_timestamp.tv_sec = server->accept_time.tv_sec;
            cr->connect_timestamp.tv_usec = server->accept_time.tv_usec;
            assert(reporthdr);
            PostReport(reporthdr);
        }
        // Now start the server side traffic threads
        if (isUDP(server)) {
            Condition_Initialize(&server->receiving);
        }
        thread_start_all(server);
        if (isUDP(server) && (mCount != 0)) {
            Condition_TimedWait(&server->receiving, 1);
        }
    }
#ifdef HAVE_THREAD_DEBUG
    thread_debug("Listener exiting port/sig/threads %d/%d/%d", mSettings->mPort, sInterupted, mCount);
#endif
} // end Run

/* -------------------------------------------------------------------
 * Setup a socket listening on a port.
 * For TCP, this calls bind() and listen().
 * For UDP, this just calls bind().
 * If inLocalhost is not null, bind to that address rather than the
 * wildcard server address, specifying what incoming interface to
 * accept connections on.
 * ------------------------------------------------------------------- */
bool Listener::my_listen () {
    int rc;
    int type;
    int domain;
    SockAddr_localAddr(mSettings);

#if (((HAVE_TUNTAP_TUN) || (HAVE_TUNTAP_TAP)) && (AF_PACKET))
    if (isTapDev(mSettings)) {
        ListenSocket = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ALL));
        FAIL_errno(ListenSocket == SOCKET_ERROR, "tuntap socket()", mSettings);
        mSettings->mSock = ListenSocket;
        rc = SockAddr_v4_Accept_BPF(ListenSocket, mSettings->mPort);
        WARN_errno((rc == SOCKET_ERROR), "tap accept bpf");
        SetSocketOptions(mSettings);
    } else
#endif
        {
            // create an AF_INET socket for the accepts
            // for the case of L2 testing and UDP, a new AF_PACKET
            // will be created to supercede this one
            type = (isUDP(mSettings)  ?  SOCK_DGRAM  :  SOCK_STREAM);
            domain = SockAddr_getAFdomain(&mSettings->local);

#ifdef WIN32
            if (SockAddr_isMulticast(&mSettings->multicast_group)) {
                // Multicast on Win32 requires special handling
                ListenSocket = WSASocket(domain, type, 0, 0, 0, WSA_FLAG_MULTIPOINT_C_LEAF | WSA_FLAG_MULTIPOINT_D_LEAF);
                WARN_errno(ListenSocket == INVALID_SOCKET, "socket");
            } else
#endif
                {
                    ListenSocket = socket(domain, type, 0);
                    WARN_errno(ListenSocket == INVALID_SOCKET, "socket");
                }
            mSettings->mSock = ListenSocket;
            SetSocketOptions(mSettings);
            SetSocketBindToDeviceIfNeeded(mSettings);
            rc = SOCKET_ERROR;
            if (isUDP(mSettings) && SockAddr_isMulticast(&mSettings->multicast_group)) {
#if HAVE_MULTICAST
#ifdef WIN32
                // Multicast on Win32 requires special handling
                rc = WSAJoinLeaf(ListenSocket, reinterpret_cast<sockaddr*> (&mSettings->local), mSettings->size_local,0,0,0,0,JL_BOTH);
                WARN_errno(rc == SOCKET_ERROR, "WSAJoinLeaf (aka bind)");
#else
#if 0 && (HAVE_DECL_IP_ADD_MEMBERSHIP || HAVE_DECL_MCAST_JOIN_GROUP) // possible future, bind to all including unicast
                iperf_sockaddr tmp;
                memcpy(&tmp, &mSettings->local, sizeof(tmp));
                SockAddr_setAddressAny(&tmp); // the multicast join will take care of this
                rc = bind(ListenSocket, reinterpret_cast<sockaddr*>(&tmp), mSettings->size_local);
                printf("***** any bind\n");
#else
                rc = bind(ListenSocket, reinterpret_cast<sockaddr*> (&mSettings->local), mSettings->size_local);
                //	    printf("***** single bind\n");
#endif
                FAIL_errno(rc == SOCKET_ERROR, "listener bind", mSettings);
                // if UDP and multicast, join the group
                if (iperf_multicast_join(mSettings) != IPERF_MULTICAST_JOIN_SUCCESS) {
                    rc = SOCKET_ERROR;
                }
#endif
#else
                fprintf(stderr, "Multicast not supported");
#endif // HAVE_MULTICAST
            } else {
                // bind socket for unicast
                rc = bind(ListenSocket, reinterpret_cast<sockaddr*>(&mSettings->local), mSettings->size_local);
            }
            FAIL_errno(rc == SOCKET_ERROR, "listener bind", mSettings);
        }
    // update the reporter thread
    if (isSettingsReport(mSettings)) {
        struct ReportHeader *tmp = InitSettingsReport(mSettings);
        setNoSettReport(mSettings);
        assert(tmp!=NULL);
        PostReport(tmp);
    }

    // listen for connections (TCP only).
    // use large (INT_MAX) backlog allowing multiple simultaneous connections
    if (!isUDP(mSettings)) {
        if (isSingleClient(mSettings)) {
            rc = listen(ListenSocket, 0 + mSettings->mThreads);
        } else {
            rc = listen(ListenSocket, INT_MAX);
        }
        WARN_errno(rc == SOCKET_ERROR, "listen");
    }
    return true;
} // end my_listen()


bool Listener::L2_setup (thread_Settings *server, int sockfd) {
#if defined(HAVE_LINUX_FILTER_H) && defined(HAVE_AF_PACKET)
    //
    //  Supporting parallel L2 UDP threads is a bit tricky.  Below are some notes as to why and the approach used.
    //
    //  The primary issues for UDP are:
    //
    //  1) We want the listener thread to hand off the flow to a server thread and not be burdened by that flow
    //  2) For -P support, the listener thread neads to detect new flows which will share the same UDP port
    //     and UDP is stateless
    //
    //  The listener thread needs to detect new traffic flows and hand them to a new server thread, and then
    //  rehang a listen/accept.  For standard iperf the "flow routing" is done using connect() per the ip quintuple.
    //  The OS will then route established connected flows to the socket descriptor handled by a server thread and won't
    //  burden the listener thread with these packets.
    //
    //  For L2 verification, we have to create a two sockets that will exist for the life of the flow.  A
    //  new packet socket (AF_PACKET) will receive L2 frames and bypasses
    //  the OS network stack.  The original AF_INET socket will still send up packets
    //  to the network stack.
    //
    //  When using packet sockets there is inherent packet duplication, the hand off to a server
    //  thread is not so straight forward as packets will continue being sent up to the listener thread
    //  (technical problem is that packet sockets do not support connect() which binds the IP quintuple as the
    //  forwarding key) Since the Listener uses recvfrom(), there is no OS mechanism to detect new flows nor
    //  to drop packets.  The listener can't listen on quintuple based connected flows because the client's source
    //  port is unknown.  Therefore the Listener thread will continue to receive packets from all established
    //  flows sharing the same dst port which will impact CPU utilization and hence performance.
    //
    //  The technique used to address this is to open an AF_PACKET socket and leave the AF_INET socket open.
    //  (This also aligns with BSD based systems)  The original AF_INET socket will remain in the (connected)
    //  state so the network stack has it's connected state.  A cBPF is then used to cause the kernel to fast drop
    //  those packets.  A cBPF is set up to drop such packets.  The test traffic will then only come over the
    //  packet (raw) socket and not the  AF_INET socket. If we were to try to close the original AF_INET socket
    //  (vs leave it open w/the fast drop cBPF) then the existing traffic will be sent up by the network stack
    //  to he Listener thread, flooding it with packets, again something we want to avoid.
    //
    //  On the packet (raw) socket itself, we do two more things to better handle performance
    //
    //  1)  Use a full quintuple cBPF allowing the kernel to filter packets (allow) per the quintuple
    //  2)  Use the packet fanout option to assign a CBPF to a socket and hence to a single server thread minimizing
    //      duplication (reduce all cBPF's filtering load)
    //
    struct sockaddr *p = reinterpret_cast<sockaddr *>(&server->peer);
    struct sockaddr *l = reinterpret_cast<sockaddr *>(&server->local);
    int rc = 0;

    //
    // Establish a packet (raw) socket to be used by the server thread giving it full L2 packets
    //
    struct sockaddr s;
    socklen_t len = sizeof(s);
    getpeername(sockfd, &s, &len);
    if (isIPV6(server)) {
        server->mSock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IPV6));
        WARN_errno(server->mSock == INVALID_SOCKET, "ip6 packet socket (AF_PACKET)");
        server->l4offset = IPV6HDRLEN + sizeof(struct ether_header);
    } else {
        server->mSock = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_IP));
        WARN_errno(server->mSock == INVALID_SOCKET, "ip packet socket (AF_PACKET)");
        unsetIPV6(server);
        server->l4offset = sizeof(struct iphdr) + sizeof(struct ether_header);
    }
    // Didn't get a valid socket, return now
    if (server->mSock < 0) {
        return false;
    }
    // More per thread settings based on using a packet socket
    server->l4payloadoffset = server->l4offset + sizeof(struct udphdr);
    server->recvflags = MSG_TRUNC;
    // The original AF_INET socket only exists to keep the connected state
    // in the OS for this flow. Fast drop packets there as
    // now packets will use the AF_PACKET (raw) socket
    // Also, store the original AF_INET socket descriptor so it can be
    // closed in the Server's destructor.  (Note: closing the
    // socket descriptors will also free the cBPF.)
    //
    server->mSockDrop = sockfd;
    rc = SockAddr_Drop_All_BPF(sockfd);
    WARN_errno(rc == SOCKET_ERROR, "l2 all drop bpf");

    // Now optimize packet flow up the raw socket
    // Establish the flow BPF to forward up only "connected" packets to this raw socket
    if (l->sa_family == AF_INET6) {
#if HAVE_IPV6
        struct in6_addr *v6peer = SockAddr_get_in6_addr(&server->peer);
        struct in6_addr *v6local = SockAddr_get_in6_addr(&server->local);
        if (isIPV6(server)) {
            rc = SockAddr_v6_Connect_BPF(server->mSock, v6local, v6peer, (reinterpret_cast<struct sockaddr_in6 *>(l))->sin6_port, (reinterpret_cast<struct sockaddr_in6 *>(p))->sin6_port);
            WARN_errno(rc == SOCKET_ERROR, "l2 connect ipv6 bpf");
        } else {
            // This is an ipv4 address in a v6 family (structure), just pull the lower 32 bits for the v4 addr
            rc = SockAddr_v4_Connect_BPF(server->mSock, v6local->s6_addr32[3], v6peer->s6_addr32[3], (reinterpret_cast<struct sockaddr_in6 *>(l))->sin6_port, (reinterpret_cast<struct sockaddr_in6 *>(p))->sin6_port);
            WARN_errno(rc == SOCKET_ERROR, "l2 v4in6 connect ip bpf");
        }
#else
        fprintf(stderr, "Unfortunately, IPv6 is not supported on this platform\n");
        return false;
#endif /* HAVE_IPV6 */
    } else {
        rc = SockAddr_v4_Connect_BPF(server->mSock, (reinterpret_cast<struct sockaddr_in *>(l))->sin_addr.s_addr, (reinterpret_cast<struct sockaddr_in *>(p))->sin_addr.s_addr, (reinterpret_cast<struct sockaddr_in *>(l))->sin_port, (reinterpret_cast<struct sockaddr_in *>(p))->sin_port);
        WARN_errno(rc == SOCKET_ERROR, "l2 connect ip bpf");
    }
    return rc >= 0;
#else
    fprintf(stderr, "Client requested --l2checks but not supported on this platform\n");
    return false;
#endif
}

bool Listener::tap_setup (thread_Settings *server, int sockfd) {
#if defined(HAVE_IF_TUNTAP) && defined(HAVE_AF_PACKET) && defined(HAVE_DECL_SO_BINDTODEVICE)
    struct sockaddr *p = reinterpret_cast<sockaddr *>(&server->peer);
    struct sockaddr *l = reinterpret_cast<sockaddr *>(&server->local);
    int rc = 0;

    //
    // Establish a packet (raw) socket to be used by the server thread giving it full L2 packets
    //
    struct sockaddr s;
    socklen_t len = sizeof(s);
    getpeername(sockfd, &s, &len);
    if (isIPV6(server)) {
        server->l4offset = IPV6HDRLEN + sizeof(struct ether_header);
    } else {
        server->l4offset = sizeof(struct iphdr) + sizeof(struct ether_header);
    }
    // Didn't get a valid socket, return now
    if (server->mSock < 0) {
        return false;
    }
    // More per thread settings based on using a packet socket
    server->l4payloadoffset = server->l4offset + sizeof(struct udphdr);
    server->recvflags = MSG_TRUNC;
    // Now optimize packet flow up the raw socket
    // Establish the flow BPF to forward up only "connected" packets to this raw socket
    if (l->sa_family == AF_INET6) {
#if HAVE_IPV6
        struct in6_addr *v6peer = SockAddr_get_in6_addr(&server->peer);
        struct in6_addr *v6local = SockAddr_get_in6_addr(&server->local);
        if (isIPV6(server)) {
            rc = SockAddr_v6_Connect_BPF(server->mSock, v6local, v6peer, (reinterpret_cast<struct sockaddr_in6 *>(l))->sin6_port, (reinterpret_cast<struct sockaddr_in6 *>(p))->sin6_port);
            WARN_errno(rc == SOCKET_ERROR, "l2 connect ipv6 bpf");
        } else {
            // This is an ipv4 address in a v6 family (structure), just pull the lower 32 bits for the v4 addr
            rc = SockAddr_v4_Connect_BPF(server->mSock, v6local->s6_addr32[3], v6peer->s6_addr32[3], (reinterpret_cast<struct sockaddr_in6 *>(l))->sin6_port, (reinterpret_cast<struct sockaddr_in6 *>(p))->sin6_port);
            WARN_errno(rc == SOCKET_ERROR, "l2 v4in6 connect ip bpf");
        }
#else
        fprintf(stderr, "Unfortunately, IPv6 is not supported on this platform\n");
        return false;
#endif /* HAVE_IPV6 */
    } else {
        rc = SockAddr_v4_Connect_BPF(server->mSock, (reinterpret_cast<struct sockaddr_in *>(l))->sin_addr.s_addr, (reinterpret_cast<struct sockaddr_in *>(p))->sin_addr.s_addr, (reinterpret_cast<struct sockaddr_in *>(l))->sin_port, (reinterpret_cast<struct sockaddr_in *>(p))->sin_port);
        WARN_errno(rc == SOCKET_ERROR, "l2 connect ip bpf");
    }
    return rc >= 0;
#else
    fprintf(stderr, "Client requested --l2checks but not supported on this platform\n");
    return false;
#endif
}

/* ------------------------------------------------------------------------
 * Do the equivalent of an accept() call for UDP sockets. This checks
 * a listening UDP socket for new or first received datagram
 * ------------------------------------------------------------------- ----*/
int Listener::udp_accept (thread_Settings *server) {
    assert(server != NULL);
    int nread = 0;
    assert(ListenSocket > 0);
    // Preset the server socket to INVALID, hang recvfrom on the Listener's socket
    // The INVALID socket is used to keep the while loop going
    server->mSock = INVALID_SOCKET;
    bool drainstalepkts = true;
    struct UDP_datagram* mBuf_UDP  = reinterpret_cast<struct UDP_datagram*>(server->mBuf);
    // Look for a UDP packet with a postive seq no while draining any neg seq no packets
    // The UDP client traffic thread uses negative seq numbers to indicate to the server that
    // traffic is over. Some of those "final" packets can be in the stack/network pipeline after the server
    // thread has ended and closed its reporting. The Listener will now receive them as if they are
    // "first packets. Any "new" packets seen by the Listener causes a new "udp accept"
    // per UDP's stateless design. So, in the case of negative seq nos, we know that this is
    // most likely not a new client thread requiring a new server thread, but remnants of an
    // old one that already ended. Hence, the Listener should ignore "first packets" when
    // they have negative seq numbers.
 RETRYREAD:
    do {
        nread = recvfrom(ListenSocket, server->mBuf, server->mBufLen, 0, \
                         reinterpret_cast<struct sockaddr*>(&server->peer), &server->size_peer);
        if (nread > 0) {
            struct client_udp_testhdr *hdr = reinterpret_cast<struct client_udp_testhdr *>(server->mBuf);
            uint32_t flags = ntohl(hdr->base.flags);
            setSeqNo64b(server);
            if (!isCompat(mSettings) && (nread >= 4) && !(flags & HEADER_SEQNO64B)) {
                unsetSeqNo64b(server);
            }
            // filter and ignore negative sequence numbers, these can be heldover from a previous run
            if (isSeqNo64b(server)) {
                // New client - Signed PacketID packed into unsigned id2,id
                intmax_t packetID64 = (static_cast<uint32_t>(ntohl(mBuf_UDP->id))) | (static_cast<uintmax_t>(ntohl(mBuf_UDP->id2)) << 32);
                if (packetID64 > 0)
                    drainstalepkts = false;
            } else {
                // Old client - Signed PacketID in Signed id
                int32_t packetID32 = static_cast<int32_t>(ntohl(mBuf_UDP->id));
                if (packetID32 > 0)
                    drainstalepkts = false;
            }
        }
    } while ((nread > 0) && drainstalepkts && !sInterupted);
    FAIL_errno(nread == SOCKET_ERROR, "recvfrom", mSettings);
    if ((nread > 0) && !sInterupted) {
        Timestamp now;
        server->accept_time.tv_sec = now.getSecs();
        server->accept_time.tv_usec = now.getUsecs();
        // Drop duplicates, may need to use a BPF drop for better performance
        // or ebfs
        if (!Iperf_push_host(server)) {
            goto RETRYREAD;
        } else {
            int rc;
            // We have a new UDP flow (based upon key of quintuple)
            // so let's hand off this socket
            // to the server and create a new listener socket
            server->mSock = ListenSocket;
            ListenSocket = INVALID_SOCKET;
            // This connect() will allow the OS to only
            // send packets with the ip quintuple up to the server
            // socket and, hence, to the server thread (yet to be created)
            // This connect() routing is only supported with AF_INET or AF_INET6 sockets,
            // e.g. AF_PACKET sockets can't do this.  We'll handle packet sockets later
            // All UDP accepts here will use AF_INET.  This is intentional and needed
            if (!isMulticast(server)) {
                rc = connect(server->mSock, reinterpret_cast<struct sockaddr*>(&server->peer), server->size_peer);
                FAIL_errno(rc == SOCKET_ERROR, "connect UDP", mSettings);
                server->size_local = sizeof(iperf_sockaddr);
                getsockname(server->mSock, reinterpret_cast<sockaddr*>(&server->local), &server->size_local);
                SockAddr_Ifrname(server);
            } else {
                server->size_multicast_group = sizeof(iperf_sockaddr);
                iperf_sockaddr sent_dstaddr;
                getsockname(server->mSock, reinterpret_cast<sockaddr*>(&sent_dstaddr), &server->size_multicast_group);
                int join_send_match = SockAddr_Hostare_Equal(&sent_dstaddr, &server->multicast_group);
#if DEBUG_MCAST
                char joinaddr[200];
                char pktaddr[200];
                size_t len=200;
                SockAddr_getHostAddress(&sent_dstaddr, joinaddr, len);
                SockAddr_getHostAddress(&server->multicast_group, pktaddr, len);
                printf("mcast(%d): join addr %s pkt group addr %s\n", join_send_match, joinaddr, pktaddr);
#endif
                WARN(!join_send_match, "mcast join and packet group addr");
                // RJM - use a cmesg to read the interface name
            }
            server->firstreadbytes = nread;
        }
    }
    return server->mSock;
}


#if (((HAVE_TUNTAP_TUN) || (HAVE_TUNTAP_TAP)) && (AF_PACKET))
int Listener::tuntap_accept(thread_Settings *server) {
    int rc = recv(server->mSock, server->mBuf, (server->mBufLen + TAPBYTESSLOP + sizeof(struct iphdr) + sizeof(struct ether_header) + sizeof(struct udphdr)), 0);
    if (rc <= 0)
        return 0;
    //	rc = udpchecksum((void *)ip_hdr, (void *)udp_hdr, udplen, (isIPV6(mSettings) ? 1 : 0));
    struct iphdr *l3hdr = (struct iphdr *)((char *)server->mBuf + sizeof(struct ether_header));
    struct udphdr *l4hdr = (struct udphdr *)((char *)server->mBuf + sizeof(struct iphdr) + sizeof(struct ether_header));
    //    uint16_t ipver = (uint16_t) ntohs(mBuf + sizeof(struct ether_header));
    //    printf ("*** version = %d\n", ipver);
    // Note: sockaddrs are stored in network bytes order
    struct sockaddr_in *local = (struct sockaddr_in *) &server->local;
    struct sockaddr_in *peer = (struct sockaddr_in *) &server->peer;
    server->size_peer = sizeof(iperf_sockaddr);
    server->size_local = sizeof(iperf_sockaddr);
    peer->sin_family = AF_INET;
    local->sin_family = AF_INET;
    peer->sin_addr.s_addr = l3hdr->saddr;
    local->sin_addr.s_addr = l3hdr->daddr;
    peer->sin_port = l4hdr->source;
    local->sin_port = l4hdr->dest;
    server->l4offset = sizeof(struct iphdr) + sizeof(struct ether_header);
    SockAddr_v4_Connect_TAP_BPF(server->mSock, local->sin_addr.s_addr, peer->sin_addr.s_addr, local->sin_port, peer->sin_port);
    server->l4payloadoffset = sizeof(struct iphdr) + sizeof(struct ether_header) + sizeof(struct udphdr);
    server->firstreadbytes = rc;
    return server->mSock;
}
#endif
/* -------------------------------------------------------------------
 * This is called by the Listener thread main loop, return a socket or error
 * ------------------------------------------------------------------- */
int Listener::my_accept (thread_Settings *server) {
    assert(server != NULL);
#ifdef HAVE_THREAD_DEBUG
    if (isUDP(server)) {
        thread_debug("Listener thread listening for UDP (sock=%d)", ListenSocket);
    } else {
        thread_debug("Listener thread listening for TCP (sock=%d)", ListenSocket);
    }
#endif
    SockAddr_zeroAddress(&server->peer);
    server->size_peer = sizeof(iperf_sockaddr);
    server->accept_time.tv_sec = 0;
    server->accept_time.tv_usec = 0;
    if (isUDP(server)) {
#if (((HAVE_TUNTAP_TUN) || (HAVE_TUNTAP_TAP)) && (AF_PACKET))
        if (isTapDev(server) || isTunDev(server)) {
            server->mSock = tuntap_accept(server);
        } else
#endif
            {
                server->mSock = udp_accept(server);
            }
        // note udp_accept will update the active host table
    } else {
        // accept a TCP  connection
        server->mSock = accept(ListenSocket, reinterpret_cast<sockaddr*>(&server->peer), &server->size_peer);
        if (server->mSock > 0) {
            Timestamp now;
            server->accept_time.tv_sec = now.getSecs();
            server->accept_time.tv_usec = now.getUsecs();
            server->size_local = sizeof(iperf_sockaddr);
            getsockname(server->mSock, reinterpret_cast<sockaddr*>(&server->local), &server->size_local);
            SockAddr_Ifrname(server);
            Iperf_push_host(server);
        }
    }
    return server->mSock;
} // end my_accept

// Read deep enough into the packet to get the client settings
// Read the headers but don't pull them from the queue in order to
// preserve server thread accounting, i.e. these exchanges will
// be part of traffic accounting. Return false if it's determined
// this traffic shouldn't be accepted for a test run
// Description of bits and fields is in include/payloads.h
bool Listener::apply_client_settings (thread_Settings *server) {
    assert(server != NULL);
    bool rc = false;

    // Set the receive timeout for the very first read
    if (mSettings->mListenerTimeout >= 0) {
        SetSocketOptionsReceiveTimeout(server, (int) (mSettings->mListenerTimeout * 1000000));
    } else {
        int sorcvtimer = DEFAULT_TESTEXCHANGETIMEOUT; // in usecs
        SetSocketOptionsReceiveTimeout(server, sorcvtimer);
    }
    server->peer_version_u = 0;
    server->peer_version_l = 0;
    server->mMode = kTest_Normal;

    if (isUDP(server)) {
        rc = apply_client_settings_udp(server);
    } else if (!isConnectOnly(server)) {
        rc = apply_client_settings_tcp(server);
    }
    if (isOverrideTOS(server)) {
        SetSocketOptionsIPTos(server, server->mRTOS);
    } else if (server->mTOS) {
        SetSocketOptionsIPTos(server, server->mTOS);
    }
    return rc;
}

inline bool Listener::test_permit_key(uint32_t flags, thread_Settings *server, int keyoffset) {
    if (!(flags & HEADER_KEYCHECK)) {
        server->mKeyCheck= false;
        return false;
    }
    struct permitKey *thiskey = reinterpret_cast<struct permitKey *>(server->mBuf + (keyoffset - sizeof(thiskey->length)));
    int keylen = ntohs(thiskey->length);
    if ((keylen < MIN_PERMITKEY_LEN) || (keylen > MAX_PERMITKEY_LEN)) {
        server->mKeyCheck= false;
        //	fprintf(stderr, "REJECT: key length bounds error (%d)\n", keylen);
        return false;
    }
    if (keylen != static_cast<int>(strlen(server->mPermitKey))) {
        server->mKeyCheck= false;
        //	fprintf(stderr, "REJECT: key length mismatch (%d!=%d)\n", keylen, (int) strlen(server->mPermitKey));
        return false;
    }
    if (!isUDP(server)) {
        int nread = 0;
        nread = recvn(server->mSock, reinterpret_cast<char *>(&thiskey->value), keylen, 0);
        FAIL_errno((nread < keylen), "read key", server);
    }
    strncpy(server->mPermitKey, thiskey->value, MAX_PERMITKEY_LEN + 1);
    if (strncmp(server->mPermitKey, mSettings->mPermitKey, keylen) != 0) {
        server->mKeyCheck= false;
        //	fprintf(stderr, "REJECT: key value mismatch per %s\n", thiskey->value);
        return false;
    }
    server->mKeyCheck= true;
    return true;
}

bool Listener::apply_client_settings_udp (thread_Settings *server) {
    struct client_udp_testhdr *hdr = reinterpret_cast<struct client_udp_testhdr *>(server->mBuf + server->l4payloadoffset);
    uint32_t flags = ntohl(hdr->base.flags);
    uint16_t upperflags = 0;
#if HAVE_THREAD_DEBUG
    thread_debug("UDP test flags = %X", flags);
#endif
    if (flags & HEADER32_SMALL_TRIPTIMES) {
#if HAVE_THREAD_DEBUG
        thread_debug("UDP small header");
#endif
        struct client_udpsmall_testhdr *smallhdr = reinterpret_cast<struct client_udpsmall_testhdr *>(server->mBuf + server->l4payloadoffset);
        server->sent_time.tv_sec = ntohl(hdr->seqno_ts.tv_sec);
        server->sent_time.tv_usec = ntohl(hdr->seqno_ts.tv_usec);
        server->txstart_epoch.tv_sec = ntohl(smallhdr->start_tv_sec);
        server->txstart_epoch.tv_usec = ntohl(smallhdr->start_tv_usec);
        uint32_t seqno = ntohl(hdr->seqno_ts.id);
        if (server->txstart_epoch.tv_sec > 0) {
            setTxStartTime(server);
        }
        if (seqno != 1) {
            fprintf(stderr, "WARN: first received packet (id=%d) was not first sent packet, report start time will be off\n", seqno);
        }
        Timestamp now;
        if (!isTxStartTime(server) && ((abs(now.getSecs() - server->sent_time.tv_sec)) > (MAXDIFFTIMESTAMPSECS + 1))) {
            fprintf(stdout,"WARN: ignore --trip-times because client didn't provide valid start timestamp within %d seconds of now\n", MAXDIFFTIMESTAMPSECS);
            unsetTripTime(server);
        } else {
            setTripTime(server);
        }
        setEnhanced(server);
    } else if ((flags & HEADER_VERSION1) || (flags & HEADER_VERSION2) || (flags & HEADER_EXTEND)) {
        if (flags & HEADER_VERSION1) {
            uint32_t tidthreads = ntohl(hdr->base.numThreads);
            if (tidthreads & HEADER_HASTRANSFERID) {
                tidthreads &= (~HEADER_HASTRANSFERID & HEADER_TRANSFERIDMASK);
                server->mPeerTransferID = tidthreads >> HEADER_TRANSFERIDSHIFT;
                setSyncTransferID(server);
            } else if (!(flags & HEADER_VERSION2)) {
                if (flags & RUN_NOW)
                    server->mMode = kTest_DualTest;
                else
                    server->mMode = kTest_TradeOff;
            }
        }
        if (flags & HEADER_EXTEND) {
            upperflags = htons(hdr->extend.upperflags);
            server->mTOS = ntohs(hdr->extend.tos);
            server->peer_version_u = ntohl(hdr->extend.version_u);
            server->peer_version_l = ntohl(hdr->extend.version_l);
            if (flags & HEADER_UDPTESTS) {
                // Handle stateless flags
                if (upperflags & HEADER_ISOCH) {
                    setIsochronous(server);
                }
                if (upperflags & HEADER_L2ETHPIPV6) {
                    setIPV6(server);
                } else {
                    unsetIPV6(server);
                }
                if (upperflags & HEADER_L2LENCHECK) {
                    setL2LengthCheck(server);
                }
                if (upperflags & HEADER_NOUDPFIN) {
                    setNoUDPfin(server);
                }
                if (upperflags & HEADER_UDPL4S) {
                    setUDPL4S(server);
                    SetSocketOptionsIPRCVTos(server);
                }
            }
            if (upperflags & HEADER_EPOCH_START) {
                server->txstart_epoch.tv_sec = ntohl(hdr->start_fq.start_tv_sec);
                server->txstart_epoch.tv_usec = ntohl(hdr->start_fq.start_tv_usec);
                Timestamp now;
                if ((abs(now.getSecs() - server->txstart_epoch.tv_sec)) > (MAXDIFFTXSTART + 1)) {
                    fprintf(stdout,"WARN: ignore --txstart-time because client didn't provide valid start timestamp within %d seconds of now\n", MAXDIFFTXSTART);
                    unsetTxStartTime(server);
                } else {
                    setTxStartTime(server);
                }
            }
            if (upperflags & HEADER_TRIPTIME) {
                server->sent_time.tv_sec = ntohl(hdr->start_fq.start_tv_sec);
                server->sent_time.tv_usec = ntohl(hdr->start_fq.start_tv_usec);
                Timestamp now;
                if (!isTxStartTime(server) && ((abs(now.getSecs() - server->sent_time.tv_sec)) > (MAXDIFFTIMESTAMPSECS + 1))) {
                    fprintf(stdout,"WARN: ignore --trip-times because client didn't provide valid start timestamp within %d seconds of now\n", MAXDIFFTIMESTAMPSECS);
                    unsetTripTime(server);
                } else {
                    setTripTime(server);
                    setEnhanced(server);
                }
            }
        }
        if (flags & HEADER_VERSION2) {
            upperflags = htons(hdr->extend.upperflags);
            if (upperflags & HEADER_FULLDUPLEX) {
                setFullDuplex(server);
                setServerReverse(server);
            }
            if (upperflags & HEADER_REVERSE)  {
                server->mThreadMode=kMode_Client;
                setServerReverse(server);
                if (server->mSumReport != NULL) {
                    server->mSumReport->info.common->ThreadMode=kMode_Client;
                    setServerReverse(server->mSumReport->info.common);
                }
                setNoUDPfin(server);
                unsetReport(server);
            }
        }
    }
    return true;
}
bool Listener::apply_client_settings_tcp (thread_Settings *server) {
    bool rc = false;
#if HAVE_TCP_STATS
    if (!isUDP(mSettings)) {
        gettcpinfo(server->mSock, &server->tcpinitstats);
    }
#endif
    int nread = recvn(server->mSock, server->mBuf, sizeof(uint32_t), 0);
    char *readptr = server->mBuf;
    if (nread == 0) {
        //peer closed the socket, with no writes e.g. a connect-only test
        WARN(1, "read tcp flags (peer close)");
        goto DONE;
    }
    if (nread < (int) sizeof(uint32_t)) {
        WARN(1, "read tcp flags (runt)");
        goto DONE;
    } else {
        rc = true;
        readptr += nread;
        struct client_tcp_testhdr *hdr = reinterpret_cast<struct client_tcp_testhdr *>(server->mBuf);
        uint32_t flags = ntohl(hdr->base.flags);
        if (flags & HEADER_BOUNCEBACK) {
            if (!isServerModeTime(server)) {
                unsetModeTime(server);
            }
            struct bounceback_hdr *bbhdr = reinterpret_cast<struct bounceback_hdr *>(server->mBuf);
            setBounceBack(server);
            nread = recvn(server->mSock, readptr, sizeof(struct bounceback_hdr), 0);
            if (nread != sizeof(struct bounceback_hdr)) {
                WARN(1, "read bounce back header failed");
                rc = false;
                goto DONE;
            }
            readptr += nread;
            server->mBounceBackBytes = ntohl(bbhdr->bbsize);
            if (server->mBounceBackBytes > server->mBufLen) {
                if (isBuflenSet(server)) {
                    WARN(1, "Buffer length (-l) too small for bounceback request. Increase -l size or don't set (for auto-adjust)");
                    rc = false;
                    goto DONE;
                } else {
                    int read_offset = readptr - server->mBuf;
                    Settings_Grow_mBuf(server, server->mBounceBackBytes);
                    readptr = server->mBuf + read_offset;
                    bbhdr = reinterpret_cast<struct bounceback_hdr *>(server->mBuf);
                }
            }
            server->mBounceBackHold = ntohl(bbhdr->bbhold);
            uint16_t bbflags = ntohs(bbhdr->bbflags);
            if (bbflags & HEADER_BBCLOCKSYNCED) {
                setTripTime(server);
                server->sent_time.tv_sec = ntohl(bbhdr->bbclientTx_ts.sec);
                server->sent_time.tv_usec = ntohl(bbhdr->bbclientTx_ts.usec);
            }
            if (bbflags & HEADER_BBTOS) {
                server->mTOS = ntohs(bbhdr->tos);
            }
#if HAVE_DECL_TCP_QUICKACK
            if (bbflags & HEADER_BBQUICKACK) {
                setTcpQuickAck(server);
            }
#endif
            if (bbflags & HEADER_BBREPLYSIZE) {
                server->mBounceBackReplyBytes = ntohl(bbhdr->bbreplysize);
            } else {
                server->mBounceBackReplyBytes = server->mBounceBackBytes;
            }
            if (server->mBounceBackReplyBytes > server->mBufLen) {
                if (isBuflenSet(server)) {
                    WARN(1, "Buffer length (-l) too small for bounceback reply. Increase -l size or don't set (for auto-adjust)");
                    rc = false;
                    goto DONE;
                } else {
                    int read_offset = readptr - server->mBuf;
                    Settings_Grow_mBuf(server, server->mBounceBackReplyBytes);
                    readptr = server->mBuf + read_offset;
                    bbhdr = reinterpret_cast<struct bounceback_hdr *>(server->mBuf);
                }
            }
            int remaining =  server->mBounceBackBytes - (sizeof(struct bounceback_hdr) + sizeof(uint32_t));
            if (remaining < 0) {
                WARN(1, "bounce back bytes too small");
                rc = false;
                goto DONE;
            } else if (remaining > 0) {
                nread = recvn(server->mSock, readptr, remaining, 0);
                if (nread != remaining) {
                    WARN(1, "read bounce back payload failed");
                    rc = false;
                    goto DONE;
                }
            }
            Timestamp now;
            bbhdr->bbserverRx_ts.sec = htonl(now.getSecs());
            bbhdr->bbserverRx_ts.usec = htonl(now.getUsecs());
        } else {
            uint16_t upperflags = 0;
            uint16_t lowerflags = 0;
            int readlen;
            // figure out the length of the test header
            if ((readlen = Settings_ClientTestHdrLen(flags, server)) > 0) {
                if (readlen > (server->mBufLen - nread)) {
                    WARN(1, "read tcp header too large");
                    rc = false;
                    goto DONE;
                }
                // read the test settings passed to the server by the client
                nread += recvn(server->mSock, readptr, (readlen - (int) sizeof(uint32_t)), 0);
                FAIL_errno((nread < readlen), "read tcp test info", server);
                if (isPermitKey(mSettings)) {
                    if (!test_permit_key(flags, server, readlen)) {
                        rc = false;
                        goto DONE;
                    }
                } else if (flags & HEADER_KEYCHECK) {
                    rc = false;
                    server->mKeyCheck = false;
                    goto DONE;
                }
                server->firstreadbytes = nread;
                struct client_tcp_testhdr *hdr = reinterpret_cast<struct client_tcp_testhdr*>(server->mBuf);
                if (flags & HEADER_VERSION1) {
                    uint32_t tidthreads = ntohl(hdr->base.numThreads);
                    if (tidthreads & HEADER_HASTRANSFERID) {
                        tidthreads &= (~HEADER_HASTRANSFERID & HEADER_TRANSFERIDMASK);
                        server->mPeerTransferID = tidthreads >> HEADER_TRANSFERIDSHIFT;
                        setSyncTransferID(server);
                    } else if (!(flags & HEADER_VERSION2)) {
                        if (flags & RUN_NOW)
                            server->mMode = kTest_DualTest;
                        else
                            server->mMode = kTest_TradeOff;
                    }
                }
                if (flags & HEADER_EXTEND) {
                    upperflags = htons(hdr->extend.upperflags);
                    lowerflags = htons(hdr->extend.lowerflags);
                    server->mTOS = ntohs(hdr->extend.tos);
                    server->peer_version_u = ntohl(hdr->extend.version_u);
                    server->peer_version_l = ntohl(hdr->extend.version_l);
                    if (upperflags & HEADER_ISOCH) {
                        setIsochronous(server);
                    }
                    if (upperflags & HEADER_EPOCH_START) {
                        server->txstart_epoch.tv_sec = ntohl(hdr->start_fq.start_tv_sec);
                        server->txstart_epoch.tv_usec = ntohl(hdr->start_fq.start_tv_usec);
                        Timestamp now;
                        if ((abs(now.getSecs() - server->txstart_epoch.tv_sec)) > (MAXDIFFTXSTART + 1)) {
                            fprintf(stdout,"WARN: ignore --txstart-time because client didn't provide valid start timestamp within %d seconds of now\n", MAXDIFFTXSTART);
                            unsetTxStartTime(server);
                        } else {
                            setTxStartTime(server);
                        }
                    }
                    if (upperflags & HEADER_TRIPTIME) {
                        Timestamp now;
                        server->sent_time.tv_sec = ntohl(hdr->start_fq.start_tv_sec);
                        server->sent_time.tv_usec = ntohl(hdr->start_fq.start_tv_usec);
                        if (!isTxStartTime(server) && ((abs(now.getSecs() - server->sent_time.tv_sec)) > (MAXDIFFTIMESTAMPSECS + 1))) {
                            fprintf(stdout,"WARN: ignore --trip-times because client didn't provide valid start timestamp within %d seconds of now\n", MAXDIFFTIMESTAMPSECS);
                            unsetTripTime(server);
                        } else {
                            setTripTime(server);
                            setEnhanced(server);
                        }
                    }
                    if (upperflags & HEADER_PERIODICBURST) {
                        setEnhanced(server);
                        setFrameInterval(server);
                        setPeriodicBurst(server);
                        {
                            struct client_tcp_testhdr *hdr = reinterpret_cast<struct client_tcp_testhdr *>(server->mBuf);
                            server->mFPS = ntohl(hdr->isoch_settings.FPSl);
                            server->mFPS += ntohl(hdr->isoch_settings.FPSu) / static_cast<double>(rMillion);
                        }
                        if (!server->mFPS) {
                            server->mFPS = 1.0;
                        }
                    }
                    if ((lowerflags & HEADER_CCA) && !isCongestionControl(server)) {
#if HAVE_DECL_TCP_CONGESTION
                        int ccalen = ntohs(hdr->cca.cca_length);
                        setCongestionControl(server);
                        setEnhanced(server);
                        server->mCongestion = new char[ccalen+1];
                        if (server->mCongestion) {
                            strncpy(server->mCongestion, hdr->cca.value, ccalen);
                            server->mCongestion[ccalen] = '\0';
                            Socklen_t len = strlen(server->mCongestion) + 1;
                            int rc = setsockopt(server->mSock, IPPROTO_TCP, TCP_CONGESTION,
                                                server->mCongestion, len);
                            if (rc == SOCKET_ERROR) {
                                fprintf(stderr, "Attempt to set '%s' congestion control failed: %s\n",
                                        server->mCongestion, strerror(errno));
                                unsetCongestionControl(server);
                                DELETE_ARRAY(server->mCongestion);
                            }
                        }
#endif
                    }
                    if (lowerflags & HEADER_BARRIER_TIME) {
                        server->barrier_time = ntohl(hdr->extend.barrier_usecs);
                    }
                    if (flags & HEADER_VERSION2) {
                        if (upperflags & HEADER_FULLDUPLEX) {
                            setFullDuplex(server);
                            setServerReverse(server);
                        }
                        if (upperflags & HEADER_REVERSE) {
                            server->mThreadMode=kMode_Client;
                            setServerReverse(server);
                            if (server->mSumReport != NULL) {
                                server->mSumReport->info.common->ThreadMode=kMode_Client;
                                setServerReverse(server->mSumReport->info.common);
                            }
                        }
                    }
#if HAVE_DECL_TCP_NOTSENT_LOWAT
                    if ((isServerReverse(server) || isFullDuplex(server)) && (upperflags & HEADER_WRITEPREFETCH)) {
                        server->mWritePrefetch = ntohl(hdr->extend.TCPWritePrefetch);
                        if (server->mWritePrefetch > 0) {
                            setWritePrefetch(server);
                        }
                    }
#endif
#if HAVE_DECL_TCP_QUICKACK
                    if (upperflags & HEADER_TCPQUICKACK) {
                        setTcpQuickAck(server);
                    }
#endif
                    if (upperflags & HEADER_BOUNCEBACK) {
                        setBounceBack(server);
                    }
                }
            }
        }
        // Handle case that requires an ack back to the client
        // Signaled by not UDP (only supported by TCP)
        // and either 2.0.13 flags or the newer 2.0.14 flag of
        // V2PEERDETECT
        if (!isUDP(server) && !isCompat(mSettings) && \
            ((!(flags & HEADER_VERSION2) && (flags & HEADER_EXTEND)) || \
             (flags & HEADER_V2PEERDETECT))) {
            client_test_ack(server);
        }
    }
 DONE:
    return rc;
}

int Listener::client_test_ack(thread_Settings *server) {
    if (isUDP(server))
        return 1;

    client_hdr_ack ack;
    int sotimer = 0;
    int size = sizeof(struct client_hdr_ack);
    ack.typelen.type  = htonl(CLIENTHDRACK);

    ack.flags = 0;
    ack.reserved1 = 0;
    ack.reserved2 = 0;
    ack.version_u = htonl(IPERF_VERSION_MAJORHEX);
    ack.version_l = htonl(IPERF_VERSION_MINORHEX);
    if (isTripTime(server)) {
        ack.ts.sent_tv_sec = htonl(server->sent_time.tv_sec);
        ack.ts.sent_tv_usec = htonl(server->sent_time.tv_usec);
        ack.ts.sentrx_tv_sec = htonl(server->accept_time.tv_sec);
        ack.ts.sentrx_tv_usec = htonl(server->accept_time.tv_usec);
        Timestamp now;
        ack.ts.ack_tv_sec = htonl(now.getSecs());
        ack.ts.ack_tv_usec = htonl(now.getUsecs());
    } else {
        size -= sizeof (struct client_hdr_ack_ts);
    }
    ack.typelen.length = htonl(size);
    int rc = 1;
    // This is a version 2.0.10 or greater client
    // write back to the client so it knows the server
    // version

    // sotimer units microseconds convert
    if (server->mInterval) {
        sotimer = static_cast<int>((server->mInterval) / 4);
    } else if (isModeTime(server)) {
        sotimer = static_cast<int>((server->mAmount * 10000) / 4);
    }
    if (sotimer > HDRXACKMAX) {
        sotimer = HDRXACKMAX;
    } else if (sotimer < HDRXACKMIN) {
        sotimer = HDRXACKMIN;
    }
    SetSocketOptionsSendTimeout(server, sotimer);
#if HAVE_DECL_TCP_NODELAY
    int optflag = 1;
    // Disable Nagle to reduce latency of this intial message
    if ((rc = setsockopt(server->mSock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char *>(&optflag), sizeof(int))) < 0) {
        WARN_errno(rc < 0, "tcpnodelay");
    }
#endif
    if ((rc = send(server->mSock, reinterpret_cast<const char*>(&ack), size, 0)) < 0) {
        WARN_errno(rc <= 0, "send_ack");
        rc = 0;
    }
#if HAVE_DECL_TCP_NODELAY
    // Re-nable Nagle
    optflag = isNoDelay(server) ? 1 : 0;
    if (!isUDP(server) && (rc = setsockopt(server->mSock, IPPROTO_TCP, TCP_NODELAY, reinterpret_cast<char *>(&optflag), sizeof(int))) < 0) {
        WARN_errno(rc < 0, "tcpnodelay");
    }
#endif
    return rc;
}
