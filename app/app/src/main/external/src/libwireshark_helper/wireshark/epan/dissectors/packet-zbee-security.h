/* packet-zbee-security.h
 * Dissector helper routines for encrypted ZigBee frames.
 * By Owen Kirby <osk@exegin.com>
 * Copyright 2009 Exegin Technologies Limited
 *
 * $Id: packet-zbee-security.h 34647 2010-10-26 05:42:24Z etxrab $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef PACKET_ZBEE_SECURITY_H
#define PACKET_ZBEE_SECURITY_H

/*  Structure containing the fields stored in the Aux Header */
typedef struct{
    /*  The fields of the Aux Header */
    guint8      control; /* needed to decrypt */
    guint32     counter; /* needed to decrypt */
    guint64     src64;   /* needed to decrypt */
    guint8      key_seqno;

    guint8      level;
    guint8      key_id;  /* needed to decrypt */
    gboolean    nonce;
} zbee_security_packet;

/* Bit masks for the Security Control Field. */
#define ZBEE_SEC_CONTROL_LEVEL  0x07
#define ZBEE_SEC_CONTROL_KEY    0x18
#define ZBEE_SEC_CONTROL_NONCE  0x20

/* ZigBee security levels. */
#define ZBEE_SEC_NONE           0x00
#define ZBEE_SEC_MIC32          0x01
#define ZBEE_SEC_MIC64          0x02
#define ZBEE_SEC_MIC128         0x03
#define ZBEE_SEC_ENC            0x04
#define ZBEE_SEC_ENC_MIC32      0x05
#define ZBEE_SEC_ENC_MIC64      0x06
#define ZBEE_SEC_ENC_MIC128     0x07

/* ZigBee Key Types */
#define ZBEE_SEC_KEY_LINK       0x00
#define ZBEE_SEC_KEY_NWK        0x01
#define ZBEE_SEC_KEY_TRANSPORT  0x02
#define ZBEE_SEC_KEY_LOAD       0x03

/* ZigBee Security Constants. */
#define ZBEE_SEC_CONST_L            2
#define ZBEE_SEC_CONST_NONCE_LEN    (ZBEE_SEC_CONST_BLOCKSIZE-ZBEE_SEC_CONST_L-1)
#define ZBEE_SEC_CONST_BLOCKSIZE    16

/* CCM* Flags */
#define ZBEE_SEC_CCM_FLAG_L             0x01    /* 3-bit encoding of (L-1). */
#define ZBEE_SEC_CCM_FLAG_M(m)          ((((m-2)/2) & 0x7)<<3)  /* 3-bit encoding of (M-2)/2 shifted 3 bits. */
#define ZBEE_SEC_CCM_FLAG_ADATA(l_a)    ((l_a>0)?0x40:0x00)     /* Adata flag. */

/* Program Constants */
#define ZBEE_SEC_PC_KEY             0

/* Init routine for the Security dissectors. */
extern void     zbee_security_register  (module_t *module, int proto);
extern void     zbee_security_handoff   (void);

/* Security Dissector Routine. */
extern tvbuff_t *dissect_zbee_secure(tvbuff_t *, packet_info *, proto_tree *, guint);

#endif /* PACKET_ZBEE_SECURITY_H */
