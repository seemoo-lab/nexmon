/* xgettext Go backend.
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
#include "x-go.h"

#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SBR_NO_PREPENDF
#include <error.h>
#include "message.h"
#include "string-desc.h"
#include "xstring-desc.h"
#include "string-buffer-reversed.h"
#include "xgettext.h"
#include "xg-pos.h"
#include "xg-mixed-string.h"
#include "xg-arglist-context.h"
#include "xg-arglist-callshape.h"
#include "xg-arglist-parser.h"
#include "xg-message.h"
#include "if-error.h"
#include "xalloc.h"
#include "str-list.h"
#include "mem-hash-map.h"
#include "gl_map.h"
#include "gl_xmap.h"
#include "gl_hash_map.h"
#include "gl_list.h"
#include "gl_xlist.h"
#include "gl_carray_list.h"
#include "gl_set.h"
#include "gl_xset.h"
#include "gl_hash_set.h"
#include "read-file.h"
#include "unistr.h"
#include "po-charset.h"
#include "gettext.h"

#define _(s) gettext(s)

/* Use tree-sitter.
   Documentation: <https://tree-sitter.github.io/tree-sitter/using-parsers>  */
#include <tree_sitter/api.h>
extern const TSLanguage *tree_sitter_go (void);


/* The Go syntax is defined in https://go.dev/ref/spec.
   String syntax:
   https://go.dev/ref/spec#String_literals
 */

#define DEBUG_GO 0


/* ==================== Preparing for Go type analysis. ==================== */

/* We use Go type analysis, because the most heavily used Go package that
   implements internationalization with PO files is
     github.com/leonelquinteros/gotext
   with a multi-locale API that uses a variable, such as

        localizer := gotext.NewLocale(localedir, language)
        ....
        fmt.Println(localizer.Get("Hello, world!"))

   or

    var localizer_table map[string]*gotext.Locale
    ...
        localizer := localizer_table[lang]
        ...
        fmt.Fprintln(w, localizer.Get("Hello world!"))

   or

        localizer := r.Context().Value(localizerKey).(*gotext.Locale)
        ...
        fmt.Fprintln(w, localizer.Get("Hello world!"))

   and because there are many other Get methods, unrelated to
   internationalization, whose string argument should not be extracted:

        x.Get("key")

   where x is of type url.Values, http.Header, http.Client, and many more.  */

enum go_type_e
{
  unknown          = 0,
  /* bool
     uint8 uint16 uint32 uint64
     int8 int16 int32 int64
     float32 float64
     complex64 complex128
     byte rune
     uint int uintptr
     string
     error
     comparable
     any  */
  predeclared      = 1,
  /* pointer type:  *eltype  */
  pointer          = 2,
  /* array type:    [N]eltype
     or slice type: []eltype  */
  array            = 3,
  /* map type:  map[keytype]eltype  */
  map              = 4,
  /* function type:  func(...) rettype
     or with multiple values:  func(...) (rettype1, ..., rettypeN)  */
  function         = 5,
  /* struct type  */
  go_struct        = 6,
  /* interface type  */
  go_interface     = 7,
  /* channel type  */
  channel          = 8,
  /* other type */
  other            = 9,
  /* indirection to a named type (only during construction)  */
  indirection      = 10
};

/* This struct represents the relevant parts (for type analysis) of a
   Go type.  */
struct go_type
{
  enum go_type_e e;
  union
  {
    /* For pointer, array, map:  */
    struct go_type *eltype;
    /* For function:  */
    struct
    {
      unsigned int n_values;
      struct go_type **values;
    } function_def;
    /* For struct:  */
    struct
    {
      unsigned int n_members;
      struct go_struct_member { const char *name; struct go_type *type; }
        *members;
      unsigned int n_methods;
      unsigned int n_methods_allocated;
      struct go_struct_method { const char *name; struct go_type *type; }
        *methods;
    } struct_def;
    /* For interface:  */
    struct
    {
      unsigned int n_methods;
      struct go_interface_member { const char *name; struct go_type *type; }
        *methods;
      unsigned int n_interfaces;
      struct go_type **interfaces;
    } interface_def;
    /* For indirection:  */
    const char *type_name;
  } u;
};

typedef struct go_type go_type_t;

/* Pre-built 'struct go_type' objects.  */
static go_type_t unknown_type = { unknown, { NULL } };
static go_type_t a_predeclared_type = { predeclared, { NULL } };
MAYBE_UNUSED static go_type_t a_channel_type = { channel, { NULL } };
static go_type_t another_type = { other, { NULL } };

/* Construction of 'struct go_type' objects.  */

static go_type_t *
create_pointer_type (go_type_t *eltype)
{
  go_type_t *result = XMALLOC (struct go_type);
  result->e = pointer;
  result->u.eltype = eltype;
  return result;
}

static go_type_t *
create_array_type (go_type_t *eltype)
{
  go_type_t *result = XMALLOC (struct go_type);
  result->e = array;
  result->u.eltype = eltype;
  return result;
}

static go_type_t *
create_map_type (go_type_t *eltype)
{
  go_type_t *result = XMALLOC (struct go_type);
  result->e = map;
  result->u.eltype = eltype;
  return result;
}

static go_type_t *
create_function_type (unsigned int n_values, struct go_type *values[])
{
  go_type_t *result = XMALLOC (struct go_type);
  go_type_t **heap_values = XNMALLOC (n_values, go_type_t *);

  for (unsigned int i = 0; i < n_values; i++)
    heap_values[i] = values[i];
  result->e = function;
  result->u.function_def.n_values = n_values;
  result->u.function_def.values = heap_values;
  return result;
}

static go_type_t *
create_struct_type (unsigned int n_members, struct go_struct_member members[])
{
  go_type_t *result = XMALLOC (struct go_type);
  struct go_struct_member *heap_members =
    XNMALLOC (n_members, struct go_struct_member);

  for (unsigned int i = 0; i < n_members; i++)
    heap_members[i] = members[i];
  result->e = go_struct;
  result->u.struct_def.n_members = n_members;
  result->u.struct_def.members = heap_members;
  result->u.struct_def.n_methods = 0;
  result->u.struct_def.n_methods_allocated = 0;
  result->u.struct_def.methods = NULL;
  return result;
}

static go_type_t *
create_interface_type (unsigned int n_methods, struct go_interface_member methods[],
                       unsigned int n_interfaces, struct go_type *interfaces[])
{
  go_type_t *result = XMALLOC (struct go_type);
  struct go_interface_member *heap_methods =
    XNMALLOC (n_methods, struct go_interface_member);
  struct go_type **heap_interfaces = XNMALLOC (n_interfaces, struct go_type *);

  for (unsigned int i = 0; i < n_methods; i++)
    heap_methods[i] = methods[i];
  for (unsigned int i = 0; i < n_interfaces; i++)
    heap_interfaces[i] = interfaces[i];
  result->e = go_interface;
  result->u.interface_def.n_methods = n_methods;
  result->u.interface_def.methods = heap_methods;
  result->u.interface_def.n_interfaces = n_interfaces;
  result->u.interface_def.interfaces = heap_interfaces;
  return result;
}

static go_type_t *
create_other_type (const char *name)
{
  (void) name;
  return &another_type;
}

#if DEBUG_GO
/* Print a type in a somewhat readable way.  */
static void
print_type_recurse (go_type_t *type, int maxdepth, FILE *fp)
{
  if (maxdepth == 0)
    {
      fprintf (fp, "...");
      return;
    }
  maxdepth--;
  switch (type->e)
    {
    case unknown:
      fprintf (fp, "unknown");
      break;
    case predeclared:
      fprintf (fp, "predeclared");
      break;
    case pointer:
      fprintf (fp, "*");
      print_type_recurse (type->u.eltype, maxdepth, fp);
      break;
    case array:
      fprintf (fp, "[]");
      print_type_recurse (type->u.eltype, maxdepth, fp);
      break;
    case map:
      fprintf (fp, "map[...]");
      print_type_recurse (type->u.eltype, maxdepth, fp);
      break;
    case function:
      fprintf (fp, "func(...) ");
      if (type->u.function_def.n_values == 1)
        print_type_recurse (type->u.function_def.values[0], maxdepth, fp);
      else
        {
          fprintf (fp, "(");
          for (unsigned int i = 0; i < type->u.function_def.n_values; i++)
            {
              if (i > 0)
                fprintf (fp, ", ");
              print_type_recurse (type->u.function_def.values[i], maxdepth, fp);
            }
          fprintf (fp, ")");
        }
      break;
    case go_struct:
      {
        fprintf (fp, "struct {\n");
        for (unsigned int i = 0; i < type->u.struct_def.n_members; i++)
          {
            fprintf (fp, "  %s ", type->u.struct_def.members[i].name);
            print_type_recurse (type->u.struct_def.members[i].type, maxdepth, fp);
            fprintf (fp, ";\n");
          }
        fprintf (fp, "  -- methods:\n");
        for (unsigned int i = 0; i < type->u.struct_def.n_methods; i++)
          {
            fprintf (fp, "  %s ", type->u.struct_def.methods[i].name);
            print_type_recurse (type->u.struct_def.methods[i].type, maxdepth, fp);
            fprintf (fp, ";\n");
          }
        fprintf (fp, "}\n");
      }
      break;
    case go_interface:
      {
        fprintf (fp, "interface {\n");
        for (unsigned int i = 0; i < type->u.interface_def.n_methods; i++)
          {
            fprintf (fp, "  %s ", type->u.interface_def.methods[i].name);
            print_type_recurse (type->u.interface_def.methods[i].type, maxdepth, fp);
            fprintf (fp, ";\n");
          }
        fprintf (fp, "  -- interfaces:\n");
        for (unsigned int i = 0; i < type->u.interface_def.n_interfaces; i++)
          {
            fprintf (fp, "  ");
            print_type_recurse (type->u.interface_def.interfaces[i], maxdepth, fp);
            fprintf (fp, ";\n");
          }
        fprintf (fp, "}\n");
      }
      break;
    case channel:
      fprintf (fp, "channel");
      break;
    case other:
      fprintf (fp, "other");
      break;
    default:
      abort ();
    }
}
static void
print_type (go_type_t *type, FILE *fp)
{
  print_type_recurse (type, 4, fp);
}
#endif

struct go_package
{
  hash_table /* const char[] -> go_type_t * */ defined_types;
  hash_table /* const char[] -> go_type_t * */ globals;
};

static void
add_to_hash_table (hash_table *htab, const char *name, go_type_t *type)
{
  if (hash_insert_entry (htab, name, strlen (name), type) == NULL)
    /* We have duplicates!  */
    abort ();
}

static void
add_method (go_type_t *recipient_type, const char *name, go_type_t *func_type)
{
  if (recipient_type->e == pointer)
    /* Defining a method on *T is equivalent to defining a method on T.  */
    add_method (recipient_type->u.eltype, name, func_type);
  else if (recipient_type->e == go_struct)
    {
      unsigned int n = recipient_type->u.struct_def.n_methods;
      if (n >= recipient_type->u.struct_def.n_methods_allocated)
        {
          unsigned int new_allocated =
            2 * recipient_type->u.struct_def.n_methods_allocated + 1;
          recipient_type->u.struct_def.methods =
            (struct go_struct_method *)
            xrealloc (recipient_type->u.struct_def.methods,
                      new_allocated * sizeof (struct go_struct_method));
          recipient_type->u.struct_def.n_methods_allocated = new_allocated;
        }
      recipient_type->u.struct_def.methods[n].name = name;
      recipient_type->u.struct_def.methods[n].type = func_type;
      recipient_type->u.struct_def.n_methods = n + 1;
    }
  else
    abort ();
}

/* Full name of package github.com/leonelquinteros/gotext.  */
#define GOTEXT_PACKAGE_FULLNAME "github.com/leonelquinteros/gotext"

/* Known type information for the package github.com/leonelquinteros/gotext.  */
static struct go_package gotext_package;

/* Initializes gotext_package.  */
static void
init_gotext_package (void)
{
  /* Hand-extracted from
     <https://pkg.go.dev/github.com/leonelquinteros/gotext@v1.7.0>  */

  /* Initialize the hash tables.  */
  hash_init (&gotext_package.defined_types, 10);
  hash_init (&gotext_package.globals, 100);

  /* Fill the gotext_package.defined_types table.  */

  struct go_type *string_type = &a_predeclared_type;
  struct go_type *func_returning_string = create_function_type (1, &string_type);

  struct go_type *HeaderMap_type =
    create_map_type (create_array_type (string_type));

  struct go_type *Domain_type;
  struct go_type *Mo_type;
  struct go_type *Po_type;
  {
    struct go_struct_member members[3] =
      {
        { "Headers", HeaderMap_type },
        { "Language", string_type },
        { "PluralForms", string_type }
      };
    Domain_type = create_struct_type (3, members);
    Mo_type = create_struct_type (3, members);
    Po_type = create_struct_type (3, members);
  }
  add_to_hash_table (&gotext_package.defined_types, "Domain", Domain_type);
  struct go_type *pDomain_type = create_pointer_type (Domain_type);
  add_to_hash_table (&gotext_package.defined_types, "Mo", Mo_type);
  struct go_type *pMo_type = create_pointer_type (Mo_type);
  add_to_hash_table (&gotext_package.defined_types, "Po", Po_type);
  struct go_type *pPo_type = create_pointer_type (Po_type);

  struct go_type *Translator_type;
  {
    struct go_interface_member methods[9] =
      {
        { "ParseFile", &unknown_type },
        { "Parse", &unknown_type },
        { "Get", string_type },
        { "GetC", string_type },
        { "GetN", string_type },
        { "GetNC", string_type },
        { "MarshalBinary", &unknown_type },
        { "UnmarshalBinary", &unknown_type },
        { "GetDomain", pDomain_type }
      };
    Translator_type = create_interface_type (9, methods, 0, NULL);
  }
  add_to_hash_table (&gotext_package.defined_types, "Translator", Translator_type);

  struct go_type *Translation_type;
  {
    struct go_struct_member members[4] =
      {
        { "ID", string_type },
        { "PluralID", string_type },
        { "Trs", create_map_type (string_type) },
        { "Refs", create_array_type (string_type) }
      };
    Translation_type = create_struct_type (4, members);
  }
  add_to_hash_table (&gotext_package.defined_types, "Translation", Translation_type);
  struct go_type *pTranslation_type = create_pointer_type (Translation_type);

  struct go_type *Locale_type;
  {
    struct go_struct_member members[2] =
      {
        { "Domains", create_map_type (Translator_type) },
        { "RWMutex", create_other_type ("sync.RWMutex") }
      };
    Locale_type = create_struct_type (2, members);
  }
  add_to_hash_table (&gotext_package.defined_types, "Locale", Locale_type);
  struct go_type *pLocale_type = create_pointer_type (Locale_type);
  struct go_type *apLocale_type = create_array_type (pLocale_type);

  /* Fill the gotext_package.globals table and
     insert methods on non-interface types.  */

  add_to_hash_table (&gotext_package.globals, "Get", func_returning_string);
  add_to_hash_table (&gotext_package.globals, "GetC", func_returning_string);
  add_to_hash_table (&gotext_package.globals, "GetD", func_returning_string);
  add_to_hash_table (&gotext_package.globals, "GetDC", func_returning_string);
  add_to_hash_table (&gotext_package.globals, "GetN", func_returning_string);
  add_to_hash_table (&gotext_package.globals, "GetNC", func_returning_string);
  add_to_hash_table (&gotext_package.globals, "GetND", func_returning_string);
  add_to_hash_table (&gotext_package.globals, "GetNDC", func_returning_string);

  add_to_hash_table (&gotext_package.globals, "NewDomain",
                     create_function_type (1, &pDomain_type));
  add_method (pDomain_type, "Get", func_returning_string);
  add_method (pDomain_type, "GetC", func_returning_string);
  add_method (pDomain_type, "GetN", func_returning_string);
  add_method (pDomain_type, "GetNC", func_returning_string);
  add_method (pDomain_type, "GetTranslations",
              create_map_type (pTranslation_type));

  add_to_hash_table (&gotext_package.globals, "NewMo",
                     create_function_type (1, &pMo_type));
  add_to_hash_table (&gotext_package.globals, "NewMoFS",
                     create_function_type (1, &pMo_type));
  add_method (pMo_type, "Get", func_returning_string);
  add_method (pMo_type, "GetC", func_returning_string);
  add_method (pMo_type, "GetN", func_returning_string);
  add_method (pMo_type, "GetNC", func_returning_string);
  add_method (pMo_type, "GetDomain", pDomain_type);

  add_to_hash_table (&gotext_package.globals, "NewPo",
                     create_function_type (1, &pPo_type));
  add_to_hash_table (&gotext_package.globals, "NewPoFS",
                     create_function_type (1, &pPo_type));
  add_method (pPo_type, "Get", func_returning_string);
  add_method (pPo_type, "GetC", func_returning_string);
  add_method (pPo_type, "GetN", func_returning_string);
  add_method (pPo_type, "GetNC", func_returning_string);
  add_method (pPo_type, "GetDomain", pDomain_type);

  add_to_hash_table (&gotext_package.globals, "NewTranslation",
                     create_function_type (1, &pTranslation_type));
  add_to_hash_table (&gotext_package.globals, "NewTranslationWithRefs",
                     create_function_type (1, &pTranslation_type));
  add_method (pTranslation_type, "Get", func_returning_string);
  add_method (pTranslation_type, "GetN", func_returning_string);

  add_to_hash_table (&gotext_package.globals, "GetLocales",
                     create_function_type (1, &apLocale_type));
  add_to_hash_table (&gotext_package.globals, "NewLocale",
                     create_function_type (1, &pLocale_type));
  add_to_hash_table (&gotext_package.globals, "NewLocaleFS",
                     create_function_type (1, &pLocale_type));
  add_to_hash_table (&gotext_package.globals, "NewLocaleFSWithPath",
                     create_function_type (1, &pLocale_type));
  add_method (pLocale_type, "Get", func_returning_string);
  add_method (pLocale_type, "GetC", func_returning_string);
  add_method (pLocale_type, "GetD", func_returning_string);
  add_method (pLocale_type, "GetDC", func_returning_string);
  add_method (pLocale_type, "GetN", func_returning_string);
  add_method (pLocale_type, "GetNC", func_returning_string);
  add_method (pLocale_type, "GetND", func_returning_string);
  add_method (pLocale_type, "GetNDC", func_returning_string);
  add_method (pLocale_type, "GetDomain", func_returning_string);
  add_method (pLocale_type, "GetTranslations",
              create_map_type (pTranslation_type));
}

/* Full name of package github.com/snapcore/go-gettext.  */
#define SNAPCORE_PACKAGE_FULLNAME "github.com/snapcore/go-gettext"
/* Short name of that package.  */
#define SNAPCORE_PACKAGE_SHORTNAME "gettext"

/* Known type information for the package github.com/snapcore/go-gettext.  */
static struct go_package snapcore_package;

/* Initializes snapcore_package.  */
static void
init_snapcore_package (void)
{
  /* Hand-extracted from
     <https://pkg.go.dev/github.com/snapcore/go-gettext>  */

  /* Initialize the hash tables.  */
  hash_init (&snapcore_package.defined_types, 10);
  hash_init (&snapcore_package.globals, 100);

  /* Fill the snapcore_package.defined_types table.  */

  struct go_type *string_type = &a_predeclared_type;
  struct go_type *func_returning_string = create_function_type (1, &string_type);

  struct go_type *Catalog_type = create_struct_type (0, NULL);
  add_to_hash_table (&snapcore_package.defined_types, "Catalog", Catalog_type);

  struct go_type *TextDomain_type;
  {
    struct go_struct_member members[2] =
      {
        { "Name", string_type },
        { "LocaleDir", string_type }
      };
    TextDomain_type = create_struct_type (2, members);
  }
  add_to_hash_table (&snapcore_package.defined_types, "TextDomain", TextDomain_type);
  struct go_type *pTextDomain_type = create_pointer_type (TextDomain_type);

  /* Fill the snapcore_package.globals table and
     insert methods on non-interface types.  */

  add_method (Catalog_type, "Gettext", func_returning_string);
  add_method (Catalog_type, "NGettext", func_returning_string);
  add_method (Catalog_type, "PGettext", func_returning_string);
  add_method (Catalog_type, "NPGettext", func_returning_string);

  add_method (pTextDomain_type, "Locale",
              create_function_type (1, &Catalog_type));
  add_method (pTextDomain_type, "UserLocale",
              create_function_type (1, &Catalog_type));
}


/* ====================== Keyword set customization.  ====================== */

/* If true extract all strings.  */
static bool extract_all = false;

/* For extracting calls like NAME (...).  */
static hash_table /* string -> struct callshapes * */ keywords;
/* For extracting calls like gotext.NAME (...).  */
static hash_table /* string -> struct callshapes * */ gotext_keywords;
/* For extracting calls like gettext.NAME (...).  */
static hash_table /* string -> struct callshapes * */ snapcore_keywords;
/* For extracting calls like gotext.TYPE.NAME (...).  */
static gl_map_t /* go_type_t * -> hash_table (string -> struct callshapes *) * */ gotext_type_keywords;
/* For extracting calls like gettext.TYPE.NAME (...).  */
static gl_map_t /* go_type_t * -> hash_table (string -> struct callshapes *) * */ snapcore_type_keywords;
static bool default_keywords = true;


void
x_go_extract_all ()
{
  extract_all = true;
}


void
x_go_keyword (const char *name)
{
  if (name == NULL)
    default_keywords = false;
  else
    {
      if (keywords.table == NULL)
        {
          hash_init (&keywords, 100);
          hash_init (&gotext_keywords, 100);
          hash_init (&snapcore_keywords, 100);
          gotext_type_keywords = gl_map_create_empty (GL_HASH_MAP, NULL, NULL, NULL, NULL);
          snapcore_type_keywords = gl_map_create_empty (GL_HASH_MAP, NULL, NULL, NULL, NULL);
        }

      const char *end;
      struct callshape shape;
      split_keywordspec (name, &end, &shape);

      /* A colon means an invalid parse in split_keywordspec().  */
      const char *colon = strchr (name, ':');
      if (colon == NULL || colon >= end)
        {
          /* The characters between name and end should form
               - either a valid Go identifier,
               - or a PACKAGE . FUNCNAME,
               - or a PACKAGE . TYPENAME . METHODNAME.  */
          const char *last_slash = strrchr (name, '/');
          if (last_slash != NULL && last_slash < end)
            {
              const char *first_dot = strchr (last_slash + 1, '.');
              if (first_dot != NULL && first_dot < end)
                {
                  const char *second_dot = strchr (first_dot + 1, '.');
                  if (second_dot != NULL && second_dot < end)
                    {
                      /* Looks like NAME is PACKAGE . TYPENAME . METHODNAME.  */
                      /* We are only interested in the gotext and snapcore packages.  */
                      if (first_dot - name == strlen (GOTEXT_PACKAGE_FULLNAME)
                          && memcmp (name, GOTEXT_PACKAGE_FULLNAME, strlen (GOTEXT_PACKAGE_FULLNAME)) == 0)
                        {
                          void *found_type;
                          if (hash_find_entry (&gotext_package.defined_types,
                                               first_dot + 1, second_dot - (first_dot + 1),
                                               &found_type)
                              == 0)
                            {
                              go_type_t *gotext_type = (go_type_t *) found_type;
                              hash_table *ht = (hash_table *) gl_map_get (gotext_type_keywords, gotext_type);
                              if (ht == NULL)
                                {
                                  ht = XMALLOC (hash_table);
                                  hash_init (ht, 100);
                                  gl_map_put (gotext_type_keywords, gotext_type, ht);
                                }
                              insert_keyword_callshape (ht,
                                                        second_dot + 1, end - (second_dot + 1),
                                                        &shape);
                            }
                        }
                      else if (first_dot - name == strlen (SNAPCORE_PACKAGE_FULLNAME)
                               && memcmp (name, SNAPCORE_PACKAGE_FULLNAME, strlen (SNAPCORE_PACKAGE_FULLNAME)) == 0)
                        {
                          void *found_type;
                          if (hash_find_entry (&snapcore_package.defined_types,
                                               first_dot + 1, second_dot - (first_dot + 1),
                                               &found_type)
                              == 0)
                            {
                              go_type_t *snapcore_type = (go_type_t *) found_type;
                              hash_table *ht = (hash_table *) gl_map_get (snapcore_type_keywords, snapcore_type);
                              if (ht == NULL)
                                {
                                  ht = XMALLOC (hash_table);
                                  hash_init (ht, 100);
                                  gl_map_put (snapcore_type_keywords, snapcore_type, ht);
                                }
                              insert_keyword_callshape (ht,
                                                        second_dot + 1, end - (second_dot + 1),
                                                        &shape);
                            }
                        }
                    }
                  else
                    {
                      /* Looks like NAME is PACKAGE . FUNCNAME.  */
                      /* We are only interested in the gotext and snapcore packages.  */
                      if (first_dot - name == strlen (GOTEXT_PACKAGE_FULLNAME)
                          && memcmp (name, GOTEXT_PACKAGE_FULLNAME, strlen (GOTEXT_PACKAGE_FULLNAME)) == 0)
                        insert_keyword_callshape (&gotext_keywords,
                                                  first_dot + 1, end - (first_dot + 1),
                                                  &shape);
                      else if (first_dot - name == strlen (SNAPCORE_PACKAGE_FULLNAME)
                               && memcmp (name, SNAPCORE_PACKAGE_FULLNAME, strlen (SNAPCORE_PACKAGE_FULLNAME)) == 0)
                        insert_keyword_callshape (&snapcore_keywords,
                                                  first_dot + 1, end - (first_dot + 1),
                                                  &shape);
                    }
                }
            }
          else
            /* NAME looks like a valid Go identifier.  */
            insert_keyword_callshape (&keywords, name, end - name, &shape);
        }
    }
}

/* Finish initializing the keywords hash table.
   Called after argument processing, before each file is processed.  */
static void
init_keywords ()
{
  if (default_keywords)
    {
      /* These are the functions defined by the github.com/leonelquinteros/gotext
         package.  */
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Get:1");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".GetC:1,2c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".GetD:2");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".GetDC:2,3c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".GetN:1,2");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".GetNC:1,2,4c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".GetND:2,3");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".GetNDC:2,3,5c");

      /* These are the methods defined on types in the github.com/leonelquinteros/gotext
         package.  */
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Translator.Get:1");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Translator.GetC:1,2c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Translator.GetN:1,2");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Translator.GetNC:1,2,4c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Domain.Get:1");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Domain.GetC:1,2c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Domain.GetN:1,2");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Domain.GetNC:1,2,4c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Mo.Get:1");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Mo.GetC:1,2c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Mo.GetN:1,2");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Mo.GetNC:1,2,4c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Po.Get:1");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Po.GetC:1,2c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Po.GetN:1,2");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Po.GetNC:1,2,4c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Locale.Get:1");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Locale.GetC:1,2c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Locale.GetD:2");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Locale.GetDC:2,3c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Locale.GetN:1,2");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Locale.GetNC:1,2,4c");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Locale.GetND:2,3");
      x_go_keyword (GOTEXT_PACKAGE_FULLNAME ".Locale.GetNDC:2,3,5c");

      /* These are the methods defined on types in the github.com/snapcore/go-gettext
         package.  */
      x_go_keyword (SNAPCORE_PACKAGE_FULLNAME ".Catalog.Gettext:1");
      x_go_keyword (SNAPCORE_PACKAGE_FULLNAME ".Catalog.NGettext:1,2");
      x_go_keyword (SNAPCORE_PACKAGE_FULLNAME ".Catalog.PGettext:1c,2");
      x_go_keyword (SNAPCORE_PACKAGE_FULLNAME ".Catalog.NPGettext:1c,2,3");

      /* These are the functions defined by the github.com/gosexy/gettext package.  */
      /* When adding new keywords here, also update the documentation in
         xgettext.texi!  */
      x_go_keyword ("Gettext:1");
      x_go_keyword ("DGettext:2");
      x_go_keyword ("DCGettext:2");
      x_go_keyword ("NGettext:1,2");
      x_go_keyword ("DNGettext:2,3");
      x_go_keyword ("DCNGettext:2,3");

      default_keywords = false;
    }
}

void
init_flag_table_go ()
{
  /* These are the functions and methods defined by the github.com/leonelquinteros/gotext
     package.  */
  xgettext_record_flag ("Get:1:pass-go-format");
  xgettext_record_flag ("GetC:1:pass-go-format");
  xgettext_record_flag ("GetD:2:pass-go-format");
  xgettext_record_flag ("GetDC:2:pass-go-format");
  xgettext_record_flag ("GetN:1:pass-go-format");
  xgettext_record_flag ("GetN:2:pass-go-format");
  xgettext_record_flag ("GetNC:1:pass-go-format");
  xgettext_record_flag ("GetNC:2:pass-go-format");
  xgettext_record_flag ("GetND:2:pass-go-format");
  xgettext_record_flag ("GetND:3:pass-go-format");
  xgettext_record_flag ("GetNDC:2:pass-go-format");
  xgettext_record_flag ("GetNDC:3:pass-go-format");
  /* These are the functions defined by the github.com/gosexy/gettext
     and github.com/snapcore/go-gettext packages.  */
  xgettext_record_flag ("Gettext:1:pass-go-format");
  xgettext_record_flag ("DGettext:2:pass-go-format");
  xgettext_record_flag ("DCGettext:2:pass-go-format");
  xgettext_record_flag ("NGettext:1:pass-go-format");
  xgettext_record_flag ("NGettext:2:pass-go-format");
  xgettext_record_flag ("DNGettext:2:pass-go-format");
  xgettext_record_flag ("DNGettext:3:pass-go-format");
  xgettext_record_flag ("DCNGettext:2:pass-go-format");
  xgettext_record_flag ("DCNGettext:3:pass-go-format");
  xgettext_record_flag ("PGettext:2:pass-go-format");
  xgettext_record_flag ("NPGettext:2:pass-go-format");
  xgettext_record_flag ("NPGettext:3:pass-go-format");
  /* These are the functions whose argument is a format string.
     https://pkg.go.dev/fmt  */
  xgettext_record_flag ("Sprintf:1:go-format");
  xgettext_record_flag ("Fprintf:2:go-format");
  xgettext_record_flag ("Printf:1:go-format");
}


/* ======================== Parsing via tree-sitter. ======================== */
/* To understand this code, look at
     tree-sitter-go/src/node-types.json
   and
     tree-sitter-go/src/grammar.json
 */

/* The tree-sitter's language object.  */
static const TSLanguage *ts_language;

/* ------------------------- Node types and symbols ------------------------- */

static TSSymbol
ts_language_symbol (const char *name, bool is_named)
{
  TSSymbol result =
    ts_language_symbol_for_name (ts_language, name, strlen (name), is_named);
  if (result == 0)
    /* If we get here, the grammar has evolved in an incompatible way.  */
    abort ();
  return result;
}

static TSFieldId
ts_language_field (const char *name)
{
  TSFieldId result =
    ts_language_field_id_for_name (ts_language, name, strlen (name));
  if (result == 0)
    /* If we get here, the grammar has evolved in an incompatible way.  */
    abort ();
  return result;
}

/* Optimization:
   Instead of
     strcmp (ts_node_type (node), "interpreted_string_literal") == 0
   it is faster to do
     ts_node_symbol (node) == ts_symbol_interpreted_string_literal
 */
static TSSymbol ts_symbol_import_declaration;
static TSSymbol ts_symbol_import_spec_list;
static TSSymbol ts_symbol_import_spec;
static TSSymbol ts_symbol_package_identifier;
static TSSymbol ts_symbol_type_declaration;
static TSSymbol ts_symbol_type_alias;
static TSSymbol ts_symbol_type_spec;
static TSSymbol ts_symbol_type_identifier;
static TSSymbol ts_symbol_generic_type;
static TSSymbol ts_symbol_qualified_type;
static TSSymbol ts_symbol_pointer_type;
static TSSymbol ts_symbol_struct_type;
static TSSymbol ts_symbol_field_declaration_list;
static TSSymbol ts_symbol_field_declaration;
static TSSymbol ts_symbol_interface_type;
static TSSymbol ts_symbol_method_elem;
static TSSymbol ts_symbol_type_elem;
static TSSymbol ts_symbol_array_type;
static TSSymbol ts_symbol_slice_type;
static TSSymbol ts_symbol_map_type;
static TSSymbol ts_symbol_channel_type;
static TSSymbol ts_symbol_function_type;
static TSSymbol ts_symbol_parameter_list;
static TSSymbol ts_symbol_parameter_declaration;
static TSSymbol ts_symbol_variadic_parameter_declaration;
static TSSymbol ts_symbol_negated_type;
static TSSymbol ts_symbol_parenthesized_type;
static TSSymbol ts_symbol_var_declaration;
static TSSymbol ts_symbol_var_spec_list;
static TSSymbol ts_symbol_var_spec;
static TSSymbol ts_symbol_const_declaration;
static TSSymbol ts_symbol_const_spec;
static TSSymbol ts_symbol_short_var_declaration;
static TSSymbol ts_symbol_expression_list;
static TSSymbol ts_symbol_unary_expression;
static TSSymbol ts_symbol_binary_expression;
static TSSymbol ts_symbol_selector_expression;
static TSSymbol ts_symbol_index_expression;
static TSSymbol ts_symbol_slice_expression;
static TSSymbol ts_symbol_call_expression;
static TSSymbol ts_symbol_type_assertion_expression;
static TSSymbol ts_symbol_type_conversion_expression;
static TSSymbol ts_symbol_type_instantiation_expression;
static TSSymbol ts_symbol_composite_literal;
static TSSymbol ts_symbol_func_literal;
static TSSymbol ts_symbol_int_literal;
static TSSymbol ts_symbol_float_literal;
static TSSymbol ts_symbol_imaginary_literal;
static TSSymbol ts_symbol_rune_literal;
static TSSymbol ts_symbol_nil;
static TSSymbol ts_symbol_true;
static TSSymbol ts_symbol_false;
static TSSymbol ts_symbol_iota;
static TSSymbol ts_symbol_parenthesized_expression;
static TSSymbol ts_symbol_function_declaration;
static TSSymbol ts_symbol_for_clause;
static TSSymbol ts_symbol_comment;
static TSSymbol ts_symbol_raw_string_literal;
static TSSymbol ts_symbol_raw_string_literal_content;
static TSSymbol ts_symbol_interpreted_string_literal;
static TSSymbol ts_symbol_interpreted_string_literal_content;
static TSSymbol ts_symbol_escape_sequence;
static TSSymbol ts_symbol_argument_list;
static TSSymbol ts_symbol_identifier;
static TSSymbol ts_symbol_field_identifier;
static TSSymbol ts_symbol_dot; /* . */
static TSSymbol ts_symbol_plus; /* + */
static TSFieldId ts_field_path;
static TSFieldId ts_field_name;
static TSFieldId ts_field_package;
static TSFieldId ts_field_type;
static TSFieldId ts_field_element;
static TSFieldId ts_field_value;
static TSFieldId ts_field_result;
static TSFieldId ts_field_operator;
static TSFieldId ts_field_left;
static TSFieldId ts_field_right;
static TSFieldId ts_field_function;
static TSFieldId ts_field_arguments;
static TSFieldId ts_field_operand;
static TSFieldId ts_field_field;
static TSFieldId ts_field_initializer;

static inline size_t
ts_node_line_number (TSNode node)
{
  return ts_node_start_point (node).row + 1;
}

/* -------------------------------- The file -------------------------------- */

/* The entire contents of the file being analyzed.  */
static const char *contents;

/* ---------------------------- String literals ---------------------------- */

/* Determines whether NODE represents a string literal or the concatenation
   of string literals (via the '+' operator).  */
static bool
is_string_literal (TSNode node)
{
 start:
  if (ts_node_symbol (node) == ts_symbol_raw_string_literal
      || ts_node_symbol (node) == ts_symbol_interpreted_string_literal)
    return true;
  if (ts_node_symbol (node) == ts_symbol_binary_expression
      && ts_node_symbol (ts_node_child_by_field_id (node, ts_field_operator)) == ts_symbol_plus
      /* Recurse into the left and right subnodes.  */
      && is_string_literal (ts_node_child_by_field_id (node, ts_field_right)))
    {
      /*return is_string_literal (ts_node_child_by_field_id (node, ts_field_left));*/
      node = ts_node_child_by_field_id (node, ts_field_left);
      goto start;
    }
  return false;
}

/* Prepends the string literal pieces from NODE to BUFFER.  */
static void
string_literal_accumulate_pieces (TSNode node,
                                  struct string_buffer_reversed *buffer)
{
 start:
  if (ts_node_symbol (node) == ts_symbol_raw_string_literal
      || ts_node_symbol (node) == ts_symbol_interpreted_string_literal)
    {
      uint32_t count = ts_node_named_child_count (node);
      for (uint32_t i = count; i > 0; )
        {
          i--;
          TSNode subnode = ts_node_named_child (node, i);
          if (ts_node_symbol (subnode) == ts_symbol_raw_string_literal_content)
            {
              string_desc_t subnode_string =
                sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                             contents + ts_node_start_byte (subnode));
              /* Eliminate '\r' characters.  */
              for (;;)
                {
                  ptrdiff_t cr_index = sd_last_index (subnode_string, '\r');
                  if (cr_index < 0)
                    break;
                  sbr_xprepend_desc (buffer,
                                     sd_substring (subnode_string, cr_index + 1,
                                                   sd_length (subnode_string)));
                  subnode_string = sd_substring (subnode_string, 0, cr_index);
                }
              sbr_xprepend_desc (buffer, subnode_string);
            }
          else if (ts_node_symbol (subnode) == ts_symbol_interpreted_string_literal_content)
            {
              string_desc_t subnode_string =
                sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                             contents + ts_node_start_byte (subnode));
              sbr_xprepend_desc (buffer, subnode_string);
            }
          else if (ts_node_symbol (subnode) == ts_symbol_escape_sequence)
            {
              const char *escape_start = contents + ts_node_start_byte (subnode);
              const char *escape_end = contents + ts_node_end_byte (subnode);
              /* The escape sequence must start with a backslash.  */
              if (!(escape_end - escape_start >= 2 && escape_start[0] == '\\'))
                abort ();
              /* tree-sitter's grammar.js allows more escape sequences than
                 the Go documentation and the Go compiler.  Give a warning
                 for those case where the Go compiler gives an error.  */
              bool invalid = false;
              if (escape_end - escape_start == 2)
                {
                  switch (escape_start[1])
                    {
                    case '\\':
                    case '"':
                      sbr_xprepend1 (buffer, escape_start[1]);
                      break;
                    case 'a':
                      sbr_xprepend1 (buffer, 0x07);
                      break;
                    case 'b':
                      sbr_xprepend1 (buffer, 0x08);
                      break;
                    case 'f':
                      sbr_xprepend1 (buffer, 0x0C);
                      break;
                    case 'n':
                      sbr_xprepend1 (buffer, '\n');
                      break;
                    case 'r':
                      sbr_xprepend1 (buffer, '\r');
                      break;
                    case 't':
                      sbr_xprepend1 (buffer, '\t');
                      break;
                    case 'v':
                      sbr_xprepend1 (buffer, 0x0B);
                      break;
                    default:
                      invalid = true;
                      break;
                    }
                }
              else if (escape_start[1] >= '0' && escape_start[1] <= '9')
                {
                  unsigned int value = 0;
                  /* Only exactly 3 octal digits are accepted.  */
                  if (escape_end - escape_start == 1 + 3)
                    {
                      for (const char *p = escape_start + 1; p < escape_end; p++)
                        {
                          /* No overflow is possible.  */
                          char c = *p;
                          if (c >= '0' && c <= '7')
                            value = (value << 3) + (c - '0');
                          else
                            invalid = true;
                        }
                      if (value > 0xFF)
                        invalid = true;
                    }
                  if (!invalid)
                    sbr_xprepend1 (buffer, (unsigned char) value);
                }
              else if ((escape_start[1] == 'x' && escape_end - escape_start == 2 + 2)
                       || (escape_start[1] == 'u' && escape_end - escape_start == 2 + 4)
                       || (escape_start[1] == 'U' && escape_end - escape_start == 2 + 8))
                {
                  unsigned int value = 0;
                  for (const char *p = escape_start + 2; p < escape_end; p++)
                    {
                      /* No overflow is possible.  */
                      char c = *p;
                      if (c >= '0' && c <= '9')
                        value = (value << 4) + (c - '0');
                      else if (c >= 'A' && c <= 'Z')
                        value = (value << 4) + (c - 'A' + 10);
                      else if (c >= 'a' && c <= 'z')
                        value = (value << 4) + (c - 'a' + 10);
                      else
                        invalid = true;
                    }
                  if (escape_start[1] == 'x')
                    {
                      if (!invalid)
                        sbr_xprepend1 (buffer, (unsigned char) value);
                    }
                  else
                    {
                      if (value >= 0x110000 || (value >= 0xD800 && value <= 0xDFFF))
                        invalid = true;
                      if (!invalid)
                        {
                          uint8_t buf[6];
                          int n = u8_uctomb (buf, value, sizeof (buf));
                          if (n > 0)
                            sbr_xprepend_desc (buffer, sd_new_addr (n, (const char *) buf));
                          else
                            invalid = true;
                        }
                    }
                }
              else
                invalid = true;
              if (invalid)
                {
                  size_t line_number = ts_node_line_number (subnode);
                  if_error (IF_SEVERITY_WARNING,
                            logical_file_name, line_number, (size_t)(-1), false,
                            _("invalid escape sequence in string"));
                }
            }
          else
            abort ();
        }
    }
  else if (ts_node_symbol (node) == ts_symbol_binary_expression
           && ts_node_symbol (ts_node_child_by_field_id (node, ts_field_operator)) == ts_symbol_plus)
    {
      /* Recurse into the left and right subnodes.  */
      string_literal_accumulate_pieces (ts_node_child_by_field_id (node, ts_field_right), buffer);
      /*string_literal_accumulate_pieces (ts_node_child_by_field_id (node, ts_field_left), buffer);*/
      node = ts_node_child_by_field_id (node, ts_field_left);
      goto start;
    }
  else
    abort ();
}

/* Combines the pieces of a raw_string_literal or interpreted_string_literal
   or concatenated string literal.
   Returns a freshly allocated, mostly UTF-8 encoded string.  */
static char *
string_literal_value (TSNode node)
{
  if (ts_node_symbol (node) == ts_symbol_interpreted_string_literal
      && ts_node_named_child_count (node) == 1)
    {
      TSNode subnode = ts_node_named_child (node, 0);
      if (ts_node_symbol (subnode) == ts_symbol_interpreted_string_literal_content)
        {
          /* Optimize the frequent special case of an interpreted string literal
             that is non-empty and has no escape sequences.  */
          string_desc_t subnode_string =
            sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                         contents + ts_node_start_byte (subnode));
          return xsd_c (subnode_string);
        }
    }

  /* The general case.  */
  struct string_buffer_reversed buffer;
  sbr_init (&buffer);
  string_literal_accumulate_pieces (node, &buffer);
  return sbr_xdupfree_c (&buffer);
}

/* ------------------- Imported packages and their names ------------------- */

/* Table that maps a package_shortname to the full package name.  */
static hash_table /* const char[] -> const char * */ package_table;

/* List of packages whose entities must be accessed without a
   package_shortname.  */
static string_list_ty unqualified_packages;

/* import_spec_node is of type import_spec.  */
static void
scan_import_spec (TSNode import_spec_node)
{
  TSNode path_node = ts_node_child_by_field_id (import_spec_node, ts_field_path);
  if (!is_string_literal (path_node))
    abort ();
  char *path = string_literal_value (path_node);

  TSNode name_node = ts_node_child_by_field_id (import_spec_node, ts_field_name);
  string_desc_t shortname;
  if (ts_node_is_null (name_node))
    {
      /* A package is imported without a name.
         The package_shortname is the last element of the path, except in
         special cases.  */
      if (strcmp (path, SNAPCORE_PACKAGE_FULLNAME) == 0)
        shortname = sd_from_c (SNAPCORE_PACKAGE_SHORTNAME);
      else
        {
          const char *last_slash = strrchr (path, '/');
          shortname = sd_from_c (last_slash != NULL ? last_slash + 1 : path);
        }
    }
  else if (ts_node_symbol (name_node) == ts_symbol_package_identifier)
    {
      /* A package is imported with a name.  */
      shortname =
        sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                     contents + ts_node_start_byte (name_node));
    }
  else
    {
      if (ts_node_symbol (name_node) == ts_symbol_dot)
        {
          /* A package is imported without a package_shortname.  */
          string_list_append (&unqualified_packages, path);
        }
      return;
    }
  hash_set_value (&package_table,
                  sd_data (shortname), sd_length (shortname),
                  path);
}

/* node is of type import_declaration.  */
static void
scan_import_declaration (TSNode node)
{
  uint32_t count = ts_node_named_child_count (node);
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode subnode = ts_node_named_child (node, i);
      if (ts_node_symbol (subnode) == ts_symbol_import_spec_list)
        {
          uint32_t count2 = ts_node_named_child_count (subnode);
          for (uint32_t j = 0; j < count2; j++)
            {
              TSNode subsubnode = ts_node_named_child (subnode, j);
              if (ts_node_symbol (subsubnode) == ts_symbol_import_spec)
                scan_import_spec (subsubnode);
            }
        }
      else if (ts_node_symbol (subnode) == ts_symbol_import_spec)
        scan_import_spec (subnode);
    }
}

/* Initializes current_package.  */
static void
init_package_table (TSNode root_node)
{
  /* Initialize the hash table.  */
  hash_init (&package_table, 50);

  /* Initialize the unqualified packages list.  */
  string_list_init (&unqualified_packages);

  /* Single pass through all top-level import declarations.  */
  {
    uint32_t count = ts_node_named_child_count (root_node);
    for (uint32_t i = 0; i < count; i++)
      {
        TSNode node = ts_node_named_child (root_node, i);
        if (ts_node_symbol (node) == ts_symbol_import_declaration)
          scan_import_declaration (node);
      }
  }
}

/* ----------------- Go type analysis: Tracking local types ----------------- */

/* A type environment consists of type bindings, each with a nested scope.
   The type environment valid outside of functions is represented by NULL.  */

struct type_binding
{
  string_desc_t name;
  go_type_t *type;
};

struct type_env
{
  struct type_env *outer_env;
  struct type_binding a_binding;
};
typedef struct type_env *type_env_t;

/* Augments a type_env_t.  */
static type_env_t
type_env_augment (type_env_t env, string_desc_t name, go_type_t *type)
{
  struct type_env *inner = XMALLOC (struct type_env);
  inner->outer_env = env;
  inner->a_binding.name = name;
  inner->a_binding.type = type;
  return inner;
}

/* --------------------- First pass of Go type analysis --------------------- */

/* Known type information for the file being parsed.  */
/* TODO: The type information should include the entire package, not only
   a single file.  */
static struct go_package current_package;

/* Forward declaration.  */
static go_type_t *get_type_from_type_node (TSNode type_node, type_env_t tenv, bool use_indirections);

/* Returns the type definition of the given type, as a 'go_type_t *'.  */
static go_type_t *
get_type_from_type_name (string_desc_t type_name, type_env_t tenv, bool use_indirections)
{
  if (sd_equals (type_name, sd_from_c ("bool"))
      || sd_equals (type_name, sd_from_c ("uint8"))
      || sd_equals (type_name, sd_from_c ("uint16"))
      || sd_equals (type_name, sd_from_c ("uint32"))
      || sd_equals (type_name, sd_from_c ("uint64"))
      || sd_equals (type_name, sd_from_c ("int8"))
      || sd_equals (type_name, sd_from_c ("int16"))
      || sd_equals (type_name, sd_from_c ("int32"))
      || sd_equals (type_name, sd_from_c ("int64"))
      || sd_equals (type_name, sd_from_c ("float32"))
      || sd_equals (type_name, sd_from_c ("float64"))
      || sd_equals (type_name, sd_from_c ("complex64"))
      || sd_equals (type_name, sd_from_c ("complex128"))
      || sd_equals (type_name, sd_from_c ("byte"))
      || sd_equals (type_name, sd_from_c ("rune"))
      || sd_equals (type_name, sd_from_c ("uint"))
      || sd_equals (type_name, sd_from_c ("int"))
      || sd_equals (type_name, sd_from_c ("uintptr"))
      || sd_equals (type_name, sd_from_c ("string"))
      || sd_equals (type_name, sd_from_c ("error"))
      || sd_equals (type_name, sd_from_c ("comparable"))
      || sd_equals (type_name, sd_from_c ("any")))
    return &a_predeclared_type;
  for (; tenv != NULL; tenv = tenv->outer_env)
    {
      if (sd_equals (type_name, tenv->a_binding.name))
        return tenv->a_binding.type;
    }
  if (use_indirections)
    {
      /* We create an indirection because the type is not yet registered in
         current_package.defined_types.  */
      go_type_t *result = XMALLOC (struct go_type);
      result->e = indirection;
      result->u.type_name = xsd_c (type_name);
      return result;
    }
  else
    {
      /* Look up the type.  */
      void *found_type;
      if (hash_find_entry (&current_package.defined_types,
                           sd_data (type_name), sd_length (type_name),
                           &found_type)
          == 0)
        return (go_type_t *) found_type;
      for (size_t i = 0; i < unqualified_packages.nitems; i++)
        {
          const char *unqualified_package = unqualified_packages.item[i];
          if (strcmp (unqualified_package, GOTEXT_PACKAGE_FULLNAME) == 0)
            {
              if (hash_find_entry (&gotext_package.defined_types,
                                   sd_data (type_name), sd_length (type_name),
                                   &found_type)
                  == 0)
                return (go_type_t *) found_type;
            }
          else if (strcmp (unqualified_package, SNAPCORE_PACKAGE_FULLNAME) == 0)
            {
              if (hash_find_entry (&snapcore_package.defined_types,
                                   sd_data (type_name), sd_length (type_name),
                                   &found_type)
                  == 0)
                return (go_type_t *) found_type;
            }
        }
      return &unknown_type;
    }
}

/* type_node is of type type_identifier.  */
static go_type_t *
get_type_from_type_identifier_node (TSNode type_node, type_env_t tenv, bool use_indirections)
{
  string_desc_t type_name =
    sd_new_addr (ts_node_end_byte (type_node) - ts_node_start_byte (type_node),
                 contents + ts_node_start_byte (type_node));
  return get_type_from_type_name (type_name, tenv, use_indirections);
}

/* type_node is of type function_type or method_elem or function_declaration or func_literal.  */
static go_type_t *
get_type_from_function_or_method_node (TSNode type_node, type_env_t tenv, bool use_indirections)
{
  TSNode result_node = ts_node_child_by_field_id (type_node, ts_field_result);
  if (ts_node_is_null (result_node))
    {
      /* A function without return value.  */
      struct go_type *value_type = &unknown_type;
      return create_function_type (1, &value_type);
    }
  else if (ts_node_symbol (result_node) == ts_symbol_parameter_list)
    {
      /* A function with multiple return values.  */
      uint32_t count = ts_node_named_child_count (result_node);
      unsigned int n_values;
      {
        n_values = 0;
        for (uint32_t i = 0; i < count; i++)
          {
            TSNode subnode = ts_node_named_child (result_node, i);
            if (ts_node_symbol (subnode) == ts_symbol_parameter_declaration)
              n_values++;
          }
      }
      struct go_type **values = XNMALLOC (n_values, struct go_type *);
      {
        unsigned int n = 0;
        for (uint32_t i = 0; i < count; i++)
          {
            TSNode subnode = ts_node_named_child (result_node, i);
            if (ts_node_symbol (subnode) == ts_symbol_parameter_declaration)
              {
                values[n] =
                  get_type_from_type_node (
                    ts_node_child_by_field_id (subnode, ts_field_type),
                    tenv, use_indirections);
                n++;
              }
          }
      }
      struct go_type *function_type = create_function_type (n_values, values);
      free (values);
      return function_type;
    }
  else
    {
      /* A function with a single return value.  */
      struct go_type *value_type =
        get_type_from_type_node (result_node, tenv, use_indirections);
      return create_function_type (1, &value_type);
    }
}

static go_type_t *
get_type_from_type_node (TSNode type_node, type_env_t tenv, bool use_indirections)
{
  #if DEBUG_GO && 0
  string_desc_t type_node_name =
    sd_new_addr (ts_node_end_byte (type_node) - ts_node_start_byte (type_node),
                 contents + ts_node_start_byte (type_node));
  fprintf (stderr, "type_node = [%s]|%s| = %s\n", ts_node_type (type_node), ts_node_string (type_node), sd_c (type_node_name));
  #endif
  while (ts_node_symbol (type_node) == ts_symbol_parenthesized_type
         && ts_node_named_child_count (type_node) == 1)
    {
      type_node = ts_node_named_child (type_node, 0);
    }

  if (ts_node_symbol (type_node) == ts_symbol_type_identifier)
    return get_type_from_type_identifier_node (type_node, tenv, use_indirections);
  else if (ts_node_symbol (type_node) == ts_symbol_qualified_type)
    {
      /* A qualified type is of the form package_shortname.name.  */
      TSNode shortname_node = ts_node_child_by_field_id (type_node, ts_field_package);
      string_desc_t shortname =
        sd_new_addr (ts_node_end_byte (shortname_node) - ts_node_start_byte (shortname_node),
                     contents + ts_node_start_byte (shortname_node));
      /* Look up the package's full name.  */
      void *found_package;
      if (hash_find_entry (&package_table,
                           sd_data (shortname), sd_length (shortname),
                           &found_package)
          == 0)
        {
          if (strcmp ((const char *) found_package, GOTEXT_PACKAGE_FULLNAME) == 0)
            {
              /* Look up the type.  */
              TSNode name_node = ts_node_child_by_field_id (type_node, ts_field_name);
              string_desc_t name =
                sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                             contents + ts_node_start_byte (name_node));
              void *found_type;
              if (hash_find_entry (&gotext_package.defined_types,
                                   sd_data (name), sd_length (name),
                                   &found_type)
                  == 0)
                return (go_type_t *) found_type;
            }
          else if (strcmp ((const char *) found_package, SNAPCORE_PACKAGE_FULLNAME) == 0)
            {
              /* Look up the type.  */
              TSNode name_node = ts_node_child_by_field_id (type_node, ts_field_name);
              string_desc_t name =
                sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                             contents + ts_node_start_byte (name_node));
              void *found_type;
              if (hash_find_entry (&snapcore_package.defined_types,
                                   sd_data (name), sd_length (name),
                                   &found_type)
                  == 0)
                return (go_type_t *) found_type;
            }
        }
      return &unknown_type;
    }
  else if (ts_node_symbol (type_node) == ts_symbol_generic_type)
    /* Ignore the generic type's type arguments.  */
    return get_type_from_type_node (
             ts_node_child_by_field_id (type_node, ts_field_type),
             tenv, use_indirections);
  else if (ts_node_symbol (type_node) == ts_symbol_pointer_type)
    {
      TSNode eltype_node = ts_node_named_child (type_node, 0);
      if (!ts_node_is_null (eltype_node))
        return create_pointer_type (
                 get_type_from_type_node (eltype_node, tenv, use_indirections));
      return &unknown_type;
    }
  else if (ts_node_symbol (type_node) == ts_symbol_struct_type)
    {
      TSNode fdlnode = ts_node_named_child (type_node, 0);
      if (!ts_node_is_null (fdlnode)
          && ts_node_symbol (fdlnode) == ts_symbol_field_declaration_list)
        {
          uint32_t count = ts_node_named_child_count (fdlnode);
          unsigned int n_members;
          {
            n_members = 0;
            for (uint32_t i = 0; i < count; i++)
              {
                TSNode fdnode = ts_node_named_child (fdlnode, i);
                if (ts_node_symbol (fdnode) == ts_symbol_field_declaration)
                  {
                    uint32_t count2 = ts_node_named_child_count (fdnode);
                    for (uint32_t j = 0; j < count2; j++)
                      {
                        TSNode subnode = ts_node_named_child (fdnode, j);
                        if (ts_node_symbol (subnode) == ts_symbol_field_identifier)
                          n_members++;
                      }
                    /* TODO: Handle embedded fields.  */
                  }
              }
          }
          struct go_struct_member *members = XNMALLOC (n_members, struct go_struct_member);
          {
            unsigned int n = 0;
            for (uint32_t i = 0; i < count; i++)
              {
                TSNode fdnode = ts_node_named_child (fdlnode, i);
                if (ts_node_symbol (fdnode) == ts_symbol_field_declaration)
                  {
                    TSNode eltype_node = ts_node_child_by_field_id (fdnode, ts_field_type);
                    go_type_t *eltype =
                      get_type_from_type_node (eltype_node, tenv, use_indirections);
                    uint32_t count2 = ts_node_named_child_count (fdnode);
                    for (uint32_t j = 0; j < count2; j++)
                      {
                        TSNode subnode = ts_node_named_child (fdnode, j);
                        if (ts_node_symbol (subnode) == ts_symbol_field_identifier)
                          {
                            members[n].name =
                              xsd_c (
                                sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                                             contents + ts_node_start_byte (subnode)));
                            members[n].type = eltype;
                            n++;
                          }
                      }
                    /* TODO: Handle embedded fields.  */
                  }
              }
          }
          struct go_type *struct_type = create_struct_type (n_members, members);
          free (members);
          return struct_type;
        }
      return &unknown_type;
    }
  else if (ts_node_symbol (type_node) == ts_symbol_interface_type)
    {
      uint32_t count = ts_node_named_child_count (type_node);
      unsigned int n_methods;
      unsigned int n_interfaces;
      {
        n_methods = 0;
        n_interfaces = 0;
        for (uint32_t i = 0; i < count; i++)
          {
            TSNode subnode = ts_node_named_child (type_node, i);
            if (ts_node_symbol (subnode) == ts_symbol_method_elem)
              n_methods++;
            else if (ts_node_symbol (subnode) == ts_symbol_type_elem)
              n_interfaces++;
            /* TODO: Support also subnodes of the form ~T or T₁|...|Tₙ  */
          }
      }
      struct go_interface_member *methods = XNMALLOC (n_methods, struct go_interface_member);
      struct go_type **interfaces = XNMALLOC (n_interfaces, struct go_type *);
      {
        unsigned int nm = 0;
        unsigned int ni = 0;
        for (uint32_t i = 0; i < count; i++)
          {
            TSNode subnode = ts_node_named_child (type_node, i);
            if (ts_node_symbol (subnode) == ts_symbol_method_elem)
              {
                TSNode name_node = ts_node_child_by_field_id (subnode, ts_field_name);
                if (ts_node_symbol (name_node) != ts_symbol_field_identifier)
                  abort ();
                string_desc_t name =
                  sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                               contents + ts_node_start_byte (name_node));
                methods[nm].name = xsd_c (name);
                methods[nm].type = get_type_from_function_or_method_node (subnode, tenv, use_indirections);
                nm++;
              }
            else if (ts_node_symbol (subnode) == ts_symbol_type_elem)
              {
                TSNode subsubnode = ts_node_named_child (subnode, 0);
                if (!ts_node_is_null (subsubnode)
                    && ts_node_symbol (subsubnode) == ts_symbol_type_identifier)
                  interfaces[ni] = get_type_from_type_identifier_node (subsubnode, tenv, use_indirections);
                else
                  interfaces[ni] = &unknown_type;
                ni++;
              }
          }
      }
      struct go_type *interface_type =
        create_interface_type (n_methods, methods, n_interfaces, interfaces);
      free (interfaces);
      free (methods);
      return interface_type;
    }
  else if (ts_node_symbol (type_node) == ts_symbol_array_type
           || ts_node_symbol (type_node) == ts_symbol_slice_type)
    {
      TSNode eltype_node = ts_node_child_by_field_id (type_node, ts_field_element);
      return create_array_type (
               get_type_from_type_node (eltype_node, tenv, use_indirections));
    }
  else if (ts_node_symbol (type_node) == ts_symbol_map_type)
    {
      TSNode eltype_node = ts_node_child_by_field_id (type_node, ts_field_value);
      return create_map_type (
               get_type_from_type_node (eltype_node, tenv, use_indirections));
    }
  else if (ts_node_symbol (type_node) == ts_symbol_channel_type)
    return &a_channel_type;
  else if (ts_node_symbol (type_node) == ts_symbol_function_type)
    return get_type_from_function_or_method_node (type_node, tenv, use_indirections);
  else
    return &unknown_type;
}

/* node is of type type_declaration.  */
static void
store_type_declaration (TSNode node)
{
  uint32_t count = ts_node_named_child_count (node);
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode subnode = ts_node_named_child (node, i);
      if (ts_node_symbol (subnode) == ts_symbol_type_alias
          || ts_node_symbol (subnode) == ts_symbol_type_spec)
        {
          TSNode name_node = ts_node_child_by_field_id (subnode, ts_field_name);
          if (ts_node_symbol (name_node) != ts_symbol_type_identifier)
            abort ();
          string_desc_t name =
            sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                         contents + ts_node_start_byte (name_node));
          #if DEBUG_GO && 0
          fprintf (stderr, "Type name = %s\n", sd_c (name));
          #endif
          TSNode type_node = ts_node_child_by_field_id (subnode, ts_field_type);
          go_type_t *type = get_type_from_type_node (type_node, NULL, true);
          /* Store the type definition in current_package.defined_types.  */
          hash_set_value (&current_package.defined_types,
                          sd_data (name), sd_length (name),
                          type);
        }
    }
}

static void
store_top_level_type_declarations (TSNode root_node)
{
  uint32_t count = ts_node_named_child_count (root_node);
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode node = ts_node_named_child (root_node, i);
      if (ts_node_symbol (node) == ts_symbol_type_declaration)
        store_type_declaration (node);
    }
}

/* Tests whether the type declaration is circular.
   Example: type ( Alias1 = Alias2
                   Alias2 = Alias1 )
 */
static bool
is_circular_type_declaration (go_type_t *type, type_env_t tenv)
{
  if (type->e != indirection)
    return false;

  /* Use Robert W. Floyd's cycle detection algorithm.  */
  go_type_t *turtle1 = type;
  go_type_t *turtle2 = type;

  for (;;)
    {
      if (turtle1->e != indirection)
        return false;
      turtle1 = get_type_from_type_name (sd_from_c (turtle1->u.type_name), tenv, false);

      if (turtle2->e != indirection)
        return false;
      turtle2 = get_type_from_type_name (sd_from_c (turtle2->u.type_name), tenv, false);
      if (turtle2->e != indirection)
        return false;
      turtle2 = get_type_from_type_name (sd_from_c (turtle2->u.type_name), tenv, false);

      if (turtle1 == turtle2)
        return true;
    }
}

/* Replace circular type declarations with unknown.
   This ensures that we don't run into endless loops later.  */
static void
eliminate_indirection_loops (void)
{
  void *iter = NULL;
  for (;;)
    {
      const void *name;
      size_t namelen;
      void **data_p;
      if (hash_iterate_modify (&current_package.defined_types, &iter,
                               &name, &namelen, &data_p) < 0)
        break;
      go_type_t *type = *data_p;
      if (is_circular_type_declaration (type, NULL))
        {
          #if DEBUG_GO
          fprintf (stderr, "Type %.*s is circular definition!\n", (int) namelen, (const char *) name);
          #endif
          *data_p = &unknown_type;
        }
    }
}

/* Resolves indirections to named types.
   This modifies the go_type_t graph destructively.  */
static void
resolve_indirections (go_type_t **type_p)
{
  go_type_t *type = *type_p;
  switch (type->e)
    {
    case indirection:
      {
        go_type_t *rtype = type;
        /* This loop terminates, because we have already eliminated circular
           type declarations.  */
        do
          rtype = get_type_from_type_name (sd_from_c (rtype->u.type_name), NULL, false);
        while (rtype->e == indirection);
        *type_p = rtype;
      }
      break;
    case pointer:
    case array:
    case map:
      resolve_indirections (&type->u.eltype);
      break;
    case function:
      {
        unsigned int n = type->u.function_def.n_values;
        for (unsigned int i = 0; i < n; i++)
          resolve_indirections (&type->u.function_def.values[i]);
      }
      break;
    case go_struct:
      {
        unsigned int n = type->u.struct_def.n_members;
        for (unsigned int i = 0; i < n; i++)
          resolve_indirections (&type->u.struct_def.members[i].type);
      }
      {
        unsigned int n = type->u.struct_def.n_methods;
        for (unsigned int i = 0; i < n; i++)
          resolve_indirections (&type->u.struct_def.methods[i].type);
      }
      break;
    case go_interface:
      {
        unsigned int n = type->u.interface_def.n_methods;
        for (unsigned int i = 0; i < n; i++)
          resolve_indirections (&type->u.interface_def.methods[i].type);
      }
      {
        unsigned int n = type->u.interface_def.n_interfaces;
        for (unsigned int i = 0; i < n; i++)
          resolve_indirections (&type->u.interface_def.interfaces[i]);
      }
      break;
    default:
      break;
    }
}

static void
resolve_all_indirections (void)
{
  void *iter = NULL;
  for (;;)
    {
      const void *name;
      size_t namelen;
      void **data_p;
      if (hash_iterate_modify (&current_package.defined_types, &iter,
                               &name, &namelen, &data_p) < 0)
        break;
      go_type_t *type = *data_p;
      #if DEBUG_GO && 0
      fprintf (stderr, "Resolving %.*s (e=%u)", (int) namelen, (const char *) name, type->e);
      #endif
      resolve_indirections (&type);
      #if DEBUG_GO && 0
      fprintf (stderr, " -> (e=%u)\n", type->e);
      #endif
      *data_p = type;
    }
}

static void
verify_no_more_indirections (void)
{
  void *iter = NULL;
  for (;;)
    {
      const void *name;
      size_t namelen;
      void *data;
      if (hash_iterate (&current_package.defined_types, &iter,
                        &name, &namelen, &data) < 0)
        break;
      go_type_t *type = data;
      #if DEBUG_GO
      fprintf (stderr, "verify: %.*s -> ", (int) namelen, (const char *) name);
      print_type (type, stderr);
      fprintf (stderr, "\n");
      #endif
      if (type->e == indirection)
        abort ();
    }
}

/* Initializes current_package.defined_types.  */
static void
init_current_package_types (TSNode root_node)
{
  /* Initialize the hash table.  */
  hash_init (&current_package.defined_types, 50);

  /* Fill the current_package.defined_types table:
     1. Single pass through all top-level type declarations.
     2. Eliminate indirection loops.
     3. Resolve indirections among the types found.  */
  store_top_level_type_declarations (root_node);
  eliminate_indirection_loops ();
  resolve_all_indirections ();
  verify_no_more_indirections ();
}

/* --------------- Go type analysis: Tracking local variables --------------- */

/* A variable environment consists of variable bindings (of which we keep track
   only of the type, not of the value), each with a nested scope.
   The variable environment valid outside of functions is represented by
   NULL.  */

struct variable_binding
{
  string_desc_t name;
  go_type_t *type;
};

struct variable_env
{
  struct variable_env *outer_env;
  struct variable_binding a_binding;
};
typedef struct variable_env *variable_env_t;

/* Augments a variable_env_t.  */
static variable_env_t
variable_env_augment (variable_env_t env, string_desc_t name, go_type_t *type)
{
  struct variable_env *inner = XMALLOC (struct variable_env);
  inner->outer_env = env;
  inner->a_binding.name = name;
  inner->a_binding.type = type;
  return inner;
}

/* Looks up the type corresponding to a variable name in a variable
   environment.  */
static go_type_t *
variable_env_lookup (string_desc_t var_name, variable_env_t venv)
{
  for (; venv != NULL; venv = venv->outer_env)
    {
      if (sd_equals (var_name, venv->a_binding.name))
        return venv->a_binding.type;
    }
  /* Lookup in the global variable environment.  */
  {
    void *found_type;
    if (hash_find_entry (&current_package.globals,
                         sd_data (var_name), sd_length (var_name),
                         &found_type)
        == 0)
      return (go_type_t *) found_type;
    for (size_t i = 0; i < unqualified_packages.nitems; i++)
      {
        const char *unqualified_package = unqualified_packages.item[i];
        if (strcmp (unqualified_package, GOTEXT_PACKAGE_FULLNAME) == 0)
          {
            if (hash_find_entry (&gotext_package.globals,
                                 sd_data (var_name), sd_length (var_name),
                                 &found_type)
                == 0)
              return (go_type_t *) found_type;
          }
        else if (strcmp (unqualified_package, SNAPCORE_PACKAGE_FULLNAME) == 0)
          {
            if (hash_find_entry (&snapcore_package.globals,
                                 sd_data (var_name), sd_length (var_name),
                                 &found_type)
                == 0)
              return (go_type_t *) found_type;
          }
      }
    return &unknown_type;
  }
}

/* ---------------- Go type analysis: Analyzing expressions ---------------- */

/* The type that contains only the nil pointer.  */
static go_type_t nil_type = { other, { NULL } };

/* Determines whether two types are equivalent for our purposes.  */
MAYBE_UNUSED static bool
type_equals (go_type_t *type1, go_type_t *type2, unsigned int maxdepth)
{
  if (maxdepth == 0)
    /* Recursion limit reached.  */
    return false;
  if (type1 == type2)
    return true;
  if (type1->e == type2->e)
    {
      maxdepth--;
      switch (type1->e)
        {
        case pointer:
        case array:
        case map:
          return type_equals (type1->u.eltype, type2->u.eltype, maxdepth);
        case function:
          if (type1->u.function_def.n_values == type2->u.function_def.n_values)
            {
              unsigned int n = type1->u.function_def.n_values;
              for (unsigned int i = 0; i < n; i++)
                if (!type_equals (type1->u.function_def.values[i],
                                  type2->u.function_def.values[i],
                                  maxdepth))
                  return false;
              return true;
            }
          return false;
        case go_struct:
          if (type1->u.struct_def.n_members == type2->u.struct_def.n_members
              && type1->u.struct_def.n_methods == type2->u.struct_def.n_methods)
            {
              {
                unsigned int n = type1->u.struct_def.n_members;
                for (unsigned int i = 0; i < n; i++)
                  if (strcmp (type1->u.struct_def.members[i].name,
                              type2->u.struct_def.members[i].name) != 0)
                    return false;
                for (unsigned int i = 0; i < n; i++)
                  if (!type_equals (type1->u.struct_def.members[i].type,
                                    type2->u.struct_def.members[i].type,
                                    maxdepth))
                    return false;
              }
              {
                unsigned int n = type1->u.struct_def.n_methods;
                for (unsigned int i = 0; i < n; i++)
                  if (strcmp (type1->u.struct_def.methods[i].name,
                              type2->u.struct_def.methods[i].name) != 0)
                    return false;
                for (unsigned int i = 0; i < n; i++)
                  if (!type_equals (type1->u.struct_def.methods[i].type,
                                    type2->u.struct_def.methods[i].type,
                                    maxdepth))
                    return false;
              }
              return true;
            }
          return false;
        case go_interface:
          if (type1->u.interface_def.n_methods == type2->u.interface_def.n_methods
              && type1->u.interface_def.n_interfaces == type2->u.interface_def.n_interfaces)
            {
              {
                unsigned int n = type1->u.interface_def.n_methods;
                for (unsigned int i = 0; i < n; i++)
                  if (strcmp (type1->u.interface_def.methods[i].name,
                              type2->u.interface_def.methods[i].name) != 0)
                    return false;
                for (unsigned int i = 0; i < n; i++)
                  if (!type_equals (type1->u.interface_def.methods[i].type,
                                    type2->u.interface_def.methods[i].type,
                                    maxdepth))
                    return false;
              }
              {
                unsigned int n = type1->u.interface_def.n_interfaces;
                for (unsigned int i = 0; i < n; i++)
                  if (!type_equals (type1->u.interface_def.interfaces[i],
                                    type2->u.interface_def.interfaces[i],
                                    maxdepth))
                    return false;
              }
              return true;
            }
          return false;
        default:
          return false;
        }
    }
  return false;
}

/* Returns the union of TYPE1 and TYPE2.  */
MAYBE_UNUSED static go_type_t *
type_union (go_type_t *type1, go_type_t *type2)
{
  if (type1 == type2)
    return type1;
  if (type2 == &nil_type && type1->e == pointer)
    return type1;
  if (type1 == &nil_type && type2->e == pointer)
    return type2;
  if (type_equals (type1, type2, 100))
    return type1;
  return &unknown_type;
}

/* Forward declaration.  */
static unsigned int
get_mvtypes_of_expression (unsigned int mvcount, go_type_t **result,
                           TSNode node, type_env_t tenv, variable_env_t venv);

/* Returns the type of an expression, assuming a single-value context,
   as far as it can be determined through a simple type analysis.  */
static go_type_t *
get_type_of_expression (TSNode node, type_env_t tenv, variable_env_t venv)
{
  go_type_t *result;
  unsigned int count = get_mvtypes_of_expression (1, &result, node, tenv, venv);
  if (count == 1)
    return result;
  else
    return &unknown_type;
}

/* Returns the type of an expression, assuming a context with mvcount values,
   as far as it can be determined through a simple type analysis.
   mvcount must be >= 1.  There is room for result[0..mvcount-1].
   The return value is the number of values found: >= 1, <= mvcount.  */
static unsigned int
get_mvtypes_of_expression (unsigned int mvcount, go_type_t **result,
                           TSNode node, type_env_t tenv, variable_env_t venv)
{
#define return1(t) \
  do { *result = (t); return 1; } while (0)

  while (ts_node_symbol (node) == ts_symbol_parenthesized_expression
         && ts_node_named_child_count (node) == 1)
    {
      node = ts_node_named_child (node, 0);
    }

  if (ts_node_symbol (node) == ts_symbol_expression_list)
    {
      if (ts_node_named_child_count (node) == mvcount)
        {
          /* Each of the mvcount expressions in node is expected to produce
             a single value.  */
          for (unsigned int i = 0; i < mvcount; i++)
            result[i] = get_type_of_expression (ts_node_named_child (node, i), tenv, venv);
          return mvcount;
        }
      else if (ts_node_named_child_count (node) == 1)
        {
          /* node is an expression that is expected to produce mvcount values.  */
          TSNode sub_expr = ts_node_named_child (node, 0);
          unsigned int sub_mvcount =
            get_mvtypes_of_expression (mvcount, result, sub_expr, tenv, venv);
          if (sub_mvcount == mvcount)
            return mvcount;
        }
      return1 (&unknown_type);
    }
  if (ts_node_symbol (node) == ts_symbol_identifier)
    {
      string_desc_t name =
        sd_new_addr (ts_node_end_byte (node) - ts_node_start_byte (node),
                     contents + ts_node_start_byte (node));
      return1 (variable_env_lookup (name, venv));
    }
  if (ts_node_symbol (node) == ts_symbol_unary_expression)
    {
      TSNode operator_node = ts_node_child_by_field_id (node, ts_field_operator);
      string_desc_t operator =
        sd_new_addr (ts_node_end_byte (operator_node) - ts_node_start_byte (operator_node),
                     contents + ts_node_start_byte (operator_node));
      if (sd_equals (operator, sd_from_c ("*")))
        {
          TSNode operand_node = ts_node_child_by_field_id (node, ts_field_operand);
          go_type_t *operand_type = get_type_of_expression (operand_node, tenv, venv);
          if (operand_type->e == pointer)
            return1 (operand_type->u.eltype);
          else
            return1 (&unknown_type);
        }
      if (sd_equals (operator, sd_from_c ("&")))
        {
          TSNode operand_node = ts_node_child_by_field_id (node, ts_field_operand);
          go_type_t *operand_type = get_type_of_expression (operand_node, tenv, venv);
          return1 (create_pointer_type (operand_type));
        }
      if (sd_equals (operator, sd_from_c ("<-")))
        return1 (&unknown_type);
      /* All other unary operators work on arithmetic types and strings.  */
      return1 (&a_predeclared_type);
    }
  if (ts_node_symbol (node) == ts_symbol_binary_expression)
    {
      /* All binary operators work on arithmetic types and strings.  */
      return1 (&a_predeclared_type);
    }
  if (ts_node_symbol (node) == ts_symbol_selector_expression)
    {
      TSNode field_node = ts_node_child_by_field_id (node, ts_field_field);
      if (ts_node_symbol (field_node) != ts_symbol_field_identifier)
        abort ();
      string_desc_t field_name =
        sd_new_addr (ts_node_end_byte (field_node) - ts_node_start_byte (field_node),
                     contents + ts_node_start_byte (field_node));
      TSNode operand_node = ts_node_child_by_field_id (node, ts_field_operand);
      /* If the operand is a package name, we have in fact a qualified identifier.  */
      if (ts_node_symbol (operand_node) == ts_symbol_identifier)
        {
          string_desc_t shortname =
            sd_new_addr (ts_node_end_byte (operand_node) - ts_node_start_byte (operand_node),
                         contents + ts_node_start_byte (operand_node));
          /* Look up the package's full name.  */
          void *found_package;
          if (hash_find_entry (&package_table,
                               sd_data (shortname), sd_length (shortname),
                               &found_package)
              == 0)
            {
              /* The operand is a package name.  */
              if (strcmp ((const char *) found_package, GOTEXT_PACKAGE_FULLNAME) == 0)
                {
                  /* Look up the entity in the package.  */
                  void *found_type;
                  if (hash_find_entry (&gotext_package.globals,
                                       sd_data (field_name), sd_length (field_name),
                                       &found_type)
                      == 0)
                    return1 ((go_type_t *) found_type);
                }
              else if (strcmp ((const char *) found_package, SNAPCORE_PACKAGE_FULLNAME) == 0)
                {
                  /* Look up the entity in the package.  */
                  void *found_type;
                  if (hash_find_entry (&snapcore_package.globals,
                                       sd_data (field_name), sd_length (field_name),
                                       &found_type)
                      == 0)
                    return1 ((go_type_t *) found_type);
                }
              return1 (&unknown_type);
            }
        }
      go_type_t *operand_type = get_type_of_expression (operand_node, tenv, venv);
      if (operand_type->e == pointer)
        operand_type = operand_type->u.eltype;
      if (operand_type->e == go_struct)
        {
          for (unsigned int i = 0; i < operand_type->u.struct_def.n_members; i++)
            if (sd_equals (field_name, sd_from_c (operand_type->u.struct_def.members[i].name)))
              return1 (operand_type->u.struct_def.members[i].type);
          for (unsigned int i = 0; i < operand_type->u.struct_def.n_methods; i++)
            if (sd_equals (field_name, sd_from_c (operand_type->u.struct_def.methods[i].name)))
              return1 (operand_type->u.struct_def.methods[i].type);
          /* TODO: Handle embedded fields.  */
        }
      else if (operand_type->e == go_interface)
        {
          /* Find a method of the given name through a breadth-first search.  */
          gl_list_t queued_interfaces =
            gl_list_create_empty (GL_CARRAY_LIST, NULL, NULL, NULL, false);
          gl_set_t visited_interfaces =
            gl_set_create_empty (GL_HASH_SET, NULL, NULL, NULL);
          gl_list_add_last (queued_interfaces, operand_type);
          while (gl_list_size (queued_interfaces) > 0)
            {
              go_type_t *itf = (go_type_t *) gl_list_get_first (queued_interfaces);
              gl_list_remove_first (queued_interfaces);
              if (itf->e == go_interface
                  && !gl_set_search (visited_interfaces, itf))
                {
                  gl_set_add (visited_interfaces, itf);
                  /* Search among the methods directly defined in itf.  */
                  for (unsigned int i = 0; i < itf->u.interface_def.n_methods; i++)
                    if (sd_equals (field_name, sd_from_c (itf->u.interface_def.methods[i].name)))
                      {
                        gl_set_free (visited_interfaces);
                        gl_list_free (queued_interfaces);
                        return1 (itf->u.interface_def.methods[i].type);
                      }
                  /* Enqueue the embedded interfaces of itf.  */
                  for (unsigned int i = 0; i < itf->u.interface_def.n_interfaces; i++)
                    gl_list_add_last (queued_interfaces, itf->u.interface_def.interfaces[i]);
                }
            }
          gl_set_free (visited_interfaces);
          gl_list_free (queued_interfaces);
        }
      return1 (&unknown_type);
    }
  if (ts_node_symbol (node) == ts_symbol_index_expression)
    {
      TSNode operand_node = ts_node_child_by_field_id (node, ts_field_operand);
      go_type_t *operand_type = get_type_of_expression (operand_node, tenv, venv);
      if (operand_type->e == array || operand_type->e == map)
        return1 (operand_type->u.eltype);
      if (operand_type->e == pointer && operand_type->u.eltype->e == array)
        return1 (operand_type->u.eltype->u.eltype);
      if (operand_type->e == predeclared)
        /* Must be a string type.  */
        return1 (&a_predeclared_type);
      /* A generic function instantiation is returned as a node of type
         index_expression.
         Example: sum[int], cf. <https://go.dev/ref/spec#Instantiations>  */
      if (operand_type->e == function)
        /* We don't distinguish between generic and non-generic functions here.  */
        return1 (operand_type);
      return1 (&unknown_type);
    }
  if (ts_node_symbol (node) == ts_symbol_slice_expression)
    {
      TSNode operand_node = ts_node_child_by_field_id (node, ts_field_operand);
      go_type_t *operand_type = get_type_of_expression (operand_node, tenv, venv);
      if (operand_type->e == array)
        return1 (operand_type);
      if (operand_type->e == pointer && operand_type->u.eltype->e == array)
        return1 (operand_type->u.eltype);
      if (operand_type->e == predeclared)
        /* Must be a string or bytestring type.  */
        return1 (&a_predeclared_type);
      return1 (&unknown_type);
    }
  if (ts_node_symbol (node) == ts_symbol_call_expression)
    {
      TSNode function_node = ts_node_child_by_field_id (node, ts_field_function);
      // 'new' and 'make' are special.
      if (ts_node_symbol (function_node) == ts_symbol_identifier)
        {
          string_desc_t function_name =
            sd_new_addr (ts_node_end_byte (function_node) - ts_node_start_byte (function_node),
                         contents + ts_node_start_byte (function_node));
          if (sd_equals (function_name, sd_from_c ("new")))
            {
              TSNode args_node = ts_node_child_by_field_id (node, ts_field_arguments);
              /* This is the field called 'arguments'.
                 Recognize the syntax 'new (TYPE)'.  */
              if (ts_node_symbol (args_node) == ts_symbol_argument_list
                  && ts_node_named_child_count (args_node) == 1)
                {
                  TSNode type_node = ts_node_named_child (args_node, 0);
                  go_type_t *type = get_type_from_type_node (type_node, tenv, false);
                  return1 (create_pointer_type (type));
                }
              return1 (&unknown_type);
            }
          if (sd_equals (function_name, sd_from_c ("make")))
            {
              TSNode args_node = ts_node_child_by_field_id (node, ts_field_arguments);
              /* This is the field called 'arguments'.
                 Recognize the syntax 'make (TYPE, ...)'.  */
              if (ts_node_symbol (args_node) == ts_symbol_argument_list
                  && ts_node_named_child_count (args_node) >= 1)
                {
                  TSNode type_node = ts_node_named_child (args_node, 0);
                  go_type_t *type = get_type_from_type_node (type_node, tenv, false);
                  if (type->e == array || type->e == map || type->e == channel)
                    return1 (type);
                }
              return1 (&unknown_type);
            }
        }
      go_type_t *function_type = get_type_of_expression (function_node, tenv, venv);
      if (function_type->e == function
          && function_type->u.function_def.n_values == mvcount)
        {
          for (unsigned int i = 0; i < mvcount; i++)
            result[i] = function_type->u.function_def.values[i];
          return mvcount;
        }
      return1 (&unknown_type);
    }
  if (ts_node_symbol (node) == ts_symbol_type_assertion_expression
      || ts_node_symbol (node) == ts_symbol_type_conversion_expression
      || ts_node_symbol (node) == ts_symbol_type_instantiation_expression)
    {
      TSNode type_node = ts_node_child_by_field_id (node, ts_field_type);
      return1 (get_type_from_type_node (type_node, tenv, false));
    }
  if (ts_node_symbol (node) == ts_symbol_composite_literal)
    {
      TSNode type_node = ts_node_child_by_field_id (node, ts_field_type);
      return1 (get_type_from_type_node (type_node, tenv, false));
    }
  if (ts_node_symbol (node) == ts_symbol_func_literal)
    return1 (get_type_from_function_or_method_node (node, tenv, false));
  if ((ts_node_symbol (node) == ts_symbol_raw_string_literal
       || ts_node_symbol (node) == ts_symbol_interpreted_string_literal)
      || ts_node_symbol (node) == ts_symbol_int_literal
      || ts_node_symbol (node) == ts_symbol_float_literal
      || ts_node_symbol (node) == ts_symbol_imaginary_literal
      || ts_node_symbol (node) == ts_symbol_rune_literal
      || ts_node_symbol (node) == ts_symbol_true
      || ts_node_symbol (node) == ts_symbol_false
      || ts_node_symbol (node) == ts_symbol_iota)
    return1 (&a_predeclared_type);
  if (ts_node_symbol (node) == ts_symbol_nil)
    return1 (&nil_type);
  return1 (&unknown_type);

#undef return1
}

/* --------------- Go global variables and functions analysis --------------- */

/* node is of type var_spec.  */
static void
store_var_spec (TSNode node)
{
  /* It may contain multiple names.  */
  TSNode type_node = ts_node_child_by_field_id (node, ts_field_type);
  if (!ts_node_is_null (type_node))
    {
      /* "If a type is present, each variable is given that type."  */
      go_type_t *type = get_type_from_type_node (type_node, NULL, false);
      uint32_t count = ts_node_named_child_count (node);
      for (uint32_t i = 0; i < count; i++)
        {
          TSNode subnode = ts_node_named_child (node, i);
          if (ts_node_symbol (subnode) == ts_symbol_identifier)
            {
              TSNode name_node = subnode;
              string_desc_t name =
                sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                             contents + ts_node_start_byte (name_node));
              #if DEBUG_GO && 0
              fprintf (stderr, "Var name = %s\n", sd_c (name));
              #endif
              /* Store the variable definition in current_package.globals.  */
              hash_set_value (&current_package.globals,
                              sd_data (name), sd_length (name),
                              type);
            }
        }
    }
  else
    {
      /* "Otherwise, each variable is given the type of the corresponding
          initialization value in the assignment."  */
      uint32_t count = ts_node_named_child_count (node);
      unsigned int mvcount;
      {
        mvcount = 0;
        for (uint32_t i = 0; i < count; i++)
          {
            TSNode subnode = ts_node_named_child (node, i);
            if (ts_node_symbol (subnode) == ts_symbol_identifier)
              mvcount++;
          }
      }
      if (mvcount > 0)
        {
          /* We are in a context where mvcount values are expected.  */
          TSNode value_node = ts_node_child_by_field_id (node, ts_field_value);
          go_type_t **value_types = XNMALLOC (mvcount, go_type_t *);
          unsigned int value_mvcount =
            get_mvtypes_of_expression (mvcount, value_types, value_node, NULL, NULL);
          if (value_mvcount != mvcount)
            {
              for (unsigned int j = 0; j < mvcount; j++)
                value_types[j] = &unknown_type;
            }
          unsigned int j = 0;
          for (uint32_t i = 0; i < count; i++)
            {
              TSNode subnode = ts_node_named_child (node, i);
              if (ts_node_symbol (subnode) == ts_symbol_identifier)
                {
                  TSNode name_node = subnode;
                  string_desc_t name =
                    sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                                 contents + ts_node_start_byte (name_node));
                  #if DEBUG_GO && 0
                  fprintf (stderr, "Var name = %s\n", sd_c (name));
                  #endif
                  /* Store the variable definition in current_package.globals.  */
                  hash_set_value (&current_package.globals,
                                  sd_data (name), sd_length (name),
                                  value_types[j]);
                  j++;
                }
            }
          free (value_types);
        }
    }
}

/* node is of type var_spec_list.  */
static void
store_var_spec_list (TSNode node)
{
  uint32_t count = ts_node_named_child_count (node);
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode subnode = ts_node_named_child (node, i);
      if (ts_node_symbol (subnode) == ts_symbol_var_spec)
        store_var_spec (subnode);
    }
}

/* Processes a var_declaration node at the top level.  */
static void
store_var_declaration (TSNode node)
{
  uint32_t count = ts_node_named_child_count (node);
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode subnode = ts_node_named_child (node, i);
      if (ts_node_symbol (subnode) == ts_symbol_var_spec_list)
        store_var_spec_list (subnode);
      else if (ts_node_symbol (subnode) == ts_symbol_var_spec)
        store_var_spec (subnode);
    }
}

/* node is of type const_spec.  */
static void
store_const_spec (TSNode node)
{
  TSNode name_node = ts_node_child_by_field_id (node, ts_field_name);
  string_desc_t name =
    sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                 contents + ts_node_start_byte (name_node));
  #if DEBUG_GO && 0
  fprintf (stderr, "Const name = %s\n", sd_c (name));
  #endif
  /* The type is always a predefined type.  */
  go_type_t *type = &a_predeclared_type;
  /* Store the const definition in current_package.globals.  */
  hash_set_value (&current_package.globals,
                  sd_data (name), sd_length (name),
                  type);
}

/* Processes a const_declaration node at the top level.  */
static void
store_const_declaration (TSNode node)
{
  uint32_t count = ts_node_named_child_count (node);
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode subnode = ts_node_named_child (node, i);
      if (ts_node_symbol (subnode) == ts_symbol_const_spec)
        store_const_spec (subnode);
    }
}

/* Processes a function_declaration at the top level.  */
static void
store_function_declaration (TSNode node)
{
  TSNode name_node = ts_node_child_by_field_id (node, ts_field_name);
  string_desc_t name =
    sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                 contents + ts_node_start_byte (name_node));
  #if DEBUG_GO && 0
  fprintf (stderr, "Func name = %s\n", sd_c (name));
  #endif
  go_type_t *type = get_type_from_function_or_method_node (node, NULL, false);
  /* Store the func definition in current_package.globals.  */
  hash_set_value (&current_package.globals,
                  sd_data (name), sd_length (name),
                  type);
}

static void
store_top_level_declarations (TSNode root_node)
{
  uint32_t count = ts_node_named_child_count (root_node);
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode node = ts_node_named_child (root_node, i);
      if (ts_node_symbol (node) == ts_symbol_var_declaration)
        store_var_declaration (node);
      else if (ts_node_symbol (node) == ts_symbol_const_declaration)
        store_const_declaration (node);
      else if (ts_node_symbol (node) == ts_symbol_function_declaration)
        store_function_declaration (node);
    }
}

/* Initializes current_package.globals.  */
static void
init_current_package_globals (TSNode root_node)
{
  /* Initialize the hash table.  */
  hash_init (&current_package.globals, 100);

  /* Fill the current_package.globals table.  */
  store_top_level_declarations (root_node);
}

/* --------- Go type analysis: Keeping track of local declarations --------- */

/* node is of type type_declaration.  */
static type_env_t
augment_for_type_declaration (TSNode node, type_env_t tenv)
{
  /* Similar to store_type_declaration.  */
  uint32_t count = ts_node_named_child_count (node);
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode subnode = ts_node_named_child (node, i);
      if (ts_node_symbol (subnode) == ts_symbol_type_alias
          || ts_node_symbol (subnode) == ts_symbol_type_spec)
        {
          TSNode name_node = ts_node_child_by_field_id (subnode, ts_field_name);
          if (ts_node_symbol (name_node) != ts_symbol_type_identifier)
            abort ();
          string_desc_t name =
            sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                         contents + ts_node_start_byte (name_node));
          #if DEBUG_GO && 0
          fprintf (stderr, "Local type name = %s\n", sd_c (name));
          #endif
          TSNode type_node = ts_node_child_by_field_id (subnode, ts_field_type);
          go_type_t *type = get_type_from_type_node (type_node, tenv, false);
          tenv = type_env_augment (tenv, name, type);
        }
    }
  return tenv;
}

/* node is of type parameter_list.  */
static variable_env_t
augment_for_parameter_list (TSNode node, type_env_t tenv, variable_env_t venv)
{
  variable_env_t augmented_venv = venv;
  uint32_t count = ts_node_named_child_count (node);
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode subnode = ts_node_named_child (node, i);
      if (ts_node_symbol (subnode) == ts_symbol_parameter_declaration)
        {
          uint32_t count2 = ts_node_named_child_count (subnode);
          for (uint32_t j = 0; j < count2; j++)
            {
              TSNode subsubnode = ts_node_named_child (subnode, j);
              if (ts_node_symbol (subsubnode) == ts_symbol_identifier)
                {
                  TSNode name_node = subsubnode;
                  string_desc_t name =
                    sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                                 contents + ts_node_start_byte (name_node));
                  #if DEBUG_GO && 0
                  fprintf (stderr, "Local parameter name = %s\n", sd_c (name));
                  #endif
                  TSNode type_node = ts_node_child_by_field_id (subnode, ts_field_type);
                  go_type_t *type = get_type_from_type_node (type_node, tenv, false);
                  augmented_venv = variable_env_augment (augmented_venv, name, type);
                }
            }
        }
      else if (ts_node_symbol (subnode) == ts_symbol_variadic_parameter_declaration)
        {
          uint32_t count2 = ts_node_named_child_count (subnode);
          for (uint32_t j = 0; j < count2; j++)
            {
              TSNode subsubnode = ts_node_named_child (subnode, j);
              if (ts_node_symbol (subsubnode) == ts_symbol_identifier)
                {
                  TSNode name_node = subsubnode;
                  string_desc_t name =
                    sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                                 contents + ts_node_start_byte (name_node));
                  #if DEBUG_GO && 0
                  fprintf (stderr, "Local variadic parameter name = %s\n", sd_c (name));
                  #endif
                  TSNode type_node = ts_node_child_by_field_id (subnode, ts_field_type);
                  go_type_t *type = get_type_from_type_node (type_node, tenv, false);
                  augmented_venv = variable_env_augment (augmented_venv, name, create_array_type (type));
                }
            }
        }
    }
  return augmented_venv;
}

/* node is of type var_spec.  */
static variable_env_t
augment_for_var_spec (TSNode node, type_env_t tenv, variable_env_t venv)
{
  /* Similar to store_var_spec.  */
  /* It may contain multiple names.  */
  TSNode type_node = ts_node_child_by_field_id (node, ts_field_type);
  if (!ts_node_is_null (type_node))
    {
      /* "If a type is present, each variable is given that type."  */
      go_type_t *type = get_type_from_type_node (type_node, tenv, false);
      uint32_t count = ts_node_named_child_count (node);
      for (uint32_t i = 0; i < count; i++)
        {
          TSNode subnode = ts_node_named_child (node, i);
          if (ts_node_symbol (subnode) == ts_symbol_identifier)
            {
              TSNode name_node = subnode;
              string_desc_t name =
                sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                             contents + ts_node_start_byte (name_node));
              #if DEBUG_GO && 0
              fprintf (stderr, "Local var name = %s\n", sd_c (name));
              #endif
              venv = variable_env_augment (venv, name, type);
            }
        }
    }
  else
    {
      /* "Otherwise, each variable is given the type of the corresponding
          initialization value in the assignment."  */
      uint32_t count = ts_node_named_child_count (node);
      unsigned int mvcount;
      {
        mvcount = 0;
        for (uint32_t i = 0; i < count; i++)
          {
            TSNode subnode = ts_node_named_child (node, i);
            if (ts_node_symbol (subnode) == ts_symbol_identifier)
              mvcount++;
          }
      }
      if (mvcount > 0)
        {
          /* We are in a context where mvcount values are expected.  */
          TSNode value_node = ts_node_child_by_field_id (node, ts_field_value);
          go_type_t **value_types = XNMALLOC (mvcount, go_type_t *);
          unsigned int value_mvcount =
            get_mvtypes_of_expression (mvcount, value_types, value_node, tenv, venv);
          if (value_mvcount != mvcount)
            {
              for (unsigned int j = 0; j < mvcount; j++)
                value_types[j] = &unknown_type;
            }
          unsigned int j = 0;
          for (uint32_t i = 0; i < count; i++)
            {
              TSNode subnode = ts_node_named_child (node, i);
              if (ts_node_symbol (subnode) == ts_symbol_identifier)
                {
                  TSNode name_node = subnode;
                  string_desc_t name =
                    sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                                 contents + ts_node_start_byte (name_node));
                  #if DEBUG_GO && 0
                  fprintf (stderr, "Local var name = %s\n", sd_c (name));
                  #endif
                  venv = variable_env_augment (venv, name, value_types[j]);
                  j++;
                }
            }
          free (value_types);
        }
    }
  return venv;
}

/* node is of type var_spec_list.  */
static variable_env_t
augment_for_var_spec_list (TSNode node, type_env_t tenv, variable_env_t venv)
{
  /* Similar to store_var_spec_list.  */
  uint32_t count = ts_node_named_child_count (node);
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode subnode = ts_node_named_child (node, i);
      if (ts_node_symbol (subnode) == ts_symbol_var_spec)
        venv = augment_for_var_spec (subnode, tenv, venv);
    }
  return venv;
}

/* node is of type var_declaration.  */
static variable_env_t
augment_for_variable_declaration (TSNode node, type_env_t tenv, variable_env_t venv)
{
  /* Similar to store_var_declaration.  */
  uint32_t count = ts_node_named_child_count (node);
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode subnode = ts_node_named_child (node, i);
      if (ts_node_symbol (subnode) == ts_symbol_var_spec_list)
        venv = augment_for_var_spec_list (subnode, tenv, venv);
      else if (ts_node_symbol (subnode) == ts_symbol_var_spec)
        venv = augment_for_var_spec (subnode, tenv, venv);
    }
  return venv;
}

/* node is of type const_spec.  */
static variable_env_t
augment_for_const_spec (TSNode node, type_env_t tenv, variable_env_t venv)
{
  /* Similar to store_const_spec.  */
  TSNode name_node = ts_node_child_by_field_id (node, ts_field_name);
  string_desc_t name =
    sd_new_addr (ts_node_end_byte (name_node) - ts_node_start_byte (name_node),
                 contents + ts_node_start_byte (name_node));
  #if DEBUG_GO && 0
  fprintf (stderr, "Local const name = %s\n", sd_c (name));
  #endif
  /* The type is always a predefined type.  */
  go_type_t *type = &a_predeclared_type;
  return variable_env_augment (venv, name, type);
}

/* node is of type const_declaration.  */
static variable_env_t
augment_for_const_declaration (TSNode node, type_env_t tenv, variable_env_t venv)
{
  /* Similar to store_const_declaration.  */
  uint32_t count = ts_node_named_child_count (node);
  for (uint32_t i = 0; i < count; i++)
    {
      TSNode subnode = ts_node_named_child (node, i);
      if (ts_node_symbol (subnode) == ts_symbol_const_spec)
        venv = augment_for_const_spec (subnode, tenv, venv);
    }
  return venv;
}

/* node is of type short_var_declaration.  */
static variable_env_t
augment_for_short_variable_declaration (TSNode node, type_env_t tenv, variable_env_t venv)
{
  TSNode left_node = ts_node_child_by_field_id (node, ts_field_left);
  if (ts_node_symbol (left_node) != ts_symbol_expression_list)
    abort ();
  unsigned int mvcount = ts_node_named_child_count (left_node);
  go_type_t **mvtypes = XNMALLOC (mvcount, go_type_t *);
  TSNode right_node = ts_node_child_by_field_id (node, ts_field_right);
  if (ts_node_symbol (right_node) != ts_symbol_expression_list)
    abort ();
  /* We are in a context where mvcount values are expected.  */
  unsigned int right_mvcount =
    get_mvtypes_of_expression (mvcount, mvtypes, right_node, tenv, venv);
  if (right_mvcount != mvcount)
    {
      for (unsigned int i = 0; i < mvcount; i++)
        mvtypes[i] = &unknown_type;
    }
  /* Now augment venv.  */
  for (unsigned int i = 0; i < mvcount; i++)
    {
      TSNode left_var_node = ts_node_named_child (left_node, i);
      if (ts_node_symbol (left_var_node) == ts_symbol_identifier)
        {
          string_desc_t left_var_name =
            sd_new_addr (ts_node_end_byte (left_var_node) - ts_node_start_byte (left_var_node),
                         contents + ts_node_start_byte (left_var_node));
          if (!sd_equals (left_var_name, sd_from_c ("_")))
            venv = variable_env_augment (venv, left_var_name, mvtypes[i]);
        }
    }
  free (mvtypes);
  return venv;
}

/* -------------------------------- Comments -------------------------------- */

/* These are for tracking whether comments count as immediately before
   keyword.  */
static int last_comment_line;
static int last_non_comment_line;

/* Saves a comment line.  */
static void save_comment_line (string_desc_t gist)
{
  /* Remove leading whitespace.  */
  while (sd_length (gist) > 0
         && (sd_char_at (gist, 0) == ' '
             || sd_char_at (gist, 0) == '\t'))
    gist = sd_substring (gist, 1, sd_length (gist));
  /* Remove trailing whitespace.  */
  size_t len = sd_length (gist);
  while (len > 0
         && (sd_char_at (gist, len - 1) == ' '
             || sd_char_at (gist, len - 1) == '\t'))
    len--;
  gist = sd_substring (gist, 0, len);
  savable_comment_add (sd_c (gist));
}

/* Does the comment handling for NODE.
   Updates savable_comment, last_comment_line, last_non_comment_line.
   It is important that this function gets called
     - for each node (not only the named nodes!),
     - in depth-first traversal order.  */
static void handle_comments (TSNode node)
{
  #if DEBUG_GO && 0
  fprintf (stderr, "LCL=%d LNCL=%d node=[%s]|%s|\n", last_comment_line, last_non_comment_line, ts_node_type (node), ts_node_string (node));
  #endif
  if (last_comment_line < last_non_comment_line
      && last_non_comment_line < ts_node_line_number (node))
    /* We have skipped over a newline.  This newline terminated a line
       with non-comment tokens, after the last comment line.  */
    savable_comment_reset ();

  if (ts_node_symbol (node) == ts_symbol_comment)
    {
      string_desc_t entire =
        sd_new_addr (ts_node_end_byte (node) - ts_node_start_byte (node),
                     contents + ts_node_start_byte (node));
      /* It should either start with two slashes, or start and end with
         the C comment markers.  */
      if (sd_length (entire) >= 2
          && sd_char_at (entire, 0) == '/'
          && sd_char_at (entire, 1) == '/')
        {
          save_comment_line (sd_substring (entire, 2, sd_length (entire)));
        }
      else if (sd_length (entire) >= 4
               && sd_char_at (entire, 0) == '/'
               && sd_char_at (entire, 1) == '*'
               && sd_char_at (entire, sd_length (entire) - 2) == '*'
               && sd_char_at (entire, sd_length (entire) - 1) == '/')
        {
          string_desc_t gist = sd_substring (entire, 2, sd_length (entire) - 2);
          /* Split into lines.
             Remove leading and trailing whitespace from each line.  */
          for (;;)
            {
              ptrdiff_t nl_index = sd_index (gist, '\n');
              if (nl_index >= 0)
                {
                  save_comment_line (sd_substring (gist, 0, nl_index));
                  gist = sd_substring (gist, nl_index + 1, sd_length (gist));
                }
              else
                {
                  save_comment_line (gist);
                  break;
                }
            }
        }
      else
        abort ();
      last_comment_line = ts_node_end_point (node).row + 1;
    }
  else
    last_non_comment_line = ts_node_line_number (node);
}

/* --------------------- Parsing and string extraction --------------------- */

/* Context lookup table.  */
static flag_context_list_table_ty *flag_context_list_table;

/* Maximum supported nesting depth.  */
#define MAX_NESTING_DEPTH 1000

static int nesting_depth;

/* The file is parsed into an abstract syntax tree.  Scan the syntax tree,
   looking for a keyword in identifier position of a call_expression or
   macro_invocation, followed by followed by a string among the arguments.
   When we see this pattern, we have something to remember.

     Normal handling: Look for
       keyword ( ... msgid ... )
     Plural handling: Look for
       keyword ( ... msgid ... msgid_plural ... )

   We use recursion because the arguments before msgid or between msgid
   and msgid_plural can contain subexpressions of the same form.  */

/* Forward declarations.  */
static void extract_from_node (TSNode node,
                               bool in_function,
                               type_env_t tenv, variable_env_t venv,
                               bool ignore,
                               flag_region_ty *outer_region,
                               message_list_ty *mlp);

/* Extracts messages from the function call consisting of
     - CALLEE_NODE: a tree node of type 'identifier' or 'selector_expression',
     - ARGS_NODE: a tree node of type 'arguments'.
   Extracted messages are added to MLP.  */
static void
extract_from_function_call (TSNode callee_node,
                            TSNode args_node,
                            type_env_t tenv, variable_env_t venv,
                            flag_region_ty *outer_region,
                            message_list_ty *mlp)
{
  uint32_t args_count = ts_node_child_count (args_node);

  TSNode function_node;
  if (ts_node_symbol (callee_node) == ts_symbol_identifier)
    function_node = callee_node;
  else if (ts_node_symbol (callee_node) == ts_symbol_selector_expression)
    function_node = ts_node_child_by_field_id (callee_node, ts_field_field);
  else
    abort ();

  if (!(ts_node_symbol (function_node) == ts_symbol_identifier
        || ts_node_symbol (function_node) == ts_symbol_field_identifier))
    abort ();

  string_desc_t function_name =
    sd_new_addr (ts_node_end_byte (function_node) - ts_node_start_byte (function_node),
                 contents + ts_node_start_byte (function_node));

  /* Context iterator.  */
  flag_context_list_iterator_ty next_context_iter =
    flag_context_list_iterator (
      flag_context_list_table_lookup (
        flag_context_list_table,
        sd_data (function_name), sd_length (function_name)));

  /* Information associated with the callee.  */
  const struct callshapes *next_shapes = NULL;

  if (ts_node_symbol (callee_node) == ts_symbol_identifier)
    {
      /* Look in the keywords table.  */
      void *keyword_value;
      if (hash_find_entry (&keywords,
                           sd_data (function_name), sd_length (function_name),
                           &keyword_value)
          == 0)
        next_shapes = (const struct callshapes *) keyword_value;
    }
  else if (ts_node_symbol (callee_node) == ts_symbol_selector_expression)
    {
      if (ts_node_symbol (function_node) != ts_symbol_field_identifier)
        abort ();
      TSNode operand_node = ts_node_child_by_field_id (callee_node, ts_field_operand);
      /* If the operand is a package name, we have in fact a qualified identifier.  */
      bool qualified_identifier = false;
      if (ts_node_symbol (operand_node) == ts_symbol_identifier)
        {
          string_desc_t shortname =
            sd_new_addr (ts_node_end_byte (operand_node) - ts_node_start_byte (operand_node),
                         contents + ts_node_start_byte (operand_node));
          /* Look up the package's full name.  */
          void *found_package;
          if (hash_find_entry (&package_table,
                               sd_data (shortname), sd_length (shortname),
                               &found_package)
              == 0)
            {
              qualified_identifier = true;
              /* The operand is a package name.  */
              if (strcmp ((const char *) found_package, GOTEXT_PACKAGE_FULLNAME) == 0)
                {
                  /* Look in the gotext_keywords table.  */
                  void *keyword_value;
                  if (hash_find_entry (&gotext_keywords,
                                       sd_data (function_name), sd_length (function_name),
                                       &keyword_value)
                      == 0)
                    next_shapes = (const struct callshapes *) keyword_value;
                }
              else if (strcmp ((const char *) found_package, SNAPCORE_PACKAGE_FULLNAME) == 0)
                {
                  /* Look in the snapcore_keywords table.  */
                  void *keyword_value;
                  if (hash_find_entry (&snapcore_keywords,
                                       sd_data (function_name), sd_length (function_name),
                                       &keyword_value)
                      == 0)
                    next_shapes = (const struct callshapes *) keyword_value;
                }
              if (next_shapes == NULL)
                {
                  /* Look in the keywords table as well.  */
                  void *keyword_value;
                  if (hash_find_entry (&keywords,
                                       sd_data (function_name), sd_length (function_name),
                                       &keyword_value)
                      == 0)
                    next_shapes = (const struct callshapes *) keyword_value;
                }
            }
        }
      if (!qualified_identifier)
        {
          go_type_t *operand_type = get_type_of_expression (operand_node, tenv, venv);
          if (operand_type->e == pointer)
            operand_type = operand_type->u.eltype;
          /* Here it is important that we can compare 'go_type_t *' pointers for equality.  */
          hash_table *ht;
          ht = (hash_table *) gl_map_get (gotext_type_keywords, operand_type);
          if (ht == NULL)
            ht = (hash_table *) gl_map_get (snapcore_type_keywords, operand_type);
          if (ht != NULL)
            {
              /* Look in this hash table.  */
              void *keyword_value;
              if (hash_find_entry (ht,
                                   sd_data (function_name), sd_length (function_name),
                                   &keyword_value)
                  == 0)
                next_shapes = (const struct callshapes *) keyword_value;
            }
        }
    }
  else
    abort ();

  if (next_shapes != NULL)
    {
      /* We have a function, named by a relevant identifier, with an argument
         list.  */

      struct arglist_parser *argparser =
        arglist_parser_alloc (mlp, next_shapes);

      /* Current argument number.  */
      uint32_t arg;

      arg = 0;
      for (uint32_t i = 0; i < args_count; i++)
        {
          TSNode arg_node = ts_node_child (args_node, i);
          handle_comments (arg_node);
          if (ts_node_is_named (arg_node)
              && ts_node_symbol (arg_node) != ts_symbol_comment)
            {
              arg++;
              flag_region_ty *arg_region =
                inheriting_region (outer_region,
                                   flag_context_list_iterator_advance (
                                     &next_context_iter));

              bool already_extracted = false;
              if (is_string_literal (arg_node))
                {
                  lex_pos_ty pos;
                  pos.file_name = logical_file_name;
                  pos.line_number = ts_node_line_number (arg_node);

                  char *string = string_literal_value (arg_node);

                  if (extract_all)
                    {
                      remember_a_message (mlp, NULL, string, true, false,
                                          arg_region, &pos,
                                          NULL, savable_comment, true);
                      already_extracted = true;
                    }
                  else
                    {
                      mixed_string_ty *mixed_string =
                        mixed_string_alloc_utf8 (string, lc_string,
                                                 pos.file_name, pos.line_number);
                      arglist_parser_remember (argparser, arg, mixed_string,
                                               arg_region,
                                               pos.file_name, pos.line_number,
                                               savable_comment, true);
                    }
                }

              if (!already_extracted)
                {
                  if (++nesting_depth > MAX_NESTING_DEPTH)
                    if_error (IF_SEVERITY_FATAL_ERROR,
                              logical_file_name, ts_node_line_number (arg_node), (size_t)(-1), false,
                              _("too many open parentheses"));
                  extract_from_node (arg_node,
                                     true,
                                     tenv, venv,
                                     false,
                                     arg_region,
                                     mlp);
                  nesting_depth--;
                }

              unref_region (arg_region);
            }
        }
      arglist_parser_done (argparser, arg);
      return;
    }

  /* Recurse.  */

  for (uint32_t i = 0; i < args_count; i++)
    {
      TSNode arg_node = ts_node_child (args_node, i);
      handle_comments (arg_node);
      if (ts_node_is_named (arg_node)
          && ts_node_symbol (arg_node) != ts_symbol_comment)
        {
          flag_region_ty *arg_region =
            inheriting_region (outer_region,
                               flag_context_list_iterator_advance (
                                 &next_context_iter));

          if (++nesting_depth > MAX_NESTING_DEPTH)
            if_error (IF_SEVERITY_FATAL_ERROR,
                      logical_file_name, ts_node_line_number (arg_node), (size_t)(-1), false,
                      _("too many open parentheses"));
          extract_from_node (arg_node,
                             true,
                             tenv, venv,
                             false,
                             arg_region,
                             mlp);
          nesting_depth--;

          unref_region (arg_region);
        }
    }
}

/* Extracts messages in the syntax tree NODE.
   Extracted messages are added to MLP.  */
static void
extract_from_node (TSNode node,
                   bool in_function,
                   type_env_t tenv, variable_env_t venv,
                   bool ignore,
                   flag_region_ty *outer_region,
                   message_list_ty *mlp)
{
  if (extract_all && !ignore && is_string_literal (node))
    {
      lex_pos_ty pos;
      pos.file_name = logical_file_name;
      pos.line_number = ts_node_line_number (node);

      char *string = string_literal_value (node);

      remember_a_message (mlp, NULL, string, true, false,
                          outer_region, &pos,
                          NULL, savable_comment, true);
    }

  if (ts_node_symbol (node) == ts_symbol_call_expression
      && ts_node_named_child_count (node) >= 2)
    {
      TSNode callee_node = ts_node_named_child (node, 0);
      /* This is the field called 'function'.  */
      if (! ts_node_eq (ts_node_child_by_field_id (node, ts_field_function),
                        callee_node))
        abort ();
      #if DEBUG_GO
      fprintf (stderr, "callee_node = [%s]|%s|\n", ts_node_type (callee_node), ts_node_string (callee_node));
      if (ts_node_symbol (callee_node) == ts_symbol_selector_expression)
        {
          TSNode operand_node = ts_node_child_by_field_id (callee_node, ts_field_operand);
          string_desc_t operand_name =
            sd_new_addr (ts_node_end_byte (operand_node) - ts_node_start_byte (operand_node),
                         contents + ts_node_start_byte (operand_node));
          fprintf (stderr, "operand_node = [%s]|%s| = %s\n", ts_node_type (operand_node), ts_node_string (operand_node), sd_c (operand_name));

          TSNode field_node = ts_node_child_by_field_id (callee_node, ts_field_field);
          string_desc_t field_name =
            sd_new_addr (ts_node_end_byte (field_node) - ts_node_start_byte (field_node),
                         contents + ts_node_start_byte (field_node));
          fprintf (stderr, "field_node = [%s]|%s| = %s\n", ts_node_type (field_node), ts_node_string (field_node), sd_c (field_name));
        }
      #endif
      if (ts_node_symbol (callee_node) == ts_symbol_identifier
          || ts_node_symbol (callee_node) == ts_symbol_selector_expression)
        {
          TSNode args_node = ts_node_child_by_field_id (node, ts_field_arguments);
          /* This is the field called 'arguments'.  */
          if (ts_node_symbol (args_node) == ts_symbol_argument_list)
            {
              /* Handle the potential comments before the 'arguments'.  */
              {
                uint32_t count = ts_node_child_count (node);
                for (uint32_t i = 0; i < count; i++)
                  {
                    TSNode subnode = ts_node_child (node, i);
                    if (ts_node_eq (subnode, args_node))
                      break;
                    handle_comments (subnode);
                  }
              }
              extract_from_function_call (callee_node, args_node,
                                          tenv, venv,
                                          outer_region,
                                          mlp);
              return;
            }
        }
    }

  #if DEBUG_GO && 0
  if (ts_node_symbol (node) == ts_symbol_call_expression)
    {
      TSNode subnode = ts_node_child_by_field_id (node, ts_field_function);
      fprintf (stderr, "-> %s\n", ts_node_string (subnode));
      if (ts_node_symbol (subnode) == ts_symbol_identifier)
        {
          string_desc_t subnode_string =
            sd_new_addr (ts_node_end_byte (subnode) - ts_node_start_byte (subnode),
                         contents + ts_node_start_byte (subnode));
          if (sd_equals (subnode_string, sd_from_c ("gettext")))
            {
              TSNode argsnode = ts_node_child_by_field_id (node, ts_field_arguments);
              fprintf (stderr, "gettext arguments: %s\n", ts_node_string (argsnode));
              fprintf (stderr, "gettext children:\n");
              uint32_t count = ts_node_named_child_count (node);
              for (uint32_t i = 0; i < count; i++)
                fprintf (stderr, "%u -> %s\n", i, ts_node_string (ts_node_named_child (node, i)));
            }
        }
    }
  #endif

  /* Recurse.  */
  if (ts_node_symbol (node) != ts_symbol_comment)
    {
      in_function = in_function
                    || ts_node_symbol (node) == ts_symbol_function_declaration;
      ignore = ignore
               || (ts_node_symbol (node) == ts_symbol_import_declaration)
               || is_string_literal (node);
      uint32_t count = ts_node_child_count (node);
      for (uint32_t i = 0; i < count; i++)
        {
          TSNode subnode = ts_node_child (node, i);
          handle_comments (subnode);

          #if DEBUG_GO
          /* For debugging: Show the type of parenthesized expressions.  */
          if (ts_node_symbol (subnode) == ts_symbol_parenthesized_expression)
            {
              print_type (get_type_of_expression (subnode, tenv, venv), stderr);
              fprintf (stderr, "\n");
            }
          #endif

          if (in_function
              && ts_node_symbol (node) == ts_symbol_function_declaration
              && ts_node_symbol (subnode) == ts_symbol_parameter_list)
            {
              /* Update venv.  */
              venv = augment_for_parameter_list (subnode, tenv, venv);
            }

          if (++nesting_depth > MAX_NESTING_DEPTH)
            if_error (IF_SEVERITY_FATAL_ERROR,
                      logical_file_name, ts_node_line_number (subnode), (size_t)(-1), false,
                      _("too many open parentheses"));
          extract_from_node (subnode,
                             in_function,
                             tenv, venv,
                             ignore,
                             outer_region,
                             mlp);
          nesting_depth--;

          if (in_function)
            {
              /* Update tenv and venv.  */
              if (ts_node_symbol (subnode) == ts_symbol_type_declaration)
                tenv = augment_for_type_declaration (subnode, tenv);
              else if (ts_node_symbol (subnode) == ts_symbol_var_declaration)
                venv = augment_for_variable_declaration (subnode, tenv, venv);
              else if (ts_node_symbol (subnode) == ts_symbol_const_declaration)
                venv = augment_for_const_declaration (subnode, tenv, venv);
              else if (ts_node_symbol (subnode) == ts_symbol_short_var_declaration)
                venv = augment_for_short_variable_declaration (subnode, tenv, venv);
              else if (ts_node_symbol (subnode) == ts_symbol_for_clause)
                {
                  /* tree-sitter returns a 'for' statement as
                     (for_statement (for_clause initializer: (short_var_declaration ...) ...) body: ...)
                     However, the scope of the variables declared in the short_var_declaration
                     is the entire for_statement, not just the for_clause.  */
                  TSNode initializer_node = ts_node_child_by_field_id (subnode, ts_field_initializer);
                  if (!ts_node_is_null (initializer_node)
                      && ts_node_symbol (initializer_node) == ts_symbol_short_var_declaration)
                    venv = augment_for_short_variable_declaration (initializer_node, tenv, venv);
                }
            }
       }
    }
}

void
extract_go (FILE *f,
            const char *real_filename, const char *logical_filename,
            flag_context_list_table_ty *flag_table,
            msgdomain_list_ty *mdlp)
{
  message_list_ty *mlp = mdlp->item[0]->messages;

  logical_file_name = xstrdup (logical_filename);

  last_comment_line = -1;
  last_non_comment_line = -1;

  flag_context_list_table = flag_table;
  nesting_depth = 0;

  if (ts_language == NULL)
    {
      init_gotext_package ();
      init_snapcore_package ();

      init_keywords ();

      ts_language = tree_sitter_go ();
      ts_symbol_import_declaration =
        ts_language_symbol ("import_declaration", true);
      ts_symbol_import_spec_list =
        ts_language_symbol ("import_spec_list", true);
      ts_symbol_import_spec =
        ts_language_symbol ("import_spec", true);
      ts_symbol_package_identifier =
        ts_language_symbol ("package_identifier", true);
      ts_symbol_type_declaration =
        ts_language_symbol ("type_declaration", true);
      ts_symbol_type_alias =
        ts_language_symbol ("type_alias", true);
      ts_symbol_type_spec =
        ts_language_symbol ("type_spec", true);
      ts_symbol_type_identifier =
        ts_language_symbol ("type_identifier", true);
      ts_symbol_generic_type =
        ts_language_symbol ("generic_type", true);
      ts_symbol_qualified_type =
        ts_language_symbol ("qualified_type", true);
      ts_symbol_pointer_type =
        ts_language_symbol ("pointer_type", true);
      ts_symbol_struct_type =
        ts_language_symbol ("struct_type", true);
      ts_symbol_field_declaration_list =
        ts_language_symbol ("field_declaration_list", true);
      ts_symbol_field_declaration =
        ts_language_symbol ("field_declaration", true);
      ts_symbol_interface_type =
        ts_language_symbol ("interface_type", true);
      ts_symbol_method_elem =
        ts_language_symbol ("method_elem", true);
      ts_symbol_type_elem =
        ts_language_symbol ("type_elem", true);
      ts_symbol_array_type =
        ts_language_symbol ("array_type", true);
      ts_symbol_slice_type =
        ts_language_symbol ("slice_type", true);
      ts_symbol_map_type =
        ts_language_symbol ("map_type", true);
      ts_symbol_channel_type =
        ts_language_symbol ("channel_type", true);
      ts_symbol_function_type =
        ts_language_symbol ("function_type", true);
      ts_symbol_parameter_list =
        ts_language_symbol ("parameter_list", true);
      ts_symbol_parameter_declaration =
        ts_language_symbol ("parameter_declaration", true);
      ts_symbol_variadic_parameter_declaration =
        ts_language_symbol ("variadic_parameter_declaration", true);
      ts_symbol_negated_type =
        ts_language_symbol ("negated_type", true);
      ts_symbol_parenthesized_type =
        ts_language_symbol ("parenthesized_type", true);
      ts_symbol_var_declaration =
        ts_language_symbol ("var_declaration", true);
      ts_symbol_var_spec_list =
        ts_language_symbol ("var_spec_list", true);
      ts_symbol_var_spec =
        ts_language_symbol ("var_spec", true);
      ts_symbol_const_declaration =
        ts_language_symbol ("const_declaration", true);
      ts_symbol_const_spec =
        ts_language_symbol ("const_spec", true);
      ts_symbol_short_var_declaration =
        ts_language_symbol ("short_var_declaration", true);
      ts_symbol_expression_list =
        ts_language_symbol ("expression_list", true);
      ts_symbol_unary_expression =
        ts_language_symbol ("unary_expression", true);
      ts_symbol_binary_expression =
        ts_language_symbol ("binary_expression", true);
      ts_symbol_selector_expression =
        ts_language_symbol ("selector_expression", true);
      ts_symbol_index_expression =
        ts_language_symbol ("index_expression", true);
      ts_symbol_slice_expression =
        ts_language_symbol ("slice_expression", true);
      ts_symbol_call_expression =
        ts_language_symbol ("call_expression", true);
      ts_symbol_type_assertion_expression =
        ts_language_symbol ("type_assertion_expression", true);
      ts_symbol_type_conversion_expression =
        ts_language_symbol ("type_conversion_expression", true);
      ts_symbol_type_instantiation_expression =
        ts_language_symbol ("type_instantiation_expression", true);
      ts_symbol_composite_literal =
        ts_language_symbol ("composite_literal", true);
      ts_symbol_func_literal =
        ts_language_symbol ("func_literal", true);
      ts_symbol_int_literal =
        ts_language_symbol ("int_literal", true);
      ts_symbol_float_literal =
        ts_language_symbol ("float_literal", true);
      ts_symbol_imaginary_literal =
        ts_language_symbol ("imaginary_literal", true);
      ts_symbol_rune_literal =
        ts_language_symbol ("rune_literal", true);
      ts_symbol_nil =
        ts_language_symbol ("nil", true);
      ts_symbol_true =
        ts_language_symbol ("true", true);
      ts_symbol_false =
        ts_language_symbol ("false", true);
      ts_symbol_iota =
        ts_language_symbol ("iota", true);
      ts_symbol_parenthesized_expression =
        ts_language_symbol ("parenthesized_expression", true);
      ts_symbol_function_declaration =
        ts_language_symbol ("function_declaration", true);
      ts_symbol_for_clause =
        ts_language_symbol ("for_clause", true);
      ts_symbol_comment =
        ts_language_symbol ("comment", true);
      ts_symbol_raw_string_literal =
        ts_language_symbol ("raw_string_literal", true);
      ts_symbol_raw_string_literal_content =
        ts_language_symbol ("raw_string_literal_content", true);
      ts_symbol_interpreted_string_literal =
        ts_language_symbol ("interpreted_string_literal", true);
      ts_symbol_interpreted_string_literal_content =
        ts_language_symbol ("interpreted_string_literal_content", true);
      ts_symbol_escape_sequence =
        ts_language_symbol ("escape_sequence", true);
      ts_symbol_argument_list =
        ts_language_symbol ("argument_list", true);
      ts_symbol_identifier =
        ts_language_symbol ("identifier", true);
      ts_symbol_field_identifier =
        ts_language_symbol ("field_identifier", true);
      ts_symbol_dot =
        ts_language_symbol ("dot", true);
      ts_symbol_plus = ts_language_symbol ("+", false);
      ts_field_path        = ts_language_field ("path");
      ts_field_name        = ts_language_field ("name");
      ts_field_package     = ts_language_field ("package");
      ts_field_type        = ts_language_field ("type");
      ts_field_element     = ts_language_field ("element");
      ts_field_value       = ts_language_field ("value");
      ts_field_result      = ts_language_field ("result");
      ts_field_operator    = ts_language_field ("operator");
      ts_field_left        = ts_language_field ("left");
      ts_field_right       = ts_language_field ("right");
      ts_field_function    = ts_language_field ("function");
      ts_field_arguments   = ts_language_field ("arguments");
      ts_field_operand     = ts_language_field ("operand");
      ts_field_field       = ts_language_field ("field");
      ts_field_initializer = ts_language_field ("initializer");
    }

  /* Read the file into memory.  */
  size_t contents_length;
  char *contents_data = read_file (real_filename, 0, &contents_length);
  if (contents_data == NULL)
    error (EXIT_FAILURE, errno, _("error while reading \"%s\""),
           real_filename);

  /* tree-sitter works only on files whose size fits in an uint32_t.  */
  if (contents_length > 0xFFFFFFFFUL)
    error (EXIT_FAILURE, 0, _("file \"%s\" is unsupported because too large"),
           real_filename);

  /* Go source files are UTF-8 encoded.
     <https://go.dev/ref/spec#Source_code_representation>  */
  if (u8_check ((uint8_t *) contents_data, contents_length) != NULL)
    error (EXIT_FAILURE, 0,
           _("file \"%s\" is invalid because not UTF-8 encoded"),
           real_filename);
  xgettext_current_source_encoding = po_charset_utf8;

  /* Create a parser.  */
  TSParser *parser = ts_parser_new ();

  /* Set the parser's language.  */
  ts_parser_set_language (parser, ts_language);

  /* Parse the file, producing a syntax tree.  */
  TSTree *tree = ts_parser_parse_string (parser, NULL, contents_data, contents_length);

  #if DEBUG_GO
  /* For debugging: Print the tree.  */
  {
    char *tree_as_string = ts_node_string (ts_tree_root_node (tree));
    fprintf (stderr, "Syntax tree: %s\n", tree_as_string);
    free (tree_as_string);
  }
  #endif

  contents = contents_data;

  init_package_table (ts_tree_root_node (tree));

  init_current_package_types (ts_tree_root_node (tree));
  init_current_package_globals (ts_tree_root_node (tree));

  extract_from_node (ts_tree_root_node (tree),
                     false,
                     NULL, NULL,
                     false,
                     null_context_region (),
                     mlp);

  ts_tree_delete (tree);
  ts_parser_delete (parser);
  free (contents_data);

  logical_file_name = NULL;
}
