/*
 * Routines for wimaxmacphy (WiMAX MAX SHY over Ethernet) packet dissection
 * Copyright 2008, Mobile Metrics - http://mobilemetrics.net/
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __PACKET_WIMAXASNCP_H__
#define __PACKET_WIMAXASNCP_H__

/**
 * @brief Registers the WiMAX MAC PHY protocol.
 *
 * This function registers the WiMAX MAC PHY protocol with Wireshark,
 * including its header fields and dissector functions.
 */
void proto_register_wimaxmacphy (void);

/**
 * @brief Registers the handoff function for the WiMAX MAC PHY protocol.
 *
 * This function registers the handoff function for the WiMAX MAC PHY protocol with Wireshark.
 */
void proto_reg_handoff_wimaxmacphy(void);

#endif  /* __PACKET_WIMAXASNCP_H__ */
