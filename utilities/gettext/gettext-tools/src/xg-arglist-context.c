/* Keeping track of the flags that apply to a string extracted
   in a certain context.
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

/* Specification.  */
#include "xg-arglist-context.h"

#include <stdlib.h>

#include "attribute.h"
#include "xalloc.h"
#include "xmalloca.h"
#include "verify.h"

#include "xg-message.h"


/* Null context.  */
static flag_context_ty null_context =
  {
    {
      { undecided, false },
      { undecided, false },
      { undecided, false },
      { undecided, false }
    }
  };

/* Transparent context.  */
MAYBE_UNUSED static flag_context_ty passthrough_context =
  {
    {
      { undecided, true },
      { undecided, true },
      { undecided, true },
      { undecided, true }
    }
  };


/* Null context list iterator.  */
flag_context_list_iterator_ty null_context_list_iterator = { 1, NULL };

/* Transparent context list iterator.  */
static flag_context_list_ty passthrough_context_circular_list =
  {
    1,
    {
      {
        { undecided, true },
        { undecided, true },
        { undecided, true },
        { undecided, true }
      }
    },
    &passthrough_context_circular_list
  };
flag_context_list_iterator_ty passthrough_context_list_iterator =
  {
    1,
    &passthrough_context_circular_list
  };


flag_context_list_iterator_ty
flag_context_list_iterator (flag_context_list_ty *list)
{
  flag_context_list_iterator_ty result;

  result.argnum = 1;
  result.head = list;
  return result;
}


flag_context_ty
flag_context_list_iterator_advance (flag_context_list_iterator_ty *iter)
{
  if (iter->head == NULL)
    return null_context;
  if (iter->argnum == iter->head->argnum)
    {
      flag_context_ty result = iter->head->flags;

      /* Special casing of circular list.  */
      if (iter->head != iter->head->next)
        {
          iter->head = iter->head->next;
          iter->argnum++;
        }

      return result;
    }
  else
    {
      iter->argnum++;
      return null_context;
    }
}


flag_context_list_ty *
flag_context_list_table_lookup (flag_context_list_table_ty *flag_table,
                                const void *key, size_t keylen)
{
  if (flag_table->table != NULL)
    {
      void *entry;
      if (hash_find_entry (flag_table, key, keylen, &entry) == 0)
        return (flag_context_list_ty *) entry;
    }
  return NULL;
}


/* In the FLAGS, set the pair (is_format, pass_format) for the format string
   type FI to (VALUE, PASS).  */
static void
set_flags_for_formatstring_type (flag_context_ty *flags, size_t fi,
                                 enum is_format value, bool pass)
{
  flags->for_formatstring[fi].is_format = value;
  flags->for_formatstring[fi].pass_format = pass;
}


void
flag_context_list_table_add (flag_context_list_table_ty *table,
                             size_t fi,
                             const char *name_start, const char *name_end,
                             int argnum, enum is_format value, bool pass)
{
  if (table->table == NULL)
    hash_init (table, 100);
  {
    void *entry;

    if (hash_find_entry (table, name_start, name_end - name_start, &entry) != 0)
      {
        /* Create new hash table entry.  */
        flag_context_list_ty *list = XMALLOC (flag_context_list_ty);
        list->argnum = argnum;
        memset (&list->flags, '\0', sizeof (list->flags));
        set_flags_for_formatstring_type (&list->flags, fi, value, pass);
        list->next = NULL;

        hash_insert_entry (table, name_start, name_end - name_start, list);
      }
    else
      {
        /* We don't put NULL entries into the table.  */
        assume (entry != NULL);

        flag_context_list_ty *list = (flag_context_list_ty *)entry;
        flag_context_list_ty **lastp = NULL;
        /* Invariant: list == (lastp != NULL ? *lastp : entry).  */

        while (list != NULL && list->argnum < argnum)
          {
            lastp = &list->next;
            list = *lastp;
          }
        if (list != NULL && list->argnum == argnum)
          {
            /* Add this flag to the current argument number.  */
            set_flags_for_formatstring_type (&list->flags, fi, value, pass);
          }
        else if (lastp != NULL)
          {
            /* Add a new list entry for this argument number.  */
            list = XMALLOC (flag_context_list_ty);
            list->argnum = argnum;
            memset (&list->flags, '\0', sizeof (list->flags));
            set_flags_for_formatstring_type (&list->flags, fi, value, pass);

            list->next = *lastp;
            *lastp = list;
          }
        else
          {
            /* Add a new list entry for this argument number, at the beginning
               of the list.  Since we don't have an API for replacing the
               value of a key in the hash table, we have to copy the first
               list element.  */
            flag_context_list_ty *copy = XMALLOC (flag_context_list_ty);
            *copy = *list;

            list->argnum = argnum;
            memset (&list->flags, '\0', sizeof (list->flags));
            set_flags_for_formatstring_type (&list->flags, fi, value, pass);

            list->next = copy;
          }
      }
  }
}


static struct remembered_message_list_ty *
remembered_message_list_alloc ()
{
  struct remembered_message_list_ty *list = XMALLOC (struct remembered_message_list_ty);
  list->refcount = 1;
  list->item = NULL;
  list->nitems = 0;
  list->nitems_max = 0;
  return list;
}

void
remembered_message_list_append (struct remembered_message_list_ty *list,
                                struct remembered_message_ty element)
{
  if (list->nitems >= list->nitems_max)
    {
      list->nitems_max = list->nitems_max * 2 + 4;
      size_t nbytes = list->nitems_max * sizeof (struct remembered_message_ty);
      list->item = xrealloc (list->item, nbytes);
    }
  list->item[list->nitems++] = element;
}

static struct remembered_message_list_ty *
remembered_message_list_ref (struct remembered_message_list_ty *list)
{
  if (list != NULL)
    list->refcount++;
  return list;
}

static void
remembered_message_list_unref (struct remembered_message_list_ty *list)
{
  if (list != NULL)
    {
      if (list->refcount > 1)
        list->refcount--;
      else
        free (list);
    }
}


/* We don't need to remember messages that were processed in the null context
   region.  Therefore the null context region can be a singleton.  This
   reduces the number of needed calls to unref_region.  */
static flag_region_ty const the_null_context_region =
  {
    1,
    {
      { true, undecided, NULL },
      { true, undecided, NULL },
      { true, undecided, NULL },
      { true, undecided, NULL }
    },
    NULL, 0, 0,
    true
  };

flag_region_ty *
null_context_region ()
{
  return (flag_region_ty *) &the_null_context_region;
}


flag_region_ty *
inheriting_region (flag_region_ty *outer_region,
                   flag_context_ty modifier_context)
{
  flag_region_ty *region = XMALLOC (flag_region_ty);
  region->refcount = 1;
  for (size_t fi = 0; fi < NXFORMATS; fi++)
    {
      region->for_formatstring[fi].pass_format = modifier_context.for_formatstring[fi].pass_format;
      if (modifier_context.for_formatstring[fi].pass_format)
        {
          region->for_formatstring[fi].is_format = outer_region->for_formatstring[fi].is_format;
          region->for_formatstring[fi].remembered =
            (current_formatstring_parser[fi] != NULL
             ? (outer_region->for_formatstring[fi].remembered != NULL
                ? remembered_message_list_ref (outer_region->for_formatstring[fi].remembered)
                : remembered_message_list_alloc ())
             : NULL);
        }
      else
        {
          region->for_formatstring[fi].is_format = modifier_context.for_formatstring[fi].is_format;
          region->for_formatstring[fi].remembered =
            (current_formatstring_parser[fi] != NULL
             ? remembered_message_list_alloc ()
             : NULL);
        }
    }
  region->subregion = NULL;
  region->nsubregions = 0;
  region->nsubregions_max = 0;
  region->inherit_from_parent_region = true;

  return region;
}


flag_region_ty *
new_sub_region (flag_region_ty *outer_region, flag_context_ty modifier_context)
{
  /* Create the new region.  */
  flag_region_ty *region = XMALLOC (flag_region_ty);
  region->refcount = 1;
  for (size_t fi = 0; fi < NXFORMATS; fi++)
    {
      region->for_formatstring[fi].pass_format = modifier_context.for_formatstring[fi].pass_format;
      if (modifier_context.for_formatstring[fi].pass_format)
        region->for_formatstring[fi].is_format = outer_region->for_formatstring[fi].is_format;
      else
        region->for_formatstring[fi].is_format = modifier_context.for_formatstring[fi].is_format;
      region->for_formatstring[fi].remembered =
        (current_formatstring_parser[fi] != NULL
         ? remembered_message_list_alloc ()
         : NULL);
    }
  region->subregion = NULL;
  region->nsubregions = 0;
  region->nsubregions_max = 0;
  /* Set to true initially.  Can be set to false later during the parsing.  */
  region->inherit_from_parent_region = true;

  if (outer_region != &the_null_context_region)
    {
      /* Register it as child of outer_region.  */
      if (outer_region->nsubregions >= outer_region->nsubregions_max)
        {
          outer_region->nsubregions_max = outer_region->nsubregions_max * 2 + 4;
          size_t nbytes = outer_region->nsubregions_max * sizeof (struct flag_region_ty *);
          outer_region->subregion = xrealloc (outer_region->subregion, nbytes);
        }
      outer_region->subregion[outer_region->nsubregions++] = region;
      region->refcount++;
    }

  return region;
}

flag_region_ty *
ref_region (flag_region_ty *region)
{
  if (region != NULL && region != &the_null_context_region)
    region->refcount++;
  return region;
}


void
unref_region (flag_region_ty *region)
{
  if (region != NULL && region != &the_null_context_region)
    {
      if (region->refcount > 1)
        region->refcount--;
      else
        {
          for (size_t i = 0; i < region->nsubregions; i++)
            unref_region (region->subregion[i]);
          free (region->subregion);
          for (size_t fi = 0; fi < NXFORMATS; fi++)
            remembered_message_list_unref (region->for_formatstring[fi].remembered);
          free (region);
        }
    }
}


void
set_format_flag_on_region (flag_region_ty *region,
                           size_t fi, enum is_format value)
{
  /* First, on this region.  */
  region->for_formatstring[fi].is_format = value;
  struct remembered_message_list_ty *rmlp =
    region->for_formatstring[fi].remembered;
  for (size_t i = 0; i < rmlp->nitems; i++)
    {
      struct remembered_message_ty *rmp = &rmlp->item[i];
      set_format_flag_from_context (rmp->mp, rmp->plural, &rmp->pos,
                                    fi, region);
    }

  /* Then, recurse through the sub-regions that inherit.  */
  for (size_t i = 0; i < region->nsubregions; i++)
    {
      flag_region_ty *sub_region = region->subregion[i];
      if (sub_region->inherit_from_parent_region
          && sub_region->for_formatstring[fi].pass_format)
        set_format_flag_on_region (sub_region, fi, value);
    }
}
