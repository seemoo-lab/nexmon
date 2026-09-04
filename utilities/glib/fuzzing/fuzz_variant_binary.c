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

#include "fuzz.h"

int
LLVMFuzzerTestOneInput (const unsigned char *data, size_t size)
{
  GVariant *variant = NULL, *normal_variant = NULL;

  fuzz_set_logging_func ();

  variant = g_variant_new_from_data (G_VARIANT_TYPE_VARIANT, data, size, FALSE,
                                     NULL, NULL);
  if (variant == NULL)
    return 0;

  normal_variant = g_variant_take_ref (g_variant_get_normal_form (variant));
  g_variant_get_data (variant);

  g_variant_unref (normal_variant);
  g_variant_unref (variant);
  return 0;
}
