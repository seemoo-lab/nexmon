/*
 * Copyright 2018 pdknsk
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
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#include "gio/gio.h"
#include "glib/glib.h"

int LLVMFuzzerTestOneInput (const unsigned char *data, size_t size);

#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
static GLogWriterOutput
empty_logging_func (GLogLevelFlags log_level, const GLogField *fields,
                    gsize n_fields, gpointer user_data)
{
  return G_LOG_WRITER_HANDLED;
}
#endif

/* Disables logging for oss-fuzz. Must be used with each target. */
static void
fuzz_set_logging_func (void)
{
#ifdef FUZZING_BUILD_MODE_UNSAFE_FOR_PRODUCTION
  static gboolean writer_set = FALSE;

  if (!writer_set)
    {
      g_log_set_writer_func (empty_logging_func, NULL, NULL);
      writer_set = TRUE;
    }
#endif
}
