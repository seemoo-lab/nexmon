/*---------------------------------------------------------------
 * Copyright (c) 2023
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
 * iperf_multicast_api.c
 * pull iperf multicast code for maitainability
 *
 * by Robert J. McMahon (rjmcmahon@rjmcmahon.com, bob.mcmahon@broadcom.com)
 *
 *
 * Joins the multicast group or source and group (SSM S,G)
 *
 * taken from: https://www.ibm.com/support/knowledgecenter/en/SSLTBW_2.1.0/com.ibm.zos.v2r1.hale001/ipv6d0141001708.htm
 *
 * Multicast function	                                        IPv4	                   IPv6	                Protocol-independent
 * ==================                                           ====                       ====                 ====================
 * Level of specified option on setsockopt()/getsockopt()	IPPROTO_IP	           IPPROTO_IPV6	IPPROTO_IP or IPPROTO_IPV6
 * Join a multicast group	                                IP_ADD_MEMBERSHIP          IPV6_JOIN_GROUP	MCAST_JOIN_GROUP
 * Leave a multicast group or leave all sources of that
 *   multicast group	                                        IP_DROP_MEMBERSHIP	   IPV6_LEAVE_GROUP	MCAST_LEAVE_GROUP
 * Select outbound interface for sending multicast datagrams	IP_MULTICAST_IF	           IPV6_MULTICAST_IF	NA
 * Set maximum hop count	                                IP_MULTICAST_TTL	   IPV6_MULTICAST_HOPS	NA
 * Enable multicast loopback	                                IP_MULTICAST_LOOP	   IPV6_MULTICAST_LOOP	NA
 * Join a source multicast group	                        IP_ADD_SOURCE_MEMBERSHIP   NA	                MCAST_JOIN_SOURCE_GROUP
 * Leave a source multicast group	                        IP_DROP_SOURCE_MEMBERSHIP  NA	                MCAST_LEAVE_SOURCE_GROUP
 * Block data from a source to a multicast group	        IP_BLOCK_SOURCE   	   NA	                MCAST_BLOCK_SOURCE
 * Unblock a previously blocked source for a multicast group	IP_UNBLOCK_SOURCE	   NA	                MCAST_UNBLOCK_SOURCE
 *
 *
 * Reminder:  The os will decide which version of IGMP or MLD to use.   This may be controlled by system settings, e.g.:
 *
 * [rmcmahon@lvnvdb0987:~/Code/ssm/iperf2-code] $ sysctl -a | grep mld | grep force
 * net.ipv6.conf.all.force_mld_version = 0
 * net.ipv6.conf.default.force_mld_version = 0
 * net.ipv6.conf.lo.force_mld_version = 0
 * net.ipv6.conf.eth0.force_mld_version = 0
 *
 * [rmcmahon@lvnvdb0987:~/Code/ssm/iperf2-code] $ sysctl -a | grep igmp | grep force
 * net.ipv4.conf.all.force_igmp_version = 0
 * net.ipv4.conf.default.force_igmp_version = 0
 * net.ipv4.conf.lo.force_igmp_version = 0
 * net.ipv4.conf.eth0.force_igmp_version = 0
 *
 * ------------------------------------------------------------------- */
#include "headers.h"
#include "Settings.hpp"
#include "iperf_multicast_api.h"
#include "SocketAddr.h"
#include "util.h"

#ifdef HAVE_IFADDRS_H
#include <ifaddrs.h>
#endif

static unsigned int mcast_iface (struct thread_Settings *inSettings) {
    unsigned int iface=0;
    /* Set the interface or any */
    if (inSettings->mIfrname) {
#if HAVE_NET_IF_H && !WIN32
	iface = if_nametoindex(inSettings->mIfrname);
	FAIL_errno(!iface, "mcast if_nametoindex", inSettings);
#else
	fprintf(stderr, "multicast bind to device not supported on this platform\n");
#endif
    }
    return iface;
}


// IP_MULTICAST_ALL is on be default, disable it here.
// If set to 1, the socket will receive messages from all the groups that have been joined
// globally on the whole system.  Otherwise, it will deliver messages only from the
// groups that have been explicitly joined (for example via the IP_ADD_MEMBERSHIP option)
// on this particular socket.
#if HAVE_MULTICAST_ALL_DISABLE
static int iperf_multicast_all_disable (struct thread_Settings *inSettings) {
    int rc = 0;
#if HAVE_DECL_IP_MULTICAST_ALL
    int mc_all = 0;
    rc = setsockopt(inSettings->mSock, IPPROTO_IP, IP_MULTICAST_ALL, (void*) &mc_all, sizeof(mc_all));
    FAIL_errno(rc == SOCKET_ERROR, "ip_multicast_all", inSettings);
#endif
    return rc;
}
#endif

// This is the older mulitcast join code. Both SSM and binding the
// an interface requires the newer socket options.  Using the older
// code here will maintain compatiblity with previous iperf versions
static int iperf_multicast_join_v4_legacy (struct thread_Settings *inSettings) {
#if HAVE_DECL_IP_ADD_MEMBERSHIP
#if (HAVE_STRUCT_IP_MREQ) || (HAVE_STRUCT_IP_MREQN)
#if HAVE_STRUCT_IP_MREQ
    struct ip_mreq mreq;
    mreq.imr_interface.s_addr = htonl(INADDR_ANY);
    size_t len = sizeof(struct ip_mreq);
#elif HAVE_STRUCT_IP_MREQN
    //    struct ip_mreqn {
    //      struct in_addr imr_multiaddr; /* IP multicast address of group */
    //      struct in_addr imr_interface; /* local IP address of interface */
    //      int            imr_ifindex;   /* interface index */
    //    }
    struct ip_mreqn mreq;
    size_t len = sizeof(struct ip_mreqn);
    mreq.imr_address.s_addr = htonl(INADDR_ANY);
    mreq.imr_ifindex = mcast_iface(inSettings);
#endif
    memcpy(&mreq.imr_multiaddr, SockAddr_get_in_addr(&inSettings->multicast_group), sizeof(mreq.imr_multiaddr));
    int rc = setsockopt(inSettings->mSock, IPPROTO_IP, IP_ADD_MEMBERSHIP,
			(char*)(&mreq), len);
    FAIL_errno(rc == SOCKET_ERROR, "multicast join", inSettings);
#if HAVE_MULTICAST_ALL_DISABLE
    iperf_multicast_all_disable(inSettings);
#endif
    return ((rc == 0) ? IPERF_MULTICAST_JOIN_SUCCESS : IPERF_MULTICAST_JOIN_FAIL);
#endif
#endif
    return IPERF_MULTICAST_JOIN_UNSUPPORTED;
}

static int iperf_multicast_join_v4_pi (struct thread_Settings *inSettings) {
#if HAVE_DECL_MCAST_JOIN_GROUP
    int rc = -1;
    struct group_req group_req;

    memset(&group_req, 0, sizeof(struct group_req));
    memcpy(&group_req.gr_group, (struct sockaddr_in *)(&inSettings->multicast_group), sizeof(struct sockaddr_in));
    group_req.gr_interface = mcast_iface(inSettings);
    group_req.gr_group.ss_family = AF_INET;
    rc = setsockopt(inSettings->mSock, IPPROTO_IP, MCAST_JOIN_GROUP, (const char *)(&group_req),
		    (socklen_t) sizeof(struct group_source_req));
    FAIL_errno(rc == SOCKET_ERROR, "mcast v4 join group pi", inSettings);
    return ((rc == 0) ? IPERF_MULTICAST_JOIN_SUCCESS : IPERF_MULTICAST_JOIN_FAIL);
#else
    return IPERF_MULTICAST_JOIN_UNSUPPORTED;
#endif
}


static int iperf_multicast_join_v6 (struct thread_Settings *inSettings) {
#if (HAVE_DECL_IPV6_JOIN_GROUP || HAVE_DECL_IPV6_ADD_MEMBERSHIP)
#if HAVE_STRUCT_IPV6_MREQ
    struct ipv6_mreq mreq;
    memcpy(&mreq.ipv6mr_multiaddr, SockAddr_get_in6_addr(&inSettings->multicast_group), sizeof(mreq.ipv6mr_multiaddr));
    mreq.ipv6mr_interface = mcast_iface(inSettings);
#if HAVE_DECL_IPV6_JOIN_GROUP
    int rc = setsockopt(inSettings->mSock, IPPROTO_IPV6, IPV6_JOIN_GROUP, \
			(char*)(&mreq), sizeof(mreq));
#else
    int rc = setsockopt(inSettings->mSock, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP, \
			(char*)(&mreq), sizeof(mreq));
#endif
    FAIL_errno(rc == SOCKET_ERROR, "multicast v6 join", inSettings);
    return ((rc == 0) ? IPERF_MULTICAST_JOIN_SUCCESS : IPERF_MULTICAST_JOIN_FAIL);
#endif
#endif
    return IPERF_MULTICAST_JOIN_UNSUPPORTED;

}

static int iperf_multicast_join_v6_pi (struct thread_Settings *inSettings) {
#if HAVE_DECL_MCAST_JOIN_GROUP
    int rc = -1;
    struct group_req group_req;

    memset(&group_req, 0, sizeof(struct group_req));
    memcpy(&group_req.gr_group, (struct sockaddr_in6 *)(&inSettings->multicast_group), sizeof(struct sockaddr_in6));
    group_req.gr_interface = mcast_iface(inSettings);
    group_req.gr_group.ss_family = AF_INET6;
    rc = setsockopt(inSettings->mSock, IPPROTO_IPV6, MCAST_JOIN_GROUP, (const char *)(&group_req),
		    (socklen_t) sizeof(struct group_source_req));
    FAIL_errno(rc == SOCKET_ERROR, "mcast v6 join group", inSettings);
    return ((rc == 0) ? IPERF_MULTICAST_JOIN_SUCCESS : IPERF_MULTICAST_JOIN_FAIL);
#endif
    return IPERF_MULTICAST_JOIN_UNSUPPORTED;
}


static int iperf_multicast_ssm_join_v4 (struct thread_Settings *inSettings) {
#if HAVE_SSM_MULTICAST
    int rc;
    struct sockaddr_in *group;
    struct sockaddr_in *source;

    // Fill out both structures because we don't which one will succeed
    // and both may need to be tried
#if HAVE_STRUCT_IP_MREQ_SOURCE
    struct ip_mreq_source imr;
    memset (&imr, 0, sizeof (imr));
#endif
#if HAVE_STRUCT_GROUP_SOURCE_REQ
    struct group_source_req group_source_req;
    memset(&group_source_req, 0, sizeof(struct group_source_req));
    group_source_req.gsr_interface = mcast_iface(inSettings);
    group=(struct sockaddr_in *)(&group_source_req.gsr_group);
    source=(struct sockaddr_in *)(&group_source_req.gsr_source);
#else
    struct sockaddr_in imrgroup;
    struct sockaddr_in imrsource;
    group = &imrgroup;
    source = &imrsource;
#endif
    source->sin_family = AF_INET;
    group->sin_family = AF_INET;
    /* Set the group and SSM source*/
    memcpy(group, (struct sockaddr_in *)(&inSettings->multicast_group), sizeof(struct sockaddr_in));
    memcpy(source, (struct sockaddr_in *)(&inSettings->multicast_group_source), sizeof(struct sockaddr_in));
#if HAVE_STRUCT_SOCKADDR_IN_SIN_LEN
    source->sin_len = group->sin_len;
#endif
    source->sin_port = 0;    /* Ignored */
    rc = -1;

#if HAVE_DECL_MCAST_JOIN_SOURCE_GROUP
    rc = setsockopt(inSettings->mSock,IPPROTO_IP,MCAST_JOIN_SOURCE_GROUP, (const char *)(&group_source_req), \
		    sizeof(struct group_source_req));
    WARN(rc == SOCKET_ERROR, "mcast v4 join ssm join_src");
#endif

#if (HAVE_DECL_IP_ADD_SOURCE_MEMBERSHIP && HAVE_STRUCT_IP_MREQ_SOURCE)
    // Some operating systems will have MCAST_JOIN_SOURCE_GROUP but still fail
    // In those cases try the IP_ADD_SOURCE_MEMBERSHIP
    if (rc < 0) {
#if HAVE_STRUCT_IP_MREQ_SOURCE_IMR_MULTIADDR_S_ADDR
	imr.imr_multiaddr = ((const struct sockaddr_in *)group)->sin_addr;
	imr.imr_sourceaddr = ((const struct sockaddr_in *)source)->sin_addr;
#else
	// Some Android versions declare mreq_source without an s_addr
	imr.imr_multiaddr = ((const struct sockaddr_in *)group)->sin_addr.s_addr;
	imr.imr_sourceaddr = ((const struct sockaddr_in *)source)->sin_addr.s_addr;
#endif
	rc = setsockopt (inSettings->mSock, IPPROTO_IP, IP_ADD_SOURCE_MEMBERSHIP, (char*)(&imr), sizeof (imr));
	FAIL_errno(rc == SOCKET_ERROR, "mcast v4 join ssm add_src", inSettings);
    }
#endif
    return ((rc == 0) ? IPERF_MULTICAST_JOIN_SUCCESS : IPERF_MULTICAST_JOIN_FAIL);
#endif
    return IPERF_MULTICAST_JOIN_UNSUPPORTED;
}

static int iperf_multicast_ssm_join_v6 (struct thread_Settings *inSettings) {
#if (HAVE_IPV6_MULTICAST && HAVE_SSM_MULTICAST && HAVE_DECL_MCAST_JOIN_SOURCE_GROUP)
    int rc;

    // Here it's either an SSM S,G multicast join or a *,G with an interface specifier
    // Use the newer socket options when these are specified
    struct group_source_req group_source_req;
    struct sockaddr_in6 *group;
    struct sockaddr_in6 *source;

    memset(&group_source_req, 0, sizeof(struct group_source_req));

    group_source_req.gsr_interface = mcast_iface(inSettings);
    group=(struct sockaddr_in6*)(&group_source_req.gsr_group);
    source=(struct sockaddr_in6*)(&group_source_req.gsr_source);
    source->sin6_family = AF_INET6;
    group->sin6_family = AF_INET6;
    /* Set the group and SSM source*/
    memcpy(group, (struct sockaddr_in *)(&inSettings->multicast_group), sizeof(struct sockaddr_in6));
    memcpy(source, (struct sockaddr_in *)(&inSettings->multicast_group_source), sizeof(struct sockaddr_in6));
    group->sin6_port = 0;    /* Ignored */
#if HAVE_STRUCT_SOCKADDR_IN6_SIN6_LEN
    source->sin6_len = group->sin6_len;
#endif
    rc = setsockopt(inSettings->mSock,IPPROTO_IPV6,MCAST_JOIN_SOURCE_GROUP, (const char *)(&group_source_req),
		    sizeof(struct group_source_req));
    FAIL_errno(rc == SOCKET_ERROR, "mcast v6 join source group", inSettings);
    return ((rc == 0) ? IPERF_MULTICAST_JOIN_SUCCESS : IPERF_MULTICAST_JOIN_FAIL);
#endif
    return IPERF_MULTICAST_JOIN_UNSUPPORTED;
}

enum McastJoinResponse iperf_multicast_join (struct thread_Settings *inSettings) {
    int rc = IPERF_MULTICAST_JOIN_FAIL;
    if (!isSSMMulticast(inSettings)) {
	// *.G join
	if (!SockAddr_isIPv6(&inSettings->multicast_group)) {
	    if (!mcast_iface(inSettings)) {
		rc = iperf_multicast_join_v4_legacy(inSettings);
	    }
	    if (rc != IPERF_MULTICAST_JOIN_SUCCESS) {
		rc = iperf_multicast_join_v4_pi(inSettings);
	    }
	} else {
	    rc = iperf_multicast_join_v6(inSettings);
	    if (rc != IPERF_MULTICAST_JOIN_SUCCESS) {
		rc = iperf_multicast_join_v6_pi(inSettings);
	    }
	}
    } else {
	// SSM or S,G join
	if (!SockAddr_isIPv6(&inSettings->multicast_group)) {
	    rc = iperf_multicast_ssm_join_v4(inSettings);
	} else {
	    rc = iperf_multicast_ssm_join_v6(inSettings);
	}
    }
    return rc;
}

static void iperf_multicast_sync_ifrname (struct thread_Settings *inSettings) {
    if (inSettings->mIfrname && !inSettings->mIfrnametx) {
	int len = strlen(inSettings->mIfrname);
	inSettings->mIfrnametx = calloc((len + 1), sizeof(char));
	if (inSettings->mIfrnametx) {
	    strncpy(inSettings->mIfrnametx, inSettings->mIfrname, len+1);
	}
    }
    if (!inSettings->mIfrname && inSettings->mIfrnametx) {
	int len = strlen(inSettings->mIfrnametx);
	inSettings->mIfrname = calloc((len + 1), sizeof(char));
	if (inSettings->mIfrname) {
	    strncpy(inSettings->mIfrname, inSettings->mIfrnametx, len+1);
	}
    }
}

bool iperf_multicast_sendif_v4 (struct thread_Settings *inSettings) {
    bool result = false;
#if HAVE_DECL_IP_MULTICAST_IF
    struct in_addr interface_addr;
    memcpy(&interface_addr, SockAddr_get_in_addr(&inSettings->local), sizeof(interface_addr));
    int rc = setsockopt(inSettings->mSock, IPPROTO_IP, IP_MULTICAST_IF, \
			(char*)(&interface_addr), sizeof(interface_addr));
    if ((rc != SOCKET_ERROR) && SockAddr_Ifrname(inSettings)) {
	iperf_multicast_sync_ifrname(inSettings);
    }
    FAIL_errno(rc == SOCKET_ERROR, "v4 multicast if", inSettings);
    result =  ((rc == 0) ? true : false);
#endif
    return result;
}

bool iperf_multicast_sendif_v6 (struct thread_Settings *inSettings) {
    int result = false;
#if HAVE_DECL_IPV6_MULTICAST_IF && HAVE_NET_IF_H && !WIN32
    if (inSettings->mIfrnametx) {
	unsigned int ifindex = if_nametoindex(inSettings->mIfrnametx);
	if (ifindex) {
	    int rc = setsockopt(inSettings->mSock, IPPROTO_IPV6, IPV6_MULTICAST_IF, &ifindex, sizeof(ifindex));
	    if (rc == 0) {
		iperf_multicast_sync_ifrname(inSettings);
		result = true;
	    }
	}
    }
#endif
    return result;
}
