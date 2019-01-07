/* catapult_dct2000.c
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
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include "wtap-int.h"
#include "file_wrappers.h"

#include "catapult_dct2000.h"

#define MAX_FIRST_LINE_LENGTH      200
#define MAX_TIMESTAMP_LINE_LENGTH  100
#define MAX_LINE_LENGTH            65536
#define MAX_TIMESTAMP_LEN          32
#define MAX_SECONDS_CHARS          16
#define MAX_SUBSECOND_DECIMALS     4
#define MAX_CONTEXT_NAME           64
#define MAX_PROTOCOL_NAME          64
#define MAX_PORT_DIGITS            2
#define MAX_VARIANT_DIGITS         32
#define MAX_OUTHDR_NAME            256
#define AAL_HEADER_CHARS           12

/* TODO:
   - support for FP over AAL0
   - support for IuR interface FP
   - support for x.25?
*/

/* 's' or 'r' of a packet as read from .out file */
typedef enum packet_direction_t
{
    sent,
    received
} packet_direction_t;


/***********************************************************************/
/* For each line, store (in case we need to dump):                     */
/* - String before time field                                          */
/* - String beween time field and data (if NULL assume " l ")          */
typedef struct
{
    gchar *before_time;
    gchar *after_time;
} line_prefix_info_t;


/*******************************************************************/
/* Information stored external to a file (wtap) needed for reading and dumping */
typedef struct dct2000_file_externals
{
    /* Remember the time at the start of capture */
    time_t  start_secs;
    guint32 start_usecs;

    /*
     * The following information is needed only for dumping.
     *
     * XXX - Wiretap is not *supposed* to require that a packet being
     * dumped come from a file of the same type that you currently have
     * open; this should be fixed.
     */

    /* Buffer to hold first line, including magic and format number */
    gchar firstline[MAX_FIRST_LINE_LENGTH];
    gint  firstline_length;

    /* Buffer to hold second line with formatted file creation data/time */
    gchar secondline[MAX_TIMESTAMP_LINE_LENGTH];
    gint  secondline_length;

    /* Hash table to store text prefix data part of displayed packets.
       Records (file offset -> line_prefix_info_t)
    */
    GHashTable *packet_prefix_table;
} dct2000_file_externals_t;

/* 'Magic number' at start of Catapult DCT2000 .out files. */
static const gchar catapult_dct2000_magic[] = "Session Transcript";

/************************************************************/
/* Functions called from wiretap core                       */
static gboolean catapult_dct2000_read(wtap *wth, int *err, gchar **err_info,
                                      gint64 *data_offset);
static gboolean catapult_dct2000_seek_read(wtap *wth, gint64 seek_off,
                                           struct wtap_pkthdr *phdr,
                                           Buffer *buf, int *err,
                                           gchar **err_info);
static void catapult_dct2000_close(wtap *wth);

static gboolean catapult_dct2000_dump(wtap_dumper *wdh, const struct wtap_pkthdr *phdr,
                                      const guint8 *pd, int *err, gchar **err_info);


/************************************************************/
/* Private helper functions                                 */
static gboolean read_new_line(FILE_T fh, gint64 *offset, gint *length,
                              gchar *buf, size_t bufsize, int *err,
                              gchar **err_info);
static gboolean parse_line(char *linebuff, gint line_length,
                           gint *seconds, gint *useconds,
                           long *before_time_offset, long *after_time_offset,
                           long *data_offset,
                           gint *data_chars,
                           packet_direction_t *direction,
                           int *encap, int *is_comment, int *is_sprint,
                           gchar *aal_header_chars,
                           gchar *context_name, guint8 *context_portp,
                           gchar *protocol_name, gchar *variant_name,
                           gchar *outhdr_name);
static gboolean process_parsed_line(wtap *wth,
                                    dct2000_file_externals_t *file_externals,
                                    struct wtap_pkthdr *phdr,
                                    Buffer *buf, gint64 file_offset,
                                    char *linebuff, long dollar_offset,
                                    int seconds, int useconds,
                                    gchar *timestamp_string,
                                    packet_direction_t direction, int encap,
                                    gchar *context_name, guint8 context_port,
                                    gchar *protocol_name, gchar *variant_name,
                                    gchar *outhdr_name, gchar *aal_header_chars,
                                    gboolean is_comment, int data_chars,
                                    int *err, gchar **err_info);
static guint8 hex_from_char(gchar c);
static void   prepare_hex_byte_from_chars_table(void);
static guint8 hex_byte_from_chars(gchar *c);
static gchar char_from_hex(guint8 hex);

static void set_aal_info(union wtap_pseudo_header *pseudo_header,
                         packet_direction_t direction,
                         gchar *aal_header_chars);
static void set_isdn_info(union wtap_pseudo_header *pseudo_header,
                          packet_direction_t direction);
static void set_ppp_info(union wtap_pseudo_header *pseudo_header,
                         packet_direction_t direction);

static gint packet_offset_equal(gconstpointer v, gconstpointer v2);
static guint packet_offset_hash_func(gconstpointer v);

static gboolean get_file_time_stamp(gchar *linebuff, time_t *secs, guint32 *usecs);
static gboolean free_line_prefix_info(gpointer key, gpointer value, gpointer user_data);



/********************************************/
/* Open file (for reading)                 */
/********************************************/
wtap_open_return_val
catapult_dct2000_open(wtap *wth, int *err, gchar **err_info)
{
    gint64  offset = 0;
    time_t  timestamp;
    guint32 usecs;
    gint firstline_length = 0;
    dct2000_file_externals_t *file_externals;
    static gchar linebuff[MAX_LINE_LENGTH];
    static gboolean hex_byte_table_values_set = FALSE;

    /* Clear errno before reading from the file */
    errno = 0;


    /********************************************************************/
    /* First line needs to contain at least as many characters as magic */

    if (!read_new_line(wth->fh, &offset, &firstline_length, linebuff,
                       sizeof linebuff, err, err_info)) {
        if (*err != 0 && *err != WTAP_ERR_SHORT_READ)
            return WTAP_OPEN_ERROR;
        return WTAP_OPEN_NOT_MINE;
    }
    if (((size_t)firstline_length < strlen(catapult_dct2000_magic)) ||
        firstline_length >= MAX_FIRST_LINE_LENGTH) {

        return WTAP_OPEN_NOT_MINE;
    }

    /* This file is not for us if it doesn't match our signature */
    if (memcmp(catapult_dct2000_magic, linebuff, strlen(catapult_dct2000_magic)) != 0) {
        return WTAP_OPEN_NOT_MINE;
    }

    /* Make sure table is ready for use */
    if (!hex_byte_table_values_set) {
        prepare_hex_byte_from_chars_table();
        hex_byte_table_values_set = TRUE;
    }

    /*********************************************************************/
    /* Need entry in file_externals table                                */

    /* Allocate a new file_externals structure for this file */
    file_externals = g_new(dct2000_file_externals_t,1);
    memset((void*)file_externals, '\0', sizeof(dct2000_file_externals_t));

    /* Copy this first line into buffer so could write out later */
    g_strlcpy(file_externals->firstline, linebuff, firstline_length+1);
    file_externals->firstline_length = firstline_length;


    /***********************************************************/
    /* Second line contains file timestamp                     */
    /* Store this offset in in file_externals                  */

    if (!read_new_line(wth->fh, &offset, &(file_externals->secondline_length),
                       linebuff, sizeof linebuff, err, err_info)) {
        g_free(file_externals);
        if (*err != 0 && *err != WTAP_ERR_SHORT_READ)
            return WTAP_OPEN_ERROR;
        return WTAP_OPEN_NOT_MINE;
    }
    if ((file_externals->secondline_length >= MAX_TIMESTAMP_LINE_LENGTH) ||
        (!get_file_time_stamp(linebuff, &timestamp, &usecs))) {

        /* Give up if file time line wasn't valid */
        g_free(file_externals);
        return WTAP_OPEN_NOT_MINE;
    }

    /* Fill in timestamp */
    file_externals->start_secs = timestamp;
    file_externals->start_usecs = usecs;

    /* Copy this second line into buffer so could write out later */
    g_strlcpy(file_externals->secondline, linebuff, file_externals->secondline_length+1);


    /************************************************************/
    /* File is for us. Fill in details so packets can be read   */

    /* Set our file type */
    wth->file_type_subtype = WTAP_FILE_TYPE_SUBTYPE_CATAPULT_DCT2000;

    /* Use our own encapsulation to send all packets to our stub dissector */
    wth->file_encap = WTAP_ENCAP_CATAPULT_DCT2000;

    /* Callbacks for reading operations */
    wth->subtype_read = catapult_dct2000_read;
    wth->subtype_seek_read = catapult_dct2000_seek_read;
    wth->subtype_close = catapult_dct2000_close;

    /* Choose microseconds (have 4 decimal places...) */
    wth->file_tsprec = WTAP_TSPREC_USEC;


    /***************************************************************/
    /* Initialise packet_prefix_table (index is offset into file)  */
    file_externals->packet_prefix_table =
        g_hash_table_new(packet_offset_hash_func, packet_offset_equal);

    /* Set this wtap to point to the file_externals */
    wth->priv = (void*)file_externals;

    *err = errno;
    return WTAP_OPEN_MINE;
}

/* Ugly, but much faster than using g_snprintf! */
static void write_timestamp_string(char *timestamp_string, int secs, int tenthousandths)
{
    int idx = 0;

    /* Secs */
    if (secs < 10) {
        timestamp_string[idx++] = ((secs % 10))         + '0';
    }
    else if (secs < 100) {
        timestamp_string[idx++] = ( secs  /       10)   + '0';
        timestamp_string[idx++] = ((secs % 10))          + '0';
    }
    else if (secs < 1000) {
        timestamp_string[idx++] = ((secs)         / 100)   + '0';
        timestamp_string[idx++] = ((secs % 100))  / 10     + '0';
        timestamp_string[idx++] = ((secs % 10))            + '0';
    }
    else if (secs < 10000) {
        timestamp_string[idx++] = ((secs)          / 1000)   + '0';
        timestamp_string[idx++] = ((secs % 1000))  / 100     + '0';
        timestamp_string[idx++] = ((secs % 100))   / 10      + '0';
        timestamp_string[idx++] = ((secs % 10))              + '0';
    }
    else if (secs < 100000) {
        timestamp_string[idx++] = ((secs)          / 10000)   + '0';
        timestamp_string[idx++] = ((secs % 10000)) / 1000     + '0';
        timestamp_string[idx++] = ((secs % 1000))  / 100      + '0';
        timestamp_string[idx++] = ((secs % 100))   / 10       + '0';
        timestamp_string[idx++] = ((secs % 10))               + '0';
    }
    else if (secs < 1000000) {
        timestamp_string[idx++] = ((secs)           / 100000) + '0';
        timestamp_string[idx++] = ((secs % 100000)) / 10000   + '0';
        timestamp_string[idx++] = ((secs % 10000))  / 1000    + '0';
        timestamp_string[idx++] = ((secs % 1000))   / 100     + '0';
        timestamp_string[idx++] = ((secs % 100))    / 10      + '0';
        timestamp_string[idx++] = ((secs % 10))               + '0';
    }
    else {
        g_snprintf(timestamp_string, MAX_TIMESTAMP_LEN, "%d.%04d", secs, tenthousandths);
        return;
    }

    timestamp_string[idx++] = '.';
    timestamp_string[idx++] = ( tenthousandths          / 1000) + '0';
    timestamp_string[idx++] = ((tenthousandths % 1000)  / 100)  + '0';
    timestamp_string[idx++] = ((tenthousandths % 100)   / 10)   + '0';
    timestamp_string[idx++] = ((tenthousandths % 10))           + '0';
    timestamp_string[idx++] = '\0';
}

/**************************************************/
/* Read packet function.                          */
/* Look for and read the next usable packet       */
/* - return TRUE and details if found             */
/**************************************************/
static gboolean
catapult_dct2000_read(wtap *wth, int *err, gchar **err_info,
                      gint64 *data_offset)
{
    gint64 offset = file_tell(wth->fh);
    long dollar_offset, before_time_offset, after_time_offset;
    packet_direction_t direction;
    int encap;

    /* Get wtap external structure for this wtap */
    dct2000_file_externals_t *file_externals =
        (dct2000_file_externals_t*)wth->priv;

    /* Search for a line containing a usable packet */
    while (1) {
        int line_length, seconds, useconds, data_chars;
        int is_comment = FALSE;
        int is_sprint = FALSE;
        gint64 this_offset = offset;
        static gchar linebuff[MAX_LINE_LENGTH+1];
        gchar aal_header_chars[AAL_HEADER_CHARS];
        gchar context_name[MAX_CONTEXT_NAME];
        guint8 context_port = 0;
        gchar protocol_name[MAX_PROTOCOL_NAME+1];
        gchar variant_name[MAX_VARIANT_DIGITS+1];
        gchar outhdr_name[MAX_OUTHDR_NAME+1];

        /* Are looking for first packet after 2nd line */
        if (file_tell(wth->fh) == 0) {
            this_offset += (file_externals->firstline_length+1+
                            file_externals->secondline_length+1);
        }

        /* Read a new line from file into linebuff */
        if (!read_new_line(wth->fh, &offset, &line_length, linebuff,
                           sizeof linebuff, err, err_info)) {
            if (*err != 0)
                return FALSE;  /* error */
            /* No more lines can be read, so quit. */
            break;
        }

        /* Try to parse the line as a frame record */
        if (parse_line(linebuff, line_length, &seconds, &useconds,
                       &before_time_offset, &after_time_offset,
                       &dollar_offset,
                       &data_chars, &direction, &encap, &is_comment, &is_sprint,
                       aal_header_chars,
                       context_name, &context_port,
                       protocol_name, variant_name, outhdr_name)) {
            line_prefix_info_t *line_prefix_info;
            char timestamp_string[MAX_TIMESTAMP_LEN+1];
            gint64 *pkey = NULL;

            write_timestamp_string(timestamp_string, seconds, useconds/100);

            /* Set data_offset to the beginning of the line we're returning.
               This will be the seek_off parameter when this frame is re-read.
            */
            *data_offset = this_offset;

            if (!process_parsed_line(wth, file_externals,
                                     &wth->phdr,
                                     wth->frame_buffer, this_offset,
                                     linebuff, dollar_offset,
                                     seconds, useconds,
                                     timestamp_string,
                                     direction, encap,
                                     context_name, context_port,
                                     protocol_name, variant_name,
                                     outhdr_name, aal_header_chars,
                                     is_comment, data_chars,
                                     err, err_info))
                return FALSE;

            /* Store the packet prefix in the hash table */
            line_prefix_info = g_new(line_prefix_info_t,1);

            /* Create and use buffer for contents before time */
            line_prefix_info->before_time = (gchar *)g_malloc(before_time_offset+1);
            memcpy(line_prefix_info->before_time, linebuff, before_time_offset);
            line_prefix_info->before_time[before_time_offset] = '\0';

            /* Create and use buffer for contents before time.
               Do this only if it doesn't correspond to " l ", which is by far the most
               common case. */
            if (((size_t)(dollar_offset - after_time_offset -1) == strlen(" l ")) &&
                (strncmp(linebuff+after_time_offset, " l ", strlen(" l ")) == 0)) {

                line_prefix_info->after_time = NULL;
            }
            else {
                /* Allocate & write buffer for line between timestamp and data */
                line_prefix_info->after_time = (gchar *)g_malloc(dollar_offset - after_time_offset);
                memcpy(line_prefix_info->after_time, linebuff+after_time_offset, dollar_offset - after_time_offset);
                line_prefix_info->after_time[dollar_offset - after_time_offset-1] = '\0';
            }

            /* Add packet entry into table */
            pkey = (gint64 *)g_malloc(sizeof(*pkey));
            *pkey = this_offset;
            g_hash_table_insert(file_externals->packet_prefix_table, pkey, line_prefix_info);

            /* OK, we have packet details to return */
            return TRUE;
        }
    }

    /* No packet details to return... */
    return FALSE;
}


/**************************************************/
/* Read & seek function.                          */
/**************************************************/
static gboolean
catapult_dct2000_seek_read(wtap *wth, gint64 seek_off,
                           struct wtap_pkthdr *phdr, Buffer *buf,
                           int *err, gchar **err_info)
{
    gint64 offset = 0;
    int length;
    long dollar_offset, before_time_offset, after_time_offset;
    static gchar linebuff[MAX_LINE_LENGTH+1];
    gchar aal_header_chars[AAL_HEADER_CHARS];
    gchar context_name[MAX_CONTEXT_NAME];
    guint8 context_port = 0;
    gchar protocol_name[MAX_PROTOCOL_NAME+1];
    gchar variant_name[MAX_VARIANT_DIGITS+1];
    gchar outhdr_name[MAX_OUTHDR_NAME+1];
    int  is_comment = FALSE;
    int  is_sprint = FALSE;
    packet_direction_t direction;
    int encap;
    int seconds, useconds, data_chars;

    /* Get wtap external structure for this wtap */
    dct2000_file_externals_t *file_externals =
        (dct2000_file_externals_t*)wth->priv;

    /* Reset errno */
    *err = errno = 0;

    /* Seek to beginning of packet */
    if (file_seek(wth->random_fh, seek_off, SEEK_SET, err) == -1) {
        return FALSE;
    }

    /* Re-read whole line (this really should succeed) */
    if (!read_new_line(wth->random_fh, &offset, &length, linebuff,
                      sizeof linebuff, err, err_info)) {
        return FALSE;
    }

    /* Try to parse this line again (should succeed as re-reading...) */
    if (parse_line(linebuff, length, &seconds, &useconds,
                   &before_time_offset, &after_time_offset,
                   &dollar_offset,
                   &data_chars, &direction, &encap, &is_comment, &is_sprint,
                   aal_header_chars,
                   context_name, &context_port,
                   protocol_name, variant_name, outhdr_name)) {
        char timestamp_string[MAX_TIMESTAMP_LEN+1];

        write_timestamp_string(timestamp_string, seconds, useconds/100);

        if (!process_parsed_line(wth, file_externals,
                                 phdr, buf, seek_off,
                                 linebuff, dollar_offset,
                                 seconds, useconds,
                                 timestamp_string,
                                 direction, encap,
                                 context_name, context_port,
                                 protocol_name, variant_name,
                                 outhdr_name, aal_header_chars,
                                 is_comment, data_chars,
                                 err, err_info))
            return FALSE;

        *err = errno = 0;
        return TRUE;
    }

    /* If get here, must have failed */
    *err = errno;
    *err_info = g_strdup_printf("catapult dct2000: seek_read failed to read/parse "
                                "line at position %" G_GINT64_MODIFIER "d",
                                seek_off);
    return FALSE;
}


/***************************************************************************/
/* Free dct2000-specific capture info from file that was open for reading  */
/***************************************************************************/
static void
catapult_dct2000_close(wtap *wth)
{
    /* Get externals for this file */
    dct2000_file_externals_t *file_externals =
        (dct2000_file_externals_t*)wth->priv;

    /* Free up its line prefix values */
    g_hash_table_foreach_remove(file_externals->packet_prefix_table,
                                free_line_prefix_info, NULL);
    /* Free up its line prefix table */
    g_hash_table_destroy(file_externals->packet_prefix_table);
}




/***************************/
/* Dump functions          */
/***************************/

typedef struct {
    gboolean   first_packet_written;
    nstime_t   start_time;
} dct2000_dump_t;

/*****************************************************/
/* The file that we are writing to has been opened.  */
/* Set other dump callbacks.                         */
/*****************************************************/
gboolean
catapult_dct2000_dump_open(wtap_dumper *wdh, int *err _U_)
{
    /* Fill in other dump callbacks */
    wdh->subtype_write = catapult_dct2000_dump;

    return TRUE;
}

/*********************************************************/
/* Respond to queries about which encap types we support */
/* writing to.                                           */
/*********************************************************/
int
catapult_dct2000_dump_can_write_encap(int encap)
{
    switch (encap) {
        case WTAP_ENCAP_CATAPULT_DCT2000:
            /* We support this */
            return 0;

        default:
            /* But don't write to any other formats... */
            return WTAP_ERR_UNWRITABLE_ENCAP;
    }
}


/*****************************************/
/* Write a single packet out to the file */
/*****************************************/

static gboolean
catapult_dct2000_dump(wtap_dumper *wdh, const struct wtap_pkthdr *phdr,
                      const guint8 *pd, int *err, gchar **err_info _U_)
{
    const union wtap_pseudo_header *pseudo_header = &phdr->pseudo_header;
    guint32 n;
    line_prefix_info_t *prefix = NULL;
    gchar time_string[16];
    gboolean is_comment;
    gboolean is_sprint = FALSE;
    dct2000_dump_t *dct2000;
    int consecutive_slashes=0;
    char *p_c;

    /******************************************************/
    /* Get the file_externals structure for this file */
    /* Find wtap external structure for this wtap */
    dct2000_file_externals_t *file_externals =
        (dct2000_file_externals_t*)pseudo_header->dct2000.wth->priv;

    /* We can only write packet records. */
    if (phdr->rec_type != REC_TYPE_PACKET) {
        *err = WTAP_ERR_UNWRITABLE_REC_TYPE;
        return FALSE;
    }

    dct2000 = (dct2000_dump_t *)wdh->priv;
    if (dct2000 == NULL) {

        /* Write out saved first line */
        if (!wtap_dump_file_write(wdh, file_externals->firstline,
                                  file_externals->firstline_length, err)) {
            return FALSE;
        }
        if (!wtap_dump_file_write(wdh, "\n", 1, err)) {
            return FALSE;
        }

        /* Also write out saved second line with timestamp corresponding to the
           opening time of the log.
        */
        if (!wtap_dump_file_write(wdh, file_externals->secondline,
                                  file_externals->secondline_length, err)) {
            return FALSE;
        }
        if (!wtap_dump_file_write(wdh, "\n", 1, err)) {
            return FALSE;
        }

        /* Allocate the dct2000-specific dump structure */
        dct2000 = (dct2000_dump_t *)g_malloc(sizeof(dct2000_dump_t));
        wdh->priv = (void *)dct2000;

        /* Copy time of beginning of file */
        dct2000->start_time.secs = file_externals->start_secs;
        dct2000->start_time.nsecs =
            (file_externals->start_usecs * 1000);

        /* Set flag so don't write header out again */
        dct2000->first_packet_written = TRUE;
    }


    /******************************************************************/
    /* Write out this packet's prefix, including calculated timestamp */

    /* Look up line data prefix using stored offset */
    prefix = (line_prefix_info_t*)g_hash_table_lookup(file_externals->packet_prefix_table,
                                                      (const void*)&(pseudo_header->dct2000.seek_off));

    /* Write out text before timestamp */
    if (!wtap_dump_file_write(wdh, prefix->before_time,
                              strlen(prefix->before_time), err)) {
        return FALSE;
    }

    /* Can infer from prefix if this is a comment (whose payload is displayed differently) */
    /* This is much faster than strstr() for "/////" */
    p_c = prefix->before_time;
    while (p_c && (*p_c != '/')) {
        p_c++;
    }
    while (p_c && (*p_c == '/')) {
        consecutive_slashes++;
        p_c++;
    }
    is_comment = (consecutive_slashes == 5);

    /* Calculate time of this packet to write, relative to start of dump */
    if (phdr->ts.nsecs >= dct2000->start_time.nsecs) {
        write_timestamp_string(time_string,
                               (int)(phdr->ts.secs - dct2000->start_time.secs),
                               (phdr->ts.nsecs - dct2000->start_time.nsecs) / 100000);
    }
    else {
        write_timestamp_string(time_string,
                               (int)(phdr->ts.secs - dct2000->start_time.secs-1),
                               ((1000000000 + (phdr->ts.nsecs / 100000)) - (dct2000->start_time.nsecs / 100000)) % 10000);
    }

    /* Write out the calculated timestamp */
    if (!wtap_dump_file_write(wdh, time_string, strlen(time_string), err)) {
        return FALSE;
    }

    /* Write out text between timestamp and start of hex data */
    if (prefix->after_time == NULL) {
        if (!wtap_dump_file_write(wdh, " l ", strlen(" l "), err)) {
            return FALSE;
        }
    }
    else {
        if (!wtap_dump_file_write(wdh, prefix->after_time,
                                  strlen(prefix->after_time), err)) {
            return FALSE;
        }
    }


    /****************************************************************/
    /* Need to skip stub header at start of pd before we reach data */

    /* Context name */
    for (n=0; pd[n] != '\0'; n++);
    n++;

    /* Context port number */
    n++;

    /* Timestamp */
    for (; pd[n] != '\0'; n++);
    n++;

    /* Protocol name */
    if (is_comment) {
        is_sprint = (strcmp((const char *)pd+n, "sprint") == 0);
    }
    for (; pd[n] != '\0'; n++);
    n++;

    /* Variant number (as string) */
    for (; pd[n] != '\0'; n++);
    n++;

    /* Outhdr (as string) */
    for (; pd[n] != '\0'; n++);
    n++;

    /* Direction & encap */
    n += 2;


    /**************************************/
    /* Remainder is encapsulated protocol */
    if (!wtap_dump_file_write(wdh, is_sprint ? " " : "$", 1, err)) {
        return FALSE;
    }

    if (!is_comment) {
        /* Each binary byte is written out as 2 hex string chars */
        for (; n < phdr->len; n++) {
            gchar c[2];
            c[0] = char_from_hex((guint8)(pd[n] >> 4));
            c[1] = char_from_hex((guint8)(pd[n] & 0x0f));

            /* Write both hex chars of byte together */
            if (!wtap_dump_file_write(wdh, c, 2, err)) {
                return FALSE;
            }
        }
    }
    else {
        for (; n < phdr->len; n++) {
            char c[1];
            c[0] = pd[n];

            /* Write both hex chars of byte together */
            if (!wtap_dump_file_write(wdh, c, 1, err)) {
                return FALSE;
            }
        }
    }

    /* End the line */
    if (!wtap_dump_file_write(wdh, "\n", 1, err)) {
        return FALSE;
    }

    return TRUE;
}


/****************************/
/* Private helper functions */
/****************************/

/**********************************************************************/
/* Read a new line from the file, starting at offset.                 */
/* - writes data to its argument linebuff                             */
/* - on return 'offset' will point to the next position to read from  */
/* - return TRUE if this read is successful                           */
/**********************************************************************/
static gboolean
read_new_line(FILE_T fh, gint64 *offset, gint *length,
              gchar *linebuff, size_t linebuffsize, int *err, gchar **err_info)
{
    /* Read in a line */
    gint64 pos_before = file_tell(fh);

    if (file_gets(linebuff, (int)linebuffsize - 1, fh) == NULL) {
        /* No characters found, or error */
        *err = file_error(fh, err_info);
        return FALSE;
    }

    /* Set length (avoiding strlen()) and offset.. */
    *length = (gint)(file_tell(fh) - pos_before);
    *offset = *offset + *length;

    /* ...but don't want to include newline in line length */
    if (*length > 0 && linebuff[*length-1] == '\n') {
        linebuff[*length-1] = '\0';
        *length = *length - 1;
    }
    /* Nor do we want '\r' (as will be written when log is created on windows) */
    if (*length > 0 && linebuff[*length-1] == '\r') {
        linebuff[*length-1] = '\0';
        *length = *length - 1;
    }

    return TRUE;
}


/**********************************************************************/
/* Parse a line from buffer, by identifying:                          */
/* - context, port and direction of packet                            */
/* - timestamp                                                        */
/* - data position and length                                         */
/* Return TRUE if this packet looks valid and can be displayed        */
/**********************************************************************/
static gboolean
parse_line(gchar *linebuff, gint line_length,
           gint *seconds, gint *useconds,
           long *before_time_offset, long *after_time_offset,
           long *data_offset, gint *data_chars,
           packet_direction_t *direction,
           int *encap, int *is_comment, int *is_sprint,
           gchar *aal_header_chars,
           gchar *context_name, guint8 *context_portp,
           gchar *protocol_name, gchar *variant_name,
           gchar *outhdr_name)
{
    int  n = 0;
    int  port_digits;
    char port_number_string[MAX_PORT_DIGITS+1];
    int  variant_digits = 0;
    int  variant = 1;
    int  protocol_chars = 0;
    int  outhdr_chars = 0;

    char seconds_buff[MAX_SECONDS_CHARS+1];
    int  seconds_chars;
    char subsecond_decimals_buff[MAX_SUBSECOND_DECIMALS+1];
    int  subsecond_decimals_chars;
    int  skip_first_byte = FALSE;
    gboolean atm_header_present = FALSE;

    *is_comment = FALSE;
    *is_sprint = FALSE;

    /* Read context name until find '.' */
    for (n=0; (n < MAX_CONTEXT_NAME) && (n+1 < line_length) && (linebuff[n] != '.'); n++) {
        if (linebuff[n] == '/') {
            context_name[n] = '\0';

            /* If not a comment (/////), not a valid line */
            if (strncmp(linebuff+n, "/////", 5) != 0) {
                return FALSE;
            }

            /* There is no variant, outhdr, etc.  Set protocol to be a comment */
            g_strlcpy(protocol_name, "comment", MAX_PROTOCOL_NAME);
            *is_comment = TRUE;
            break;
        }
        if (!g_ascii_isalnum(linebuff[n]) && (linebuff[n] != '_') && (linebuff[n] != '-')) {
            return FALSE;
        }
        context_name[n] = linebuff[n];
    }
    if (n == MAX_CONTEXT_NAME || (n+1 >= line_length)) {
        return FALSE;
    }

    /* Reset strings (that won't be set by comments) */
    variant_name[0] = '\0';
    outhdr_name[0] = '\0';
    port_number_string[0] = '\0';

    if (!(*is_comment)) {
        /* '.' must follow context name */
        if (linebuff[n] != '.') {
            return FALSE;
        }
        context_name[n] = '\0';
        /* Skip it */
        n++;

        /* Now read port number */
        for (port_digits = 0;
             (linebuff[n] != '/') && (port_digits <= MAX_PORT_DIGITS) && (n+1 < line_length);
             n++, port_digits++) {

            if (!g_ascii_isdigit(linebuff[n])) {
                return FALSE;
            }
            port_number_string[port_digits] = linebuff[n];
        }
        if (port_digits > MAX_PORT_DIGITS || (n+1 >= line_length)) {
            return FALSE;
        }

        /* Slash char must follow port number */
        if (linebuff[n] != '/')
        {
            return FALSE;
        }
        port_number_string[port_digits] = '\0';
        if (port_digits == 1) {
            *context_portp = port_number_string[0] - '0';
        }
        else {
            *context_portp = atoi(port_number_string);
        }
        /* Skip it */
        n++;

        /* Now for the protocol name */
        for (protocol_chars = 0;
             (linebuff[n] != '/') && (protocol_chars < MAX_PROTOCOL_NAME) && (n < line_length);
             n++, protocol_chars++) {

            if (!g_ascii_isalnum(linebuff[n]) && linebuff[n] != '_') {
                return FALSE;
            }
            protocol_name[protocol_chars] = linebuff[n];
        }
        if (protocol_chars == MAX_PROTOCOL_NAME || n >= line_length) {
            /* If doesn't fit, fail rather than truncate */
            return FALSE;
        }
        protocol_name[protocol_chars] = '\0';

        /* Slash char must follow protocol name */
        if (linebuff[n] != '/') {
            return FALSE;
        }
        /* Skip it */
        n++;


        /* Following the / is the variant number.  No digits indicate 1 */
        for (variant_digits = 0;
             (g_ascii_isdigit(linebuff[n])) && (variant_digits <= MAX_VARIANT_DIGITS) && (n+1 < line_length);
             n++, variant_digits++) {

            if (!g_ascii_isdigit(linebuff[n])) {
                return FALSE;
            }
            variant_name[variant_digits] = linebuff[n];
        }
        if (variant_digits > MAX_VARIANT_DIGITS || (n+1 >= line_length)) {
            return FALSE;
        }

        if (variant_digits > 0) {
            variant_name[variant_digits] = '\0';
            if (variant_digits == 1) {
                variant = variant_name[0] - '0';
            }
            else {
                variant = atoi(variant_name);
            }
        }
        else {
            variant_name[0] = '1';
            variant_name[1] = '\0';
        }


        /* Outheader values may follow */
        outhdr_name[0] = '\0';
        if (linebuff[n] == ',') {
            /* Skip , */
            n++;

            for (outhdr_chars = 0;
                 (g_ascii_isdigit(linebuff[n]) || linebuff[n] == ',') &&
                 (outhdr_chars <= MAX_OUTHDR_NAME) && (n+1 < line_length);
                 n++, outhdr_chars++) {

                if (!g_ascii_isdigit(linebuff[n]) && (linebuff[n] != ',')) {
                    return FALSE;
                }
                outhdr_name[outhdr_chars] = linebuff[n];
            }
            if (outhdr_chars > MAX_OUTHDR_NAME || (n+1 >= line_length)) {
                return FALSE;
            }
            /* Terminate (possibly empty) string */
            outhdr_name[outhdr_chars] = '\0';
        }
    }


    /******************************************************************/
    /* Now check whether we know how to use a packet of this protocol */

    if ((strcmp(protocol_name, "ip") == 0) ||
        (strcmp(protocol_name, "sctp") == 0) ||
        (strcmp(protocol_name, "gre") == 0) ||
        (strcmp(protocol_name, "mipv6") == 0) ||
        (strcmp(protocol_name, "igmp") == 0)) {

        *encap = WTAP_ENCAP_RAW_IP;
    }
    else

    /* FP may be carried over ATM, which has separate atm header to parse */
    if ((strcmp(protocol_name, "fp") == 0) ||
        (strncmp(protocol_name, "fp_r", 4) == 0)) {

        if ((variant > 256) && (variant % 256 == 3)) {
            /* FP over udp is contained in IPPrim... */
            *encap = 0;
        }
        else {
            /* FP over AAL0 or AAL2 */
            *encap = WTAP_ENCAP_ATM_PDUS_UNTRUNCATED;
            atm_header_present = TRUE;
        }
    }
    else if (strcmp(protocol_name, "fpiur_r5") == 0) {
        /* FP (IuR) over AAL2 */
        *encap = WTAP_ENCAP_ATM_PDUS_UNTRUNCATED;
        atm_header_present = TRUE;
    }

    else
    if (strcmp(protocol_name, "ppp") == 0) {
        *encap = WTAP_ENCAP_PPP;
    }
    else
    if (strcmp(protocol_name, "isdn_l3") == 0) {
       /* TODO: find out what this byte means... */
        skip_first_byte = TRUE;
        *encap = WTAP_ENCAP_ISDN;
    }
    else
    if (strcmp(protocol_name, "isdn_l2") == 0) {
        *encap = WTAP_ENCAP_ISDN;
    }
    else
    if (strcmp(protocol_name, "ethernet") == 0) {
        *encap = WTAP_ENCAP_ETHERNET;
    }
    else
    if ((strcmp(protocol_name, "saalnni_sscop") == 0) ||
        (strcmp(protocol_name, "saaluni_sscop") == 0)) {

        *encap = DCT2000_ENCAP_SSCOP;
    }
    else
    if (strcmp(protocol_name, "frelay_l2") == 0) {
        *encap = WTAP_ENCAP_FRELAY;
    }
    else
    if (strcmp(protocol_name, "ss7_mtp2") == 0) {
        *encap = DCT2000_ENCAP_MTP2;
    }
    else
    if ((strcmp(protocol_name, "nbap") == 0) ||
        (strcmp(protocol_name, "nbap_r4") == 0) ||
        (strncmp(protocol_name, "nbap_sscfuni", strlen("nbap_sscfuni")) == 0)) {

        /* The entire message in these cases is nbap, so use an encap value. */
        *encap = DCT2000_ENCAP_NBAP;
    }
    else {
        /* Not a supported board port protocol/encap, but can show as raw data or
           in some cases find protocol embedded inside primitive */
        *encap = DCT2000_ENCAP_UNHANDLED;
    }


    /* Find separate ATM header if necessary */
    if (atm_header_present) {
        int header_chars_seen = 0;

        /* Scan ahead to the next $ */
        for (; (linebuff[n] != '$') && (n+1 < line_length); n++);
        /* Skip it */
        n++;
        if (n+1 >= line_length) {
            return FALSE;
        }

        /* Read consecutive hex chars into atm header buffer */
        for (;
             ((n < line_length) &&
              (linebuff[n] >= '0') && (linebuff[n] <= '?') &&
              (header_chars_seen < AAL_HEADER_CHARS));
             n++, header_chars_seen++) {

            aal_header_chars[header_chars_seen] = linebuff[n];
            /* Next 6 characters after '9' are mapped to a->f */
            if (!g_ascii_isdigit(linebuff[n])) {
                aal_header_chars[header_chars_seen] = 'a' + (linebuff[n] - '9') -1;
            }
        }

        if (header_chars_seen != AAL_HEADER_CHARS || n >= line_length) {
            return FALSE;
        }
    }

    /* Skip next '/' */
    n++;

    /* If there is a number, skip all info to next '/'.
       TODO: for IP encapsulation, should store PDCP ueid, drb in pseudo info
       and display dct2000 dissector... */
    if (g_ascii_isdigit(linebuff[n])) {
        while ((n+1 < line_length) && linebuff[n] != '/') {
            n++;
        }
    }

    /* Skip '/' */
    while ((n+1 < line_length) && linebuff[n] == '/') {
        n++;
    }

    /* Skip a space that may happen here */
    if ((n+1 < line_length) && linebuff[n] == ' ') {
        n++;
    }

    /* Next character gives direction of message (must be 's' or 'r') */
    if (!(*is_comment)) {
        if (linebuff[n] == 's') {
            *direction = sent;
        }
        else
        if (linebuff[n] == 'r') {
            *direction = received;
        }
        else {
            return FALSE;
        }
        /* Skip it */
        n++;
    }
    else {
        *direction = sent;
    }


    /*********************************************************************/
    /* Find and read the timestamp                                       */

    /* Now scan to the next digit, which should be the start of the timestamp */
    /* This will involve skipping " tm "                                      */

    for (; ((linebuff[n] != 't') || (linebuff[n+1] != 'm')) && (n+1 < line_length); n++);
    if (n >= line_length) {
        return FALSE;
    }

    for (; (n < line_length) && !g_ascii_isdigit(linebuff[n]); n++);
    if (n >= line_length) {
        return FALSE;
    }

    *before_time_offset = n;

    /* Seconds */
    for (seconds_chars = 0;
         (linebuff[n] != '.') &&
         (seconds_chars <= MAX_SECONDS_CHARS) &&
         (n < line_length);
         n++, seconds_chars++) {

        if (!g_ascii_isdigit(linebuff[n])) {
            /* Found a non-digit before decimal point. Fail */
            return FALSE;
        }
        seconds_buff[seconds_chars] = linebuff[n];
    }
    if (seconds_chars > MAX_SECONDS_CHARS || n >= line_length) {
        /* Didn't fit in buffer.  Fail rather than use truncated */
        return FALSE;
    }

    /* Convert found value into number */
    seconds_buff[seconds_chars] = '\0';
    *seconds = atoi(seconds_buff);

    /* The decimal point must follow the last of the seconds digits */
    if (linebuff[n] != '.') {
        return FALSE;
    }
    /* Skip it */
    n++;

    /* Subsecond decimal digits (expect 4-digit accuracy) */
    for (subsecond_decimals_chars = 0;
         (linebuff[n] != ' ') &&
         (subsecond_decimals_chars <= MAX_SUBSECOND_DECIMALS) &&
         (n < line_length);
         n++, subsecond_decimals_chars++) {

        if (!g_ascii_isdigit(linebuff[n])) {
            return FALSE;
        }
        subsecond_decimals_buff[subsecond_decimals_chars] = linebuff[n];
    }
    if (subsecond_decimals_chars > MAX_SUBSECOND_DECIMALS || n >= line_length) {
        /* More numbers than expected - give up */
        return FALSE;
    }
    /* Convert found value into microseconds */
    subsecond_decimals_buff[subsecond_decimals_chars] = '\0';
    *useconds = atoi(subsecond_decimals_buff) * 100;

    /* Space character must follow end of timestamp */
    if (linebuff[n] != ' ') {
        return FALSE;
    }

    *after_time_offset = n++;

    /* If we have a string message, it could either be a comment (with '$') or
       a sprint line (no '$') */
    if (*is_comment) {
        if (strncmp(linebuff+n, "l $", 3) != 0) {
            *is_sprint = TRUE;
            g_strlcpy(protocol_name, "sprint", MAX_PROTOCOL_NAME);
        }
    }

    if (!(*is_sprint)) {
        /* Now skip ahead to find start of data (marked by '$') */
        for (; (linebuff[n] != '$') && (linebuff[n] != '\'') && (n+1 < line_length); n++);
        if ((linebuff[n] == '\'') || (n+1 >= line_length)) {
            return FALSE;
        }
        /* Skip it */
        n++;
    }

    /* Set offset to data start within line */
    *data_offset = n;

    /* Set number of chars that comprise the hex string protocol data */
    *data_chars = line_length - n;

    /* May need to skip first byte (2 hex string chars) */
    if (skip_first_byte) {
        *data_offset += 2;
        *data_chars -= 2;
    }

    return TRUE;
}

/***********************************/
/* Process results of parse_line() */
/***********************************/
static gboolean
process_parsed_line(wtap *wth, dct2000_file_externals_t *file_externals,
                    struct wtap_pkthdr *phdr,
                    Buffer *buf, gint64 file_offset,
                    char *linebuff, long dollar_offset,
                    int seconds, int useconds, gchar *timestamp_string,
                    packet_direction_t direction, int encap,
                    gchar *context_name, guint8 context_port,
                    gchar *protocol_name, gchar *variant_name,
                    gchar *outhdr_name, gchar *aal_header_chars,
                    gboolean is_comment, int data_chars,
                    int *err, gchar **err_info)
{
    int n;
    int stub_offset = 0;
    gsize length;
    guint8 *frame_buffer;

    phdr->rec_type = REC_TYPE_PACKET;
    phdr->presence_flags = WTAP_HAS_TS;

    /* Make sure all packets go to Catapult DCT2000 dissector */
    phdr->pkt_encap = WTAP_ENCAP_CATAPULT_DCT2000;

    /* Fill in timestamp (capture base + packet offset) */
    phdr->ts.secs = file_externals->start_secs + seconds;
    if ((file_externals->start_usecs + useconds) >= 1000000) {
        phdr->ts.secs++;
    }
    phdr->ts.nsecs =
        ((file_externals->start_usecs + useconds) % 1000000) *1000;

    /*
     * Calculate the length of the stub info and the packet data.
     * The packet data length is half bytestring length.
     */
    phdr->caplen = (guint)strlen(context_name)+1 +     /* Context name */
                   1 +                                 /* port */
                   (guint)strlen(timestamp_string)+1 + /* timestamp */
                   (guint)strlen(variant_name)+1 +     /* variant */
                   (guint)strlen(outhdr_name)+1 +      /* outhdr */
                   (guint)strlen(protocol_name)+1 +    /* Protocol name */
                   1 +                                 /* direction */
                   1 +                                 /* encap */
                   (is_comment ? data_chars : (data_chars/2));
    if (phdr->caplen > WTAP_MAX_PACKET_SIZE) {
        /*
         * Probably a corrupt capture file; return an error,
         * so that our caller doesn't blow up trying to allocate
         * space for an immensely-large packet.
         */
        *err = WTAP_ERR_BAD_FILE;
        *err_info = g_strdup_printf("catapult dct2000: File has %u-byte packet, bigger than maximum of %u",
                                    phdr->caplen, WTAP_MAX_PACKET_SIZE);
        return FALSE;
    }
    phdr->len = phdr->caplen;

    /*****************************/
    /* Get the data buffer ready */
    ws_buffer_assure_space(buf, phdr->caplen);
    frame_buffer = ws_buffer_start_ptr(buf);

    /******************************************/
    /* Write the stub info to the data buffer */

    /* Context name */
    length = g_strlcpy((char*)frame_buffer, context_name, MAX_CONTEXT_NAME+1);
    stub_offset += (int)(length + 1);

    /* Context port number */
    frame_buffer[stub_offset] = context_port;
    stub_offset++;

    /* Timestamp within file */
    length = g_strlcpy((char*)&frame_buffer[stub_offset], timestamp_string, MAX_TIMESTAMP_LEN+1);
    stub_offset += (int)(length + 1);

    /* Protocol name */
    length = g_strlcpy((char*)&frame_buffer[stub_offset], protocol_name, MAX_PROTOCOL_NAME+1);
    stub_offset += (int)(length + 1);

    /* Protocol variant number (as string) */
    length = g_strlcpy((gchar*)&frame_buffer[stub_offset], variant_name, MAX_VARIANT_DIGITS+1);
    stub_offset += (int)(length + 1);

    /* Outhdr */
    length = g_strlcpy((char*)&frame_buffer[stub_offset], outhdr_name, MAX_OUTHDR_NAME+1);
    stub_offset += (int)(length + 1);

    /* Direction */
    frame_buffer[stub_offset] = direction;
    stub_offset++;

    /* Encap */
    frame_buffer[stub_offset] = (guint8)encap;
    stub_offset++;

    if (!is_comment) {
        /***********************************************************/
        /* Copy packet data into buffer, converting from ascii hex */
        for (n=0; n < data_chars; n+=2) {
            frame_buffer[stub_offset + n/2] =
                hex_byte_from_chars(linebuff+dollar_offset+n);
        }
    }
    else {
        /***********************************************************/
        /* Copy packet data into buffer, just copying ascii chars  */
        for (n=0; n < data_chars; n++) {
            frame_buffer[stub_offset + n] = linebuff[dollar_offset+n];
        }
    }

    /*****************************************/
    /* Set packet pseudo-header if necessary */
    phdr->pseudo_header.dct2000.seek_off = file_offset;
    phdr->pseudo_header.dct2000.wth = wth;

    switch (encap) {
        case WTAP_ENCAP_ATM_PDUS_UNTRUNCATED:
            set_aal_info(&phdr->pseudo_header, direction, aal_header_chars);
            break;
        case WTAP_ENCAP_ISDN:
            set_isdn_info(&phdr->pseudo_header, direction);
            break;
        case WTAP_ENCAP_PPP:
            set_ppp_info(&phdr->pseudo_header, direction);
            break;

        default:
            /* Other supported types don't need to set anything here... */
            break;
    }

    return TRUE;
}

/*********************************************/
/* Fill in atm pseudo-header with known info */
/*********************************************/
static void
set_aal_info(union wtap_pseudo_header *pseudo_header,
             packet_direction_t direction,
             gchar *aal_header_chars)
{
    /* 'aal_head_chars' has this format (for AAL2 at least):
       Global Flow Control (4 bits) | VPI (8 bits) | VCI (16 bits) |
       Payload Type (4 bits) | Padding (3 bits?) | Link? (1 bit) |
       Channel Identifier (8 bits) | ...
    */

    /* Indicate that this is a reassembled PDU */
    pseudo_header->dct2000.inner_pseudo_header.atm.flags = 0x00;

    /* Channel 0 is DTE->DCE, 1 is DCE->DTE. Always set 0 for now.
       TODO: Can we infer the correct value here?
       Meanwhile, just use the direction to make them distinguishable...
    */
    pseudo_header->dct2000.inner_pseudo_header.atm.channel = (direction == received);

    /* Assume always AAL2 for FP */
    pseudo_header->dct2000.inner_pseudo_header.atm.aal = AAL_2;

    pseudo_header->dct2000.inner_pseudo_header.atm.type = TRAF_UMTS_FP;
    pseudo_header->dct2000.inner_pseudo_header.atm.subtype = TRAF_ST_UNKNOWN;

    /* vpi is 8 bits (2nd & 3rd nibble) */
    pseudo_header->dct2000.inner_pseudo_header.atm.vpi =
        hex_byte_from_chars(aal_header_chars+1);

    /* vci is next 16 bits */
    pseudo_header->dct2000.inner_pseudo_header.atm.vci =
        ((hex_from_char(aal_header_chars[3]) << 12) |
         (hex_from_char(aal_header_chars[4]) << 8) |
         (hex_from_char(aal_header_chars[5]) << 4) |
         hex_from_char(aal_header_chars[6]));

    /* 0 means we don't know how many cells the frame comprises. */
    pseudo_header->dct2000.inner_pseudo_header.atm.cells = 0;

    /* cid is usually last byte.  Unless last char is not hex digit, in which
       case cid is derived from last char in ascii */
    if (g_ascii_isalnum(aal_header_chars[11])) {
        pseudo_header->dct2000.inner_pseudo_header.atm.aal2_cid =
            hex_byte_from_chars(aal_header_chars+10);
    }
    else {
        pseudo_header->dct2000.inner_pseudo_header.atm.aal2_cid =
            (int)aal_header_chars[11] - '0';
    }
}


/**********************************************/
/* Fill in isdn pseudo-header with known info */
/**********************************************/
static void
set_isdn_info(union wtap_pseudo_header *pseudo_header,
              packet_direction_t direction)
{
    /* This field is used to set the 'Source' and 'Destination' columns to
       'User' or 'Network'. If we assume that we're simulating the network,
       treat Received messages as being destined for the network.
    */
    pseudo_header->dct2000.inner_pseudo_header.isdn.uton = (direction == received);

    /* This corresponds to the circuit ID.  0 is treated as LAPD,
       everything else would be treated as a B-channel
    */
    pseudo_header->dct2000.inner_pseudo_header.isdn.channel = 0;
}


/*********************************************/
/* Fill in ppp pseudo-header with known info */
/*********************************************/
static void
set_ppp_info(union wtap_pseudo_header *pseudo_header,
             packet_direction_t direction)
{
    /* Set direction. */
    pseudo_header->dct2000.inner_pseudo_header.p2p.sent = (direction == sent);
}


/********************************************************/
/* Return hex nibble equivalent of hex string character */
/********************************************************/
static guint8
hex_from_char(gchar c)
{
    if ((c >= '0') && (c <= '9')) {
        return c - '0';
    }

    if ((c >= 'a') && (c <= 'f')) {
        return 0x0a + (c - 'a');
    }

    /* Not a valid hex string character */
    return 0xff;
}



/* Table allowing fast lookup from a pair of ascii hex characters to a guint8 */
static guint8 s_tableValues[256][256];

/* Prepare table values so ready so don't need to check inside hex_byte_from_chars() */
static void  prepare_hex_byte_from_chars_table(void)
{
    guchar hex_char_array[16] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
                                  'a', 'b', 'c', 'd', 'e', 'f' };

    gint i, j;
    for (i=0; i < 16; i++) {
        for (j=0; j < 16; j++) {
            s_tableValues[hex_char_array[i]][hex_char_array[j]] = i*16 + j;
        }
    }
}

/* Extract and return a byte value from 2 ascii hex chars, starting from the given pointer */
static guint8 hex_byte_from_chars(gchar *c)
{
    /* Return value from quick table lookup */
    return s_tableValues[(unsigned char)c[0]][(unsigned char)c[1]];
}



/********************************************************/
/* Return character corresponding to hex nibble value   */
/********************************************************/
static gchar
char_from_hex(guint8 hex)
{
    static const char hex_lookup[16] =
    { '0','1','2','3','4','5','6','7','8','9','a','b','c','d','e','f'};

    if (hex > 15) {
        return '?';
    }

    return hex_lookup[hex];
}

/***********************************************/
/* Equality test for packet prefix hash tables */
/***********************************************/
static gint
packet_offset_equal(gconstpointer v, gconstpointer v2)
{
    /* Dereferenced pointers must have same gint64 offset value */
    return (*(const gint64*)v == *(const gint64*)v2);
}


/********************************************/
/* Hash function for packet-prefix hash table */
/********************************************/
static guint
packet_offset_hash_func(gconstpointer v)
{
    /* Use low-order bits of gint64 offset value */
    return (guint)(*(const gint64*)v);
}


/************************************************************************/
/* Parse year, month, day, hour, minute, seconds out of formatted line. */
/* Set secs and usecs as output                                         */
/* Return FALSE if no valid time can be read                            */
/************************************************************************/
static gboolean
get_file_time_stamp(gchar *linebuff, time_t *secs, guint32 *usecs)
{
    struct tm tm;
    #define MAX_MONTH_LETTERS 9
    char month[MAX_MONTH_LETTERS+1];

    int day, year, hour, minute, second;
    int scan_found;

    /* If line longer than expected, file is probably not correctly formatted */
    if (strlen(linebuff) > MAX_TIMESTAMP_LINE_LENGTH) {
        return FALSE;
    }

    /********************************************************/
    /* Scan for all fields                                  */
    scan_found = sscanf(linebuff, "%9s %2d, %4d     %2d:%2d:%2d.%4u",
                        month, &day, &year, &hour, &minute, &second, usecs);
    if (scan_found != 7) {
        /* Give up if not all found */
        return FALSE;
    }

    if      (strcmp(month, "January"  ) == 0)  tm.tm_mon = 0;
    else if (strcmp(month, "February" ) == 0)  tm.tm_mon = 1;
    else if (strcmp(month, "March"    ) == 0)  tm.tm_mon = 2;
    else if (strcmp(month, "April"    ) == 0)  tm.tm_mon = 3;
    else if (strcmp(month, "May"      ) == 0)  tm.tm_mon = 4;
    else if (strcmp(month, "June"     ) == 0)  tm.tm_mon = 5;
    else if (strcmp(month, "July"     ) == 0)  tm.tm_mon = 6;
    else if (strcmp(month, "August"   ) == 0)  tm.tm_mon = 7;
    else if (strcmp(month, "September") == 0)  tm.tm_mon = 8;
    else if (strcmp(month, "October"  ) == 0)  tm.tm_mon = 9;
    else if (strcmp(month, "November" ) == 0)  tm.tm_mon = 10;
    else if (strcmp(month, "December" ) == 0)  tm.tm_mon = 11;
    else {
        /* Give up if not found a properly-formatted date */
        return FALSE;
    }

    /******************************************************/
    /* Fill in remaining fields and return it in a time_t */
    tm.tm_year = year - 1900;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    tm.tm_sec = second;
    tm.tm_isdst = -1;    /* daylight saving time info not known */

    /* Get seconds from this time */
    *secs = mktime(&tm);

    /* Multiply 4 digits given to get micro-seconds */
    *usecs = *usecs * 100;

    return TRUE;
}

/* Free the data allocated inside a line_prefix_info_t */
static gboolean
free_line_prefix_info(gpointer key, gpointer value,
                      gpointer user_data _U_)
{
    line_prefix_info_t *info = (line_prefix_info_t*)value;

    /* Free the 64-bit key value */
    g_free(key);

    /* Free the strings inside */
    g_free(info->before_time);
    if (info->after_time) {
        g_free(info->after_time);
    }

    /* And the structure itself */
    g_free(info);

    /* Item will always be removed from table */
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
