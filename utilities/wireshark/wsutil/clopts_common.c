/* clopts_common.c
 * Handle command-line arguments common to various programs
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <stdlib.h>

#include <wsutil/cmdarg_err.h>

#include <wsutil/clopts_common.h>

int
get_natural_int(const char *string, const char *name)
{
  long number;
  char *p;

  number = strtol(string, &p, 10);
  if (p == string || *p != '\0') {
    cmdarg_err("The specified %s \"%s\" isn't a decimal number", name, string);
    exit(1);
  }
  if (number < 0) {
    cmdarg_err("The specified %s \"%s\" is a negative number", name, string);
    exit(1);
  }
  if (number > INT_MAX) {
    cmdarg_err("The specified %s \"%s\" is too large (greater than %d)",
               name, string, INT_MAX);
    exit(1);
  }
  return (int)number;
}


int
get_positive_int(const char *string, const char *name)
{
  int number;

  number = get_natural_int(string, name);

  if (number == 0) {
    cmdarg_err("The specified %s is zero", name);
    exit(1);
  }

  return number;
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
