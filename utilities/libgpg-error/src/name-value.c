/* name-value.c - Parser and writer for a name-value format.
 * Copyright (C) 2016, 2025 g10 Code GmbH
 *
 * This file is part of Libgpg-error.
 *
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this program; if not, see <https://www.gnu.org/licenses/>.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This file was originally a part of GnuPG.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <stdlib.h>
#include <string.h>

#include "gpgrt-int.h"


struct _gpgrt_name_value_container
{
  struct _gpgrt_name_value_entry *first;
  struct _gpgrt_name_value_entry *last;
  unsigned int wipe_on_free:1;
  unsigned int private_key_mode:1;
  unsigned int section_mode:1;
  unsigned int modified:1;
};


struct _gpgrt_name_value_entry
{
  struct _gpgrt_name_value_entry *prev;
  struct _gpgrt_name_value_entry *next;

  unsigned int wipe_on_free:1;  /* Copied from the container.  */

  /* The length of the NAME (to save calling strlen).  */
  unsigned int namelen:8;

  /* The name.  Comments and blank lines have NAME set to NULL.  */
  char *name;

  /* The value as stored in the file.  We store it when we parse
   * a file so that we can reproduce it.  */
  gpgrt_strlist_t raw_value;

  /* The decoded value.  */
  char *value;
};


/* This macro is a simple ascii only versions of the standard (and
 * locale affected) isspace(3).  This macro does not consider \v and
 * \f as white space not any non-ascii character.  */
#define ascii_isspace(a) ((a)==' ' || (a)=='\n' || (a)=='\r' || (a)=='\t')

/* Other simple macros.  */
#define spacep(p)   (*(p) == ' ' || *(p) == '\t')
#define digitp(p)   (*(p) >= '0' && *(p) <= '9')
#define alphap(p)   ((*(p) >= 'A' && *(p) <= 'Z') \
                     || (*(p) >= 'a' && *(p) <= 'z'))
#define alnump(p)   (alphap (p) || digitp (p))


static GPGRT_INLINE int
ascii_toupper (int c)
{
  if (c >= 'a' && c <= 'z')
    c &= ~0x20;
  return c;
}


static int
ascii_strcasecmp (const char *a, const char *b)
{
  if (a == b)
    return 0;

  for (; *a && *b; a++, b++)
    {
      if (*a != *b && ascii_toupper(*a) != ascii_toupper(*b))
        break;
    }
  return *a == *b? 0 : (ascii_toupper (*a) - ascii_toupper (*b));
}

static int
ascii_memcasecmp (const void *a_arg, const void *b_arg, size_t n )
{
  const char *a = a_arg;
  const char *b = b_arg;

  if (a == b)
    return 0;
  for ( ; n; n--, a++, b++ )
    {
      if( *a != *b  && ascii_toupper (*a) != ascii_toupper (*b) )
        return *a == *b? 0 : (ascii_toupper (*a) - ascii_toupper (*b));
    }
  return 0;
}


/* Return true if NAME of length NAMELEN is the same as STRING.  The
 * comparison is case-insensitive for ascii characters.  */
static int
same_name_p (const char *name, size_t namelen, const char *string)
{
  size_t stringlen = strlen (string);

  if (stringlen && string[stringlen-1] == ':')
    stringlen--;

  if (namelen != stringlen)
    return 0;
  return !ascii_memcasecmp (name, string, namelen);
}



/* Allocate a name value container structure.  */
gpgrt_nvc_t
_gpgrt_nvc_new (unsigned int flags)
{
  gpgrt_nvc_t nvc;

  nvc = xtrycalloc (1, sizeof *nvc);
  if (!nvc)
    return NULL;
  nvc->modified = 1;
  if ((flags & GPGRT_NVC_PRIVKEY))
    {
      nvc->wipe_on_free     = 1;
      nvc->private_key_mode = 1;
    }
  else if ((flags & GPGRT_NVC_WIPE))
    nvc->wipe_on_free     = 1;
  nvc->section_mode = !!(flags & GPGRT_NVC_SECTION);

  return nvc;
}


/* Helper to free an entry.  */
static void
nve_release (gpgrt_nve_t entry, int with_wipe)
{
  if (!entry)
    return;

  xfree (entry->name);
  if (entry->value && with_wipe)
    _gpgrt_wipememory (entry->value, strlen (entry->value));
  xfree (entry->value);
  _gpgrt_strlist_free (entry->raw_value);
  xfree (entry);
}


/* Release a name value container.  */
void
_gpgrt_nvc_release (gpgrt_nvc_t cont)
{
  gpgrt_nve_t e, next;

  if (!cont)
    return;

  for (e = cont->first; e; e = next)
    {
      next = e->next;
      nve_release (e, cont->wipe_on_free);
    }

  xfree (cont);
}


/* Return true if the specified flag is set.  Only one bit should be
 * set in FLAGS.  If the GPGRT_NVC_MODIFIED flag is requested and
 * CLEAR is set, that flag is cleared; note that this modified flag is
 * set for a new container and set with each update.  */
int
_gpgrt_nvc_get_flag (gpgrt_nvc_t cont, unsigned int flags, int clear)
{
  int ret = 0;

  if (!cont)
    ;
  else if ((flags & GPGRT_NVC_MODIFIED))
    {
      ret = cont->modified;
      if (clear)
        cont->modified = 0;
    }
  else if ((flags & GPGRT_NVC_PRIVKEY))
    ret = cont->private_key_mode;
  else if ((flags & GPGRT_NVC_WIPE))
    ret = cont->wipe_on_free;
  else if ((flags & GPGRT_NVC_SECTION))
    ret = cont->section_mode;

  return !!ret;
}



/* Dealing with names and values.  */

/* Check whether the given name is valid.  Valid names start with a
 * letter, optionally end with a colon, and contain only alphanumeric
 * characters and the hyphen.  Returns the length of the name sans the
 * optional colon if NAME is valid; returns 0 if the name is not
 * valid.  The length of the name must be less than 255.  */
static unsigned int
valid_name (const char *name, int sectionmode)
{
  size_t i;
  size_t len, extralen;
  const char *s;

  /* In section mode we skip over the first colon.  We require that
   * after the colon at least one other characters is found.  */
  if (sectionmode && (s=strchr (name, ':')) && s[1] && s[1] != ':')
    {
      extralen = s + 1 - name;
      name = s+1;
    }
  else
    extralen = 0;

  len = strlen (name);
  if (!alphap (name) || !len || len > 255)
    return 0;
  if (name[len-1] == ':') /* The colon is optional.  */
    len--;
  if (!len)
    return 0;

  for (i = 1; i < len; i++)
    if (!alnump (name + i) && name[i] != '-')
      return 0;

  return len + extralen;
}


/* Makes sure that ENTRY has a RAW_VALUE.  */
static gpg_err_code_t
assert_raw_value (gpgrt_nve_t entry)
{
  gpg_err_code_t err = 0;
  size_t len, offset;
#define LINELEN	70
  char buf[LINELEN+3];

  if (entry->raw_value)
    return 0;

  len = strlen (entry->value);
  offset = 0;
  while (len)
    {
      size_t amount, linelen = LINELEN;

      /* On the first line we need to subtract space for the name.  */
      if (entry->raw_value == NULL && strlen (entry->name) < linelen)
	linelen -= strlen (entry->name);

      /* See if the rest of the value fits in this line.  */
      if (len <= linelen)
	amount = len;
      else
	{
	  size_t i;

	  /* Find a suitable space to break on.  */
	  for (i = linelen - 1; linelen - i < 30; i--)
	    if (ascii_isspace (entry->value[offset+i]))
	      break;

	  if (ascii_isspace (entry->value[offset+i]))
	    {
	      /* Found one.  */
	      amount = i;
	    }
	  else
	    {
	      /* Just induce a hard break.  */
	      amount = linelen;
	    }
	}

      snprintf (buf, sizeof buf, " %.*s\n", (int) amount,
		&entry->value[offset]);
      if (!_gpgrt_strlist_add (&entry->raw_value, buf,
                               (GPGRT_STRLIST_APPEND
                                | (entry->wipe_on_free? GPGRT_STRLIST_WIPE:0))))
	{
	  err = _gpg_err_code_from_syserror ();
	  goto leave;
	}

      offset += amount;
      len -= amount;
    }

 leave:
  if (err)
    {
      _gpgrt_strlist_free (entry->raw_value);
      entry->raw_value = NULL;
    }

  return err;
#undef LINELEN
}


/* Computes the length of the value encoded as continuation.  If
 * *SWALLOW_WS is set, all whitespace at the beginning of S is
 * swallowed.  If START is given, a pointer to the beginning of the
 * value is stored there.  */
static size_t
continuation_length (const char *s, int *swallow_ws, const char **start)
{
  size_t len;

  if (*swallow_ws)
    {
      /* The previous line was a blank line and we inserted a newline.
	 Swallow all whitespace at the beginning of this line.  */
      while (ascii_isspace (*s))
	s++;
    }
  else
    {
      /* Iff a continuation starts with more than one space, it
	 encodes a space.  */
      if (ascii_isspace (*s))
	s++;
    }

  /* Strip whitespace at the end.  */
  len = strlen (s);
  while (len > 0 && ascii_isspace (s[len-1]))
    len--;

  if (!len)
    {
      /* Blank lines encode newlines.  */
      len = 1;
      s = "\n";
      *swallow_ws = 1;
    }
  else
    *swallow_ws = 0;

  if (start)
    *start = s;

  return len;
}


/* Makes sure that ENTRY has a VALUE.  */
static gpg_err_code_t
assert_value (gpgrt_nve_t entry)
{
  size_t len;
  int swallow_ws;
  gpgrt_strlist_t s;
  char *p;

  if (entry->value)
    return 0;

  len = 0;
  swallow_ws = 0;
  for (s = entry->raw_value; s; s = s->next)
    len += continuation_length (s->d, &swallow_ws, NULL);

  /* Add one for the terminating zero.  */
  len += 1;

  entry->value = p = xtrymalloc (len);
  if (!entry->value)
    return _gpg_err_code_from_syserror ();

  swallow_ws = 0;
  for (s = entry->raw_value; s; s = s->next)
    {
      const char *start;
      size_t l = continuation_length (s->d, &swallow_ws, &start);

      memcpy (p, start, l);
      p += l;
    }

  *p++ = 0;
  gpgrt_assert (p - entry->value == len);

  return 0;
}


/* Get the name.  */
const char *
_gpgrt_nve_name (gpgrt_nve_t entry)
{
  return entry->name;
}


/* Get the value.  */
const char *
_gpgrt_nve_value (gpgrt_nve_t entry)
{
  if (assert_value (entry))
    return NULL;
  return entry->value;
}



/* Adding and modifying values.  */

/* Add (NAME, VALUE, RAW_VALUE) to CONT.  NAME may be NULL for comments
 * and blank lines.  At least one of VALUE and RAW_VALUE must be
 * given.  If PRESERVE_ORDER is not given, entries with the same name
 * are grouped.  NAME, VALUE and RAW_VALUE is consumed.  */
static gpg_err_code_t
do_nvc_add (gpgrt_nvc_t cont, char *name, char *value,
            gpgrt_strlist_t raw_value, int preserve_order)
{
  gpg_err_code_t err = 0;
  gpgrt_nve_t e;
  unsigned int namelen;

  gpgrt_assert (value || raw_value);

  namelen = name ? valid_name (name, cont->section_mode) : 0;
  if (name && !namelen)
    {
      err = GPG_ERR_INV_NAME;
      goto leave;
    }

  if (name
      && cont->private_key_mode
      && same_name_p (name, namelen, "Key")
      && _gpgrt_nvc_lookup (cont, "Key"))
    {
      err = GPG_ERR_INV_NAME;
      goto leave;
    }

  e = xtrycalloc (1, sizeof *e);
  if (!e)
    {
      err = _gpg_err_code_from_syserror ();
      goto leave;
    }

  e->name = name;
  e->namelen = namelen;
  e->value = value;
  e->raw_value = raw_value;
  e->wipe_on_free = cont->wipe_on_free;

  if (cont->first)
    {
      gpgrt_nve_t last;

      if (preserve_order || !name)
	last = cont->last;
      else
	{
	  /* See if there is already an entry with NAME.  */
	  last = _gpgrt_nvc_lookup (cont, name);

	  /* If so, find the last in that block.  */
	  if (last)
            {
              while (last->next)
                {
                  gpgrt_nve_t next = last->next;

                  if (next->name
                      && same_name_p (next->name, next->namelen, name))
                    last = next;
                  else
                    break;
                }
            }
	  else /* Otherwise, just find the last entry.  */
	    last = cont->last;
	}

      if (last->next)
	{
	  e->prev = last;
	  e->next = last->next;
	  last->next = e;
	  e->next->prev = e;
	}
      else
	{
	  e->prev = last;
	  last->next = e;
	  cont->last = e;
	}
    }
  else
    cont->first = cont->last = e;

  cont->modified = 1;

 leave:
  if (err)
    {
      xfree (name);
      if (value && cont->wipe_on_free)
	_gpgrt_wipememory (value, strlen (value));
      xfree (value);
      _gpgrt_strlist_free (raw_value);
    }

  return err;
}


/* Add (NAME, VALUE) to CONT.  If an entry with NAME already exists, it
 * is not updated but the new entry is appended.  */
gpg_err_code_t
_gpgrt_nvc_add (gpgrt_nvc_t cont, const char *name, const char *value)
{
  char *k, *v;

  k = xtrystrdup (name);
  if (!k)
    return _gpg_err_code_from_syserror ();

  v = xtrystrdup (value);
  if (!v)
    {
      xfree (k);
      return _gpg_err_code_from_syserror ();
    }

  return do_nvc_add (cont, k, v, NULL, 0);
}


/* Add (NAME, VALUE) to CONT.  If an entry with NAME already exists, it
 * is updated with VALUE.  If multiple entries with NAME exist, the
 * first entry is updated.  */
gpg_err_code_t
_gpgrt_nvc_set (gpgrt_nvc_t cont, const char *name, const char *value)
{
  gpgrt_nve_t e;

  if (! valid_name (name, cont->section_mode))
    return GPG_ERR_INV_NAME;

  e = _gpgrt_nvc_lookup (cont, name);
  if (e)
    return _gpgrt_nve_set (cont, e, value);
  else
    return _gpgrt_nvc_add (cont, name, value);
}


/* Update entry E to VALUE.  CONT is optional; if given its modified
 * flag will be updated.  */
gpg_err_code_t
_gpgrt_nve_set (gpgrt_nvc_t cont, gpgrt_nve_t e, const char *value)
{
  char *v;

  if (!e)
    return GPG_ERR_INV_ARG;

  if (e->value && value && !strcmp (e->value, value))
    {
      /* Setting same value - ignore this call and don't set the
       * modified flag (if CONT is given).  */
      return 0;
    }

  v = xtrystrdup (value? value:"");
  if (!v)
    return _gpg_err_code_from_syserror ();

  _gpgrt_strlist_free (e->raw_value);
  e->raw_value = NULL;
  if (e->value)
    _gpgrt_wipememory (e->value, strlen (e->value));
  xfree (e->value);
  e->value = v;
  if (cont)
    cont->modified = 1;

  return 0;
}


/* Delete the given ENTRY from the container CONT.  */
static void
do_nvc_delete (gpgrt_nvc_t cont, gpgrt_nve_t entry)
{
  if (entry->prev)
    entry->prev->next = entry->next;
  else
    cont->first = entry->next;

  if (entry->next)
    entry->next->prev = entry->prev;
  else
    cont->last = entry->prev;

  nve_release (entry, cont->private_key_mode);
  cont->modified = 1;
}


/* Delete entries from the container CONT.  Either ENTRY or NAME may
 * be given.  IF Entry is given only this entry is removed.  if NAME
 * is given all entries with this name are deleted.  */
void
_gpgrt_nvc_delete (gpgrt_nvc_t cont, gpgrt_nve_t entry, const char *name)
{
  if (entry)
    do_nvc_delete (cont, entry);
  else if (valid_name (name, cont->section_mode))
    {
      while ((entry = _gpgrt_nvc_lookup (cont, name)))
        do_nvc_delete (cont, entry);
    }
}



/* Lookup and iteration.  */

/* Get the first entry with the given name.  Return NULL if it does
 * not exist.  If NAME is NULL the first non-comment entry is
 * returned.  */
gpgrt_nve_t
_gpgrt_nvc_lookup (gpgrt_nvc_t cont, const char *name)
{
  gpgrt_nve_t entry;

  if (!cont)
    return NULL;

  if (!name)
    {
      for (entry = cont->first; entry; entry = entry->next)
        if (entry->name)
          return entry;

      return NULL;
    }

  for (entry = cont->first; entry; entry = entry->next)
    if (entry->name && same_name_p (entry->name, entry->namelen, name))
      return entry;

  return NULL;
}


/* Get the next non-comment entry.  If NAME is given the next entry
 * with that name is returned.  */
gpgrt_nve_t
_gpgrt_nve_next (gpgrt_nve_t entry, const char *name)
{
  if (!entry)
    return NULL;

  if (name)
    {
      for (entry = entry->next; entry; entry = entry->next)
        if (entry->name && same_name_p (entry->name, entry->namelen, name))
          return entry;
    }
  else
    {
      for (entry = entry->next; entry; entry = entry->next)
        if (entry->name)
          return entry;
    }
  return NULL;
}



/* Parsing and serialization.  */

static gpg_err_code_t
do_nvc_parse (gpgrt_nvc_t *result, int *errlinep, estream_t stream,
              unsigned int flags)
{
  gpg_err_code_t err = 0;
  gpgrt_ssize_t len;
  char *buf = NULL;
  size_t buf_len = 0;
  char *name = NULL;
  char *section = NULL;
  gpgrt_strlist_t raw_value = NULL;
  unsigned int slflags;

  *result = _gpgrt_nvc_new (flags);
  if (!*result)
    return _gpg_err_code_from_syserror ();

  slflags = (GPGRT_STRLIST_APPEND
             | ((flags & GPGRT_NVC_WIPE)? GPGRT_STRLIST_WIPE:0));

  if (errlinep)
    *errlinep = 0;
  while ((len = _gpgrt_read_line (stream, &buf, &buf_len, NULL)) > 0)
    {
      char *p, *p2;

      if (errlinep)
	*errlinep += 1;

      /* Skip any whitespace.  */
      for (p = buf; *p && ascii_isspace (*p); p++)
	/* Do nothing.  */;

      if (name && (spacep (buf) || !*p))
	{
	  /* A continuation.  */
	  if (!_gpgrt_strlist_add (&raw_value, buf, slflags))
	    {
	      err = _gpg_err_code_from_syserror ();
	      goto leave;
	    }
	  continue;
	}

      /* No continuation.  Add the current entry if any.  */
      if (raw_value)
	{
	  err = do_nvc_add (*result, name, NULL,
                            raw_value, 1 /*preserve order*/);
          name = NULL;
	  if (err)
	    goto leave;
	}

      /* And prepare for the next one.  */
      name = NULL;
      raw_value = NULL;

      if ((flags & GPGRT_NVC_SECTION) && *p == '[' && (p2=strchr (p+1, ']')))
        {
          /* This is a section header.  Extract it so that we can
           * prepend all names with it.  We allow a comment after the
           * section and spaces after the [ and before the ].  No
           * spaces inside the section name.  We also limit the name
           * to 200 characters. */
          _gpgrt_trim_spaces (p2+1);
          if (p == p2 || (p2[1] && p2[1] != '#'))
            {
              err = GPG_ERR_INV_VALUE;
	      goto leave;
            }
          *p2 = 0;
          p++;
          _gpgrt_trim_spaces (p);
          if (!*p || strpbrk (p, " \t#:") || strlen (p) > 200)
            {
              err = GPG_ERR_INV_VALUE;  /* No or invalid section name */
	      goto leave;
            }
          xfree (section);
          section = xtrystrdup (p);
          if (!section)
            {
	      err = _gpg_err_code_from_syserror ();
	      goto leave;
	    }
          /* Map all backslashes to slashes.  */
          for (p2=section; *p2; p2++)
            if (*p2 == '\\')
              *p2 = '/';

          continue;
        }


      if (*p && *p != '#')
	{
	  char *colon, *value, tmp;

	  colon = strchr (buf, ':');
	  if (!colon)
	    {
	      err = GPG_ERR_INV_VALUE;
	      goto leave;
	    }

	  value = colon + 1;
	  tmp = *value;
	  *value = 0;
          if (section)
            name = _gpgrt_strconcat (section, ":", p, NULL);
          else
            name = xtrystrdup (p);
	  *value = tmp;
	  if (!name)
	    {
	      err = _gpg_err_code_from_syserror ();
	      goto leave;
	    }

	  if (!_gpgrt_strlist_add (&raw_value, value, slflags))
	    {
	      err = _gpg_err_code_from_syserror ();
	      goto leave;
	    }
	  continue;
	}

      if (!_gpgrt_strlist_add (&raw_value, buf, slflags))
	{
	  err = _gpg_err_code_from_syserror ();
	  goto leave;
	}
    }
  if (len < 0)
    {
      err = _gpg_err_code_from_syserror ();
      goto leave;
    }

  /* Add the final entry.  */
  if (raw_value)
    {
      err = do_nvc_add (*result, name, NULL, raw_value, 1 /*preserve order*/);
      name = NULL;
    }

 leave:
  xfree (section);
  xfree (name);
  xfree (buf);
  if (err)
    {
      _gpgrt_nvc_release (*result);
      *result = NULL;
    }

  return err;
}


/* Parse STREAM and return a newly allocated name value container
 * structure in RESULT.  If ERRLINEP is given, the line number the
 * parser was last considering is stored there.  For private key
 * containers the GPGRT_NVC_PRIVKEY bit should be set in FLAGS.  */
gpg_err_code_t
_gpgrt_nvc_parse (gpgrt_nvc_t *result, int *errlinep, estream_t stream,
                  unsigned int flags)
{
  return do_nvc_parse (result, errlinep, stream, flags);
}


/* Helper for nvc_write.  */
static gpg_err_code_t
write_one_entry (gpgrt_nve_t entry, estream_t stream)
{
  gpg_err_code_t err;
  gpgrt_strlist_t sl;

  if (entry->name)
    _gpgrt_fputs (entry->name, stream);

  err = assert_raw_value (entry);
  if (err)
    return err;

  for (sl = entry->raw_value; sl; sl = sl->next)
    _gpgrt_fputs (sl->d, stream);

  if (_gpgrt_ferror (stream))
    return _gpg_err_code_from_syserror ();

  return 0;
}


/* Write a representation of CONT to STREAM.  */
gpg_err_code_t
_gpgrt_nvc_write (gpgrt_nvc_t cont, estream_t stream)
{
  gpg_err_code_t err = 0;
  gpgrt_nve_t entry;
  gpgrt_nve_t keyentry = NULL;

  /* We can't yet write out in section mode because we merge entries
   * from the same section and do not keep a raw representation.  */
  if (cont->section_mode)
    return GPG_ERR_NOT_IMPLEMENTED;

  for (entry = cont->first; entry; entry = entry->next)
    {
      if (cont->private_key_mode
          && entry->name && same_name_p (entry->name, entry->namelen, "Key"))
        {
          if (!keyentry)
            keyentry = entry;
          continue;
        }

      err = write_one_entry (entry, stream);
      if (err)
	return err;
    }

  /* In private key mode we write the Key always last.  */
  if (keyentry)
    err = write_one_entry (keyentry, stream);

  return err;
}



/*
 * Convenience functions.
 */

/* Return the string for the first entry in NVC with NAME.  If an
 * entry with NAME is missing in NVC or its value is the empty string
 * NULL is returned.  Note that the returned string is a pointer
 * into NVC.  */
const char *
_gpgrt_nvc_get_string (gpgrt_nvc_t nvc, const char *name)
{
  gpgrt_nve_t item;

  if (!nvc)
    return NULL;
  item = _gpgrt_nvc_lookup (nvc, name);
  if (!item)
    return NULL;
  return _gpgrt_nve_value (item);
}


/* Return true (ie. a non-zero value) if NAME exists and its value is
 * true; that is either "yes", "true", or a decimal value unequal to 0.  */
int
_gpgrt_nvc_get_bool (gpgrt_nvc_t nvc, const char *name)
{
  gpgrt_nve_t item;
  const char *s;
  int n;

  if (!nvc)
    return 0;
  item = _gpgrt_nvc_lookup (nvc, name);
  if (!item)
    return 0;
  s = _gpgrt_nve_value (item);
  if (!s)
    return 0;
  n = atoi (s);
  if (n)
    return n;
  if (!ascii_strcasecmp (s, "yes") || !ascii_strcasecmp (s, "true"))
    return 1;
  return 0;
}
