/* fuzz_radiotap.c — AFL/libFuzzer harness for rawproxy radiotap parsing (rawproxy.c:166)
 *
 * Extracts the radiotap length check that was hardened against 0xFFFF pcap:
 *   rtap_len = (packet[3] << 8) | packet[2];
 *   if (rtap_len < 0 || rtap_len > caplen) continue;
 *
 * Build:
 *   gcc -o fuzz_radiotap fuzz_radiotap.c -DTEST_HARNESS
 *   clang -fsanitize=address,undefined -o fuzz_radiotap fuzz_radiotap.c
 *   clang -fsanitize=fuzzer,address -o fuzz_radiotap_fuzzer fuzz_radiotap.c  # libFuzzer
 *   afl-gcc -o fuzz_radiotap_afl fuzz_radiotap.c
 *
 * Run:
 *   ./fuzz_radiotap < corpus/pcap_sample
 *   ./fuzz_radiotap_fuzzer corpus/
 *   afl-fuzz -i corpus -o findings ./fuzz_radiotap_afl
 */

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef TEST_HARNESS
#include <assert.h>
#endif

/* Extracted from rawproxy.c:166 — must stay in sync */
static int rawproxy_radiotap_check(const uint8_t *packet, uint32_t caplen, uint32_t *out_p_len) {
    if (caplen < 8) return 0; /* would continue (skip) */
    int rtap_len = ((packet[3] & 0xff) << 8) | (packet[2] & 0xff);
    if (rtap_len < 8 || (uint32_t)rtap_len > caplen) return 0;
    if (out_p_len) *out_p_len = caplen - (uint32_t)rtap_len;
    return 1; /* would process */
}

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    /* data is treated as a raw pcap packet payload with caplen=size */
    uint32_t p_len = 0;
    rawproxy_radiotap_check(data, (uint32_t)size, &p_len);
    /* Also test the pcap_next -> header.caplen path by feeding size as caplen */
    /* Ensure no crash on any input, including 0xFFFF */
    return 0;
}
#endif

#ifdef __AFL_FUZZ_TESTCASE_LEN
__AFL_FUZZ_INIT();
int main(int argc, char **argv) {
    (void)argc; (void)argv;
#ifdef __AFL_HAVE_MANUAL_CONTROL
    __AFL_INIT();
#endif
    unsigned char *buf = __AFL_FUZZ_TESTCASE_BUF;
    while (__AFL_LOOP(10000)) {
        size_t len = __AFL_FUZZ_TESTCASE_LEN;
        uint32_t p_len = 0;
        if (len >= 4) {
            rawproxy_radiotap_check(buf, (uint32_t)len, &p_len);
        } else {
            rawproxy_radiotap_check(buf, (uint32_t)len, NULL);
        }
    }
    return 0;
}
#elif defined(TEST_HARNESS)
int main(void) {
    /* Deterministic unit tests for the 0xFFFF regression */
    struct {
        uint8_t packet[8];
        uint32_t caplen;
        int expect_process;
    } cases[] = {
        /* caplen 100, rtap_len 0xFFFF -> reject */
        { {0,0, 0xFF,0xFF, 0,0,0,0}, 100, 0 },
        /* caplen 65535, rtap_len 0xFFFF (equal) -> process (p_len 0) */
        { {0,0, 0xFF,0xFF, 0,0,0,0}, 65535, 1 },
        /* caplen 65534, rtap_len 0xFFFF -> reject */
        { {0,0, 0xFF,0xFF, 0,0,0,0}, 65534, 0 },
        /* normal: caplen 100, rtap_len 10 -> process */
        { {0,0, 0x0A,0x00, 0,0,0,0}, 100, 1 },
        /* minimum valid radiotap header: caplen 100, rtap_len 8 -> process */
        { {0,0, 0x08,0x00, 0,0,0,0}, 100, 1 },
        /* caplen <8 -> reject */
        { {0,0, 0x08,0x00, 0,0,0,0}, 7, 0 },
        /* rtap_len 0 (<8) -> reject */
        { {0,0, 0x00,0x00, 0,0,0,0}, 100, 0 },
        /* rtap_len 7 (<8) -> reject */
        { {0,0, 0x07,0x00, 0,0,0,0}, 100, 0 },
    };
    for (size_t i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        uint32_t p_len = 0xDEADBEEF;
        int got = rawproxy_radiotap_check(cases[i].packet, cases[i].caplen, &p_len);
        if (got != cases[i].expect_process) {
            fprintf(stderr, "FAIL case %zu: caplen=%u rtap_len=%d expect %d got %d\n",
                    i, cases[i].caplen,
                    (cases[i].packet[3]<<8)|cases[i].packet[2],
                    cases[i].expect_process, got);
            return 1;
        }
        if (got && p_len != cases[i].caplen - (((cases[i].packet[3]<<8)|cases[i].packet[2]))) {
            fprintf(stderr, "FAIL p_len case %zu\n", i);
            return 1;
        }
    }
    /* Fuzz smoke: 1000 random */
    for (int i = 0; i < 1000; i++) {
        uint8_t buf[8];
        for (int j=0;j<8;j++) buf[j] = rand() & 0xff;
        uint32_t cap = rand() % 70000;
        uint32_t p_len;
        rawproxy_radiotap_check(buf, cap, &p_len);
    }
    printf("PASS: all radiotap checks ok\n");
    return 0;
}
#else
/* Normal build — no harness, just a stub so the file compiles as part of rawproxy */
int rawproxy_fuzz_stub(void) { return 0; }
#endif
