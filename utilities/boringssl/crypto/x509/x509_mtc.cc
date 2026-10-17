// Copyright 2026 The BoringSSL Authors
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include <openssl/base.h>
#include <openssl/bytestring.h>
#include <openssl/digest.h>
#include <openssl/evp.h>
#include <openssl/span.h>

#include "../internal.h"
#include "./internal.h"


BSSL_NAMESPACE_BEGIN
namespace {

// Prefix for domain separation to denote a Merkle Tree interior node.
const uint8_t kMtcNodeDomainSeparator[] = {0x01};

// HashNode executes one hashing step in the evaluation of an inclusion proof.
void HashNode(Span<uint8_t> out, const EVP_MD *log_hash,
              Span<const uint8_t> left_child, Span<const uint8_t> right_child) {
  assert(out.size() == EVP_MD_size(log_hash));
  assert(left_child.size() == EVP_MD_size(log_hash));
  assert(right_child.size() == EVP_MD_size(log_hash));
  ScopedEVP_MD_CTX ctx;
  EVP_DigestInit_ex(ctx.get(), log_hash, nullptr);
  EVP_DigestUpdate(ctx.get(), kMtcNodeDomainSeparator,
                   sizeof(kMtcNodeDomainSeparator));
  EVP_DigestUpdate(ctx.get(), left_child.data(), left_child.size());
  EVP_DigestUpdate(ctx.get(), right_child.data(), right_child.size());
  EVP_DigestFinal_ex(ctx.get(), out.data(), nullptr);
}

// lsb returns whether the least-significant bit of `n` is set.
inline bool lsb(uint64_t n) { return n & 1; }

// Returns the first hash value (of size `log_hash_size`) remaining in
// `inclusion_proof` and advances `inclusion_proof` past the returned value.
// Returns an empty span if there are no more hash values of the appropriate
// size.
Span<const uint8_t> GetNextValueFromInclusionProof(
    size_t log_hash_size, Span<const uint8_t> &inclusion_proof) {
  if (inclusion_proof.size() < log_hash_size) {
    return Span<const uint8_t>();
  }
  Span<const uint8_t> value = inclusion_proof.first(log_hash_size);
  inclusion_proof = inclusion_proof.subspan(log_hash_size);
  return value;
}

}  // namespace

bool x509_evaluate_mtc_subtree_inclusion_proof(
    Span<uint8_t> out, const EVP_MD *log_hash,
    Span<const uint8_t> inclusion_proof, uint64_t index,
    Span<const uint8_t> entry_hash, uint64_t subtree_start,
    uint64_t subtree_end) {
  const size_t log_hash_size = EVP_MD_size(log_hash);
  if (out.size() != log_hash_size || entry_hash.size() != log_hash_size) {
    return false;
  }

  // Check that `subtree_start` and `subtree_end` define a valid subtree.
  if (subtree_start > subtree_end) {
    return false;
  }
  // The subtree must be aligned and not have a ragged left edge, i.e. the size
  // must not exceed the largest power of 2 that divides the start index.
  const uint64_t subtree_size = subtree_end - subtree_start;
  if (subtree_start != 0 &&
      subtree_size > (subtree_start & (~subtree_start + 1))) {
    return false;
  }
  // Check that `index` is in range for the subtree.
  if (index < subtree_start || subtree_end <= index) {
    return false;
  }

  OPENSSL_memcpy(out.data(), entry_hash.data(), log_hash_size);

  // `fn` is the index of the entry if the subtree were re-numbered to start at
  // 0, and `sn` is what the last entry of such a re-numbered subtree would be.
  uint64_t fn = index - subtree_start;
  uint64_t sn = subtree_size - 1;
  while (!inclusion_proof.empty()) {
    Span<const uint8_t> p =
        GetNextValueFromInclusionProof(log_hash_size, inclusion_proof);
    if (p.empty()) {
      // Truncated hash in inclusion proof, or trailing data after last full
      // hash.
      return false;
    }
    assert(p.size() == log_hash_size);
    if (sn == 0) {
      // More hashes in the inclusion proof than expected.
      return false;
    }
    if (lsb(fn) || fn == sn) {
      HashNode(out, log_hash, /*left_child=*/p, /*right_child=*/out);
      while (!lsb(fn)) {
        fn >>= 1;
        sn >>= 1;
      }
    } else {
      HashNode(out, log_hash, /*left_child=*/out, /*right_child=*/p);
    }
    fn >>= 1;
    sn >>= 1;
  }

  if (sn != 0) {
    // Not enough hashes in inclusion proof.
    return false;
  }

  return true;
}

BSSL_NAMESPACE_END
