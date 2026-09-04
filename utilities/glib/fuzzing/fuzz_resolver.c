/*
 * Copyright 2021 Endless OS Foundation, LLC
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

#include "fuzz.h"
#include "gio/gnetworking.h"

#include "../gio/gthreadedresolver.h"
#include "../gio/gthreadedresolver-private.h"

static void
test_for_rrtype (const guint8 *data,
                 gsize         data_len,
                 gint          rrtype)
{
  /* g_resolver_records_from_res_query() is only available on Unix */
#ifdef G_OS_UNIX
  GList *record_list = NULL;

  /* Data too long? */
  if (data_len > G_MAXSSIZE)
    return;

  /* rrname is only used in error messages, so doesn’t need to vary.
   * herr is used similarly, so is just set to zero. */
  record_list = g_resolver_records_from_res_query ("rrname",
                                                   rrtype,
                                                   data,
                                                   data_len,
                                                   0,
                                                   NULL);

  g_list_free_full (record_list, (GDestroyNotify) g_variant_unref);
#endif  /* G_OS_UNIX */
}

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  const gint rrtypes_to_test[] =
    {
      /* See https://en.wikipedia.org/wiki/List_of_DNS_record_types */
      33  /* SRV */,
      15  /* MX */,
      6  /* SOA */,
      2  /* NS */,
      16  /* TXT */,
      999,  /* not currently a valid rrtype, to test the ‘unknown’ code path */
    };
  gsize i;

  fuzz_set_logging_func ();

  for (i = 0; i < G_N_ELEMENTS (rrtypes_to_test); i++)
    test_for_rrtype (data, size, rrtypes_to_test[i]);

  return 0;
}
