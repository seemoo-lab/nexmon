/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
 * Copyright (C) 2008 Novell, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 * Author: Tor Lillqvist <tml@novell.com>
 */

#ifndef __G_WINHTTP_FILE_OUTPUT_STREAM_H__
#define __G_WINHTTP_FILE_OUTPUT_STREAM_H__

#include <gio/gfileoutputstream.h>

#include "gwinhttpfile.h"

G_BEGIN_DECLS

#define G_TYPE_WINHTTP_FILE_OUTPUT_STREAM         (_g_winhttp_file_output_stream_get_type ())
#define G_WINHTTP_FILE_OUTPUT_STREAM(o)           (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_WINHTTP_FILE_OUTPUT_STREAM, GWinHttpFileOutputStream))
#define G_WINHTTP_FILE_OUTPUT_STREAM_CLASS(k)     (G_TYPE_CHECK_CLASS_CAST((k), G_TYPE_WINHTTP_FILE_OUTPUT_STREAM, GWinHttpFileOutputStreamClass))
#define G_IS_WINHTTP_FILE_OUTPUT_STREAM(o)        (G_TYPE_CHECK_INSTANCE_TYPE ((o), G_TYPE_WINHTTP_FILE_OUTPUT_STREAM))
#define G_IS_WINHTTP_FILE_OUTPUT_STREAM_CLASS(k)  (G_TYPE_CHECK_CLASS_TYPE ((k), G_TYPE_WINHTTP_FILE_OUTPUT_STREAM))
#define G_WINHTTP_FILE_OUTPUT_STREAM_GET_CLASS(o) (G_TYPE_INSTANCE_GET_CLASS ((o), G_TYPE_WINHTTP_FILE_OUTPUT_STREAM, GWinHttpFileOutputStreamClass))

typedef struct _GWinHttpFileOutputStream         GWinHttpFileOutputStream;
typedef struct _GWinHttpFileOutputStreamClass    GWinHttpFileOutputStreamClass;

GType _g_winhttp_file_output_stream_get_type (void) G_GNUC_CONST;

GFileOutputStream *_g_winhttp_file_output_stream_new (GWinHttpFile *file,
                                                      HINTERNET     connection);

G_END_DECLS

#endif /* __G_WINHTTP_FILE_OUTPUT_STREAM_H__ */
