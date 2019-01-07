/*
 * Copyright 2013 The Android Open Source Project
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Google Inc. nor the names of its contributors may
 *       be used to endorse or promote products derived from this software
 *       without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY Google Inc. ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL Google Inc. BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/cdefs.h>

#include "mincrypt/dsa_sig.h"
#include "mincrypt/p256.h"
#include "mincrypt/p256_ecdsa.h"
#include "mincrypt/sha256.h"

#ifndef __unused
#define __unused __attribute__((__unused__))
#endif

/**
 * Messages signed using:
 *
-----BEGIN EC PRIVATE KEY-----
MHcCAQEEIDw6UiziVMbjlfSpOAIpA2tcL+v1OlznZLnpadO8BGi1oAoGCCqGSM49
AwEHoUQDQgAEZw7VAOjAXYRFuhZWYBgjahdOvkwcAnjGkxQWytZW+iS1hI3ZGE24
6XmNka9IGxAgj2n/ip+MuZJMFoJ9DRea3g==
-----END EC PRIVATE KEY-----
 */

p256_int key_x = {
    .a = {0xd656fa24u, 0x931416cau, 0x1c0278c6u, 0x174ebe4cu,
          0x6018236au, 0x45ba1656u, 0xe8c05d84u, 0x670ed500u}
};
p256_int key_y = {
    .a = {0x0d179adeu, 0x4c16827du, 0x9f8cb992u, 0x8f69ff8au,
          0x481b1020u, 0x798d91afu, 0x184db8e9u, 0xb5848dd9u}
};

char* message_1 =
    "f4 5d 55 f3 55 51 e9 75 d6 a8 dc 7e a9 f4 88 59"
    "39 40 cc 75 69 4a 27 8f 27 e5 78 a1 63 d8 39 b3"
    "40 40 84 18 08 cf 9c 58 c9 b8 72 8b f5 f9 ce 8e"
    "e8 11 ea 91 71 4f 47 ba b9 2d 0f 6d 5a 26 fc fe"
    "ea 6c d9 3b 91 0c 0a 2c 96 3e 64 eb 18 23 f1 02"
    "75 3d 41 f0 33 59 10 ad 3a 97 71 04 f1 aa f6 c3"
    "74 27 16 a9 75 5d 11 b8 ee d6 90 47 7f 44 5c 5d"
    "27 20 8b 2e 28 43 30 fa 3d 30 14 23 fa 7f 2d 08"
    "6e 0a d0 b8 92 b9 db 54 4e 45 6d 3f 0d ab 85 d9"
    "53 c1 2d 34 0a a8 73 ed a7 27 c8 a6 49 db 7f a6"
    "37 40 e2 5e 9a f1 53 3b 30 7e 61 32 99 93 11 0e"
    "95 19 4e 03 93 99 c3 82 4d 24 c5 1f 22 b2 6b de"
    "10 24 cd 39 59 58 a2 df eb 48 16 a6 e8 ad ed b5"
    "0b 1f 6b 56 d0 b3 06 0f f0 f1 c4 cb 0d 0e 00 1d"
    "d5 9d 73 be 12";

char* signature_1 =
    "30 44 02 20 43 18 fc eb 3b a8 3a a8 a3 cf 41 b7"
    "81 4a f9 01 e1 8b 6e 95 c1 3a 83 25 9e a5 2e 66"
    "7c 98 25 d9 02 20 54 f3 7f 5a e9 36 9c a2 f0 51"
    "e0 6e 78 48 60 a3 f9 8a d5 2c 37 5a 0a 29 c9 f7"
    "ea 57 7e 88 46 12";

// Same as signature 1, but with leading zeroes.
char* message_2 =
    "f4 5d 55 f3 55 51 e9 75 d6 a8 dc 7e a9 f4 88 59"
    "39 40 cc 75 69 4a 27 8f 27 e5 78 a1 63 d8 39 b3"
    "40 40 84 18 08 cf 9c 58 c9 b8 72 8b f5 f9 ce 8e"
    "e8 11 ea 91 71 4f 47 ba b9 2d 0f 6d 5a 26 fc fe"
    "ea 6c d9 3b 91 0c 0a 2c 96 3e 64 eb 18 23 f1 02"
    "75 3d 41 f0 33 59 10 ad 3a 97 71 04 f1 aa f6 c3"
    "74 27 16 a9 75 5d 11 b8 ee d6 90 47 7f 44 5c 5d"
    "27 20 8b 2e 28 43 30 fa 3d 30 14 23 fa 7f 2d 08"
    "6e 0a d0 b8 92 b9 db 54 4e 45 6d 3f 0d ab 85 d9"
    "53 c1 2d 34 0a a8 73 ed a7 27 c8 a6 49 db 7f a6"
    "37 40 e2 5e 9a f1 53 3b 30 7e 61 32 99 93 11 0e"
    "95 19 4e 03 93 99 c3 82 4d 24 c5 1f 22 b2 6b de"
    "10 24 cd 39 59 58 a2 df eb 48 16 a6 e8 ad ed b5"
    "0b 1f 6b 56 d0 b3 06 0f f0 f1 c4 cb 0d 0e 00 1d"
    "d5 9d 73 be 12";

char* signature_2 =
    "30 46 02 21 00 43 18 fc eb 3b a8 3a a8 a3 cf 41 b7"
    "81 4a f9 01 e1 8b 6e 95 c1 3a 83 25 9e a5 2e 66"
    "7c 98 25 d9 02 21 00 54 f3 7f 5a e9 36 9c a2 f0 51"
    "e0 6e 78 48 60 a3 f9 8a d5 2c 37 5a 0a 29 c9 f7"
    "ea 57 7e 88 46 12";

// Excessive zeroes on the signature
char* message_3 =
    "f4 5d 55 f3 55 51 e9 75 d6 a8 dc 7e a9 f4 88 59"
    "39 40 cc 75 69 4a 27 8f 27 e5 78 a1 63 d8 39 b3"
    "40 40 84 18 08 cf 9c 58 c9 b8 72 8b f5 f9 ce 8e"
    "e8 11 ea 91 71 4f 47 ba b9 2d 0f 6d 5a 26 fc fe"
    "ea 6c d9 3b 91 0c 0a 2c 96 3e 64 eb 18 23 f1 02"
    "75 3d 41 f0 33 59 10 ad 3a 97 71 04 f1 aa f6 c3"
    "74 27 16 a9 75 5d 11 b8 ee d6 90 47 7f 44 5c 5d"
    "27 20 8b 2e 28 43 30 fa 3d 30 14 23 fa 7f 2d 08"
    "6e 0a d0 b8 92 b9 db 54 4e 45 6d 3f 0d ab 85 d9"
    "53 c1 2d 34 0a a8 73 ed a7 27 c8 a6 49 db 7f a6"
    "37 40 e2 5e 9a f1 53 3b 30 7e 61 32 99 93 11 0e"
    "95 19 4e 03 93 99 c3 82 4d 24 c5 1f 22 b2 6b de"
    "10 24 cd 39 59 58 a2 df eb 48 16 a6 e8 ad ed b5"
    "0b 1f 6b 56 d0 b3 06 0f f0 f1 c4 cb 0d 0e 00 1d"
    "d5 9d 73 be 12";

char* signature_3 =
    "30 4c 02 24 00 00 00 00 43 18 fc eb 3b a8 3a a8 a3 cf 41 b7"
    "81 4a f9 01 e1 8b 6e 95 c1 3a 83 25 9e a5 2e 66"
    "7c 98 25 d9 02 24 00 00 00 00 54 f3 7f 5a e9 36 9c a2 f0 51"
    "e0 6e 78 48 60 a3 f9 8a d5 2c 37 5a 0a 29 c9 f7"
    "ea 57 7e 88 46 12";


char* good_dsa_signature_1 =
    "30 0D 02 01 01 02 08 00 A5 55 5A 01 FF A5 01";
p256_int good_dsa_signature_1_r = {
    .a = {0x00000001U, 0x00000000U, 0x00000000U, 0x00000000U,
          0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U}
};
p256_int good_dsa_signature_1_s = {
    .a = {0x01FFA501U, 0x00A5555AU, 0x00000000U, 0x00000000U,
          0x00000000U, 0x00000000U, 0x00000000U, 0x00000000U}
};


char* bad_dsa_signature_1 =
     "a0 06 02 01 01 02 01 01";

char* bad_dsa_signature_2 =
     "30 07 02 01 01 02 01 01";

char* bad_dsa_signature_3 =
     "30 06 82 01 01 02 01 01";

char* bad_dsa_signature_4 =
     "30 06 02 00 01 02 01 01";

char* bad_dsa_signature_5 =
     "30 06 02 01 01 82 01 01";

char* bad_dsa_signature_6 =
     "30 05 02 01 01 02 00";

char* bad_dsa_signature_7 =
     "30 06 02 01 01 02 00 01";

unsigned char* parsehex(char* str, int* len) {
    // result can't be longer than input
    unsigned char* result = malloc(strlen(str));

    unsigned char* p = result;
    *len = 0;

    while (*str) {
        int b;

        while (isspace(*str)) str++;

        switch (*str) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                b = (*str - '0') << 4; break;
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                b = (*str - 'a' + 10) << 4; break;
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                b = (*str - 'A' + 10) << 4; break;
            case '\0':
                return result;
            default:
                return NULL;
        }
        str++;

        while (isspace(*str)) str++;

        switch (*str) {
            case '0': case '1': case '2': case '3': case '4':
            case '5': case '6': case '7': case '8': case '9':
                b |= *str - '0'; break;
            case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                b |= *str - 'a' + 10; break;
            case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                b |= *str - 'A' + 10; break;
            default:
                return NULL;
        }
        str++;

        *p++ = b;
        ++*len;
    }

    return result;
}

int main(int arg __unused, char** argv __unused) {

    unsigned char hash_buf[SHA256_DIGEST_SIZE];

    unsigned char* message;
    int mlen;
    unsigned char* signature;
    int slen;

    p256_int hash;
    p256_int r;
    p256_int s;

    int success = 1;

#define CHECK_DSA_SIG(sig, good) do {\
    message = parsehex(sig, &mlen); \
    int result = dsa_sig_unpack(message, mlen, &r, &s); \
    printf(#sig ": %s\n", result ? "good" : "bad"); \
    success = success && !(good ^ result); \
    free(message); \
    } while(0)
#define CHECK_GOOD_DSA_SIG(n) do {\
    CHECK_DSA_SIG(good_dsa_signature_##n, 1); \
    int result = !memcmp(P256_DIGITS(&good_dsa_signature_##n##_r), P256_DIGITS(&r), \
                         P256_NBYTES); \
    success = success && result; \
    printf("    R value %s\n", result ? "good" : "bad"); \
    result = !memcmp(P256_DIGITS(&good_dsa_signature_##n##_s), P256_DIGITS(&s), \
                    P256_NBYTES); \
    success = success && result; \
    printf("    S value %s\n", result ? "good" : "bad"); \
    } while (0)
#define CHECK_BAD_DSA_SIG(n) \
    CHECK_DSA_SIG(bad_dsa_signature_##n, 0)

    CHECK_GOOD_DSA_SIG(1);

    CHECK_BAD_DSA_SIG(1);
    CHECK_BAD_DSA_SIG(2);
    CHECK_BAD_DSA_SIG(3);
    CHECK_BAD_DSA_SIG(4);
    CHECK_BAD_DSA_SIG(5);
    CHECK_BAD_DSA_SIG(6);
    CHECK_BAD_DSA_SIG(7);


#define TEST_MESSAGE(n) do {\
    message = parsehex(message_##n, &mlen); \
    SHA256_hash(message, mlen, hash_buf); \
    p256_from_bin(hash_buf, &hash); \
    signature = parsehex(signature_##n, &slen); \
    int result = dsa_sig_unpack(signature, slen, &r, &s); \
    if (result) { result = p256_ecdsa_verify(&key_x, &key_y, &hash, &r, &s); } \
    printf("message %d: %s\n", n, result ? "verified" : "not verified"); \
    success = success && result; \
    free(signature); \
    } while(0)

    TEST_MESSAGE(1);
    TEST_MESSAGE(2);
    TEST_MESSAGE(3);

    printf("\n%s\n\n", success ? "PASS" : "FAIL");

    return !success;
}
