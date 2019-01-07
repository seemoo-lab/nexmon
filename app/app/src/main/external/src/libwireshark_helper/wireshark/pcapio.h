/* pcapio.h
 * Declarations of our own routins for writing libpcap files.
 *
 * $Id: pcapio.h 28161 2009-04-26 19:24:36Z tuexen $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * Derived from code in the Wiretap Library
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

/* Returns a FILE * to write to on success, NULL on failure */
extern FILE *
libpcap_fdopen(int fd, int *err);

/* Write the file header to a dump file.
   Returns TRUE on success, FALSE on failure.
   Sets "*err" to an error code, or 0 for a short write, on failure*/
extern gboolean
libpcap_write_file_header(FILE *fp, int linktype, int snaplen, long *bytes_written, int *err);

/* Write a record for a packet to a dump file.
   Returns TRUE on success, FALSE on failure. */
extern gboolean
libpcap_write_packet(FILE *fp, const struct pcap_pkthdr *phdr, const u_char *pd,
    long *bytes_written, int *err);

extern gboolean
libpcap_write_session_header_block(FILE *fp,
                                   char *appname,
                                   long *bytes_written,
                                   int *err);

extern gboolean
libpcap_write_interface_description_block(FILE *fp,
                                          char *name,
                                          char *filter,
                                          int link_type,
                                          int snap_len,
                                          long *bytes_written,
                                          int *err);

extern gboolean
libpcap_write_interface_statistics_block(FILE *fp,
                                         guint32 interface_id,
                                         pcap_t *pd,
                                         long *bytes_written,
                                         int *err);

extern gboolean
libpcap_write_enhanced_packet_block(FILE *fp,
                                    const struct pcap_pkthdr *phdr,
                                    guint32 interface_id,
                                    const u_char *pd,
                                    long *bytes_written,
                                    int *err);

extern gboolean
libpcap_dump_flush(FILE *pd, int *err);

extern gboolean
libpcap_dump_close(FILE *pd, int *err);
