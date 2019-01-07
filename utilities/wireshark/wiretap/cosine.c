/* cosine.c
 *
 * CoSine IPNOS L2 debug output parsing
 * Copyright (c) 2002 by Motonori Shindo <motonori@shin.do>
 *
 * Wiretap Library
 * Copyright (c) 1998 by Gilbert Ramirez <gram@alumni.rice.edu>
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

#include "config.h"
#include "wtap-int.h"
#include "cosine.h"
#include "file_wrappers.h"

#include <stdlib.h>
#include <string.h>

/*

  IPNOS: CONFIG VPN(100) VR(1.1.1.1)# diags
  ipnos diags: Control (1/0) :: layer-2 ?
  Registered commands for area "layer-2"
      apply-pkt-log-profile  Configure packet logging on an interface
      create-pkt-log-profile  Set packet-log-profile to be used for packet logging (see layer-2 pkt-log)
      detail                Get Layer 2 low-level details

  ipnos diags: Control (1/0) :: layer-2 create ?
      create-pkt-log-profile  <pkt-log-profile-id ctl-tx-trace-length ctl-rx-trace-length data-tx-trace-length data-rx-trace-length pe-logging-or-control-blade>

  ipnos diags: Control (1/0) :: layer-2 create 1 32 32 0 0 0
  ipnos diags: Control (1/0) :: layer-2 create 2 32 32 100 100 0
  ipnos diags: Control (1/0) :: layer-2 apply ?
      apply-pkt-log-profile  <slot port channel subif pkt-log-profile-id>

  ipnos diags: Control (1/0) :: layer-2 apply 3 0x0701 100 0 1
  Successfully applied packet-log-profile on LI

  -- Note that only the control packets are logged because the data packet size parameters are 0 in profile 1
  IPNOS: CONFIG VPN(200) VR(3.3.3.3)# ping 20.20.20.43
  vpn 200 : [max tries 4, timeout 5 seconds, data length 64 bytes, ttl 255]
  ping #1 ok, RTT 0.000 seconds
  ping #2 ok, RTT 0.000 seconds
  ping #3 ok, RTT 0.000 seconds
  ping #4 ok, RTT 0.000 seconds
  [finished]

  IPNOS: CONFIG VPN(200) VR(3.3.3.3)# 2000-2-1,18:19:46.8:  l2-tx (PPP:3/7/1:100), Length:16, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4000, 0x0]


  2000-2-1,18:19:46.8:  l2-rx (PPP:3/7/1:100), Length:16, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4001, 0x30000]

  2000-2-1,18:19:46.8:  l2-tx (PPP:3/7/1:100), Length:16, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4000, 0x0]

  2000-2-1,18:19:46.8:  l2-rx (PPP:3/7/1:100), Length:16, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4001, 0x8030000]

  ipnos diags: Control (1/0) :: layer-2 apply 3 0x0701 100 0 0
  Successfully applied packet-log-profile on LI
  ipnos diags: Control (1/0) :: layer-2 apply 3 0x0701 100 0 2
  Successfully applied packet-log-profile on LI

  -- Note that both control and data packets are logged because the data packet size parameter is 100 in profile 2
     Please ignore the event-log messages getting mixed up with the ping command
  ping 20.20.20.43 cou2000-2-1,18:20:17.0:  l2-tx (PPP:3/7/1:100), Length:16, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4000, 0x0]

          00 D0 D8 D2 FF 03 C0 21  09 29 00 08 6B 60 84 AA

  2000-2-1,18:20:17.0:  l2-rx (PPP:3/7/1:100), Length:16, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4001, 0x30000]
          00 D0 D8 D2 FF 03 C0 21  09 29 00 08 6D FE FA AA

  2000-2-1,18:20:17.0:  l2-tx (PPP:3/7/1:100), Length:16, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4000, 0x0]
          00 D0 D8 D2 FF 03 C0 21  0A 29 00 08 6B 60 84 AA

  2000-2-1,18:20:17.0:  l2-rx (PPP:3/7/1:100), Length:16, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4001, 0x8030000]
          00 D0 D8 D2 FF 03 C0 21  0A 29 00 08 6D FE FA AA

  nt 1 length 500
  vpn 200 : [max tries 1, timeout 5 seconds, data length 500 bytes, ttl 255]
  2000-2-1,18:20:24.1:  l2-tx (PPP:3/7/1:100), Length:536, Pro:1, Off:8, Pri:7, RM:0, Err:0 [0x4070, 0x801]
          00 D0 D8 D2 FF 03 00 21  45 00 02 10 00 27 00 00
          FF 01 69 51 14 14 14 22  14 14 14 2B 08 00 AD B8
          00 03 00 01 10 11 12 13  14 15 16 17 18 19 1A 1B
          1C 1D 1E 1F 20 21 22 23  24 25 26 27 28 29 2A 2B
          2C 2D 2E 2F 30 31 32 33  34 35 36 37 38 39 3A 3B
          3C 3D 3E 3F 40 41 42 43  44 45 46 47 48 49 4A 4B
          4C 4D 4E 4F

  ping #1 ok, RTT 0.010 seconds
  2000-2-1,18:20:24.1:  l2-rx (PPP:3/7/1:100), Length:536, Pro:1, Off:8, Pri:7, RM:0, Err:0 [0x4071, 0x30801]
          00 D0 D8 D2 FF 03 00 21  45 00 02 10 00 23 00 00
          FF 01 69 55 14 14 14 2B  14 14 14 22 00 00 B5 B8
          00 03 00 01 10 11 12 13  14 15 16 17 18 19 1A 1B
          1C 1D 1E 1F 20 21 22 23  24 25 26 27 28 29 2A 2B
          2C 2D 2E 2F 30 31 32 33  34 35 36 37 38 39 3A 3B
          3C 3D 3E 3F 40 41 42 43  44 45 46 47 48 49 4A 4B
          4C 4D 4E 4F

  [finished]

  IPNOS: CONFIG VPN(200) VR(3.3.3.3)# 2000-2-1,18:20:27.0:  l2-tx (PPP:3/7/1:100), Length:16, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4000, 0x0]

          00 D0 D8 D2 FF 03 C0 21  09 2A 00 08 6B 60 84 AA

  2000-2-1,18:20:27.0:  l2-rx (PPP:3/7/1:100), Length:16, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4001, 0x30000]
          00 D0 D8 D2 FF 03 C0 21  09 2A 00 08 6D FE FA AA

  2000-2-1,18:20:27.0:  l2-tx (PPP:3/7/1:100), Length:16, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4000, 0x0]
          00 D0 D8 D2 FF 03 C0 21  0A 2A 00 08 6B 60 84 AA

  2000-2-1,18:20:27.0:  l2-rx (PPP:3/7/1:100), Length:16, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4001, 0x30000]
          00 D0 D8 D2 FF 03 C0 21  0A 2A 00 08 6D FE FA AA


  ipnos diags: Control (1/0) :: layer-2 apply 3 0x0701 100 0 0
  Successfully applied packet-log-profile on LI
  ipnos diags: Control (1/0) ::

 */

/* XXX TODO:

  o Handle a case where an empty line doesn't exists as a delimiter of
    each packet. If the output is sent to a control blade and
    displayed as an event log, there's always an empty line between
    each packet output, but it may not be true when it is an PE
    output.

  o Some telnet client on Windows may put in a line break at 80
    columns when it save the session to a text file ("CRT" is such an
    example). I don't think it's a good idea for the telnet client to
    do so, but CRT is widely used in Windows community, I should
    take care of that in the future.

*/

/* Magic text to check for CoSine L2 debug output */
#define COSINE_HDR_MAGIC_STR1	"l2-tx"
#define COSINE_HDR_MAGIC_STR2	"l2-rx"

/* Magic text for start of packet */
#define COSINE_REC_MAGIC_STR1	COSINE_HDR_MAGIC_STR1
#define COSINE_REC_MAGIC_STR2	COSINE_HDR_MAGIC_STR2

#define COSINE_HEADER_LINES_TO_CHECK	200
#define COSINE_LINE_LENGTH		240

static gboolean empty_line(const gchar *line);
static gint64 cosine_seek_next_packet(wtap *wth, int *err, gchar **err_info,
	char *hdr);
static gboolean cosine_check_file_type(wtap *wth, int *err, gchar **err_info);
static gboolean cosine_read(wtap *wth, int *err, gchar **err_info,
	gint64 *data_offset);
static gboolean cosine_seek_read(wtap *wth, gint64 seek_off,
	struct wtap_pkthdr *phdr, Buffer *buf, int *err, gchar **err_info);
static int parse_cosine_packet(FILE_T fh, struct wtap_pkthdr *phdr, Buffer* buf,
	char *line, int *err, gchar **err_info);
static int parse_single_hex_dump_line(char* rec, guint8 *buf,
	guint byte_offset);

/* Returns TRUE if the line appears to be an empty line. Otherwise it
   returns FALSE. */
static gboolean empty_line(const gchar *line)
{
	while (*line) {
		if (g_ascii_isspace(*line)) {
			line++;
			continue;
		} else {
			break;
		}
	}
	if (*line == '\0')
		return TRUE;
	else
		return FALSE;
}

/* Seeks to the beginning of the next packet, and returns the
   byte offset. Copy the header line to hdr. Returns -1 on failure,
   and sets "*err" to the error and sets "*err_info" to null or an
   additional error string. */
static gint64 cosine_seek_next_packet(wtap *wth, int *err, gchar **err_info,
	char *hdr)
{
	gint64 cur_off;
	char buf[COSINE_LINE_LENGTH];

	while (1) {
		cur_off = file_tell(wth->fh);
		if (cur_off == -1) {
			/* Error */
			*err = file_error(wth->fh, err_info);
			return -1;
		}
		if (file_gets(buf, sizeof(buf), wth->fh) == NULL) {
			*err = file_error(wth->fh, err_info);
			return -1;
		}
		if (strstr(buf, COSINE_REC_MAGIC_STR1) ||
		    strstr(buf, COSINE_REC_MAGIC_STR2)) {
			g_strlcpy(hdr, buf, COSINE_LINE_LENGTH);
			return cur_off;
		}
	}
	return -1;
}

/* Look through the first part of a file to see if this is
 * a CoSine L2 debug output.
 *
 * Returns TRUE if it is, FALSE if it isn't or if we get an I/O error;
 * if we get an I/O error, "*err" will be set to a non-zero value and
 * "*err_info" will be set to null or an additional error string.
 */
static gboolean cosine_check_file_type(wtap *wth, int *err, gchar **err_info)
{
	char	buf[COSINE_LINE_LENGTH];
	gsize	reclen;
	guint	line;

	buf[COSINE_LINE_LENGTH-1] = '\0';

	for (line = 0; line < COSINE_HEADER_LINES_TO_CHECK; line++) {
		if (file_gets(buf, COSINE_LINE_LENGTH, wth->fh) == NULL) {
			/* EOF or error. */
			*err = file_error(wth->fh, err_info);
			return FALSE;
		}

		reclen = strlen(buf);
		if (reclen < strlen(COSINE_HDR_MAGIC_STR1) ||
			reclen < strlen(COSINE_HDR_MAGIC_STR2)) {
			continue;
		}

		if (strstr(buf, COSINE_HDR_MAGIC_STR1) ||
		    strstr(buf, COSINE_HDR_MAGIC_STR2)) {
			return TRUE;
		}
	}
	*err = 0;
	return FALSE;
}


wtap_open_return_val cosine_open(wtap *wth, int *err, gchar **err_info)
{
	/* Look for CoSine header */
	if (!cosine_check_file_type(wth, err, err_info)) {
		if (*err != 0 && *err != WTAP_ERR_SHORT_READ)
			return WTAP_OPEN_ERROR;
		return WTAP_OPEN_NOT_MINE;
	}

	if (file_seek(wth->fh, 0L, SEEK_SET, err) == -1)	/* rewind */
		return WTAP_OPEN_ERROR;

	wth->file_encap = WTAP_ENCAP_COSINE;
	wth->file_type_subtype = WTAP_FILE_TYPE_SUBTYPE_COSINE;
	wth->snapshot_length = 0; /* not known */
	wth->subtype_read = cosine_read;
	wth->subtype_seek_read = cosine_seek_read;
	wth->file_tsprec = WTAP_TSPREC_CSEC;

	return WTAP_OPEN_MINE;
}

/* Find the next packet and parse it; called from wtap_read(). */
static gboolean cosine_read(wtap *wth, int *err, gchar **err_info,
    gint64 *data_offset)
{
	gint64	offset;
	char	line[COSINE_LINE_LENGTH];

	/* Find the next packet */
	offset = cosine_seek_next_packet(wth, err, err_info, line);
	if (offset < 0)
		return FALSE;
	*data_offset = offset;

	/* Parse the header and convert the ASCII hex dump to binary data */
	return parse_cosine_packet(wth->fh, &wth->phdr, wth->frame_buffer,
	    line, err, err_info);
}

/* Used to read packets in random-access fashion */
static gboolean
cosine_seek_read(wtap *wth, gint64 seek_off, struct wtap_pkthdr *phdr,
	Buffer *buf, int *err, gchar **err_info)
{
	char	line[COSINE_LINE_LENGTH];

	if (file_seek(wth->random_fh, seek_off, SEEK_SET, err) == -1)
		return FALSE;

	if (file_gets(line, COSINE_LINE_LENGTH, wth->random_fh) == NULL) {
		*err = file_error(wth->random_fh, err_info);
		if (*err == 0) {
			*err = WTAP_ERR_SHORT_READ;
		}
		return FALSE;
	}

	/* Parse the header and convert the ASCII hex dump to binary data */
	return parse_cosine_packet(wth->random_fh, phdr, buf, line, err,
	    err_info);
}

/* Parses a packet record header. There are two possible formats:
    1) output to a control blade with date and time
        2002-5-10,20:1:31.4:  l2-tx (FR:3/7/1:1), Length:18, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4000, 0x0]
    2) output to PE without date and time
        l2-tx (FR:3/7/1:1), Length:18, Pro:0, Off:0, Pri:0, RM:0, Err:0 [0x4000, 0x0] */
static gboolean
parse_cosine_packet(FILE_T fh, struct wtap_pkthdr *phdr, Buffer *buf,
    char *line, int *err, gchar **err_info)
{
	union wtap_pseudo_header *pseudo_header = &phdr->pseudo_header;
	int	num_items_scanned;
	int	yy, mm, dd, hr, min, sec, csec, pkt_len;
	int	pro, off, pri, rm, error;
	guint	code1, code2;
	char	if_name[COSINE_MAX_IF_NAME_LEN] = "", direction[6] = "";
	struct	tm tm;
	guint8 *pd;
	int	i, hex_lines, n, caplen = 0;

	if (sscanf(line, "%4d-%2d-%2d,%2d:%2d:%2d.%9d:",
		   &yy, &mm, &dd, &hr, &min, &sec, &csec) == 7) {
		/* appears to be output to a control blade */
		num_items_scanned = sscanf(line,
		   "%4d-%2d-%2d,%2d:%2d:%2d.%9d: %5s (%127[A-Za-z0-9/:]), Length:%9d, Pro:%9d, Off:%9d, Pri:%9d, RM:%9d, Err:%9d [%8x, %8x]",
			&yy, &mm, &dd, &hr, &min, &sec, &csec,
				   direction, if_name, &pkt_len,
				   &pro, &off, &pri, &rm, &error,
				   &code1, &code2);

		if (num_items_scanned != 17) {
			*err = WTAP_ERR_BAD_FILE;
			*err_info = g_strdup("cosine: purported control blade line doesn't have code values");
			return FALSE;
		}
	} else {
		/* appears to be output to PE */
		num_items_scanned = sscanf(line,
		   "%5s (%127[A-Za-z0-9/:]), Length:%9d, Pro:%9d, Off:%9d, Pri:%9d, RM:%9d, Err:%9d [%8x, %8x]",
				   direction, if_name, &pkt_len,
				   &pro, &off, &pri, &rm, &error,
				   &code1, &code2);

		if (num_items_scanned != 10) {
			*err = WTAP_ERR_BAD_FILE;
			*err_info = g_strdup("cosine: header line is neither control blade nor PE output");
			return FALSE;
		}
		yy = mm = dd = hr = min = sec = csec = 0;
	}
	if (pkt_len < 0) {
		*err = WTAP_ERR_BAD_FILE;
		*err_info = g_strdup("cosine: packet header has a negative packet length");
		return FALSE;
	}
	if (pkt_len > WTAP_MAX_PACKET_SIZE) {
		/*
		 * Probably a corrupt capture file; don't blow up trying
		 * to allocate space for an immensely-large packet.
		 */
		*err = WTAP_ERR_BAD_FILE;
		*err_info = g_strdup_printf("cosine: File has %u-byte packet, bigger than maximum of %u",
		    pkt_len, WTAP_MAX_PACKET_SIZE);
		return FALSE;
	}

	phdr->rec_type = REC_TYPE_PACKET;
	phdr->presence_flags = WTAP_HAS_TS|WTAP_HAS_CAP_LEN;
	tm.tm_year = yy - 1900;
	tm.tm_mon = mm - 1;
	tm.tm_mday = dd;
	tm.tm_hour = hr;
	tm.tm_min = min;
	tm.tm_sec = sec;
	tm.tm_isdst = -1;
	phdr->ts.secs = mktime(&tm);
	phdr->ts.nsecs = csec * 10000000;
	phdr->len = pkt_len;

	/* XXX need to handle other encapsulations like Cisco HDLC,
	   Frame Relay and ATM */
	if (strncmp(if_name, "TEST:", 5) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_TEST;
	} else if (strncmp(if_name, "PPoATM:", 7) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_PPoATM;
	} else if (strncmp(if_name, "PPoFR:", 6) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_PPoFR;
	} else if (strncmp(if_name, "ATM:", 4) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_ATM;
	} else if (strncmp(if_name, "FR:", 3) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_FR;
	} else if (strncmp(if_name, "HDLC:", 5) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_HDLC;
	} else if (strncmp(if_name, "PPP:", 4) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_PPP;
	} else if (strncmp(if_name, "ETH:", 4) == 0) {
		pseudo_header->cosine.encap = COSINE_ENCAP_ETH;
	} else {
		pseudo_header->cosine.encap = COSINE_ENCAP_UNKNOWN;
	}
	if (strncmp(direction, "l2-tx", 5) == 0) {
		pseudo_header->cosine.direction = COSINE_DIR_TX;
	} else if (strncmp(direction, "l2-rx", 5) == 0) {
		pseudo_header->cosine.direction = COSINE_DIR_RX;
	}
	g_strlcpy(pseudo_header->cosine.if_name, if_name,
		COSINE_MAX_IF_NAME_LEN);
	pseudo_header->cosine.pro = pro;
	pseudo_header->cosine.off = off;
	pseudo_header->cosine.pri = pri;
	pseudo_header->cosine.rm = rm;
	pseudo_header->cosine.err = error;

	/* Make sure we have enough room for the packet */
	ws_buffer_assure_space(buf, pkt_len);
	pd = ws_buffer_start_ptr(buf);

	/* Calculate the number of hex dump lines, each
	 * containing 16 bytes of data */
	hex_lines = pkt_len / 16 + ((pkt_len % 16) ? 1 : 0);

	for (i = 0; i < hex_lines; i++) {
		if (file_gets(line, COSINE_LINE_LENGTH, fh) == NULL) {
			*err = file_error(fh, err_info);
			if (*err == 0) {
				*err = WTAP_ERR_SHORT_READ;
			}
			return FALSE;
		}
		if (empty_line(line)) {
			break;
		}
		if ((n = parse_single_hex_dump_line(line, pd, i*16)) == -1) {
			*err = WTAP_ERR_BAD_FILE;
			*err_info = g_strdup("cosine: hex dump line doesn't have 16 numbers");
			return FALSE;
		}
		caplen += n;
	}
	phdr->caplen = caplen;
	return TRUE;
}

/* Take a string representing one line from a hex dump and converts
 * the text to binary data. We place the bytes in the buffer at the
 * specified offset.
 *
 * Returns number of bytes successfully read, -1 if bad.  */
static int
parse_single_hex_dump_line(char* rec, guint8 *buf, guint byte_offset)
{
	int num_items_scanned, i;
	unsigned int bytes[16];

	num_items_scanned = sscanf(rec, "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x",
			       &bytes[0], &bytes[1], &bytes[2], &bytes[3],
			       &bytes[4], &bytes[5], &bytes[6], &bytes[7],
			       &bytes[8], &bytes[9], &bytes[10], &bytes[11],
			       &bytes[12], &bytes[13], &bytes[14], &bytes[15]);
	if (num_items_scanned == 0)
		return -1;

	if (num_items_scanned > 16)
		num_items_scanned = 16;

	for (i=0; i<num_items_scanned; i++) {
		buf[byte_offset + i] = (guint8)bytes[i];
	}

	return num_items_scanned;
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
