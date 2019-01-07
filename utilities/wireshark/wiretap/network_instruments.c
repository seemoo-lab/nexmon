/***************************************************************************
                          network_instruments.c  -  description
                             -------------------
    begin                : Wed Oct 29 2003
    copyright            : (C) 2003 by root
    email                : scotte[AT}netinst.com
 ***************************************************************************/

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "wtap-int.h"
#include "file_wrappers.h"
#include "network_instruments.h"

static const char network_instruments_magic[] = {"ObserverPktBufferVersion=15.00"};
static const int true_magic_length = 17;

static const guint32 observer_packet_magic = 0x88888888;

/*
 * This structure is used to keep state when writing files. An instance is
 * allocated for each file, and its address is stored in the wtap_dumper.priv
 * pointer field.
 */
typedef struct {
    guint64 packet_count;
    guint8  network_type;
    guint32 time_format;
} observer_dump_private_state;

/*
 * Some time offsets are calculated in advance here, when the first Observer
 * file is opened for reading or writing, and are then used to adjust frame
 * timestamps as they are read or written.
 *
 * The Wiretap API expects timestamps in nanoseconds relative to
 * January 1, 1970, 00:00:00 GMT (the Wiretap epoch).
 *
 * Observer versions before 13.10 encode frame timestamps in nanoseconds
 * relative to January 1, 2000, 00:00:00 local time (the Observer epoch).
 * Versions 13.10 and later switch over to GMT encoding. Which encoding was used
 * when saving the file is identified via the time format TLV following
 * the file header.
 *
 * Unfortunately, even though Observer versions before 13.10 saved in local
 * time, they didn't include the timezone from which the frames were captured,
 * so converting to GMT correctly from all timezones is impossible. So an
 * assumption is made that the file is being read from within the same timezone
 * that it was written.
 *
 * All code herein is normalized to versions 13.10 and later, special casing for
 * versions earlier. In other words, timestamps are worked with as if
 * they are GMT-encoded, and adjustments from local time are made only if
 * the source file warrants it.
 *
 * All destination files are saved in GMT format.
 */
static const time_t ansi_to_observer_epoch_offset = 946684800;
static time_t gmt_to_localtime_offset = (time_t) -1;

static void init_gmt_to_localtime_offset(void)
{
    if (gmt_to_localtime_offset == (time_t) -1) {
        time_t ansi_epoch_plus_one_day = 86400;
        struct tm gmt_tm;
        struct tm local_tm;

        /*
         * Compute the local time zone offset as the number of seconds west
         * of GMT. There's no obvious cross-platform API for querying this
         * directly. As a workaround, GMT and local tm structures are populated
         * relative to the ANSI time_t epoch (plus one day to ensure that
         * local time stays after 1970/1/1 00:00:00). They are then converted
         * back to time_t as if they were both local times, resulting in the
         * time zone offset being the difference between them.
         */
        gmt_tm = *gmtime(&ansi_epoch_plus_one_day);
        local_tm = *localtime(&ansi_epoch_plus_one_day);
        local_tm.tm_isdst = 0;
        gmt_to_localtime_offset = mktime(&gmt_tm) - mktime(&local_tm);
    }
}

static gboolean observer_read(wtap *wth, int *err, gchar **err_info,
    gint64 *data_offset);
static gboolean observer_seek_read(wtap *wth, gint64 seek_off,
    struct wtap_pkthdr *phdr, Buffer *buf, int *err, gchar **err_info);
static int read_packet_header(wtap *wth, FILE_T fh, union wtap_pseudo_header *pseudo_header,
    packet_entry_header *packet_header, int *err, gchar **err_info);
static gboolean process_packet_header(wtap *wth,
    packet_entry_header *packet_header, struct wtap_pkthdr *phdr, int *err,
    gchar **err_info);
static int read_packet_data(FILE_T fh, int offset_to_frame, int current_offset_from_packet_header,
    Buffer *buf, int length, int *err, char **err_info);
static gboolean skip_to_next_packet(wtap *wth, int offset_to_next_packet,
    int current_offset_from_packet_header, int *err, char **err_info);
static gboolean observer_dump(wtap_dumper *wdh, const struct wtap_pkthdr *phdr,
    const guint8 *pd, int *err, gchar **err_info);
static gint observer_to_wtap_encap(int observer_encap);
static gint wtap_to_observer_encap(int wtap_encap);

wtap_open_return_val network_instruments_open(wtap *wth, int *err, gchar **err_info)
{
    int offset;
    capture_file_header file_header;
    guint i;
    tlv_header tlvh;
    int seek_increment;
    int header_offset;
    packet_entry_header packet_header;
    observer_dump_private_state * private_state = NULL;

    offset = 0;

    /* read in the buffer file header */
    if (!wtap_read_bytes(wth->fh, &file_header, sizeof file_header,
                         err, err_info)) {
        if (*err != WTAP_ERR_SHORT_READ)
            return WTAP_OPEN_ERROR;
        return WTAP_OPEN_NOT_MINE;
    }
    offset += (int)sizeof file_header;
    CAPTURE_FILE_HEADER_FROM_LE_IN_PLACE(file_header);

    /* check if version info is present */
    if (memcmp(file_header.observer_version, network_instruments_magic, true_magic_length)!=0) {
        return WTAP_OPEN_NOT_MINE;
    }

    /* initialize the private state */
    private_state = (observer_dump_private_state *) g_malloc(sizeof(observer_dump_private_state));
    private_state->time_format = TIME_INFO_LOCAL;
    wth->priv = (void *) private_state;

    /* get the location of the first packet */
    /* v15 and newer uses high byte offset, in previous versions it will be 0 */
    header_offset = file_header.offset_to_first_packet + ((int)(file_header.offset_to_first_packet_high_byte)<<16);

    /* process extra information */
    for (i = 0; i < file_header.number_of_information_elements; i++) {
        /* for safety break if we've reached the first packet */
        if (offset >= header_offset)
            break;

        /* read the TLV header */
        if (!wtap_read_bytes(wth->fh, &tlvh, sizeof tlvh, err, err_info))
            return WTAP_OPEN_ERROR;
        offset += (int)sizeof tlvh;
        TLV_HEADER_FROM_LE_IN_PLACE(tlvh);

        if (tlvh.length < sizeof tlvh) {
            *err = WTAP_ERR_BAD_FILE;
            *err_info = g_strdup_printf("Observer: bad record (TLV length %u < %lu)",
                tlvh.length, (unsigned long)sizeof tlvh);
            return WTAP_OPEN_ERROR;
        }

        /* process (or skip over) the current TLV */
        switch (tlvh.type) {
        case INFORMATION_TYPE_TIME_INFO:
            if (!wtap_read_bytes(wth->fh, &private_state->time_format,
                                 sizeof private_state->time_format,
                                 err, err_info))
                return WTAP_OPEN_ERROR;
            private_state->time_format = GUINT32_FROM_LE(private_state->time_format);
            offset += (int)sizeof private_state->time_format;
            break;
        default:
            seek_increment = tlvh.length - (int)sizeof tlvh;
            if (seek_increment > 0) {
                if (file_seek(wth->fh, seek_increment, SEEK_CUR, err) == -1)
                    return WTAP_OPEN_ERROR;
            }
            offset += seek_increment;
        }
    }

    /* get to the first packet */
    if (header_offset < offset) {
        *err = WTAP_ERR_BAD_FILE;
        *err_info = g_strdup_printf("Observer: bad record (offset to first packet %d < %d)",
            header_offset, offset);
        return WTAP_OPEN_ERROR;
    }
    seek_increment = header_offset - offset;
    if (seek_increment > 0) {
        if (file_seek(wth->fh, seek_increment, SEEK_CUR, err) == -1)
            return WTAP_OPEN_ERROR;
    }

    /* pull off the packet header */
    if (!wtap_read_bytes(wth->fh, &packet_header, sizeof packet_header,
                         err, err_info))
        return WTAP_OPEN_ERROR;
    PACKET_ENTRY_HEADER_FROM_LE_IN_PLACE(packet_header);

    /* check the packet's magic number */
    if (packet_header.packet_magic != observer_packet_magic) {
        *err = WTAP_ERR_UNSUPPORTED;
        *err_info = g_strdup_printf("Observer: unsupported packet version %ul", packet_header.packet_magic);
        return WTAP_OPEN_ERROR;
    }

    /* check the data link type */
    if (observer_to_wtap_encap(packet_header.network_type) == WTAP_ENCAP_UNKNOWN) {
        *err = WTAP_ERR_UNSUPPORTED;
        *err_info = g_strdup_printf("Observer: network type %u unknown or unsupported", packet_header.network_type);
        return WTAP_OPEN_ERROR;
    }
    wth->file_encap = observer_to_wtap_encap(packet_header.network_type);

    /* set up the rest of the capture parameters */
    private_state->packet_count = 0;
    private_state->network_type = wtap_to_observer_encap(wth->file_encap);
    wth->subtype_read = observer_read;
    wth->subtype_seek_read = observer_seek_read;
    wth->subtype_close = NULL;
    wth->subtype_sequential_close = NULL;
    wth->snapshot_length = 0;    /* not available in header */
    wth->file_tsprec = WTAP_TSPREC_NSEC;
    wth->file_type_subtype = WTAP_FILE_TYPE_SUBTYPE_NETWORK_INSTRUMENTS;

    /* reset the pointer to the first packet */
    if (file_seek(wth->fh, header_offset, SEEK_SET, err) == -1)
        return WTAP_OPEN_ERROR;

    init_gmt_to_localtime_offset();

    return WTAP_OPEN_MINE;
}

/* Reads the next packet. */
static gboolean observer_read(wtap *wth, int *err, gchar **err_info,
    gint64 *data_offset)
{
    int header_bytes_consumed;
    int data_bytes_consumed;
    packet_entry_header packet_header;

    /* skip records other than data records */
    for (;;) {
        *data_offset = file_tell(wth->fh);

        /* process the packet header, including TLVs */
        header_bytes_consumed = read_packet_header(wth, wth->fh, &wth->phdr.pseudo_header, &packet_header, err,
            err_info);
        if (header_bytes_consumed <= 0)
            return FALSE;    /* EOF or error */

        if (packet_header.packet_type == PACKET_TYPE_DATA_PACKET)
            break;

        /* skip to next packet */
        if (!skip_to_next_packet(wth, packet_header.offset_to_next_packet,
                header_bytes_consumed, err, err_info)) {
            return FALSE;    /* EOF or error */
        }
    }

    if (!process_packet_header(wth, &packet_header, &wth->phdr, err, err_info))
        return FALSE;

    /* read the frame data */
    data_bytes_consumed = read_packet_data(wth->fh, packet_header.offset_to_frame,
            header_bytes_consumed, wth->frame_buffer,
            wth->phdr.caplen, err, err_info);
    if (data_bytes_consumed < 0) {
        return FALSE;
    }

    /* skip over any extra bytes following the frame data */
    if (!skip_to_next_packet(wth, packet_header.offset_to_next_packet,
            header_bytes_consumed + data_bytes_consumed, err, err_info)) {
        return FALSE;
    }

    return TRUE;
}

/* Reads a packet at an offset. */
static gboolean observer_seek_read(wtap *wth, gint64 seek_off,
    struct wtap_pkthdr *phdr, Buffer *buf, int *err, gchar **err_info)
{
    union wtap_pseudo_header *pseudo_header = &phdr->pseudo_header;
    packet_entry_header packet_header;
    int offset;
    int data_bytes_consumed;

    if (file_seek(wth->random_fh, seek_off, SEEK_SET, err) == -1)
        return FALSE;

    /* process the packet header, including TLVs */
    offset = read_packet_header(wth, wth->random_fh, pseudo_header, &packet_header, err,
        err_info);
    if (offset <= 0)
        return FALSE;    /* EOF or error */

    if (!process_packet_header(wth, &packet_header, phdr, err, err_info))
        return FALSE;

    /* read the frame data */
    data_bytes_consumed = read_packet_data(wth->random_fh, packet_header.offset_to_frame,
        offset, buf, phdr->caplen, err, err_info);
    if (data_bytes_consumed < 0) {
        return FALSE;
    }

    return TRUE;
}

static int
read_packet_header(wtap *wth, FILE_T fh, union wtap_pseudo_header *pseudo_header,
    packet_entry_header *packet_header, int *err, gchar **err_info)
{
    int offset;
    guint i;
    tlv_header tlvh;
    int seek_increment;
    tlv_wireless_info wireless_header;

    offset = 0;

    /* pull off the packet header */
    if (!wtap_read_bytes_or_eof(fh, packet_header, sizeof *packet_header,
                                err, err_info)) {
        if (*err != 0)
            return -1;
        return 0;    /* EOF */
    }
    offset += (int)sizeof *packet_header;
    PACKET_ENTRY_HEADER_FROM_LE_IN_PLACE(*packet_header);

    /* check the packet's magic number */
    if (packet_header->packet_magic != observer_packet_magic) {

        /*
         * Some files are zero-padded at the end. There is no warning of this
         * in the previous packet header information, such as setting
         * offset_to_next_packet to zero. So detect this situation by treating
         * an all-zero header as a sentinel. Return EOF when it is encountered,
         * rather than treat it as a bad record.
         */
        for (i = 0; i < sizeof *packet_header; i++) {
            if (((guint8*) packet_header)[i] != 0)
                break;
        }
        if (i == sizeof *packet_header) {
            *err = 0;
            return 0;    /* EOF */
        }

        *err = WTAP_ERR_BAD_FILE;
        *err_info = g_strdup_printf("Observer: bad record: Invalid magic number 0x%08x",
            packet_header->packet_magic);
        return -1;
    }

    /* initialize the pseudo header */
    switch (wth->file_encap) {
    case WTAP_ENCAP_ETHERNET:
        /* There is no FCS in the frame */
        pseudo_header->eth.fcs_len = 0;
        break;
    case WTAP_ENCAP_IEEE_802_11_WITH_RADIO:
        memset(&pseudo_header->ieee_802_11, 0, sizeof(pseudo_header->ieee_802_11));
        pseudo_header->ieee_802_11.fcs_len = 0;
        pseudo_header->ieee_802_11.decrypted = FALSE;
        pseudo_header->ieee_802_11.datapad = FALSE;
        pseudo_header->ieee_802_11.phy = PHDR_802_11_PHY_UNKNOWN;
        /* Updated below */
        break;
    }

    /* process extra information */
    for (i = 0; i < packet_header->number_of_information_elements; i++) {
        /* read the TLV header */
        if (!wtap_read_bytes(fh, &tlvh, sizeof tlvh, err, err_info))
            return -1;
        offset += (int)sizeof tlvh;
        TLV_HEADER_FROM_LE_IN_PLACE(tlvh);

        if (tlvh.length < sizeof tlvh) {
            *err = WTAP_ERR_BAD_FILE;
            *err_info = g_strdup_printf("Observer: bad record (TLV length %u < %lu)",
                tlvh.length, (unsigned long)sizeof tlvh);
            return -1;
        }

        /* process (or skip over) the current TLV */
        switch (tlvh.type) {
        case INFORMATION_TYPE_WIRELESS:
            if (!wtap_read_bytes(fh, &wireless_header, sizeof wireless_header,
                                 err, err_info))
                return -1;
            /* set decryption status */
            /* XXX - what other bits are there in conditions? */
            pseudo_header->ieee_802_11.decrypted = (wireless_header.conditions & WIRELESS_WEP_SUCCESS) != 0;
            pseudo_header->ieee_802_11.has_channel = TRUE;
            pseudo_header->ieee_802_11.channel = wireless_header.frequency;
            pseudo_header->ieee_802_11.has_data_rate = TRUE;
            pseudo_header->ieee_802_11.data_rate = wireless_header.rate;
            pseudo_header->ieee_802_11.has_signal_percent = TRUE;
            pseudo_header->ieee_802_11.signal_percent = wireless_header.strengthPercent;
            offset += (int)sizeof wireless_header;
            break;
        default:
            /* skip the TLV data */
            seek_increment = tlvh.length - (int)sizeof tlvh;
            if (seek_increment > 0) {
                if (file_seek(fh, seek_increment, SEEK_CUR, err) == -1)
                    return -1;
            }
            offset += seek_increment;
        }
    }

    return offset;
}

static gboolean
process_packet_header(wtap *wth, packet_entry_header *packet_header,
    struct wtap_pkthdr *phdr, int *err, gchar **err_info)
{
    /* set the wiretap packet header fields */
    phdr->rec_type = REC_TYPE_PACKET;
    phdr->presence_flags = WTAP_HAS_TS|WTAP_HAS_CAP_LEN;
    phdr->pkt_encap = observer_to_wtap_encap(packet_header->network_type);
    if(wth->file_encap == WTAP_ENCAP_FIBRE_CHANNEL_FC2_WITH_FRAME_DELIMS) {
        phdr->len = packet_header->network_size;
        phdr->caplen = packet_header->captured_size;
    } else {
        /*
         * XXX - what are those 4 bytes?
         *
         * The comment in the code said "neglect frame markers for wiretap",
         * but in the captures I've seen, there's no actual data corresponding
         * to them that might be a "frame marker".
         *
         * Instead, the packets had a network_size 4 bytes larger than the
         * captured_size; does the network_size include the CRC, even
         * though it's not included in a capture?  If so, most other
         * network analyzers that have a "network size" and a "captured
         * size" don't include the CRC in the "network size" if they
         * don't include the CRC in a full-length captured packet; the
         * "captured size" is less than the "network size" only if a
         * user-specified "snapshot length" caused the packet to be
         * sliced at a particular point.
         *
         * That's the model that wiretap and Wireshark/TShark use, so
         * we implement that model here.
         */
        if (packet_header->network_size < 4) {
            *err = WTAP_ERR_BAD_FILE;
            *err_info = g_strdup_printf("Observer: bad record: Packet length %u < 4",
                                        packet_header->network_size);
            return FALSE;
        }

        phdr->len = packet_header->network_size - 4;
        phdr->caplen = MIN(packet_header->captured_size, phdr->len);
    }
    /*
     * The maximum value of packet_header->captured_size is 65535, which
     * is less than WTAP_MAX_PACKET_SIZE will ever be, so we don't need
     * to check it.
     */

    /* set the wiretap timestamp, assuming for the moment that Observer encoded it in GMT */
    phdr->ts.secs = (time_t) ((packet_header->nano_seconds_since_2000 / 1000000000) + ansi_to_observer_epoch_offset);
    phdr->ts.nsecs = (int) (packet_header->nano_seconds_since_2000 % 1000000000);

    /* adjust to local time, if necessary, also accounting for DST if the frame
       was captured while it was in effect */
    if (((observer_dump_private_state*)wth->priv)->time_format == TIME_INFO_LOCAL)
    {
        struct tm daylight_tm;
        struct tm standard_tm;
        time_t    dst_offset;

        /* the Observer timestamp was encoded as local time, so add a
           correction from local time to GMT */
        phdr->ts.secs += gmt_to_localtime_offset;

        /* perform a DST adjustment if necessary */
        standard_tm = *localtime(&phdr->ts.secs);
        if (standard_tm.tm_isdst > 0) {
            daylight_tm = standard_tm;
            standard_tm.tm_isdst = 0;
            dst_offset = mktime(&standard_tm) - mktime(&daylight_tm);
            phdr->ts.secs -= dst_offset;
        }
    }

    return TRUE;
}

static int
read_packet_data(FILE_T fh, int offset_to_frame, int current_offset_from_packet_header, Buffer *buf,
    int length, int *err, char **err_info)
{
    int seek_increment;
    int bytes_consumed = 0;

    /* validate offsets */
    if (offset_to_frame < current_offset_from_packet_header) {
        *err = WTAP_ERR_BAD_FILE;
        *err_info = g_strdup_printf("Observer: bad record (offset to packet data %d < %d)",
            offset_to_frame, current_offset_from_packet_header);
        return -1;
    }

    /* skip to the packet data */
    seek_increment = offset_to_frame - current_offset_from_packet_header;
    if (seek_increment > 0) {
        if (file_seek(fh, seek_increment, SEEK_CUR, err) == -1) {
            return -1;
        }
        bytes_consumed += seek_increment;
    }

    /* set-up the packet buffer */
    ws_buffer_assure_space(buf, length);

    /* read in the packet data */
    if (!wtap_read_packet_bytes(fh, buf, length, err, err_info))
        return FALSE;
    bytes_consumed += length;

    return bytes_consumed;
}

static gboolean
skip_to_next_packet(wtap *wth, int offset_to_next_packet, int current_offset_from_packet_header, int *err,
    char **err_info)
{
    int seek_increment;

    /* validate offsets */
    if (offset_to_next_packet < current_offset_from_packet_header) {
        *err = WTAP_ERR_BAD_FILE;
        *err_info = g_strdup_printf("Observer: bad record (offset to next packet %d < %d)",
            offset_to_next_packet, current_offset_from_packet_header);
        return FALSE;
    }

    /* skip to the next packet header */
    seek_increment = offset_to_next_packet - current_offset_from_packet_header;
    if (seek_increment > 0) {
        if (file_seek(wth->fh, seek_increment, SEEK_CUR, err) == -1)
            return FALSE;
    }

    return TRUE;
}

/* Returns 0 if we could write the specified encapsulation type,
   an error indication otherwise. */
int network_instruments_dump_can_write_encap(int encap)
{
    /* per-packet encapsulations aren't supported */
    if (encap == WTAP_ENCAP_PER_PACKET)
        return WTAP_ERR_ENCAP_PER_PACKET_UNSUPPORTED;

    if (encap < 0 || (wtap_to_observer_encap(encap) == OBSERVER_UNDEFINED))
        return WTAP_ERR_UNWRITABLE_ENCAP;

    return 0;
}

/* Returns TRUE on success, FALSE on failure; sets "*err" to an error code on
   failure. */
gboolean network_instruments_dump_open(wtap_dumper *wdh, int *err)
{
    observer_dump_private_state * private_state = NULL;
    capture_file_header file_header;

    tlv_header comment_header;
    tlv_time_info time_header;
    char comment[64];
    size_t comment_length;
    struct tm * current_time;
    time_t system_time;

    /* initialize the private state */
    private_state = (observer_dump_private_state *) g_malloc(sizeof(observer_dump_private_state));
    private_state->packet_count = 0;
    private_state->network_type = wtap_to_observer_encap(wdh->encap);
    private_state->time_format = TIME_INFO_GMT;

    /* populate the fields of wdh */
    wdh->priv = (void *) private_state;
    wdh->subtype_write = observer_dump;

    /* initialize the file header */
    memset(&file_header, 0x00, sizeof(file_header));
    g_strlcpy(file_header.observer_version, network_instruments_magic, 31);
    file_header.offset_to_first_packet = (guint16)sizeof(file_header);
    file_header.offset_to_first_packet_high_byte = 0;

    /* create the file comment TLV */
    {
        time(&system_time);
        /* We trusst the OS not to return a time before the Epoch */
        current_time = localtime(&system_time);
        memset(&comment, 0x00, sizeof(comment));
        g_snprintf(comment, 64, "This capture was saved from Wireshark on %s", asctime(current_time));
        comment_length = strlen(comment);

        comment_header.type = INFORMATION_TYPE_COMMENT;
        comment_header.length = (guint16) (sizeof(comment_header) + comment_length);

        /* update the file header to account for the comment TLV */
        file_header.number_of_information_elements++;
        file_header.offset_to_first_packet += comment_header.length;
    }

    /* create the timestamp encoding TLV */
    {
        time_header.type = INFORMATION_TYPE_TIME_INFO;
        time_header.length = (guint16) (sizeof(time_header));
        time_header.time_format = TIME_INFO_GMT;

        /* update the file header to account for the timestamp encoding TLV */
        file_header.number_of_information_elements++;
        file_header.offset_to_first_packet += time_header.length;
    }

    /* write the file header, swapping any multibyte fields first */
    CAPTURE_FILE_HEADER_TO_LE_IN_PLACE(file_header);
    if (!wtap_dump_file_write(wdh, &file_header, sizeof(file_header), err)) {
        return FALSE;
    }
    wdh->bytes_dumped += sizeof(file_header);

    /* write the comment TLV */
    {
        TLV_HEADER_TO_LE_IN_PLACE(comment_header);
        if (!wtap_dump_file_write(wdh, &comment_header, sizeof(comment_header), err)) {
            return FALSE;
        }
        wdh->bytes_dumped += sizeof(comment_header);

        if (!wtap_dump_file_write(wdh, &comment, comment_length, err)) {
            return FALSE;
        }
        wdh->bytes_dumped += comment_length;
    }

    /* write the time info TLV */
    {
        TLV_TIME_INFO_TO_LE_IN_PLACE(time_header);
        if (!wtap_dump_file_write(wdh, &time_header, sizeof(time_header), err)) {
            return FALSE;
        }
        wdh->bytes_dumped += sizeof(time_header);
    }

    init_gmt_to_localtime_offset();

    return TRUE;
}

/* Write a record for a packet to a dump file.
   Returns TRUE on success, FALSE on failure. */
static gboolean observer_dump(wtap_dumper *wdh, const struct wtap_pkthdr *phdr,
    const guint8 *pd,
    int *err, gchar **err_info _U_)
{
    observer_dump_private_state * private_state = NULL;
    packet_entry_header           packet_header;
    guint64                       seconds_since_2000;

    /* We can only write packet records. */
    if (phdr->rec_type != REC_TYPE_PACKET) {
        *err = WTAP_ERR_UNWRITABLE_REC_TYPE;
        return FALSE;
    }

    /* The captured size field is 16 bits, so there's a hard limit of
       65535. */
    if (phdr->caplen > 65535) {
        *err = WTAP_ERR_PACKET_TOO_LARGE;
        return FALSE;
    }

    /* convert the number of seconds since epoch from ANSI-relative to
       Observer-relative */
    if (phdr->ts.secs < ansi_to_observer_epoch_offset) {
        if(phdr->ts.secs > (time_t) 0) {
            seconds_since_2000 = phdr->ts.secs;
        } else {
            seconds_since_2000 = (time_t) 0;
        }
    } else {
        seconds_since_2000 = phdr->ts.secs - ansi_to_observer_epoch_offset;
    }

    /* populate the fields of the packet header */
    private_state = (observer_dump_private_state *) wdh->priv;

    memset(&packet_header, 0x00, sizeof(packet_header));
    packet_header.packet_magic = observer_packet_magic;
    packet_header.network_speed = 1000000;
    packet_header.captured_size = (guint16) phdr->caplen;
    packet_header.network_size = (guint16) (phdr->len + 4);
    packet_header.offset_to_frame = sizeof(packet_header);
    /* XXX - what if this doesn't fit in 16 bits?  It's not guaranteed to... */
    packet_header.offset_to_next_packet = (guint16)sizeof(packet_header) + phdr->caplen;
    packet_header.network_type = private_state->network_type;
    packet_header.flags = 0x00;
    packet_header.number_of_information_elements = 0;
    packet_header.packet_type = PACKET_TYPE_DATA_PACKET;
    packet_header.packet_number = private_state->packet_count;
    packet_header.original_packet_number = packet_header.packet_number;
    packet_header.nano_seconds_since_2000 = seconds_since_2000 * 1000000000 + phdr->ts.nsecs;

    private_state->packet_count++;

    /* write the packet header */
    PACKET_ENTRY_HEADER_TO_LE_IN_PLACE(packet_header);
    if (!wtap_dump_file_write(wdh, &packet_header, sizeof(packet_header), err)) {
        return FALSE;
    }
    wdh->bytes_dumped += sizeof(packet_header);

    /* write the packet data */
    if (!wtap_dump_file_write(wdh, pd, phdr->caplen, err)) {
        return FALSE;
    }
    wdh->bytes_dumped += phdr->caplen;

    return TRUE;
}

static gint observer_to_wtap_encap(int observer_encap)
{
    switch(observer_encap) {
    case OBSERVER_ETHERNET:
        return WTAP_ENCAP_ETHERNET;
    case OBSERVER_TOKENRING:
        return WTAP_ENCAP_TOKEN_RING;
    case OBSERVER_FIBRE_CHANNEL:
        return WTAP_ENCAP_FIBRE_CHANNEL_FC2_WITH_FRAME_DELIMS;
    case OBSERVER_WIRELESS_802_11:
        return WTAP_ENCAP_IEEE_802_11_WITH_RADIO;
    case OBSERVER_UNDEFINED:
        return WTAP_ENCAP_UNKNOWN;
    }
    return WTAP_ENCAP_UNKNOWN;
}

static gint wtap_to_observer_encap(int wtap_encap)
{
    switch(wtap_encap) {
    case WTAP_ENCAP_ETHERNET:
        return OBSERVER_ETHERNET;
    case WTAP_ENCAP_TOKEN_RING:
        return OBSERVER_TOKENRING;
    case WTAP_ENCAP_FIBRE_CHANNEL_FC2_WITH_FRAME_DELIMS:
        return OBSERVER_FIBRE_CHANNEL;
    case WTAP_ENCAP_UNKNOWN:
        return OBSERVER_UNDEFINED;
    }
    return OBSERVER_UNDEFINED;
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
