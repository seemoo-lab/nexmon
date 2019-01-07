/* ringbuffer.h
 * Definitions for capture ringbuffer files
 *
 * $Id: ringbuffer.h 33978 2010-08-28 11:05:51Z sake $
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

#ifndef __RINGBUFFER_H__
#define __RINGBUFFER_H__

#include <stdio.h>
#include "file.h"
#include "wiretap/wtap.h"

#define RINGBUFFER_UNLIMITED_FILES 0
/* Minimum number of ringbuffer files */
#define RINGBUFFER_MIN_NUM_FILES 0
/* Maximum number of ringbuffer files */
/* Avoid crashes on very large numbers. Should be a power of 10 */
#define RINGBUFFER_MAX_NUM_FILES 100000
/* Maximum number for FAT filesystems */
#define RINGBUFFER_WARN_NUM_FILES 65535

int ringbuf_init(const char *capture_name, guint num_files, gboolean group_read_access);
const gchar *ringbuf_current_filename(void);
FILE *ringbuf_init_libpcap_fdopen(int *err);
gboolean ringbuf_switch_file(FILE **pdh, gchar **save_file, int *save_file_fd,
                             int *err);
gboolean ringbuf_libpcap_dump_close(gchar **save_file, int *err);
void ringbuf_free(void);
void ringbuf_error_cleanup(void);

#endif /* ringbuffer.h */
