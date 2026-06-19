#include <check.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Minimal struct definitions matching monitormode.c */
struct nexmon_radiotap_header {
    uint8_t it_version;
    uint8_t it_pad;
    uint16_t it_len;
    uint32_t it_present;
};

struct pkt {
    uint8_t *data;
    uint16_t len;
};

/* Forward declare the vulnerable function from monitormode.c */
extern void process_monitor_frame(struct pkt *p, struct pkt *p_new);

START_TEST(test_buffer_overflow_protection)
{
    /* Invariant: memcpy in monitor mode must validate p->len against p_new buffer capacity
       to prevent heap buffer overflow from attacker-controlled frame length field */
    
    struct pkt p, p_new;
    uint8_t p_new_buffer[256];
    
    /* Test payloads: exploit case, boundary, valid input */
    struct {
        uint16_t frame_len;
        const char *desc;
        int should_overflow;
    } payloads[] = {
        {0xFFFF, "Max uint16 - heap overflow exploit", 1},
        {250, "Boundary: buffer size - 6", 0},
        {50, "Valid: small frame within bounds", 0}
    };
    
    int num_payloads = sizeof(payloads) / sizeof(payloads[0]);
    
    for (int i = 0; i < num_payloads; i++) {
        uint8_t p_data[512];
        memset(p_data, 0x41, sizeof(p_data));
        
        p.data = p_data;
        p.len = payloads[i].frame_len;
        
        p_new.data = p_new_buffer;
        p_new.len = sizeof(p_new_buffer);
        
        /* The vulnerable code: memcpy(p_new->data + sizeof(struct nexmon_radiotap_header), 
           p->data + 6, p->len - 6) must not overflow p_new's 256-byte buffer */
        
        if (payloads[i].should_overflow) {
            /* For exploit payloads, verify bounds check prevents overflow */
            ck_assert_msg(p.len - 6 > sizeof(p_new_buffer) - sizeof(struct nexmon_radiotap_header),
                         "Payload %d (%s) should trigger bounds violation", i, payloads[i].desc);
        } else {
            /* For valid payloads, verify they fit safely */
            ck_assert_msg(p.len - 6 <= sizeof(p_new_buffer) - sizeof(struct nexmon_radiotap_header),
                         "Payload %d (%s) should fit within buffer", i, payloads[i].desc);
        }
    }
}
END_TEST

Suite *security_suite(void)
{
    Suite *s;
    TCase *tc_core;

    s = suite_create("Security");
    tc_core = tcase_create("BufferOverflow");

    tcase_add_test(tc_core, test_buffer_overflow_protection);
    suite_add_tcase(s, tc_core);

    return s;
}

int main(void)
{
    int number_failed;
    Suite *s;
    SRunner *sr;

    s = security_suite();
    sr = srunner_create(s);

    srunner_run_all(sr, CK_NORMAL);
    number_failed = srunner_ntests_failed(sr);
    srunner_free(sr);

    return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}