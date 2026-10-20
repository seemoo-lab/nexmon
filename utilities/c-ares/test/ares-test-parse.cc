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

#include <sstream>
#include <vector>

namespace ares {
namespace test {

TEST_F(LibraryTest, ParseRootName) {
  DNSPacket pkt;
  pkt.set_qid(0x1234).set_response().set_aa()
    .add_question(new DNSQuestion(".", T_A))
    .add_answer(new DNSARR(".", 100, {0x02, 0x03, 0x04, 0x05}));
  std::vector<byte> data = pkt.data();

  struct hostent *host = nullptr;
  struct ares_addrttl info[2];
  int count = 2;
  EXPECT_EQ(ARES_SUCCESS, ares_parse_a_reply(data.data(), (int)data.size(),
                                             &host, info, &count));
  EXPECT_EQ(1, count);
  std::stringstream ss;
  ss << HostEnt(host);
  EXPECT_EQ("{'' aliases=[] addrs=[2.3.4.5]}", ss.str());
  ares_free_hostent(host);
}

TEST_F(LibraryTest, ParseIndirectRootName) {
  std::vector<byte> data = {
    0x12, 0x34,  // qid
    0x84, // response + query + AA + not-TC + not-RD
    0x00, // not-RA + not-Z + not-AD + not-CD + rc=NoError
    0x00, 0x01,  // num questions
    0x00, 0x01,  // num answer RRs
    0x00, 0x00,  // num authority RRs
    0x00, 0x00,  // num additional RRs
    // Question
    0xC0, 0x04,  // weird: pointer to a random zero earlier in the message
    0x00, 0x01,  // type A
    0x00, 0x01,  // class IN
    // Answer 1
    0xC0, 0x04,
    0x00, 0x01,  // RR type
    0x00, 0x01,  // class IN
    0x01, 0x02, 0x03, 0x04, // TTL
    0x00, 0x04,  // rdata length
    0x02, 0x03, 0x04, 0x05,
  };

  struct hostent *host = nullptr;
  struct ares_addrttl info[2];
  int count = 2;
  EXPECT_EQ(ARES_SUCCESS, ares_parse_a_reply(data.data(), (int)data.size(),
                                             &host, info, &count));
  EXPECT_EQ(1, count);
  std::stringstream ss;
  ss << HostEnt(host);
  EXPECT_EQ("{'' aliases=[] addrs=[2.3.4.5]}", ss.str());
  ares_free_hostent(host);
}


#if 0 /* We are validating hostnames now, its not clear how this would ever be valid */
TEST_F(LibraryTest, ParseEscapedName) {
  std::vector<byte> data = {
    0x12, 0x34,  // qid
    0x84, // response + query + AA + not-TC + not-RD
    0x00, // not-RA + not-Z + not-AD + not-CD + rc=NoError
    0x00, 0x01,  // num questions
    0x00, 0x01,  // num answer RRs
    0x00, 0x00,  // num authority RRs
    0x00, 0x00,  // num additional RRs
    // Question
    0x05, 'a', '\\', 'b', '.', 'c',
    0x03, 'c', 'o', 'm',
    0x00,
    0x00, 0x01,  // type A
    0x00, 0x01,  // class IN
    // Answer 1
    0x05, 'a', '\\', 'b', '.', 'c',
    0x03, 'c', 'o', 'm',
    0x00,
    0x00, 0x01,  // RR type
    0x00, 0x01,  // class IN
    0x01, 0x02, 0x03, 0x04, // TTL
    0x00, 0x04,  // rdata length
    0x02, 0x03, 0x04, 0x05,
  };
  struct hostent *host = nullptr;
  struct ares_addrttl info[2];
  int count = 2;
  EXPECT_EQ(ARES_SUCCESS, ares_parse_a_reply(data.data(), data.size(),
                                             &host, info, &count));
  EXPECT_EQ(1, count);
  HostEnt hent(host);
  std::stringstream ss;
  ss << hent;
  // The printable name is expanded with escapes.
  EXPECT_EQ(11, hent.name_.size());
  EXPECT_EQ('a', hent.name_[0]);
  EXPECT_EQ('\\', hent.name_[1]);
  EXPECT_EQ('\\', hent.name_[2]);
  EXPECT_EQ('b', hent.name_[3]);
  EXPECT_EQ('\\', hent.name_[4]);
  EXPECT_EQ('.', hent.name_[5]);
  EXPECT_EQ('c', hent.name_[6]);
  ares_free_hostent(host);
}
#endif

TEST_F(LibraryTest, ParsePartialCompressedName) {
  std::vector<byte> data = {
    0x12, 0x34,  // qid
    0x84, // response + query + AA + not-TC + not-RD
    0x00, // not-RA + not-Z + not-AD + not-CD + rc=NoError
    0x00, 0x01,  // num questions
    0x00, 0x01,  // num answer RRs
    0x00, 0x00,  // num authority RRs
    0x00, 0x00,  // num additional RRs
    // Question
    0x03, 'w', 'w', 'w',
    0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e',
    0x03, 'c', 'o', 'm',
    0x00,
    0x00, 0x01,  // type A
    0x00, 0x01,  // class IN
    // Answer 1
    0x03, 'w', 'w', 'w',
    0xc0, 0x10,  // offset 16
    0x00, 0x01,  // RR type
    0x00, 0x01,  // class IN
    0x01, 0x02, 0x03, 0x04, // TTL
    0x00, 0x04,  // rdata length
    0x02, 0x03, 0x04, 0x05,
  };
  struct hostent *host = nullptr;
  struct ares_addrttl info[2];
  int count = 2;
  EXPECT_EQ(ARES_SUCCESS, ares_parse_a_reply(data.data(), (int)data.size(),
                                             &host, info, &count));
  ASSERT_NE(nullptr, host);
  std::stringstream ss;
  ss << HostEnt(host);
  EXPECT_EQ("{'www.example.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
  ares_free_hostent(host);
}

TEST_F(LibraryTest, ParseFullyCompressedName) {
  std::vector<byte> data = {
    0x12, 0x34,  // qid
    0x84, // response + query + AA + not-TC + not-RD
    0x00, // not-RA + not-Z + not-AD + not-CD + rc=NoError
    0x00, 0x01,  // num questions
    0x00, 0x01,  // num answer RRs
    0x00, 0x00,  // num authority RRs
    0x00, 0x00,  // num additional RRs
    // Question
    0x03, 'w', 'w', 'w',
    0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e',
    0x03, 'c', 'o', 'm',
    0x00,
    0x00, 0x01,  // type A
    0x00, 0x01,  // class IN
    // Answer 1
    0xc0, 0x0c,  // offset 12
    0x00, 0x01,  // RR type
    0x00, 0x01,  // class IN
    0x01, 0x02, 0x03, 0x04, // TTL
    0x00, 0x04,  // rdata length
    0x02, 0x03, 0x04, 0x05,
  };
  struct hostent *host = nullptr;
  struct ares_addrttl info[2];
  int count = 2;
  EXPECT_EQ(ARES_SUCCESS, ares_parse_a_reply(data.data(), (int)data.size(),
                                             &host, info, &count));
  ASSERT_NE(nullptr, host);
  std::stringstream ss;
  ss << HostEnt(host);
  EXPECT_EQ("{'www.example.com' aliases=[] addrs=[2.3.4.5]}", ss.str());
  ares_free_hostent(host);
}

TEST_F(LibraryTest, ParseMalformedRRCount) {
  ares_dns_record_t *dnsrec = NULL;
  const unsigned char data[] = {
    0x12, 0x34, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01,
    0x00, 0x00, 0x00, 0x00, 0x01, 'a',  0x00, 0x00,
    0x01, 0x00, 0x01,
  };

  EXPECT_EQ(ARES_EBADRESP, ares_dns_parse(data, sizeof(data), 0, &dnsrec));
  EXPECT_EQ(nullptr, dnsrec);
}

TEST_F(LibraryTest, ParseRejectsOverlongName) {
  // RFC 1035 3.1 limits a name to 255 octets.  A longer name (here built from
  // six 63-octet labels) must be rejected instead of expanded.
  std::vector<byte> data = {
    0x12, 0x34, 0x81, 0x80,
    0x00, 0x01,  // num questions
    0x00, 0x01,  // num answer RRs
    0x00, 0x00,  // num authority RRs
    0x00, 0x00,  // num additional RRs
    // Question
    0x01, 'a', 0x00,
    0x00, 0x01,  // type A
    0x00, 0x01,  // class IN
  };
  // Answer name: 6 x 63 label octets + 5 separators = 383 presentation octets
  // (> 255), then root terminator
  for (int i = 0; i < 6; i++) {
    data.push_back(63);
    for (int j = 0; j < 63; j++) {
      data.push_back('a');
    }
  }
  data.push_back(0x00);
  std::vector<byte> tail = {
    0x00, 0x01,              // type A
    0x00, 0x01,              // class IN
    0x00, 0x00, 0x00, 0x00,  // TTL
    0x00, 0x04,              // rdata length
    0x01, 0x02, 0x03, 0x04,
  };
  data.insert(data.end(), tail.begin(), tail.end());

  ares_dns_record_t *dnsrec = NULL;
  EXPECT_EQ(ARES_EBADNAME, ares_dns_parse(data.data(), data.size(), 0, &dnsrec));
  EXPECT_EQ(nullptr, dnsrec);
}

TEST_F(LibraryTest, ParseRejectsOverlongNameViaPointers) {
  // The answer name is one 63-octet label followed by a compression pointer back
  // to the question name (four 63-octet labels).  Expanded that is 5 x 63 label
  // octets + 4 separators = 319 presentation octets (> 255), so the length has
  // to accumulate across the pointer jump for this to be rejected.
  std::vector<byte> data = {
    0x12, 0x34, 0x81, 0x80,
    0x00, 0x01,  // num questions
    0x00, 0x01,  // num answer RRs
    0x00, 0x00,  // num authority RRs
    0x00, 0x00,  // num additional RRs
  };
  // Question name: 4 x 63 label octets + 3 separators = 255 presentation octets,
  // exactly the limit, so the question itself is accepted.
  for (int i = 0; i < 4; i++) {
    data.push_back(63);
    for (int j = 0; j < 63; j++) {
      data.push_back('a');
    }
  }
  data.push_back(0x00);
  std::vector<byte> qtail = {0x00, 0x01, 0x00, 0x01};  // type A, class IN
  data.insert(data.end(), qtail.begin(), qtail.end());
  // Answer name: one 63-octet label, then a pointer to the question name.
  data.push_back(63);
  for (int j = 0; j < 63; j++) {
    data.push_back('a');
  }
  data.push_back(0xc0);
  data.push_back(0x0c);  // -> offset 12, start of question name
  std::vector<byte> tail = {
    0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 1, 2, 3, 4,
  };
  data.insert(data.end(), tail.begin(), tail.end());
  ares_dns_record_t *dnsrec = NULL;
  EXPECT_EQ(ARES_EBADNAME, ares_dns_parse(data.data(), data.size(), 0, &dnsrec));
  EXPECT_EQ(nullptr, dnsrec);
}

TEST_F(LibraryTest, ParseRejectsPointerChain) {
  // A name built purely from compression pointers adds no label octets, so the
  // length cap alone can never stop it.  A long backward chain of pointers is
  // hidden in the opaque rdata of a raw RR, and a second RR's name points at the
  // top of the chain; walking it must trip the indirection cap and return
  // EBADNAME rather than following every jump.
  std::vector<byte> data = {
    0x12, 0x34, 0x81, 0x80,
    0x00, 0x01,  // num questions
    0x00, 0x02,  // num answer RRs
    0x00, 0x00,  // num authority RRs
    0x00, 0x00,  // num additional RRs
    // Question
    0x01, 'a', 0x00,
    0x00, 0x01,  // type A
    0x00, 0x01,  // class IN
    // RR1: raw RR (unknown type) whose rdata carries the pointer chain
    0x00,        // name = root
    0xff, 0xfe,  // type (unknown -> stored raw, rdata not name-parsed)
    0x00, 0x01,  // class IN
    0x00, 0x00, 0x00, 0x00,  // TTL
  };
  // The chain lives in RR1's rdata, which begins two octets past here (after the
  // rdlength we are about to write), so compute absolute offsets from there.
  const size_t rdata_off = data.size() + 2;
  std::vector<byte> chain;
  chain.push_back(0x00);          // deepest target: root terminator
  size_t prev = rdata_off;        // absolute offset of the root
  size_t top  = prev;
  const int N = 200;              // well past the 128 indirection cap
  for (int i = 0; i < N; i++) {
    size_t here = rdata_off + chain.size();
    chain.push_back(0xc0 | ((prev >> 8) & 0x3f));
    chain.push_back(prev & 0xff);
    top  = here;
    prev = here;
  }
  data.push_back((chain.size() >> 8) & 0xff);  // rdlength
  data.push_back(chain.size() & 0xff);
  data.insert(data.end(), chain.begin(), chain.end());
  // RR2: name = pointer to the top of the chain.
  data.push_back(0xc0 | ((top >> 8) & 0x3f));
  data.push_back(top & 0xff);
  std::vector<byte> tail = {
    0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 1, 2, 3, 4,
  };
  data.insert(data.end(), tail.begin(), tail.end());

  ares_dns_record_t *dnsrec = NULL;
  EXPECT_EQ(ARES_EBADNAME, ares_dns_parse(data.data(), data.size(), 0, &dnsrec));
  EXPECT_EQ(nullptr, dnsrec);
}

TEST_F(LibraryTest, ParseAcceptsMaxLengthName) {
  // Exactly 255 presentation octets: 4 x 63 = 252 + 3 separators = 255, which
  // must still be accepted (the bound is "> 255", not ">= 255").
  std::vector<byte> data = {
    0x12, 0x34, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x01, 'a',  0x00, 0x00, 0x01, 0x00, 0x01,
  };
  for (int i = 0; i < 4; i++) {
    data.push_back(63);
    for (int j = 0; j < 63; j++) {
      data.push_back('a');
    }
  }
  data.push_back(0x00);
  std::vector<byte> tail = {
    0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 1, 2, 3, 4,
  };
  data.insert(data.end(), tail.begin(), tail.end());
  ares_dns_record_t *dnsrec = NULL;
  EXPECT_EQ(ARES_SUCCESS, ares_dns_parse(data.data(), data.size(), 0, &dnsrec));
  EXPECT_NE(nullptr, dnsrec);
  ares_dns_record_destroy(dnsrec);
}

TEST_F(LibraryTest, ParseRejectsName256) {
  // One octet over: labels 63,63,63,62,1 = 252 + 4 separators = 256 -> reject.
  std::vector<byte> data = {
    0x12, 0x34, 0x81, 0x80, 0x00, 0x01, 0x00, 0x01, 0x00, 0x00,
    0x00, 0x00, 0x01, 'a',  0x00, 0x00, 0x01, 0x00, 0x01,
  };
  const int lens[] = {63, 63, 63, 62, 1};
  for (int i = 0; i < 5; i++) {
    data.push_back(static_cast<byte>(lens[i]));
    for (int j = 0; j < lens[i]; j++) {
      data.push_back('a');
    }
  }
  data.push_back(0x00);
  std::vector<byte> tail = {
    0x00, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x04, 1, 2, 3, 4,
  };
  data.insert(data.end(), tail.begin(), tail.end());
  ares_dns_record_t *dnsrec = NULL;
  EXPECT_EQ(ARES_EBADNAME, ares_dns_parse(data.data(), data.size(), 0, &dnsrec));
  EXPECT_EQ(nullptr, dnsrec);
}

// A single OPT record in the additional section is valid (EDNS0).
TEST_F(LibraryTest, ParseSingleOpt) {
  DNSPacket pkt;
  pkt.set_qid(0x1234).set_response()
    .add_question(new DNSQuestion("example.com", T_A))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, {}, {}, false));
  std::vector<byte> data = pkt.data();

  ares_dns_record_t *dnsrec = NULL;
  EXPECT_EQ(ARES_SUCCESS, ares_dns_parse(data.data(), data.size(), 0, &dnsrec));
  EXPECT_NE(nullptr, dnsrec);
  ares_dns_record_destroy(dnsrec);
}

// RFC 6891 6.1.1: more than one OPT record in a message is a format error.
TEST_F(LibraryTest, ParseMultipleOptRejected) {
  DNSPacket pkt;
  pkt.set_qid(0x1234).set_response()
    .add_question(new DNSQuestion("example.com", T_A))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, {}, {}, false))
    .add_additional(new DNSOptRR(0, 0, 0, 1280, {}, {}, false));
  std::vector<byte> data = pkt.data();

  ares_dns_record_t *dnsrec = NULL;
  EXPECT_EQ(ARES_EBADRESP, ares_dns_parse(data.data(), data.size(), 0, &dnsrec));
  EXPECT_EQ(nullptr, dnsrec);
}

// RFC 3597: only RR types defined in RFC 1035 may use name compression within
// their RDATA.  A modern type such as SVCB must reject a compressed target,
// same as SRV -- this exercises the shared policy, not an SRV special case.
TEST_F(LibraryTest, ParseSvcbRejectsCompressedTarget) {
  const unsigned char data[] = {
    0x12, 0x34,              // qid
    0x84, 0x00,              // response + AA
    0x00, 0x01,              // qdcount
    0x00, 0x01,              // ancount
    0x00, 0x00,              // nscount
    0x00, 0x00,              // arcount
    // Question (name at offset 0x0c)
    0x07, 'e', 'x', 'a', 'm', 'p', 'l', 'e',
    0x03, 'c', 'o', 'm',
    0x00,
    0x00, 0x40,              // type SVCB (64)
    0x00, 0x01,              // class IN
    // Answer
    0xc0, 0x0c,              // name -> pointer to example.com
    0x00, 0x40,              // type SVCB
    0x00, 0x01,              // class IN
    0x00, 0x00, 0x00, 0x3c,  // TTL
    0x00, 0x04,              // rdlength
    0x00, 0x01,              // SvcPriority
    0xc0, 0x0c,              // TargetName -> compression pointer (illegal)
  };

  ares_dns_record_t *dnsrec = NULL;
  EXPECT_EQ(ARES_EBADNAME, ares_dns_parse(data, sizeof(data), 0, &dnsrec));
  EXPECT_EQ(nullptr, dnsrec);
}

}  // namespace test
}  // namespace ares
