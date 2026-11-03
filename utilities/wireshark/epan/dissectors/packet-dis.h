/* packet-dis.h
 * Routines for DIS tap data.
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __PACKET_DIS_H__
#define __PACKET_DIS_H__

#include <stdbool.h>
#include <stdint.h>

#define DIS_PAYLOAD_TYPE_INVALID 0xff
#define DIS_TAP_PDU_TRANSMITTER 25
#define DIS_TAP_PDU_SIGNAL 26

typedef struct dis_tap_info {
    bool info_valid_radio_pdu_tap;
    uint8_t info_pdu_type;
    uint16_t info_radio_id;
    uint16_t info_entity_id_site;
    uint16_t info_entity_id_appl;
    uint16_t info_entity_id_entity;
    uint8_t info_transmit_state;
    uint8_t info_radio_input_source;
    uint8_t info_payload_type;
    uint32_t info_tx_timestamp;
    uint32_t info_sample_rate;
    uint32_t info_num_samples;
    bool info_all_data_present;
    unsigned info_payload_offset;
    unsigned info_payload_len;
    const uint8_t *info_data;
    const char *info_payload_type_str;
} dis_tap_info_t;

#endif /* __PACKET_DIS_H__ */
