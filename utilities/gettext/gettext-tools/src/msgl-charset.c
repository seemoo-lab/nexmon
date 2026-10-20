/* Message list charset and locale charset handling.
   Copyright (C) 2001-2026 Free Software Foundation, Inc.

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

/* Written by Bruno Haible.  */


#include <config.h>
#include <alloca.h>

/* Specification.  */
#include "msgl-charset.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include <error.h>
#include "msgl-ascii.h"
#include "po-charset.h"
#include "localcharset.h"
#include "progname.h"
#include "basename-lgpl.h"
#include "xmalloca.h"
#include "xerror.h"
#include "xvasprintf.h"
#include "message.h"
#include "c-strstr.h"
#include "gettext.h"

#define _(str) gettext (str)

/* Check whether the POT file's encoding is ASCII or UTF-8.  Otherwise
   emit a warning.
   Rationale: A POT file is routinely copied by a translator to a PO file.
   If a POT file contains non-ASCII messages (or comments) in an encoding
   other than UTF-8, the translator will most likely encounter trouble adding
   her own translations in the same encoding.  A translator should not have
   to convert the POT file to UTF-8 first; instead, the POT file should
   already be prepeared ready-to-use.  */
void
check_pot_charset (const msgdomain_list_ty *mdlp, const char *filename)
{
  for (size_t k = 0; k < mdlp->nitems; k++)
    {
      const message_list_ty *mlp = mdlp->item[k]->messages;

      for (size_t j = 0; j < mlp->nitems; j++)
        if (is_header (mlp->item[j]) && !mlp->item[j]->obsolete)
          {
            const char *header = mlp->item[j]->msgstr;

            if (header != NULL)
              {
                const char *charsetstr = c_strstr (header, "charset=");

                if (charsetstr != NULL)
                  {
                    charsetstr += strlen ("charset=");
                    size_t len = strcspn (charsetstr, " \t\n");

                    char *charset = (char *) xmalloca (len + 1);
                    memcpy (charset, charsetstr, len);
                    charset[len] = '\0';

                    const char *canon_charset = po_charset_canonicalize (charset);

                    /* "CHARSET" is often used as a placeholder, equivalent
                       to "any" or "ASCII".  */
                    if (!(strcmp (charset, "CHARSET") == 0)
                        && canon_charset == NULL)
                      error (EXIT_FAILURE, 0,
                             _("%s: The present charset \"%s\" is not a portable encoding name."),
                             filename, charset);
                    if (!is_ascii_message_list (mlp)
                        && canon_charset != po_charset_utf8)
                      error (EXIT_FAILURE, 0,
                             _("%s: The file contains non-ASCII characters but the present charset \"%s\" is not %s."),
                             filename, charset, "UTF-8");

                    freea (charset);
                  }
              }
          }
    }
}

void
compare_po_locale_charsets (const msgdomain_list_ty *mdlp)
{
  /* Check whether the locale encoding and the PO file's encoding are the
     same.  Otherwise emit a warning.  */
  const char *locale_code = locale_charset ();
  const char *canon_locale_code = po_charset_canonicalize (locale_code);
  bool warned = false;
  for (size_t k = 0; k < mdlp->nitems; k++)
    {
      const message_list_ty *mlp = mdlp->item[k]->messages;

      for (size_t j = 0; j < mlp->nitems; j++)
        if (is_header (mlp->item[j]) && !mlp->item[j]->obsolete)
          {
            const char *header = mlp->item[j]->msgstr;

            if (header != NULL)
              {
                const char *charsetstr = c_strstr (header, "charset=");

                if (charsetstr != NULL)
                  {
                    charsetstr += strlen ("charset=");
                    size_t len = strcspn (charsetstr, " \t\n");

                    char *charset = (char *) xmalloca (len + 1);
                    memcpy (charset, charsetstr, len);
                    charset[len] = '\0';

                    const char *canon_charset = po_charset_canonicalize (charset);
                    if (canon_charset == NULL)
                      error (EXIT_FAILURE, 0,
                             _("present charset \"%s\" is not a portable encoding name"),
                             charset);

                    freea (charset);

                    if (canon_locale_code != canon_charset)
                      {
                        size_t prefix_width =
                          multiline_warning (xasprintf (_("warning: ")),
                                             xasprintf (_("\
Locale charset \"%s\" is different from\n\
input file charset \"%s\".\n\
Output of '%s' might be incorrect.\n\
Possible workarounds are:\n\
"), locale_code, canon_charset, last_component (program_name)));
                        multiline_append (prefix_width,
                                          xasprintf (_("\
- Set LC_ALL to a locale with encoding %s.\n\
"), canon_charset));
                        if (canon_locale_code != NULL)
                          multiline_append (prefix_width,
                                            xasprintf (_("\
- Convert the translation catalog to %s using 'msgconv',\n\
  then apply '%s',\n\
  then convert back to %s using 'msgconv'.\n\
"), canon_locale_code, last_component (program_name), canon_charset));
                        if (strcmp (canon_charset, "UTF-8") != 0
                            && (canon_locale_code == NULL
                                || strcmp (canon_locale_code, "UTF-8") != 0))
                          multiline_append (prefix_width,
                                            xasprintf (_("\
- Set LC_ALL to a locale with encoding %s,\n\
  convert the translation catalog to %s using 'msgconv',\n\
  then apply '%s',\n\
  then convert back to %s using 'msgconv'.\n\
"), "UTF-8", "UTF-8", last_component (program_name), canon_charset));
                        warned = true;
                      }
                  }
              }
          }
      }
  if (canon_locale_code == NULL && !warned)
    multiline_warning (xasprintf (_("warning: ")),
                       xasprintf (_("\
Locale charset \"%s\" is not a portable encoding name.\n\
Output of '%s' might be incorrect.\n\
A possible workaround is to set LC_ALL=C.\n\
"), locale_code, last_component (program_name)));
}
