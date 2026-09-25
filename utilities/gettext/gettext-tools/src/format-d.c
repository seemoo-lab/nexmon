/* D format strings.
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

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#include "format.h"
#include "attribute.h"
#include "c-ctype.h"
#include "gcd.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "format-invalid.h"
#include "minmax.h"
#include "gettext.h"

#define _(str) gettext (str)


/* Assertion macro.  Could be defined to empty for speed.  */
#define ASSERT(expr) if (!(expr)) abort ();


/* D format strings are described in the description of the std.format module
   <https://dlang.org/library/std/format.html> and implemented in
   gcc-14.2.0/libphobos/src/std/format/spec.d
   gcc-14.2.0/libphobos/src/std/format/write.d
   gcc-14.2.0/libphobos/src/std/format/internal/write.d .

   A format string consists of literal text (that is output verbatim), doubled
   percent-signs ('%%', that lead to a single percent-sign when output), and
   directives.
   A directive
   - starts with '%',
   - is optionally followed by
       a positive integer m, then '$', or
       a positive integer m, then ':', then a positive integer m₂ ≥ m, then '$', or
       a positive integer m, then ':', then '$',
   - is optionally followed by a sequence of flags, each being one of
       '+', '-', ' ', '0', '#', '=',
   - is optionally followed by a width specification:
       a positive integer, or
       '*', or
       '*', then a positive integer, then '$',
   - is optionally followed by a precision specification:
       '.' then optionally:
         a positive integer, or
         '*', or
         '*', then a positive integer, then '$',
   - is optionally followed by a separator specification:
       ',' then optionally:
         a positive integer, or
         '*',
       then optionally a '?',
   - is followed by
       either a format specifier
       or a compound specifier:
         - a '(',
         - a format string that eats 1 or 2 arguments,
         - optionally '%|' then literal text, possibly with doubled
           percent-signs,
         - '%)'.
 */

/* Data structure describing format string derived constraints for an
   argument list.  It is a recursive list structure.  Structure sharing
   is not allowed.  */

enum format_cdr_type
{
  FCT_REQUIRED, /* The format argument list cannot end before this argument.  */
  FCT_OPTIONAL  /* The format argument list may end before this argument.  */
};

enum format_arg_type
{
  FAT_NONE           = 0,
  FAT_BOOL           = 1 << 0,
  FAT_INTEGER        = 1 << 1,
  FAT_FLOATINGPOINT  = 1 << 2,
  FAT_CHAR           = 1 << 3,
  FAT_ARRAY          = 1 << 4, /* string or array */
  FAT_ASSOCIATIVE    = 1 << 5,
  FAT_IRANGE         = 1 << 6, /* irange or simd */
  FAT_STRUCT         = 1 << 7, /* struct or class or union */
  FAT_POINTER        = 1 << 8, /* pointer or null */
  /* Note: enum are not listed here, since enum values can be formatted with
     any specifier available for their base type.  */
  FAT_ANY_TYPE       = (FAT_BOOL | FAT_INTEGER | FAT_FLOATINGPOINT | FAT_CHAR
                        | FAT_ARRAY | FAT_ASSOCIATIVE | FAT_IRANGE | FAT_STRUCT
                        | FAT_POINTER),
  /* A flag: */
  FAT_ELEMENTWISE    = 1 << 10,
  /* Combination of allowed types and flag: */
  FAT_ELEMENTWISE_1  = FAT_ELEMENTWISE | FAT_ARRAY | FAT_IRANGE,
  FAT_ELEMENTWISE_2  = FAT_ELEMENTWISE | FAT_ASSOCIATIVE
};

struct format_arg
{
  size_t repcount;       /* Number of consecutive arguments this constraint
                            applies to.  Normally 1, but unconstrained
                            arguments are often repeated.  */
  enum format_cdr_type presence; /* Can the argument list end right before
                                    this argument?  */
  enum format_arg_type type;    /* Possible values for this argument.  */
  struct format_arg_list *list; /* For FAT_ELEMENTWISE.  */
};

struct segment
{
  size_t count;          /* Number of format_arg records used.  */
  size_t allocated;
  struct format_arg *element;   /* Argument constraints.  */
  size_t length;         /* Number of arguments represented by this segment.
                            This is the sum of all repcounts in the segment.  */
};

struct format_arg_list
{
  /* The constraints for the potentially infinite argument list are assumed
     to become ultimately periodic.  Such a periodic sequence can be split into
     an initial segment and an endlessly repeated loop segment.
     A finite sequence is represented entirely in the initial segment; the
     loop segment is empty.
     In this file, the loop segment is always either empty or has length 1.
     But it is not worth exploiting this property: The code is more future-proof
     in the general form, shared with format-lisp.c and format-scheme.c.  */

  struct segment initial;       /* Initial arguments segment.  */
  struct segment repeated;      /* Endlessly repeated segment.  */
};

struct spec
{
  size_t directives;
  /* We consider a directive as "likely intentional" if it does not contain a
     space.  This prevents xgettext from flagging strings like "100% complete"
     as 'd-format' if they don't occur in a context that requires a format
     string.  */
  size_t likely_intentional_directives;
  struct format_arg_list *list;
};


/* Forward declaration of local functions.  */
static void verify_list (const struct format_arg_list *list);
static void free_list (struct format_arg_list *list);
static struct format_arg_list * copy_list (const struct format_arg_list *list);
static bool equal_list (const struct format_arg_list *list1,
                        const struct format_arg_list *list2);
static struct format_arg_list * make_intersected_list
                                               (struct format_arg_list *list1,
                                                struct format_arg_list *list2);


/* ======================= Verify a format_arg_list ======================= */

/* Verify some invariants.  */
static void
verify_element (const struct format_arg * e)
{
  ASSERT (e->repcount > 0);
  if (e->type & FAT_ELEMENTWISE)
    verify_list (e->list);
}

/* Verify some invariants.  */
/* Memory effects: none.  */
static void
verify_list (const struct format_arg_list *list)
{
  ASSERT (list->initial.count <= list->initial.allocated);
  {
    size_t total_repcount;

    total_repcount = 0;
    for (size_t i = 0; i < list->initial.count; i++)
      {
        verify_element (&list->initial.element[i]);
        total_repcount += list->initial.element[i].repcount;
      }

    ASSERT (total_repcount == list->initial.length);
  }

  ASSERT (list->repeated.count <= list->repeated.allocated);
  {
    size_t total_repcount;

    total_repcount = 0;
    for (size_t i = 0; i < list->repeated.count; i++)
      {
        verify_element (&list->repeated.element[i]);
        total_repcount += list->repeated.element[i].repcount;
      }

    ASSERT (total_repcount == list->repeated.length);
  }
}

/* Assertion macro.  Could be defined to empty for speed.  */
#define VERIFY_LIST(list) verify_list (list)


/* ======================== Free a format_arg_list ======================== */

/* Free the data belonging to an argument list element.  */
static inline void
free_element (struct format_arg *element)
{
  if (element->type & FAT_ELEMENTWISE)
    free_list (element->list);
}

/* Free an argument list.  */
/* Memory effects: Frees list.  */
static void
free_list (struct format_arg_list *list)
{
  for (size_t i = 0; i < list->initial.count; i++)
    free_element (&list->initial.element[i]);
  if (list->initial.element != NULL)
    free (list->initial.element);

  for (size_t i = 0; i < list->repeated.count; i++)
    free_element (&list->repeated.element[i]);
  if (list->repeated.element != NULL)
    free (list->repeated.element);
}


/* ======================== Copy a format_arg_list ======================== */

/* Copy the data belonging to an argument list element.  */
static inline void
copy_element (struct format_arg *newelement,
              const struct format_arg *oldelement)
{
  newelement->repcount = oldelement->repcount;
  newelement->presence = oldelement->presence;
  newelement->type = oldelement->type;
  if (oldelement->type & FAT_ELEMENTWISE)
    newelement->list = copy_list (oldelement->list);
}

/* Copy an argument list.  */
/* Memory effects: Freshly allocated result.  */
static struct format_arg_list *
copy_list (const struct format_arg_list *list)
{
  VERIFY_LIST (list);

  struct format_arg_list *newlist = XMALLOC (struct format_arg_list);

  newlist->initial.count = newlist->initial.allocated = list->initial.count;
  {
    size_t length = 0;
    if (list->initial.count == 0)
      newlist->initial.element = NULL;
    else
      {
        newlist->initial.element =
          XNMALLOC (newlist->initial.allocated, struct format_arg);
        for (size_t i = 0; i < list->initial.count; i++)
          {
            copy_element (&newlist->initial.element[i],
                          &list->initial.element[i]);
            length += list->initial.element[i].repcount;
          }
      }
    ASSERT (length == list->initial.length);
    newlist->initial.length = length;
  }

  newlist->repeated.count = newlist->repeated.allocated = list->repeated.count;
  {
    size_t length = 0;
    if (list->repeated.count == 0)
      newlist->repeated.element = NULL;
    else
      {
        newlist->repeated.element =
          XNMALLOC (newlist->repeated.allocated, struct format_arg);
        for (size_t i = 0; i < list->repeated.count; i++)
          {
            copy_element (&newlist->repeated.element[i],
                          &list->repeated.element[i]);
            length += list->repeated.element[i].repcount;
          }
      }
    ASSERT (length == list->repeated.length);
    newlist->repeated.length = length;
  }

  VERIFY_LIST (newlist);

  return newlist;
}


/* ===================== Compare two format_arg_lists ===================== */

/* Tests whether two normalized argument constraints are equivalent,
   ignoring the repcount.  */
static bool
equal_element (const struct format_arg * e1, const struct format_arg * e2)
{
  return (e1->presence == e2->presence
          && e1->type == e2->type
          && (e1->type & FAT_ELEMENTWISE ? equal_list (e1->list, e2->list) :
              true));
}

/* Tests whether two normalized argument list constraints are equivalent.  */
/* Memory effects: none.  */
static bool
equal_list (const struct format_arg_list *list1,
            const struct format_arg_list *list2)
{
  VERIFY_LIST (list1);
  VERIFY_LIST (list2);

  {
    size_t n = list1->initial.count;
    if (n != list2->initial.count)
      return false;
    for (size_t i = 0; i < n; i++)
      {
        const struct format_arg * e1 = &list1->initial.element[i];
        const struct format_arg * e2 = &list2->initial.element[i];

        if (!(e1->repcount == e2->repcount && equal_element (e1, e2)))
          return false;
      }
  }
  {
    size_t n = list1->repeated.count;
    if (n != list2->repeated.count)
      return false;
    for (size_t i = 0; i < n; i++)
      {
        const struct format_arg * e1 = &list1->repeated.element[i];
        const struct format_arg * e2 = &list2->repeated.element[i];

        if (!(e1->repcount == e2->repcount && equal_element (e1, e2)))
          return false;
      }
  }

  return true;
}


/* ===================== Incremental memory allocation ===================== */

/* Ensure list->initial.allocated >= newcount.  */
static inline void
ensure_initial_alloc (struct format_arg_list *list, size_t newcount)
{
  if (newcount > list->initial.allocated)
    {
      list->initial.allocated =
        MAX (2 * list->initial.allocated + 1, newcount);
      list->initial.element =
        (struct format_arg *)
        xrealloc (list->initial.element,
                  list->initial.allocated * sizeof (struct format_arg));
    }
}

/* Ensure list->initial.allocated > list->initial.count.  */
static inline void
grow_initial_alloc (struct format_arg_list *list)
{
  if (list->initial.count >= list->initial.allocated)
    {
      list->initial.allocated =
        MAX (2 * list->initial.allocated + 1, list->initial.count + 1);
      list->initial.element =
        (struct format_arg *)
        xrealloc (list->initial.element,
                  list->initial.allocated * sizeof (struct format_arg));
    }
}

/* Ensure list->repeated.allocated >= newcount.  */
static inline void
ensure_repeated_alloc (struct format_arg_list *list, size_t newcount)
{
  if (newcount > list->repeated.allocated)
    {
      list->repeated.allocated =
        MAX (2 * list->repeated.allocated + 1, newcount);
      list->repeated.element =
        (struct format_arg *)
        xrealloc (list->repeated.element,
                  list->repeated.allocated * sizeof (struct format_arg));
    }
}

/* Ensure list->repeated.allocated > list->repeated.count.  */
static inline void
grow_repeated_alloc (struct format_arg_list *list)
{
  if (list->repeated.count >= list->repeated.allocated)
    {
      list->repeated.allocated =
        MAX (2 * list->repeated.allocated + 1, list->repeated.count + 1);
      list->repeated.element =
        (struct format_arg *)
        xrealloc (list->repeated.element,
                  list->repeated.allocated * sizeof (struct format_arg));
    }
}


/* ====================== Normalize a format_arg_list ====================== */

/* Normalize an argument list constraint, assuming all sublists are already
   normalized.  */
/* Memory effects: Destructively modifies list.  */
static void
normalize_outermost_list (struct format_arg_list *list)
{
  /* Step 1: Combine adjacent elements.
     Copy from i to j, keeping 0 <= j <= i.  */
  {
    size_t n = list->initial.count;
    size_t i, j;
    for (i = j = 0; i < n; i++)
      if (j > 0
          && equal_element (&list->initial.element[i],
                            &list->initial.element[j-1]))
        {
          list->initial.element[j-1].repcount +=
            list->initial.element[i].repcount;
          free_element (&list->initial.element[i]);
        }
      else
        {
          if (j < i)
            list->initial.element[j] = list->initial.element[i];
          j++;
        }
    list->initial.count = j;
  }
  {
    size_t n = list->repeated.count;
    size_t i, j;
    for (i = j = 0; i < n; i++)
      if (j > 0
          && equal_element (&list->repeated.element[i],
                            &list->repeated.element[j-1]))
        {
          list->repeated.element[j-1].repcount +=
            list->repeated.element[i].repcount;
          free_element (&list->repeated.element[i]);
        }
      else
        {
          if (j < i)
            list->repeated.element[j] = list->repeated.element[i];
          j++;
        }
    list->repeated.count = j;
  }

  /* Nothing more to be done if the loop segment is empty.  */
  if (list->repeated.count > 0)
    {
      size_t repcount0_extra;

      /* Step 2: Reduce the loop period.  */
      size_t n = list->repeated.count;
      repcount0_extra = 0;
      if (n > 1
          && equal_element (&list->repeated.element[0],
                            &list->repeated.element[n-1]))
        {
          repcount0_extra = list->repeated.element[n-1].repcount;
          n--;
        }
      /* Proceed as if the loop period were n, with
         list->repeated.element[0].repcount incremented by repcount0_extra.  */
      for (size_t m = 2; m <= n / 2; m++)
        if ((n % m) == 0)
          {
            /* m is a divisor of n.  Try to reduce the loop period to n.  */
            bool ok = true;

            for (size_t i = 0; i < n - m; i++)
              if (!((list->repeated.element[i].repcount
                     + (i == 0 ? repcount0_extra : 0)
                     == list->repeated.element[i+m].repcount)
                    && equal_element (&list->repeated.element[i],
                                      &list->repeated.element[i+m])))
                {
                  ok = false;
                  break;
                }
            if (ok)
              {
                for (size_t i = m; i < n; i++)
                  free_element (&list->repeated.element[i]);
                if (n < list->repeated.count)
                  list->repeated.element[m] = list->repeated.element[n];
                list->repeated.count = list->repeated.count - n + m;
                list->repeated.length /= n / m;
                break;
              }
          }
      if (list->repeated.count == 1)
        {
          /* The loop has period 1.  Normalize the repcount.  */
          list->repeated.element[0].repcount = 1;
          list->repeated.length = 1;
        }

      /* Step 3: Roll as much as possible of the initial segment's tail
         into the loop.  */
      if (list->repeated.count == 1)
        {
          if (list->initial.count > 0
              && equal_element (&list->initial.element[list->initial.count-1],
                                &list->repeated.element[0]))
            {
              /* Roll the last element of the initial segment into the loop.
                 Its repcount is irrelevant.  The second-to-last element is
                 certainly different and doesn't need to be considered.  */
              list->initial.length -=
                list->initial.element[list->initial.count-1].repcount;
              free_element (&list->initial.element[list->initial.count-1]);
              list->initial.count--;
            }
        }
      else
        {
          while (list->initial.count > 0
                 && equal_element (&list->initial.element[list->initial.count-1],
                                   &list->repeated.element[list->repeated.count-1]))
            {
              size_t moved_repcount =
                MIN (list->initial.element[list->initial.count-1].repcount,
                     list->repeated.element[list->repeated.count-1].repcount);

              /* Add the element at the start of list->repeated.  */
              if (equal_element (&list->repeated.element[0],
                                 &list->repeated.element[list->repeated.count-1]))
                list->repeated.element[0].repcount += moved_repcount;
              else
                {
                  size_t newcount = list->repeated.count + 1;
                  ensure_repeated_alloc (list, newcount);
                  for (size_t i = newcount - 1; i > 0; i--)
                    list->repeated.element[i] = list->repeated.element[i-1];
                  list->repeated.count = newcount;
                  copy_element (&list->repeated.element[0],
                                &list->repeated.element[list->repeated.count-1]);
                  list->repeated.element[0].repcount = moved_repcount;
                }

              /* Remove the element from the end of list->repeated.  */
              list->repeated.element[list->repeated.count-1].repcount -=
                moved_repcount;
              if (list->repeated.element[list->repeated.count-1].repcount == 0)
                {
                  free_element (&list->repeated.element[list->repeated.count-1]);
                  list->repeated.count--;
                }

              /* Remove the element from the end of list->initial.  */
              list->initial.element[list->initial.count-1].repcount -=
                moved_repcount;
              if (list->initial.element[list->initial.count-1].repcount == 0)
                {
                  free_element (&list->initial.element[list->initial.count-1]);
                  list->initial.count--;
                }
              list->initial.length -= moved_repcount;
            }
        }
    }
}

/* Normalize an argument list constraint.  */
/* Memory effects: Destructively modifies list.  */
static void
normalize_list (struct format_arg_list *list)
{
  VERIFY_LIST (list);

  /* First normalize all elements, recursively.  */
  {
    size_t n = list->initial.count;
    for (size_t i = 0; i < n; i++)
      if (list->initial.element[i].type & FAT_ELEMENTWISE)
        normalize_list (list->initial.element[i].list);
  }
  {
    size_t n = list->repeated.count;
    for (size_t i = 0; i < n; i++)
      if (list->repeated.element[i].type & FAT_ELEMENTWISE)
        normalize_list (list->repeated.element[i].list);
  }

  /* Then normalize the top level list.  */
  normalize_outermost_list (list);

  VERIFY_LIST (list);
}


/* ===================== Unconstrained and empty lists ===================== */

/* It's easier to allocate these on demand, than to be careful not to
   accidentally modify statically allocated lists.  */


/* Create an unconstrained argument list.  */
/* Memory effects: Freshly allocated result.  */
static struct format_arg_list *
make_unconstrained_list ()
{
  struct format_arg_list *list = XMALLOC (struct format_arg_list);
  list->initial.count = 0;
  list->initial.allocated = 0;
  list->initial.element = NULL;
  list->initial.length = 0;
  list->repeated.count = 1;
  list->repeated.allocated = 1;
  list->repeated.element = XNMALLOC (1, struct format_arg);
  list->repeated.element[0].repcount = 1;
  list->repeated.element[0].presence = FCT_OPTIONAL;
  list->repeated.element[0].type = FAT_ANY_TYPE;
  list->repeated.length = 1;

  VERIFY_LIST (list);

  return list;
}


/* Create an empty argument list.  */
/* Memory effects: Freshly allocated result.  */
static struct format_arg_list *
make_empty_list ()
{
  struct format_arg_list *list = XMALLOC (struct format_arg_list);
  list->initial.count = 0;
  list->initial.allocated = 0;
  list->initial.element = NULL;
  list->initial.length = 0;
  list->repeated.count = 0;
  list->repeated.allocated = 0;
  list->repeated.element = NULL;
  list->repeated.length = 0;

  VERIFY_LIST (list);

  return list;
}


/* Test for an empty list.  */
/* Memory effects: none.  */
MAYBE_UNUSED static bool
is_empty_list (const struct format_arg_list *list)
{
  return (list->initial.count == 0 && list->repeated.count == 0);
}


/* ======================== format_arg_list surgery ======================== */

/* Unfold list->repeated m times, where m >= 1.
   Assumes list->repeated.count > 0.  */
/* Memory effects: list is destructively modified.  */
static void
unfold_loop (struct format_arg_list *list, size_t m)
{
  if (m > 1)
    {
      size_t newcount = list->repeated.count * m;
      ensure_repeated_alloc (list, newcount);
      size_t i = list->repeated.count;
      for (size_t k = 1; k < m; k++)
        for (size_t j = 0; j < list->repeated.count; j++)
          {
            copy_element (&list->repeated.element[i], &list->repeated.element[j]);
            i++;
          }
      list->repeated.count = newcount;
      list->repeated.length = list->repeated.length * m;
    }
}

/* Ensure list->initial.length := m, where m >= list->initial.length.
   Assumes list->repeated.count > 0.  */
/* Memory effects: list is destructively modified.  */
static void
rotate_loop (struct format_arg_list *list, size_t m)
{
  if (m == list->initial.length)
    return;

  if (list->repeated.count == 1)
    {
      /* Instead of multiple copies of list->repeated.element[0], a single
         copy with higher repcount is appended to list->initial.  */
      size_t newcount = list->initial.count + 1;
      ensure_initial_alloc (list, newcount);
      size_t i = list->initial.count;
      copy_element (&list->initial.element[i], &list->repeated.element[0]);
      list->initial.element[i].repcount = m - list->initial.length;
      list->initial.count = newcount;
      list->initial.length = m;
    }
  else
    {
      size_t n = list->repeated.length;

      /* Write m = list->initial.length + q * n + r with 0 <= r < n.  */
      size_t q = (m - list->initial.length) / n;
      size_t r = (m - list->initial.length) % n;

      /* Determine how many entries of list->repeated are needed for
         length r.  */
      size_t s;
      size_t t;

      for (t = r, s = 0;
           s < list->repeated.count && t >= list->repeated.element[s].repcount;
           t -= list->repeated.element[s].repcount, s++)
        ;

      /* s must be < list->repeated.count, otherwise r would have been >= n.  */
      ASSERT (s < list->repeated.count);

      /* So we need to add to list->initial:
         q full copies of list->repeated,
         plus the s first elements of list->repeated,
         plus, if t > 0, a splitoff of list->repeated.element[s].  */
      {
        size_t i = list->initial.count;
        size_t newcount = i + q * list->repeated.count + s + (t > 0 ? 1 : 0);
        ensure_initial_alloc (list, newcount);
        for (size_t k = 0; k < q; k++)
          for (size_t j = 0; j < list->repeated.count; j++)
            {
              copy_element (&list->initial.element[i], &list->repeated.element[j]);
              i++;
            }
        for (size_t j = 0; j < s; j++)
          {
            copy_element (&list->initial.element[i], &list->repeated.element[j]);
            i++;
          }
        if (t > 0)
          {
            copy_element (&list->initial.element[i], &list->repeated.element[s]);
            list->initial.element[i].repcount = t;
            i++;
          }
        ASSERT (i == newcount);
        list->initial.count = newcount;
        /* The new length of the initial segment is
           = list->initial.length
             + q * list->repeated.length
             + list->repeated[0..s-1].repcount + t
           = list->initial.length + q * n + r
           = m.
         */
        list->initial.length = m;
      }

      /* And rotate list->repeated.  */
      if (r > 0)
        {
          size_t oldcount = list->repeated.count;
          size_t newcount = list->repeated.count + (t > 0 ? 1 : 0);
          struct format_arg *newelement = XNMALLOC (newcount, struct format_arg);
          size_t i = 0;
          for (size_t j = s; j < oldcount; j++)
            {
              newelement[i] = list->repeated.element[j];
              i++;
            }
          for (size_t j = 0; j < s; j++)
            {
              newelement[i] = list->repeated.element[j];
              i++;
            }
          if (t > 0)
            {
              copy_element (&newelement[oldcount], &newelement[0]);
              newelement[0].repcount -= t;
              newelement[oldcount].repcount = t;
            }
          free (list->repeated.element);
          list->repeated.element = newelement;
          list->repeated.count = newcount;
        }
    }
}


/* Ensure index n in the initial segment falls on a split between elements,
   i.e. if 0 < n < list->initial.length, then n-1 and n are covered by two
   different adjacent elements.  */
/* Memory effects: list is destructively modified.  */
static size_t
initial_splitelement (struct format_arg_list *list, size_t n)
{
  VERIFY_LIST (list);

  if (n > list->initial.length)
    {
      ASSERT (list->repeated.count > 0);
      rotate_loop (list, n);
      ASSERT (n <= list->initial.length);
    }

  /* Determine how many entries of list->initial need to be skipped.  */
  size_t s;
  size_t t;
  for (t = n, s = 0;
       s < list->initial.count && t >= list->initial.element[s].repcount;
       t -= list->initial.element[s].repcount, s++)
    ;

  if (t == 0)
    return s;

  ASSERT (s < list->initial.count);

  /* Split the entry into two entries.  */
  size_t oldrepcount = list->initial.element[s].repcount;
  size_t newcount = list->initial.count + 1;
  ensure_initial_alloc (list, newcount);
  for (size_t i = list->initial.count - 1; i > s; i--)
    list->initial.element[i+1] = list->initial.element[i];
  copy_element (&list->initial.element[s+1], &list->initial.element[s]);
  list->initial.element[s].repcount = t;
  list->initial.element[s+1].repcount = oldrepcount - t;
  list->initial.count = newcount;

  VERIFY_LIST (list);

  return s+1;
}


/* Ensure index n in the initial segment is not shared.  Return its index.  */
/* Memory effects: list is destructively modified.  */
MAYBE_UNUSED static size_t
initial_unshare (struct format_arg_list *list, size_t n)
{
  /* This does the same side effects as
       initial_splitelement (list, n);
       initial_splitelement (list, n + 1);
   */

  VERIFY_LIST (list);

  if (n >= list->initial.length)
    {
      ASSERT (list->repeated.count > 0);
      rotate_loop (list, n + 1);
      ASSERT (n < list->initial.length);
    }

  /* Determine how many entries of list->initial need to be skipped.  */
  size_t s;
  size_t t;
  for (t = n, s = 0;
       s < list->initial.count && t >= list->initial.element[s].repcount;
       t -= list->initial.element[s].repcount, s++)
    ;

  /* s must be < list->initial.count.  */
  ASSERT (s < list->initial.count);

  if (list->initial.element[s].repcount > 1)
    {
      /* Split the entry into at most three entries: for indices < n,
         for index n, and for indices > n.  */
      size_t oldrepcount = list->initial.element[s].repcount;
      size_t newcount =
        list->initial.count + (t == 0 || t == oldrepcount - 1 ? 1 : 2);
      ensure_initial_alloc (list, newcount);
      if (t == 0 || t == oldrepcount - 1)
        {
          for (size_t i = list->initial.count - 1; i > s; i--)
            list->initial.element[i+1] = list->initial.element[i];
          copy_element (&list->initial.element[s+1], &list->initial.element[s]);
          if (t == 0)
            {
              list->initial.element[s].repcount = 1;
              list->initial.element[s+1].repcount = oldrepcount - 1;
            }
          else
            {
              list->initial.element[s].repcount = oldrepcount - 1;
              list->initial.element[s+1].repcount = 1;
            }
        }
      else
        {
          for (size_t i = list->initial.count - 1; i > s; i--)
            list->initial.element[i+2] = list->initial.element[i];
          copy_element (&list->initial.element[s+2], &list->initial.element[s]);
          copy_element (&list->initial.element[s+1], &list->initial.element[s]);
          list->initial.element[s].repcount = t;
          list->initial.element[s+1].repcount = 1;
          list->initial.element[s+2].repcount = oldrepcount - 1 - t;
        }
      list->initial.count = newcount;
      if (t > 0)
        s++;
    }

  /* Now the entry for index n has repcount 1.  */
  ASSERT (list->initial.element[s].repcount == 1);

  VERIFY_LIST (list);

  return s;
}


/* ================= Intersection of two format_arg_lists ================= */

/* Create the intersection (i.e. combined constraints) of two argument
   constraints.  Return false if the intersection is empty, i.e. if the
   two constraints give a contradiction.  */
/* Memory effects: Freshly allocated element's sublist.  */
static bool
make_intersected_element (struct format_arg *re,
                          const struct format_arg * e1,
                          const struct format_arg * e2)
{
  /* Intersect the cdr types.  */
  if (e1->presence == FCT_REQUIRED || e2->presence == FCT_REQUIRED)
    re->presence = FCT_REQUIRED;
  else
    re->presence = FCT_OPTIONAL;

  /* Intersect the arg types.  */
  if (e1->type == FAT_ANY_TYPE)
    {
      re->type = e2->type;
      if (e2->type & FAT_ELEMENTWISE)
        re->list = copy_list (e2->list);
    }
  else if (e2->type == FAT_ANY_TYPE)
    {
      re->type = e1->type;
      if (e1->type & FAT_ELEMENTWISE)
        re->list = copy_list (e1->list);
    }
  else if (e1->type & e2->type & FAT_ELEMENTWISE)
    {
      if ((e1->type == FAT_ELEMENTWISE_1 && e2->type == FAT_ELEMENTWISE_1)
          || (e1->type == FAT_ELEMENTWISE_2 && e2->type == FAT_ELEMENTWISE_2))
        {
          re->type = e1->type;
          re->list = make_intersected_list (copy_list (e1->list),
                                            copy_list (e2->list));
          if (re->list == NULL)
            return false;
        }
      else
        return false;
    }
  else
    {
      re->type = e1->type & e2->type;
      if (re->type == FAT_NONE)
        return false;
      if (e1->type & FAT_ELEMENTWISE)
        {
          re->type |= FAT_ELEMENTWISE;
          re->list = copy_list (e1->list);
        }
      else if (e2->type & FAT_ELEMENTWISE)
        {
          re->type |= FAT_ELEMENTWISE;
          re->list = copy_list (e2->list);
        }
    }

  return true;
}

/* Append list->repeated to list->initial, and clear list->repeated.  */
/* Memory effects: list is destructively modified.  */
static void
append_repeated_to_initial (struct format_arg_list *list)
{
  if (list->repeated.count > 0)
    {
      /* Move list->repeated over to list->initial.  */
      size_t newcount = list->initial.count + list->repeated.count;
      ensure_initial_alloc (list, newcount);
      size_t i = list->initial.count;
      for (size_t j = 0; j < list->repeated.count; j++)
        {
          list->initial.element[i] = list->repeated.element[j];
          i++;
        }
      list->initial.count = newcount;
      list->initial.length = list->initial.length + list->repeated.length;
      free (list->repeated.element);
      list->repeated.element = NULL;
      list->repeated.allocated = 0;
      list->repeated.count = 0;
      list->repeated.length = 0;
    }
}

/* Handle a contradiction during building of a format_arg_list.
   The list consists only of an initial segment.  The repeated segment is
   empty.  This function searches the last FCT_OPTIONAL and cuts off the
   list at this point, or - if none is found - returns NULL.  */
/* Memory effects: list is destructively modified.  If NULL is returned,
   list is freed.  */
static struct format_arg_list *
backtrack_in_initial (struct format_arg_list *list)
{
  ASSERT (list->repeated.count == 0);

  while (list->initial.count > 0)
    {
      size_t i = list->initial.count - 1;
      if (list->initial.element[i].presence == FCT_REQUIRED)
        {
          /* Throw away this element.  */
          list->initial.length -= list->initial.element[i].repcount;
          free_element (&list->initial.element[i]);
          list->initial.count = i;
        }
      else /* list->initial.element[i].presence == FCT_OPTIONAL */
        {
          /* The list must end here.  */
          list->initial.length--;
          if (list->initial.element[i].repcount > 1)
            list->initial.element[i].repcount--;
          else
            {
              free_element (&list->initial.element[i]);
              list->initial.count = i;
            }
          VERIFY_LIST (list);
          return list;
        }
    }

  free_list (list);
  return NULL;
}

/* Create the intersection (i.e. combined constraints) of two argument list
   constraints.  Free both argument lists when done.  Return NULL if the
   intersection is empty, i.e. if the two constraints give a contradiction.  */
/* Memory effects: list1 and list2 are freed.  The result, if non-NULL, is
   freshly allocated.  */
static struct format_arg_list *
make_intersected_list (struct format_arg_list *list1,
                       struct format_arg_list *list2)
{
  struct format_arg_list *result;

  VERIFY_LIST (list1);
  VERIFY_LIST (list2);

  if (list1->repeated.length > 0 && list2->repeated.length > 0)
    /* Step 1: Ensure list1->repeated.length == list2->repeated.length.  */
    {
      size_t n1 = list1->repeated.length;
      size_t n2 = list2->repeated.length;
      size_t g = gcd (n1, n2);
      size_t m1 = n2 / g; /* = lcm(n1,n2) / n1 */
      size_t m2 = n1 / g; /* = lcm(n1,n2) / n2 */

      unfold_loop (list1, m1);
      unfold_loop (list2, m2);
      /* Now list1->repeated.length = list2->repeated.length = lcm(n1,n2).  */
    }

  if (list1->repeated.length > 0 || list2->repeated.length > 0)
    /* Step 2: Ensure the initial segment of the result can be computed
       from the initial segments of list1 and list2.  If both have a
       repeated segment, this means to ensure
       list1->initial.length == list2->initial.length.  */
    {
      size_t m = MAX (list1->initial.length, list2->initial.length);

      if (list1->repeated.length > 0)
        rotate_loop (list1, m);
      if (list2->repeated.length > 0)
        rotate_loop (list2, m);
    }

  if (list1->repeated.length > 0 && list2->repeated.length > 0)
    {
      ASSERT (list1->initial.length == list2->initial.length);
      ASSERT (list1->repeated.length == list2->repeated.length);
    }

  /* Step 3: Allocate the result.  */
  result = XMALLOC (struct format_arg_list);
  result->initial.count = 0;
  result->initial.allocated = 0;
  result->initial.element = NULL;
  result->initial.length = 0;
  result->repeated.count = 0;
  result->repeated.allocated = 0;
  result->repeated.element = NULL;
  result->repeated.length = 0;

  /* Step 4: Elementwise intersection of list1->initial, list2->initial.  */
  {
    struct format_arg *e1 = list1->initial.element;
    size_t c1 = list1->initial.count;
    struct format_arg *e2 = list2->initial.element;
    size_t c2 = list2->initial.count;
    while (c1 > 0 && c2 > 0)
      {
        /* Ensure room in result->initial.  */
        grow_initial_alloc (result);
        struct format_arg *re = &result->initial.element[result->initial.count];
        re->repcount = MIN (e1->repcount, e2->repcount);

        /* Intersect the argument types.  */
        if (!make_intersected_element (re, e1, e2))
          {
            /* If re->presence == FCT_OPTIONAL, the result list ends here.  */
            if (re->presence == FCT_REQUIRED)
              /* Contradiction.  Backtrack.  */
              result = backtrack_in_initial (result);
            goto done;
          }

        result->initial.count++;
        result->initial.length += re->repcount;

        e1->repcount -= re->repcount;
        if (e1->repcount == 0)
          {
            e1++;
            c1--;
          }
        e2->repcount -= re->repcount;
        if (e2->repcount == 0)
          {
            e2++;
            c2--;
          }
      }

    if (list1->repeated.count == 0 && list2->repeated.count == 0)
      {
        /* Intersecting two finite lists.  */
        if (c1 > 0)
          {
            /* list1 longer than list2.  */
            if (e1->presence == FCT_REQUIRED)
              /* Contradiction.  Backtrack.  */
              result = backtrack_in_initial (result);
          }
        else if (c2 > 0)
          {
            /* list2 longer than list1.  */
            if (e2->presence == FCT_REQUIRED)
              /* Contradiction.  Backtrack.  */
              result = backtrack_in_initial (result);
          }
        goto done;
      }
    else if (list1->repeated.count == 0)
      {
        /* Intersecting a finite and an infinite list.  */
        ASSERT (c1 == 0);
        if ((c2 > 0 ? e2->presence : list2->repeated.element[0].presence)
            == FCT_REQUIRED)
          /* Contradiction.  Backtrack.  */
          result = backtrack_in_initial (result);
        goto done;
      }
    else if (list2->repeated.count == 0)
      {
        /* Intersecting an infinite and a finite list.  */
        ASSERT (c2 == 0);
        if ((c1 > 0 ? e1->presence : list1->repeated.element[0].presence)
            == FCT_REQUIRED)
          /* Contradiction.  Backtrack.  */
          result = backtrack_in_initial (result);
        goto done;
      }
    /* Intersecting two infinite lists.  */
    ASSERT (c1 == 0 && c2 == 0);
  }

  /* Step 5: Elementwise intersection of list1->repeated, list2->repeated.  */
  {
    struct format_arg *e1 = list1->repeated.element;
    size_t c1 = list1->repeated.count;
    struct format_arg *e2 = list2->repeated.element;
    size_t c2 = list2->repeated.count;
    while (c1 > 0 && c2 > 0)
      {
        /* Ensure room in result->repeated.  */
        grow_repeated_alloc (result);
        struct format_arg *re = &result->repeated.element[result->repeated.count];
        re->repcount = MIN (e1->repcount, e2->repcount);

        /* Intersect the argument types.  */
        if (!make_intersected_element (re, e1, e2))
          {
            bool re_is_required = re->presence == FCT_REQUIRED;

            append_repeated_to_initial (result);

            /* If re->presence == FCT_OPTIONAL, the result list ends here.  */
            if (re_is_required)
              /* Contradiction.  Backtrack.  */
              result = backtrack_in_initial (result);

            goto done;
          }

        result->repeated.count++;
        result->repeated.length += re->repcount;

        e1->repcount -= re->repcount;
        if (e1->repcount == 0)
          {
            e1++;
            c1--;
          }
        e2->repcount -= re->repcount;
        if (e2->repcount == 0)
          {
            e2++;
            c2--;
          }
      }
    ASSERT (c1 == 0 && c2 == 0);
  }

 done:
  free_list (list1);
  free_list (list2);
  if (result != NULL)
    {
      /* Undo the loop unfolding and unrolling done above.  */
      normalize_outermost_list (result);
      VERIFY_LIST (result);
    }
  return result;
}


/* Create the intersection of an argument list and the empty list.
   Return NULL if the intersection is empty.  */
/* Memory effects: The result, if non-NULL, is freshly allocated.  */
MAYBE_UNUSED static struct format_arg_list *
make_intersection_with_empty_list (struct format_arg_list *list)
{
#if 0 /* equivalent but slower */
  return make_intersected_list (copy_list (list), make_empty_list ());
#else
  if (list->initial.count > 0
      ? list->initial.element[0].presence == FCT_REQUIRED
      : list->repeated.count > 0
        && list->repeated.element[0].presence == FCT_REQUIRED)
    return NULL;
  else
    return make_empty_list ();
#endif
}


/* Create the intersection of two argument list constraints.  NULL stands
   for an impossible situation, i.e. a contradiction.  */
/* Memory effects: list1 and list2 are freed if non-NULL.  The result,
   if non-NULL, is freshly allocated.  */
MAYBE_UNUSED static struct format_arg_list *
intersection (struct format_arg_list *list1, struct format_arg_list *list2)
{
  if (list1 != NULL)
    {
      if (list2 != NULL)
        return make_intersected_list (list1, list2);
      else
        {
          free_list (list1);
          return NULL;
        }
    }
  else
    {
      if (list2 != NULL)
        {
          free_list (list2);
          return NULL;
        }
      else
        return NULL;
    }
}


/* ===================== Union of two format_arg_lists ===================== */

/* Create the union of an argument list and the empty list.  */
/* Memory effects: list is freed.  The result is freshly allocated.  */
MAYBE_UNUSED static struct format_arg_list *
make_union_with_empty_list (struct format_arg_list *list)
{
  VERIFY_LIST (list);

  if (list->initial.count > 0
      ? list->initial.element[0].presence == FCT_REQUIRED
      : list->repeated.count > 0
        && list->repeated.element[0].presence == FCT_REQUIRED)
    {
      initial_splitelement (list, 1);
      ASSERT (list->initial.count > 0);
      ASSERT (list->initial.element[0].repcount == 1);
      ASSERT (list->initial.element[0].presence == FCT_REQUIRED);
      list->initial.element[0].presence = FCT_OPTIONAL;

      /* We might need to merge list->initial.element[0] and
         list->initial.element[1].  */
      normalize_outermost_list (list);
    }

  VERIFY_LIST (list);

  return list;
}


/* =========== Adding specific constraints to a format_arg_list =========== */


/* Test whether arguments 0..n are required arguments in a list.  */
MAYBE_UNUSED static bool
is_required (const struct format_arg_list *list, size_t n)
{
  size_t t;

  /* We'll check whether the first n+1 presence flags are FCT_REQUIRED.  */
  t = n + 1;

  /* Walk the list->initial segment.  */
  {
    size_t s;

    for (s = 0;
         s < list->initial.count && t >= list->initial.element[s].repcount;
         t -= list->initial.element[s].repcount, s++)
      if (list->initial.element[s].presence != FCT_REQUIRED)
        return false;

    if (t == 0)
      return true;

    if (s < list->initial.count)
      {
        if (list->initial.element[s].presence != FCT_REQUIRED)
          return false;
        else
          return true;
      }
  }

  /* Walk the list->repeated segment.  */
  if (list->repeated.count == 0)
    return false;

  {
    size_t s;

    for (s = 0;
         s < list->repeated.count && t >= list->repeated.element[s].repcount;
         t -= list->repeated.element[s].repcount, s++)
      if (list->repeated.element[s].presence != FCT_REQUIRED)
        return false;

    if (t == 0)
      return true;

    if (s < list->repeated.count)
      {
        if (list->repeated.element[s].presence != FCT_REQUIRED)
          return false;
        else
          return true;
      }
  }

  /* The list->repeated segment consists only of FCT_REQUIRED.  So,
     regardless how many more passes through list->repeated would be
     needed until t becomes 0, the result is true.  */
  return true;
}


/* Add a constraint to an argument list, namely that the arguments 0...n are
   present.  NULL stands for an impossible situation, i.e. a contradiction.  */
/* Memory effects: list is freed.  The result is freshly allocated.  */
static struct format_arg_list *
add_required_constraint (struct format_arg_list *list, size_t n)
{
  if (list == NULL)
    return NULL;

  VERIFY_LIST (list);

  if (list->repeated.count == 0 && list->initial.length <= n)
    {
      /* list is already constrained to have at most length n.
         Contradiction.  */
      free_list (list);
      return NULL;
    }

  initial_splitelement (list, n + 1);

  {
    size_t i = 0;
    for (size_t rest = n + 1; rest > 0; )
      {
        list->initial.element[i].presence = FCT_REQUIRED;
        rest -= list->initial.element[i].repcount;
        i++;
      }
  }

  VERIFY_LIST (list);

  return list;
}


/* Add a constraint to an argument list, namely that the argument n is
   never present.  NULL stands for an impossible situation, i.e. a
   contradiction.  */
/* Memory effects: list is freed.  The result is freshly allocated.  */
static struct format_arg_list *
add_end_constraint (struct format_arg_list *list, size_t n)
{
  if (list == NULL)
    return NULL;

  VERIFY_LIST (list);

  if (list->repeated.count == 0 && list->initial.length <= n)
    /* list is already constrained to have at most length n.  */
    return list;

  size_t s = initial_splitelement (list, n);
  enum format_cdr_type n_presence =
    (s < list->initial.count
     ? /* n < list->initial.length */ list->initial.element[s].presence
     : /* n >= list->initial.length */ list->repeated.element[0].presence);

  for (size_t i = s; i < list->initial.count; i++)
    {
      list->initial.length -= list->initial.element[i].repcount;
      free_element (&list->initial.element[i]);
    }
  list->initial.count = s;

  for (size_t i = 0; i < list->repeated.count; i++)
    free_element (&list->repeated.element[i]);
  if (list->repeated.element != NULL)
    free (list->repeated.element);
  list->repeated.element = NULL;
  list->repeated.allocated = 0;
  list->repeated.count = 0;
  list->repeated.length = 0;

  if (n_presence == FCT_REQUIRED)
    return backtrack_in_initial (list);
  else
    return list;
}


/* Add a constraint to an argument list, namely that the arguments n1..n2
   (n1 <= n2) are of a given list type or (if sublist is NULL) of a given
   non-list type.  NULL stands for an impossible situation, i.e. a
   contradiction.  Assumes a preceding add_required_constraint (list, n2).  */
/* Memory effects: list is freed.  The result is freshly allocated.  */
static struct format_arg_list *
add_type_constraint (struct format_arg_list *list,
                     size_t n1, size_t n2,
                     enum format_arg_type type,
                     struct format_arg_list *sublist)
{
  if (list == NULL)
    return NULL;

  /* Through the previous add_required_constraint, we can assume
     list->initial.length >= n2+1.  */

  struct format_arg newconstraint;
  newconstraint.presence = FCT_OPTIONAL;
  newconstraint.type = type;
  newconstraint.list = sublist;

  size_t s = initial_splitelement (list, n1);
  initial_splitelement (list, n2 + 1);

  /* Modify the elements that represent the indices n1..n2.  */
  size_t n = n1;
  while (n <= n2)
    {
      struct format_arg tmpelement;
      if (!make_intersected_element (&tmpelement,
                                     &list->initial.element[s], &newconstraint))
        {
          list = add_end_constraint (list, n);
          break;
        }
      free_element (&list->initial.element[s]);
      list->initial.element[s].type = tmpelement.type;
      list->initial.element[s].list = tmpelement.list;
      n += list->initial.element[s].repcount;
      s++;
    }

  if (list != NULL)
    VERIFY_LIST (list);

  return list;
}


/* Add a constraint to an argument list, namely that all the arguments
   n, n+1, n+2, ..., if they exist, are of a given list type or (if sublist is
   NULL) of a given non-list type.  NULL stands for an impossible situation,
   i.e. a contradiction.  */
/* Memory effects: list is freed.  The result is freshly allocated.  */
static struct format_arg_list *
add_repeated_opt_type_constraint (struct format_arg_list *list,
                                  size_t n,
                                  enum format_arg_type type,
                                  struct format_arg_list *sublist)
{
  if (list == NULL)
    return NULL;

  struct format_arg newconstraint;
  newconstraint.presence = FCT_OPTIONAL;
  newconstraint.type = type;
  newconstraint.list = sublist;

  /* Modify the initial elements that represent the indices >= n.  */
  {
    size_t s = initial_splitelement (list, n);

    for (; s < list->initial.count; s++)
      {
        struct format_arg tmpelement;
        if (!make_intersected_element (&tmpelement,
                                       &list->initial.element[s], &newconstraint))
          {
            list = add_end_constraint (list, n);
            goto done;
          }
        free_element (&list->initial.element[s]);
        list->initial.element[s].type = tmpelement.type;
        list->initial.element[s].list = tmpelement.list;
        n += list->initial.element[s].repcount;
      }
  }

  /* Modify the repeated elements.  */
  for (size_t s = 0; s < list->repeated.count; s++)
    {
      struct format_arg tmpelement;
      if (!make_intersected_element (&tmpelement,
                                     &list->repeated.element[s], &newconstraint))
        {
          list = add_end_constraint (list, n);
          goto done;
        }
      free_element (&list->repeated.element[s]);
      list->repeated.element[s].type = tmpelement.type;
      list->repeated.element[s].list = tmpelement.list;
      n += list->repeated.element[s].repcount;
    }

 done:
  if (list != NULL)
    VERIFY_LIST (list);

  return list;
}


/* ============= Subroutines used by the format string parser ============= */

static void
add_req_type_constraint (struct format_arg_list **listp,
                         size_t position1, size_t position2,
                         enum format_arg_type type,
                         struct format_arg_list *sublist)
{
  *listp = add_required_constraint (*listp, position2);
  if (type & FAT_ELEMENTWISE)
    {
      ASSERT (sublist != NULL);
      *listp = add_type_constraint (*listp, position1, position2,
                                    type, sublist);
    }
  else
    {
      ASSERT (sublist == NULL);
      *listp = add_type_constraint (*listp, position1, position2, type, NULL);
    }
}


/* ======================= The format string parser ======================= */

#define INVALID_ARGNO_ORDER(directive_number) \
  xasprintf (_("In the directive number %zu, the first argument number is greater than the second argument number."), directive_number)

#define INVALID_COMPOUND_VARARG(directive_number) \
  xasprintf (_("In the directive number %zu, the compound specifier consumes a variable number of arguments."), directive_number)

#define INVALID_COMPOUND_ARGCOUNT(directive_number, num_arguments) \
  xasprintf (_("In the directive number %zu, the compound specifier consumes %zu arguments."), directive_number, num_arguments)

#define INVALID_BAR_OUTSIDE_COMPOUND() \
  xstrdup (_("Found '%|' outside of '%(...%)'."))

#define INVALID_UNTERMINATED_COMPOUND() \
  xstrdup (_("The string ends in the middle of a compound specifier."))

#define INVALID_COMPOUND_DELIMITER(directive_number) \
  xasprintf (_("In the directive number %zu, there is an invalid directive in the delimiter part of a compound specifier."), directive_number)

#define INVALID_NESTING(found_char, notfound_char) \
  xasprintf (_("Found '%%%c' without matching '%%%c'."), found_char, notfound_char)

#define INVALID_ARG_PAST_LAST(directive_number) \
  xasprintf (_("The directive number %zu references an argument after the last argument."), directive_number)

#undef INVALID_INCOMPATIBLE_ARG_TYPES
#define INVALID_INCOMPATIBLE_ARG_TYPES() \
  xstrdup (_("The string refers to some argument in incompatible ways."))

/* Parse a piece of format string, until the matching terminating format
   directive is encountered.
   spec is the global struct spec.
   format is the remainder of the format string.
   It is updated upon valid return.
   compound is true inside a compound specifier.
   fdi is an array to be filled with format directive indicators, or NULL.
   If the format string is invalid, false is returned and *invalid_reason is
   set to an error message explaining why.  */
static bool
parse_upto (struct spec *spec,
            const char **formatp, bool compound,
            char *fdi, char **invalid_reason)
{
  const char *format = *formatp;
  const char *const format_start = format;
  size_t arg_count = 0;

  for (; *format != '\0'; )
    {
      char c = *format++;

      if (c == '%')
        {
          FDI_SET (format - 1, FMTDIR_START);

          /* Count number of directives.  */
          spec->directives++;

          bool likely_intentional = true;

          if (*format == '\0')
            {
              *invalid_reason = INVALID_UNTERMINATED_DIRECTIVE ();
              FDI_SET (format - 1, FMTDIR_ERROR);
              return false;
            }
          if (*format == '%')
            /* A doubled percent-sign.  */
            ;
          else
            {
              /* A directive.  */

              /* Parse position.  */
              size_t first_number = 0;
              size_t second_number = 0;
              bool second_is_last = false;
              if (c_isdigit (*format))
                {
                  const char *f = format;
                  size_t m = 0;

                  do
                    {
                      m = 10 * m + (*f - '0');
                      f++;
                    }
                  while (c_isdigit (*f));

                  if (*f == '$')
                    {
                      if (m == 0)
                        {
                          *invalid_reason = INVALID_ARGNO_0 (spec->directives);
                          FDI_SET (f, FMTDIR_ERROR);
                          return false;
                        }
                      first_number = m;
                      format = ++f;
                    }
                  else if (*f == ':')
                    {
                      f++;
                      if (c_isdigit (*f))
                        {
                          size_t m2 = 0;

                          do
                            {
                              m2 = 10 * m2 + (*f - '0');
                              f++;
                            }
                          while (c_isdigit (*f));

                          if (*f == '$')
                            {
                              if (m2 == 0)
                                {
                                  *invalid_reason = INVALID_ARGNO_0 (spec->directives);
                                  FDI_SET (f, FMTDIR_ERROR);
                                  return false;
                                }
                              if (m > m2)
                                {
                                  *invalid_reason = INVALID_ARGNO_ORDER (spec->directives);
                                  FDI_SET (f, FMTDIR_ERROR);
                                  return false;
                                }
                              first_number = m;
                              second_number = m2;
                              format = ++f;
                            }
                        }
                      else if (*f == '$')
                        {
                          first_number = m;
                          second_is_last = true;
                          format = ++f;
                        }
                    }
                }

              /* Parse flags.  */
              while (*format == ' ' || *format == '+' || *format == '-'
                     || *format == '#' || *format == '0' || *format == '=')
                {
                  if (*format == ' ')
                    likely_intentional = false;
                  format++;
                }

              /* Parse width.  */
              size_t width_number = 0;
              bool width_from_arg = false;
              if (c_isdigit (*format))
                {
                  do format++; while (c_isdigit (*format));
                }
              else if (*format == '*')
                {
                  format++;
                  if (c_isdigit (*format))
                    {
                      const char *f = format;
                      size_t m = 0;

                      do
                        {
                          m = 10 * m + (*f - '0');
                          f++;
                        }
                      while (c_isdigit (*f));

                      if (*f == '$')
                        {
                          if (m == 0)
                            {
                              *invalid_reason = INVALID_WIDTH_ARGNO_0 (spec->directives);
                              FDI_SET (f, FMTDIR_ERROR);
                              return false;
                            }
                          width_number = m;
                          format = ++f;
                        }
                    }
                  if (width_number == 0)
                    width_from_arg = true;
                }

              /* Parse precision.  */
              size_t precision_number = 0;
              bool precision_from_arg = false;
              if (*format == '.')
                {
                  format++;

                  if (c_isdigit (*format))
                    {
                      do format++; while (c_isdigit (*format));
                    }
                  else if (*format == '*')
                    {
                      format++;
                      if (c_isdigit (*format))
                        {
                          const char *f = format;
                          size_t m = 0;

                          do
                            {
                              m = 10 * m + (*f - '0');
                              f++;
                            }
                          while (c_isdigit (*f));

                          if (*f == '$')
                            {
                              if (m == 0)
                                {
                                  *invalid_reason = INVALID_WIDTH_ARGNO_0 (spec->directives);
                                  FDI_SET (f, FMTDIR_ERROR);
                                  return false;
                                }
                              precision_number = m;
                              format = ++f;
                            }
                        }
                      if (precision_number == 0)
                        precision_from_arg = true;
                    }
                }

              /* Parse separator.  */
              bool separator_digits_from_arg = false;
              bool separator_char_from_arg = false;
              if (*format == ',')
                {
                  format++;

                  if (c_isdigit (*format))
                    {
                      do format++; while (c_isdigit (*format));
                    }
                  else if (*format == '*')
                    {
                      format++;
                      separator_digits_from_arg = true;
                    }

                  if (*format == '?')
                    {
                      format++;
                      separator_char_from_arg = true;
                    }
                }

              enum format_arg_type type;
              struct format_arg_list *elementwise_list = NULL;

              /* Parse specifier.  */
              switch (*format)
                {
                case 's':
                  type = FAT_BOOL | FAT_INTEGER | FAT_FLOATINGPOINT | FAT_CHAR | FAT_ARRAY | FAT_ASSOCIATIVE | FAT_IRANGE | FAT_STRUCT | FAT_POINTER;
                  break;
                case 'c':
                  type = FAT_CHAR;
                  break;
                case 'd': case 'u': case 'b': case 'o':
                  type = FAT_BOOL | FAT_INTEGER | FAT_CHAR;
                  break;
                case 'x': case 'X':
                  type = FAT_BOOL | FAT_INTEGER | FAT_CHAR | FAT_POINTER;
                  break;
                case 'e': case 'E': case 'f': case 'F':
                case 'g': case 'G': case 'a': case 'A':
                  type = FAT_INTEGER | FAT_FLOATINGPOINT;
                  break;
                case 'r':
                  type = FAT_BOOL | FAT_INTEGER | FAT_FLOATINGPOINT | FAT_CHAR | FAT_ARRAY | FAT_IRANGE;
                  break;
                case '(':
                  /* A compound specifier.  */
                  format++;
                  {
                    struct spec sub_spec;
                    sub_spec.directives = 0;
                    sub_spec.list = make_unconstrained_list ();
                    *formatp = format;
                    if (!parse_upto (&sub_spec, formatp, true, fdi, invalid_reason))
                      {
                        FDI_SET (**formatp == '\0' ? *formatp - 1 : *formatp,
                                 FMTDIR_ERROR);
                        return false;
                      }
                    format = *formatp;
                    elementwise_list = sub_spec.list;
                    if (elementwise_list->repeated.count > 0)
                      {
                        /* Test case: "%(%1:$s%)"  */
                        *invalid_reason = INVALID_COMPOUND_VARARG (spec->directives);
                        FDI_SET (format - 1, FMTDIR_ERROR);
                        return false;
                      }
                    if (elementwise_list->initial.length == 1)
                      type = FAT_ELEMENTWISE_1;
                    else if (elementwise_list->initial.length == 2)
                      type = FAT_ELEMENTWISE_2;
                    else
                      {
                        /* Test case: "%(%s %s %s%)"  */
                        *invalid_reason = INVALID_COMPOUND_ARGCOUNT (spec->directives, elementwise_list->initial.length);
                        FDI_SET (format - 1, FMTDIR_ERROR);
                        return false;
                      }
                  }
                  break;
                case '|':
                  if (!compound)
                    {
                      *invalid_reason = INVALID_BAR_OUTSIDE_COMPOUND ();
                      FDI_SET (format, FMTDIR_ERROR);
                      return false;
                    }
                  /* Parse the second part of a compound specifier.  */
                  format++;
                  for (;;)
                    {
                      if (*format == '\0')
                        {
                          *invalid_reason = INVALID_UNTERMINATED_COMPOUND ();
                          FDI_SET (format - 1, FMTDIR_ERROR);
                          return false;
                        }
                      if (*format == '%')
                        {
                          format++;
                          if (*format == '%')
                            format++;
                          else if (*format == ')')
                            break;
                          else
                            {
                              *invalid_reason = INVALID_COMPOUND_DELIMITER (spec->directives);
                              FDI_SET (format, FMTDIR_ERROR);
                              return false;
                            }
                        }
                      else
                        format++;
                    }
                  /* Here (*format == ')').  */
                  FALLTHROUGH;
                case ')':
                  if (!compound)
                    {
                      *invalid_reason = INVALID_NESTING (')', '(');
                      FDI_SET (format, FMTDIR_ERROR);
                      return false;
                    }
                  goto done;
                default:
                  if (*format == '\0')
                    {
                      *invalid_reason = INVALID_UNTERMINATED_DIRECTIVE ();
                      FDI_SET (format - 1, FMTDIR_ERROR);
                    }
                  else
                    {
                      *invalid_reason = INVALID_CONVERSION_SPECIFIER (spec->directives, *format);
                      FDI_SET (format, FMTDIR_ERROR);
                    }
                  return false;
                }

              if (width_number > 0)
                {
                  add_req_type_constraint (&spec->list, width_number - 1, width_number - 1,
                                           FAT_INTEGER, NULL);
                  if (arg_count < width_number)
                    arg_count = width_number;
                }
              else if (width_from_arg)
                {
                  if (arg_count == SIZE_MAX)
                    {
                      *invalid_reason = INVALID_ARG_PAST_LAST (spec->directives);
                      FDI_SET (format, FMTDIR_ERROR);
                      return false;
                    }
                  add_req_type_constraint (&spec->list, arg_count, arg_count,
                                           FAT_INTEGER, NULL);
                  arg_count++;
                }

              if (precision_number > 0)
                {
                  add_req_type_constraint (&spec->list, precision_number - 1, precision_number - 1,
                                           FAT_INTEGER, NULL);
                  if (arg_count < precision_number)
                    arg_count = precision_number;
                }
              else if (precision_from_arg)
                {
                  if (arg_count == SIZE_MAX)
                    {
                      *invalid_reason = INVALID_ARG_PAST_LAST (spec->directives);
                      FDI_SET (format, FMTDIR_ERROR);
                      return false;
                    }
                  add_req_type_constraint (&spec->list, arg_count, arg_count,
                                           FAT_INTEGER, NULL);
                  arg_count++;
                }

              if (separator_digits_from_arg)
                {
                  if (arg_count == SIZE_MAX)
                    {
                      *invalid_reason = INVALID_ARG_PAST_LAST (spec->directives);
                      FDI_SET (format, FMTDIR_ERROR);
                      return false;
                    }
                  add_req_type_constraint (&spec->list, arg_count, arg_count,
                                           FAT_INTEGER, NULL);
                  arg_count++;
                }

              if (separator_char_from_arg)
                {
                  if (arg_count == SIZE_MAX)
                    {
                      *invalid_reason = INVALID_ARG_PAST_LAST (spec->directives);
                      FDI_SET (format, FMTDIR_ERROR);
                      return false;
                    }
                  add_req_type_constraint (&spec->list, arg_count, arg_count,
                                           FAT_CHAR, NULL);
                  arg_count++;
                }

              if (first_number > 0)
                {
                  if (second_number > 0)
                    {
                      add_req_type_constraint (&spec->list, first_number - 1, second_number - 1,
                                               type, elementwise_list);
                      if (arg_count < second_number)
                        arg_count = second_number;
                    }
                  else if (second_is_last)
                    {
                      add_req_type_constraint (&spec->list, first_number - 1, first_number - 1,
                                               type, elementwise_list);
                      spec->list = add_repeated_opt_type_constraint (spec->list, first_number,
                                                                     type, elementwise_list);
                      arg_count = SIZE_MAX;
                    }
                  else
                    {
                      add_req_type_constraint (&spec->list, first_number - 1, first_number - 1,
                                               type, elementwise_list);
                      if (arg_count < first_number)
                        arg_count = first_number;
                    }
                }
              else
                {
                  if (arg_count == SIZE_MAX)
                    {
                      *invalid_reason = INVALID_ARG_PAST_LAST (spec->directives);
                      FDI_SET (format, FMTDIR_ERROR);
                      return false;
                    }
                  add_req_type_constraint (&spec->list, arg_count, arg_count,
                                           type, elementwise_list);
                  arg_count++;
                }

              if (type & FAT_ELEMENTWISE)
                free_list (elementwise_list);
            }

          if (likely_intentional)
            spec->likely_intentional_directives++;
          FDI_SET (format, FMTDIR_END);

          format++;
        }
    }

  if (compound)
    {
      *invalid_reason = INVALID_NESTING ('(', ')');
      return false;
    }

 done:
  *formatp = format;

  /* Extra arguments at the end are not allowed.  */
  if (arg_count != SIZE_MAX)
    {
      spec->list = add_end_constraint (spec->list, arg_count);
      if (spec->list == NULL)
        return false;
    }

  return true;
}


/* ============== Top level format string handling functions ============== */

static void *
format_parse (const char *format, bool translated, char *fdi,
              char **invalid_reason)
{
  struct spec spec;
  spec.directives = 0;
  spec.likely_intentional_directives = 0;
  spec.list = make_unconstrained_list ();

  if (!parse_upto (&spec, &format, false,
                   fdi, invalid_reason))
    /* Invalid format string.  */
    return NULL;

  if (spec.list == NULL)
    {
      /* Contradictory argument type information.  */
      *invalid_reason = INVALID_INCOMPATIBLE_ARG_TYPES ();
      return NULL;
    }

  /* Normalize the result.  */
  normalize_list (spec.list);

  struct spec *result = XMALLOC (struct spec);
  *result = spec;
  return result;
}

static void
format_free (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  free_list (spec->list);
}

static int
format_get_number_of_directives (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  return spec->directives;
}

static bool
format_is_unlikely_intentional (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  return spec->likely_intentional_directives == 0;
}

static bool
format_check (void *msgid_descr, void *msgstr_descr, bool equality,
              formatstring_error_logger_t error_logger, void *error_logger_data,
              const char *pretty_msgid, const char *pretty_msgstr)
{
  struct spec *spec1 = (struct spec *) msgid_descr;
  struct spec *spec2 = (struct spec *) msgstr_descr;

  /* The formatting functions in the D module std.format treat an unused
     argument at the end of the argument list as an error.  Therefore here
     the translator must not omit some of the arguments.
     This could be mitigated in format strings with two or more directives.
     Example:
       "%2$s bought a piece." vs. "%2$s bought %1$d pieces."
     Here the unused argument (argument 1) would not be at the end of the
     argument list.  But this does not help with the more frequent case:
       "a piece" vs. "%d pieces"
     Therefore we recommend the zero-precision workaround in the documentation:
       "%.0sa piece" vs. "%s pieces"
   */
  equality = true;

  bool err = false;

  if (equality)
    {
      if (!equal_list (spec1->list, spec2->list))
        {
          if (error_logger)
            error_logger (error_logger_data,
                          _("format specifications in '%s' and '%s' are not equivalent"),
                          pretty_msgid, pretty_msgstr);
          err = true;
        }
    }
  else
    {
      struct format_arg_list *intersection =
        make_intersected_list (copy_list (spec1->list),
                               copy_list (spec2->list));

      if (!(intersection != NULL
            && (normalize_list (intersection),
                equal_list (intersection, spec2->list))))
        {
          if (error_logger)
            error_logger (error_logger_data,
                          _("format specifications in '%s' are not a subset of those in '%s'"),
                          pretty_msgstr, pretty_msgid);
          err = true;
        }
    }

  return err;
}


struct formatstring_parser formatstring_d =
{
  format_parse,
  format_free,
  format_get_number_of_directives,
  format_is_unlikely_intentional,
  format_check
};


/* ============================= Testing code ============================= */

#ifdef TEST

/* Test program: Print the argument list specification returned by
   format_parse for strings read from standard input.  */

#include <stdio.h>

static void print_list (struct format_arg_list *list);

static void
print_element (struct format_arg *element)
{
  switch (element->presence)
    {
    case FCT_REQUIRED:
      break;
    case FCT_OPTIONAL:
      printf (". ");
      break;
    default:
      abort ();
    }

  if (element->type == FAT_NONE)
    abort ();
  if (element->type & FAT_ELEMENTWISE)
    {
      switch (element->type)
        {
        case FAT_ELEMENTWISE_1:
          printf ("1");
          break;
        case FAT_ELEMENTWISE_2:
          printf ("2");
          break;
        default:
          abort ();
        }
      print_list (element->list);
    }
  else
    {
      if (element->type == FAT_ANY_TYPE)
        printf ("*");
      else
        {
          if (element->type & FAT_BOOL)
            printf ("b");
          if (element->type & FAT_INTEGER)
            printf ("i");
          if (element->type & FAT_FLOATINGPOINT)
            printf ("f");
          if (element->type & FAT_CHAR)
            printf ("c");
          if (element->type & FAT_ARRAY)
            printf ("a");
          if (element->type & FAT_ASSOCIATIVE)
            printf ("@");
          if (element->type & FAT_IRANGE)
            printf ("r");
          if (element->type & FAT_STRUCT)
            printf ("s");
          if (element->type & FAT_POINTER)
            printf ("p");
        }
    }
}

static void
print_list (struct format_arg_list *list)
{
  printf ("(");

  for (size_t i = 0; i < list->initial.count; i++)
    for (size_t j = 0; j < list->initial.element[i].repcount; j++)
      {
        if (i > 0 || j > 0)
          printf (" ");
        print_element (&list->initial.element[i]);
      }

  if (list->repeated.count > 0)
    {
      printf (" |");
      for (size_t i = 0; i < list->repeated.count; i++)
        for (size_t j = 0; j < list->repeated.element[i].repcount; j++)
          {
            printf (" ");
            print_element (&list->repeated.element[i]);
          }
    }

  printf (")");
}

static void
format_print (void *descr)
{
  struct spec *spec = (struct spec *) descr;

  if (spec == NULL)
    {
      printf ("INVALID");
      return;
    }

  print_list (spec->list);
}

int
main ()
{
  for (;;)
    {
      char *line = NULL;
      size_t line_size = 0;
      int line_len = getline (&line, &line_size, stdin);
      if (line_len < 0)
        break;
      if (line_len > 0 && line[line_len - 1] == '\n')
        line[--line_len] = '\0';

      char *invalid_reason = NULL;
      void *descr = format_parse (line, false, NULL, &invalid_reason);

      format_print (descr);
      printf ("\n");
      if (descr == NULL)
        printf ("%s\n", invalid_reason);

      free (invalid_reason);
      free (line);
    }

  return 0;
}

/*
 * For Emacs M-x compile
 * Local Variables:
 * compile-command: "/bin/sh ../libtool --tag=CC --mode=link gcc -o a.out -static -O -g -Wall -I.. -I../gnulib-lib -I../../gettext-runtime/intl -DTEST format-d.c ../gnulib-lib/libgettextlib.la"
 * End:
 */

#endif /* TEST */
