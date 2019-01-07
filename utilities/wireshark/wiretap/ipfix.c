/* ipfix.c
 *
 * Wiretap Library
 * Copyright (c) 1998 by Gilbert Ramirez <gram@alumni.rice.edu>
 *
 * File format support for ipfix file format
 * Copyright (c) 2010 by Hadriel Kaplan <hadrielk@yahoo.com>
 *   with generous copying from other wiretaps, such as pcapng
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

/* File format reference:
 *   RFC 5655 and 5101
 *   http://tools.ietf.org/rfc/rfc5655
 *   http://tools.ietf.org/rfc/rfc5101
 *
 * This wiretap is for an ipfix file format reader, per RFC 5655/5101.
 * All "records" in the file are IPFIX messages, beginning with an IPFIX
 *  message header of 16 bytes as follows from RFC 5101:
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |       Version Number          |            Length             |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           Export Time                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                       Sequence Number                         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                    Observation Domain ID                      |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

   Figure F: IPFIX Message Header Format

 * which is then followed by one or more "Sets": Data Sets, Template Sets,
 * and Options Template Sets.  Each Set then has one or more Records in
 * it.
 *
 * All IPFIX files are recorded in big-endian form (network byte order),
 * per the RFCs.  That means if we're on a little-endian system, all
 * hell will break loose if we don't g_ntohX.
 *
 * Since wireshark already has an IPFIX dissector (implemented in
 * packet-netflow.c), this reader will just set that dissector upon
 * reading each message.  Thus, an IPFIX Message is treated as a packet
 * as far as the dissector is concerned.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "wtap-int.h"
#include "file_wrappers.h"
#include "ipfix.h"

#if 0
#define ipfix_debug(...) g_warning(__VA_ARGS__)
#else
#define ipfix_debug(...)
#endif

#define RECORDS_FOR_IPFIX_CHECK 20

static gboolean
ipfix_read(wtap *wth, int *err, gchar **err_info,
    gint64 *data_offset);
static gboolean
ipfix_seek_read(wtap *wth, gint64 seek_off,
    struct wtap_pkthdr *phdr, Buffer *buf, int *err, gchar **err_info);

#define IPFIX_VERSION 10

/* ipfix: message header */
typedef struct ipfix_message_header_s {
    guint16 version;
    guint16 message_length;
    guint32 export_time_secs;
    guint32 sequence_number;
    guint32 observation_id; /* might be 0 for none */
    /* x bytes msg_body */
} ipfix_message_header_t;
#define IPFIX_MSG_HDR_SIZE 16

/* ipfix: common Set header for every Set type */
typedef struct ipfix_set_header_s {
    guint16 set_type;
    guint16 set_length;
    /* x bytes set_body */
} ipfix_set_header_t;
#define IPFIX_SET_HDR_SIZE 4


/* Read IPFIX message header from file.  Return true on success.  Set *err to
 * 0 on EOF, any other value for "real" errors (EOF is ok, since return
 * value is still FALSE)
 */
static gboolean
ipfix_read_message_header(ipfix_message_header_t *pfx_hdr, FILE_T fh, int *err, gchar **err_info)
{
    if (!wtap_read_bytes_or_eof(fh, pfx_hdr, IPFIX_MSG_HDR_SIZE, err, err_info))
        return FALSE;

    /* fix endianess, because IPFIX files are always big-endian */
    pfx_hdr->version = g_ntohs(pfx_hdr->version);
    pfx_hdr->message_length = g_ntohs(pfx_hdr->message_length);
    pfx_hdr->export_time_secs = g_ntohl(pfx_hdr->export_time_secs);
    pfx_hdr->sequence_number = g_ntohl(pfx_hdr->sequence_number);
    pfx_hdr->observation_id = g_ntohl(pfx_hdr->observation_id);

    /* is the version number one we expect? */
    if (pfx_hdr->version != IPFIX_VERSION) {
        /* Not an ipfix file. */
        *err = WTAP_ERR_BAD_FILE;
        *err_info = g_strdup_printf("ipfix: wrong version %d", pfx_hdr->version);
        return FALSE;
    }

    if (pfx_hdr->message_length < 16) {
        *err = WTAP_ERR_BAD_FILE;
        *err_info = g_strdup_printf("ipfix: message length %u is too short", pfx_hdr->message_length);
        return FALSE;
    }

    /* go back to before header */
    if (file_seek(fh, 0 - IPFIX_MSG_HDR_SIZE, SEEK_CUR, err) == -1) {
        ipfix_debug("ipfix_read: couldn't go back in file before header");
        return FALSE;
    }

    return TRUE;
}


/* Read IPFIX message header from file and fill in the struct wtap_pkthdr
 * for the packet, and, if that succeeds, read the packet data.
 * Return true on success.  Set *err to 0 on EOF, any other value for "real"
 * errors (EOF is ok, since return value is still FALSE).
 */
static gboolean
ipfix_read_message(FILE_T fh, struct wtap_pkthdr *phdr, Buffer *buf, int *err, gchar **err_info)
{
    ipfix_message_header_t msg_hdr;

    if (!ipfix_read_message_header(&msg_hdr, fh, err, err_info))
        return FALSE;
    /*
     * The maximum value of msg_hdr.message_length is 65535, which is
     * less than WTAP_MAX_PACKET_SIZE will ever be, so we don't need
     * to check it.
     */

    phdr->rec_type = REC_TYPE_PACKET;
    phdr->presence_flags = WTAP_HAS_TS;
    phdr->len = msg_hdr.message_length;
    phdr->caplen = msg_hdr.message_length;
    phdr->ts.secs = msg_hdr.export_time_secs;
    phdr->ts.nsecs = 0;

    return wtap_read_packet_bytes(fh, buf, msg_hdr.message_length, err, err_info);
}



/* classic wtap: open capture file.  Return WTAP_OPEN_MINE on success,
 * WTAP_OPEN_NOT_MINE on normal failure like malformed format,
 * WTAP_OPEN_ERROR on bad error like file system
 */
wtap_open_return_val
ipfix_open(wtap *wth, int *err, gchar **err_info)
{
    gint i, n, records_for_ipfix_check = RECORDS_FOR_IPFIX_CHECK;
    gchar *s;
    guint16 checked_len = 0;
    ipfix_message_header_t msg_hdr;
    ipfix_set_header_t set_hdr;

    ipfix_debug("ipfix_open: opening file");

    /* number of records to scan before deciding if this really is IPFIX */
    if ((s = getenv("IPFIX_RECORDS_TO_CHECK")) != NULL) {
        if ((n = atoi(s)) > 0 && n < 101) {
            records_for_ipfix_check = n;
        }
    }

    /*
     * IPFIX is a little hard because there's no magic number; we look at
     * the first few records and see if they look enough like IPFIX
     * records.
     */
    for (i = 0; i < records_for_ipfix_check; i++) {
        /* read first message header to check version */
        if (!ipfix_read_message_header(&msg_hdr, wth->fh, err, err_info)) {
            ipfix_debug("ipfix_open: couldn't read message header #%d with err code #%d (%s)",
                         i, *err, *err_info);
            if (*err == WTAP_ERR_BAD_FILE) {
                *err = 0;            /* not actually an error in this case */
                g_free(*err_info);
                *err_info = NULL;
                return WTAP_OPEN_NOT_MINE;
            }
            if (*err != 0 && *err != WTAP_ERR_SHORT_READ)
                return WTAP_OPEN_ERROR; /* real failure */
            /* else it's EOF */
            if (i < 1) {
                /* we haven't seen enough to prove this is a ipfix file */
                return WTAP_OPEN_NOT_MINE;
            }
            /*
             * If we got here, it's EOF and we haven't yet seen anything
             * that doesn't look like an IPFIX record - i.e. everything
             * we've seen looks like an IPFIX record - so we assume this
             * is an IPFIX file.
             */
            break;
        }
        if (file_seek(wth->fh, IPFIX_MSG_HDR_SIZE, SEEK_CUR, err) == -1) {
            ipfix_debug("ipfix_open: failed seek to next message in file, %d bytes away",
                         msg_hdr.message_length);
            return WTAP_OPEN_NOT_MINE;
        }
        checked_len = IPFIX_MSG_HDR_SIZE;

        /* check each Set in IPFIX Message for sanity */
        while (checked_len < msg_hdr.message_length) {
            if (!wtap_read_bytes(wth->fh, &set_hdr, IPFIX_SET_HDR_SIZE,
                                 err, err_info)) {
                if (*err == WTAP_ERR_SHORT_READ) {
                    /* Not a valid IPFIX Set, so not an IPFIX file. */
                    ipfix_debug("ipfix_open: error %d reading set", *err);
                    return WTAP_OPEN_NOT_MINE;
                }

                /* A real I/O error; fail. */
                return WTAP_OPEN_ERROR;
            }
            set_hdr.set_length = g_ntohs(set_hdr.set_length);
            if ((set_hdr.set_length < IPFIX_SET_HDR_SIZE) ||
                ((set_hdr.set_length + checked_len) > msg_hdr.message_length))  {
                ipfix_debug("ipfix_open: found invalid set_length of %d",
                             set_hdr.set_length);
                return WTAP_OPEN_NOT_MINE;
            }

            if (file_seek(wth->fh, set_hdr.set_length - IPFIX_SET_HDR_SIZE,
                 SEEK_CUR, err) == -1)
            {
                ipfix_debug("ipfix_open: failed seek to next set in file, %d bytes away",
                             set_hdr.set_length - IPFIX_SET_HDR_SIZE);
                return WTAP_OPEN_ERROR;
            }
            checked_len += set_hdr.set_length;
        }
    }

    /* go back to beginning of file */
    if (file_seek (wth->fh, 0, SEEK_SET, err) != 0)
    {
        return WTAP_OPEN_ERROR;
    }

    /* all's good, this is a IPFIX file */
    wth->file_encap = WTAP_ENCAP_RAW_IPFIX;
    wth->snapshot_length = 0;
    wth->file_tsprec = WTAP_TSPREC_SEC;
    wth->subtype_read = ipfix_read;
    wth->subtype_seek_read = ipfix_seek_read;
    wth->file_type_subtype = WTAP_FILE_TYPE_SUBTYPE_IPFIX;

    return WTAP_OPEN_MINE;
}


/* classic wtap: read packet */
static gboolean
ipfix_read(wtap *wth, int *err, gchar **err_info, gint64 *data_offset)
{
    *data_offset = file_tell(wth->fh);
    ipfix_debug("ipfix_read: data_offset is initially %" G_GINT64_MODIFIER "d", *data_offset);

    if (!ipfix_read_message(wth->fh, &wth->phdr, wth->frame_buffer, err, err_info)) {
        ipfix_debug("ipfix_read: couldn't read message header with code: %d\n, and error '%s'",
                     *err, *err_info);
        return FALSE;
    }

    return TRUE;
}


/* classic wtap: seek to file position and read packet */
static gboolean
ipfix_seek_read(wtap *wth, gint64 seek_off, struct wtap_pkthdr *phdr,
    Buffer *buf, int *err, gchar **err_info)
{
    /* seek to the right file position */
    if (file_seek(wth->random_fh, seek_off, SEEK_SET, err) == -1) {
        ipfix_debug("ipfix_seek_read: couldn't read message header with code: %d\n, and error '%s'",
                     *err, *err_info);
        return FALSE;   /* Seek error */
    }

    ipfix_debug("ipfix_seek_read: reading at offset %" G_GINT64_MODIFIER "u", seek_off);

    if (!ipfix_read_message(wth->random_fh, phdr, buf, err, err_info)) {
        ipfix_debug("ipfix_seek_read: couldn't read message header");
        if (*err == 0)
            *err = WTAP_ERR_SHORT_READ;
        return FALSE;
    }
    return TRUE;
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
