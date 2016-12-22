/* Writing binary .mo files.
   Copyright (C) 1995-1998, 2000-2007, 2015-2016 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, April 1995.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <alloca.h>

/* Specification.  */
#include "write-mo.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if HAVE_SYS_PARAM_H
# include <sys/param.h>
#endif

/* These two include files describe the binary .mo format.  */
#include "gmo.h"
#include "hash-string.h"

#include "byteswap.h"
#include "error.h"
#include "hash.h"
#include "message.h"
#include "format.h"
#include "xsize.h"
#include "xalloc.h"
#include "xmalloca.h"
#include "binary-io.h"
#include "fwriteerror.h"
#include "gettext.h"

#define _(str) gettext (str)

#define freea(p) /* nothing */

/* Usually defined in <sys/param.h>.  */
#ifndef roundup
# if defined __GNUC__ && __GNUC__ >= 2
#  define roundup(x, y) ({typeof(x) _x = (x); typeof(y) _y = (y); \
                          ((_x + _y - 1) / _y) * _y; })
# else
#  define roundup(x, y) ((((x)+((y)-1))/(y))*(y))
# endif /* GNU CC2  */
#endif /* roundup  */


/* Alignment of strings in resulting .mo file.  */
size_t alignment;

/* True if writing a .mo file in opposite endianness than the host.  */
bool byteswap;

/* True if no hash table in .mo is wanted.  */
bool no_hash_table;


/* Destructively changes the byte order of a 32-bit value in memory.  */
#define BSWAP32(x) (x) = bswap_32 (x)


/* Indices into the strings contained in 'struct pre_message' and
   'struct pre_sysdep_message'.  */
enum
{
  M_ID = 0,     /* msgid - the original string */
  M_STR = 1     /* msgstr - the translated string */
};

/* An intermediate data structure representing a 'struct string_desc'.  */
struct pre_string
{
  size_t length;
  const char *pointer;
};

/* An intermediate data structure representing a message.  */
struct pre_message
{
  struct pre_string str[2];
  const char *id_plural;
  size_t id_plural_len;
};

static int
compare_id (const void *pval1, const void *pval2)
{
  return strcmp (((struct pre_message *) pval1)->str[M_ID].pointer,
                 ((struct pre_message *) pval2)->str[M_ID].pointer);
}


/* An intermediate data structure representing a 'struct sysdep_segment'.  */
struct pre_sysdep_segment
{
  size_t length;
  const char *pointer;
};

/* An intermediate data structure representing a 'struct segment_pair'.  */
struct pre_segment_pair
{
  size_t segsize;
  const char *segptr;
  size_t sysdepref;
};

/* An intermediate data structure representing a 'struct sysdep_string'.  */
struct pre_sysdep_string
{
  unsigned int segmentcount;
  struct pre_segment_pair segments[1];
};

/* An intermediate data structure representing a message with system dependent
   strings.  */
struct pre_sysdep_message
{
  struct pre_sysdep_string *str[2];
  const char *id_plural;
  size_t id_plural_len;
};

/* Write the message list to the given open file.  */
static void
write_table (FILE *output_file, message_list_ty *mlp)
{
  char **msgctid_arr;
  size_t nstrings;
  struct pre_message *msg_arr;
  size_t n_sysdep_strings;
  struct pre_sysdep_message *sysdep_msg_arr;
  size_t n_sysdep_segments;
  struct pre_sysdep_segment *sysdep_segments;
  bool have_outdigits;
  int major_revision;
  int minor_revision;
  bool omit_hash_table;
  nls_uint32 hash_tab_size;
  struct mo_file_header header; /* Header of the .mo file to be written.  */
  size_t header_size;
  size_t offset;
  struct string_desc *orig_tab;
  struct string_desc *trans_tab;
  size_t sysdep_tab_offset = 0;
  size_t end_offset;
  char *null;
  size_t j, m;

  /* First pass: Move the static string pairs into an array, for sorting,
     and at the same time, compute the segments of the system dependent
     strings.  */
  msgctid_arr = XNMALLOC (mlp->nitems, char *);
  nstrings = 0;
  msg_arr = XNMALLOC (mlp->nitems, struct pre_message);
  n_sysdep_strings = 0;
  sysdep_msg_arr = XNMALLOC (mlp->nitems, struct pre_sysdep_message);
  n_sysdep_segments = 0;
  sysdep_segments = NULL;
  have_outdigits = false;
  for (j = 0; j < mlp->nitems; j++)
    {
      message_ty *mp = mlp->item[j];
      size_t msgctlen;
      char *msgctid;
      struct interval *intervals[2];
      size_t nintervals[2];

      /* Concatenate mp->msgctxt and mp->msgid into msgctid.  */
      msgctlen = (mp->msgctxt != NULL ? strlen (mp->msgctxt) + 1 : 0);
      msgctid = XNMALLOC (msgctlen + strlen (mp->msgid) + 1, char);
      if (mp->msgctxt != NULL)
        {
          memcpy (msgctid, mp->msgctxt, msgctlen - 1);
          msgctid[msgctlen - 1] = MSGCTXT_SEPARATOR;
        }
      strcpy (msgctid + msgctlen, mp->msgid);
      msgctid_arr[j] = msgctid;

      intervals[M_ID] = NULL;
      nintervals[M_ID] = 0;
      intervals[M_STR] = NULL;
      nintervals[M_STR] = 0;

      /* Test if mp contains system dependent strings and thus
         requires the use of the .mo file minor revision 1.  */
      if (possible_format_p (mp->is_format[format_c])
          || possible_format_p (mp->is_format[format_objc]))
        {
          /* Check whether msgid or msgstr contain ISO C 99 <inttypes.h>
             format string directives.  No need to check msgid_plural, because
             it is not accessed by the [n]gettext() function family.  */
          const char *p_end;
          const char *p;

          get_sysdep_c_format_directives (mp->msgid, false,
                                          &intervals[M_ID], &nintervals[M_ID]);
          if (msgctlen > 0)
            {
              struct interval *id_intervals = intervals[M_ID];
              size_t id_nintervals = nintervals[M_ID];

              if (id_nintervals > 0)
                {
                  unsigned int i;

                  for (i = 0; i < id_nintervals; i++)
                    {
                      id_intervals[i].startpos += msgctlen;
                      id_intervals[i].endpos += msgctlen;
                    }
                }
            }

          p_end = mp->msgstr + mp->msgstr_len;
          for (p = mp->msgstr; p < p_end; p += strlen (p) + 1)
            {
              struct interval *part_intervals;
              size_t part_nintervals;

              get_sysdep_c_format_directives (p, true,
                                              &part_intervals,
                                              &part_nintervals);
              if (part_nintervals > 0)
                {
                  size_t d = p - mp->msgstr;
                  unsigned int i;

                  intervals[M_STR] =
                    (struct interval *)
                    xrealloc (intervals[M_STR],
                              (nintervals[M_STR] + part_nintervals)
                              * sizeof (struct interval));
                  for (i = 0; i < part_nintervals; i++)
                    {
                      intervals[M_STR][nintervals[M_STR] + i].startpos =
                        d + part_intervals[i].startpos;
                      intervals[M_STR][nintervals[M_STR] + i].endpos =
                        d + part_intervals[i].endpos;
                    }
                  nintervals[M_STR] += part_nintervals;
                }
            }
        }

      if (nintervals[M_ID] > 0 || nintervals[M_STR] > 0)
        {
          /* System dependent string pair.  */
          for (m = 0; m < 2; m++)
            {
              struct pre_sysdep_string *pre =
                (struct pre_sysdep_string *)
                xmalloc (xsum (sizeof (struct pre_sysdep_string),
                               xtimes (nintervals[m],
                                       sizeof (struct pre_segment_pair))));
              const char *str;
              size_t str_len;
              size_t lastpos;
              unsigned int i;

              if (m == M_ID)
                {
                  str = msgctid; /* concatenation of mp->msgctxt + mp->msgid  */
                  str_len = strlen (msgctid) + 1;
                }
              else
                {
                  str = mp->msgstr;
                  str_len = mp->msgstr_len;
                }

              lastpos = 0;
              pre->segmentcount = nintervals[m];
              for (i = 0; i < nintervals[m]; i++)
                {
                  size_t length;
                  const char *pointer;
                  size_t r;

                  pre->segments[i].segptr = str + lastpos;
                  pre->segments[i].segsize = intervals[m][i].startpos - lastpos;

                  length = intervals[m][i].endpos - intervals[m][i].startpos;
                  pointer = str + intervals[m][i].startpos;
                  if (length >= 2
                      && pointer[0] == '<' && pointer[length - 1] == '>')
                    {
                      /* Skip the '<' and '>' markers.  */
                      length -= 2;
                      pointer += 1;
                    }

                  for (r = 0; r < n_sysdep_segments; r++)
                    if (sysdep_segments[r].length == length
                        && memcmp (sysdep_segments[r].pointer, pointer, length)
                           == 0)
                      break;
                  if (r == n_sysdep_segments)
                    {
                      n_sysdep_segments++;
                      sysdep_segments =
                        (struct pre_sysdep_segment *)
                        xrealloc (sysdep_segments,
                                  n_sysdep_segments
                                  * sizeof (struct pre_sysdep_segment));
                      sysdep_segments[r].length = length;
                      sysdep_segments[r].pointer = pointer;
                    }

                  pre->segments[i].sysdepref = r;

                  if (length == 1 && *pointer == 'I')
                    have_outdigits = true;

                  lastpos = intervals[m][i].endpos;
                }
              pre->segments[i].segptr = str + lastpos;
              pre->segments[i].segsize = str_len - lastpos;
              pre->segments[i].sysdepref = SEGMENTS_END;

              sysdep_msg_arr[n_sysdep_strings].str[m] = pre;
            }

          sysdep_msg_arr[n_sysdep_strings].id_plural = mp->msgid_plural;
          sysdep_msg_arr[n_sysdep_strings].id_plural_len =
            (mp->msgid_plural != NULL ? strlen (mp->msgid_plural) + 1 : 0);
          n_sysdep_strings++;
        }
      else
        {
          /* Static string pair.  */
          msg_arr[nstrings].str[M_ID].pointer = msgctid;
          msg_arr[nstrings].str[M_ID].length = strlen (msgctid) + 1;
          msg_arr[nstrings].str[M_STR].pointer = mp->msgstr;
          msg_arr[nstrings].str[M_STR].length = mp->msgstr_len;
          msg_arr[nstrings].id_plural = mp->msgid_plural;
          msg_arr[nstrings].id_plural_len =
            (mp->msgid_plural != NULL ? strlen (mp->msgid_plural) + 1 : 0);
          nstrings++;
        }

      for (m = 0; m < 2; m++)
        if (intervals[m] != NULL)
          free (intervals[m]);
    }

  /* Sort the table according to original string.  */
  if (nstrings > 0)
    qsort (msg_arr, nstrings, sizeof (struct pre_message), compare_id);

  /* We need major revision 1 if there are system dependent strings that use
     "I" because older versions of gettext() crash when this occurs in a .mo
     file.  Otherwise use major revision 0.  */
  major_revision =
    (have_outdigits ? MO_REVISION_NUMBER_WITH_SYSDEP_I : MO_REVISION_NUMBER);

  /* We need minor revision 1 if there are system dependent strings.
     Otherwise we choose minor revision 0 because it's supported by older
     versions of libintl and revision 1 isn't.  */
  minor_revision = (n_sysdep_strings > 0 ? 1 : 0);

  /* In minor revision >= 1, the hash table is obligatory.  */
  omit_hash_table = (no_hash_table && minor_revision == 0);

  /* This should be explained:
     Each string has an associate hashing value V, computed by a fixed
     function.  To locate the string we use open addressing with double
     hashing.  The first index will be V % M, where M is the size of the
     hashing table.  If no entry is found, iterating with a second,
     independent hashing function takes place.  This second value will
     be 1 + V % (M - 2).
     The approximate number of probes will be

       for unsuccessful search:  (1 - N / M) ^ -1
       for successful search:    - (N / M) ^ -1 * ln (1 - N / M)

     where N is the number of keys.

     If we now choose M to be the next prime bigger than 4 / 3 * N,
     we get the values
                         4   and   1.85  resp.
     Because unsuccessful searches are unlikely this is a good value.
     Formulas: [Knuth, The Art of Computer Programming, Volume 3,
                Sorting and Searching, 1973, Addison Wesley]  */
  if (!omit_hash_table)
    {
      hash_tab_size = next_prime ((mlp->nitems * 4) / 3);
      /* Ensure M > 2.  */
      if (hash_tab_size <= 2)
        hash_tab_size = 3;
    }
  else
    hash_tab_size = 0;


  /* Second pass: Fill the structure describing the header.  At the same time,
     compute the sizes and offsets of the non-string parts of the file.  */

  /* Magic number.  */
  header.magic = _MAGIC;
  /* Revision number of file format.  */
  header.revision = (major_revision << 16) + minor_revision;

  header_size =
    (minor_revision == 0
     ? offsetof (struct mo_file_header, n_sysdep_segments)
     : sizeof (struct mo_file_header));
  offset = header_size;

  /* Number of static string pairs.  */
  header.nstrings = nstrings;

  /* Offset of table for original string offsets.  */
  header.orig_tab_offset = offset;
  offset += nstrings * sizeof (struct string_desc);
  orig_tab = XNMALLOC (nstrings, struct string_desc);

  /* Offset of table for translated string offsets.  */
  header.trans_tab_offset = offset;
  offset += nstrings * sizeof (struct string_desc);
  trans_tab = XNMALLOC (nstrings, struct string_desc);

  /* Size of hash table.  */
  header.hash_tab_size = hash_tab_size;
  /* Offset of hash table.  */
  header.hash_tab_offset = offset;
  offset += hash_tab_size * sizeof (nls_uint32);

  if (minor_revision >= 1)
    {
      /* Size of table describing system dependent segments.  */
      header.n_sysdep_segments = n_sysdep_segments;
      /* Offset of table describing system dependent segments.  */
      header.sysdep_segments_offset = offset;
      offset += n_sysdep_segments * sizeof (struct sysdep_segment);

      /* Number of system dependent string pairs.  */
      header.n_sysdep_strings = n_sysdep_strings;

      /* Offset of table for original sysdep string offsets.  */
      header.orig_sysdep_tab_offset = offset;
      offset += n_sysdep_strings * sizeof (nls_uint32);

      /* Offset of table for translated sysdep string offsets.  */
      header.trans_sysdep_tab_offset = offset;
      offset += n_sysdep_strings * sizeof (nls_uint32);

      /* System dependent string descriptors.  */
      sysdep_tab_offset = offset;
      for (m = 0; m < 2; m++)
        for (j = 0; j < n_sysdep_strings; j++)
          offset += sizeof (struct sysdep_string)
                    + sysdep_msg_arr[j].str[m]->segmentcount
                      * sizeof (struct segment_pair);
    }

  end_offset = offset;


  /* Third pass: Write the non-string parts of the file.  At the same time,
     compute the offsets of each string, including the proper alignment.  */

  /* Write the header out.  */
  if (byteswap)
    {
      BSWAP32 (header.magic);
      BSWAP32 (header.revision);
      BSWAP32 (header.nstrings);
      BSWAP32 (header.orig_tab_offset);
      BSWAP32 (header.trans_tab_offset);
      BSWAP32 (header.hash_tab_size);
      BSWAP32 (header.hash_tab_offset);
      if (minor_revision >= 1)
        {
          BSWAP32 (header.n_sysdep_segments);
          BSWAP32 (header.sysdep_segments_offset);
          BSWAP32 (header.n_sysdep_strings);
          BSWAP32 (header.orig_sysdep_tab_offset);
          BSWAP32 (header.trans_sysdep_tab_offset);
        }
    }
  fwrite (&header, header_size, 1, output_file);

  /* Table for original string offsets.  */
  /* Here output_file is at position header.orig_tab_offset.  */

  for (j = 0; j < nstrings; j++)
    {
      offset = roundup (offset, alignment);
      orig_tab[j].length =
        msg_arr[j].str[M_ID].length + msg_arr[j].id_plural_len;
      orig_tab[j].offset = offset;
      offset += orig_tab[j].length;
      /* Subtract 1 because of the terminating NUL.  */
      orig_tab[j].length--;
    }
  if (byteswap)
    for (j = 0; j < nstrings; j++)
      {
        BSWAP32 (orig_tab[j].length);
        BSWAP32 (orig_tab[j].offset);
      }
  fwrite (orig_tab, nstrings * sizeof (struct string_desc), 1, output_file);

  /* Table for translated string offsets.  */
  /* Here output_file is at position header.trans_tab_offset.  */

  for (j = 0; j < nstrings; j++)
    {
      offset = roundup (offset, alignment);
      trans_tab[j].length = msg_arr[j].str[M_STR].length;
      trans_tab[j].offset = offset;
      offset += trans_tab[j].length;
      /* Subtract 1 because of the terminating NUL.  */
      trans_tab[j].length--;
    }
  if (byteswap)
    for (j = 0; j < nstrings; j++)
      {
        BSWAP32 (trans_tab[j].length);
        BSWAP32 (trans_tab[j].offset);
      }
  fwrite (trans_tab, nstrings * sizeof (struct string_desc), 1, output_file);

  /* Skip this part when no hash table is needed.  */
  if (!omit_hash_table)
    {
      nls_uint32 *hash_tab;
      unsigned int j;

      /* Here output_file is at position header.hash_tab_offset.  */

      /* Allocate room for the hashing table to be written out.  */
      hash_tab = XNMALLOC (hash_tab_size, nls_uint32);
      memset (hash_tab, '\0', hash_tab_size * sizeof (nls_uint32));

      /* Insert all value in the hash table, following the algorithm described
         above.  */
      for (j = 0; j < nstrings; j++)
        {
          nls_uint32 hash_val = hash_string (msg_arr[j].str[M_ID].pointer);
          nls_uint32 idx = hash_val % hash_tab_size;

          if (hash_tab[idx] != 0)
            {
              /* We need the second hashing function.  */
              nls_uint32 incr = 1 + (hash_val % (hash_tab_size - 2));

              do
                if (idx >= hash_tab_size - incr)
                  idx -= hash_tab_size - incr;
                else
                  idx += incr;
              while (hash_tab[idx] != 0);
            }

          hash_tab[idx] = j + 1;
        }

      /* Write the hash table out.  */
      if (byteswap)
        for (j = 0; j < hash_tab_size; j++)
          BSWAP32 (hash_tab[j]);
      fwrite (hash_tab, hash_tab_size * sizeof (nls_uint32), 1, output_file);

      free (hash_tab);
    }

  if (minor_revision >= 1)
    {
      struct sysdep_segment *sysdep_segments_tab;
      nls_uint32 *sysdep_tab;
      size_t stoffset;
      unsigned int i;

      /* Here output_file is at position header.sysdep_segments_offset.  */

      sysdep_segments_tab =
        XNMALLOC (n_sysdep_segments, struct sysdep_segment);
      for (i = 0; i < n_sysdep_segments; i++)
        {
          offset = roundup (offset, alignment);
          /* The "+ 1" accounts for the trailing NUL byte.  */
          sysdep_segments_tab[i].length = sysdep_segments[i].length + 1;
          sysdep_segments_tab[i].offset = offset;
          offset += sysdep_segments_tab[i].length;
        }

      if (byteswap)
        for (i = 0; i < n_sysdep_segments; i++)
          {
            BSWAP32 (sysdep_segments_tab[i].length);
            BSWAP32 (sysdep_segments_tab[i].offset);
          }
      fwrite (sysdep_segments_tab,
              n_sysdep_segments * sizeof (struct sysdep_segment), 1,
              output_file);

      free (sysdep_segments_tab);

      sysdep_tab = XNMALLOC (n_sysdep_strings, nls_uint32);
      stoffset = sysdep_tab_offset;

      for (m = 0; m < 2; m++)
        {
          /* Here output_file is at position
             m == M_ID  -> header.orig_sysdep_tab_offset,
             m == M_STR -> header.trans_sysdep_tab_offset.  */

          for (j = 0; j < n_sysdep_strings; j++)
            {
              sysdep_tab[j] = stoffset;
              stoffset += sizeof (struct sysdep_string)
                          + sysdep_msg_arr[j].str[m]->segmentcount
                            * sizeof (struct segment_pair);
            }
          /* Write the table for original/translated sysdep string offsets.  */
          if (byteswap)
            for (j = 0; j < n_sysdep_strings; j++)
              BSWAP32 (sysdep_tab[j]);
          fwrite (sysdep_tab, n_sysdep_strings * sizeof (nls_uint32), 1,
                  output_file);
        }

      free (sysdep_tab);

      /* Here output_file is at position sysdep_tab_offset.  */

      for (m = 0; m < 2; m++)
        for (j = 0; j < n_sysdep_strings; j++)
          {
            struct pre_sysdep_message *msg = &sysdep_msg_arr[j];
            struct pre_sysdep_string *pre = msg->str[m];
            struct sysdep_string *str =
              (struct sysdep_string *)
              xmalloca (sizeof (struct sysdep_string)
                        + pre->segmentcount * sizeof (struct segment_pair));
            unsigned int i;

            offset = roundup (offset, alignment);
            str->offset = offset;
            for (i = 0; i <= pre->segmentcount; i++)
              {
                str->segments[i].segsize = pre->segments[i].segsize;
                str->segments[i].sysdepref = pre->segments[i].sysdepref;
                offset += str->segments[i].segsize;
              }
            if (m == M_ID && msg->id_plural_len > 0)
              {
                str->segments[pre->segmentcount].segsize += msg->id_plural_len;
                offset += msg->id_plural_len;
              }
            if (byteswap)
              {
                BSWAP32 (str->offset);
                for (i = 0; i <= pre->segmentcount; i++)
                  {
                    BSWAP32 (str->segments[i].segsize);
                    BSWAP32 (str->segments[i].sysdepref);
                  }
              }
            fwrite (str,
                    sizeof (struct sysdep_string)
                    + pre->segmentcount * sizeof (struct segment_pair),
                    1, output_file);

            freea (str);
          }
    }

  /* Here output_file is at position end_offset.  */

  free (trans_tab);
  free (orig_tab);


  /* Fourth pass: Write the strings.  */

  offset = end_offset;

  /* A few zero bytes for padding.  */
  null = (char *) alloca (alignment);
  memset (null, '\0', alignment);

  /* Now write the original strings.  */
  for (j = 0; j < nstrings; j++)
    {
      fwrite (null, roundup (offset, alignment) - offset, 1, output_file);
      offset = roundup (offset, alignment);

      fwrite (msg_arr[j].str[M_ID].pointer, msg_arr[j].str[M_ID].length, 1,
              output_file);
      if (msg_arr[j].id_plural_len > 0)
        fwrite (msg_arr[j].id_plural, msg_arr[j].id_plural_len, 1,
                output_file);
      offset += msg_arr[j].str[M_ID].length + msg_arr[j].id_plural_len;
    }

  /* Now write the translated strings.  */
  for (j = 0; j < nstrings; j++)
    {
      fwrite (null, roundup (offset, alignment) - offset, 1, output_file);
      offset = roundup (offset, alignment);

      fwrite (msg_arr[j].str[M_STR].pointer, msg_arr[j].str[M_STR].length, 1,
              output_file);
      offset += msg_arr[j].str[M_STR].length;
    }

  if (minor_revision >= 1)
    {
      unsigned int i;

      for (i = 0; i < n_sysdep_segments; i++)
        {
          fwrite (null, roundup (offset, alignment) - offset, 1, output_file);
          offset = roundup (offset, alignment);

          fwrite (sysdep_segments[i].pointer, sysdep_segments[i].length, 1,
                  output_file);
          fwrite (null, 1, 1, output_file);
          offset += sysdep_segments[i].length + 1;
        }

      for (m = 0; m < 2; m++)
        for (j = 0; j < n_sysdep_strings; j++)
          {
            struct pre_sysdep_message *msg = &sysdep_msg_arr[j];
            struct pre_sysdep_string *pre = msg->str[m];

            fwrite (null, roundup (offset, alignment) - offset, 1,
                    output_file);
            offset = roundup (offset, alignment);

            for (i = 0; i <= pre->segmentcount; i++)
              {
                fwrite (pre->segments[i].segptr, pre->segments[i].segsize, 1,
                        output_file);
                offset += pre->segments[i].segsize;
              }
            if (m == M_ID && msg->id_plural_len > 0)
              {
                fwrite (msg->id_plural, msg->id_plural_len, 1, output_file);
                offset += msg->id_plural_len;
              }

            free (pre);
          }
    }

  freea (null);
  for (j = 0; j < mlp->nitems; j++)
    free (msgctid_arr[j]);
  free (sysdep_msg_arr);
  free (msg_arr);
  free (msgctid_arr);
}


int
msgdomain_write_mo (message_list_ty *mlp,
                    const char *domain_name,
                    const char *file_name)
{
  FILE *output_file;

  /* If no entry for this domain don't even create the file.  */
  if (mlp->nitems != 0)
    {
      if (strcmp (domain_name, "-") == 0)
        {
          output_file = stdout;
          SET_BINARY (fileno (output_file));
        }
      else
        {
          output_file = fopen (file_name, "wb");
          if (output_file == NULL)
            {
              error (0, errno, _("error while opening \"%s\" for writing"),
                     file_name);
              return 1;
            }
        }

      if (output_file != NULL)
        {
          write_table (output_file, mlp);

          /* Make sure nothing went wrong.  */
          if (fwriteerror (output_file))
            error (EXIT_FAILURE, errno, _("error while writing \"%s\" file"),
                   file_name);
        }
    }

  return 0;
}
