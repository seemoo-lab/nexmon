/* Test parts of the API.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

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

#include <gettext-po.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>

static void
xerror (int severity,
        po_message_t message,
        const char *filename, size_t lineno, size_t column,
        int multiline_p, const char *message_text)
{
  fprintf (stderr, "%s\n", message_text);
}

static void
xerror2 (int severity,
         po_message_t message1,
         const char *filename1, size_t lineno1, size_t column1,
         int multiline_p1, const char *message_text1,
         po_message_t message2,
         const char *filename2, size_t lineno2, size_t column2,
         int multiline_p2, const char *message_text2)
{
  fprintf (stderr, "%s...\n", message_text1);
  fprintf (stderr, "...%s\n", message_text2);
}

struct po_xerror_handler handler =
{
  xerror,
  xerror2
};

int
main ()
{
  po_file_t file = po_file_read (SRCDIR "de.po", &handler);
  assert (file != NULL);

  const char * const * domains = po_file_domains (file);
  assert (domains != NULL);
  assert (domains[0] != NULL);
  assert (strcmp (domains[0], "messages") == 0);
  assert (domains[1] == NULL);

  const char *domain = domains[0];

  const char *header = po_file_domain_header (file, domain);
  assert (header != NULL);
  {
    char *field = po_header_field (header, "Language");
    assert (field != NULL);
    assert (strcmp (field, "de") == 0);
    free (field);
  }
  {
    char *field = po_header_field (header, "X-Generator");
    assert (field == NULL);
  }

  {
    po_message_iterator_t iter = po_message_iterator (file, domain);
    int min, max;

    {
      po_message_t message = po_next_message (iter);
      assert (message != NULL);
      assert (strcmp (po_message_msgid (message), "") == 0);
      assert (po_message_msgid_plural (message) == NULL);
      assert (!po_message_is_obsolete (message));
      assert (!po_message_is_fuzzy (message));
      assert (!po_message_is_format (message, "c-format"));
      assert (!po_message_is_format (message, "python-format"));
      assert (!po_message_is_range (message, &min, &max));
      assert (po_message_filepos (message, 0) == NULL);
    }
    {
      po_message_t message = po_next_message (iter);
      assert (message != NULL);
      assert (strcmp (po_message_msgid (message), "found %d fatal error") == 0);
      assert (strcmp (po_message_msgid_plural (message), "found %d fatal errors") == 0);
      assert (!po_message_is_obsolete (message));
      assert (!po_message_is_fuzzy (message));
      assert (po_message_is_format (message, "c-format"));
      assert (!po_message_is_format (message, "python-format"));
      assert (!po_message_is_range (message, &min, &max));
      {
        po_filepos_t pos;
        pos = po_message_filepos (message, 0);
        assert (pos != NULL);
        assert (strcmp (po_filepos_file (pos), "src/msgcmp.c") == 0);
        assert (po_filepos_start_line (pos) == 561);
        pos = po_message_filepos (message, 1);
        assert (pos != NULL);
        assert (strcmp (po_filepos_file (pos), "src/msgfmt.c") == 0);
        assert (po_filepos_start_line (pos) == 799);
        pos = po_message_filepos (message, 2);
        assert (pos != NULL);
        assert (strcmp (po_filepos_file (pos), "src/msgfmt.c") == 0);
        assert (po_filepos_start_line (pos) == 1643);
        pos = po_message_filepos (message, 3);
        assert (pos != NULL);
        assert (strcmp (po_filepos_file (pos), "src/xgettext.c") == 0);
        assert (po_filepos_start_line (pos) == 1111);
        pos = po_message_filepos (message, 4);
        assert (pos == NULL);
      }
    }
    {
      po_message_t message = po_next_message (iter);
      assert (message == NULL);
    }
  }

  po_file_free (file);

  return 0;
}
