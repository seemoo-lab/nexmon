/* MIT License
 *
 * Copyright (c) The c-ares project and its contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * SPDX-License-Identifier: MIT
 */
#include "ares-test.h"
#include "dns-proto.h"

#ifndef WIN32
#include <sys/types.h>
#include <sys/stat.h>
#endif

#ifndef _WIN32
#  include <errno.h>
#  include <fcntl.h>
#endif

#include <sstream>
#include <vector>

using testing::InvokeWithoutArgs;
using testing::DoAll;

namespace ares {
namespace test {

class NoDNS0x20MockTest
    : public MockChannelOptsTest,
      public ::testing::WithParamInterface<int> {
 public:
  NoDNS0x20MockTest()
    : MockChannelOptsTest(1, GetParam(), false, false,
                          FillOptions(&opts_),
                          ARES_OPT_FLAGS) {}
  static struct ares_options* FillOptions(struct ares_options * opts) {
    memset(opts, 0, sizeof(struct ares_options));
    opts->flags = ARES_FLAG_EDNS;
    return opts;
  }
 private:
  struct ares_options opts_;
};

static std::vector<byte> MakeMaxReadTcpAReply(void) {
  std::vector<byte> reply = {
    0x00, 0x00,  // qid
    0x84, 0x00,  // response + query + AA + noerror
    0x00, 0x01,  // qdcount
    0x00, 0x01,  // ancount
    0x00, 0x00,  // nscount
    0x00, 0x00,  // arcount
    // Question: www.google.com IN A
    0x03, 'w', 'w', 'w',
    0x06, 'g', 'o', 'o', 'g', 'l', 'e',
    0x03, 'c', 'o', 'm',
    0x00,
    0x00, 0x01,
    0x00, 0x01,
    // Answer: name ptr, IN A 1.2.3.4
    0xc0, 0x0c,
    0x00, 0x01,
    0x00, 0x01,
    0x00, 0x00, 0x00, 0x64,
    0x00, 0x04,
    0x01, 0x02, 0x03, 0x04
  };

  /* Keep payload at 65533 bytes so TCP frame length is exactly 65535,
   * forcing the read loop to attempt one more recv() where disconnect can
   * race with already-buffered data. */
  reply.resize(65533, 0x00);
  return reply;
}


TEST_P(NoDNS0x20MockTest, Basic) {
  std::vector<byte> reply = {
    0x00, 0x00,  // qid
    0x84, // response + query + AA + not-TC + not-RD
    0x00, // not-RA + not-Z + not-AD + not-CD + rc=NoError
    0x00, 0x01,  // 1 question
    0x00, 0x01,  // 1 answer RRs
    0x00, 0x00,  // 0 authority RRs
    0x00, 0x00,  // 0 additional RRs
    // Question
    0x03, 'w', 'w', 'w',
    0x06, 'g', 'o', 'o', 'g', 'l', 'e',
    0x03, 'c', 'o', 'm',
    0x00,
    0x00, 0x01,  // type A
    0x00, 0x01,  // class IN
    // Answer
    0x03, 'w', 'w', 'w',
    0x06, 'g', 'o', 'o', 'g', 'l', 'e',
    0x03, 'c', 'o', 'm',
    0x00,
    0x00, 0x01,  // type A
    0x00, 0x01,  // class IN
    0x00, 0x00, 0x01, 0x00,  // TTL
    0x00, 0x04,  // rdata length
    0x01, 0x02, 0x03, 0x04
  };

  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReplyData(&server_, reply));

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[1.2.3.4]}", ss.str());
}

TEST_P(MockUDPChannelTest, DNS0x20BadReply) {
  std::vector<byte> reply = {
    0x00, 0x00,  // qid
    0x84, // response + query + AA + not-TC + not-RD
    0x00, // not-RA + not-Z + not-AD + not-CD + rc=NoError
    0x00, 0x01,  // 1 question
    0x00, 0x01,  // 1 answer RRs
    0x00, 0x00,  // 0 authority RRs
    0x00, 0x00,  // 0 additional RRs
    // Question
    0x03, 'w', 'w', 'w',
    0x1D, 's', 'o', 'm', 'e', 'l', 'o', 'n', 'g', 'd', 'o', 'm', 'a', 'i', 'n', 'n', 'a', 'm', 'e', 'b', 'e', 'c', 'a', 'u', 's', 'e', 'p', 'r', 'n', 'g',
    0x03, 'c', 'o', 'm',
    0x00,
    0x00, 0x01,  // type A
    0x00, 0x01,  // class IN
    // Answer
    0x03, 'w', 'w', 'w',
    0x1D, 's', 'o', 'm', 'e', 'l', 'o', 'n', 'g', 'd', 'o', 'm', 'a', 'i', 'n', 'n', 'a', 'm', 'e', 'b', 'e', 'c', 'a', 'u', 's', 'e', 'p', 'r', 'n', 'g',
    0x03, 'c', 'o', 'm',
    0x00,
    0x00, 0x01,  // type A
    0x00, 0x01,  // class IN
    0x00, 0x00, 0x01, 0x00,  // TTL
    0x00, 0x04,  // rdata length
    0x01, 0x02, 0x03, 0x04
  };

  ON_CALL(server_, OnRequest("www.somelongdomainnamebecauseprng.com", T_A))
    .WillByDefault(SetReplyData(&server_, reply));

  /* Reply will be thrown out due to mismatched case for DNS 0x20 in response,
   * its technically possible this test case may not fail if somehow the
   * PRNG returns all lowercase domain name so we need to make this domain
   * fairly long to make sure those odds are very very very low */
  HostResult result;
  ares_gethostbyname(channel_, "www.somelongdomainnamebecauseprng.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ETIMEOUT, result.status_);
}

// UDP only so mock server doesn't get confused by concatenated requests
TEST_P(MockUDPChannelTest, GetHostByNameParallelLookups) {
  DNSPacket rsp1;
  rsp1.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp1));
  DNSPacket rsp2;
  rsp2.set_response().set_aa()
    .add_question(new DNSQuestion("www.example.com", T_A))
    .add_answer(new DNSARR("www.example.com", 100, {1, 2, 3, 4}));
  ON_CALL(server_, OnRequest("www.example.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp2));

  HostResult result1;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result1);
  HostResult result2;
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback, &result2);
  HostResult result3;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result3);
  Process();
  EXPECT_TRUE(result1.done_);
  EXPECT_TRUE(result2.done_);
  EXPECT_TRUE(result3.done_);
  std::stringstream ss1;
  ss1 << result1.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[2.3.4.5]}", ss1.str());
  std::stringstream ss2;
  ss2 << result2.host_;
  EXPECT_EQ("{'www.example.com' aliases=[] addrs=[1.2.3.4]}", ss2.str());
  std::stringstream ss3;
  ss3 << result3.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[2.3.4.5]}", ss3.str());
}

// UDP to TCP specific test
TEST_P(MockUDPChannelTest, TruncationRetry) {
  DNSPacket rsptruncated;
  rsptruncated.set_response().set_aa().set_tc()
    .add_question(new DNSQuestion("www.google.com", T_A));
  DNSPacket rspok;
  rspok.set_response()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {1, 2, 3, 4}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &rsptruncated))
    .WillOnce(SetReply(&server_, &rspok));
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[1.2.3.4]}", ss.str());
}

TEST_P(MockUDPChannelTest, UTF8BadName) {
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("españa.icom.museum", T_A))
    .add_answer(new DNSARR("españa.icom.museum", 100, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("españa.icom.museum", T_A))
    .WillByDefault(SetReply(&server_, &reply));

  HostResult result;
  ares_gethostbyname(channel_, "españa.icom.museum", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_EBADNAME, result.status_);
}

static int sock_cb_count = 0;
static int SocketConnectCallback(ares_socket_t fd, int type, void *data) {
  int rc = *(int*)data;
  (void)type;
  if (verbose) std::cerr << "SocketConnectCallback(" << fd << ") invoked" << std::endl;
  sock_cb_count++;
  return rc;
}

TEST_P(MockChannelTest, SockCallback) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &rsp));

  // Get notified of new sockets
  int rc = ARES_SUCCESS;
  ares_set_socket_callback(channel_, SocketConnectCallback, &rc);

  HostResult result;
  sock_cb_count = 0;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_EQ(1, sock_cb_count);
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
}

TEST_P(MockChannelTest, SockFailCallback) {
  // Notification of new sockets gives an error.
  int rc = -1;
  ares_set_socket_callback(channel_, SocketConnectCallback, &rc);

  HostResult result;
  sock_cb_count = 0;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_LT(1, sock_cb_count);
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ECONNREFUSED, result.status_);
}

static int sock_config_cb_count = 0;
static int SocketConfigureCallback(ares_socket_t fd, int type, void *data) {
  int rc = *(int*)data;
  (void)type;
  if (verbose) std::cerr << "SocketConfigureCallback(" << fd << ") invoked" << std::endl;
  sock_config_cb_count++;
  return rc;
}

TEST_P(MockChannelTest, SockConfigureCallback) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &rsp));

  // Get notified of new sockets
  int rc = ARES_SUCCESS;
  ares_set_socket_configure_callback(channel_, SocketConfigureCallback, &rc);

  HostResult result;
  sock_config_cb_count = 0;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_EQ(1, sock_config_cb_count);
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
}

TEST_P(MockChannelTest, SockConfigureFailCallback) {
  // Notification of new sockets gives an error.
  int rc = -1;
  ares_set_socket_configure_callback(channel_, SocketConfigureCallback, &rc);

  HostResult result;
  sock_config_cb_count = 0;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_LT(1, sock_config_cb_count);
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ECONNREFUSED, result.status_);
}

// Define a server state callback for testing. The custom userdata should be
// the expected server string that the callback is invoked with.
static int server_state_cb_success_count = 0;
static int server_state_cb_failure_count = 0;
static void ServerStateCallback(const char *server_string,
                                ares_bool_t success, int flags, void *data) {
  // Increment overall success/failure counts appropriately.
  if (verbose) std::cerr << "ServerStateCallback("
                         << server_string << ", "
                         << success       << ", "
                         << flags         << ") invoked" << std::endl;
  if (success == ARES_TRUE) server_state_cb_success_count++;
  else server_state_cb_failure_count++;

  // Check that the server string is as expected.
  char *exp_server_string = *(char **)(data);
  EXPECT_STREQ(exp_server_string, server_string);

  // The callback should be invoked with either the UDP flag or the TCP flag,
  // but not both.
  ares_bool_t udp = (flags & ARES_SERV_STATE_UDP) ? ARES_TRUE: ARES_FALSE;
  ares_bool_t tcp = (flags & ARES_SERV_STATE_TCP) ? ARES_TRUE: ARES_FALSE;
  EXPECT_NE(udp, tcp);
}

TEST_P(MockChannelTest, ServStateCallbackSuccess) {
  // Set up the server response. The server returns successfully with an answer
  // to the query.
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &rsp));

  // Set up the server state callback. The channel used for this test has a
  // single server configured.
  char *exp_server_string = ares_get_servers_csv(channel_);
  ares_set_server_state_callback(channel_, ServerStateCallback,
                                 &exp_server_string);

  // Perform the hostname lookup. Expect 1 successful query to the server.
  HostResult result;
  server_state_cb_success_count = 0;
  server_state_cb_failure_count = 0;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback,
                     &result);
  Process();
  EXPECT_EQ(1, server_state_cb_success_count);
  EXPECT_EQ(0, server_state_cb_failure_count);
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[2.3.4.5]}", ss.str());

  ares_free_string(exp_server_string);
}

TEST_P(MockChannelTest, ServStateCallbackFailure) {
  // Set up the server response. The server always returns SERVFAIL.
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A));
  rsp.set_rcode(SERVFAIL);
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));

  // Set up the server state callback. The channel used for this test has a
  // single server configured.
  char *exp_server_string = ares_get_servers_csv(channel_);
  ares_set_server_state_callback(channel_, ServerStateCallback,
                                 &exp_server_string);

  // Perform the hostname lookup. Expect 3 failed queries to the server (due to
  // retries).
  HostResult result;
  server_state_cb_success_count = 0;
  server_state_cb_failure_count = 0;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback,
                     &result);
  Process();
  EXPECT_EQ(0, server_state_cb_success_count);
  EXPECT_EQ(3, server_state_cb_failure_count);
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ESERVFAIL, result.status_);

  ares_free_string(exp_server_string);
}

TEST_P(MockChannelTest, ServStateCallbackRecover) {
  // Set up the server response. The server initially times out, but then
  // returns successfully (with NXDOMAIN) on the first retry.
  std::vector<byte> nothing;
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A));
  rsp.set_rcode(NXDOMAIN);
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReplyData(&server_, nothing))
    .WillOnce(SetReply(&server_, &rsp));

  // Set up the server state callback. The channel used for this test has a
  // single server configured.
  char *exp_server_string = ares_get_servers_csv(channel_);
  ares_set_server_state_callback(channel_, ServerStateCallback,
                                 &exp_server_string);

  // Perform the hostname lookup. Expect 1 failed query and 1 successful query
  // to the server.
  HostResult result;
  server_state_cb_success_count = 0;
  server_state_cb_failure_count = 0;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback,
                     &result);
  Process();
  EXPECT_EQ(1, server_state_cb_success_count);
  EXPECT_EQ(1, server_state_cb_failure_count);
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENOTFOUND, result.status_);

  ares_free_string(exp_server_string);
}

TEST_P(MockChannelTest, ReInit) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &rsp));

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  EXPECT_EQ(ARES_SUCCESS, ares_reinit(channel_));
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
}

#define MAXUDPQUERIES_TOTAL 32
#define MAXUDPQUERIES_LIMIT 8

class MockUDPMaxQueriesTest
    : public MockChannelOptsTest,
      public ::testing::WithParamInterface<int> {
 public:
  MockUDPMaxQueriesTest()
    : MockChannelOptsTest(1, GetParam(), false, false,
                          FillOptions(&opts_),
                          ARES_OPT_UDP_MAX_QUERIES) {}
  static struct ares_options* FillOptions(struct ares_options * opts) {
    memset(opts, 0, sizeof(struct ares_options));
    opts->udp_max_queries = MAXUDPQUERIES_LIMIT;
    return opts;
  }
 private:
  struct ares_options opts_;
};

TEST_P(MockUDPMaxQueriesTest, GetHostByNameParallelLookups) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));

  // Get notified of new sockets so we can validate how many are created
  int rc = ARES_SUCCESS;
  ares_set_socket_callback(channel_, SocketConnectCallback, &rc);
  sock_cb_count = 0;

  HostResult result[MAXUDPQUERIES_TOTAL];
  for (size_t i=0; i<MAXUDPQUERIES_TOTAL; i++) {
    ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result[i]);
  }

  Process();

  EXPECT_EQ(MAXUDPQUERIES_TOTAL / MAXUDPQUERIES_LIMIT, sock_cb_count);

  for (size_t i=0; i<MAXUDPQUERIES_TOTAL; i++) {
    std::stringstream ss;
    EXPECT_TRUE(result[i].done_);
    ss << result[i].host_;
    EXPECT_EQ("{'www.google.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
  }
}

// Regression test for #1152.  A transient failure (a single query timeout)
// must not force a brand new UDP socket to be opened for every subsequent
// query to the same server.  The connection that saw the timeout is retired
// for new queries (ARES_CONN_FLAG_NONEW) and drains, while new queries share
// one fresh connection.  Before the fix, ares_fetch_connection() rejected
// reuse whenever the server's consec_failures was > 0, so a single dropped
// packet caused a new socket per query and exhausted sockets/ports under load.
class MockUDPTransientFailureTest
    : public MockChannelOptsTest,
      public ::testing::WithParamInterface<int> {
 public:
  MockUDPTransientFailureTest()
    : MockChannelOptsTest(1, GetParam(), false, false,
                          FillOptions(&opts_),
                          ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES) {}
  static struct ares_options* FillOptions(struct ares_options * opts) {
    memset(opts, 0, sizeof(struct ares_options));
    opts->timeout = 150; /* ms (ARES_OPT_TIMEOUTMS) - keep the test quick */
    opts->tries   = 3;
    return opts;
  }
 private:
  struct ares_options opts_;
};

TEST_P(MockUDPTransientFailureTest, NoSocketExplosionOnTimeout) {
  // Configure the server to never reply, so every query times out (a
  // transient failure).  No SetReply() means ProcessRequest() drops the
  // packet; the AnyNumber() expectation just silences uninteresting-call
  // warnings for the many dropped requests.
  EXPECT_CALL(server_, OnRequest("www.example.com", T_A))
    .Times(testing::AnyNumber());

  int rc = ARES_SUCCESS;
  ares_set_socket_callback(channel_, SocketConnectCallback, &rc);
  sock_cb_count = 0;

  const size_t nqueries = 16;
  HostResult result[nqueries];
  for (size_t i = 0; i < nqueries; i++) {
    ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback,
                       &result[i]);
  }
  Process();

  // All queries complete (they fail with a timeout since the server never
  // replied).
  for (size_t i = 0; i < nqueries; i++) {
    EXPECT_TRUE(result[i].done_);
  }

  // The key assertion: with tries=3 the fix opens roughly one connection per
  // retry round (the retired connection is skipped and the single replacement
  // is reused by all queries), not one per query.  Before the fix this was on
  // the order of nqueries*tries sockets.
  EXPECT_LE(sock_cb_count, 6)
    << "opened " << sock_cb_count << " sockets for " << nqueries
    << " queries; a transient timeout must not force a new socket per query";
}

INSTANTIATE_TEST_SUITE_P(AddressFamilies, MockUDPTransientFailureTest,
                         ::testing::ValuesIn(ares::test::families), PrintFamily);

class CacheQueriesTest
    : public MockChannelOptsTest,
      public ::testing::WithParamInterface<int> {
 public:
  CacheQueriesTest()
    : MockChannelOptsTest(1, GetParam(), false, false,
                          FillOptions(&opts_),
                          ARES_OPT_QUERY_CACHE) {}
  static struct ares_options* FillOptions(struct ares_options * opts) {
    memset(opts, 0, sizeof(struct ares_options));
    opts->qcache_max_ttl = 3600;
    return opts;
  }
 private:
  struct ares_options opts_;
};

// A cache hit used to hand the callback a pointer to the cache's own record.
// That is fine only while the callback does nothing else -- but re-entering
// c-ares from a callback is documented as allowed, and two ordinary things a
// callback might do destroy the entry underneath it: changing the server list
// flushes the whole cache, and starting another lookup runs the expiry reaper
// first.  The record must therefore be the caller's own copy.
//
// Here the callback changes the server list, which flushes the cache, and then
// reads the record it was handed.
struct QcacheReentrantCbData {
  ares_channel_t *channel;
  bool            done;
  size_t          ancount;
};

static void QcacheReentrantCallback(void *arg, ares_status_t status,
                                    size_t timeouts,
                                    const ares_dns_record_t *dnsrec) {
  QcacheReentrantCbData *data = static_cast<QcacheReentrantCbData *>(arg);
  (void)timeouts;
  data->done = true;
  if (status != ARES_SUCCESS || dnsrec == NULL) {
    return;
  }
  /* Documented re-entry.  Reconfiguring servers flushes the query cache. */
  EXPECT_EQ(ARES_SUCCESS,
            ares_set_servers_csv(data->channel, "127.0.0.1:53055"));
  /* Still inside the callback, so this record must still be readable. */
  data->ancount = ares_dns_record_rr_cnt(dnsrec, ARES_SECTION_ANSWER);
}

TEST_P(CacheQueriesTest, ReentrantFlushInCallbackNoUseAfterFree) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));

  /* Populate the cache. */
  QueryResult first;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    QueryCallback, &first, NULL);
  Process();
  EXPECT_TRUE(first.done_);

  /* Served from the cache, so the callback runs synchronously holding the
   * record the cache handed it. */
  QcacheReentrantCbData data;
  data.channel = channel_;
  data.done    = false;
  data.ancount = 0;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    QcacheReentrantCallback, &data, NULL);

  EXPECT_TRUE(data.done);
  EXPECT_EQ((size_t)1, data.ancount);
}

// The cache key covered the question and a couple of header flags but ignored
// the request's OPT record entirely, so two requests differing only in their
// EDNS options collided.  An answer obtained under one set of options is not
// interchangeable with one obtained under another -- most obviously, a request
// with the DO bit set must not be handed an answer fetched without it.
//
// Both requests here ask the identical question; only the EDNS state differs,
// so the server must see two of them.
TEST_P(CacheQueriesTest, EdnsOptionsSeparateCacheEntries) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .Times(2)
    .WillRepeatedly(SetReply(&server_, &rsp));

  /* First: no OPT record at all. */
  ares_dns_record_t *req1 = nullptr;
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_record_create(&req1, 0, ARES_FLAG_RD, ARES_OPCODE_QUERY,
                                   ARES_RCODE_NOERROR));
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_record_query_add(req1, "www.google.com", ARES_REC_TYPE_A,
                                      ARES_CLASS_IN));
  QueryResult r1;
  EXPECT_EQ(ARES_SUCCESS,
            ares_send_dnsrec(channel_, req1, QueryCallback, &r1, nullptr));
  Process();
  EXPECT_TRUE(r1.done_);
  EXPECT_EQ(ARES_SUCCESS, r1.status_);

  /* Second: same question, but carrying EDNS with the DO bit set. */
  ares_dns_record_t *req2 = nullptr;
  ares_dns_rr_t     *opt  = nullptr;
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_record_create(&req2, 0, ARES_FLAG_RD, ARES_OPCODE_QUERY,
                                   ARES_RCODE_NOERROR));
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_record_query_add(req2, "www.google.com", ARES_REC_TYPE_A,
                                      ARES_CLASS_IN));
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_record_rr_add(&opt, req2, ARES_SECTION_ADDITIONAL, "",
                                   ARES_REC_TYPE_OPT, ARES_CLASS_IN, 0));
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_rr_set_u16(opt, ARES_RR_OPT_UDP_SIZE, 1232));
  EXPECT_EQ(ARES_SUCCESS, ares_dns_rr_set_u8(opt, ARES_RR_OPT_VERSION, 0));
  /* DO */
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_rr_set_u16(opt, ARES_RR_OPT_FLAGS, 0x8000));

  QueryResult r2;
  EXPECT_EQ(ARES_SUCCESS,
            ares_send_dnsrec(channel_, req2, QueryCallback, &r2, nullptr));
  Process();
  EXPECT_TRUE(r2.done_);
  EXPECT_EQ(ARES_SUCCESS, r2.status_);

  ares_dns_record_destroy(req1);
  ares_dns_record_destroy(req2);
}

// The test above sets the DO bit, which lives in the OPT flags -- it never
// reaches the loop that keys the individual EDNS options.  That loop is what
// separates two client subnets, so it needs its own coverage.
// The mirror image of the tests above: two things are deliberately left out
// of the cache key, and leaving them out is a security-relevant judgement --
// it declares that two requests differing only in these still have
// interchangeable answers.  Without a test, keying on either one later would
// turn every request from an affected application into a permanent cache miss
// and nothing would fail.

// A DNS cookie is per-server transport state and says nothing about the
// content of an answer, so two requests differing only in their cookie share
// an entry.  The server must therefore be asked exactly once.
// Two query types with no mnemonic in ares_dns_rec_type_tostr() both render as
// "UNKNOWN", so keying the mnemonic made every such type share one entry -- a
// lookup answered with a record of a type it never asked for.  Any 16-bit
// value reaches here through ares_query_dnsrec(); LOC (29) and DNAME (39) are
// both real and both outside the table.
TEST_P(CacheQueriesTest, UnknownQueryTypesDoNotShareCacheEntries) {
  DNSPacket rsp29;
  rsp29.set_response().set_aa().add_question(
    new DNSQuestion("example.net", 29));
  DNSPacket rsp39;
  rsp39.set_response().set_aa().add_question(
    new DNSQuestion("example.net", 39));

  // One call per type: neither may be served the other's answer.
  EXPECT_CALL(server_, OnRequest("example.net", 29))
    .Times(1)
    .WillRepeatedly(SetReply(&server_, &rsp29));
  EXPECT_CALL(server_, OnRequest("example.net", 39))
    .Times(1)
    .WillRepeatedly(SetReply(&server_, &rsp39));

  QueryResult r1;
  ares_query_dnsrec(channel_, "example.net", ARES_CLASS_IN,
                    static_cast<ares_dns_rec_type_t>(29), QueryCallback, &r1,
                    nullptr);
  Process();
  EXPECT_TRUE(r1.done_);

  QueryResult r2;
  ares_query_dnsrec(channel_, "example.net", ARES_CLASS_IN,
                    static_cast<ares_dns_rec_type_t>(39), QueryCallback, &r2,
                    nullptr);
  Process();
  EXPECT_TRUE(r2.done_);
}

// Handing back a duplicate materialises the pending TTL decrement, so a
// callback that reads the TTL off the record it is given sees the remaining
// lifetime rather than the value the server sent.  docs/ares_init_options.3
// states this, so pin it.  Only visible to a caller that does not duplicate
// first -- QueryResult does, which is why the other cache tests cannot see it.
static unsigned int g_qcache_hit_ttl = 0;

static void QcacheTtlCallback(void *arg, ares_status_t status, size_t timeouts,
                              const ares_dns_record_t *dnsrec) {
  const ares_dns_rr_t *rr;
  (void)arg;
  (void)timeouts;
  if (status != ARES_SUCCESS || dnsrec == NULL) {
    return;
  }
  rr = ares_dns_record_rr_get_const(dnsrec, ARES_SECTION_ANSWER, 0);
  g_qcache_hit_ttl = rr ? ares_dns_rr_get_ttl(rr) : 0;
}

TEST_P(CacheQueriesTest, CacheHitReportsRemainingTtl) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .Times(1)
    .WillRepeatedly(SetReply(&server_, &rsp));

  g_qcache_hit_ttl = 0;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    QcacheTtlCallback, nullptr, nullptr);
  Process();
  EXPECT_EQ((unsigned int)100, g_qcache_hit_ttl);

  ares_sleep_time(2100);

  g_qcache_hit_ttl = 0;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    QcacheTtlCallback, nullptr, nullptr);
  Process();
  // Served from the cache, and reporting what is left rather than the
  // original 100.
  EXPECT_GT(g_qcache_hit_ttl, (unsigned int)0);
  EXPECT_LT(g_qcache_hit_ttl, (unsigned int)100);
}

TEST_P(CacheQueriesTest, CookieDoesNotSplitCacheEntries) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .Times(1)
    .WillRepeatedly(SetReply(&server_, &rsp));

  const unsigned char  ck1[]   = {1, 2, 3, 4, 5, 6, 7, 8};
  const unsigned char  ck2[]   = {8, 7, 6, 5, 4, 3, 2, 1};
  const unsigned char *ck[2]   = {ck1, ck2};
  QueryResult          r[2];

  for (int i = 0; i < 2; i++) {
    ares_dns_record_t *req = nullptr;
    ares_dns_rr_t     *opt = nullptr;
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_record_create(&req, 0, ARES_FLAG_RD, ARES_OPCODE_QUERY,
                                     ARES_RCODE_NOERROR));
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_record_query_add(req, "www.google.com", ARES_REC_TYPE_A,
                                        ARES_CLASS_IN));
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_record_rr_add(&opt, req, ARES_SECTION_ADDITIONAL, "",
                                     ARES_REC_TYPE_OPT, ARES_CLASS_IN, 0));
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_rr_set_u16(opt, ARES_RR_OPT_UDP_SIZE, 1232));
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_rr_set_opt(opt, ARES_RR_OPT_OPTIONS,
                                  ARES_OPT_PARAM_COOKIE, ck[i], sizeof(ck1)));
    EXPECT_EQ(ARES_SUCCESS,
              ares_send_dnsrec(channel_, req, QueryCallback, &r[i], nullptr));
    Process();
    EXPECT_TRUE(r[i].done_);
    EXPECT_EQ(ARES_SUCCESS, r[i].status_);
    ares_dns_record_destroy(req);
  }
}

// The advertised UDP payload size constrains how a response is transported,
// not what it says, so it is excluded too.  Same expectation: one request.
TEST_P(CacheQueriesTest, UdpPayloadSizeDoesNotSplitCacheEntries) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .Times(1)
    .WillRepeatedly(SetReply(&server_, &rsp));

  const unsigned short sizes[2] = {512, 1232};
  QueryResult          r[2];

  for (int i = 0; i < 2; i++) {
    ares_dns_record_t *req = nullptr;
    ares_dns_rr_t     *opt = nullptr;
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_record_create(&req, 0, ARES_FLAG_RD, ARES_OPCODE_QUERY,
                                     ARES_RCODE_NOERROR));
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_record_query_add(req, "www.google.com", ARES_REC_TYPE_A,
                                        ARES_CLASS_IN));
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_record_rr_add(&opt, req, ARES_SECTION_ADDITIONAL, "",
                                     ARES_REC_TYPE_OPT, ARES_CLASS_IN, 0));
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_rr_set_u16(opt, ARES_RR_OPT_UDP_SIZE, sizes[i]));
    EXPECT_EQ(ARES_SUCCESS,
              ares_send_dnsrec(channel_, req, QueryCallback, &r[i], nullptr));
    Process();
    EXPECT_TRUE(r[i].done_);
    EXPECT_EQ(ARES_SUCCESS, r[i].status_);
    ares_dns_record_destroy(req);
  }
}

TEST_P(CacheQueriesTest, EdnsClientSubnetSeparateCacheEntries) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .Times(2)
    .WillRepeatedly(SetReply(&server_, &rsp));

  /* 192.0.2.0/24 and 198.51.100.0/24 */
  const unsigned char  ecs1[]  = {0x00, 0x01, 0x18, 0x00, 0xc0, 0x00, 0x02};
  const unsigned char  ecs2[]  = {0x00, 0x01, 0x18, 0x00, 0xc6, 0x33, 0x64};
  const unsigned char *ecs[2]  = {ecs1, ecs2};
  QueryResult          r[2];

  for (int i = 0; i < 2; i++) {
    ares_dns_record_t *req = nullptr;
    ares_dns_rr_t     *opt = nullptr;
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_record_create(&req, 0, ARES_FLAG_RD, ARES_OPCODE_QUERY,
                                     ARES_RCODE_NOERROR));
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_record_query_add(req, "www.google.com", ARES_REC_TYPE_A,
                                        ARES_CLASS_IN));
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_record_rr_add(&opt, req, ARES_SECTION_ADDITIONAL, "",
                                     ARES_REC_TYPE_OPT, ARES_CLASS_IN, 0));
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_rr_set_u16(opt, ARES_RR_OPT_UDP_SIZE, 1232));
    EXPECT_EQ(ARES_SUCCESS,
              ares_dns_rr_set_opt(opt, ARES_RR_OPT_OPTIONS,
                                  ARES_OPT_PARAM_EDNS_CLIENT_SUBNET, ecs[i],
                                  sizeof(ecs1)));
    EXPECT_EQ(ARES_SUCCESS,
              ares_send_dnsrec(channel_, req, QueryCallback, &r[i], nullptr));
    Process();
    EXPECT_TRUE(r[i].done_);
    EXPECT_EQ(ARES_SUCCESS, r[i].status_);
    ares_dns_record_destroy(req);
  }
}

// A server answering an EDNS query with FORMERR makes c-ares strip the OPT
// record from query->query and retry.  The cache key must not be derived from
// that record afterwards, or the entry is stored under a key no later lookup
// computes and the cache silently stops working for that name.
TEST_P(CacheQueriesTest, EdnsFallbackStillCaches) {
  DNSPacket rspfail;
  rspfail.set_response().set_aa().set_rcode(FORMERR)
    .add_question(new DNSQuestion("www.google.com", T_A));
  DNSPacket rspok;
  rspok.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {1, 2, 3, 4}));
  /* Two: the FORMERR and the retry.  The second lookup must be a cache hit. */
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .Times(2)
    .WillOnce(SetReply(&server_, &rspfail))
    .WillRepeatedly(SetReply(&server_, &rspok));

  HostResult r1;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &r1);
  Process();
  EXPECT_TRUE(r1.done_);
  EXPECT_EQ(ARES_SUCCESS, r1.status_);

  HostResult r2;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &r2);
  Process();
  EXPECT_TRUE(r2.done_);
  EXPECT_EQ(ARES_SUCCESS, r2.status_);
}

// The question name is caller-supplied and unescaped, so anything keyed after
// it could be imitated by a name containing that same text.  A request for
// "example.net|EDNS|0|8000" with no OPT record must not collide with a request
// for "example.net" carrying EDNS with the DNSSEC OK bit set.
TEST_P(CacheQueriesTest, EdnsSectionCannotBeImitatedByName) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("example.net", T_A))
    .add_answer(new DNSARR("example.net", 100, {5, 6, 7, 8}));
  ON_CALL(server_, OnRequest("example.net", T_A))
    .WillByDefault(SetReply(&server_, &rsp));

  /* Cache an answer for the real name, with EDNS and the DO bit set. */
  ares_dns_record_t *req = nullptr;
  ares_dns_rr_t     *opt = nullptr;
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_record_create(&req, 0, ARES_FLAG_RD, ARES_OPCODE_QUERY,
                                   ARES_RCODE_NOERROR));
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_record_query_add(req, "example.net", ARES_REC_TYPE_A,
                                      ARES_CLASS_IN));
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_record_rr_add(&opt, req, ARES_SECTION_ADDITIONAL, "",
                                   ARES_REC_TYPE_OPT, ARES_CLASS_IN, 0));
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_rr_set_u16(opt, ARES_RR_OPT_UDP_SIZE, 1232));
  EXPECT_EQ(ARES_SUCCESS, ares_dns_rr_set_u16(opt, ARES_RR_OPT_FLAGS, 0x8000));
  QueryResult r1;
  EXPECT_EQ(ARES_SUCCESS,
            ares_send_dnsrec(channel_, req, QueryCallback, &r1, nullptr));
  Process();
  EXPECT_TRUE(r1.done_);
  EXPECT_EQ(ARES_SUCCESS, r1.status_);
  ares_dns_record_destroy(req);

  /* A different name that spells out the EDNS suffix, with no OPT record.
   * It must not be served the entry above; with the cache keyed correctly it
   * is refused, because '|' is not valid in a hostname. */
  ares_dns_record_t *req2 = nullptr;
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_record_create(&req2, 0, ARES_FLAG_RD, ARES_OPCODE_QUERY,
                                   ARES_RCODE_NOERROR));
  EXPECT_EQ(ARES_SUCCESS,
            ares_dns_record_query_add(req2, "example.net|EDNS|0|8000",
                                      ARES_REC_TYPE_A, ARES_CLASS_IN));
  QueryResult r2;
  ares_send_dnsrec(channel_, req2, QueryCallback, &r2, nullptr);
  Process();
  EXPECT_TRUE(r2.done_);
  EXPECT_NE(ARES_SUCCESS, r2.status_);
  ares_dns_record_destroy(req2);
}

TEST_P(CacheQueriesTest, GetHostByNameCache) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));

  // Get notified of new sockets so we can validate how many are created
  int rc = ARES_SUCCESS;
  ares_set_socket_callback(channel_, SocketConnectCallback, &rc);
  sock_cb_count = 0;

  HostResult result1;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result1);
  Process();

  std::stringstream ss1;
  EXPECT_TRUE(result1.done_);
  ss1 << result1.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[2.3.4.5]}", ss1.str());

  /* Run again, should return cached result */
  HostResult result2;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result2);
  Process();

  std::stringstream ss2;
  EXPECT_TRUE(result2.done_);
  ss2 << result2.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[2.3.4.5]}", ss2.str());

  EXPECT_EQ(1, sock_cb_count);
}

#define TCPPARALLELLOOKUPS 32
TEST_P(MockTCPChannelTest, GetHostByNameParallelLookups) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));

  // Get notified of new sockets so we can validate how many are created
  int rc = ARES_SUCCESS;
  ares_set_socket_callback(channel_, SocketConnectCallback, &rc);
  sock_cb_count = 0;

  HostResult result[TCPPARALLELLOOKUPS];
  for (size_t i=0; i<TCPPARALLELLOOKUPS; i++) {
    ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result[i]);
  }

  Process();

  EXPECT_EQ(1, sock_cb_count);

  for (size_t i=0; i<TCPPARALLELLOOKUPS; i++) {
    std::stringstream ss;
    EXPECT_TRUE(result[i].done_);
    ss << result[i].host_;
    EXPECT_EQ("{'www.google.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
  }
}

TEST_P(MockTCPChannelTest, ReadFullFrameThenDisconnect) {
  std::vector<byte> reply = MakeMaxReadTcpAReply();

  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(DoAll(SetReplyData(&server_, reply),
                    DisconnectAfterReply(&server_)));

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback,
                     &result);
  Process();

  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_EQ(0, result.timeouts_);

  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[1.2.3.4]}", ss.str());
}

TEST_P(MockTCPChannelTest, MalformedResponse) {
  std::vector<byte> one = {0x00};
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReplyData(&server_, one));

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_EBADRESP, result.status_);
}

TEST_P(MockTCPChannelTest, FormErrResponse) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A));
  rsp.set_rcode(FORMERR);
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &rsp));
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_EFORMERR, result.status_);
}

TEST_P(MockTCPChannelTest, ServFailResponse) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A));
  rsp.set_rcode(SERVFAIL);
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ESERVFAIL, result.status_);
}

TEST_P(MockTCPChannelTest, NotImplResponse) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A));
  rsp.set_rcode(NOTIMP);
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENOTIMP, result.status_);
}

TEST_P(MockTCPChannelTest, RefusedResponse) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A));
  rsp.set_rcode(REFUSED);
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_EREFUSED, result.status_);
}

TEST_P(MockTCPChannelTest, YXDomainResponse) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A));
  rsp.set_rcode(YXDOMAIN);
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &rsp));
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENODATA, result.status_);
}

class MockExtraOptsTest
    : public MockChannelOptsTest,
      public ::testing::WithParamInterface< std::pair<int, bool> > {
 public:
  MockExtraOptsTest()
    : MockChannelOptsTest(1, GetParam().first, GetParam().second, false,
                          FillOptions(&opts_),
                          ARES_OPT_SOCK_SNDBUF|ARES_OPT_SOCK_RCVBUF) {}
  static struct ares_options* FillOptions(struct ares_options * opts) {
    memset(opts, 0, sizeof(struct ares_options));
    // Set a few options that affect socket communications
    opts->socket_send_buffer_size = 514;
    opts->socket_receive_buffer_size = 514;
    return opts;
  }
 private:
  struct ares_options opts_;
};

TEST_P(MockExtraOptsTest, SimpleQuery) {
  ares_set_local_ip4(channel_, 0x7F000001);
  byte addr6[16] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01};
  ares_set_local_ip6(channel_, addr6);
  ares_set_local_dev(channel_, "dummy");

  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
}

class MockFlagsChannelOptsTest
    : public MockChannelOptsTest,
      public ::testing::WithParamInterface< std::pair<int, bool> > {
 public:
  MockFlagsChannelOptsTest(int flags)
    : MockChannelOptsTest(1, GetParam().first, GetParam().second, false,
                          FillOptions(&opts_, flags), ARES_OPT_FLAGS) {}
  static struct ares_options* FillOptions(struct ares_options * opts, int flags) {
    memset(opts, 0, sizeof(struct ares_options));
    opts->flags = flags;
    return opts;
  }
 private:
  struct ares_options opts_;
};

class MockNoCheckRespChannelTest : public MockFlagsChannelOptsTest {
 public:
  MockNoCheckRespChannelTest() : MockFlagsChannelOptsTest(ARES_FLAG_NOCHECKRESP) {}
};

TEST_P(MockNoCheckRespChannelTest, ServFailResponse) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A));
  rsp.set_rcode(SERVFAIL);
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ESERVFAIL, result.status_);
}

TEST_P(MockNoCheckRespChannelTest, NotImplResponse) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A));
  rsp.set_rcode(NOTIMP);
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENOTIMP, result.status_);
}

TEST_P(MockNoCheckRespChannelTest, RefusedResponse) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A));
  rsp.set_rcode(REFUSED);
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_EREFUSED, result.status_);
}

class MockEDNSChannelTest : public MockFlagsChannelOptsTest {
 public:
  MockEDNSChannelTest() : MockFlagsChannelOptsTest(ARES_FLAG_EDNS) {}
};

TEST_P(MockEDNSChannelTest, RetryWithoutEDNS) {
  DNSPacket rspfail;
  rspfail.set_response().set_aa().set_rcode(FORMERR)
    .add_question(new DNSQuestion("www.google.com", T_A));
  DNSPacket rspok;
  rspok.set_response()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {1, 2, 3, 4}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &rspfail))
    .WillOnce(SetReply(&server_, &rspok));
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[1.2.3.4]}", ss.str());
}


// Issue #911
TEST_P(MockUDPChannelTest, RetryWithoutEDNSNonCompliant) {
  DNSPacket rspfail;
  rspfail.set_response().set_aa().set_rcode(FORMERR)
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, { }, { }, false));
  DNSPacket rspok;
  rspok.set_response()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {1, 2, 3, 4}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &rspfail))
    .WillOnce(SetReply(&server_, &rspok));
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[1.2.3.4]}", ss.str());
}

TEST_P(MockChannelTest, SearchDomains) {
  DNSPacket nofirst;
  nofirst.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("www.first.com", T_A));
  ON_CALL(server_, OnRequest("www.first.com", T_A))
    .WillByDefault(SetReply(&server_, &nofirst));
  DNSPacket nosecond;
  nosecond.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("www.second.org", T_A));
  ON_CALL(server_, OnRequest("www.second.org", T_A))
    .WillByDefault(SetReply(&server_, &nosecond));
  DNSPacket yesthird;
  yesthird.set_response().set_aa()
    .add_question(new DNSQuestion("www.third.gov", T_A))
    .add_answer(new DNSARR("www.third.gov", 0x0200, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www.third.gov", T_A))
    .WillByDefault(SetReply(&server_, &yesthird));

  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.third.gov' aliases=[] addrs=[2.3.4.5]}", ss.str());
}

#ifdef HAVE_CONTAINER
// Issue #852
class ContainedMockChannelSysConfig
    : public MockChannelOptsTest,
      public ::testing::WithParamInterface<std::pair<int, bool>> {
 public:
  ContainedMockChannelSysConfig()
    : MockChannelOptsTest(1, GetParam().first, GetParam().second, true, nullptr, 0) {}
};

static NameContentList files_no_ndots = {
  {"/etc/resolv.conf", "nameserver 1.2.3.4\n" // Will be replaced
                       "search example.com example.org\n"
                       "options edns0 trust-ad\n"}, // ndots:1 is default
  {"/etc/hosts", "3.4.5.6 ahostname.com\n"},
  {"/etc/nsswitch.conf", "hosts: files dns\n"}};
CONTAINED_TEST_P(ContainedMockChannelSysConfig, SysConfigNdotsDefault,
                 "myhostname", "mydomainname.org", files_no_ndots) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.example.com", T_A))
    .add_answer(new DNSARR("www.example.com", 0x0200, {2, 3, 4, 5}));
  EXPECT_CALL(server_, OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(&server_, &rsp));

  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.example.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
  return HasFailure();
}


static NameContentList files_ndots0 = {
  {"/etc/resolv.conf", "nameserver 1.2.3.4\n" // Will be replaced
                       "search example.com example.org\n"
                       "options edns0 trust-ad ndots:0\n"}, // ndots:1 is default
  {"/etc/hosts", "3.4.5.6 ahostname.com\n"},
  {"/etc/nsswitch.conf", "hosts: files dns\n"}};
CONTAINED_TEST_P(ContainedMockChannelSysConfig, SysConfigNdots0,
                 "myhostname", "mydomainname.org", files_ndots0) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www", T_A))
    .add_answer(new DNSARR("www", 0x0200, {1, 2, 3, 4}));
  EXPECT_CALL(server_, OnRequest("www", T_A))
    .WillOnce(SetReply(&server_, &rsp));

  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www' aliases=[] addrs=[1.2.3.4]}", ss.str());
  return HasFailure();
}
#endif

// Issue #858
TEST_P(CacheQueriesTest, BlankName) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion(".", T_SOA))
    .add_answer(new DNSSoaRR(".", 600, "a.root-servers.net", "nstld.verisign-grs.com", 123456, 3600, 3600, 3600, 3600));
  EXPECT_CALL(server_, OnRequest("", T_SOA))
    .WillOnce(SetReply(&server_, &rsp));

  QueryResult result;
  ares_query_dnsrec(channel_, ".", ARES_CLASS_IN, ARES_REC_TYPE_SOA, QueryCallback, &result, NULL);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(0, result.timeouts_);

  QueryResult cacheresult;
  ares_query_dnsrec(channel_, ".", ARES_CLASS_IN, ARES_REC_TYPE_SOA, QueryCallback, &cacheresult, NULL);
  Process();
  EXPECT_TRUE(cacheresult.done_);
  EXPECT_EQ(0, cacheresult.timeouts_);
}

TEST_P(CacheQueriesTest, SearchDomainsCache) {
  DNSPacket nofirst;
  nofirst.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("www.first.com", T_A))
    .add_auth(new DNSSoaRR("first.com", 600, "ns1.first.com", "admin.first.com", 123456, 3600, 3600, 3600, 3600));
  EXPECT_CALL(server_, OnRequest("www.first.com", T_A))
    .WillOnce(SetReply(&server_, &nofirst));
  DNSPacket nosecond;
  nosecond.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("www.second.org", T_A))
    .add_auth(new DNSSoaRR("second.org", 600, "ns1.second.org", "admin.second.org", 123456, 3600, 3600, 3600, 3600));
  EXPECT_CALL(server_, OnRequest("www.second.org", T_A))
    .WillOnce(SetReply(&server_, &nosecond));
  DNSPacket yesthird;
  yesthird.set_response().set_aa()
    .add_question(new DNSQuestion("www.third.gov", T_A))
    .add_answer(new DNSARR("www.third.gov", 0x0200, {2, 3, 4, 5}));
  EXPECT_CALL(server_, OnRequest("www.third.gov", T_A))
    .WillOnce(SetReply(&server_, &yesthird));

  // First pass through should send the queries.  The EXPECT_CALL .WillOnce
  // will make sure this only happens once (vs ON_CALL .WillByDefault)
  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.third.gov' aliases=[] addrs=[2.3.4.5]}", ss.str());

  // This pass should be fully served by cache and yield the same result
  HostResult cacheresult;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &cacheresult);
  Process();
  EXPECT_TRUE(cacheresult.done_);
  std::stringstream sscache;
  sscache << cacheresult.host_;
  EXPECT_EQ("{'www.third.gov' aliases=[] addrs=[2.3.4.5]}", sscache.str());
}

// Relies on retries so is UDP-only
TEST_P(MockUDPChannelTest, SearchDomainsWithResentReply) {
  DNSPacket nofirst;
  nofirst.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("www.first.com", T_A));
  EXPECT_CALL(server_, OnRequest("www.first.com", T_A))
    .WillOnce(SetReply(&server_, &nofirst));
  DNSPacket nosecond;
  nosecond.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("www.second.org", T_A));
  EXPECT_CALL(server_, OnRequest("www.second.org", T_A))
    .WillOnce(SetReply(&server_, &nosecond));
  DNSPacket yesthird;
  yesthird.set_response().set_aa()
    .add_question(new DNSQuestion("www.third.gov", T_A))
    .add_answer(new DNSARR("www.third.gov", 0x0200, {2, 3, 4, 5}));
  // Before sending the real answer, resend an earlier reply
  EXPECT_CALL(server_, OnRequest("www.third.gov", T_A))
    .WillOnce(DoAll(SetReply(&server_, &nofirst),
                    SetReplyQID(&server_, 123)))
    .WillOnce(DoAll(SetReply(&server_, &yesthird),
                    SetReplyQID(&server_, -1)));

  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.third.gov' aliases=[] addrs=[2.3.4.5]}", ss.str());
}

TEST_P(MockChannelTest, SearchDomainsBare) {
  DNSPacket nofirst;
  nofirst.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("www.first.com", T_A));
  ON_CALL(server_, OnRequest("www.first.com", T_A))
    .WillByDefault(SetReply(&server_, &nofirst));
  DNSPacket nosecond;
  nosecond.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("www.second.org", T_A));
  ON_CALL(server_, OnRequest("www.second.org", T_A))
    .WillByDefault(SetReply(&server_, &nosecond));
  DNSPacket nothird;
  nothird.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("www.third.gov", T_A));
  ON_CALL(server_, OnRequest("www.third.gov", T_A))
    .WillByDefault(SetReply(&server_, &nothird));
  DNSPacket yesbare;
  yesbare.set_response().set_aa()
    .add_question(new DNSQuestion("www", T_A))
    .add_answer(new DNSARR("www", 0x0200, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www", T_A))
    .WillByDefault(SetReply(&server_, &yesbare));

  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(0, result.timeouts_);

  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www' aliases=[] addrs=[2.3.4.5]}", ss.str());
}

TEST_P(MockChannelTest, SearchNoDataThenSuccess) {
  // First two search domains recognize the name but have no A records.
  DNSPacket nofirst;
  nofirst.set_response().set_aa()
    .add_question(new DNSQuestion("www.first.com", T_A));
  ON_CALL(server_, OnRequest("www.first.com", T_A))
    .WillByDefault(SetReply(&server_, &nofirst));
  DNSPacket nosecond;
  nosecond.set_response().set_aa()
    .add_question(new DNSQuestion("www.second.org", T_A));
  ON_CALL(server_, OnRequest("www.second.org", T_A))
    .WillByDefault(SetReply(&server_, &nosecond));
  DNSPacket yesthird;
  yesthird.set_response().set_aa()
    .add_question(new DNSQuestion("www.third.gov", T_A))
    .add_answer(new DNSARR("www.third.gov", 0x0200, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www.third.gov", T_A))
    .WillByDefault(SetReply(&server_, &yesthird));

  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.third.gov' aliases=[] addrs=[2.3.4.5]}", ss.str());
}

TEST_P(MockChannelTest, SearchNoDataThenNoDataBare) {
  // First two search domains recognize the name but have no A records.
  DNSPacket nofirst;
  nofirst.set_response().set_aa()
    .add_question(new DNSQuestion("www.first.com", T_A));
  ON_CALL(server_, OnRequest("www.first.com", T_A))
    .WillByDefault(SetReply(&server_, &nofirst));
  DNSPacket nosecond;
  nosecond.set_response().set_aa()
    .add_question(new DNSQuestion("www.second.org", T_A));
  ON_CALL(server_, OnRequest("www.second.org", T_A))
    .WillByDefault(SetReply(&server_, &nosecond));
  DNSPacket nothird;
  nothird.set_response().set_aa()
    .add_question(new DNSQuestion("www.third.gov", T_A));
  ON_CALL(server_, OnRequest("www.third.gov", T_A))
    .WillByDefault(SetReply(&server_, &nothird));
  DNSPacket nobare;
  nobare.set_response().set_aa()
    .add_question(new DNSQuestion("www", T_A));
  ON_CALL(server_, OnRequest("www", T_A))
    .WillByDefault(SetReply(&server_, &nobare));

  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENODATA, result.status_);
}

TEST_P(MockChannelTest, SearchNoDataThenFail) {
  // First two search domains recognize the name but have no A records.
  DNSPacket nofirst;
  nofirst.set_response().set_aa()
    .add_question(new DNSQuestion("www.first.com", T_A));
  ON_CALL(server_, OnRequest("www.first.com", T_A))
    .WillByDefault(SetReply(&server_, &nofirst));
  DNSPacket nosecond;
  nosecond.set_response().set_aa()
    .add_question(new DNSQuestion("www.second.org", T_A));
  ON_CALL(server_, OnRequest("www.second.org", T_A))
    .WillByDefault(SetReply(&server_, &nosecond));
  DNSPacket nothird;
  nothird.set_response().set_aa()
    .add_question(new DNSQuestion("www.third.gov", T_A));
  ON_CALL(server_, OnRequest("www.third.gov", T_A))
    .WillByDefault(SetReply(&server_, &nothird));
  DNSPacket nobare;
  nobare.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("www", T_A));
  ON_CALL(server_, OnRequest("www", T_A))
    .WillByDefault(SetReply(&server_, &nobare));

  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENODATA, result.status_);
}

TEST_P(MockChannelTest, SearchAllocFailure) {
  SearchResult result;
  SetAllocFail(1);
  ares_search(channel_, "fully.qualified.", C_IN, T_A, SearchCallback, &result);
  /* Already done */
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENOMEM, result.status_);
}

TEST_P(MockChannelTest, SearchHighNdots) {
  DNSPacket nobare;
  nobare.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("a.b.c.w.w.w", T_A));
  ON_CALL(server_, OnRequest("a.b.c.w.w.w", T_A))
    .WillByDefault(SetReply(&server_, &nobare));
  DNSPacket yesfirst;
  yesfirst.set_response().set_aa()
    .add_question(new DNSQuestion("a.b.c.w.w.w.first.com", T_A))
    .add_answer(new DNSARR("a.b.c.w.w.w.first.com", 0x0200, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("a.b.c.w.w.w.first.com", T_A))
    .WillByDefault(SetReply(&server_, &yesfirst));

  SearchResult result;
  ares_search(channel_, "a.b.c.w.w.w", C_IN, T_A, SearchCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  std::stringstream ss;
  ss << PacketToString(result.data_);
  EXPECT_EQ("RSP QRY AA NOERROR Q:{'a.b.c.w.w.w.first.com' IN A} "
            "A:{'a.b.c.w.w.w.first.com' IN A TTL=512 2.3.4.5}",
            ss.str());
}

// Test that performing an EDNS search with an OPT RR options value works. The
// options value should be included on the requests to the mock server.
// We are going to do this only via TCP since this won't include the dynamically
// generated DNS cookie that would otherwise mess with this result.
TEST_P(MockTCPChannelTest, SearchOptVal) {
  /* Define the OPT RR options code and value to use. */
  unsigned short opt_opt = 3;
  unsigned char opt_val[] = { 'c', '-', 'a', 'r', 'e', 's' };

  /* Set up the expected request and reply on the mock server for the first,
   * second and third domains. The expected requests contain the OPT RR options
   * value defined above.
   */
  std::string nofirst_req = "REQ QRY RD  Q:{'example.first.com' IN A} "
    "ADD:{'' MAXUDP=1232 OPT RCODE2=0 "
    "0003"  // opt_opt
    "0006"  // length of opt_val
    "632d61726573"  // opt_val in hex
    "}";
  DNSPacket nofirst_rep;
  nofirst_rep.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("example.first.com", T_A));
  ON_CALL(server_, OnRequest("example.first.com", T_A))
    .WillByDefault(SetReplyExpRequest(&server_, &nofirst_rep, nofirst_req));

  std::string nosecond_req = "REQ QRY RD  Q:{'example.second.org' IN A} "
    "ADD:{'' MAXUDP=1232 OPT RCODE2=0 "
    "0003"  // opt_opt
    "0006"  // length of opt_val
    "632d61726573"  // opt_val in hex
    "}";
  DNSPacket nosecond_rep;
  nosecond_rep.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("example.second.org", T_A));
  ON_CALL(server_, OnRequest("example.second.org", T_A))
    .WillByDefault(SetReplyExpRequest(&server_, &nosecond_rep, nosecond_req));

  std::string nothird_req = "REQ QRY RD  Q:{'example.third.gov' IN A} "
    "ADD:{'' MAXUDP=1232 OPT RCODE2=0 "
    "0003"  // opt_opt
    "0006"  // length of opt_val
    "632d61726573"  // opt_val in hex
    "}";
  DNSPacket nothird_rep;
  nothird_rep.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("example.third.gov", T_A));
  ON_CALL(server_, OnRequest("example.third.gov", T_A))
    .WillByDefault(SetReplyExpRequest(&server_, &nothird_rep, nothird_req));

  /* Set up the expected request and reply on the mock server for the bare
   * domain. The expected request contains the OPT RR options value defined
   * above.
   */
  std::string yesbare_req = "REQ QRY RD  Q:{'example' IN A} "
    "ADD:{'' MAXUDP=1232 OPT RCODE2=0 "
    "0003"  // opt_opt
    "0006"  // length of opt_val
    "632d61726573"  // opt_val in hex
    "}";
  DNSPacket yesbare_rep;
  yesbare_rep.set_response().set_aa()
    .add_question(new DNSQuestion("example", T_A))
    .add_answer(new DNSARR("example", 0x0200, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("example", T_A))
    .WillByDefault(SetReplyExpRequest(&server_, &yesbare_rep, yesbare_req));

  /* Construct the DNS record to search. */
  ares_dns_record_t *dnsrec = NULL;
  ares_dns_rr_t *rr = NULL;
  EXPECT_EQ(ARES_SUCCESS,
    ares_dns_record_create(&dnsrec, 0, ARES_FLAG_RD, ARES_OPCODE_QUERY,
      ARES_RCODE_NOERROR));
  EXPECT_EQ(ARES_SUCCESS,
    ares_dns_record_query_add(dnsrec, "example", (ares_dns_rec_type_t)T_A,
      (ares_dns_class_t)C_IN));
  EXPECT_EQ(ARES_SUCCESS,
    ares_dns_record_rr_add(&rr, dnsrec, ARES_SECTION_ADDITIONAL, "",
      ARES_REC_TYPE_OPT, ARES_CLASS_IN, 0));
  EXPECT_EQ(ARES_SUCCESS,
    ares_dns_rr_set_u16(rr, ARES_RR_OPT_UDP_SIZE, 1232));
  EXPECT_EQ(ARES_SUCCESS, ares_dns_rr_set_u8(rr, ARES_RR_OPT_VERSION, 0));
  EXPECT_EQ(ARES_SUCCESS, ares_dns_rr_set_u16(rr, ARES_RR_OPT_FLAGS, 0));
  EXPECT_EQ(ARES_SUCCESS,
    ares_dns_rr_set_opt(rr, ARES_RR_OPT_OPTIONS, opt_opt, opt_val,
      sizeof(opt_val)));

  /* Perform the search. Check that it succeeds with the expected response. */
  SearchResult result;
  ares_search_dnsrec(channel_, dnsrec, SearchCallbackDnsRec, &result);
  ares_dns_record_destroy(dnsrec);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  std::stringstream ss;
  ss << PacketToString(result.data_);
  EXPECT_EQ("RSP QRY AA NOERROR Q:{'example' IN A} "
            "A:{'example' IN A TTL=512 2.3.4.5}",
            ss.str());
}

TEST_P(MockChannelTest, V4WorksV6Timeout) {
  std::vector<byte> nothing;
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}));

  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &reply));

  ON_CALL(server_, OnRequest("www.google.com", T_AAAA))
    .WillByDefault(SetReplyData(&server_, nothing));

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_UNSPEC, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(1, result.timeouts_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[1.2.3.4]}", ss.str());
}

// Test case for Issue #662
TEST_P(MockChannelTest, PartialQueryCancel) {
  std::vector<byte> nothing;
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}));

  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &reply));

  ON_CALL(server_, OnRequest("www.google.com", T_AAAA))
    .WillByDefault(SetReplyData(&server_, nothing));

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_UNSPEC, HostCallback, &result);
  // After 100ms, issues ares_cancel(), this should be enough time for the A
  // record reply, but before the timeout on the AAAA record.
  Process(100);
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ECANCELLED, result.status_);
}

TEST_P(MockChannelTest, UnspecifiedFamilyV6) {
  DNSPacket rsp6;
  rsp6.set_response().set_aa()
    .add_question(new DNSQuestion("example.com", T_AAAA))
    .add_answer(new DNSAaaaRR("example.com", 100,
                              {0x21, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03}));
  ON_CALL(server_, OnRequest("example.com", T_AAAA))
    .WillByDefault(SetReply(&server_, &rsp6));

  DNSPacket rsp4;
  rsp4.set_response().set_aa()
    .add_question(new DNSQuestion("example.com", T_A));
  ON_CALL(server_, OnRequest("example.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp4));

  HostResult result;
  ares_gethostbyname(channel_, "example.com.", AF_UNSPEC, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  // Default to IPv6 when both are available.
  EXPECT_EQ("{'example.com' aliases=[] addrs=[2121:0000:0000:0000:0000:0000:0000:0303]}", ss.str());
}

TEST_P(MockChannelTest, UnspecifiedFamilyV4) {
  DNSPacket rsp6;
  rsp6.set_response().set_aa()
    .add_question(new DNSQuestion("example.com", T_AAAA));
  ON_CALL(server_, OnRequest("example.com", T_AAAA))
    .WillByDefault(SetReply(&server_, &rsp6));
  DNSPacket rsp4;
  rsp4.set_response().set_aa()
    .add_question(new DNSQuestion("example.com", T_A))
    .add_answer(new DNSARR("example.com", 100, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("example.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp4));

  HostResult result;
  ares_gethostbyname(channel_, "example.com.", AF_UNSPEC, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'example.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
}

TEST_P(MockChannelTest, UnspecifiedFamilyNoData) {
  DNSPacket rsp6;
  rsp6.set_response().set_aa()
    .add_question(new DNSQuestion("example.com", T_AAAA))
    .add_answer(new DNSCnameRR("example.com", 100, "elsewhere.com"));
  ON_CALL(server_, OnRequest("example.com", T_AAAA))
    .WillByDefault(SetReply(&server_, &rsp6));
  DNSPacket rsp4;
  rsp4.set_response().set_aa()
    .add_question(new DNSQuestion("example.com", T_A));
  ON_CALL(server_, OnRequest("example.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp4));

  HostResult result;
  ares_gethostbyname(channel_, "example.com.", AF_UNSPEC, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'' aliases=[] addrs=[]}", ss.str());
}

TEST_P(MockChannelTest, UnspecifiedFamilyCname6A4) {
  DNSPacket rsp6;
  rsp6.set_response().set_aa()
    .add_question(new DNSQuestion("example.com", T_AAAA))
    .add_answer(new DNSCnameRR("example.com", 100, "elsewhere.com"));
  ON_CALL(server_, OnRequest("example.com", T_AAAA))
    .WillByDefault(SetReply(&server_, &rsp6));
  DNSPacket rsp4;
  rsp4.set_response().set_aa()
    .add_question(new DNSQuestion("example.com", T_A))
    .add_answer(new DNSARR("example.com", 100, {1, 2, 3, 4}));
  ON_CALL(server_, OnRequest("example.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp4));

  HostResult result;
  ares_gethostbyname(channel_, "example.com.", AF_UNSPEC, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'example.com' aliases=[] addrs=[1.2.3.4]}", ss.str());
}

TEST_P(MockChannelTest, ExplicitIP) {
  HostResult result;
  ares_gethostbyname(channel_, "1.2.3.4", AF_INET, HostCallback, &result);
  EXPECT_TRUE(result.done_);  // Immediate return
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'1.2.3.4' aliases=[] addrs=[1.2.3.4]}", ss.str());
}

TEST_P(MockChannelTest, ExplicitIPAllocFail) {
  HostResult result;
  SetAllocSizeFail(strlen("1.2.3.4") + 1);
  ares_gethostbyname(channel_, "1.2.3.4", AF_INET, HostCallback, &result);
  EXPECT_TRUE(result.done_);  // Immediate return
  EXPECT_EQ(ARES_ENOMEM, result.status_);
}

TEST_P(MockChannelTest, SortListV4) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("example.com", T_A))
    .add_answer(new DNSARR("example.com", 100, {22, 23, 24, 25}))
    .add_answer(new DNSARR("example.com", 100, {12, 13, 14, 15}))
    .add_answer(new DNSARR("example.com", 100, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("example.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));

  {
    EXPECT_EQ(ARES_SUCCESS, ares_set_sortlist(channel_, "12.13.0.0/255.255.0.0 1234::5678"));
    HostResult result;
    ares_gethostbyname(channel_, "example.com.", AF_INET, HostCallback, &result);
    Process();
    EXPECT_TRUE(result.done_);
    std::stringstream ss;
    ss << result.host_;
    EXPECT_EQ("{'example.com' aliases=[] addrs=[12.13.14.15, 22.23.24.25, 2.3.4.5]}", ss.str());
  }
  {
    EXPECT_EQ(ARES_SUCCESS, ares_set_sortlist(channel_, "2.3.0.0/16 130.140.150.160/26"));
    HostResult result;
    ares_gethostbyname(channel_, "example.com.", AF_INET, HostCallback, &result);
    Process();
    EXPECT_TRUE(result.done_);
    std::stringstream ss;
    ss << result.host_;
    EXPECT_EQ("{'example.com' aliases=[] addrs=[2.3.4.5, 22.23.24.25, 12.13.14.15]}", ss.str());
  }
  struct ares_options options;
  memset(&options, 0, sizeof(options));
  int optmask = 0;
  EXPECT_EQ(ARES_SUCCESS, ares_save_options(channel_, &options, &optmask));
  EXPECT_TRUE((optmask & ARES_OPT_SORTLIST) == ARES_OPT_SORTLIST);
  ares_destroy_options(&options);
}

TEST_P(MockChannelTest, SortListV6) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("example.com", T_AAAA))
    .add_answer(new DNSAaaaRR("example.com", 100,
                              {0x11, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x02, 0x02}))
    .add_answer(new DNSAaaaRR("example.com", 100,
                              {0x21, 0x21, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                               0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03}));
  ON_CALL(server_, OnRequest("example.com", T_AAAA))
    .WillByDefault(SetReply(&server_, &rsp));

  {
    ares_set_sortlist(channel_, "1111::/16 2.3.0.0/255.255.0.0");
    HostResult result;
    ares_gethostbyname(channel_, "example.com.", AF_INET6, HostCallback, &result);
    Process();
    EXPECT_TRUE(result.done_);
    std::stringstream ss;
    ss << result.host_;
    EXPECT_EQ("{'example.com' aliases=[] addrs=[1111:0000:0000:0000:0000:0000:0000:0202, "
              "2121:0000:0000:0000:0000:0000:0000:0303]}", ss.str());
  }
  {
    ares_set_sortlist(channel_, "2121::/8");
    HostResult result;
    ares_gethostbyname(channel_, "example.com.", AF_INET6, HostCallback, &result);
    Process();
    EXPECT_TRUE(result.done_);
    std::stringstream ss;
    ss << result.host_;
    EXPECT_EQ("{'example.com' aliases=[] addrs=[2121:0000:0000:0000:0000:0000:0000:0303, "
              "1111:0000:0000:0000:0000:0000:0000:0202]}", ss.str());
  }
}

// Relies on retries so is UDP-only
TEST_P(MockUDPChannelTest, SearchDomainsAllocFail) {
  DNSPacket nofirst;
  nofirst.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("www.first.com", T_A));
  ON_CALL(server_, OnRequest("www.first.com", T_A))
    .WillByDefault(SetReply(&server_, &nofirst));
  DNSPacket nosecond;
  nosecond.set_response().set_aa().set_rcode(NXDOMAIN)
    .add_question(new DNSQuestion("www.second.org", T_A));
  ON_CALL(server_, OnRequest("www.second.org", T_A))
    .WillByDefault(SetReply(&server_, &nosecond));
  DNSPacket yesthird;
  yesthird.set_response().set_aa()
    .add_question(new DNSQuestion("www.third.gov", T_A))
    .add_answer(new DNSARR("www.third.gov", 0x0200, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www.third.gov", T_A))
    .WillByDefault(SetReply(&server_, &yesthird));

  // Fail a variety of different memory allocations, and confirm
  // that the operation either fails with ENOMEM or succeeds
  // with the expected result.
  const int kCount = 34;
  HostResult results[kCount];
  for (int ii = 1; ii <= kCount; ii++) {
    HostResult* result = &(results[ii - 1]);
    ClearFails();
    SetAllocFail(ii);
    ares_gethostbyname(channel_, "www", AF_INET, HostCallback, result);
    Process();
    EXPECT_TRUE(result->done_);
    if (result->status_ == ARES_SUCCESS) {
      std::stringstream ss;
      ss << result->host_;
      EXPECT_EQ("{'www.third.gov' aliases=[] addrs=[2.3.4.5]}", ss.str()) << " failed alloc #" << ii;
      if (verbose) std::cerr << "Succeeded despite failure of alloc #" << ii << std::endl;
    }
  }

  // Explicitly destroy the channel now, so that the HostResult objects
  // are still valid (in case any pending work refers to them).
  ares_destroy(channel_);
  channel_ = nullptr;
}

// Relies on retries so is UDP-only
TEST_P(MockUDPChannelTest, Resend) {
  std::vector<byte> nothing;
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}));

  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReplyData(&server_, nothing))
    .WillOnce(SetReplyData(&server_, nothing))
    .WillOnce(SetReply(&server_, &reply));

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(2, result.timeouts_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[1.2.3.4]}", ss.str());
}

TEST_P(MockChannelTest, CancelImmediate) {
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  ares_cancel(channel_);
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ECANCELLED, result.status_);
  EXPECT_EQ(0, result.timeouts_);
}

TEST_P(MockChannelTest, CancelImmediateGetHostByAddr) {
  HostResult result;
  struct in_addr addr;
  addr.s_addr = htonl(0x08080808);

  ares_gethostbyaddr(channel_, &addr, sizeof(addr), AF_INET, HostCallback, &result);
  ares_cancel(channel_);
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ECANCELLED, result.status_);
  EXPECT_EQ(0, result.timeouts_);
}

// Relies on retries so is UDP-only
TEST_P(MockUDPChannelTest, CancelLater) {
  std::vector<byte> nothing;

  // On second request, cancel the channel.
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReplyData(&server_, nothing))
    .WillOnce(CancelChannel(&server_, channel_));

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ECANCELLED, result.status_);
  EXPECT_EQ(0, result.timeouts_);
}

TEST_P(MockChannelTest, DisconnectFirstAttempt) {
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}));

  // On second request, cancel the channel.
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(Disconnect(&server_))
    .WillOnce(SetReply(&server_, &reply));

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[1.2.3.4]}", ss.str());
}

TEST_P(MockChannelTest, GetHostByNameDestroyAbsolute) {
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);

  ares_destroy(channel_);
  channel_ = nullptr;

  EXPECT_TRUE(result.done_);  // Synchronous
  EXPECT_EQ(ARES_EDESTRUCTION, result.status_);
  EXPECT_EQ(0, result.timeouts_);
}

TEST_P(MockChannelTest, GetHostByNameDestroyRelative) {
  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);

  ares_destroy(channel_);
  channel_ = nullptr;

  EXPECT_TRUE(result.done_);  // Synchronous
  EXPECT_EQ(ARES_EDESTRUCTION, result.status_);
  EXPECT_EQ(0, result.timeouts_);
}

TEST_P(MockChannelTest, GetHostByNameCNAMENoData) {
  DNSPacket response;
  response.set_response().set_aa()
    .add_question(new DNSQuestion("cname.first.com", T_A))
    .add_answer(new DNSCnameRR("cname.first.com", 100, "a.first.com"));
  ON_CALL(server_, OnRequest("cname.first.com", T_A))
    .WillByDefault(SetReply(&server_, &response));

  HostResult result;
  ares_gethostbyname(channel_, "cname.first.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ENODATA, result.status_);
}

TEST_P(MockChannelTest, GetHostByAddrDestroy) {
  unsigned char gdns_addr4[4] = {0x08, 0x08, 0x08, 0x08};
  HostResult result;
  ares_gethostbyaddr(channel_, gdns_addr4, sizeof(gdns_addr4), AF_INET, HostCallback, &result);

  ares_destroy(channel_);
  channel_ = nullptr;

  EXPECT_TRUE(result.done_);  // Synchronous
  EXPECT_EQ(ARES_EDESTRUCTION, result.status_);
  EXPECT_EQ(0, result.timeouts_);
}

TEST_P(MockChannelTest, TriggerResendThenConnFailSERVFAIL) {
  // Set up the server response. The server always returns SERVFAIL.
  DNSPacket badrsp;
  badrsp.set_response().set_aa().set_rcode(SERVFAIL)
    .add_question(new DNSQuestion("www.google.com", T_A));
  DNSPacket goodrsp;
  goodrsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReplyAndFailSend(&server_, &badrsp))
    .WillOnce(SetReply(&server_, &goodrsp));

  ares_socket_functions sock_funcs;
  memset(&sock_funcs, 0, sizeof(sock_funcs));

  sock_funcs.asendv = ares_sendv_fail;

  ares_set_socket_functions(channel_, &sock_funcs, NULL);

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback,
                     &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[1.2.3.4]}", ss.str());
}

TEST_P(MockUDPChannelTest, TriggerResendThenConnFailEDNS) {
  // Set up the server response to simulate an EDNS failure
  DNSPacket badrsp;
  badrsp.set_response().set_aa().set_rcode(FORMERR)
    .add_question(new DNSQuestion("www.google.com", T_A));
  DNSPacket goodrsp;
  goodrsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReplyAndFailSend(&server_, &badrsp))
    .WillOnce(SetReply(&server_, &goodrsp));

  ares_socket_functions sock_funcs;
  memset(&sock_funcs, 0, sizeof(sock_funcs));

  sock_funcs.asendv = ares_sendv_fail;

  ares_set_socket_functions(channel_, &sock_funcs, NULL);

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback,
                     &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[1.2.3.4]}", ss.str());
}

// Regression for issue #1043: a long chain of retryable connection failures
// used to recurse ares_requeue_query() -> ares_send_query() until the stack was
// exhausted.  With a very high retry count and a socket that always fails to be
// created (a retryable ARES_ECONNREFUSED), the retries must be processed
// iteratively and the query must terminate cleanly rather than crashing.
class MockRetryDepthChannelTest : public MockChannelOptsTest,
                                  public ::testing::WithParamInterface<int> {
public:
  MockRetryDepthChannelTest()
    : MockChannelOptsTest(1, GetParam(), false, false, FillOptions(&opts_),
                          ARES_OPT_TRIES)
  {
  }

  static struct ares_options *FillOptions(struct ares_options *opts)
  {
    memset(opts, 0, sizeof(struct ares_options));
    /* Large enough that the old recursive path would overflow the stack. */
    opts->tries = 100000;
    return opts;
  }

private:
  struct ares_options opts_;
};

static size_t g_always_fail_socket_calls = 0;

static ares_socket_t always_fail_socket(int af, int type, int protocol,
                                        void *user_data)
{
  (void)af;
  (void)type;
  (void)protocol;
  (void)user_data;
  g_always_fail_socket_calls++;
  return ARES_SOCKET_BAD;
}

TEST_P(MockRetryDepthChannelTest, HighRetryNoStackOverflow) {
  ares_socket_functions sock_funcs;
  memset(&sock_funcs, 0, sizeof(sock_funcs));
  sock_funcs.asocket = always_fail_socket;
  ares_set_socket_functions(channel_, &sock_funcs, NULL);

  g_always_fail_socket_calls = 0;

  QueryResult result;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    QueryCallback, &result, NULL);
  Process();

  /* If the retries didn't drive the query to a terminal state on their own
   * (e.g. it parked awaiting a response that will never arrive), cancel it so
   * the query terminates deterministically. */
  if (!result.done_) {
    ares_cancel(channel_);
    Process();
  }

  /* The essential property is that we reached this point at all: on unpatched
   * code the retryable failures recursed ares_requeue_query()/ares_send_query()
   * until the stack overflowed and the process aborted before getting here.  We
   * also confirm the retries were attempted iteratively (more than one socket
   * creation) and that the query reached a terminal state. */
  EXPECT_GT(g_always_fail_socket_calls, (size_t)1);
  EXPECT_TRUE(result.done_);
}

// Regression for CVE-2026-33630 / GHSA-6wfj-rwm7-3542: invoking ares_cancel()
// from within a normal response callback must not double-free the query.  The
// callback is dispatched from the read_answers() deferred-requeue flush, and the
// query must be detached from all lookup lists before the callback runs so that
// the reentrant ares_cancel() cannot find and free it a second time.
struct CancelInCbData {
  ares_channel_t *channel;
  bool            done;
  size_t          cbs;
};

static void CancelChannelCallback(void *arg, ares_status_t status,
                                  size_t timeouts,
                                  const ares_dns_record_t *dnsrec)
{
  CancelInCbData *data = static_cast<CancelInCbData *>(arg);
  (void)status;
  (void)timeouts;
  (void)dnsrec;
  data->done = true;
  data->cbs++;
  /* Reentrant cancel from within the callback.  Must not double-free. */
  ares_cancel(data->channel);
}

// ares_destroy() notifies each still-running query that the channel is going
// away, and holds a pointer to the next one in its list while doing so.  A
// reentrant ares_cancel() from one of those callbacks frees every remaining
// query -- including that cached next, which ares_destroy() then follows.
//
// ares_cancel() is immune to the same pattern because it swaps the list head
// before walking it; ares_destroy() did not.  Two queries are required: with
// only one in flight there is no next to cache and nothing goes wrong.
// tries=1 so a teardown ends the query outright instead of retrying it, which
// is the precondition this family of bugs shares.
class MockUDPSingleTryTest : public MockChannelOptsTest,
                             public ::testing::WithParamInterface<int> {
public:
  MockUDPSingleTryTest()
    : MockChannelOptsTest(1, GetParam(), false, false, FillOptions(&opts_),
                          ARES_OPT_TRIES)
  {
  }

  static struct ares_options *FillOptions(struct ares_options *opts)
  {
    memset(opts, 0, sizeof(struct ares_options));
    opts->tries = 1;
    return opts;
  }

private:
  struct ares_options opts_;
};

// ares_cancel() and ares_destroy() free a query themselves rather than going
// through end_query().  Both used to invoke the application callback while the
// query was still in queries_by_qid, on conn->queries_to_conn and in
// queries_by_timeout -- only all_queries was unlinked first.  A reentrant
// teardown from within that callback therefore still found the query, ended it
// a second time, and freed it; the ares_free_query() that followed then
// double-freed.  One query in flight is enough.
static void ReconfigureServersCallback(void *arg, ares_status_t status,
                                       size_t                   timeouts,
                                       const ares_dns_record_t *dnsrec) {
  CancelInCbData *data = static_cast<CancelInCbData *>(arg);
  (void)status;
  (void)timeouts;
  (void)dnsrec;
  data->done = true;
  data->cbs++;
  /* Frees every server absent from the new configuration, closing their
   * connections -- which still hold the queries being torn down. */
  ares_set_servers_csv(data->channel, "127.0.0.1:53055");
}

// Reconfiguring the server list from a cancellation callback.
TEST_P(MockUDPSingleTryTest, ReconfigureInCancelCallbackNoDoubleFree) {
  CancelInCbData data;
  data.channel = channel_;
  data.done    = false;
  data.cbs     = 0;

  QueryResult result;
  /* Left unanswered, so both stay attached to the connection. */
  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, ReconfigureServersCallback,
                              &data, NULL));
  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel_, "www.example.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, QueryCallback, &result, NULL));

  ares_cancel(channel_);
  EXPECT_TRUE(data.done);
  EXPECT_TRUE(result.done_);
  /* Each lookup must be notified exactly once, not twice. */
  EXPECT_EQ((size_t)1, data.cbs);
}

/* The replacement lookup's result outlives the callback that starts it. */
static QueryResult g_replacement_result;

static void NewQueryOnFailingWriteCallback(void *arg, ares_status_t status,
                                           size_t                   timeouts,
                                           const ares_dns_record_t *dnsrec) {
  /* ares_set_socket_functions() stores the pointer rather than copying it, so
   * this must outlive the call. */
  static ares_socket_functions sock_funcs;
  CancelInCbData              *data = static_cast<CancelInCbData *>(arg);
  (void)status;
  (void)timeouts;
  (void)dnsrec;

  data->cbs++;

  /* Only the first cancellation callback starts a replacement lookup. */
  if (data->done) {
    return;
  }
  data->done = true;

  memset(&sock_funcs, 0, sizeof(sock_funcs));
  sock_funcs.asendv = LibraryTest::ares_sendv_fail;
  ares_set_socket_functions(data->channel, &sock_funcs, NULL);
  LibraryTest::SetFailSend();

  ares_query_dnsrec(data->channel, "www.probe.com", ARES_CLASS_IN,
                    ARES_REC_TYPE_A, QueryCallback, &g_replacement_result,
                    NULL);
}

// No reconfiguration at all: the cancellation callback simply starts a
// replacement lookup -- an ordinary retry pattern -- and that lookup's write
// fails.  ares_close_connection() then walks conn->queries_to_conn, which
// still holds the queries ares_cancel() is in the middle of.  Wider than the
// case above: it needs nothing more than a connection error.
TEST_P(MockUDPSingleTryTest, NewQueryInCancelCallbackConnErrorNoDoubleFree) {
  CancelInCbData data;
  data.channel = channel_;
  data.done    = false;
  data.cbs     = 0;

  g_replacement_result.done_ = false;

  QueryResult result;
  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, NewQueryOnFailingWriteCallback,
                              &data, NULL));
  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel_, "www.example.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, QueryCallback, &result, NULL));

  ares_cancel(channel_);
  EXPECT_TRUE(data.done);
  EXPECT_TRUE(result.done_);
  /* Each lookup must be notified exactly once, not twice. */
  EXPECT_EQ((size_t)1, data.cbs);
  /* Pins that the replacement lookup ran and its write failed, which is what
   * drives the connection teardown this test is about. */
  EXPECT_TRUE(g_replacement_result.done_);
}

// The same shape through ares_destroy() rather than ares_cancel().  Swapping
// the all_queries head does not help here: the reentrant teardown reaches the
// query through the connection, not through all_queries.
TEST_P(MockUDPSingleTryTest, ReconfigureInDestroyCallbackNoDoubleFree) {
  CancelInCbData data;
  data.channel = channel_;
  data.done    = false;
  data.cbs     = 0;

  QueryResult result;
  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, ReconfigureServersCallback,
                              &data, NULL));
  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel_, "www.example.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, QueryCallback, &result, NULL));

  ares_destroy(channel_);
  channel_ = nullptr; /* destroyed here, don't let the fixture repeat it */

  EXPECT_TRUE(data.done);
  EXPECT_TRUE(result.done_);
  /* Each lookup must be notified exactly once, not twice. */
  EXPECT_EQ((size_t)1, data.cbs);
}

// Once ares_destroy() has begun, a lookup started from a destruction callback
// is refused rather than allocated into a channel being torn down -- and the
// refusal still runs the caller's callback with ARES_EDESTRUCTION, so context
// attached to `arg` can be reclaimed.  ares_cancel() leaves sys_up true, so a
// destruction callback is the only way to reach this.
struct DestroyReentryData {
  ares_channel_t *channel;
  size_t          outer_cbs;
  size_t          replacement_cbs;
  ares_status_t   replacement_status;
};

static void DestroyReplacementCallback(void *arg, ares_status_t status,
                                       size_t                   timeouts,
                                       const ares_dns_record_t *dnsrec) {
  DestroyReentryData *data = static_cast<DestroyReentryData *>(arg);
  (void)timeouts;
  (void)dnsrec;
  data->replacement_cbs++;
  data->replacement_status = status;
}

static void DestroyReentryCallback(void *arg, ares_status_t status,
                                   size_t                   timeouts,
                                   const ares_dns_record_t *dnsrec) {
  DestroyReentryData *data = static_cast<DestroyReentryData *>(arg);
  (void)status;
  (void)timeouts;
  (void)dnsrec;
  data->outer_cbs++;
  ares_query_dnsrec(data->channel, "www.example.com", ARES_CLASS_IN,
                    ARES_REC_TYPE_A, DestroyReplacementCallback, data, NULL);
}

TEST_F(LibraryTest, NewQueryInDestroyCallbackIsRefusedAndNotified) {
  ares_channel_t     *channel = nullptr;
  struct ares_options opts;
  int                 optmask = 0;

  memset(&opts, 0, sizeof(opts));
  opts.tries  = 1;
  optmask    |= ARES_OPT_TRIES;
  opts.flags  = ARES_FLAG_NOSEARCH;
  optmask    |= ARES_OPT_FLAGS;

  EXPECT_EQ(ARES_SUCCESS, ares_init_options(&channel, &opts, optmask));
  // Nothing listens here; no traffic is needed.
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(channel, "127.0.0.1:53055"));

  DestroyReentryData data;
  data.channel            = channel;
  data.outer_cbs          = 0;
  data.replacement_cbs    = 0;
  data.replacement_status = ARES_SUCCESS;

  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel, "www.google.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, DestroyReentryCallback, &data,
                              NULL));

  ares_destroy(channel);

  EXPECT_EQ((size_t)1, data.outer_cbs);
  // Refused -- but notified, so the caller can release whatever it attached.
  EXPECT_EQ((size_t)1, data.replacement_cbs);
  EXPECT_EQ(ARES_EDESTRUCTION, data.replacement_status);
}

INSTANTIATE_TEST_SUITE_P(AddressFamilies, MockUDPSingleTryTest,
                         ::testing::ValuesIn(ares::test::families),
                         PrintFamily);

// ares_process_pending_write() flushes each server's TCP connection.  A flush
// that fails tears the connection down, which ends a query that is out of
// retries and runs its callback right there.  A callback that reconfigures the
// servers removes the one being flushed, and the walk must not go on to use
// that server, its connection or its place in the list.
class MockTCPSingleTryStayOpenTest
  : public MockChannelOptsTest,
    public ::testing::WithParamInterface<int> {
public:
  MockTCPSingleTryStayOpenTest()
    : MockChannelOptsTest(1, GetParam(), true, false, FillOptions(&opts_),
                          ARES_OPT_TRIES | ARES_OPT_FLAGS)
  {
  }

  static struct ares_options *FillOptions(struct ares_options *opts)
  {
    memset(opts, 0, sizeof(struct ares_options));
    opts->tries = 1;
    /* ARES_OPT_FLAGS replaces the defaults, which include EDNS. */
    opts->flags = ARES_FLAG_STAYOPEN | ARES_FLAG_EDNS;
    return opts;
  }

private:
  struct ares_options opts_;
};

static void PendingWriteNotifyCallback(void *data)
{
  *static_cast<bool *>(data) = true;
}

TEST_P(MockTCPSingleTryStayOpenTest, ReconfigureInPendingWriteFailureCallback) {
  DNSPacket rsp;
  rsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &rsp));

  /* Answered, so the TCP connection is connected; STAYOPEN keeps it. */
  QueryResult first;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    QueryCallback, &first, NULL);
  Process();
  EXPECT_TRUE(first.done_);

  /* From here writes on that connection wait for ares_process_pending_write. */
  bool pending = false;
  ares_set_pending_write_cb(channel_, PendingWriteNotifyCallback, &pending);

  CancelInCbData data;
  data.channel = channel_;
  data.done    = false;
  data.cbs     = 0;
  ares_query_dnsrec(channel_, "www.example.com", ARES_CLASS_IN,
                    ARES_REC_TYPE_A, ReconfigureServersCallback, &data, NULL);
  EXPECT_TRUE(pending);
  EXPECT_FALSE(data.done);

  /* ares_set_socket_functions() stores the pointer rather than copying it. */
  static ares_socket_functions sock_funcs;
  memset(&sock_funcs, 0, sizeof(sock_funcs));
  sock_funcs.asendv = ares_sendv_fail;
  ares_set_socket_functions(channel_, &sock_funcs, NULL);
  LibraryTest::SetFailSend();

  ares_process_pending_write(channel_);

  EXPECT_TRUE(data.done);
  EXPECT_EQ((size_t)1, data.cbs);

  ares_set_pending_write_cb(channel_, NULL, NULL);
}

INSTANTIATE_TEST_SUITE_P(AddressFamilies, MockTCPSingleTryStayOpenTest,
                         ::testing::ValuesIn(ares::test::families),
                         PrintFamily);

TEST_P(MockUDPChannelTest, CancelInDestroyCallbackNoUseAfterFree) {
  CancelInCbData data;
  data.channel = channel_;
  data.done    = false;
  data.cbs     = 0;

  /* Left unanswered, so both are still pending when the channel goes down. */
  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, CancelChannelCallback, &data,
                              NULL));
  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel_, "www.example.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, CancelChannelCallback, &data,
                              NULL));
  /* Without this the test can pass vacuously if a query failed to start. */
  EXPECT_EQ(2, ares_queue_active_queries(channel_));

  ares_destroy(channel_);
  channel_ = nullptr; /* destroyed here, don't let the fixture repeat it */

  EXPECT_TRUE(data.done);
  /* Every query must be notified exactly once. */
  EXPECT_EQ((size_t)2, data.cbs);
}

// Fixture with tries=1 so the first failure ends the query outright instead of
// retrying it -- that is what drives execution into end_query()'s synchronous
// (requeue==NULL) branch.
// The tests below use the MockUDPSingleTryTest fixture defined above: tries=1,
// so the first failure ends the query outright instead of retrying it, which
// is what drives execution into end_query()'s synchronous (requeue==NULL)
// branch.
//

// Connection-teardown route into the synchronous branch.  A malformed reply
// makes process_answer() return EBADRESP, so handle_conn_error() tears the
// connection down and ares_close_connection() requeues its queries with a NULL
// requeue; with tries=1 the query is ended right there.  CVE-2026-33630 fixed
// the deferred flush path only, so this one still double-freed.
TEST_P(MockUDPSingleTryTest, CancelInCallbackConnTeardownNoDoubleFree) {
  /* Too short to be a DNS header, so it fails to parse. */
  std::vector<byte> malformed = {0x00, 0x00, 0x00, 0x00};
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReplyData(&server_, malformed));

  CancelInCbData data;
  data.channel = channel_;
  data.done    = false;
  data.cbs     = 0;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    CancelChannelCallback, &data, NULL);
  Process();
  EXPECT_TRUE(data.done);
}

// Server-removal route into the same synchronous branch, which the
// connection-teardown test above does not cover.  Reconfiguring the server list
// destroys the servers that are gone, and ares_close_sockets() closes their
// connections with a NULL requeue -- a second way to reach end_query() with the
// query still linked.  No Process() call is needed: the query is attached to a
// connection by ares_query_dnsrec() itself, and nothing has replied to it.
TEST_P(MockUDPSingleTryTest, CancelInCallbackServerRemovalNoDoubleFree) {
  CancelInCbData data;
  data.channel = channel_;
  data.done    = false;
  data.cbs     = 0;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    CancelChannelCallback, &data, NULL);

  /* Drops the server the in-flight query is attached to. */
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(channel_, "127.0.0.1:53055"));
  EXPECT_TRUE(data.done);
}

// ares_send_query_int() calls handle_conn_error() on a retryable write failure
// and then keeps using its own query.  That teardown requeues the *other*
// queries on the connection, and with tries=1 they are ended outright; if one
// of their callbacks reentrantly cancels the channel, this query is freed too
// -- it is still linked in all_queries, having only joined
// conn->queries_to_conn on the success path further down.  Using it afterwards
// is a use-after-free.
//
// The first query is left unanswered so it stays on the connection, then the
// next write is made to fail with ECONNREFUSED so the second query takes the
// error path while the first is still attached.
//
// Note this is masked on code that has not fixed the end_query() double-free:
// that aborts inside handle_conn_error() before control returns here.
TEST_P(MockUDPSingleTryTest, CancelInSiblingCallbackConnErrorNoUseAfterFree) {
  CancelInCbData data;
  data.channel = channel_;
  data.done    = false;
  data.cbs     = 0;

  /* No reply configured for this one, so it stays attached to the connection. */
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    CancelChannelCallback, &data, NULL);

  ares_socket_functions sock_funcs;
  memset(&sock_funcs, 0, sizeof(sock_funcs));
  sock_funcs.asendv = ares_sendv_fail;
  ares_set_socket_functions(channel_, &sock_funcs, NULL);
  LibraryTest::SetFailSend();

  QueryResult result;
  ares_query_dnsrec(channel_, "www.example.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    QueryCallback, &result, NULL);

  /* The write failure above tears the connection down, which ends the first
   * query, whose callback cancels the channel and frees this one. */
  EXPECT_TRUE(data.done);
  /* The sibling is cancelled from the first query's callback rather than
   * requeued; pin that rather than leaving it unstated. */
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ECANCELLED, result.status_);
}

// The socket-state callback -- registered with ARES_OPT_SOCK_STATE_CB, the
// usual way c-ares is driven from an event loop -- used to be dispatched from
// the middle of opening a connection, and ares_open_connection() keeps using
// that connection afterwards.  A reentrant ares_cancel() from the callback
// frees both the connection (through the idle sweep) and the in-flight query,
// leaving the library using either.
struct SockStateCancelData {
  ares_channel_t *channel;
  size_t          calls;
};

static void SockStateCancelCallback(void *data, ares_socket_t socket_fd,
                                    int readable, int writable)
{
  SockStateCancelData *d = static_cast<SockStateCancelData *>(data);
  (void)socket_fd;
  (void)readable;
  (void)writable;
  d->calls++;
  // Reentrant cancel from the socket-state notification.
  if (d->channel != nullptr) {
    ares_cancel(d->channel);
  }
}

TEST_F(LibraryTest, CancelInSockStateCallbackNoUseAfterFree) {
  SockStateCancelData data;
  ares_channel_t     *chan = nullptr;
  struct ares_options opts;
  int                 optmask = 0;

  data.channel = nullptr;
  data.calls   = 0;

  memset(&opts, 0, sizeof(opts));
  opts.sock_state_cb      = SockStateCancelCallback;
  opts.sock_state_cb_data = &data;
  optmask                 = ARES_OPT_SOCK_STATE_CB;

  EXPECT_EQ(ARES_SUCCESS, ares_init_options(&chan, &opts, optmask));
  ASSERT_NE(nullptr, chan);
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(chan, "127.0.0.1:53055"));

  /* Armed only now, so channel setup itself does not cancel. */
  data.channel = chan;

  QueryResult result;
  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    QueryCallback, &result, NULL);

  /* Opening the connection notifies, the notification cancels, and the
   * library must still be able to finish -- and the lookup must actually be
   * cancelled rather than the cancel quietly doing nothing. */
  EXPECT_GT(data.calls, (size_t)0);
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_ECANCELLED, result.status_);

  data.channel = nullptr; /* don't cancel from teardown notifications */
  ares_destroy(chan);
}

// Deferring the notification must not reorder it past the close of the
// descriptor it names.  An event loop is told "stop watching fd N"; if fd N is
// already closed by then, the number can have been handed to something else --
// including to application code running from a deferred query callback -- and
// the application ends up setting and clearing interest in a descriptor it now
// owns for another purpose.
//
// The invariant is checked directly: while the callback naming the descriptor
// runs, that descriptor must still be open.
#ifndef _WIN32
struct SockStateCloseOrderData {
  size_t closes_seen;
  size_t closed_while_already_shut;
};

static void SockStateCloseOrderCallback(void *data, ares_socket_t socket_fd,
                                        int readable, int writable)
{
  SockStateCloseOrderData *d = static_cast<SockStateCloseOrderData *>(data);
  if (readable || writable) {
    return;
  }
  d->closes_seen++;
  // F_GETFD on a closed descriptor fails with EBADF.  c-ares must not have
  // closed it yet at this point.
  if (fcntl(static_cast<int>(socket_fd), F_GETFD) == -1 && errno == EBADF) {
    d->closed_while_already_shut++;
  }
}

TEST_F(LibraryTest, SockStateCloseIsReportedBeforeDescriptorCloses) {
  SockStateCloseOrderData data;
  ares_channel_t         *chan = nullptr;
  struct ares_options     opts;

  data.closes_seen               = 0;
  data.closed_while_already_shut = 0;

  memset(&opts, 0, sizeof(opts));
  opts.sock_state_cb      = SockStateCloseOrderCallback;
  opts.sock_state_cb_data = &data;

  EXPECT_EQ(ARES_SUCCESS,
            ares_init_options(&chan, &opts, ARES_OPT_SOCK_STATE_CB));
  ASSERT_NE(nullptr, chan);
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(chan, "127.0.0.1:53055"));

  QueryResult result;
  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    QueryCallback, &result, NULL);
  ares_destroy(chan);

  EXPECT_GT(data.closes_seen, (size_t)0);
  EXPECT_EQ((size_t)0, data.closed_while_already_shut)
    << "the application was told to stop watching a descriptor c-ares had "
       "already closed";
}
#endif

// Reconfiguring the servers from an I/O hook frees the server and its
// connection while the library is still holding both.  Deferred reclamation is
// what makes these safe: the objects are unlinked from every lookup structure
// immediately, so nothing can find them, but their memory survives until the
// outermost call unwinds.  Without it these are heap-use-after-free in
// ares_conn_connect() and handle_conn_error() respectively.
//
// This is the class behind CVE-2025-31498 and CVE-2025-62408 -- a connection
// cleaned up while a caller further up the stack still held it.
struct ReconfigureFromHookData {
  ares_channel_t *channel;
  bool            done;
  int             status;
};

static int ReconfigureConfigureCallback(ares_socket_t fd, int type, void *data)
{
  ReconfigureFromHookData *d = static_cast<ReconfigureFromHookData *>(data);
  (void)fd;
  (void)type;
  // Frees the server this connection is being opened for.
  ares_set_servers_csv(d->channel, "127.0.0.1:53056");
  return 0;
}

static void ReconfigureFromHookCallback(void *arg, ares_status_t status,
                                        size_t                   timeouts,
                                        const ares_dns_record_t *dnsrec)
{
  ReconfigureFromHookData *d = static_cast<ReconfigureFromHookData *>(arg);
  (void)timeouts;
  (void)dnsrec;
  d->done   = true;
  d->status = static_cast<int>(status);
}

TEST_F(LibraryTest, ReconfigureInConfigureHookNoUseAfterFree) {
  ares_channel_t         *chan = nullptr;
  struct ares_options     opts;
  int                     optmask = 0;
  ReconfigureFromHookData data;

  memset(&opts, 0, sizeof(opts));
  opts.tries  = 1;
  optmask    |= ARES_OPT_TRIES;
  opts.flags  = ARES_FLAG_NOSEARCH;
  optmask    |= ARES_OPT_FLAGS;

  EXPECT_EQ(ARES_SUCCESS, ares_init_options(&chan, &opts, optmask));
  ASSERT_NE(nullptr, chan);
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(chan, "127.0.0.1:53055"));

  data.channel = chan;
  data.done    = false;
  data.status  = ARES_SUCCESS;
  ares_set_socket_configure_callback(chan, ReconfigureConfigureCallback, &data);

  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    ReconfigureFromHookCallback, &data, NULL);

  // Must complete rather than be stranded -- every exit from the send path
  // either enqueues the query or ends it.
  EXPECT_TRUE(data.done);
  EXPECT_EQ((size_t)0, ares_queue_active_queries(chan));
  ares_destroy(chan);
}

// Reconfiguring the servers from the application's own send function.  The
// write itself goes through: the send function reports every byte sent.  But
// the reconfiguration has already closed the connection being written to and
// removed its server, so the query must be moved to the new configuration
// rather than attached to the closed connection -- where it would sit on a
// list that is freed once the call unwinds, in flight on no socket.
struct ReconfigureFromSendData {
  ares_channel_t *channel;
  bool            done;
  int             status;
};

static ares_ssize_t ReconfigureSendv(ares_socket_t fd, const struct iovec *iov,
                                     int iovcnt, void *user_data)
{
  ReconfigureFromSendData *d =
    static_cast<ReconfigureFromSendData *>(user_data);
  ares_ssize_t len = 0;
  (void)fd;
  for (int i = 0; i < iovcnt; i++) {
    len += static_cast<ares_ssize_t>(iov[i].iov_len);
  }
  // Frees the server this connection belongs to.  Reconfiguring to the same
  // list again is a no-op, so the retry is written normally.
  ares_set_servers_csv(d->channel, "127.0.0.1:53056");
  return len;
}

static void ReconfigureFromSendCallback(void *arg, ares_status_t status,
                                        size_t                   timeouts,
                                        const ares_dns_record_t *dnsrec)
{
  ReconfigureFromSendData *d = static_cast<ReconfigureFromSendData *>(arg);
  (void)timeouts;
  (void)dnsrec;
  d->done   = true;
  d->status = static_cast<int>(status);
}

TEST_F(LibraryTest, ReconfigureInSuccessfulSendNoUseAfterFree) {
  ares_channel_t         *chan = nullptr;
  struct ares_options     opts;
  int                     optmask = 0;
  ares_socket_functions   funcs;
  ReconfigureFromSendData data;
  ares_socket_t           socks[ARES_GETSOCK_MAXNUM];

  memset(&opts, 0, sizeof(opts));
  opts.flags  = ARES_FLAG_NOSEARCH;
  optmask    |= ARES_OPT_FLAGS;

  EXPECT_EQ(ARES_SUCCESS, ares_init_options(&chan, &opts, optmask));
  ASSERT_NE(nullptr, chan);
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(chan, "127.0.0.1:53055"));

  data.channel = chan;
  data.done    = false;
  data.status  = ARES_SUCCESS;
  memset(&funcs, 0, sizeof(funcs));
  funcs.asendv = ReconfigureSendv;
  ares_set_socket_functions(chan, &funcs, &data);

  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    ReconfigureFromSendCallback, &data, NULL);

  // Moved to the new server and written there, so it is in flight on a
  // socket the application can watch.
  EXPECT_FALSE(data.done);
  EXPECT_EQ((size_t)1, ares_queue_active_queries(chan));
  EXPECT_NE(0, ares_getsock(chan, socks, ARES_GETSOCK_MAXNUM));

  ares_cancel(chan);
  EXPECT_TRUE(data.done);
  EXPECT_EQ(ARES_ECANCELLED, data.status);
  EXPECT_EQ((size_t)0, ares_queue_active_queries(chan));
  ares_destroy(chan);
}

// The connection the send function tore down has already been reported as
// closed, and its descriptor closed or handed to that notification.  Nothing
// after the send may report socket state for it: doing so names
// ARES_SOCKET_BAD, a descriptor the application was never given.
struct BadSocketStateData {
  size_t calls;
  size_t bad;
};

static void BadSocketStateCallback(void *data, ares_socket_t socket_fd,
                                   int readable, int writable)
{
  BadSocketStateData *d = static_cast<BadSocketStateData *>(data);
  (void)readable;
  (void)writable;
  d->calls++;
  if (socket_fd == ARES_SOCKET_BAD) {
    d->bad++;
  }
}

TEST_F(LibraryTest, ReconfigureInSendReportsNoBadSocket) {
  ares_channel_t         *chan = nullptr;
  struct ares_options     opts;
  int                     optmask = 0;
  ares_socket_functions   funcs;
  ReconfigureFromSendData data;
  BadSocketStateData      sock;

  memset(&opts, 0, sizeof(opts));
  memset(&sock, 0, sizeof(sock));
  opts.flags               = ARES_FLAG_NOSEARCH;
  optmask                 |= ARES_OPT_FLAGS;
  opts.sock_state_cb       = BadSocketStateCallback;
  opts.sock_state_cb_data  = &sock;
  optmask                 |= ARES_OPT_SOCK_STATE_CB;

  EXPECT_EQ(ARES_SUCCESS, ares_init_options(&chan, &opts, optmask));
  ASSERT_NE(nullptr, chan);
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(chan, "127.0.0.1:53055"));

  data.channel = chan;
  data.done    = false;
  data.status  = ARES_SUCCESS;
  memset(&funcs, 0, sizeof(funcs));
  funcs.asendv = ReconfigureSendv;
  ares_set_socket_functions(chan, &funcs, &data);

  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    ReconfigureFromSendCallback, &data, NULL);
  ares_cancel(chan);
  EXPECT_TRUE(data.done);

  EXPECT_NE((size_t)0, sock.calls);
  EXPECT_EQ((size_t)0, sock.bad);
  ares_destroy(chan);
}

// The same reconfiguration from the send function, reached through the
// deferred TCP write: ares_set_pending_write_cb() queues the write and the
// application later flushes it with ares_process_pending_write().  That flush
// is its own outermost library call, so the connection and server the
// reconfiguration closes must stay readable until it returns, and its walk of
// the server list must not step from a list node the reconfiguration freed.
// Covers both a send that reports success and one that fails.  A failure
// on the removed server must not be charged to the server that replaced it.
struct PendingWriteReconfigureData {
  ares_channel_t *channel;
  bool            reconfigure;
  bool            fail;
  bool            pending;
  size_t          done;
  size_t          server_failures;
};

static int PendingWriteConnect(ares_socket_t fd, const struct sockaddr *addr,
                               ares_socklen_t addrlen, void *user_data)
{
  (void)fd;
  (void)addr;
  (void)addrlen;
  (void)user_data;
  // Pretend the connection is in progress; the test signals it writable.
  return 0;
}

static ares_ssize_t PendingWriteSendv(ares_socket_t fd, const struct iovec *iov,
                                      int iovcnt, void *user_data)
{
  PendingWriteReconfigureData *d =
    static_cast<PendingWriteReconfigureData *>(user_data);
  ares_ssize_t len = 0;
  (void)fd;
  for (int i = 0; i < iovcnt; i++) {
    len += static_cast<ares_ssize_t>(iov[i].iov_len);
  }
  if (!d->reconfigure) {
    return len;
  }
  d->reconfigure = false;
  // Closes this connection and frees its server.
  ares_set_servers_csv(d->channel, "127.0.0.1:53056");
  if (d->fail) {
#ifdef USE_WINSOCK
    WSASetLastError(WSAECONNREFUSED);
#else
    errno = ECONNREFUSED;
#endif
    return -1;
  }
  return len;
}

static void PendingWriteNotify(void *user_data)
{
  PendingWriteReconfigureData *d =
    static_cast<PendingWriteReconfigureData *>(user_data);
  d->pending = true;
}

static void PendingWriteServerState(const char *server_string,
                                    ares_bool_t success, int flags,
                                    void *user_data)
{
  PendingWriteReconfigureData *d =
    static_cast<PendingWriteReconfigureData *>(user_data);
  (void)server_string;
  (void)flags;
  if (!success) {
    d->server_failures++;
  }
}

static void PendingWriteCallback(void *arg, ares_status_t status,
                                 size_t                   timeouts,
                                 const ares_dns_record_t *dnsrec)
{
  PendingWriteReconfigureData *d =
    static_cast<PendingWriteReconfigureData *>(arg);
  (void)timeouts;
  (void)dnsrec;
  EXPECT_EQ(ARES_ECANCELLED, status);
  d->done++;
}

static void ReconfigureInPendingWrite(bool fail)
{
  ares_channel_t             *chan = nullptr;
  struct ares_options         opts;
  int                         optmask = 0;
  ares_socket_functions       funcs;
  PendingWriteReconfigureData data;
  ares_socket_t               socks[ARES_GETSOCK_MAXNUM];
  int                         bitmask;

  memset(&opts, 0, sizeof(opts));
  opts.flags  = ARES_FLAG_NOSEARCH | ARES_FLAG_USEVC;
  optmask    |= ARES_OPT_FLAGS;

  EXPECT_EQ(ARES_SUCCESS, ares_init_options(&chan, &opts, optmask));
  ASSERT_NE(nullptr, chan);
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(chan, "127.0.0.1:53055"));

  data.channel         = chan;
  data.reconfigure     = false;
  data.fail            = fail;
  data.pending         = false;
  data.done            = 0;
  data.server_failures = 0;
  memset(&funcs, 0, sizeof(funcs));
  funcs.aconnect = PendingWriteConnect;
  funcs.asendv   = PendingWriteSendv;
  ares_set_socket_functions(chan, &funcs, &data);
  ares_set_pending_write_cb(chan, PendingWriteNotify, &data);
  ares_set_server_state_callback(chan, PendingWriteServerState, &data);

  // The first query opens the connection; its write waits for it to connect.
  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    PendingWriteCallback, &data, NULL);
  bitmask = ares_getsock(chan, socks, ARES_GETSOCK_MAXNUM);
  ASSERT_NE(0, ARES_GETSOCK_WRITABLE(bitmask, 0));

  // Report it connected, which writes the first query.
  ares_process_fd(chan, ARES_SOCKET_BAD, socks[0]);
  EXPECT_FALSE(data.pending);

  // Now connected, so the second query's write is deferred to the
  // application.
  ares_query_dnsrec(chan, "www.example.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    PendingWriteCallback, &data, NULL);
  EXPECT_TRUE(data.pending);

  data.reconfigure = true;
  ares_process_pending_write(chan);
  EXPECT_FALSE(data.reconfigure);

  // Both queries were moved to the new server, which has not failed.
  EXPECT_EQ((size_t)0, data.done);
  EXPECT_EQ((size_t)0, data.server_failures);
  EXPECT_EQ((size_t)2, ares_queue_active_queries(chan));

  ares_cancel(chan);
  EXPECT_EQ((size_t)2, data.done);
  EXPECT_EQ((size_t)0, ares_queue_active_queries(chan));
  ares_destroy(chan);
}

TEST_F(LibraryTest, ReconfigureInSuccessfulPendingWriteNoUseAfterFree) {
  ReconfigureInPendingWrite(false);
}

TEST_F(LibraryTest, ReconfigureInFailedPendingWriteNoUseAfterFree) {
  ReconfigureInPendingWrite(true);
}

// The pending-write callback is application code too, and runs once the query
// has been buffered on the connection.  Reconfiguring from it closes that
// connection, so the query must be requeued rather than attached to it.
static void PendingWriteReconfigureNotify(void *user_data)
{
  PendingWriteReconfigureData *d =
    static_cast<PendingWriteReconfigureData *>(user_data);
  d->pending = true;
  if (d->reconfigure) {
    d->reconfigure = false;
    // Closes the connection the query was just buffered on.
    ares_set_servers_csv(d->channel, "127.0.0.1:53056");
  }
}

TEST_F(LibraryTest, ReconfigureInPendingWriteCallbackNoUseAfterFree) {
  ares_channel_t             *chan = nullptr;
  struct ares_options         opts;
  int                         optmask = 0;
  ares_socket_functions       funcs;
  PendingWriteReconfigureData data;
  ares_socket_t               socks[ARES_GETSOCK_MAXNUM];
  int                         bitmask;

  memset(&opts, 0, sizeof(opts));
  opts.flags  = ARES_FLAG_NOSEARCH | ARES_FLAG_USEVC;
  optmask    |= ARES_OPT_FLAGS;

  EXPECT_EQ(ARES_SUCCESS, ares_init_options(&chan, &opts, optmask));
  ASSERT_NE(nullptr, chan);
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(chan, "127.0.0.1:53055"));

  data.channel         = chan;
  data.reconfigure     = false;
  data.fail            = false;
  data.pending         = false;
  data.done            = 0;
  data.server_failures = 0;
  memset(&funcs, 0, sizeof(funcs));
  funcs.aconnect = PendingWriteConnect;
  funcs.asendv   = PendingWriteSendv;
  ares_set_socket_functions(chan, &funcs, &data);
  ares_set_pending_write_cb(chan, PendingWriteReconfigureNotify, &data);

  // The first query opens the connection; its write waits for it to connect.
  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    PendingWriteCallback, &data, NULL);
  bitmask = ares_getsock(chan, socks, ARES_GETSOCK_MAXNUM);
  ASSERT_NE(0, ARES_GETSOCK_WRITABLE(bitmask, 0));

  // Report it connected, which writes the first query.
  ares_process_fd(chan, ARES_SOCKET_BAD, socks[0]);
  EXPECT_FALSE(data.pending);

  // Now connected, so the second query is buffered and the pending-write
  // callback runs, reconfiguring the servers.
  data.reconfigure = true;
  ares_query_dnsrec(chan, "www.example.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    PendingWriteCallback, &data, NULL);
  EXPECT_TRUE(data.pending);
  EXPECT_FALSE(data.reconfigure);

  // Both queries were moved to the new server.
  EXPECT_EQ((size_t)0, data.done);
  EXPECT_EQ((size_t)2, ares_queue_active_queries(chan));

  ares_cancel(chan);
  EXPECT_EQ((size_t)2, data.done);
  EXPECT_EQ((size_t)0, ares_queue_active_queries(chan));
  ares_destroy(chan);
}

// Fake socket functions, installed with ares_set_socket_functions_ex(), that
// never touch a real descriptor.  Each test arms one of them to reconfigure
// the servers part-way through c-ares's use of a connection, and they count
// every call that names ARES_SOCKET_BAD, which a closed connection must never
// pass to application code.
struct FakeSockData {
  ares_channel_t    *channel;
  ares_socket_t      next_fd;
  bool               tfo;
  bool               getsockname_fail_first;
  ares_socket_t      getsockname_last_fd;
  bool               getsockname_reconfigure;
  bool               recv_reconfigure;
  bool               recv_answer_then_reconfigure;
  bool               recv_answered;
  bool               recv_answer;
  bool               close_reconfigure;
  unsigned char      query[512];
  size_t             query_len;
  struct sockaddr_in peer;
  unsigned short     recv_port; /* network order; 0 reports the peer's port */
  bool               recv_no_port; /* report the peer's address with port 0 */
  size_t             recv_calls;
  size_t             bad_fd_calls;
  size_t             sock_state_bad;
  bool               done;
  int                status;
};

static void FakeSockSetErrno(bool wouldblock)
{
#ifdef USE_WINSOCK
  WSASetLastError(wouldblock ? WSAEWOULDBLOCK : WSAENOTCONN);
#else
  errno = wouldblock ? EAGAIN : ENOTCONN;
#endif
}

static ares_socket_t FakeSockSocket(int domain, int type, int protocol,
                                    void *user_data)
{
  FakeSockData *d = static_cast<FakeSockData *>(user_data);
  (void)domain;
  (void)type;
  (void)protocol;
  return d->next_fd++;
}

static int FakeSockClose(ares_socket_t sock, void *user_data)
{
  FakeSockData *d = static_cast<FakeSockData *>(user_data);
  if (sock == ARES_SOCKET_BAD) {
    d->bad_fd_calls++;
  }
  if (d->close_reconfigure) {
    d->close_reconfigure = false;
    // Frees the server this connection belonged to.
    ares_set_servers_csv(d->channel, "127.0.0.1:53056");
  }
  return 0;
}

static int FakeSockSetsockopt(ares_socket_t sock, ares_socket_opt_t opt,
                              const void *val, ares_socklen_t val_size,
                              void *user_data)
{
  FakeSockData *d = static_cast<FakeSockData *>(user_data);
  (void)sock;
  (void)val;
  (void)val_size;
  if (opt == ARES_SOCKET_OPT_TCP_FASTOPEN && !d->tfo) {
    return -1;
  }
  return 0;
}

static int FakeSockConnect(ares_socket_t sock, const struct sockaddr *address,
                           ares_socklen_t address_len, unsigned int flags,
                           void *user_data)
{
  FakeSockData *d = static_cast<FakeSockData *>(user_data);
  (void)sock;
  (void)flags;
  if (address->sa_family == AF_INET && address_len >= sizeof(d->peer)) {
    memcpy(&d->peer, address, sizeof(d->peer));
  }
  return 0;
}

static ares_ssize_t FakeSockRecvfrom(ares_socket_t sock, void *buffer,
                                     size_t length, int flags,
                                     struct sockaddr *address,
                                     ares_socklen_t  *address_len,
                                     void            *user_data)
{
  FakeSockData *d = static_cast<FakeSockData *>(user_data);
  (void)flags;
  d->recv_calls++;
  if (sock == ARES_SOCKET_BAD) {
    d->bad_fd_calls++;
  }
  if (d->query_len < 12 || length < d->query_len) {
    FakeSockSetErrno(true);
    return -1;
  }
  if (d->recv_answer_then_reconfigure) {
    // Answer first; reconfigure on the next call, which has nothing to read.
    if (d->recv_answered) {
      d->recv_answer_then_reconfigure = false;
      ares_set_servers_csv(d->channel, "127.0.0.1:53056");
      FakeSockSetErrno(true);
      return -1;
    }
    d->recv_answered = true;
  } else if (d->recv_reconfigure) {
    d->recv_reconfigure = false;
    // Frees the server this connection belongs to.
    ares_set_servers_csv(d->channel, "127.0.0.1:53056");
  } else if (d->recv_answer) {
    d->recv_answer = false;
  } else {
    FakeSockSetErrno(true);
    return -1;
  }
  // Answer the query: the same message with QR set, no records.
  memcpy(buffer, d->query, d->query_len);
  static_cast<unsigned char *>(buffer)[2] |= 0x80;
  if (address != NULL && address_len != NULL &&
      *address_len >= (ares_socklen_t)sizeof(d->peer)) {
    memcpy(address, &d->peer, sizeof(d->peer));
    if (d->recv_no_port) {
      reinterpret_cast<struct sockaddr_in *>(address)->sin_port = 0;
    } else if (d->recv_port != 0) {
      reinterpret_cast<struct sockaddr_in *>(address)->sin_port = d->recv_port;
    }
    *address_len = sizeof(d->peer);
  }
  return static_cast<ares_ssize_t>(d->query_len);
}

static ares_ssize_t FakeSockSendto(ares_socket_t sock, const void *buffer,
                                   size_t length, int flags,
                                   const struct sockaddr *address,
                                   ares_socklen_t address_len, void *user_data)
{
  FakeSockData *d = static_cast<FakeSockData *>(user_data);
  (void)flags;
  (void)address;
  (void)address_len;
  if (sock == ARES_SOCKET_BAD) {
    d->bad_fd_calls++;
  }
  if (length <= sizeof(d->query)) {
    memcpy(d->query, buffer, length);
    d->query_len = length;
  }
  return static_cast<ares_ssize_t>(length);
}

static int FakeSockGetsockname(ares_socket_t sock, struct sockaddr *address,
                               ares_socklen_t *address_len, void *user_data)
{
  FakeSockData      *d = static_cast<FakeSockData *>(user_data);
  struct sockaddr_in sin;
  if (sock == ARES_SOCKET_BAD) {
    d->bad_fd_calls++;
  }
  // As during TCP Fast Open, before the first write has connected.
  if (d->getsockname_fail_first && sock != d->getsockname_last_fd) {
    d->getsockname_last_fd = sock;
    FakeSockSetErrno(false);
    return -1;
  }
  if (d->getsockname_reconfigure) {
    d->getsockname_reconfigure = false;
    // Frees the server this connection belongs to.
    ares_set_servers_csv(d->channel, "127.0.0.1:53056");
  }
  if (*address_len < (ares_socklen_t)sizeof(sin)) {
    FakeSockSetErrno(false);
    return -1;
  }
  memset(&sin, 0, sizeof(sin));
  sin.sin_family      = AF_INET;
  sin.sin_port        = htons(40000);
  sin.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  memcpy(address, &sin, sizeof(sin));
  *address_len = sizeof(sin);
  return 0;
}

static void FakeSockStateCallback(void *data, ares_socket_t socket_fd,
                                  int readable, int writable)
{
  FakeSockData *d = static_cast<FakeSockData *>(data);
  (void)readable;
  (void)writable;
  if (socket_fd == ARES_SOCKET_BAD) {
    d->sock_state_bad++;
  }
}

static void FakeSockQueryCallback(void *arg, ares_status_t status,
                                  size_t                   timeouts,
                                  const ares_dns_record_t *dnsrec)
{
  FakeSockData *d = static_cast<FakeSockData *>(arg);
  (void)timeouts;
  (void)dnsrec;
  d->done   = true;
  d->status = static_cast<int>(status);
}

static void FakeSockInit(ares_channel_t **chan, FakeSockData *data,
                         int extra_flags, unsigned int sockfunc_flags,
                         bool sock_state = true)
{
  struct ares_options             opts;
  int                             optmask = 0;
  struct ares_socket_functions_ex funcs;

  memset(&opts, 0, sizeof(opts));
  opts.flags  = ARES_FLAG_NOSEARCH | extra_flags;
  optmask    |= ARES_OPT_FLAGS;
  // A socket-state callback defers the close of a connection's descriptor
  // until the outermost call unwinds; without one it is closed inline.
  if (sock_state) {
    opts.sock_state_cb       = FakeSockStateCallback;
    opts.sock_state_cb_data  = data;
    optmask                 |= ARES_OPT_SOCK_STATE_CB;
  }
  ASSERT_EQ(ARES_SUCCESS, ares_init_options(chan, &opts, optmask));
  ASSERT_NE(nullptr, *chan);
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(*chan, "127.0.0.1:53055"));

  data->channel             = *chan;
  data->next_fd             = 100;
  data->getsockname_last_fd = ARES_SOCKET_BAD;

  memset(&funcs, 0, sizeof(funcs));
  funcs.version      = 1;
  funcs.flags        = sockfunc_flags;
  funcs.asocket      = FakeSockSocket;
  funcs.aclose       = FakeSockClose;
  funcs.asetsockopt  = FakeSockSetsockopt;
  funcs.aconnect     = FakeSockConnect;
  funcs.arecvfrom    = FakeSockRecvfrom;
  funcs.asendto      = FakeSockSendto;
  funcs.agetsockname = FakeSockGetsockname;
  EXPECT_EQ(ARES_SUCCESS, ares_set_socket_functions_ex(*chan, &funcs, data));
}

// The getsockname function runs for every new connection, after the checks
// that follow the socket-configure and socket-create callbacks.  Reconfiguring
// from it must not leave the connection registered on the removed server.
TEST_F(LibraryTest, ReconfigureInGetsocknameNoUseAfterFree) {
  ares_channel_t *chan = nullptr;
  FakeSockData    data;

  memset(&data, 0, sizeof(data));
  FakeSockInit(&chan, &data, 0, 0);
  data.getsockname_reconfigure = true;

  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    FakeSockQueryCallback, &data, NULL);
  EXPECT_FALSE(data.getsockname_reconfigure);

  // The first connection's descriptor: no longer known to the channel.
  ares_process_fd(chan, 100, ARES_SOCKET_BAD);

  // Moved to the new server.
  EXPECT_FALSE(data.done);
  EXPECT_EQ((size_t)1, ares_queue_active_queries(chan));

  ares_cancel(chan);
  EXPECT_TRUE(data.done);
  EXPECT_EQ(ARES_ECANCELLED, data.status);
  EXPECT_EQ((size_t)0, data.bad_fd_calls);
  EXPECT_EQ((size_t)0, data.sock_state_bad);
  ares_destroy(chan);
}

// With TCP Fast Open the first getsockname may fail, and is retried after the
// first write -- from inside ares_conn_write(), after the send function has
// returned.  Reconfiguring from that retry closes the connection the query was
// just written on.
TEST_F(LibraryTest, ReconfigureInFastOpenGetsocknameNoUseAfterFree) {
  ares_channel_t *chan = nullptr;
  FakeSockData    data;

  memset(&data, 0, sizeof(data));
  FakeSockInit(&chan, &data, ARES_FLAG_USEVC, 0);
  data.tfo                     = true;
  data.getsockname_fail_first  = true;
  data.getsockname_reconfigure = true;

  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    FakeSockQueryCallback, &data, NULL);
  EXPECT_FALSE(data.getsockname_reconfigure);

  // Moved to the new server.
  EXPECT_FALSE(data.done);
  EXPECT_EQ((size_t)1, ares_queue_active_queries(chan));
  EXPECT_EQ((size_t)0, data.sock_state_bad);

  ares_cancel(chan);
  EXPECT_TRUE(data.done);
  EXPECT_EQ(ARES_ECANCELLED, data.status);
  EXPECT_EQ((size_t)0, data.bad_fd_calls);
  ares_destroy(chan);
}

// A non-blocking receive function that reconfigures the servers and returns
// an answer.  The connection it read from is closed, so the answer must be
// dropped -- the query has already moved to the new server -- and the
// receive function must not be called again on the closed connection.
TEST_F(LibraryTest, ReconfigureInRecvDropsStaleAnswer) {
  ares_channel_t *chan = nullptr;
  FakeSockData    data;

  memset(&data, 0, sizeof(data));
  FakeSockInit(&chan, &data, 0, ARES_SOCKFUNC_FLAG_NONBLOCKING);

  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    FakeSockQueryCallback, &data, NULL);
  ASSERT_NE((size_t)0, data.query_len);

  data.recv_reconfigure = true;
  ares_process_fd(chan, 100, ARES_SOCKET_BAD);
  EXPECT_FALSE(data.recv_reconfigure);

  EXPECT_EQ((size_t)1, data.recv_calls);
  EXPECT_EQ((size_t)0, data.bad_fd_calls);
  // Still in flight on the new server.
  EXPECT_FALSE(data.done);
  EXPECT_EQ((size_t)1, ares_queue_active_queries(chan));

  ares_cancel(chan);
  EXPECT_TRUE(data.done);
  EXPECT_EQ(ARES_ECANCELLED, data.status);
  ares_destroy(chan);
}

// As above, but the answer comes from an earlier receive call, and a later
// call -- which has nothing more to read -- reconfigures the servers.  The
// answer is already buffered on the connection that call tore down, and must
// be dropped with it.
TEST_F(LibraryTest, ReconfigureInRecvDropsBufferedAnswer) {
  ares_channel_t *chan = nullptr;
  FakeSockData    data;

  memset(&data, 0, sizeof(data));
  FakeSockInit(&chan, &data, 0, ARES_SOCKFUNC_FLAG_NONBLOCKING);

  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    FakeSockQueryCallback, &data, NULL);
  ASSERT_NE((size_t)0, data.query_len);

  data.recv_answer_then_reconfigure = true;
  ares_process_fd(chan, 100, ARES_SOCKET_BAD);
  EXPECT_FALSE(data.recv_answer_then_reconfigure);

  EXPECT_EQ((size_t)2, data.recv_calls);
  EXPECT_EQ((size_t)0, data.bad_fd_calls);
  // Still in flight on the new server.
  EXPECT_FALSE(data.done);
  EXPECT_EQ((size_t)1, ares_queue_active_queries(chan));

  ares_cancel(chan);
  EXPECT_TRUE(data.done);
  EXPECT_EQ(ARES_ECANCELLED, data.status);
  ares_destroy(chan);
}

// Socket functions need not filter on connect(), so ares_conn_read() checks
// a UDP datagram's source itself.  An answer reported from the server's
// address but another port (here 127.0.0.1:53 for a server at
// 127.0.0.1:53055) must be dropped, so that one server cannot answer for
// another on the same address.  The same answer from the right port is then
// accepted.
TEST_F(LibraryTest, UDPAnswerFromWrongPortDropped) {
  ares_channel_t *chan = nullptr;
  FakeSockData    data;

  memset(&data, 0, sizeof(data));
  FakeSockInit(&chan, &data, 0, ARES_SOCKFUNC_FLAG_NONBLOCKING);

  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    FakeSockQueryCallback, &data, NULL);
  ASSERT_NE((size_t)0, data.query_len);
  ASSERT_EQ(htons(53055), data.peer.sin_port);

  data.recv_port   = htons(53);
  data.recv_answer = true;
  ares_process_fd(chan, 100, ARES_SOCKET_BAD);
  EXPECT_FALSE(data.recv_answer);
  EXPECT_FALSE(data.done);
  EXPECT_EQ((size_t)1, ares_queue_active_queries(chan));

  data.recv_port   = 0;
  data.recv_answer = true;
  ares_process_fd(chan, 100, ARES_SOCKET_BAD);
  EXPECT_FALSE(data.recv_answer);
  EXPECT_TRUE(data.done);
  // The fake answer carries no records.
  EXPECT_EQ(ARES_ENODATA, data.status);
  EXPECT_EQ((size_t)0, ares_queue_active_queries(chan));
  ares_destroy(chan);
}

// Socket functions written before the port check may fill in the source
// address but leave the port 0.  The address is still checked, but the answer
// must not be dropped for its port.
TEST_F(LibraryTest, UDPAnswerWithoutPortAccepted)
{
  ares_channel_t *chan = nullptr;
  FakeSockData    data;

  memset(&data, 0, sizeof(data));
  FakeSockInit(&chan, &data, 0, ARES_SOCKFUNC_FLAG_NONBLOCKING);

  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    FakeSockQueryCallback, &data, NULL);
  ASSERT_NE((size_t)0, data.query_len);
  ASSERT_EQ(htons(53055), data.peer.sin_port);

  data.recv_no_port = true;
  data.recv_answer  = true;
  ares_process_fd(chan, 100, ARES_SOCKET_BAD);
  EXPECT_FALSE(data.recv_answer);
  EXPECT_TRUE(data.done);
  // The fake answer carries no records.
  EXPECT_EQ(ARES_ENODATA, data.status);
  EXPECT_EQ((size_t)0, ares_queue_active_queries(chan));
  ares_destroy(chan);
}

// The close function is application code too.  With no socket-state callback
// to carry the descriptor, it runs inline while ares_check_cleanup_conns()
// walks the server list and each server's connections, closing idle ones.
// Reconfiguring from it frees the list nodes that walk holds.
TEST_F(LibraryTest, ReconfigureInCloseNoUseAfterFree) {
  ares_channel_t *chan = nullptr;
  FakeSockData    data;

  memset(&data, 0, sizeof(data));
  FakeSockInit(&chan, &data, 0, 0, false);

  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    FakeSockQueryCallback, &data, NULL);
  ASSERT_NE((size_t)0, data.query_len);

  // Answer it; the now idle connection is then closed.
  data.recv_answer       = true;
  data.close_reconfigure = true;
  ares_process_fd(chan, 100, ARES_SOCKET_BAD);
  EXPECT_FALSE(data.recv_answer);
  EXPECT_FALSE(data.close_reconfigure);

  EXPECT_TRUE(data.done);
  // The fake answer carries no records.
  EXPECT_EQ(ARES_ENODATA, data.status);
  EXPECT_EQ((size_t)0, ares_queue_active_queries(chan));
  EXPECT_EQ((size_t)0, data.bad_fd_calls);
  ares_destroy(chan);
}

// The second route to the same freed connection, and the one that drove the
// change from suppressing the idle sweep to deferring the notification:
// reconfiguring the servers from the socket-state callback destroys the server
// and with it the connection ares_open_connection() is still using.  Nothing
// covered this route, so a regression in it would have gone unnoticed.
struct SockStateReconfigureData {
  ares_channel_t *channel;
  size_t          calls;
};

static void SockStateReconfigureCallback(void *data, ares_socket_t socket_fd,
                                         int readable, int writable)
{
  SockStateReconfigureData *d =
    static_cast<SockStateReconfigureData *>(data);
  (void)socket_fd;
  (void)readable;
  (void)writable;
  d->calls++;
  if (d->channel != nullptr) {
    // Destroys the server, and with it the connection being opened.
    ares_set_servers_csv(d->channel, "127.0.0.1:53056");
  }
}

TEST_F(LibraryTest, ReconfigureInSockStateCallbackNoUseAfterFree) {
  SockStateReconfigureData data;
  ares_channel_t          *chan = nullptr;
  struct ares_options      opts;

  data.channel = nullptr;
  data.calls   = 0;

  memset(&opts, 0, sizeof(opts));
  opts.sock_state_cb      = SockStateReconfigureCallback;
  opts.sock_state_cb_data = &data;

  EXPECT_EQ(ARES_SUCCESS,
            ares_init_options(&chan, &opts, ARES_OPT_SOCK_STATE_CB));
  ASSERT_NE(nullptr, chan);
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(chan, "127.0.0.1:53055"));

  /* Armed only now, so channel setup itself does not reconfigure. */
  data.channel = chan;

  QueryResult result;
  ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    QueryCallback, &result, NULL);

  EXPECT_GT(data.calls, (size_t)0);

  data.channel = nullptr; /* don't reconfigure from teardown notifications */
  ares_destroy(chan);
}
// Server-state notifications raised while a public call is issuing its first
// request must not be delivered until that whole call has unwound.  A
// failover handler that reconfigures the servers ends the in-flight request,
// and with it the state the issuing call still has to finish with.  The IPv6
// server cannot get a socket, so the first send fails over to the IPv4 one
// and queues a failure notification for the handler below.
static ares_socket_t FailoverNoIPv6Socket(int af, int type, int protocol,
                                          void *user_data)
{
  (void)user_data;
  if (af == AF_INET6) {
    return ARES_SOCKET_BAD;
  }
  return socket(af, type, protocol);
}

static void FailoverOnServerDown(const char *server_string, ares_bool_t success,
                                 int flags, void *data)
{
  (void)server_string;
  (void)flags;
  if (!success) {
    ares_set_servers_csv(static_cast<ares_channel_t *>(data),
                         "127.0.0.1:53063");
  }
}

static ares_channel_t *FailoverChannel(ares_socket_functions *funcs)
{
  ares_channel_t     *chan = nullptr;
  struct ares_options opts;

  memset(&opts, 0, sizeof(opts));
  opts.tries = 1;
  opts.flags = ARES_FLAG_NOSEARCH;
  EXPECT_EQ(ARES_SUCCESS,
            ares_init_options(&chan, &opts, ARES_OPT_TRIES | ARES_OPT_FLAGS));
  if (chan == nullptr) {
    return nullptr;
  }
  EXPECT_EQ(ARES_SUCCESS,
            ares_set_servers_csv(chan, "[::1]:53061,127.0.0.1:53062"));
  ares_set_server_state_callback(chan, FailoverOnServerDown, chan);

  memset(funcs, 0, sizeof(*funcs));
  funcs->asocket = FailoverNoIPv6Socket;
  ares_set_socket_functions(chan, funcs, NULL);
  return chan;
}

TEST_F(LibraryTest, ServerFailoverDuringGetAddrInfo) {
  ares_socket_functions      funcs;
  struct ares_addrinfo_hints hints;
  AddrInfoResult             result;
  ares_channel_t            *chan = FailoverChannel(&funcs);
  ASSERT_NE(nullptr, chan);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  ares_getaddrinfo(chan, "www.google.com", NULL, &hints, AddrInfoCallback,
                   &result);

  ares_cancel(chan);
  EXPECT_TRUE(result.done_);
  ares_destroy(chan);
}

// Same, through ares_query_dnsrec(): the query id must already have been
// reported to the caller by the time the query can complete.
struct FailoverQueryData {
  unsigned short qid;
  unsigned short qid_at_callback;
  bool           done;
};

static void FailoverQueryCallback(void *data, ares_status_t status,
                                  size_t                   timeouts,
                                  const ares_dns_record_t *dnsrec)
{
  FailoverQueryData *d = static_cast<FailoverQueryData *>(data);
  (void)status;
  (void)timeouts;
  (void)dnsrec;
  d->qid_at_callback = d->qid;
  d->done            = true;
}

TEST_F(LibraryTest, ServerFailoverDuringQuery) {
  ares_socket_functions funcs;
  FailoverQueryData     data;
  ares_channel_t       *chan = FailoverChannel(&funcs);
  ASSERT_NE(nullptr, chan);

  data.qid             = 0;
  data.qid_at_callback = 0;
  data.done            = false;
  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(chan, "www.google.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, FailoverQueryCallback, &data,
                              &data.qid));

  EXPECT_TRUE(data.done);
  EXPECT_EQ(data.qid, data.qid_at_callback);
  ares_destroy(chan);
}


// Deferring the notifications must not lose them: every public entry point
// that closes a socket still has to tell the application about it before it
// returns, or an event loop is left polling a descriptor that c-ares closed.
struct SockStateRecordData {
  std::vector<std::pair<ares_socket_t, bool>> events; /* fd, still-armed */
};

static void SockStateRecordCallback(void *data, ares_socket_t socket_fd,
                                    int readable, int writable)
{
  SockStateRecordData *d = static_cast<SockStateRecordData *>(data);
  d->events.push_back(std::make_pair(socket_fd, readable || writable));
}

static bool SawClose(const SockStateRecordData &d)
{
  for (size_t i = 0; i < d.events.size(); i++) {
    if (!d.events[i].second) {
      return true;
    }
  }
  return false;
}

class SockStateDeliveryTest : public LibraryTest {
protected:
  void OpenConnection(ares_channel_t **chan, SockStateRecordData *data,
                      QueryResult *result)
  {
    struct ares_options opts;
    memset(&opts, 0, sizeof(opts));
    opts.sock_state_cb      = SockStateRecordCallback;
    opts.sock_state_cb_data = data;

    ASSERT_EQ(ARES_SUCCESS,
              ares_init_options(chan, &opts, ARES_OPT_SOCK_STATE_CB));
    ASSERT_NE(nullptr, *chan);
    /* TEST-NET-1, guaranteed not to answer, so the socket is still open when
     * the call under test runs. */
    ASSERT_EQ(ARES_SUCCESS, ares_set_servers_csv(*chan, "192.0.2.1:53"));

    ares_query_dnsrec(*chan, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                      QueryCallback, result, NULL);
    ASSERT_FALSE(result->done_);
    ASSERT_FALSE(data->events.empty());
    ASSERT_TRUE(data->events.back().second); /* opened, armed for read */
    data->events.clear();
  }
};

TEST_F(SockStateDeliveryTest, CancelReportsSocketClose) {
  ares_channel_t     *chan = nullptr;
  SockStateRecordData data;
  QueryResult         result;

  OpenConnection(&chan, &data, &result);
  ares_cancel(chan);
  EXPECT_TRUE(SawClose(data)) << "ares_cancel() closed the socket without "
                                 "telling the application";
  ares_destroy(chan);
}

TEST_F(SockStateDeliveryTest, SetServersReportsSocketClose) {
  ares_channel_t     *chan = nullptr;
  SockStateRecordData data;
  QueryResult         result;

  OpenConnection(&chan, &data, &result);
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(chan, "198.51.100.1:53"));
  EXPECT_TRUE(SawClose(data)) << "ares_set_servers_csv() closed the socket "
                                 "without telling the application";
  ares_destroy(chan);
}

TEST_F(SockStateDeliveryTest, DestroyReportsSocketClose) {
  ares_channel_t     *chan = nullptr;
  SockStateRecordData data;
  QueryResult         result;

  OpenConnection(&chan, &data, &result);
  ares_destroy(chan);
  EXPECT_TRUE(SawClose(data)) << "ares_destroy() closed the socket without "
                                 "telling the application";
}

// The server-state callback is dispatched from the middle of processing, and
// every caller keeps using its query afterwards.  Nothing documents the
// callback as non-reentrant, and the channel lock is recursive precisely so
// API calls from callbacks are allowed -- so an application that cancels from
// it left the library using a freed query.
//
// No hostile input is needed: an ordinary timeout reaches the same place.  A
// SERVFAIL is used here because it is deterministic in the harness.
struct ServerStateCancelData {
  ares_channel_t *channel;
  int             server_cb_calls;
  bool            query_done;
};

static void ServerStateCancelCallback(const char *server_string,
                                      ares_bool_t success, int flags,
                                      void *data) {
  ServerStateCancelData *d = static_cast<ServerStateCancelData *>(data);
  (void)server_string;
  (void)success;
  (void)flags;
  d->server_cb_calls++;
  // Reentrant cancel from the notification.
  ares_cancel(d->channel);
}

static void ServerStateQueryCallback(void *arg, ares_status_t status,
                                     size_t timeouts,
                                     const ares_dns_record_t *dnsrec) {
  ServerStateCancelData *d = static_cast<ServerStateCancelData *>(arg);
  (void)status;
  (void)timeouts;
  (void)dnsrec;
  d->query_done = true;
}

// The advisory's headline claim is that no hostile input is needed, because an
// ordinary timeout reaches the same place.  The SERVFAIL test below does not
// show that; this one does.
class ServerStateTimeoutCancelTest
  : public MockChannelOptsTest,
    public ::testing::WithParamInterface<int> {
 public:
  ServerStateTimeoutCancelTest()
    : MockChannelOptsTest(1, GetParam(), false, false, FillOptions(&opts_),
                          ARES_OPT_TIMEOUTMS | ARES_OPT_TRIES) {}
  static struct ares_options *FillOptions(struct ares_options *opts) {
    memset(opts, 0, sizeof(struct ares_options));
    opts->timeout = 100;
    opts->tries   = 3;
    return opts;
  }

 private:
  struct ares_options opts_;
};

TEST_P(ServerStateTimeoutCancelTest, CancelInServerStateCallbackOnTimeout) {
  ServerStateCancelData data;
  data.channel         = channel_;
  data.server_cb_calls = 0;
  data.query_done      = false;

  ares_set_server_state_callback(channel_, ServerStateCancelCallback, &data);

  // No reply configured, so the query times out and the notification comes
  // from process_timeouts() rather than from a response.
  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, ServerStateQueryCallback, &data,
                              NULL));
  Process();

  EXPECT_GT(data.server_cb_calls, 0);
  EXPECT_TRUE(data.query_done);
}

INSTANTIATE_TEST_SUITE_P(AddressFamilies, ServerStateTimeoutCancelTest,
                         ::testing::ValuesIn(ares::test::families),
                         PrintFamily);

// Unregistering with a NULL callback discards whatever is still queued -- the
// documented behaviour of deferring delivery, and the only way an application
// can opt back out once notifications are in flight.
struct ServerStateUnregisterData {
  ares_channel_t *channel;
  size_t          server_cb_calls;
  bool            query_done;
};

static void ServerStateUnregisterCallback(const char *server_string,
                                          ares_bool_t success,
                                          int flags, void *user_data)
{
  ServerStateUnregisterData *d =
    static_cast<ServerStateUnregisterData *>(user_data);
  (void)server_string;
  (void)success;
  (void)flags;
  d->server_cb_calls++;
  // Clear ourselves from inside the first notification.  Any notification
  // still queued behind this one must be dropped rather than delivered.
  ares_set_server_state_callback(d->channel, NULL, NULL);
}

static void ServerStateUnregisterQueryCallback(void *arg, ares_status_t status,
                                               size_t timeouts,
                                               const ares_dns_record_t *dnsrec)
{
  ServerStateUnregisterData *d =
    static_cast<ServerStateUnregisterData *>(arg);
  (void)status;
  (void)timeouts;
  (void)dnsrec;
  d->query_done = true;
}

TEST_P(ServerStateTimeoutCancelTest, UnregisterInCallbackDiscardsQueued) {
  ServerStateUnregisterData data;
  data.channel         = channel_;
  data.server_cb_calls = 0;
  data.query_done      = false;

  ares_set_server_state_callback(channel_, ServerStateUnregisterCallback,
                                 &data);

  // No reply configured: the query times out on every try, so more than one
  // notification is queued before the flush point is reached.
  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A,
                              ServerStateUnregisterQueryCallback, &data, NULL));
  Process();

  EXPECT_EQ((size_t)1, data.server_cb_calls);
  EXPECT_TRUE(data.query_done);
}

TEST_P(MockUDPChannelTest, CancelInServerStateCallbackNoUseAfterFree) {
  DNSPacket servfail;
  servfail.set_response().set_aa().set_rcode(SERVFAIL)
    .add_question(new DNSQuestion("www.google.com", T_A));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &servfail));

  ServerStateCancelData data;
  data.channel         = channel_;
  data.server_cb_calls = 0;
  data.query_done      = false;

  ares_set_server_state_callback(channel_, ServerStateCancelCallback, &data);

  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    ServerStateQueryCallback, &data, NULL);
  Process();

  EXPECT_GT(data.server_cb_calls, 0);
  EXPECT_TRUE(data.query_done);
}

// The deferred requeue list records the target server as a raw pointer, and
// the drain never revalidated it -- although it does revalidate the query, by
// qid, for exactly the same reason.  Application callbacks run from inside that
// drain, and a callback may legally reconfigure the servers, which frees the
// ones no longer in the list.  A later entry in the same drain then dispatches
// through a freed ares_server_t.
//
// Two replies must be read in one batch: an ordinary answer whose callback
// reconfigures, and a FORMERR carrying no OPT record, which is what makes
// process_answer() queue a retry *to the same server* (the EDNS fallback --
// the only site that records a server pointer at all).
struct ReconfigureInCbData {
  ares_channel_t *channel;
  bool            done;
};

static void ReconfigureChannelCallback(void *arg, ares_status_t status,
                                       size_t timeouts,
                                       const ares_dns_record_t *dnsrec) {
  ReconfigureInCbData *data = static_cast<ReconfigureInCbData *>(arg);
  (void)status;
  (void)timeouts;
  (void)dnsrec;
  data->done = true;
  /* Frees every server absent from the new configuration. */
  ares_set_servers_csv(data->channel, "127.0.0.1:53055");
}

TEST_P(MockUDPChannelTest, ReconfigureInCallbackStaleRequeueServer) {
  DNSPacket okrsp;
  okrsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 100, {1, 2, 3, 4}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &okrsp));

  /* FORMERR with no OPT record: the server looks like it does not understand
   * EDNS, so the query is queued for retry to this same server. */
  DNSPacket formerr;
  formerr.set_response().set_aa().set_rcode(FORMERR)
    .add_question(new DNSQuestion("www.example.com", T_A));
  ON_CALL(server_, OnRequest("www.example.com", T_A))
    .WillByDefault(SetReply(&server_, &formerr));

  ReconfigureInCbData data;
  data.channel = channel_;
  data.done    = false;

  QueryResult second;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    ReconfigureChannelCallback, &data, NULL);
  ares_query_dnsrec(channel_, "www.example.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    QueryCallback, &second, NULL);

  /* Both replies have to be consumed in a single pass so they share one
   * deferred-requeue scope -- that is what puts the ENDQUERY entry whose
   * callback reconfigures ahead of the REQUEUE entry holding a server
   * pointer.  The usual Process() loop lets c-ares read between the two, so
   * they never share a batch; answer both first, then read once. */
  for (int i = 0; i < 4; i++) {
    std::set<ares_socket_t> server_fds = fds();
    fd_set                  r;
    struct timeval          tv   = {0, 20000};
    int                     nfds = 0;
    FD_ZERO(&r);
    for (ares_socket_t fd : server_fds) {
      FD_SET(fd, &r);
      if ((int)fd >= nfds) {
        nfds = (int)fd + 1;
      }
    }
    if (select(nfds, &r, nullptr, nullptr, &tv) <= 0) {
      break;
    }
    for (ares_socket_t fd : server_fds) {
      if (FD_ISSET(fd, &r)) {
        ProcessFD(fd);
      }
    }
  }
  {
    fd_set         readers;
    fd_set         writers;
    struct timeval tv = {0, 0};
    int            nfds;
    FD_ZERO(&readers);
    FD_ZERO(&writers);
    nfds = ares_fds(channel_, &readers, &writers);
    select(nfds, &readers, &writers, nullptr, &tv);
    ares_process(channel_, &readers, &writers);
  }

  EXPECT_TRUE(data.done);

  /* With the fix the FORMERR query is correctly re-dispatched (to the newly
   * configured, unreachable server) rather than going through a freed one, so
   * it is still outstanding here.  Cancel it so its callback runs while its
   * result object is still in scope. */
  ares_cancel(channel_);
  EXPECT_TRUE(second.done_);
}

// ares_servers_remove_stale() destroys each stale server in turn, and that
// reaches application callbacks (ares_destroy_server() -> ares_close_sockets()
// -> ares_requeue_queries() -> end_query()).  A callback that reconfigures the
// servers again destroys the servers the outer loop has not visited yet, so a
// cached "next" node points at freed memory.
struct NestedReconfigureData {
  ares_channel_t *channel;
  bool            nested;
};

static void NestedReconfigureCallback(void *arg, ares_status_t status,
                                      size_t                   timeouts,
                                      const ares_dns_record_t *dnsrec) {
  NestedReconfigureData *data = static_cast<NestedReconfigureData *>(arg);
  (void)status;
  (void)timeouts;
  (void)dnsrec;
  if (data->nested) {
    return;
  }
  data->nested = true;
  // Destroys the stale servers the outer removal loop has not reached yet.
  ares_set_servers_csv(data->channel, "");
}

TEST_F(LibraryTest, NestedReconfigureInCallbackRemoveStale) {
  ares_channel_t     *channel = nullptr;
  struct ares_options opts;
  int                 optmask = 0;

  memset(&opts, 0, sizeof(opts));
  /* One try per server, so tearing them down exhausts the query's retries and
   * runs its callback from inside the removal loop. */
  opts.tries  = 1;
  optmask    |= ARES_OPT_TRIES;
  opts.flags  = ARES_FLAG_NOSEARCH;
  optmask    |= ARES_OPT_FLAGS;

  EXPECT_EQ(ARES_SUCCESS, ares_init_options(&channel, &opts, optmask));
  /* Three servers, so the loop still has entries after destroying the first.
   * Nothing listens on these ports; no traffic is needed. */
  EXPECT_EQ(ARES_SUCCESS,
            ares_set_servers_csv(channel, "127.0.0.1:53101,127.0.0.1:53102,"
                                          "127.0.0.1:53103"));

  NestedReconfigureData data;
  data.channel = channel;
  data.nested  = false;

  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel, "www.example.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, NestedReconfigureCallback,
                              &data, NULL));

  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(channel, ""));
  EXPECT_TRUE(data.nested);

  ares_destroy(channel);
}

// The same nesting, but the nested reconfiguration *installs* a server rather
// than clearing the list.  Restarting the removal walk from the head then
// re-judges that server against the outer call's list, decides it is stale and
// deletes it -- so ares_set_servers_csv() returns success and leaves the
// channel with no servers at all.  Memory-safe and still wrong.
static void NestedReplaceCallback(void *arg, ares_status_t status,
                                  size_t                   timeouts,
                                  const ares_dns_record_t *dnsrec) {
  NestedReconfigureData *data = static_cast<NestedReconfigureData *>(arg);
  (void)status;
  (void)timeouts;
  (void)dnsrec;
  if (data->nested) {
    return;
  }
  data->nested = true;
  // Installs a server the outer loop has never seen.
  ares_set_servers_csv(data->channel, "127.0.0.1:53201");
}

TEST_F(LibraryTest, NestedReconfigureInCallbackKeepsNestedServers) {
  ares_channel_t     *channel = nullptr;
  struct ares_options opts;
  int                 optmask = 0;

  memset(&opts, 0, sizeof(opts));
  opts.tries  = 1;
  optmask    |= ARES_OPT_TRIES;
  opts.flags  = ARES_FLAG_NOSEARCH;
  optmask    |= ARES_OPT_FLAGS;

  EXPECT_EQ(ARES_SUCCESS, ares_init_options(&channel, &opts, optmask));
  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(channel, "127.0.0.1:53101"));

  NestedReconfigureData data;
  data.channel = channel;
  data.nested  = false;

  EXPECT_EQ(ARES_SUCCESS,
            ares_query_dnsrec(channel, "www.example.com", ARES_CLASS_IN,
                              ARES_REC_TYPE_A, NestedReplaceCallback, &data,
                              NULL));

  EXPECT_EQ(ARES_SUCCESS, ares_set_servers_csv(channel, "127.0.0.1:53301"));
  EXPECT_TRUE(data.nested);

  // The nested call reconciled the whole list; what it installed must survive.
  char *csv = ares_get_servers_csv(channel);
  ASSERT_NE(nullptr, csv);
  EXPECT_STREQ("127.0.0.1:53201", csv);
  ares_free_string(csv);

  ares_destroy(channel);
}

TEST_P(MockUDPChannelTest, CancelInCallbackNoDoubleFree) {
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &reply));

  CancelInCbData data;
  data.channel = channel_;
  data.done    = false;
  data.cbs     = 0;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A,
                    CancelChannelCallback, &data, NULL);
  Process();
  EXPECT_TRUE(data.done);
}

TEST_P(MockUDPChannelTest, GetSock) {
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &reply));

  ares_socket_t socks[3] = {ARES_SOCKET_BAD, ARES_SOCKET_BAD, ARES_SOCKET_BAD};
  int bitmask;

  bitmask = ares_getsock(channel_, socks, 3);
  EXPECT_EQ(0, bitmask);
  bitmask = ares_getsock(channel_, nullptr, 0);
  EXPECT_EQ(0, bitmask);

  // Ask again with a pending query.
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  bitmask = ares_getsock(channel_, socks, 3);
  EXPECT_NE(0, bitmask);

  size_t sock_cnt = 0;
  for (size_t i=0; i<3; i++) {
    if (ARES_GETSOCK_READABLE(bitmask, i) || ARES_GETSOCK_WRITABLE(bitmask, i)) {
      EXPECT_NE(ARES_SOCKET_BAD, socks[i]);
      if (socks[i] != ARES_SOCKET_BAD)
        sock_cnt++;
    }
  }
  EXPECT_NE((size_t)0, sock_cnt);

  Process();

  bitmask = ares_getsock(channel_, nullptr, 0);
  EXPECT_EQ(0, bitmask);
}

TEST_P(MockTCPChannelTest, GetSock) {
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &reply));

  ares_socket_t socks[3] = {ARES_SOCKET_BAD, ARES_SOCKET_BAD, ARES_SOCKET_BAD};
  int bitmask;

  bitmask = ares_getsock(channel_, socks, 3);
  EXPECT_EQ(0, bitmask);
  bitmask = ares_getsock(channel_, nullptr, 0);
  EXPECT_EQ(0, bitmask);

  // Ask again with a pending query.
  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  bitmask = ares_getsock(channel_, socks, 3);
  EXPECT_NE(0, bitmask);

  size_t sock_cnt = 0;
  for (size_t i=0; i<3; i++) {
    if (ARES_GETSOCK_READABLE(bitmask, i) || ARES_GETSOCK_WRITABLE(bitmask, i)) {
      EXPECT_NE(ARES_SOCKET_BAD, socks[i]);
      if (socks[i] != ARES_SOCKET_BAD)
        sock_cnt++;
    }
  }
  EXPECT_NE((size_t)0, sock_cnt);

  Process();

  bitmask = ares_getsock(channel_, nullptr, 0);
  EXPECT_EQ(0, bitmask);
}


TEST_P(MockChannelTest, VerifySocketFunctionCallback) {
  ares_socket_functions sock_funcs;
  memset(&sock_funcs, 0, sizeof(sock_funcs));

  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &reply));

  size_t count = 0;

  sock_funcs.asocket = [](int af, int type, int protocol, void * p) -> ares_socket_t {
    EXPECT_NE(nullptr, p);
    (*reinterpret_cast<size_t *>(p))++;
    return ::socket(af, type, protocol);
  };

  ares_set_socket_functions(channel_, &sock_funcs, &count);

  {
    count = 0;
    HostResult result;
    ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
    Process();

    EXPECT_TRUE(result.done_);
    EXPECT_EQ(ARES_SUCCESS, result.status_);
    EXPECT_EQ(0, result.timeouts_);
    EXPECT_NE((size_t)0, count);
  }

  {
    count = 0;
    ares_channel_t *copy;
    EXPECT_EQ(ARES_SUCCESS, ares_dup(&copy, channel_));

    HostResult result;
    ares_gethostbyname(copy, "www.google.com.", AF_INET, HostCallback, &result);

    ProcessAltChannel(copy);

    EXPECT_TRUE(result.done_);
    ares_destroy(copy);
    EXPECT_NE((size_t)0, count);
    EXPECT_EQ(ARES_SUCCESS, result.status_);
    EXPECT_EQ(0, result.timeouts_);
  }

}

TEST_P(MockChannelTest, DupSocketFunctionCallbackDestroySource) {
  ares_socket_functions sock_funcs;
  memset(&sock_funcs, 0, sizeof(sock_funcs));

  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &reply));

  size_t count = 0;

  sock_funcs.asocket = [](int af, int type, int protocol, void * p) -> ares_socket_t {
    EXPECT_NE(nullptr, p);
    (*reinterpret_cast<size_t *>(p))++;
    return ::socket(af, type, protocol);
  };

  ares_set_socket_functions(channel_, &sock_funcs, &count);

  /* The copy must keep using the socket functions once the channel it was
   * duplicated from is gone. */
  ares_channel_t *copy = nullptr;
  ASSERT_EQ(ARES_SUCCESS, ares_dup(&copy, channel_));
  ares_destroy(channel_);
  channel_ = copy;

  HostResult result;
  ares_gethostbyname(channel_, "www.google.com.", AF_INET, HostCallback, &result);
  Process();

  EXPECT_TRUE(result.done_);
  EXPECT_NE((size_t)0, count);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_EQ(0, result.timeouts_);
}

static const unsigned char *
  fetch_server_cookie(const ares_dns_record_t *dnsrec, size_t *len)
{
  const ares_dns_rr_t *rr  = fetch_rr_opt(dnsrec);
  const unsigned char *val = NULL;
  *len                     = 0;

  if (rr == NULL) {
    return NULL;
  }

  if (!ares_dns_rr_get_opt_byid(rr, ARES_RR_OPT_OPTIONS, ARES_OPT_PARAM_COOKIE,
                                &val, len)) {
    return NULL;
  }

  if (*len <= 8) {
    *len = 0;
    return NULL;
  }

  *len -= 8;
  val  += 8;
  return val;
}

static const unsigned char *
  fetch_client_cookie(const ares_dns_record_t *dnsrec, size_t *len)
{
  const ares_dns_rr_t *rr  = fetch_rr_opt(dnsrec);
  const unsigned char *val = NULL;
  *len                     = 0;

  if (rr == NULL) {
    return NULL;
  }

  if (!ares_dns_rr_get_opt_byid(rr, ARES_RR_OPT_OPTIONS, ARES_OPT_PARAM_COOKIE,
                                &val, len)) {
    return NULL;
  }

  if (*len < 8) {
    *len = 0;
    return NULL;
  }

  *len = 8;
  return val;
}

TEST_P(MockUDPChannelTest, DNSCookieSingle) {
  DNSPacket reply;
  std::vector<byte> server_cookie = { 1, 2, 3, 4, 5, 6, 7, 8 };
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, { }, server_cookie, false));
  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &reply));

  QueryResult result;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A, QueryCallback, &result, NULL);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(0, result.timeouts_);

  size_t len;
  const unsigned char *returned_cookie = fetch_server_cookie(result.dnsrec_.dnsrec_, &len);
  EXPECT_EQ(len, server_cookie.size());
  EXPECT_TRUE(memcmp(server_cookie.data(), returned_cookie, len) == 0);
}

TEST_P(MockUDPChannelTest, DNSCookieMissingAfterGood) {
  std::vector<byte> server_cookie = { 1, 2, 3, 4, 5, 6, 7, 8 };
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, { }, server_cookie, false));
  DNSPacket reply_nocookie;
  reply_nocookie.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, { }, { }, false));
  DNSPacket reply_ensurecookie;
  reply_ensurecookie.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, { }, server_cookie, true));

  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &reply))
    .WillOnce(SetReply(&server_, &reply_nocookie))
    .WillOnce(SetReply(&server_, &reply_ensurecookie));

  /* This test will establish the server supports cookies, then the next reply
   * will be missing the server cookie and therefore be rejected and timeout, then
   * an internal retry will occur and the cookie will be present again and it
   * will be verified a server cookie was actually present that matches the
   * server cookie. */
  QueryResult result1;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A, QueryCallback, &result1, NULL);
  Process();
  EXPECT_TRUE(result1.done_);
  EXPECT_EQ(0, result1.timeouts_);

  QueryResult result2;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A, QueryCallback, &result2, NULL);
  Process();
  EXPECT_TRUE(result2.done_);
  EXPECT_EQ(1, result2.timeouts_);

  /* Client cookie should NOT have rotated */
  size_t len1;
  const unsigned char *client_cookie_1 = fetch_client_cookie(result1.dnsrec_.dnsrec_, &len1);
  size_t len2;
  const unsigned char *client_cookie_2 = fetch_client_cookie(result2.dnsrec_.dnsrec_, &len2);
  EXPECT_EQ(len1, 8);
  EXPECT_EQ(len1, len2);
  EXPECT_TRUE(memcmp(client_cookie_1, client_cookie_2, len1) == 0);
}


TEST_P(MockUDPChannelTest, DNSCookieBadLen) {
  std::vector<byte> server_cookie = { 1, 2, 3, 4, 5, 6, 7, 8 };
  std::vector<byte> server_cookie_bad = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF, 0 };
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, { }, server_cookie, false));
  DNSPacket reply_badcookielen;
  reply_badcookielen.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, { }, server_cookie_bad, false));

  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &reply_badcookielen))
    .WillOnce(SetReply(&server_, &reply));

  /* This test will send back a malformed cookie len, then when it times out and retry occurs will send back a valid cookie. */
  QueryResult result1;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A, QueryCallback, &result1, NULL);
  Process();
  EXPECT_TRUE(result1.done_);
  EXPECT_EQ(1, result1.timeouts_);
}


TEST_P(MockUDPChannelTest, DNSCookieServerRotate) {
  std::vector<byte> server_cookie = { 1, 2, 3, 4, 5, 6, 7, 8 };
  std::vector<byte> server_cookie_rotate = { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xA, 0xB, 0xC, 0xD, 0xE, 0xF };

  DNSPacket reply_cookie1;
  reply_cookie1.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, {}, server_cookie, false));
  DNSPacket reply_cookie2_badcookie;
  reply_cookie2_badcookie.set_response().set_aa().set_rcode(ARES_RCODE_BADCOOKIE & 0xF)
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR((ARES_RCODE_BADCOOKIE >> 4) & 0xFF, 0, 0, 1280, { }, server_cookie_rotate, false));
  DNSPacket reply_cookie2;
  reply_cookie2.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, { }, server_cookie_rotate, true));

  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &reply_cookie1))
    .WillOnce(SetReply(&server_, &reply_cookie2_badcookie))
    .WillOnce(SetReply(&server_, &reply_cookie2));

  /* This test will establish the server supports cookies, then the next reply
   * the server returns BADCOOKIE indicating the cookie has rotated and
   * returns a new cookie. Then the query will be automatically retried with
   * the newly returned cookie. No timeouts should be indicated, and the
   * client cookie should not rotate. */
  QueryResult result1;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A, QueryCallback, &result1, NULL);
  Process();
  EXPECT_TRUE(result1.done_);
  EXPECT_EQ(0, result1.timeouts_);

  QueryResult result2;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A, QueryCallback, &result2, NULL);
  Process();
  EXPECT_TRUE(result2.done_);
  EXPECT_EQ(0, result2.timeouts_);

  /* Client cookie should NOT have rotated */
  size_t len1;
  const unsigned char *client_cookie_1 = fetch_client_cookie(result1.dnsrec_.dnsrec_, &len1);
  size_t len2;
  const unsigned char *client_cookie_2 = fetch_client_cookie(result2.dnsrec_.dnsrec_, &len2);
  EXPECT_EQ(len1, 8);
  EXPECT_EQ(len1, len2);
  EXPECT_TRUE(memcmp(client_cookie_1, client_cookie_2, len1) == 0);
}

TEST_P(MockUDPChannelTest, DNSCookieSpoof) {
  std::vector<byte> client_cookie = { 1, 2, 3, 4, 5, 6, 7, 8 };
  std::vector<byte> server_cookie = { 1, 2, 3, 4, 5, 6, 7, 8 };

  DNSPacket reply_spoof;
  reply_spoof.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, client_cookie, server_cookie, false));
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, { }, server_cookie, false));

  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &reply_spoof))
    .WillOnce(SetReply(&server_, &reply));

  /* This test will return a reply that doesn't have the same client cookie as
   * was sent, this should result in a drop of the packet alltogether, then
   * the library will retry and a proper result will be sent. */
  QueryResult result;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A, QueryCallback, &result, NULL);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(1, result.timeouts_);
}

TEST_P(MockUDPChannelTest, DNSCookieTCPUpgrade) {
  std::vector<byte> server_cookie = { 1, 2, 3, 4, 5, 6, 7, 8 };

  DNSPacket reply_badcookie;
  reply_badcookie.set_response().set_aa().set_rcode(ARES_RCODE_BADCOOKIE & 0xF)
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR((ARES_RCODE_BADCOOKIE >> 4) & 0xFF, 0, 0, 1280, { }, server_cookie, false));
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, { }, { }, false));

  EXPECT_CALL(server_, OnRequest("www.google.com", T_A))
    .WillOnce(SetReply(&server_, &reply_badcookie))
    .WillOnce(SetReply(&server_, &reply_badcookie))
    .WillOnce(SetReply(&server_, &reply_badcookie))
    .WillOnce(SetReply(&server_, &reply));

  /* This test will establish the server supports cookies, but continuously
   * returns BADCOOKIE which is an indicator that there is some form of
   * AnyCast issue across servers, so it upgrades to TCP afterwards.  No
   * timeouts are recorded as the queries are sent back-to-back as immediate
   * reattempts after the response. */
  QueryResult result;
  ares_query_dnsrec(channel_, "www.google.com", ARES_CLASS_IN, ARES_REC_TYPE_A, QueryCallback, &result, NULL);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(0, result.timeouts_);
}


#ifndef WIN32
TEST_P(MockChannelTest, HostAlias) {
  DNSPacket reply;
  reply.set_response().set_aa()
    .add_question(new DNSQuestion("www.google.com", T_A))
    .add_answer(new DNSARR("www.google.com", 0x0100, {0x01, 0x02, 0x03, 0x04}));
  ON_CALL(server_, OnRequest("www.google.com", T_A))
    .WillByDefault(SetReply(&server_, &reply));

  TempFile aliases("\n\n# www commentedout\nwww www.google.com\n");
  EnvValue with_env("HOSTALIASES", aliases.filename());

  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.google.com' aliases=[] addrs=[1.2.3.4]}", ss.str());
}

TEST_P(MockChannelTest, HostAliasMissing) {
  DNSPacket yesfirst;
  yesfirst.set_response().set_aa()
    .add_question(new DNSQuestion("www.first.com", T_A))
    .add_answer(new DNSARR("www.first.com", 0x0200, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www.first.com", T_A))
    .WillByDefault(SetReply(&server_, &yesfirst));

  TempFile aliases("\n\n# www commentedout\nww www.google.com\n");
  EnvValue with_env("HOSTALIASES", aliases.filename());
  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.first.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
}

TEST_P(MockChannelTest, HostAliasMissingFile) {
  DNSPacket yesfirst;
  yesfirst.set_response().set_aa()
    .add_question(new DNSQuestion("www.first.com", T_A))
    .add_answer(new DNSARR("www.first.com", 0x0200, {2, 3, 4, 5}));
  ON_CALL(server_, OnRequest("www.first.com", T_A))
    .WillByDefault(SetReply(&server_, &yesfirst));

  EnvValue with_env("HOSTALIASES", "bogus.mcfile");
  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  std::stringstream ss;
  ss << result.host_;
  EXPECT_EQ("{'www.first.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
}

TEST_P(MockChannelTest, HostAliasUnreadable) {
  TempFile aliases("www www.google.com\n");
  EXPECT_EQ(chmod(aliases.filename(), 0), 0);

  /* Perform OS sanity checks.  We are observing on Debian after the chmod(fn, 0)
   * that we are still able to fopen() the file which is unexpected.  Skip the
   * test if we observe this behavior */
  struct stat st;
  EXPECT_EQ(stat(aliases.filename(), &st), 0);
  EXPECT_EQ(st.st_mode & (S_IRWXU|S_IRWXG|S_IRWXO), 0);
  FILE *fp = fopen(aliases.filename(), "r");
  if (fp != NULL) {
    if (verbose) std::cerr << "Skipping Test due to OS incompatibility (open file caching)" << std::endl;
    fclose(fp);
    return;
  }

  EnvValue with_env("HOSTALIASES", aliases.filename());

  HostResult result;
  ares_gethostbyname(channel_, "www", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(ARES_EFILE, result.status_);
  chmod(aliases.filename(), 0777);
}
#endif

class MockMultiServerChannelTest
  : public MockChannelOptsTest,
    public ::testing::WithParamInterface< std::pair<int, bool> > {
 public:
  MockMultiServerChannelTest(ares_options *opts, int optmask)
    : MockChannelOptsTest(3, GetParam().first, GetParam().second, false, opts, optmask) {}
  void CheckExample() {
    HostResult result;
    ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback, &result);
    Process();
    EXPECT_TRUE(result.done_);
    std::stringstream ss;
    ss << result.host_;
    EXPECT_EQ("{'www.example.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
  }
};

class NoRotateMultiMockTest : public MockMultiServerChannelTest {
 public:
  NoRotateMultiMockTest() : MockMultiServerChannelTest(nullptr, ARES_OPT_NOROTATE) {}
};


TEST_P(NoRotateMultiMockTest, ThirdServer) {
  struct ares_options opts;
  int optmask = 0;
  memset(&opts, 0, sizeof(opts));
  EXPECT_EQ(ARES_SUCCESS, ares_save_options(channel_, &opts, &optmask));
  EXPECT_EQ(ARES_OPT_NOROTATE, (optmask & ARES_OPT_NOROTATE));
  ares_destroy_options(&opts);

  DNSPacket servfailrsp;
  servfailrsp.set_response().set_aa().set_rcode(SERVFAIL)
    .add_question(new DNSQuestion("www.example.com", T_A));
  DNSPacket notimplrsp;
  notimplrsp.set_response().set_aa().set_rcode(NOTIMP)
    .add_question(new DNSQuestion("www.example.com", T_A));
  DNSPacket okrsp;
  okrsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.example.com", T_A))
    .add_answer(new DNSARR("www.example.com", 100, {2,3,4,5}));

  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp));
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &notimplrsp));
  EXPECT_CALL(*servers_[2], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[2].get(), &okrsp));
  CheckExample();

  // Second time around, still starts from server [2], as [0] and [1] both
  // recorded failures
  EXPECT_CALL(*servers_[2], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[2].get(), &servfailrsp));
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &notimplrsp));
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &okrsp));
  CheckExample();

  // Third time around, server order is [1] (f0), [2] (f1), [0] (f2), which
  // means [1] will get called twice in a row as after the first call
  // order will be  [1] (f1), [2] (f1), [0] (f2) since sort order is
  // (failure count, index)
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[1].get(), &notimplrsp));
  EXPECT_CALL(*servers_[2], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[2].get(), &notimplrsp));
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &okrsp));
  CheckExample();
}

TEST_P(NoRotateMultiMockTest, ServerNoResponseFailover) {
  std::vector<byte> nothing;
  DNSPacket okrsp;
  okrsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.example.com", T_A))
    .add_answer(new DNSARR("www.example.com", 100, {2,3,4,5}));

  /* Server #1 works fine on first attempt, then acts like its offline on
   * second, then backonline on the third. */
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &okrsp))
    .WillOnce(SetReplyData(servers_[0].get(), nothing))
    .WillOnce(SetReply(servers_[0].get(), &okrsp));

  /* Server #2 always acts like its offline */
  ON_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillByDefault(SetReplyData(servers_[1].get(), nothing));

  /* Server #3 works fine on first and second request, then no reply on 3rd */
  EXPECT_CALL(*servers_[2], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[2].get(), &okrsp))
    .WillOnce(SetReply(servers_[2].get(), &okrsp))
    .WillOnce(SetReplyData(servers_[2].get(), nothing));

  HostResult result;

  /* 1. First server returns a response on the first request immediately, normal
   *    operation on channel. */
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(0, result.timeouts_);
  std::stringstream ss1;
  ss1 << result.host_;
  EXPECT_EQ("{'www.example.com' aliases=[] addrs=[2.3.4.5]}", ss1.str());

  /* 2. On the second request, simulate the first and second servers not
   *    returning a response at all, but the 3rd server works, so should have
   *    2 timeouts. */
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(2, result.timeouts_);
  std::stringstream ss2;
  ss2 << result.host_;
  EXPECT_EQ("{'www.example.com' aliases=[] addrs=[2.3.4.5]}", ss2.str());

  /* 3. On the third request, the active server should be #3, so should respond
   *    immediately with no timeouts */
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(0, result.timeouts_);
  std::stringstream ss3;
  ss3 << result.host_;
  EXPECT_EQ("{'www.example.com' aliases=[] addrs=[2.3.4.5]}", ss3.str());

  /* 4. On the fourth request, the active server should be #3, but will timeout,
   *    and the first server should then respond */
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback, &result);
  Process();
  EXPECT_TRUE(result.done_);
  EXPECT_EQ(1, result.timeouts_);
  std::stringstream ss4;
  ss4 << result.host_;
  EXPECT_EQ("{'www.example.com' aliases=[] addrs=[2.3.4.5]}", ss4.str());
}

#if defined(_WIN32)
#  define SERVER_FAILOVER_RETRY_DELAY 500
#else
#  define SERVER_FAILOVER_RETRY_DELAY 330
#endif

class ServerFailoverOptsMultiMockTest
  : public MockChannelOptsTest,
    public ::testing::WithParamInterface< std::pair<int, bool> > {
 public:
  ServerFailoverOptsMultiMockTest()
    : MockChannelOptsTest(4, GetParam().first, GetParam().second, false,
                          FillOptions(&opts_),
                          ARES_OPT_SERVER_FAILOVER | ARES_OPT_NOROTATE) {}
  void CheckExample() {
    HostResult result;
    ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback, &result);
    Process();
    EXPECT_TRUE(result.done_);
    std::stringstream ss;
    ss << result.host_;
    EXPECT_EQ("{'www.example.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
  }

  static struct ares_options* FillOptions(struct ares_options *opts) {
    memset(opts, 0, sizeof(struct ares_options));
    opts->server_failover_opts.retry_chance = 1;
    opts->server_failover_opts.retry_delay = SERVER_FAILOVER_RETRY_DELAY;
    return opts;
  }
 private:
  struct ares_options opts_;
};


// Test case to trigger server failover behavior. We use a retry chance of
// 100% and a retry delay so that we can test behavior reliably.
TEST_P(ServerFailoverOptsMultiMockTest, ServerFailoverOpts) {
 DNSPacket servfailrsp;
  servfailrsp.set_response().set_aa().set_rcode(SERVFAIL)
    .add_question(new DNSQuestion("www.example.com", T_A));
  DNSPacket okrsp;
  okrsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.example.com", T_A))
    .add_answer(new DNSARR("www.example.com", 100, {2,3,4,5}));

  auto tv_begin = std::chrono::high_resolution_clock::now();
  auto tv_now   = std::chrono::high_resolution_clock::now();
  unsigned int delay_ms;

  // At start all servers are healthy, first server should be selected
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: First server should be selected" << std::endl;
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &okrsp));
  CheckExample();

  // Fail server #0 but leave server #1 as healthy.  This results in server
  // order:
  //  #1 (failures: 0), #2 (failures: 0), #3 (failures: 0), #0 (failures: 1)
  tv_now = std::chrono::high_resolution_clock::now();
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: Server0 will fail but leave Server1 as healthy" << std::endl;
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp));
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &okrsp));
  CheckExample();

  // Sleep for the retry delay (actually a little more than the retry delay to account
  // for unreliable timing, e.g. NTP slew) and send in another query. The real
  // query will be sent to Server #1 (which will succeed) and Server #0 will
  // be probed and return a successful result.  This leaves the server order
  // of:
  //   #0 (failures: 0), #1 (failures: 0), #2 (failures: 0), #3 (failures: 0)
  tv_now = std::chrono::high_resolution_clock::now();
  delay_ms = SERVER_FAILOVER_RETRY_DELAY + (SERVER_FAILOVER_RETRY_DELAY / 10);
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: sleep " << delay_ms << "ms" << std::endl;
  ares_sleep_time(delay_ms);
  tv_now = std::chrono::high_resolution_clock::now();
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: Server0 should be past retry delay and should be probed (successful), server 1 will respond successful for real query" << std::endl;
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &okrsp));
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &okrsp));
  CheckExample();


  // Fail all servers for the first round of tries. On the second round, #0
  // responds successfully. This should leave server order of:
  //   #1 (failures: 0), #2 (failures: 1), #3 (failures: 1), #0 (failures: 2)
  // NOTE: A single query being retried won't spawn probes to downed servers,
  //       only an initial query attempt is eligible to spawn probes.  So
  //       no probes are sent for this test.
  tv_now = std::chrono::high_resolution_clock::now();
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: All 4 servers will fail on the first attempt, server 0 will fail on second. Server 1 will succeed on second." << std::endl;
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp));
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[1].get(), &okrsp));
  EXPECT_CALL(*servers_[2], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[2].get(), &servfailrsp));
  EXPECT_CALL(*servers_[3], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[3].get(), &servfailrsp));
  CheckExample();


  // Sleep for the retry delay and send in another query. Server #1 is the
  // highest priority server and will respond with success, however a probe
  // will be sent for Server #2 which will succeed:
  //  #1 (failures: 0), #2 (failures: 0), #3 (failures: 1 - expired), #0 (failures: 2 - expired)
  tv_now = std::chrono::high_resolution_clock::now();
  delay_ms = SERVER_FAILOVER_RETRY_DELAY + (SERVER_FAILOVER_RETRY_DELAY / 10);
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: sleep " << delay_ms << "ms" << std::endl;
  ares_sleep_time(delay_ms);
  tv_now = std::chrono::high_resolution_clock::now();
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: Past retry delay, will query Server 1 and probe Server 2, both will succeed." << std::endl;
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &okrsp));
  EXPECT_CALL(*servers_[2], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[2].get(), &okrsp));
  CheckExample();

  // Cause another server to fail so we have at least one non-expired failed
  // server and one expired failed server.  #1 is highest priority, which we
  // will fail, #2 will succeed, and #3 will be probed and succeed:
  //  #2 (failures: 0), #3 (failures: 0), #1 (failures: 1 not expired), #0 (failures: 2 expired)
  tv_now = std::chrono::high_resolution_clock::now();
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: Will query Server 1 and fail, Server 2 will answer successfully. Server 3 will be probed and succeed." << std::endl;
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &servfailrsp));
  EXPECT_CALL(*servers_[2], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[2].get(), &okrsp));
  EXPECT_CALL(*servers_[3], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[3].get(), &okrsp));
  CheckExample();

  // We need to make sure that if there is a failed server that is higher priority
  // but not yet expired that it will probe the next failed server instead.
  // In this case #2 is the server that the query will go to and succeed, and
  // then a probe will be sent for #0 (since #1 is not expired) and succeed.  We
  // will sleep for 1/4 the retry duration before spawning the queries so we can
  // then sleep for the rest for the follow-up test.  This will leave the servers
  // in this state:
  //   #0 (failures: 0), #2 (failures: 0), #3 (failures: 0), #1 (failures: 1 not expired)
  tv_now = std::chrono::high_resolution_clock::now();

  // We need to track retry delay time to know what is expired when.
  auto elapse_start = tv_now;

  delay_ms = (SERVER_FAILOVER_RETRY_DELAY/4);
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: sleep " << delay_ms << "ms" << std::endl;
  ares_sleep_time(delay_ms);
  tv_now = std::chrono::high_resolution_clock::now();

  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: Retry delay has not been hit yet. Server2 will be queried and succeed. Server 0 (not server 1 due to non-expired retry delay) will be probed and succeed." << std::endl;
  EXPECT_CALL(*servers_[2], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[2].get(), &okrsp));
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &okrsp));
  CheckExample();

  // Finally we sleep for the remainder of the retry delay, send another
  // query, which should succeed on Server #0, and also probe Server #1 which
  // will also succeed.
  tv_now = std::chrono::high_resolution_clock::now();

  unsigned int elapsed_time = (unsigned int)std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - elapse_start).count();
  delay_ms = (SERVER_FAILOVER_RETRY_DELAY) + (SERVER_FAILOVER_RETRY_DELAY / 10);
  if (elapsed_time > delay_ms) {
    if (verbose) std::cerr << "elapsed duration " << elapsed_time << "ms greater than desired delay of " << delay_ms << "ms, not sleeping" << std::endl;
  } else {
    delay_ms -= elapsed_time; // subtract already elapsed time
    if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: sleep " << delay_ms << "ms" << std::endl;
    ares_sleep_time(delay_ms);
  }
  tv_now = std::chrono::high_resolution_clock::now();
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: Retry delay has expired on Server1, Server 0 will be queried and succeed, Server 1 will be probed and succeed." << std::endl;
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &okrsp));
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &okrsp));
  CheckExample();
};


// Test that probe_pending flag is properly cleared when a probe times out,
// by testing a scenario where a server is probed, the probe times out, and
// then the server is probed again.
TEST_P(ServerFailoverOptsMultiMockTest, ProbeAfterProbeTimeout) {
  std::vector<byte> nothing;
  DNSPacket okrsp;
  okrsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.example.com", T_A))
    .add_answer(new DNSARR("www.example.com", 100, {2,3,4,5}));

  auto tv_begin = std::chrono::high_resolution_clock::now();
  auto tv_now   = std::chrono::high_resolution_clock::now();
  unsigned int delay_ms;

  // First, fail server #0 so it becomes marked as failed
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: Failing Server0 with timeout" << std::endl;
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReplyData(servers_[0].get(), nothing));
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &okrsp));
  HostResult result1;
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback, &result1);
  Process();
  EXPECT_TRUE(result1.done_);

  // Sleep past the retry delay and send another query
  // This should trigger a probe to server #0, which will timeout
  tv_now = std::chrono::high_resolution_clock::now();
  delay_ms = SERVER_FAILOVER_RETRY_DELAY + (SERVER_FAILOVER_RETRY_DELAY / 10);
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: sleep " << delay_ms << "ms" << std::endl;
  ares_sleep_time(delay_ms);
  tv_now = std::chrono::high_resolution_clock::now();

  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: Sending query that triggers probe to Server0, probe will timeout" << std::endl;
  // Server #1 will handle the real query, Server #0 will be probed and timeout
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &okrsp));
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReplyData(servers_[0].get(), nothing));  // Probe times out
  HostResult result2;
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback, &result2);
  Process();
  EXPECT_TRUE(result2.done_);

  // Sleep past the retry delay again
  // The next query will trigger another probe to server #0
  tv_now = std::chrono::high_resolution_clock::now();
  delay_ms = SERVER_FAILOVER_RETRY_DELAY + (SERVER_FAILOVER_RETRY_DELAY / 10);
  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: sleep " << delay_ms << "ms again" << std::endl;
  ares_sleep_time(delay_ms);
  tv_now = std::chrono::high_resolution_clock::now();

  if (verbose) std::cerr << std::chrono::duration_cast<std::chrono::milliseconds>(tv_now - tv_begin).count() << "ms: Sending another query - should trigger another probe to Server0" << std::endl;
  // Server #1 will handle the real query
  // Server #0 should be probed again
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &okrsp));
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &okrsp));
  HostResult result3;
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback, &result3);
  Process();
  EXPECT_TRUE(result3.done_);
}

class ServerRecoveryMultiMockTest
  : public MockChannelOptsTest,
    public ::testing::WithParamInterface< std::pair<int, bool> > {
 public:
  ServerRecoveryMultiMockTest()
    : MockChannelOptsTest(2, GetParam().first, GetParam().second, false,
                          FillOptions(&opts_),
                          ARES_OPT_SERVER_FAILOVER | ARES_OPT_NOROTATE) {}

  static struct ares_options* FillOptions(struct ares_options *opts) {
    memset(opts, 0, sizeof(struct ares_options));
    opts->server_failover_opts.retry_chance = 1;
    opts->server_failover_opts.retry_delay = SERVER_FAILOVER_RETRY_DELAY;
    return opts;
  }
 private:
  struct ares_options opts_;
};

// Test that when every server has failures, a query sent to a failed server
// still spawns a probe of another failed server.  Without this, a server
// that recovers from an extended outage while another (better-sorted) server
// is still failing is never noticed until the failure counts catch up
// ("sticky recovery").
TEST_P(ServerRecoveryMultiMockTest, StickyRecoveryProbe) {
  DNSPacket servfailrsp;
  servfailrsp.set_response().set_aa().set_rcode(SERVFAIL)
    .add_question(new DNSQuestion("www.example.com", T_A));
  DNSPacket okrsp;
  okrsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.example.com", T_A))
    .add_answer(new DNSARR("www.example.com", 100, {2,3,4,5}));
  unsigned int delay_ms = SERVER_FAILOVER_RETRY_DELAY +
                          (SERVER_FAILOVER_RETRY_DELAY / 10);

  // Fail all attempts of the first query so both servers end up failed.
  // Attempts alternate between the servers as each failure re-sorts the
  // list, so each server sees 3 requests (2 servers x 3 tries).  No probes
  // are spawned: retries never probe, and the initial attempt is sent while
  // all servers are still healthy.
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp));
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[1].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[1].get(), &servfailrsp));
  HostResult result1;
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback,
                     &result1);
  Process();
  EXPECT_TRUE(result1.done_);
  EXPECT_EQ(ARES_ESERVFAIL, result1.status_);

  // Sleep past the retry delay so both servers are eligible for probing.
  ares_sleep_time(delay_ms);

  // Server #0 recovers.  The query is sent to server #0 (both servers have
  // equal failure counts and #0 failed least recently).  Even though the
  // chosen server has failures, a probe must be spawned to the other failed
  // server (#1), which is still down.
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &okrsp));
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &servfailrsp));
  HostResult result2;
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback,
                     &result2);
  Process();
  EXPECT_TRUE(result2.done_);
  EXPECT_EQ(ARES_SUCCESS, result2.status_);

  // Sleep past the retry delay again so server #1 is once more eligible for
  // probing.
  ares_sleep_time(delay_ms);

  // Server #1 has now recovered.  The query goes to healthy server #0 and a
  // probe is spawned to server #1 which succeeds, marking it healthy again.
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &okrsp));
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &okrsp));
  HostResult result3;
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback,
                     &result3);
  Process();
  EXPECT_TRUE(result3.done_);
  EXPECT_EQ(ARES_SUCCESS, result3.status_);
}

class ServerRecoveryNoProbeMultiMockTest
  : public MockChannelOptsTest,
    public ::testing::WithParamInterface< std::pair<int, bool> > {
 public:
  ServerRecoveryNoProbeMultiMockTest()
    : MockChannelOptsTest(2, GetParam().first, GetParam().second, false,
                          FillOptions(&opts_),
                          ARES_OPT_SERVER_FAILOVER | ARES_OPT_NOROTATE) {}

  static struct ares_options* FillOptions(struct ares_options *opts) {
    memset(opts, 0, sizeof(struct ares_options));
    // Probing disabled: recovery must come from server selection alone
    opts->server_failover_opts.retry_chance = 0;
    opts->server_failover_opts.retry_delay = SERVER_FAILOVER_RETRY_DELAY;
    return opts;
  }
 private:
  struct ares_options opts_;
};

// Test that with probing disabled, a server whose failure count went stale
// during an outage is still retried once an actively-failing server reaches
// the same count: servers with equal failure counts are tried
// least-recently-failed first rather than in configuration order.
TEST_P(ServerRecoveryNoProbeMultiMockTest, StickyRecoveryTieBreak) {
  DNSPacket servfailrsp;
  servfailrsp.set_response().set_aa().set_rcode(SERVFAIL)
    .add_question(new DNSQuestion("www.example.com", T_A));
  DNSPacket okrsp;
  okrsp.set_response().set_aa()
    .add_question(new DNSQuestion("www.example.com", T_A))
    .add_answer(new DNSARR("www.example.com", 100, {2,3,4,5}));

  // Fail all attempts of the first query so both servers end up failed with
  // a failure count of 3 (2 servers x 3 tries, alternating).
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp));
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[1].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[1].get(), &servfailrsp));
  HostResult result1;
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback,
                     &result1);
  Process();
  EXPECT_TRUE(result1.done_);
  EXPECT_EQ(ARES_ESERVFAIL, result1.status_);

  // Server #0 recovers briefly: the next query succeeds and resets its
  // failure count, while server #1 stays failed with its count frozen at 3.
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &okrsp));
  HostResult result2;
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback,
                     &result2);
  Process();
  EXPECT_TRUE(result2.done_);
  EXPECT_EQ(ARES_SUCCESS, result2.status_);

  // Server #0 starts failing again while server #1 has (unnoticed)
  // recovered.  The query retries server #0 until its failure count catches
  // up to server #1's stale count of 3.  At that point server #1 must sort
  // first (it failed least recently) and answer successfully.  Without the
  // tie-break the remaining attempts would all go to server #0 and the query
  // would fail without server #1 ever being tried.
  EXPECT_CALL(*servers_[0], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp))
    .WillOnce(SetReply(servers_[0].get(), &servfailrsp));
  EXPECT_CALL(*servers_[1], OnRequest("www.example.com", T_A))
    .WillOnce(SetReply(servers_[1].get(), &okrsp));
  HostResult result3;
  ares_gethostbyname(channel_, "www.example.com.", AF_INET, HostCallback,
                     &result3);
  Process();
  EXPECT_TRUE(result3.done_);
  EXPECT_EQ(ARES_SUCCESS, result3.status_);
}

const char *af_tostr(int af)
{
  switch (af) {
    case AF_INET:
      return "ipv4";
    case AF_INET6:
      return "ipv6";
  }
  return "ipunknown";
}

const char *mode_tostr(bool mode)
{
  return mode?"ForceTCP":"DefaultUDP";
}

std::string PrintFamilyMode(const testing::TestParamInfo<std::pair<int, bool>> &info)
{
  std::string name;

  name += af_tostr(std::get<0>(info.param));
  name += "_";
  name += mode_tostr(std::get<1>(info.param));
  return name;
}

std::string PrintFamily(const testing::TestParamInfo<int> &info)
{
  std::string name;

  name += af_tostr(info.param);
  return name;
}

INSTANTIATE_TEST_SUITE_P(AddressFamilies, NoDNS0x20MockTest, ::testing::ValuesIn(ares::test::families), PrintFamily);

INSTANTIATE_TEST_SUITE_P(AddressFamilies, MockChannelTest, ::testing::ValuesIn(ares::test::families_modes), PrintFamilyMode);

#ifdef HAVE_CONTAINER
INSTANTIATE_TEST_SUITE_P(AddressFamilies, ContainedMockChannelSysConfig, ::testing::ValuesIn(ares::test::families_modes), PrintFamilyMode);
#endif

INSTANTIATE_TEST_SUITE_P(AddressFamilies, MockUDPChannelTest, ::testing::ValuesIn(ares::test::families), PrintFamily);

INSTANTIATE_TEST_SUITE_P(AddressFamilies, MockRetryDepthChannelTest, ::testing::ValuesIn(ares::test::families), PrintFamily);

INSTANTIATE_TEST_SUITE_P(AddressFamilies, MockUDPMaxQueriesTest, ::testing::ValuesIn(ares::test::families), PrintFamily);

INSTANTIATE_TEST_SUITE_P(AddressFamilies, CacheQueriesTest, ::testing::ValuesIn(ares::test::families), PrintFamily);

INSTANTIATE_TEST_SUITE_P(AddressFamilies, MockTCPChannelTest, ::testing::ValuesIn(ares::test::families), PrintFamily);

INSTANTIATE_TEST_SUITE_P(AddressFamilies, MockExtraOptsTest, ::testing::ValuesIn(ares::test::families_modes), PrintFamilyMode);

INSTANTIATE_TEST_SUITE_P(AddressFamilies, MockNoCheckRespChannelTest, ::testing::ValuesIn(ares::test::families_modes), PrintFamilyMode);

INSTANTIATE_TEST_SUITE_P(AddressFamilies, MockEDNSChannelTest, ::testing::ValuesIn(ares::test::families_modes), PrintFamilyMode);

INSTANTIATE_TEST_SUITE_P(TransportModes, NoRotateMultiMockTest, ::testing::ValuesIn(ares::test::families_modes), PrintFamilyMode);

INSTANTIATE_TEST_SUITE_P(TransportModes, ServerFailoverOptsMultiMockTest, ::testing::ValuesIn(ares::test::families_modes), PrintFamilyMode);

INSTANTIATE_TEST_SUITE_P(TransportModes, ServerRecoveryMultiMockTest, ::testing::ValuesIn(ares::test::families_modes), PrintFamilyMode);

INSTANTIATE_TEST_SUITE_P(TransportModes, ServerRecoveryNoProbeMultiMockTest, ::testing::ValuesIn(ares::test::families_modes), PrintFamilyMode);

// An answer is only accepted on the connection its query was sent on
// (RFC 5452 section 9.1).  Two servers: #0 is demoted by a SERVFAIL so a later
// query goes only to #1, while #0's connection stays open (STAYOPEN).  Server
// #0 then answers that query on its own connection.  The answer must be
// dropped without side effects, and the query must then complete normally.
class ReplyConnMultiMockTest
  : public MockChannelOptsTest,
    public ::testing::WithParamInterface< std::pair<int, bool> > {
 public:
  ReplyConnMultiMockTest()
    : MockChannelOptsTest(2, GetParam().first, GetParam().second, false,
                          FillOptions(&opts_),
                          ARES_OPT_FLAGS | ARES_OPT_TRIES |
                          ARES_OPT_SERVER_FAILOVER | ARES_OPT_NOROTATE) {}

  static struct ares_options* FillOptions(struct ares_options *opts) {
    memset(opts, 0, sizeof(struct ares_options));
    /* Keep the demoted server's connection open; no DNS 0x20 so the replies
     * built here match the question exactly */
    opts->flags = ARES_FLAG_STAYOPEN | ARES_FLAG_EDNS;
    /* One try per server: the query goes to #1, then on timeout to #0 */
    opts->tries = 1;
    /* Never probe the demoted server, so the query is sent only to #1 */
    opts->server_failover_opts.retry_chance = 0;
    opts->server_failover_opts.retry_delay  = 10000;
    return opts;
  }
 private:
  struct ares_options opts_;
};

static std::string ReplyConnFirstA(const QueryResult &result)
{
  const ares_dns_rr_t *rr;
  char                 addr[64];

  if (result.dnsrec_.dnsrec_ == nullptr ||
      ares_dns_record_rr_cnt(result.dnsrec_.dnsrec_, ARES_SECTION_ANSWER) == 0) {
    return "";
  }
  rr = ares_dns_record_rr_get_const(result.dnsrec_.dnsrec_,
                                    ARES_SECTION_ANSWER, 0);
  if (ares_dns_rr_get_type(rr) != ARES_REC_TYPE_A ||
      ares_inet_ntop(AF_INET, ares_dns_rr_get_addr(rr, ARES_RR_A_ADDR), addr,
                     sizeof(addr)) == nullptr) {
    return "";
  }
  return addr;
}

TEST_P(ReplyConnMultiMockTest, AnswerOnOtherServerConnectionDropped) {
  DNSPacket warm_servfail;
  warm_servfail.set_response().set_aa().set_rcode(SERVFAIL)
    .add_question(new DNSQuestion("warmup.example", T_A));
  DNSPacket warm_ok;
  warm_ok.set_response().set_aa()
    .add_question(new DNSQuestion("warmup.example", T_A))
    .add_answer(new DNSARR("warmup.example", 100, {1, 1, 1, 1}));
  DNSPacket forged;
  forged.set_response().set_aa()
    .add_question(new DNSQuestion("victim.example", T_A))
    .add_answer(new DNSARR("victim.example", 100, {6, 6, 6, 6}));
  DNSPacket genuine;
  genuine.set_response().set_aa()
    .add_question(new DNSQuestion("victim.example", T_A))
    .add_answer(new DNSARR("victim.example", 100, {1, 2, 3, 4}));
  unsigned short qid = 0;

  /* Demote #0, leaving a connection to it open */
  EXPECT_CALL(*servers_[0], OnRequest("warmup.example", T_A))
    .WillOnce(SetReply(servers_[0].get(), &warm_servfail));
  EXPECT_CALL(*servers_[1], OnRequest("warmup.example", T_A))
    .WillOnce(SetReply(servers_[1].get(), &warm_ok));
  QueryResult warm;
  ares_query_dnsrec(channel_, "warmup.example", ARES_CLASS_IN,
                    ARES_REC_TYPE_A, QueryCallback, &warm, NULL);
  Process();
  ASSERT_TRUE(warm.done_);
  ASSERT_EQ(ARES_SUCCESS, warm.status_);

  /* The query goes only to #1, which does not answer.  Meanwhile #0 answers
   * it on #0's own connection.  That answer must be dropped, so the query
   * times out on #1 and is retried on #0, whose genuine answer is accepted on
   * the same (still in sync, for TCP) connection. */
  EXPECT_CALL(*servers_[1], OnRequest("victim.example", T_A))
    .WillOnce(InvokeWithoutArgs([&]() {
      servers_[0]->SendUnsolicited(&forged, qid);
      servers_[1]->SetReplyData(std::vector<byte>());
    }));
  EXPECT_CALL(*servers_[0], OnRequest("victim.example", T_A))
    .WillOnce(SetReply(servers_[0].get(), &genuine));
  QueryResult result;
  ares_query_dnsrec(channel_, "victim.example", ARES_CLASS_IN,
                    ARES_REC_TYPE_A, QueryCallback, &result, &qid);
  Process();
  ASSERT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_EQ("1.2.3.4", ReplyConnFirstA(result));
  EXPECT_EQ(1, (int)result.timeouts_);
}

// DNS cookies (RFC 7873) are per-server state, validated against the server
// the query was sent to.  #1 supports cookies, #0 does not.  A cookie-less
// answer arriving on #0's connection for a query sent only to #1 must not be
// accepted: it would otherwise be checked against #0's cookie state and pass.
class ReplyConnCookieMultiMockTest
  : public MockChannelOptsTest,
    public ::testing::WithParamInterface<int> {
 public:
  ReplyConnCookieMultiMockTest()
    : MockChannelOptsTest(2, GetParam(), false, false,
                          ReplyConnMultiMockTest::FillOptions(&opts_),
                          ARES_OPT_FLAGS | ARES_OPT_TRIES |
                          ARES_OPT_SERVER_FAILOVER | ARES_OPT_NOROTATE) {}
 private:
  struct ares_options opts_;
};

TEST_P(ReplyConnCookieMultiMockTest, CookielessAnswerOnOtherServerDropped) {
  std::vector<byte> server_cookie = { 1, 2, 3, 4, 5, 6, 7, 8 };
  DNSPacket warm_servfail;
  warm_servfail.set_response().set_aa().set_rcode(SERVFAIL)
    .add_question(new DNSQuestion("warmup.example", T_A));
  DNSPacket warm_ok;
  warm_ok.set_response().set_aa()
    .add_question(new DNSQuestion("warmup.example", T_A))
    .add_answer(new DNSARR("warmup.example", 100, {1, 1, 1, 1}))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, { }, server_cookie, false));
  DNSPacket forged;
  forged.set_response().set_aa()
    .add_question(new DNSQuestion("victim.example", T_A))
    .add_answer(new DNSARR("victim.example", 100, {6, 6, 6, 6}));
  DNSPacket genuine;
  genuine.set_response().set_aa()
    .add_question(new DNSQuestion("victim.example", T_A))
    .add_answer(new DNSARR("victim.example", 100, {1, 2, 3, 4}));
  unsigned short qid = 0;

  /* #0 answers without a cookie (and is demoted); #1 answers with one, so
   * #1's cookie state becomes SUPPORTED */
  EXPECT_CALL(*servers_[0], OnRequest("warmup.example", T_A))
    .WillOnce(SetReply(servers_[0].get(), &warm_servfail));
  EXPECT_CALL(*servers_[1], OnRequest("warmup.example", T_A))
    .WillOnce(SetReply(servers_[1].get(), &warm_ok));
  QueryResult warm;
  ares_query_dnsrec(channel_, "warmup.example", ARES_CLASS_IN,
                    ARES_REC_TYPE_A, QueryCallback, &warm, NULL);
  Process();
  ASSERT_TRUE(warm.done_);
  ASSERT_EQ(ARES_SUCCESS, warm.status_);
  size_t cookie_len = 0;
  fetch_server_cookie(warm.dnsrec_.dnsrec_, &cookie_len);
  ASSERT_EQ(server_cookie.size(), cookie_len);

  /* The cookie-protected query goes only to #1; a cookie-less answer for it
   * arrives on #0's connection and must be dropped.  The retry to #0 is then
   * legitimately answered by #0, which does not use cookies. */
  EXPECT_CALL(*servers_[1], OnRequest("victim.example", T_A))
    .WillOnce(InvokeWithoutArgs([&]() {
      servers_[0]->SendUnsolicited(&forged, qid);
      servers_[1]->SetReplyData(std::vector<byte>());
    }));
  EXPECT_CALL(*servers_[0], OnRequest("victim.example", T_A))
    .WillOnce(SetReply(servers_[0].get(), &genuine));
  QueryResult result;
  ares_query_dnsrec(channel_, "victim.example", ARES_CLASS_IN,
                    ARES_REC_TYPE_A, QueryCallback, &result, &qid);
  Process();
  ASSERT_TRUE(result.done_);
  EXPECT_EQ(ARES_SUCCESS, result.status_);
  EXPECT_EQ("1.2.3.4", ReplyConnFirstA(result));
  EXPECT_EQ(1, (int)result.timeouts_);
}

INSTANTIATE_TEST_SUITE_P(TransportModes, ReplyConnMultiMockTest, ::testing::ValuesIn(ares::test::families_modes), PrintFamilyMode);

INSTANTIATE_TEST_SUITE_P(AddressFamilies, ReplyConnCookieMultiMockTest, ::testing::ValuesIn(ares::test::families), PrintFamily);

}  // namespace test
}  // namespace ares
