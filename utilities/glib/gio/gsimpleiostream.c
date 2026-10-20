/*
 * Copyright © 2014 NICE s.r.l.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Ignacio Casal Quinteiro <ignacio.casal@nice-software.com>
 */


#include "config.h"
#include <glib.h>
#include "glibintl.h"

#include "gsimpleiostream.h"
#include "gtask.h"

/**
 * GSimpleIOStream:
 *
 * `GSimpleIOStream` creates a [class@Gio.IOStream] from an arbitrary
 * [class@Gio.InputStream] and [class@Gio.OutputStream]. This allows any pair of
 * input and output streams to be used with [class@Gio.IOStream] methods.
 *
 * This is useful when you obtained a [class@Gio.InputStream] and a
 * [class@Gio.OutputStream] by other means, for instance creating them with
 * platform specific methods as
 * [`g_unix_input_stream_new()`](../gio-unix/ctor.UnixInputStream.new.html)
 * (from `gio-unix-2.0.pc` / `GioUnix-2.0`), and you want to
 * take advantage of the methods provided by [class@Gio.IOStream].
 *
 * Since: 2.44
 */
struct _GSimpleIOStream
{
  GIOStream parent;

  GInputStream *input_stream;
  GOutputStream *output_stream;
};

struct _GSimpleIOStreamClass
{
  GIOStreamClass parent;
};

typedef struct _GSimpleIOStreamClass GSimpleIOStreamClass;

enum
{
  PROP_0,
  PROP_INPUT_STREAM,
  PROP_OUTPUT_STREAM
};

G_DEFINE_TYPE (GSimpleIOStream, g_simple_io_stream, G_TYPE_IO_STREAM)

static void
g_simple_io_stream_finalize (GObject *object)
{
  GSimpleIOStream *stream = G_SIMPLE_IO_STREAM (object);

  if (stream->input_stream)
    g_object_unref (stream->input_stream);

  if (stream->output_stream)
    g_object_unref (stream->output_stream);

  G_OBJECT_CLASS (g_simple_io_stream_parent_class)->finalize (object);
}

static void
g_simple_io_stream_set_property (GObject      *object,
                                 guint         prop_id,
                                 const GValue *value,
                                 GParamSpec   *pspec)
{
  GSimpleIOStream *stream = G_SIMPLE_IO_STREAM (object);

  switch (prop_id)
    {
    case PROP_INPUT_STREAM:
      stream->input_stream = g_value_dup_object (value);
      break;

    case PROP_OUTPUT_STREAM:
      stream->output_stream = g_value_dup_object (value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static void
g_simple_io_stream_get_property (GObject    *object,
                                 guint       prop_id,
                                 GValue     *value,
                                 GParamSpec *pspec)
{
  GSimpleIOStream *stream = G_SIMPLE_IO_STREAM (object);

  switch (prop_id)
    {
    case PROP_INPUT_STREAM:
      g_value_set_object (value, stream->input_stream);
      break;

    case PROP_OUTPUT_STREAM:
      g_value_set_object (value, stream->output_stream);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
      break;
    }
}

static GInputStream *
g_simple_io_stream_get_input_stream (GIOStream *stream)
{
  GSimpleIOStream *simple_stream = G_SIMPLE_IO_STREAM (stream);

  return simple_stream->input_stream;
}

static GOutputStream *
g_simple_io_stream_get_output_stream (GIOStream *stream)
{
  GSimpleIOStream *simple_stream = G_SIMPLE_IO_STREAM (stream);

  return simple_stream->output_stream;
}

static void
g_simple_io_stream_class_init (GSimpleIOStreamClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  GIOStreamClass *io_class = G_IO_STREAM_CLASS (class);

  gobject_class->finalize = g_simple_io_stream_finalize;
  gobject_class->get_property = g_simple_io_stream_get_property;
  gobject_class->set_property = g_simple_io_stream_set_property;

  io_class->get_input_stream = g_simple_io_stream_get_input_stream;
  io_class->get_output_stream = g_simple_io_stream_get_output_stream;

  /**
   * GSimpleIOStream:input-stream:
   *
   * The [class@Gio.InputStream] to read from.
   *
   * Since: 2.44
   */
  g_object_class_install_property (gobject_class, PROP_INPUT_STREAM,
                                   g_param_spec_object ("input-stream", NULL, NULL,
                                                        G_TYPE_INPUT_STREAM,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_CONSTRUCT_ONLY));

  /**
   * GSimpleIOStream:output-stream:
   *
   * The [class@Gio.OutputStream] to write to.
   *
   * Since: 2.44
   */
  g_object_class_install_property (gobject_class, PROP_OUTPUT_STREAM,
                                   g_param_spec_object ("output-stream", NULL, NULL,
                                                        G_TYPE_OUTPUT_STREAM,
                                                        G_PARAM_READWRITE |
                                                        G_PARAM_STATIC_STRINGS |
                                                        G_PARAM_CONSTRUCT_ONLY));
}

static void
g_simple_io_stream_init (GSimpleIOStream *stream)
{
}

/**
 * g_simple_io_stream_new:
 * @input_stream: a #GInputStream.
 * @output_stream: a #GOutputStream.
 *
 * Creates a new #GSimpleIOStream wrapping @input_stream and @output_stream.
 * See also #GIOStream.
 *
 * Returns: a new #GSimpleIOStream instance.
 *
 * Since: 2.44
 */
GIOStream *
g_simple_io_stream_new (GInputStream  *input_stream,
                        GOutputStream *output_stream)
{
  return g_object_new (G_TYPE_SIMPLE_IO_STREAM,
                       "input-stream", input_stream,
                       "output-stream", output_stream,
                       NULL);
}
