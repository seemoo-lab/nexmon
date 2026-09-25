/* GLib testing framework examples and tests
 * Copyright (C) 2024 Red Hat, Inc.
 * Authors: Matthias Clasen
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
 */

#include <glib/glib.h>
#include <gio/gio.h>
#include <stdlib.h>
#include <string.h>

static void
test_extra_bytes_at_end (void)
{
  char data[1024];
  gsize size;
  GBytes *bytes;
  GConverter *converter;
  GError *error = NULL;
  GBytes *result;

  /* Create some simple data to encode */
  data[0] = 0;
  bytes = g_bytes_new_static (data, 1);

  /* encode the data */
  converter = G_CONVERTER (g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP, 9));
  result = g_converter_convert_bytes (converter, bytes, &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);
  g_bytes_unref (bytes);
  g_clear_object (&converter);

  /* Append a 0 byte to the encoded data */
  size = g_bytes_get_size (result);
  g_assert_cmpuint (size, <, G_N_ELEMENTS (data)); /* just to be very sure */
  memcpy (data, g_bytes_get_data (result, NULL), size);
  data[size] = 0;
  bytes = g_bytes_new_static (data, size + 1);
  g_bytes_unref (result);

  /* Decompress the just compressed bytes with the extra 0 */
  converter = G_CONVERTER (g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP));
  result = g_converter_convert_bytes (converter, bytes, &error);
  g_assert_error (error, G_IO_ERROR, G_IO_ERROR_MESSAGE_TOO_LARGE);
  g_assert_null (result);
  g_clear_error (&error);

  g_object_unref (converter);
  g_bytes_unref (bytes);
}

static void
test_convert_bytes (void)
{
  char data[8192];
  GBytes *bytes;
  GConverter *converter;
  GError *error = NULL;
  GBytes *result;

  for (gsize i = 0; i < sizeof (data); i++)
    data[i] = g_test_rand_int_range (0, 256);

  bytes = g_bytes_new_static (data, sizeof (data));

  converter = G_CONVERTER (g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP, 9));
  result = g_converter_convert_bytes (converter, bytes, &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);

  g_bytes_unref (result);
  g_object_unref (converter);

  converter = G_CONVERTER (g_zlib_decompressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP));
  result = g_converter_convert_bytes (converter, bytes, &error);
  g_assert_nonnull (error);
  g_error_free (error);
  g_assert_true (result == NULL);

  g_object_unref (converter);
  g_bytes_unref (bytes);
}

static void
test_gzip_os_property (void)
{
  const int os_testvalue = 42;
  const char *data = "Hello World";
  GBytes *bytes;
  GZlibCompressor *compressor;
  GError *error = NULL;
  GBytes *result;
  int os;
  const char *encoded;

  bytes = g_bytes_new_static (data, sizeof (data));

  compressor = g_zlib_compressor_new (G_ZLIB_COMPRESSOR_FORMAT_GZIP, 9);
  g_zlib_compressor_set_os (compressor, os_testvalue);
  g_assert_cmpint (g_zlib_compressor_get_os (compressor), ==, os_testvalue);

  g_object_get (compressor, "os", &os, NULL);
  g_assert_cmpint (os, ==, os_testvalue);

  result = g_converter_convert_bytes (G_CONVERTER (compressor), bytes, &error);
  g_assert_no_error (error);
  g_assert_nonnull (result);

  /* The OS is stored in the 10th byte of the header, see RFC 1952 */
  g_assert_cmpint (g_bytes_get_size (result), >, 10);
  encoded = g_bytes_get_data (result, NULL);
  g_assert_cmpint (encoded[9], ==, os_testvalue);

  g_bytes_unref (result);
  g_object_unref (compressor);
  g_bytes_unref (bytes);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/converter/bytes", test_convert_bytes);
  g_test_add_func ("/converter/extra-bytes-at-end", test_extra_bytes_at_end);
  g_test_add_func ("/converter/gzip-os-property", test_gzip_os_property);

  return g_test_run();
}
