/* Message list header manipulation.
   Copyright (C) 2007-2026 Free Software Foundation, Inc.

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

/* Specification.  */
#include "msgl-header.h"

#include <string.h>

#include "xalloc.h"

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))


void
header_set_charset (message_ty *header_mp, const char *charsetstr,
                    const char *value)
{
  const char *msgstr = header_mp->msgstr;

  size_t len = strcspn (charsetstr, " \t\n");

  size_t len1 = charsetstr - msgstr;
  size_t len2 = strlen (value);
  size_t len3 = (msgstr + strlen (msgstr)) - (charsetstr + len);

  char *new_msgstr = XNMALLOC (len1 + len2 + len3 + 1, char);
  memcpy (new_msgstr, msgstr, len1);
  memcpy (new_msgstr + len1, value, len2);
  memcpy (new_msgstr + len1 + len2, charsetstr + len, len3 + 1);

  header_mp->msgstr = new_msgstr;
  header_mp->msgstr_len = len1 + len2 + len3 + 1;
}


/* The known fields in their usual order.  */
static const struct
{
  const char *name;
  size_t len;
}
known_fields[] =
  {
    { "Project-Id-Version:", sizeof ("Project-Id-Version:") - 1 },
    { "Report-Msgid-Bugs-To:", sizeof ("Report-Msgid-Bugs-To:") - 1 },
    { "POT-Creation-Date:", sizeof ("POT-Creation-Date:") - 1 },
    { "PO-Revision-Date:", sizeof ("PO-Revision-Date:") - 1 },
    { "Last-Translator:", sizeof ("Last-Translator:") - 1 },
    { "Language-Team:", sizeof ("Language-Team:") - 1 },
    { "Language:", sizeof ("Language:") - 1 },
    { "MIME-Version:", sizeof ("MIME-Version:") - 1 },
    { "Content-Type:", sizeof ("Content-Type:") - 1 },
    { "Content-Transfer-Encoding:", sizeof ("Content-Transfer-Encoding:") - 1 }
  };


void
msgdomain_list_set_header_field (msgdomain_list_ty *mdlp,
                                 const char *field, const char *value)
{
  size_t field_len = strlen (field);

  /* Search the field in known_fields[].  */
  int field_index = -1;
  for (size_t k = 0; k < SIZEOF (known_fields); k++)
    if (strcmp (known_fields[k].name, field) == 0)
      {
        field_index = k;
        break;
      }

  for (size_t i = 0; i < mdlp->nitems; i++)
    {
      message_list_ty *mlp = mdlp->item[i]->messages;

      /* Search the header entry.  */
      for (size_t j = 0; j < mlp->nitems; j++)
        if (is_header (mlp->item[j]) && !mlp->item[j]->obsolete)
          {
            message_ty *mp = mlp->item[j];

            /* Modify the header entry.  */
            const char *header = mp->msgstr;
            char *new_header =
              XNMALLOC (strlen (header) + 1
                        + strlen (field) + 1 + strlen (value) + 1 + 1,
                        char);

            /* Test whether the field already occurs in the header entry.  */
            const char *h;

            for (h = header; *h != '\0'; )
              {
                if (strncmp (h, field, field_len) == 0)
                  break;
                h = strchr (h, '\n');
                if (h == NULL)
                  break;
                h++;
              }
            if (h != NULL && *h != '\0')
              {
                /* Replace the field.  */
                char *p = new_header;
                memcpy (p, header, h - header);
                p += h - header;
                p = stpcpy (p, field);
                p = stpcpy (stpcpy (stpcpy (p, " "), value), "\n");
                h = strchr (h, '\n');
                if (h != NULL)
                  {
                    h++;
                    stpcpy (p, h);
                  }
              }
            else if (field_index < 0)
              {
                /* An unknown field.  Append it at the end.  */
                char *p = new_header;
                p = stpcpy (p, header);
                if (p > new_header && p[-1] != '\n')
                  *p++ = '\n';
                p = stpcpy (p, field);
                stpcpy (stpcpy (stpcpy (p, " "), value), "\n");
              }
            else
              {
                /* Find the appropriate position for inserting the field.  */
                for (h = header; *h != '\0'; )
                  {
                    /* Test whether h starts with a field name whose index is
                       > field_index.  */
                    size_t k;
                    for (k = field_index + 1; k < SIZEOF (known_fields); k++)
                      if (strncmp (h, known_fields[k].name, known_fields[k].len)
                          == 0)
                        break;
                    if (k < SIZEOF (known_fields))
                      break;
                    h = strchr (h, '\n');
                    if (h == NULL)
                      break;
                    h++;
                  }
                if (h != NULL && *h != '\0')
                  {
                    /* Insert the field at position h.  */
                    char *p = new_header;
                    memcpy (p, header, h - header);
                    p += h - header;
                    p = stpcpy (p, field);
                    p = stpcpy (stpcpy (stpcpy (p, " "), value), "\n");
                    stpcpy (p, h);
                  }
                else
                  {
                    /* Append it at the end.  */
                    char *p = new_header;
                    p = stpcpy (p, header);
                    if (p > new_header && p[-1] != '\n')
                      *p++ = '\n';
                    p = stpcpy (p, field);
                    stpcpy (stpcpy (stpcpy (p, " "), value), "\n");
                  }
              }

            mp->msgstr = new_header;
            mp->msgstr_len = strlen (new_header) + 1;
          }
    }
}


void
message_list_delete_header_field (message_list_ty *mlp,
                                  const char *field)
{
  size_t field_len = strlen (field);

  /* Search the header entry.  */
  for (size_t j = 0; j < mlp->nitems; j++)
    if (is_header (mlp->item[j]) && !mlp->item[j]->obsolete)
      {
        message_ty *mp = mlp->item[j];

        /* Modify the header entry.  */
        const char *header = mp->msgstr;

        /* Test whether the field occurs in the header entry.  */
        const char *h;

        for (h = header; *h != '\0'; )
          {
            if (strncmp (h, field, field_len) == 0)
              break;
            h = strchr (h, '\n');
            if (h == NULL)
              break;
            h++;
          }
        if (h != NULL && *h != '\0')
          {
            /* Delete the field.  */
            char *new_header = XCALLOC (strlen (header) + 1, char);

            char *p = new_header;
            memcpy (p, header, h - header);
            p += h - header;
            h = strchr (h, '\n');
            if (h != NULL)
              {
                h++;
                strcpy (p, h);
              }
            else
              *p = '\0';

            mp->msgstr = new_header;
            mp->msgstr_len = strlen (new_header) + 1;
          }
      }
}
