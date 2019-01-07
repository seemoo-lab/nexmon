/* lanalyzer.c
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
#include <stdlib.h>
#include <errno.h>
#include "wtap-int.h"
#include "file_wrappers.h"
#include "lanalyzer.h"
#include "pcapng.h"

/* The LANalyzer format is documented (at least in part) in Novell document
   TID022037, which can be found at, among other places:

     http://www.windowsecurity.com/whitepapers/Description_of_the_LANalysers_output_file.html
 */

/*    Record header format */

typedef struct {
      guint8    record_type[2];
      guint8    record_length[2];
} LA_RecordHeader;

#define LA_RecordHeaderSize 4

/*    Record type codes:                */

#define     RT_HeaderRegular       0x1001
#define     RT_HeaderCyclic        0x1007
#define     RT_RxChannelName       0x1006
#define     RT_TxChannelName       0x100b
#define     RT_FilterName          0x1032
#define     RT_RxTemplateName      0x1035
#define     RT_TxTemplateName      0x1036
#define     RT_DisplayOptions      0x100a
#define     RT_Summary             0x1002
#define     RT_SubfileSummary      0x1003
#define     RT_CyclicInformation   0x1009
#define     RT_Index               0x1004
#define     RT_PacketData          0x1005

#define     LA_ProFileLimit       (1024 * 1024 * 32)

typedef guint8  Eadr[6];
typedef guint16 TimeStamp[3];  /* 0.5 microseconds since start of trace */

/*
 * These records have only 2-byte alignment for 4-byte quantities,
 * so the structures aren't necessarily valid; they're kept as comments
 * for reference purposes.
 */

/*
 * typedef struct {
 *       guint8      day;
 *       guint8      mon;
 *       gint16      year;
 *       } Date;
 */

/*
 * typedef struct {
 *       guint8      second;
 *       guint8      minute;
 *       guint8      hour;
 *       guint8      day;
 *       gint16      reserved;
 *       } Time;
 */

/*
 * RT_Summary:
 *
 * typedef struct {
 *       Date        datcre;
 *       Date        datclo;
 *       Time        timeopn;
 *       Time        timeclo;
 *       Eadr        statadr;
 *       gint16      mxseqno;
 *       gint16      slcoff;
 *       gint16      mxslc;
 *       gint32      totpktt;
 *       gint32      statrg;
 *       gint32      stptrg;
 *       gint32      mxpkta[36];
 *       gint16      board_type;
 *       gint16      board_version;
 *       gint8       reserved[18];
 *       } Summary;
 */

#define SummarySize (18+22+(4*36)+6+6+6+4+4)

/*
 * typedef struct {
 *       gint16      rid;
 *       gint16      rlen;
 *       Summary     s;
 *       } LA_SummaryRecord;
 */

#define LA_SummaryRecordSize (SummarySize + 4)

/* LANalyzer board types (which indicate the type of network on which
   the capture was done). */
#define BOARD_325               226     /* LANalyzer 325 (Ethernet) */
#define BOARD_325TR             227     /* LANalyzer 325TR (Token-ring) */


/*
 * typedef struct {
 *       gint16      rid;
 *       gint16      rlen;
 *       gint16      seqno;
 *       gint32      totpktf;
 *       } LA_SubfileSummaryRecord;
 */

#define LA_SubfileSummaryRecordSize 10


#define LA_IndexSize 500

/*
 * typedef struct {
 *       gint16      rid;
 *       gint16      rlen;
 *       gint16      idxsp;                    = LA_IndexSize
 *       gint16      idxct;
 *       gint8       idxgranu;
 *       gint8       idxvd;
 *       gint32      trcidx[LA_IndexSize + 2]; +2 undocumented but used by La 2.2
 *       } LA_IndexRecord;
 */

#define LA_IndexRecordSize (10 + 4 * (LA_IndexSize + 2))


/*
 * typedef struct {
 *       guint16     rx_channels;
 *       guint16     rx_errors;
 *       gint16      rx_frm_len;
 *       gint16      rx_frm_sln;
 *       TimeStamp   rx_time;
 *       guint32     pktno;
 *       gint16      prvlen;
 *       gint16      offset;
 *       gint16      tx_errs;
 *       gint16      rx_filters;
 *       gint8       unused[2];
 *       gint16      hwcolls;
 *       gint16      hwcollschans;
 *       Packetdata ....;
 *       } LA_PacketRecord;
 */

#define LA_PacketRecordSize 32

typedef struct {
      gboolean        init;
      nstime_t        start;
      guint32         pkts;
      int             encap;
      int             lastlen;
      } LA_TmpInfo;

static const guint8 LA_HeaderRegularFake[] = {
0x01,0x10,0x4c,0x00,0x01,0x05,0x54,0x72,0x61,0x63,0x65,0x20,0x44,0x69,0x73,0x70,
0x6c,0x61,0x79,0x20,0x54,0x72,0x61,0x63,0x65,0x20,0x46,0x69,0x6c,0x65,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
      };

static const guint8 LA_RxChannelNameFake[] = {
0x06,0x10,0x80,0x00,0x43,0x68,0x61,0x6e ,0x6e,0x65,0x6c,0x31,0x00,0x43,0x68,0x61,
0x6e,0x6e,0x65,0x6c,0x32,0x00,0x43,0x68 ,0x61,0x6e,0x6e,0x65,0x6c,0x33,0x00,0x43,
0x68,0x61,0x6e,0x6e,0x65,0x6c,0x34,0x00 ,0x43,0x68,0x61,0x6e,0x6e,0x65,0x6c,0x35,
0x00,0x43,0x68,0x61,0x6e,0x6e,0x65,0x6c ,0x36,0x00,0x43,0x68,0x61,0x6e,0x6e,0x65,
0x6c,0x37,0x00,0x43,0x68,0x61,0x6e,0x6e ,0x65,0x6c,0x38,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00
      };

static const guint8 LA_TxChannelNameFake[] = {
                    0x0b,0x10,0x36,0x00 ,0x54,0x72,0x61,0x6e,0x73,0x31,0x00,0x00,
0x00,0x54,0x72,0x61,0x6e,0x73,0x32,0x00 ,0x00,0x00,0x54,0x72,0x61,0x6e,0x73,0x33,
0x00,0x00,0x00,0x54,0x72,0x61,0x6e,0x73 ,0x34,0x00,0x00,0x00,0x54,0x72,0x61,0x6e,
0x73,0x35,0x00,0x00,0x00,0x54,0x72,0x61 ,0x6e,0x73,0x36,0x00,0x00,0x00
      };

static const guint8 LA_RxTemplateNameFake[] = {
                                                                       0x35,0x10,
0x90,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00
      };

static const guint8 LA_TxTemplateNameFake[] = {
          0x36,0x10,0x36,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00
      };

static const guint8 LA_DisplayOptionsFake[] = {
                                                             0x0a,0x10,0x0a,0x01,
0x00,0x00,0x01,0x00,0x01,0x02,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00
      };

static const guint8 LA_CyclicInformationFake[] = {
                                                   0x09,0x10,0x1a,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00 ,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
      };

static const guint8 z64[64] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00
      };

typedef struct {
      time_t  start;
} lanalyzer_t;

static gboolean lanalyzer_read(wtap *wth, int *err, gchar **err_info,
    gint64 *data_offset);
static gboolean lanalyzer_seek_read(wtap *wth, gint64 seek_off,
    struct wtap_pkthdr *phdr, Buffer *buf, int *err, gchar **err_info);
static gboolean lanalyzer_dump_finish(wtap_dumper *wdh, int *err);

wtap_open_return_val lanalyzer_open(wtap *wth, int *err, gchar **err_info)
{
      LA_RecordHeader rec_header;
      char header_fixed[2];
      char *comment;
      char summary[210];
      guint16 board_type, mxslc;
      guint16 record_type, record_length;
      guint8 cr_day, cr_month;
      guint16 cr_year;
      struct tm tm;
      lanalyzer_t *lanalyzer;

      if (!wtap_read_bytes(wth->fh, &rec_header, LA_RecordHeaderSize,
                           err, err_info)) {
            if (*err != WTAP_ERR_SHORT_READ)
                  return WTAP_OPEN_ERROR;
            return WTAP_OPEN_NOT_MINE;
      }
      record_type = pletoh16(rec_header.record_type);
      record_length = pletoh16(rec_header.record_length); /* make sure to do this for while() loop */

      if (record_type != RT_HeaderRegular && record_type != RT_HeaderCyclic) {
            return WTAP_OPEN_NOT_MINE;
      }

      /* Read the major and minor version numbers */
      if (record_length < 2) {
            /*
             * Not enough room for the major and minor version numbers.
             * Just treat that as a "not a LANalyzer file" indication.
             */
            return WTAP_OPEN_NOT_MINE;
      }
      if (!wtap_read_bytes(wth->fh, &header_fixed, sizeof header_fixed,
                           err, err_info)) {
            if (*err != WTAP_ERR_SHORT_READ)
                  return WTAP_OPEN_ERROR;
            return WTAP_OPEN_NOT_MINE;
      }
      record_length -= sizeof header_fixed;

      if (record_length != 0) {
            /* Read the rest of the record as a comment. */
            comment = (char *)g_malloc(record_length + 1);
            if (!wtap_read_bytes(wth->fh, comment, record_length,
                                 err, err_info)) {
                  if (*err != WTAP_ERR_SHORT_READ) {
                      g_free(comment);
                      return WTAP_OPEN_ERROR;
                  }
                  g_free(comment);
                  return WTAP_OPEN_NOT_MINE;
            }
            wtap_block_add_string_option(g_array_index(wth->shb_hdrs, wtap_block_t, 0), OPT_COMMENT, comment, record_length);
            g_free(comment);
      }

      /* If we made it this far, then the file is a LANAlyzer file.
       * Let's get some info from it. Note that we get wth->snapshot_length
       * from a record later in the file. */
      wth->file_type_subtype = WTAP_FILE_TYPE_SUBTYPE_LANALYZER;
      lanalyzer = (lanalyzer_t *)g_malloc(sizeof(lanalyzer_t));
      wth->priv = (void *)lanalyzer;
      wth->subtype_read = lanalyzer_read;
      wth->subtype_seek_read = lanalyzer_seek_read;
      wth->snapshot_length = 0;
      wth->file_tsprec = WTAP_TSPREC_NSEC;

      /* Read records until we find the start of packets */
      while (1) {
            if (!wtap_read_bytes_or_eof(wth->fh, &rec_header,
                                        LA_RecordHeaderSize, err, err_info)) {
                  if (*err == 0) {
                        /*
                         * End of file and no packets;
                         * accept this file.
                         */
                        return WTAP_OPEN_MINE;
                  }
                  return WTAP_OPEN_ERROR;
            }

            record_type = pletoh16(rec_header.record_type);
            record_length = pletoh16(rec_header.record_length);

            /*g_message("Record 0x%04X Length %d", record_type, record_length);*/
            switch (record_type) {
                  /* Trace Summary Record */
            case RT_Summary:
                  if (!wtap_read_bytes(wth->fh, summary,
                                       sizeof summary, err, err_info))
                        return WTAP_OPEN_ERROR;

                  /* Assume that the date of the creation of the trace file
                   * is the same date of the trace. Lanalyzer doesn't
                   * store the creation date/time of the trace, but only of
                   * the file. Unless you traced at 11:55 PM and saved at 00:05
                   * AM, the assumption that trace.date == file.date is true.
                   */
                  cr_day = summary[0];
                  cr_month = summary[1];
                  cr_year = pletoh16(&summary[2]);
                  /*g_message("Day %d Month %d Year %d (%04X)", cr_day, cr_month,
                    cr_year, cr_year);*/

                  /* Get capture start time. I learned how to do
                   * this from Guy's code in ngsniffer.c
                   */
                  tm.tm_year = cr_year - 1900;
                  tm.tm_mon = cr_month - 1;
                  tm.tm_mday = cr_day;
                  tm.tm_hour = 0;
                  tm.tm_min = 0;
                  tm.tm_sec = 0;
                  tm.tm_isdst = -1;
                  lanalyzer->start = mktime(&tm);
                  /*g_message("Day %d Month %d Year %d", tm.tm_mday,
                    tm.tm_mon, tm.tm_year);*/
                  mxslc = pletoh16(&summary[30]);
                  wth->snapshot_length = mxslc;

                  board_type = pletoh16(&summary[188]);
                  switch (board_type) {
                  case BOARD_325:
                        wth->file_encap = WTAP_ENCAP_ETHERNET;
                        break;
                  case BOARD_325TR:
                        wth->file_encap = WTAP_ENCAP_TOKEN_RING;
                        break;
                  default:
                        *err = WTAP_ERR_UNSUPPORTED;
                        *err_info = g_strdup_printf("lanalyzer: board type %u unknown",
                                                    board_type);
                        return WTAP_OPEN_ERROR;
                  }
                  break;

                  /* Trace Packet Data Record */
            case RT_PacketData:
                  /* Go back header number of bytes so that lanalyzer_read
                   * can read this header */
                  if (file_seek(wth->fh, -LA_RecordHeaderSize, SEEK_CUR, err) == -1) {
                        return WTAP_OPEN_ERROR;
                  }
                  return WTAP_OPEN_MINE;

            default:
                  if (file_seek(wth->fh, record_length, SEEK_CUR, err) == -1) {
                        return WTAP_OPEN_ERROR;
                  }
                  break;
            }
      }
}

#define DESCRIPTOR_LEN  32

static gboolean lanalyzer_read_trace_record(wtap *wth, FILE_T fh,
                                            struct wtap_pkthdr *phdr, Buffer *buf, int *err, gchar **err_info)
{
      char         LE_record_type[2];
      char         LE_record_length[2];
      guint16      record_type, record_length;
      int          record_data_size;
      int          packet_size;
      gchar        descriptor[DESCRIPTOR_LEN];
      lanalyzer_t *lanalyzer;
      guint16      time_low, time_med, time_high, true_size;
      guint64      t;
      time_t       tsecs;

      /* read the record type and length. */
      if (!wtap_read_bytes_or_eof(fh, LE_record_type, 2, err, err_info))
            return FALSE;
      if (!wtap_read_bytes(fh, LE_record_length, 2, err, err_info))
            return FALSE;

      record_type = pletoh16(LE_record_type);
      record_length = pletoh16(LE_record_length);

      /* Only Trace Packet Data Records should occur now that we're in
       * the middle of reading packets.  If any other record type exists
       * after a Trace Packet Data Record, mark it as an error. */
      if (record_type != RT_PacketData) {
            *err = WTAP_ERR_BAD_FILE;
            *err_info = g_strdup_printf("lanalyzer: record type %u seen after trace summary record",
                                        record_type);
            return FALSE;
      }

      if (record_length < DESCRIPTOR_LEN) {
            /*
             * Uh-oh, the record isn't big enough to even have a
             * descriptor.
             */
            *err = WTAP_ERR_BAD_FILE;
            *err_info = g_strdup_printf("lanalyzer: file has a %u-byte record, too small to have even a packet descriptor",
                                        record_length);
            return FALSE;
      }
      record_data_size = record_length - DESCRIPTOR_LEN;

      /* Read the descriptor data */
      if (!wtap_read_bytes(fh, descriptor, DESCRIPTOR_LEN, err, err_info))
            return FALSE;

      true_size = pletoh16(&descriptor[4]);
      packet_size = pletoh16(&descriptor[6]);
      /*
       * The maximum value of packet_size is 65535, which is less than
       * WTAP_MAX_PACKET_SIZE will ever be, so we don't need to check
       * it.
       */

      /*
       * OK, is the frame data size greater than than what's left of the
       * record?
       */
      if (packet_size > record_data_size) {
            /*
             * Yes - treat this as an error.
             */
            *err = WTAP_ERR_BAD_FILE;
            *err_info = g_strdup("lanalyzer: Record length is less than packet size");
            return FALSE;
      }

      phdr->rec_type = REC_TYPE_PACKET;
      phdr->presence_flags = WTAP_HAS_TS|WTAP_HAS_CAP_LEN;

      time_low = pletoh16(&descriptor[8]);
      time_med = pletoh16(&descriptor[10]);
      time_high = pletoh16(&descriptor[12]);
      t = (((guint64)time_low) << 0) + (((guint64)time_med) << 16) +
            (((guint64)time_high) << 32);
      tsecs = (time_t) (t/2000000);
      lanalyzer = (lanalyzer_t *)wth->priv;
      phdr->ts.secs = tsecs + lanalyzer->start;
      phdr->ts.nsecs = ((guint32) (t - tsecs*2000000)) * 500;

      if (true_size - 4 >= packet_size) {
            /*
             * It appears that the "true size" includes the FCS;
             * make it reflect the non-FCS size (the "packet size"
             * appears never to include the FCS, even if no slicing
             * is done).
             */
            true_size -= 4;
      }
      phdr->len = true_size;
      phdr->caplen = packet_size;

      switch (wth->file_encap) {

      case WTAP_ENCAP_ETHERNET:
            /* We assume there's no FCS in this frame. */
            phdr->pseudo_header.eth.fcs_len = 0;
            break;
      }

      /* Read the packet data */
      return wtap_read_packet_bytes(fh, buf, packet_size, err, err_info);
}

/* Read the next packet */
static gboolean lanalyzer_read(wtap *wth, int *err, gchar **err_info,
                               gint64 *data_offset)
{
      *data_offset = file_tell(wth->fh);

      /* Read the record  */
      return lanalyzer_read_trace_record(wth, wth->fh, &wth->phdr,
                                         wth->frame_buffer, err, err_info);
}

static gboolean lanalyzer_seek_read(wtap *wth, gint64 seek_off,
                                    struct wtap_pkthdr *phdr, Buffer *buf, int *err, gchar **err_info)
{
      if (file_seek(wth->random_fh, seek_off, SEEK_SET, err) == -1)
            return FALSE;

      /* Read the record  */
      if (!lanalyzer_read_trace_record(wth, wth->random_fh, phdr, buf,
                                       err, err_info)) {
            if (*err == 0)
                  *err = WTAP_ERR_SHORT_READ;
            return FALSE;
      }
      return TRUE;
}

/*---------------------------------------------------
 * Returns TRUE on success, FALSE on error
 * Write "cnt" bytes of zero with error control
 *---------------------------------------------------*/
static gboolean s0write(wtap_dumper *wdh, size_t cnt, int *err)
{
      size_t snack;

      while (cnt) {
            snack = cnt > 64 ? 64 : cnt;

            if (!wtap_dump_file_write(wdh, z64, snack, err))
                  return FALSE;
            cnt -= snack;
      }
      return TRUE; /* ok */
}

/*---------------------------------------------------
 * Returns TRUE on success, FALSE on error
 * Write an 8-bit value
 *---------------------------------------------------*/
static gboolean s8write(wtap_dumper *wdh, const guint8 s8, int *err)
{
      return wtap_dump_file_write(wdh, &s8, 1, err);
}
/*---------------------------------------------------
 * Returns TRUE on success, FALSE on error
 * Write a 16-bit value as little-endian
 *---------------------------------------------------*/
static gboolean s16write(wtap_dumper *wdh, const guint16 s16, int *err)
{
      guint16 s16_le = GUINT16_TO_LE(s16);
      return wtap_dump_file_write(wdh, &s16_le, 2, err);
}
/*---------------------------------------------------
 * Returns TRUE on success, FALSE on error
 * Write a 32-bit value as little-endian
 *---------------------------------------------------*/
static gboolean s32write(wtap_dumper *wdh, const guint32 s32, int *err)
{
      guint32 s32_le = GUINT32_TO_LE(s32);
      return wtap_dump_file_write(wdh, &s32_le, 4, err);
}
/*---------------------------------------------------
 * Returns TRUE on success, FALSE on error
 * Write a 48-bit value as little-endian
 *---------------------------------------------------*/
static gboolean s48write(wtap_dumper *wdh, const guint64 s48, int *err)
{
#ifdef WORDS_BIGENDIAN
      guint16 s48_upper_le = GUINT16_SWAP_LE_BE((guint16) (s48 >> 32));
      guint32 s48_lower_le = GUINT32_SWAP_LE_BE((guint32) (s48 & 0xFFFFFFFF));
#else
      guint16 s48_upper_le = (guint16) (s48 >> 32);
      guint32 s48_lower_le = (guint32) (s48 & 0xFFFFFFFF);
#endif
      return wtap_dump_file_write(wdh, &s48_lower_le, 4, err) &&
             wtap_dump_file_write(wdh, &s48_upper_le, 2, err);
}
/*---------------------------------------------------
 * Write a record for a packet to a dump file.
 * Returns TRUE on success, FALSE on failure.
 *---------------------------------------------------*/
static gboolean lanalyzer_dump(wtap_dumper *wdh,
        const struct wtap_pkthdr *phdr,
        const guint8 *pd, int *err, gchar **err_info _U_)
{
      guint64 x;
      int    len;

      LA_TmpInfo *itmp = (LA_TmpInfo*)(wdh->priv);
      nstime_t td;
      int    thisSize = phdr->caplen + LA_PacketRecordSize + LA_RecordHeaderSize;

      /* We can only write packet records. */
      if (phdr->rec_type != REC_TYPE_PACKET) {
            *err = WTAP_ERR_UNWRITABLE_REC_TYPE;
            return FALSE;
            }

      if (wdh->bytes_dumped + thisSize > LA_ProFileLimit) {
            /* printf(" LA_ProFileLimit reached\n");     */
            *err = EFBIG;
            return FALSE; /* and don't forget the header */
            }

      len = phdr->caplen + (phdr->caplen ? LA_PacketRecordSize : 0);

      /* len goes into a 16-bit field, so there's a hard limit of 65535. */
      if (len > 65535) {
            *err = WTAP_ERR_PACKET_TOO_LARGE;
            return FALSE;
            }

      if (!s16write(wdh, 0x1005, err))
            return FALSE;
      if (!s16write(wdh, (guint16)len, err))
            return FALSE;

      if (!itmp->init) {
            /* collect some information for the
             * finally written header
             */
            itmp->start   = phdr->ts;
            itmp->pkts    = 0;
            itmp->init    = TRUE;
            itmp->encap   = wdh->encap;
            itmp->lastlen = 0;
            }

      if (!s16write(wdh, 0x0001, err))                    /* pr.rx_channels */
            return FALSE;
      if (!s16write(wdh, 0x0008, err))                    /* pr.rx_errors   */
            return FALSE;
      if (!s16write(wdh, (guint16) (phdr->len + 4), err)) /* pr.rx_frm_len  */
            return FALSE;
      if (!s16write(wdh, (guint16) phdr->caplen, err))    /* pr.rx_frm_sln  */
            return FALSE;

      nstime_delta(&td, &phdr->ts, &itmp->start);

      /* Convert to half-microseconds, rounded up. */
      x = (td.nsecs + 250) / 500;  /* nanoseconds -> half-microseconds, rounded */
      x += td.secs * 2000000;      /* seconds -> half-microseconds */

      if (!s48write(wdh, x, err))                        /* pr.rx_time[i]  */
            return FALSE;

      if (!s32write(wdh, ++itmp->pkts, err))             /* pr.pktno      */
            return FALSE;
      if (!s16write(wdh, (guint16)itmp->lastlen, err))   /* pr.prlen      */
            return FALSE;
      itmp->lastlen = len;

      if (!s0write(wdh, 12, err))
            return FALSE;

      if (!wtap_dump_file_write(wdh, pd, phdr->caplen, err))
            return FALSE;

      wdh->bytes_dumped += thisSize;

      return TRUE;
}

/*---------------------------------------------------
 * Returns 0 if we could write the specified encapsulation type,
 * an error indication otherwise.
 *---------------------------------------------------*/
int lanalyzer_dump_can_write_encap(int encap)
{
      /* Per-packet encapsulations aren't supported. */
      if (encap == WTAP_ENCAP_PER_PACKET)
                  return WTAP_ERR_ENCAP_PER_PACKET_UNSUPPORTED;

      if ( encap != WTAP_ENCAP_ETHERNET
        && encap != WTAP_ENCAP_TOKEN_RING )
                  return WTAP_ERR_UNWRITABLE_ENCAP;
      /*
       * printf("lanalyzer_dump_can_write_encap(%d)\n",encap);
       */
      return 0;
}

/*---------------------------------------------------
 * Returns TRUE on success, FALSE on failure; sets "*err" to an
 * error code on failure
 *---------------------------------------------------*/
gboolean lanalyzer_dump_open(wtap_dumper *wdh, int *err)
{
      int   jump;
      void  *tmp;

      tmp = g_malloc(sizeof(LA_TmpInfo));
      if (!tmp) {
            *err = errno;
            return FALSE;
            }

      ((LA_TmpInfo*)tmp)->init = FALSE;
      wdh->priv           = tmp;
      wdh->subtype_write  = lanalyzer_dump;
      wdh->subtype_finish = lanalyzer_dump_finish;

      /* Some of the fields in the file header aren't known yet so
       just skip over it for now.  It will be created after all
       of the packets have been written. */

      jump = sizeof (LA_HeaderRegularFake)
           + sizeof (LA_RxChannelNameFake)
           + sizeof (LA_TxChannelNameFake)
           + sizeof (LA_RxTemplateNameFake)
           + sizeof (LA_TxTemplateNameFake)
           + sizeof (LA_DisplayOptionsFake)
           + LA_SummaryRecordSize
           + LA_SubfileSummaryRecordSize
           + sizeof (LA_CyclicInformationFake)
           + LA_IndexRecordSize;

      if (wtap_dump_file_seek(wdh, jump, SEEK_SET, err) == -1)
            return FALSE;

      wdh->bytes_dumped = jump;
      return TRUE;
}

/*---------------------------------------------------
 *
 *---------------------------------------------------*/
static gboolean lanalyzer_dump_header(wtap_dumper *wdh, int *err)
{
      LA_TmpInfo *itmp   = (LA_TmpInfo*)(wdh->priv);
      guint16 board_type = itmp->encap == WTAP_ENCAP_TOKEN_RING
                              ? BOARD_325TR     /* LANalyzer Board Type */
                              : BOARD_325;      /* LANalyzer Board Type */
      struct tm *fT;

      fT = localtime(&itmp->start.secs);
      if (fT == NULL)
            return FALSE;

      if (wtap_dump_file_seek(wdh, 0, SEEK_SET, err) == -1)
            return FALSE;

      if (!wtap_dump_file_write(wdh, &LA_HeaderRegularFake,
                                sizeof LA_HeaderRegularFake, err))
            return FALSE;
      if (!wtap_dump_file_write(wdh, &LA_RxChannelNameFake,
                                sizeof LA_RxChannelNameFake, err))
            return FALSE;
      if (!wtap_dump_file_write(wdh, &LA_TxChannelNameFake,
                                sizeof LA_TxChannelNameFake, err))
            return FALSE;
      if (!wtap_dump_file_write(wdh, &LA_RxTemplateNameFake,
                                sizeof LA_RxTemplateNameFake, err))
            return FALSE;
      if (!wtap_dump_file_write(wdh, &LA_TxTemplateNameFake,
                                sizeof LA_TxTemplateNameFake, err))
            return FALSE;
      if (!wtap_dump_file_write(wdh, &LA_DisplayOptionsFake,
                                sizeof LA_DisplayOptionsFake, err))
            return FALSE;
      /*-----------------------------------------------------------------*/
      if (!s16write(wdh, RT_Summary, err))         /* rid */
            return FALSE;
      if (!s16write(wdh, SummarySize, err))        /* rlen */
            return FALSE;
      if (!s8write(wdh, (guint8) fT->tm_mday, err))        /* s.datcre.day */
            return FALSE;
      if (!s8write(wdh, (guint8) (fT->tm_mon+1), err))     /* s.datcre.mon */
            return FALSE;
      if (!s16write(wdh, (guint16) (fT->tm_year + 1900), err)) /* s.datcre.year */
            return FALSE;
      if (!s8write(wdh, (guint8) fT->tm_mday, err))        /* s.datclo.day */
            return FALSE;
      if (!s8write(wdh, (guint8) (fT->tm_mon+1), err))     /* s.datclo.mon */
            return FALSE;
      if (!s16write(wdh, (guint16) (fT->tm_year + 1900), err)) /* s.datclo.year */
            return FALSE;
      if (!s8write(wdh, (guint8) fT->tm_sec, err))         /* s.timeopn.second */
            return FALSE;
      if (!s8write(wdh, (guint8) fT->tm_min, err))         /* s.timeopn.minute */
            return FALSE;
      if (!s8write(wdh, (guint8) fT->tm_hour, err))        /* s.timeopn.hour */
            return FALSE;
      if (!s8write(wdh, (guint8) fT->tm_mday, err))        /* s.timeopn.mday */
            return FALSE;
      if (!s0write(wdh, 2, err))
            return FALSE;
      if (!s8write(wdh, (guint8) fT->tm_sec, err))         /* s.timeclo.second */
            return FALSE;
      if (!s8write(wdh, (guint8) fT->tm_min, err))         /* s.timeclo.minute */
            return FALSE;
      if (!s8write(wdh, (guint8) fT->tm_hour, err))        /* s.timeclo.hour */
            return FALSE;
      if (!s8write(wdh, (guint8) fT->tm_mday, err))        /* s.timeclo.mday */
            return FALSE;
      if (!s0write(wdh, 2, err))
            return FALSE;
      if (!s0write(wdh, 6, err))                           /* EAddr  == 0      */
            return FALSE;
      if (!s16write(wdh, 1, err))                  /* s.mxseqno */
            return FALSE;
      if (!s16write(wdh, 0, err))                  /* s.slcoffo */
            return FALSE;
      if (!s16write(wdh, 1514, err))               /* s.mxslc */
            return FALSE;
      if (!s32write(wdh, itmp->pkts, err))         /* s.totpktt */
            return FALSE;
      /*
       * statrg == 0; ? -1
       * stptrg == 0; ? -1
       * s.mxpkta[0]=0
       */
      if (!s0write(wdh, 12, err))
            return FALSE;
      if (!s32write(wdh, itmp->pkts, err))         /* sr.s.mxpkta[1]  */
            return FALSE;
      if (!s0write(wdh, 34*4, err))                /* s.mxpkta[2-33]=0  */
            return FALSE;
      if (!s16write(wdh, board_type, err))
            return FALSE;
      if (!s0write(wdh, 20, err))                     /* board_version == 0 */
            return FALSE;
      /*-----------------------------------------------------------------*/
      if (!s16write(wdh, RT_SubfileSummary, err))     /* ssr.rid */
            return FALSE;
      if (!s16write(wdh, LA_SubfileSummaryRecordSize-4, err)) /* ssr.rlen */
            return FALSE;
      if (!s16write(wdh, 1, err))                     /* ssr.seqno */
            return FALSE;
      if (!s32write(wdh, itmp->pkts, err))            /* ssr.totpkts */
            return FALSE;
      /*-----------------------------------------------------------------*/
      if (!wtap_dump_file_write(wdh, &LA_CyclicInformationFake,
                                sizeof LA_CyclicInformationFake, err))
            return FALSE;
      /*-----------------------------------------------------------------*/
      if (!s16write(wdh, RT_Index, err))              /* rid */
            return FALSE;
      if (!s16write(wdh, LA_IndexRecordSize -4, err)) /* rlen */
            return FALSE;
      if (!s16write(wdh, LA_IndexSize, err))          /* idxsp */
            return FALSE;
      if (!s0write(wdh, LA_IndexRecordSize - 6, err))
            return FALSE;

      return TRUE;
}

/*---------------------------------------------------
 * Finish writing to a dump file.
 * Returns TRUE on success, FALSE on failure.
 *---------------------------------------------------*/
static gboolean lanalyzer_dump_finish(wtap_dumper *wdh, int *err)
{
      lanalyzer_dump_header(wdh,err);
      return *err ? FALSE : TRUE;
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 6
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=6 tabstop=8 expandtab:
 * :indentSize=6:tabSize=8:noTabs=true:
 */
