/*---------------------------------------------------------------
 * Copyrig h(c) 1999,2000,2001,2002,2003
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
 * active_hosts.c (was List.cpp)
 * rewrite by Robert McMahon
 *
 * This is a list to hold active traffic and create sum groups
 * sum groups are traffic sessions from the same client host
 * -------------------------------------------------------------------
 */

#include "active_hosts.h"
#include "Mutex.h"
#include "SocketAddr.h"
#include "Reporter.h"

/*
 * Global table with active hosts, their sum reports and active thread counts
 */
static struct Iperf_Table active_table;
static struct Iperf_ListEntry* Iperf_host_present (iperf_sockaddr *find);
static struct Iperf_ListEntry* Iperf_flow_present (iperf_sockaddr *find);

#if HAVE_THREAD_DEBUG
static void rcvfrom_peer_debug (thread_Settings *server, bool duplicate) {
    char tmpaddr[200];
    size_t len=200;
    unsigned short port = SockAddr_getPort(&server->peer);
    SockAddr_getHostAddress(&server->peer, tmpaddr, len);
    thread_debug("rcvfrom peer: %s port %d dup=%s", tmpaddr, port, (duplicate ? "true" : "false"));
}

static void active_table_show_entry(const char *action, Iperf_ListEntry *entry, int found) {
    assert(action != NULL);
    assert(entry != NULL);
    char tmpaddr[200];
    size_t len=200;
    unsigned short port = SockAddr_getPort(&(entry->host));
    SockAddr_getHostAddress(&(entry->host), tmpaddr, len);
    thread_debug("active table: %s %s port %d (flag=%d) rootp=%p entryp=%p hostcnt/flowcnt/threadcnt = %d/%d/%d", \
		 action, tmpaddr, port, found, (void *) active_table.sum_root, (void *) entry->sumreport, active_table.sum_count, \
		 active_table.flow_count, entry->thread_count);
}
static void active_table_show_compare(const char *action, Iperf_ListEntry *entry, iperf_sockaddr *host, const char *type) {
    assert(action != NULL);
    assert(entry != NULL);
    char lookupaddr[200];
    char findaddr[200];
    size_t len=200;
    unsigned short port = SockAddr_getPort(&(entry->host));
    unsigned short findport = SockAddr_getPort(host);
    SockAddr_getHostAddress(&(entry->host), lookupaddr, len);
    SockAddr_getHostAddress(host, findaddr, len);
    thread_debug("active table: compare table entry %s %s/%d against host %s/%d (%s)", type, lookupaddr, port, findaddr, findport, action);
}
#endif

void Iperf_initialize_active_table () {
    Mutex_Initialize(&active_table.my_mutex);
    active_table.flow_root = NULL;
    active_table.sum_root = NULL;
    active_table.groupid = 0;
#if HAVE_THREAD_DEBUG
    active_table.sum_count = 0;
    active_table.flow_count = 0;
#endif
}

/*
 * Add Entry add to the list or update thread count, return 0 on UDP tuple duplicate
 */
static inline struct Iperf_ListEntry *hostkey_insert (iperf_sockaddr *host) {
    struct Iperf_ListEntry *this_key = new Iperf_ListEntry();
    assert(this_key != NULL);
    if (!this_key) {
	fprintf(stderr, "Memory alloc failure in key insert\n");
	exit(1);
    }
    this_key->next = active_table.sum_root;
    active_table.sum_root = this_key;
    this_key->host = *host;
    this_key->thread_count = 0;
#if HAVE_THREAD_DEBUG
    active_table.sum_count++;
    active_table_show_entry("new host entry", this_key, ((SockAddr_are_Equal(&this_key->host, host) && SockAddr_Hostare_Equal(&this_key->host, host))));
#endif
    return this_key;
}

static inline struct Iperf_ListEntry *flowkey_insert (iperf_sockaddr *host) {
    struct Iperf_ListEntry *this_key = new Iperf_ListEntry();
    assert(this_key != NULL);
    if (!this_key) {
	fprintf(stderr, "Memory alloc failure in key insert\n");
	exit(1);
    }
    this_key->next = active_table.flow_root;
    active_table.flow_root = this_key;
    this_key->host = *host;
#if HAVE_THREAD_DEBUG
    active_table.flow_count++;
//    active_table_show_flow_entry("new flow entry", this_key, ((SockAddr_are_Equal(&this_key->host, host) && SockAddr_Hostare_Equal(&this_key->host, host))));
#endif
    return this_key;
}

static inline iperf_sockaddr *active_table_get_host_key (struct thread_Settings *agent) {
    iperf_sockaddr *key = ((isIncrDstIP(agent) || isSumServerDstIP(agent)) ? &agent->local : &agent->peer);
    return key;
}

static bool Iperf_push_flow (iperf_sockaddr *host) {
    bool rc;
    if (Iperf_flow_present(host)) {
	rc = false;
    } else {
	flowkey_insert(host);
	rc = true;
    }
    return rc;
}

// Thread access to store a host
bool Iperf_push_host (struct thread_Settings *agent) {
    Mutex_Lock(&active_table.my_mutex);
    if (isUDP(agent) && (agent->mThreadMode == kMode_Server)) {
	if (!Iperf_push_flow(&agent->peer)) {
	    // this is a duplicate on UDP, should just ignore
	    Mutex_Unlock(&active_table.my_mutex);
#if HAVE_THREAD_DEBUG
	    rcvfrom_peer_debug(agent, true);
#endif
	    return false;
	}
    }
    struct Iperf_ListEntry *this_host = Iperf_host_present(active_table_get_host_key(agent));
    if (!this_host) {
	this_host = hostkey_insert(active_table_get_host_key(agent));
	active_table.groupid++;
	this_host->sumreport = InitSumReport(agent, -active_table.groupid, false);
	this_host->sumreport->info.common->transferID = -active_table.groupid;
#if HAVE_THREAD_DEBUG
	active_table_show_entry("new sum report", this_host , 0);
#endif
    }
    agent->mSumReport = this_host->sumreport;
    this_host->thread_count++;
    IncrSumReportRefCounter(this_host->sumreport);
    this_host->socket = agent->mSock;
#if HAVE_THREAD_DEBUG
    active_table_show_entry("bind sum report", this_host, 0);
#endif
    Mutex_Unlock(&active_table.my_mutex);
    return true;
}

/*
 * Remove a host from the table
 */
void Iperf_remove_host (struct thread_Settings *agent) {
    iperf_sockaddr *del;
    // remove_list_entry(entry) {
    //     indirect = &head;
    //     while ((*indirect) != entry) {
    //	       indirect = &(*indirect)->next;
    //     }
    //     *indirect = entry->next
    Mutex_Lock(&active_table.my_mutex);
    // Delete any flow entries first
    if (isUDP(agent)) {
	del = &agent->peer;
	Iperf_ListEntry **tmp = &active_table.flow_root;
	while ((*tmp) && !(SockAddr_are_Equal(&(*tmp)->host, del))) {
	    tmp = &(*tmp)->next;
	}
	if (*tmp) {
	    Iperf_ListEntry *remove = (*tmp);
#if HAVE_THREAD_DEBUG
	    active_table.flow_count--;
#endif
	    *tmp = remove->next;
	    delete remove;
	}
    }

    del = active_table_get_host_key(agent);
    Iperf_ListEntry **tmp = &active_table.sum_root;
    while ((*tmp) && !(SockAddr_Hostare_Equal(&(*tmp)->host, del))) {
#if HAVE_THREAD_DEBUG
        active_table_show_compare("miss", *tmp, del, "client ip");
#endif
	tmp = &(*tmp)->next;
    }
    if (*tmp) {
	if (--(*tmp)->thread_count == 0) {
	    Iperf_ListEntry *remove = (*tmp);
	    agent->mSumReport = NULL;
#if HAVE_THREAD_DEBUG
	    active_table.sum_count--;
	    active_table_show_entry("delete", remove, 1);
#endif
	    *tmp = remove->next;
	    FreeSumReport(remove->sumreport);
	    delete remove;
	} else {
	    DecrSumReportRefCounter((*tmp)->sumreport);
#if HAVE_THREAD_DEBUG
	    active_table_show_entry("decr", (*tmp), 1);
#endif
	}
    }
    Mutex_Unlock(&active_table.my_mutex);
}

/*
 * Destroy the table
 */
void Iperf_destroy_active_table () {
    Iperf_ListEntry *itr1 = active_table.sum_root, *itr2;
    while (itr1 != NULL) {
        itr2 = itr1->next;
        delete itr1;
        itr1 = itr2;
    }
    itr1 = active_table.flow_root;
    while (itr1 != NULL) {
        itr2 = itr1->next;
        delete itr1;
        itr1 = itr2;
    }
    Mutex_Destroy(&active_table.my_mutex);
    active_table.sum_root = NULL;
#if HAVE_THREAD_DEBUG
    active_table.sum_count = 0;
#endif
}

/*
 * Check if the host and port are present in the active table
 */
struct Iperf_ListEntry* Iperf_flow_present (iperf_sockaddr *find) {
    Iperf_ListEntry *itr = active_table.flow_root;
    while (itr != NULL) {
        if (SockAddr_are_Equal(&itr->host, find)) {
#if HAVE_THREAD_DEBUG
	    active_table_show_compare("match host/port", itr, find, "client ip/port");
#endif
            break;
        } else {
#if HAVE_THREAD_DEBUG
	    active_table_show_compare("miss host/port", itr, find, "client ip/port");
#endif
	    itr = itr->next;
	}
    }
    return itr;
}

/*
 * Check if the host is present in the active table
 */
static Iperf_ListEntry* Iperf_host_present (iperf_sockaddr *find) {
    Iperf_ListEntry *itr = active_table.sum_root;
    while (itr != NULL) {
        if (SockAddr_Hostare_Equal(&itr->host, find)) {
#if HAVE_THREAD_DEBUG
	    active_table_show_compare("match host", itr, find, "client ip");
#endif
            break;
        } else {
#if HAVE_THREAD_DEBUG
	    active_table_show_compare("miss host", itr, find, "client ip");
#endif
	    itr = itr->next;
	}
    }
    return itr;
}
