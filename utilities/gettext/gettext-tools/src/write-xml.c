/* Writing XML files.
   Copyright (C) 1995-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Daiki Ueno.  */

#include <config.h>

/* Specification.  */
#include "write-xml.h"

#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>

#include <error.h>
#include "msgl-iconv.h"
#include "xerror-handler.h"
#include "msgl-header.h"
#include "po-charset.h"
#include "read-catalog.h"
#include "read-po.h"
#include "fwriteerror.h"
#include "xalloc.h"
#include "gettext.h"

#define _(str) gettext (str)

int
msgdomain_write_xml_bulk (msgfmt_operand_list_ty *operands,
                          const char *template_file_name,
                          its_rule_list_ty *its_rules,
                          bool replace_text,
                          const char *file_name)
{
  FILE *fp;
  if (strcmp (file_name, "-") == 0)
    fp = stdout;
  else
    {
      fp = fopen (file_name, "wb");
      if (fp == NULL)
        {
          error (0, errno, _("cannot create output file \"%s\""),
                 file_name);
          return 1;
        }
    }

  its_merge_context_ty *context =
    its_merge_context_alloc (its_rules, template_file_name);
  for (size_t i = 0; i < operands->nitems; i++)
    its_merge_context_merge (context,
                             operands->items[i].language,
                             operands->items[i].mlp,
                             replace_text);
  its_merge_context_write (context, fp);
  its_merge_context_free (context);

  /* Make sure nothing went wrong.  */
  if (fwriteerror (fp))
    {
      error (0, errno, _("error while writing \"%s\" file"),
             file_name);
      return 1;
    }

  return 0;
}

int
msgdomain_write_xml (message_list_ty *mlp,
                     const char *canon_encoding,
                     const char *locale_name,
                     const char *template_file_name,
                     its_rule_list_ty *its_rules,
                     bool replace_text,
                     const char *file_name)
{
  /* Convert the messages to Unicode.  */
  iconv_message_list (mlp, canon_encoding, po_charset_utf8, NULL,
                      textmode_xerror_handler);

  /* Support for "reproducible builds": Delete information that may vary
     between builds in the same conditions.  */
  message_list_delete_header_field (mlp, "POT-Creation-Date:");

  /* Create a single-element operands and run the bulk operation on it.  */
  msgfmt_operand_ty operand;
  operand.language = (char *) locale_name;
  operand.mlp = mlp;
  msgfmt_operand_list_ty operands;
  operands.nitems = 1;
  operands.items = &operand;

  return msgdomain_write_xml_bulk (&operands,
                                   template_file_name,
                                   its_rules,
                                   replace_text,
                                   file_name);
}
