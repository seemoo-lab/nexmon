/* Reading textual message catalogs (such as PO files).
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

/* Written by Peter Miller, Ulrich Drepper, and Bruno Haible.  */

#include <config.h>

/* Specification.  */
#include "read-catalog.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "po-charset.h"
#include "read-po-lex.h"
#include "xerror-handler.h"
#include "xalloc.h"
#include "read-catalog-special.h"
#include "gettext.h"

#define _(str) gettext (str)


/* ========================================================================= */
/* Inline functions to invoke the methods.  */

static inline void
call_set_domain (struct default_catalog_reader_ty *dcatr,
                 char *name, lex_pos_ty *name_pos)
{
  default_catalog_reader_class_ty *methods =
    (default_catalog_reader_class_ty *) dcatr->methods;

  if (methods->set_domain)
    methods->set_domain (dcatr, name, name_pos);
}

static inline void
call_add_message (struct default_catalog_reader_ty *dcatr,
                  char *msgctxt,
                  char *msgid, lex_pos_ty *msgid_pos, char *msgid_plural,
                  char *msgstr, size_t msgstr_len, lex_pos_ty *msgstr_pos,
                  char *prev_msgctxt, char *prev_msgid, char *prev_msgid_plural,
                  bool force_fuzzy, bool obsolete)
{
  default_catalog_reader_class_ty *methods =
    (default_catalog_reader_class_ty *) dcatr->methods;

  if (methods->add_message)
    methods->add_message (dcatr, msgctxt,
                          msgid, msgid_pos, msgid_plural,
                          msgstr, msgstr_len, msgstr_pos,
                          prev_msgctxt, prev_msgid, prev_msgid_plural,
                          force_fuzzy, obsolete);
}

static inline void
call_frob_new_message (struct default_catalog_reader_ty *dcatr, message_ty *mp,
                       const lex_pos_ty *msgid_pos,
                       const lex_pos_ty *msgstr_pos)
{
  default_catalog_reader_class_ty *methods =
    (default_catalog_reader_class_ty *) dcatr->methods;

  if (methods->frob_new_message)
    methods->frob_new_message (dcatr, mp, msgid_pos, msgstr_pos);
}


/* ========================================================================= */
/* Implementation of default_catalog_reader_ty's methods.  */


/* Implementation of methods declared in the superclass.  */


/* Prepare for first message.  */
void
default_constructor (abstract_catalog_reader_ty *catr)
{
  default_catalog_reader_ty *dcatr = (default_catalog_reader_ty *) catr;

  dcatr->domain = MESSAGE_DOMAIN_DEFAULT;
  dcatr->comment = NULL;
  dcatr->comment_dot = NULL;
  dcatr->filepos_count = 0;
  dcatr->filepos = NULL;
  dcatr->is_fuzzy = false;
  for (size_t i = 0; i < NFORMATS; i++)
    dcatr->is_format[i] = undecided;
  dcatr->range.min = -1;
  dcatr->range.max = -1;
  dcatr->do_wrap = undecided;
}


void
default_destructor (abstract_catalog_reader_ty *catr)
{
  default_catalog_reader_ty *dcatr = (default_catalog_reader_ty *) catr;

  /* Do not free dcatr->mdlp and dcatr->mlp.  */
  if (dcatr->handle_comments)
    {
      if (dcatr->comment != NULL)
        string_list_free (dcatr->comment);
      if (dcatr->comment_dot != NULL)
        string_list_free (dcatr->comment_dot);
    }

  for (size_t j = 0; j < dcatr->filepos_count; ++j)
    free ((char *) dcatr->filepos[j].file_name);
  if (dcatr->filepos != NULL)
    free (dcatr->filepos);
}


void
default_parse_brief (abstract_catalog_reader_ty *catr)
{
  /* We need to parse comments, because even if dcatr->handle_comments
     is false, we need to know which messages are fuzzy.  */
  catr->pass_comments = true;
}


void
default_parse_debrief (abstract_catalog_reader_ty *catr)
{
}


/* Add the accumulated comments to the message.  */
static void
default_copy_comment_state (default_catalog_reader_ty *dcatr, message_ty *mp)
{
  if (dcatr->handle_comments)
    {
      if (dcatr->comment != NULL)
        for (size_t j = 0; j < dcatr->comment->nitems; ++j)
          message_comment_append (mp, dcatr->comment->item[j]);
      if (dcatr->comment_dot != NULL)
        for (size_t j = 0; j < dcatr->comment_dot->nitems; ++j)
          message_comment_dot_append (mp, dcatr->comment_dot->item[j]);
    }
  for (size_t j = 0; j < dcatr->filepos_count; ++j)
    {
      lex_pos_ty *pp = &dcatr->filepos[j];

      message_comment_filepos (mp, pp->file_name, pp->line_number);
    }
  mp->is_fuzzy = dcatr->is_fuzzy;
  for (size_t i = 0; i < NFORMATS; i++)
    mp->is_format[i] = dcatr->is_format[i];
  mp->range = dcatr->range;
  mp->do_wrap = dcatr->do_wrap;
}


static void
default_reset_comment_state (default_catalog_reader_ty *dcatr)
{
  if (dcatr->handle_comments)
    {
      if (dcatr->comment != NULL)
        {
          string_list_free (dcatr->comment);
          dcatr->comment = NULL;
        }
      if (dcatr->comment_dot != NULL)
        {
          string_list_free (dcatr->comment_dot);
          dcatr->comment_dot = NULL;
        }
    }
  for (size_t j = 0; j < dcatr->filepos_count; ++j)
    free ((char *) dcatr->filepos[j].file_name);
  if (dcatr->filepos != NULL)
    free (dcatr->filepos);
  dcatr->filepos_count = 0;
  dcatr->filepos = NULL;
  dcatr->is_fuzzy = false;
  for (size_t i = 0; i < NFORMATS; i++)
    dcatr->is_format[i] = undecided;
  dcatr->range.min = -1;
  dcatr->range.max = -1;
  dcatr->do_wrap = undecided;
}


/* Process 'domain' directive from .po file.  */
void
default_directive_domain (abstract_catalog_reader_ty *catr,
                          char *name, lex_pos_ty *name_pos)
{
  default_catalog_reader_ty *dcatr = (default_catalog_reader_ty *) catr;

  call_set_domain (dcatr, name, name_pos);

  /* If there are accumulated comments, throw them away, they are
     probably part of the file header, or about the domain directive,
     and will be unrelated to the next message.  */
  default_reset_comment_state (dcatr);
}


/* Process ['msgctxt'/]'msgid'/'msgstr' pair from .po file.  */
void
default_directive_message (abstract_catalog_reader_ty *catr,
                           char *msgctxt,
                           char *msgid,
                           lex_pos_ty *msgid_pos,
                           char *msgid_plural,
                           char *msgstr, size_t msgstr_len,
                           lex_pos_ty *msgstr_pos,
                           char *prev_msgctxt,
                           char *prev_msgid, char *prev_msgid_plural,
                           bool force_fuzzy, bool obsolete)
{
  default_catalog_reader_ty *dcatr = (default_catalog_reader_ty *) catr;

  call_add_message (dcatr, msgctxt, msgid, msgid_pos, msgid_plural,
                    msgstr, msgstr_len, msgstr_pos,
                    prev_msgctxt, prev_msgid, prev_msgid_plural,
                    force_fuzzy, obsolete);

  /* Prepare for next message.  */
  default_reset_comment_state (dcatr);
}


void
default_comment (abstract_catalog_reader_ty *catr, const char *s)
{
  default_catalog_reader_ty *dcatr = (default_catalog_reader_ty *) catr;

  if (dcatr->handle_comments)
    {
      if (dcatr->comment == NULL)
        dcatr->comment = string_list_alloc ();
      string_list_append (dcatr->comment, s);
    }
}


void
default_comment_dot (abstract_catalog_reader_ty *catr, const char *s)
{
  default_catalog_reader_ty *dcatr = (default_catalog_reader_ty *) catr;

  if (dcatr->handle_comments)
    {
      if (dcatr->comment_dot == NULL)
        dcatr->comment_dot = string_list_alloc ();
      string_list_append (dcatr->comment_dot, s);
    }
}


void
default_comment_filepos (abstract_catalog_reader_ty *catr,
                         const char *file_name, size_t line_number)
{
  default_catalog_reader_ty *dcatr = (default_catalog_reader_ty *) catr;

  size_t nbytes = (dcatr->filepos_count + 1) * sizeof (dcatr->filepos[0]);
  dcatr->filepos = xrealloc (dcatr->filepos, nbytes);

  lex_pos_ty *pp = &dcatr->filepos[dcatr->filepos_count++];
  pp->file_name = xstrdup (file_name);
  pp->line_number = line_number;
}


/* Test for '#, fuzzy' or '#= fuzzy' comments and warn.  */
void
default_comment_special (abstract_catalog_reader_ty *catr, const char *s)
{
  default_catalog_reader_ty *dcatr = (default_catalog_reader_ty *) catr;

  bool tmp_fuzzy;
  enum is_format tmp_format[NFORMATS];
  struct argument_range tmp_range;
  enum is_wrap tmp_wrap;
  parse_comment_special (s, &tmp_fuzzy, tmp_format, &tmp_range, &tmp_wrap,
                         NULL);

  if (tmp_fuzzy)
    dcatr->is_fuzzy = true;
  for (size_t i = 0; i < NFORMATS; i++)
    if (tmp_format[i] != undecided)
      dcatr->is_format[i] = tmp_format[i];
  if (has_range_p (tmp_range))
    {
      if (has_range_p (dcatr->range))
        {
          if (tmp_range.min < dcatr->range.min)
            dcatr->range.min = tmp_range.min;
          if (tmp_range.max > dcatr->range.max)
            dcatr->range.max = tmp_range.max;
        }
      else
        dcatr->range = tmp_range;
    }
  if (tmp_wrap != undecided)
    dcatr->do_wrap = tmp_wrap;
}


/* Default implementation of methods not inherited from the superclass.  */


void
default_set_domain (default_catalog_reader_ty *dcatr,
                    char *name, lex_pos_ty *name_pos)
{
  if (dcatr->allow_domain_directives)
    /* Override current domain name.  Don't free memory.  */
    dcatr->domain = name;
  else
    {
      dcatr->xeh->xerror (CAT_SEVERITY_ERROR, NULL,
                          name_pos->file_name, name_pos->line_number, (size_t)(-1),
                          false,
                          _("this file may not contain domain directives"));

      /* NAME was allocated in read-po-gram.y but is not used anywhere.  */
      free (name);
    }
}

void
default_add_message (default_catalog_reader_ty *dcatr,
                     char *msgctxt,
                     char *msgid,
                     lex_pos_ty *msgid_pos,
                     char *msgid_plural,
                     char *msgstr, size_t msgstr_len,
                     lex_pos_ty *msgstr_pos,
                     char *prev_msgctxt,
                     char *prev_msgid,
                     char *prev_msgid_plural,
                     bool force_fuzzy, bool obsolete)
{
  if (dcatr->mdlp != NULL)
    /* Select the appropriate sublist of dcatr->mdlp.  */
    dcatr->mlp = msgdomain_list_sublist (dcatr->mdlp, dcatr->domain, true);

  message_ty *mp;
  if (dcatr->allow_duplicates && msgid[0] != '\0')
    /* Doesn't matter if this message ID has been seen before.  */
    mp = NULL;
  else
    /* See if this message ID has been seen before.  */
    mp = message_list_search (dcatr->mlp, msgctxt, msgid);

  if (mp)
    {
      if (!(dcatr->allow_duplicates_if_same_msgstr
            && msgstr_len == mp->msgstr_len
            && memcmp (msgstr, mp->msgstr, msgstr_len) == 0))
        {
          /* We give a fatal error about this, regardless whether the
             translations are equal or different.  This is for consistency
             with msgmerge, msgcat and others.  The user can use the
             msguniq program to get rid of duplicates.  */
          dcatr->xeh->xerror2 (CAT_SEVERITY_ERROR,
                               NULL,
                               msgid_pos->file_name, msgid_pos->line_number, (size_t)(-1),
                               false,
                               _("duplicate message definition"),
                               mp,
                               NULL, 0, 0,
                               false,
                               _("this is the location of the first definition"));
        }
      /* We don't need the just constructed entries' parameter string
         (allocated in read-po-gram.y).  */
      free (msgid);
      if (msgid_plural != NULL)
        free (msgid_plural);
      free (msgstr);
      if (msgctxt != NULL)
        free (msgctxt);
      if (prev_msgctxt != NULL)
        free (prev_msgctxt);
      if (prev_msgid != NULL)
        free (prev_msgid);
      if (prev_msgid_plural != NULL)
        free (prev_msgid_plural);

      /* Add the accumulated comments to the message.  */
      default_copy_comment_state (dcatr, mp);
    }
  else
    {
      /* Construct message to add to the list.
         Obsolete message go into the list at least for duplicate checking.
         It's the caller's responsibility to ignore obsolete messages when
         appropriate.  */
      mp = message_alloc (msgctxt, msgid, msgid_plural, msgstr, msgstr_len,
                          msgstr_pos);
      if (msgid_plural != NULL)
        free (msgid_plural);
      mp->prev_msgctxt = prev_msgctxt;
      mp->prev_msgid = prev_msgid;
      mp->prev_msgid_plural = prev_msgid_plural;
      mp->obsolete = obsolete;
      default_copy_comment_state (dcatr, mp);
      if (force_fuzzy)
        mp->is_fuzzy = true;

      call_frob_new_message (dcatr, mp, msgid_pos, msgstr_pos);

      message_list_append (dcatr->mlp, mp);
    }
}


/* So that the one parser can be used for multiple programs, and also
   use good data hiding and encapsulation practices, an object
   oriented approach has been taken.  An object instance is allocated,
   and all actions resulting from the parse will be through
   invocations of method functions of that object.  */

static default_catalog_reader_class_ty default_methods =
{
  {
    sizeof (default_catalog_reader_ty),
    default_constructor,
    default_destructor,
    default_parse_brief,
    default_parse_debrief,
    default_directive_domain,
    default_directive_message,
    default_comment,
    default_comment_dot,
    default_comment_filepos,
    default_comment_special
  },
  default_set_domain, /* set_domain */
  default_add_message, /* add_message */
  NULL /* frob_new_message */
};


default_catalog_reader_ty *
default_catalog_reader_alloc (default_catalog_reader_class_ty *method_table,
                              xerror_handler_ty xerror_handler)
{
  return
    (default_catalog_reader_ty *)
    catalog_reader_alloc (&method_table->super, xerror_handler);
}


/* ========================================================================= */
/* Exported functions.  */


/* If false, duplicate msgids in the same domain and file generate an error.
   If true, such msgids are allowed; the caller should treat them
   appropriately.  Defaults to false.  */
bool allow_duplicates = false;


msgdomain_list_ty *
read_catalog_stream (FILE *fp, const char *real_filename,
                     const char *logical_filename,
                     catalog_input_format_ty input_syntax,
                     xerror_handler_ty xerror_handler,
                     string_list_ty *arena)
{
  default_catalog_reader_ty *dcatr =
    default_catalog_reader_alloc (&default_methods, xerror_handler);
  dcatr->pass_obsolete_entries = true;
  dcatr->handle_comments = true;
  dcatr->allow_domain_directives = true;
  dcatr->allow_duplicates = allow_duplicates;
  dcatr->allow_duplicates_if_same_msgstr = false;
  dcatr->file_name = real_filename;
  dcatr->mdlp = msgdomain_list_alloc (!dcatr->allow_duplicates);
  dcatr->mlp = msgdomain_list_sublist (dcatr->mdlp, dcatr->domain, true);
  if (input_syntax->produces_utf8)
    /* We know a priori that input_syntax->parse convert strings to UTF-8.  */
    dcatr->mdlp->encoding = po_charset_utf8;

  catalog_reader_parse ((abstract_catalog_reader_ty *) dcatr, fp, real_filename,
                        logical_filename, false, input_syntax, arena);

  msgdomain_list_ty *mdlp = dcatr->mdlp;
  catalog_reader_free ((abstract_catalog_reader_ty *) dcatr);
  return mdlp;
}
