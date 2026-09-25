/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*-
 * GObject introspection: A parser for the XML GIR format
 *
 * Copyright (C) 2005 Matthias Clasen
 * Copyright (C) 2008 Philip Van Hoof
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "config.h"

#include "girparser-private.h"

#include "girnode-private.h"
#include "gitypelib-internal.h"
#include "glib-private.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>       /* For time_t */
#include <sys/types.h>  /* For off_t on both Unix and Windows */

#ifdef G_OS_UNIX
#include <sys/socket.h> /* For socklen_t */
#endif

/* This is a "major" version in the sense that it's only bumped
 * for incompatible changes.
 */
#define SUPPORTED_GIR_VERSION "1.2"

#ifdef G_OS_WIN32

#include <windows.h>

#ifdef GIR_DIR
#undef GIR_DIR
#endif

/* GIR_DIR is used only in code called just once,
 * so no problem leaking this
 */
#define GIR_DIR \
  g_build_filename (g_win32_get_package_installation_directory_of_module(NULL), \
    "share", \
    GIR_SUFFIX, \
    NULL)
#endif

struct _GIIrParser
{
  char **includes;
  char **gi_gir_path;
  GList *parsed_modules; /* All previously parsed modules */
  GLogLevelFlags logged_levels;
};

typedef enum
{
  STATE_NONE = 0,
  STATE_START,
  STATE_END,
  STATE_REPOSITORY,
  STATE_INCLUDE,
  STATE_C_INCLUDE,     /* 5 */
  STATE_PACKAGE,
  STATE_NAMESPACE,
  STATE_ENUM,
  STATE_BITFIELD,
  STATE_FUNCTION,      /* 10 */
  STATE_FUNCTION_RETURN,
  STATE_FUNCTION_PARAMETERS,
  STATE_FUNCTION_PARAMETER,
  STATE_CLASS,
  STATE_CLASS_FIELD,   /* 15 */
  STATE_CLASS_PROPERTY,
  STATE_INTERFACE,
  STATE_INTERFACE_PROPERTY,
  STATE_INTERFACE_FIELD,
  STATE_IMPLEMENTS,    /* 20 */
  STATE_PREREQUISITE,
  STATE_BOXED,
  STATE_BOXED_FIELD,
  STATE_STRUCT,
  STATE_STRUCT_FIELD,  /* 25 */
  STATE_UNION,
  STATE_UNION_FIELD,
  STATE_NAMESPACE_CONSTANT,
  STATE_CLASS_CONSTANT,
  STATE_INTERFACE_CONSTANT,  /* 30 */
  STATE_ALIAS,
  STATE_TYPE,
  STATE_ATTRIBUTE,
  STATE_PASSTHROUGH,
  STATE_DOC_FORMAT,  /* 35 */
} ParseState;

typedef struct _ParseContext ParseContext;
struct _ParseContext
{
  GIIrParser *parser;

  ParseState state;
  int unknown_depth;
  ParseState prev_state;

  GList *modules;
  GList *include_modules;
  GPtrArray *dependencies;
  GHashTable *aliases;
  GHashTable *disguised_structures;
  GHashTable *pointer_structures;

  const char *file_path;
  const char *namespace;
  const char *c_prefix;
  GIIrModule *current_module;
  GSList *node_stack;
  char *current_alias;
  GIIrNode *current_typed;
  GList *type_stack;
  GList *type_parameters;
  int type_depth;
  ParseState in_embedded_state;
};
#define CURRENT_NODE(ctx) ((GIIrNode *)((ctx)->node_stack->data))

static void start_element_handler (GMarkupParseContext  *context,
                                   const char           *element_name,
                                   const char          **attribute_names,
                                   const char          **attribute_values,
                                   void                 *user_data,
                                   GError              **error);
static void end_element_handler   (GMarkupParseContext  *context,
                                   const char           *element_name,
                                   void                 *user_data,
                                   GError              **error);
static void text_handler          (GMarkupParseContext  *context,
                                   const char           *text,
                                   gsize                 text_len,
                                   void                 *user_data,
                                   GError              **error);
static void cleanup               (GMarkupParseContext *context,
                                   GError              *error,
                                   void                *user_data);
static void state_switch (ParseContext *ctx, ParseState newstate);


static GMarkupParser markup_parser =
{
  start_element_handler,
  end_element_handler,
  text_handler,
  NULL,
  cleanup
};

static gboolean
start_alias (GMarkupParseContext  *context,
             const char           *element_name,
             const char          **attribute_names,
             const char          **attribute_values,
             ParseContext         *ctx,
             GError              **error);
static gboolean
start_type (GMarkupParseContext  *context,
            const char           *element_name,
            const char          **attribute_names,
            const char          **attribute_values,
            ParseContext         *ctx,
            GError              **error);

static const char *find_attribute (const char  *name,
                                   const char **attribute_names,
                                   const char **attribute_values);


GIIrParser *
gi_ir_parser_new (void)
{
  GIIrParser *parser = g_slice_new0 (GIIrParser);
  const char *gi_gir_path = g_getenv ("GI_GIR_PATH");

  if (gi_gir_path != NULL)
    parser->gi_gir_path = g_strsplit (gi_gir_path, G_SEARCHPATH_SEPARATOR_S, 0);

  parser->logged_levels = G_LOG_LEVEL_MASK & ~(G_LOG_LEVEL_MESSAGE | G_LOG_LEVEL_DEBUG);
  return parser;
}

void
gi_ir_parser_set_debug (GIIrParser     *parser,
                        GLogLevelFlags  logged_levels)
{
  parser->logged_levels = logged_levels;
}

void
gi_ir_parser_free (GIIrParser *parser)
{
  g_strfreev (parser->includes);
  g_strfreev (parser->gi_gir_path);

  g_clear_list (&parser->parsed_modules, (GDestroyNotify) gi_ir_module_free);

  g_slice_free (GIIrParser, parser);
}

void
gi_ir_parser_set_includes (GIIrParser         *parser,
                           const char *const *includes)
{
  g_strfreev (parser->includes);

  parser->includes = g_strdupv ((char **)includes);
}

static void
firstpass_start_element_handler (GMarkupParseContext  *context,
                                 const char           *element_name,
                                 const char          **attribute_names,
                                 const char          **attribute_values,
                                 void                 *user_data,
                                 GError              **error)
{
  ParseContext *ctx = user_data;

  if (strcmp (element_name, "alias") == 0)
    {
      start_alias (context, element_name, attribute_names, attribute_values,
                   ctx, error);
    }
  else if (ctx->state == STATE_ALIAS && strcmp (element_name, "type") == 0)
    {
      start_type (context, element_name, attribute_names, attribute_values,
                  ctx, error);
    }
  else if (strcmp (element_name, "record") == 0)
    {
      const char *name;
      const char *disguised;
      const char *pointer;

      name = find_attribute ("name", attribute_names, attribute_values);
      disguised = find_attribute ("disguised", attribute_names, attribute_values);
      pointer = find_attribute ("pointer", attribute_names, attribute_values);

      if (g_strcmp0 (pointer, "1") == 0)
        {
          char *key;

          key = g_strdup_printf ("%s.%s", ctx->namespace, name);
          g_hash_table_replace (ctx->pointer_structures, key, GINT_TO_POINTER (1));
        }
      else if (g_strcmp0 (disguised, "1") == 0)
        {
          char *key;

          key = g_strdup_printf ("%s.%s", ctx->namespace, name);
          g_hash_table_replace (ctx->disguised_structures, key, GINT_TO_POINTER (1));
        }
    }
}

static void
firstpass_end_element_handler (GMarkupParseContext  *context,
                               const char           *element_name,
                               gpointer              user_data,
                               GError              **error)
{
  ParseContext *ctx = user_data;
  if (strcmp (element_name, "alias") == 0)
    {
      state_switch (ctx, STATE_NAMESPACE);
      g_free (ctx->current_alias);
      ctx->current_alias = NULL;
    }
  else if (strcmp (element_name, "type") == 0 && ctx->state == STATE_TYPE)
    state_switch (ctx, ctx->prev_state);
}

static GMarkupParser firstpass_parser =
{
  firstpass_start_element_handler,
  firstpass_end_element_handler,
  NULL,
  NULL,
  NULL,
};

/* If you change this search order, gobject-introspection.git and
 * tests/installed/glibconfig.py.in will probably also need updating */
static char *
locate_gir (GIIrParser *parser,
            const char *girname)
{
  const char *const *datadirs;
  const char *const *dir;
  char *path = NULL;

  g_debug ("Looking for %s", girname);
  datadirs = g_get_system_data_dirs ();

  if (parser->includes != NULL)
    {
      for (dir = (const char *const *)parser->includes; *dir; dir++)
        {
          path = g_build_filename (*dir, girname, NULL);
          g_debug ("Trying %s from includes", path);
          if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
            return g_steal_pointer (&path);
          g_clear_pointer (&path, g_free);
        }
    }

  if (parser->gi_gir_path != NULL)
    {
      for (dir = (const char *const *) parser->gi_gir_path; *dir; dir++)
        {
          if (**dir == '\0')
            continue;

          path = g_build_filename (*dir, girname, NULL);
          g_debug ("Trying %s from GI_GIR_PATH", path);
          if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
            return g_steal_pointer (&path);
          g_clear_pointer (&path, g_free);
        }
    }

  path = g_build_filename (g_get_user_data_dir (), GIR_SUFFIX, girname, NULL);
  g_debug ("Trying %s from user data dir", path);
  if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
    return g_steal_pointer (&path);
  g_clear_pointer (&path, g_free);

  for (dir = datadirs; *dir; dir++)
    {
      path = g_build_filename (*dir, GIR_SUFFIX, girname, NULL);
      g_debug ("Trying %s from system data dirs", path);
      if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
        return g_steal_pointer (&path);
      g_clear_pointer (&path, g_free);
    }

  path = g_build_filename (GIR_DIR, girname, NULL);
  g_debug ("Trying %s from GIR_DIR", path);
  if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
    return g_steal_pointer (&path);
  g_clear_pointer (&path, g_free);

  path = g_build_filename (GOBJECT_INTROSPECTION_DATADIR, GIR_SUFFIX, girname, NULL);
  g_debug ("Trying %s from DATADIR", path);
  if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
    return g_steal_pointer (&path);
  g_clear_pointer (&path, g_free);

#ifdef G_OS_UNIX
  path = g_build_filename ("/usr/share", GIR_SUFFIX, girname, NULL);
  g_debug ("Trying %s", path);
  if (g_file_test (path, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR))
    return g_steal_pointer (&path);
  g_clear_pointer (&path, g_free);
#endif

  g_debug ("Did not find %s", girname);
  return NULL;
}

#define MISSING_ATTRIBUTE(context,error,element,attribute)                                \
  do {                                                                          \
    int line_number, char_number;                                                \
    g_markup_parse_context_get_position (context, &line_number, &char_number);  \
    g_set_error (error,                                                         \
                    G_MARKUP_ERROR,                                                \
                 G_MARKUP_ERROR_INVALID_CONTENT,                                \
                 "Line %d, character %d: The attribute '%s' on the element '%s' must be specified",    \
                 line_number, char_number, attribute, element);                \
  } while (0)

#define INVALID_ATTRIBUTE(context,error,element,attribute,reason)                                \
  do {                                                                          \
    int line_number, char_number;                                                \
    g_markup_parse_context_get_position (context, &line_number, &char_number);  \
    g_set_error (error,                                                         \
                    G_MARKUP_ERROR,                                                \
                 G_MARKUP_ERROR_INVALID_CONTENT,                                \
                 "Line %d, character %d: The attribute '%s' on the element '%s' is not valid: %s",    \
                 line_number, char_number, attribute, element, reason);                \
  } while (0)

static const char *
find_attribute (const char   *name,
                const char **attribute_names,
                const char **attribute_values)
{
  size_t i;

  for (i = 0; attribute_names[i] != NULL; i++)
    if (strcmp (attribute_names[i], name) == 0)
      return attribute_values[i];

  return 0;
}

static void
state_switch (ParseContext *ctx, ParseState newstate)
{
  g_assert (ctx->state != newstate);
  ctx->prev_state = ctx->state;
  ctx->state = newstate;

  if (ctx->state == STATE_PASSTHROUGH)
    ctx->unknown_depth = 1;
}

static GIIrNode *
pop_node (ParseContext *ctx)
{
  GSList *top;
  GIIrNode *node;
  g_assert (ctx->node_stack != 0);

  top = ctx->node_stack;
  node = top->data;

  g_debug ("popping node %d %s", node->type, node->name);
  ctx->node_stack = top->next;
  g_slist_free_1 (top);
  return node;
}

static void
push_node (ParseContext *ctx, GIIrNode *node)
{
  g_assert (node != NULL);
  g_debug ("pushing node %d %s", node->type, node->name);
  ctx->node_stack = g_slist_prepend (ctx->node_stack, node);
}

static GIIrNodeType * parse_type_internal (GIIrModule  *module,
                                           const char  *str,
                                           char       **next,
                                           gboolean     in_glib,
                                           gboolean     in_gobject,
                                           GError     **error);

typedef struct {
  const char *str;
  size_t size;
  unsigned int is_signed : 1;
} IntegerAliasInfo;

static IntegerAliasInfo integer_aliases[] = {
  /* It is platform-dependent whether gchar is signed or unsigned, but
   * GObject-Introspection has traditionally treated it as signed,
   * so continue to hard-code that instead of using INTEGER_ALIAS */
  { "gchar", sizeof (gchar), 1 },

#define INTEGER_ALIAS(T) { #T, sizeof (T), G_SIGNEDNESS_OF (T) }
  INTEGER_ALIAS (guchar),
  INTEGER_ALIAS (gshort),
  INTEGER_ALIAS (gushort),
  INTEGER_ALIAS (gint),
  INTEGER_ALIAS (guint),
  INTEGER_ALIAS (glong),
  INTEGER_ALIAS (gulong),
  INTEGER_ALIAS (gssize),
  INTEGER_ALIAS (gsize),
  INTEGER_ALIAS (gintptr),
  INTEGER_ALIAS (guintptr),
  INTEGER_ALIAS (off_t),
  INTEGER_ALIAS (time_t),
#ifdef G_OS_UNIX
  INTEGER_ALIAS (dev_t),
  INTEGER_ALIAS (gid_t),
  INTEGER_ALIAS (pid_t),
  INTEGER_ALIAS (socklen_t),
  INTEGER_ALIAS (uid_t),
#endif
#undef INTEGER_ALIAS
};

typedef struct {
  const char *str;
  GITypeTag tag;
  gboolean pointer;
} BasicTypeInfo;

#define BASIC_TYPE_FIXED_OFFSET 3

static BasicTypeInfo basic_types[] = {
    { "none",      GI_TYPE_TAG_VOID,    0 },
    { "gpointer",  GI_TYPE_TAG_VOID,    1 },

    { "gboolean",  GI_TYPE_TAG_BOOLEAN, 0 },
    { "gint8",     GI_TYPE_TAG_INT8,    0 }, /* Start of BASIC_TYPE_FIXED_OFFSET */
    { "guint8",    GI_TYPE_TAG_UINT8,   0 },
    { "gint16",    GI_TYPE_TAG_INT16,   0 },
    { "guint16",   GI_TYPE_TAG_UINT16,  0 },
    { "gint32",    GI_TYPE_TAG_INT32,   0 },
    { "guint32",   GI_TYPE_TAG_UINT32,  0 },
    { "gint64",    GI_TYPE_TAG_INT64,   0 },
    { "guint64",   GI_TYPE_TAG_UINT64,  0 },
    { "gfloat",    GI_TYPE_TAG_FLOAT,   0 },
    { "gdouble",   GI_TYPE_TAG_DOUBLE,  0 },
    { "GType",     GI_TYPE_TAG_GTYPE,   0 },
    { "utf8",      GI_TYPE_TAG_UTF8,    1 },
    { "filename",  GI_TYPE_TAG_FILENAME,1 },
    { "gunichar",  GI_TYPE_TAG_UNICHAR, 0 },
};

static const BasicTypeInfo *
parse_basic (const char *str)
{
  size_t i;
  size_t n_basic = G_N_ELEMENTS (basic_types);

  for (i = 0; i < n_basic; i++)
    {
      if (strcmp (str, basic_types[i].str) == 0)
        return &(basic_types[i]);
    }
  for (i = 0; i < G_N_ELEMENTS (integer_aliases); i++)
    {
      if (strcmp (str, integer_aliases[i].str) == 0)
        {
          switch (integer_aliases[i].size)
            {
            case sizeof (uint8_t):
              if (integer_aliases[i].is_signed)
                return &basic_types[BASIC_TYPE_FIXED_OFFSET];
              else
                return &basic_types[BASIC_TYPE_FIXED_OFFSET+1];
              break;
            case sizeof (uint16_t):
              if (integer_aliases[i].is_signed)
                return &basic_types[BASIC_TYPE_FIXED_OFFSET+2];
              else
                return &basic_types[BASIC_TYPE_FIXED_OFFSET+3];
              break;
            case sizeof (uint32_t):
              if (integer_aliases[i].is_signed)
                return &basic_types[BASIC_TYPE_FIXED_OFFSET+4];
              else
                return &basic_types[BASIC_TYPE_FIXED_OFFSET+5];
              break;
            case sizeof (uint64_t):
              if (integer_aliases[i].is_signed)
                return &basic_types[BASIC_TYPE_FIXED_OFFSET+6];
              else
                return &basic_types[BASIC_TYPE_FIXED_OFFSET+7];
              break;
            default:
              g_assert_not_reached ();
            }
        }
    }
  return NULL;
}

static GIIrNodeType *
parse_type_internal (GIIrModule  *module,
                     const char  *str,
                     char       **next,
                     gboolean     in_glib,
                     gboolean     in_gobject,
                     GError     **error)
{
  const BasicTypeInfo *basic;
  GIIrNodeType *type;
  char *temporary_type = NULL;

  type = (GIIrNodeType *)gi_ir_node_new (GI_IR_NODE_TYPE, module);

  type->unparsed = g_strdup (str);

  /* See comment below on GLib.List handling */
  if (in_gobject && strcmp (str, "Type") == 0)
    {
      temporary_type = g_strdup ("GLib.Type");
      str = temporary_type;
    }

  basic = parse_basic (str);
  if (basic != NULL)
    {
      type->is_basic = TRUE;
      type->tag = basic->tag;
      type->is_pointer = basic->pointer;

      str += strlen(basic->str);
    }
  else if (in_glib)
    {
      /* If we're inside GLib, handle "List" etc. by prefixing with
       * "GLib." so the parsing code below doesn't have to get more
       * special.
       */
      if (g_str_has_prefix (str, "List<") ||
          strcmp (str, "List") == 0)
        {
          temporary_type = g_strdup_printf ("GLib.List%s", str + 4);
          str = temporary_type;
        }
      else if (g_str_has_prefix (str, "SList<") ||
          strcmp (str, "SList") == 0)
        {
          temporary_type = g_strdup_printf ("GLib.SList%s", str + 5);
          str = temporary_type;
        }
      else if (g_str_has_prefix (str, "HashTable<") ||
          strcmp (str, "HashTable") == 0)
        {
          temporary_type = g_strdup_printf ("GLib.HashTable%s", str + 9);
          str = temporary_type;
        }
      else if (g_str_has_prefix (str, "Error<") ||
          strcmp (str, "Error") == 0)
        {
          temporary_type = g_strdup_printf ("GLib.Error%s", str + 5);
          str = temporary_type;
        }
    }

  if (basic != NULL)
    /* found a basic type */;
  else if (g_str_has_prefix (str, "GLib.List") ||
           g_str_has_prefix (str, "GLib.SList"))
    {
      str += strlen ("GLib.");
      if (g_str_has_prefix (str, "List"))
        {
          type->tag = GI_TYPE_TAG_GLIST;
          type->is_glist = TRUE;
          type->is_pointer = TRUE;
          str += strlen ("List");
        }
      else
        {
          type->tag = GI_TYPE_TAG_GSLIST;
          type->is_gslist = TRUE;
          type->is_pointer = TRUE;
          str += strlen ("SList");
        }
    }
  else if (g_str_has_prefix (str, "GLib.HashTable"))
    {
      str += strlen ("GLib.");

      type->tag = GI_TYPE_TAG_GHASH;
      type->is_ghashtable = TRUE;
      type->is_pointer = TRUE;
      str += strlen ("HashTable");
    }
  else if (g_str_has_prefix (str, "GLib.Error"))
    {
      str += strlen ("GLib.");

      type->tag = GI_TYPE_TAG_ERROR;
      type->is_error = TRUE;
      type->is_pointer = TRUE;
      str += strlen ("Error");

      /* Silence a scan-build false positive */
      g_assert (str != NULL);

      if (*str == '<')
        {
          const char *end;
          char *tmp;
          (str)++;

          end = strchr (str, '>');
          if (end == NULL)
            {
              g_set_error (error, G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           "Failed to parse type ‘%s’", type->unparsed);
              goto error;
            }
          tmp = g_strndup (str, (size_t) (end - str));
          type->errors = g_strsplit (tmp, ",", 0);
          g_free (tmp);

          str = end;
        }
    }
  else
    {
      const char *start;
      type->tag = GI_TYPE_TAG_INTERFACE;
      type->is_interface = TRUE;
      start = str;

      /* must be an interface type */
      while (g_ascii_isalnum (*str) ||
             *str == '.' ||
             *str == '-' ||
             *str == '_' ||
             *str == ':')
        (str)++;

      type->giinterface = g_strndup (start, (size_t) (str - start));
    }

  if (next)
    *next = (char*)str;
  g_assert (type->tag >= 0 && type->tag < GI_TYPE_TAG_N_TYPES);
  g_free (temporary_type);
  return type;

error:
  g_assert (error == NULL || *error != NULL);
  gi_ir_node_free ((GIIrNode *)type);
  g_free (temporary_type);
  return NULL;
}

static const char *
resolve_aliases (ParseContext *ctx, const char *type)
{
  void *orig;
  void *value;
  GSList *seen_values = NULL;
  const char *lookup;
  char *prefixed;

  if (strchr (type, '.') == NULL)
    {
      prefixed = g_strdup_printf ("%s.%s", ctx->namespace, type);
      lookup = prefixed;
    }
  else
    {
      lookup = type;
      prefixed = NULL;
    }

  seen_values = g_slist_prepend (seen_values, (char*)lookup);
  while (g_hash_table_lookup_extended (ctx->current_module->aliases, lookup, &orig, &value))
    {
      g_debug ("Resolved: %s => %s", lookup, (char*)value);
      lookup = value;
      if (g_slist_find_custom (seen_values, lookup,
                               (GCompareFunc)strcmp) != NULL)
        break;
      seen_values = g_slist_prepend (seen_values, (char*) lookup);
    }
  g_slist_free (seen_values);

  if (lookup == prefixed)
    lookup = type;

  g_free (prefixed);

  return lookup;
}

static void
is_pointer_or_disguised_structure (ParseContext *ctx,
                                   const char *type,
                                   gboolean *is_pointer,
                                   gboolean *is_disguised)
{
  const char *lookup;
  char *prefixed;

  if (strchr (type, '.') == NULL)
    {
      prefixed = g_strdup_printf ("%s.%s", ctx->namespace, type);
      lookup = prefixed;
    }
  else
    {
      lookup = type;
      prefixed = NULL;
    }

  if (is_pointer != NULL)
    *is_pointer = g_hash_table_lookup (ctx->current_module->pointer_structures, lookup) != NULL;
  if (is_disguised != NULL)
    *is_disguised = g_hash_table_lookup (ctx->current_module->disguised_structures, lookup) != NULL;

  g_free (prefixed);
}

static GIIrNodeType *
parse_type (ParseContext  *ctx,
            const char    *type,
            GError       **error)
{
  GIIrNodeType *node;
  const BasicTypeInfo *basic;
  gboolean in_glib, in_gobject;

  in_glib = strcmp (ctx->namespace, "GLib") == 0;
  in_gobject = strcmp (ctx->namespace, "GObject") == 0;

  /* Do not search aliases for basic types */
  basic = parse_basic (type);
  if (basic == NULL)
    type = resolve_aliases (ctx, type);

  node = parse_type_internal (ctx->current_module, type, NULL, in_glib, in_gobject, error);
  if (node)
    g_debug ("Parsed type: %s => %d", type, node->tag);
  else
    g_debug ("Failed to parse type: '%s'", type);

  return node;
}

static gboolean
introspectable_prelude (GMarkupParseContext  *context,
                        const char          **attribute_names,
                        const char          **attribute_values,
                        ParseContext         *ctx,
                        ParseState            new_state)
{
  const char *introspectable_arg;
  const char *shadowed_by;
  gboolean introspectable;

  g_assert (ctx->state != STATE_PASSTHROUGH);

  introspectable_arg = find_attribute ("introspectable", attribute_names, attribute_values);
  shadowed_by = find_attribute ("shadowed-by", attribute_names, attribute_values);

  introspectable = !(introspectable_arg && atoi (introspectable_arg) == 0) && shadowed_by == NULL;

  if (introspectable)
    state_switch (ctx, new_state);
  else
    state_switch (ctx, STATE_PASSTHROUGH);

  return introspectable;
}

static gboolean
start_glib_boxed (GMarkupParseContext  *context,
                  const char           *element_name,
                  const char          **attribute_names,
                  const char          **attribute_values,
                  ParseContext         *ctx,
                  GError              **error)
{
  const char *name;
  const char *typename;
  const char *typeinit;
  const char *deprecated;
  GIIrNodeBoxed *boxed;

  if (!(strcmp (element_name, "glib:boxed") == 0 &&
        ctx->state == STATE_NAMESPACE))
    return FALSE;

  if (!introspectable_prelude (context, attribute_names, attribute_values, ctx, STATE_BOXED))
    return TRUE;

  name = find_attribute ("glib:name", attribute_names, attribute_values);
  typename = find_attribute ("glib:type-name", attribute_names, attribute_values);
  typeinit = find_attribute ("glib:get-type", attribute_names, attribute_values);
  deprecated = find_attribute ("deprecated", attribute_names, attribute_values);

  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "glib:name");
      return FALSE;
    }
  else if (typename == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "glib:type-name");
      return FALSE;
    }
  else if (typeinit == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "glib:get-type");
      return FALSE;
    }

  boxed = (GIIrNodeBoxed *) gi_ir_node_new (GI_IR_NODE_BOXED,
                                            ctx->current_module);

  ((GIIrNode *)boxed)->name = g_strdup (name);
  boxed->gtype_name = g_strdup (typename);
  boxed->gtype_init = g_strdup (typeinit);
  if (deprecated)
    boxed->deprecated = TRUE;
  else
    boxed->deprecated = FALSE;

  push_node (ctx, (GIIrNode *)boxed);
  ctx->current_module->entries =
    g_list_append (ctx->current_module->entries, boxed);

  return TRUE;
}

static gboolean
start_function (GMarkupParseContext  *context,
                const char           *element_name,
                const char          **attribute_names,
                const char          **attribute_values,
                ParseContext         *ctx,
                GError              **error)
{
  const char *name;
  const char *shadows;
  const char *symbol;
  const char *deprecated;
  const char *throws;
  const char *set_property;
  const char *get_property;
  const char *finish_func;
  const char *async_func;
  const char *sync_func;
  GIIrNodeFunction *function;
  gboolean found = FALSE;
  ParseState in_embedded_state = STATE_NONE;

  switch (ctx->state)
    {
    case STATE_NAMESPACE:
      found = (strcmp (element_name, "function") == 0 ||
               strcmp (element_name, "callback") == 0);
      break;
    case STATE_CLASS:
    case STATE_BOXED:
    case STATE_STRUCT:
    case STATE_UNION:
      found = strcmp (element_name, "constructor") == 0;
      /* fallthrough */
      G_GNUC_FALLTHROUGH;
    case STATE_INTERFACE:
      found = (found ||
               strcmp (element_name, "function") == 0 ||
               strcmp (element_name, "method") == 0 ||
               strcmp (element_name, "callback") == 0);
      break;
    case STATE_ENUM:
      found = strcmp (element_name, "function") == 0;
      break;
    case STATE_CLASS_FIELD:
    case STATE_STRUCT_FIELD:
      found = (found || strcmp (element_name, "callback") == 0);
      in_embedded_state = ctx->state;
      break;
    default:
      break;
    }

  if (!found)
    return FALSE;

  if (!introspectable_prelude (context, attribute_names, attribute_values, ctx, STATE_FUNCTION))
    return TRUE;

  ctx->in_embedded_state = in_embedded_state;

  name = find_attribute ("name", attribute_names, attribute_values);
  shadows = find_attribute ("shadows", attribute_names, attribute_values);
  symbol = find_attribute ("c:identifier", attribute_names, attribute_values);
  deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
  throws = find_attribute ("throws", attribute_names, attribute_values);
  set_property = find_attribute ("glib:set-property", attribute_names, attribute_values);
  get_property = find_attribute ("glib:get-property", attribute_names, attribute_values);
  finish_func = find_attribute ("glib:finish-func", attribute_names, attribute_values);
  sync_func = find_attribute ("glib:sync-func", attribute_names, attribute_values);
  async_func = find_attribute ("glib:async-func", attribute_names, attribute_values);

  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }
  else if (strcmp (element_name, "callback") != 0 && symbol == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "c:identifier");
      return FALSE;
    }

  if (shadows)
    name = shadows;

  function = (GIIrNodeFunction *) gi_ir_node_new (GI_IR_NODE_FUNCTION,
                                                  ctx->current_module);

  ((GIIrNode *)function)->name = g_strdup (name);
  function->symbol = g_strdup (symbol);
  function->parameters = NULL;
  if (deprecated)
    function->deprecated = TRUE;
  else
    function->deprecated = FALSE;

  function->is_async = FALSE;
  function->async_func = NULL;
  function->sync_func = NULL;
  function->finish_func = NULL;

  // Only asynchronous functions have a glib:sync-func defined
  if (sync_func != NULL)
    {
      if (G_UNLIKELY (async_func != NULL))
        {
          INVALID_ATTRIBUTE (context, error, element_name, "glib:sync-func", "glib:sync-func should only be defined with asynchronous "
                 "functions");
        
          return FALSE;
        }

      function->is_async = TRUE;
      function->sync_func = g_strdup (sync_func);
    }

  // Only synchronous functions have a glib:async-func defined
  if (async_func != NULL)
    {
      if (G_UNLIKELY (sync_func != NULL))
        {
          INVALID_ATTRIBUTE (context, error, element_name, "glib:async-func", "glib:async-func should only be defined with synchronous "
                 "functions");
        
          return FALSE;
        }

      function->is_async = FALSE;
      function->async_func = g_strdup (async_func);
    }

  if (finish_func != NULL)
    {
      if (G_UNLIKELY (async_func != NULL))
        {
          INVALID_ATTRIBUTE (context, error, element_name, "glib:finish-func", "glib:finish-func should only be defined with asynchronous "
                 "functions");
        
          return FALSE;
        }

      function->is_async = TRUE;
      function->finish_func = g_strdup (finish_func);
    }

  if (strcmp (element_name, "method") == 0 ||
      strcmp (element_name, "constructor") == 0)
    {
      function->is_method = TRUE;

      if (strcmp (element_name, "constructor") == 0)
        function->is_constructor = TRUE;
      else
        function->is_constructor = FALSE;

      if (set_property != NULL)
        {
          function->is_setter = TRUE;
          function->is_getter = FALSE;
          function->property = g_strdup (set_property);
        }
      else if (get_property != NULL)
        {
          function->is_setter = FALSE;
          function->is_getter = TRUE;
          function->property = g_strdup (get_property);
        }
      else
        {
          function->is_setter = FALSE;
          function->is_getter = FALSE;
          function->property = NULL;
        }
    }
  else
    {
      function->is_method = FALSE;
      function->is_setter = FALSE;
      function->is_getter = FALSE;
      function->is_constructor = FALSE;
      if (strcmp (element_name, "callback") == 0)
        ((GIIrNode *)function)->type = GI_IR_NODE_CALLBACK;
    }

  if (throws && strcmp (throws, "1") == 0)
    function->throws = TRUE;
  else
    function->throws = FALSE;

  if (ctx->node_stack == NULL)
    {
      ctx->current_module->entries =
        g_list_append (ctx->current_module->entries, function);
    }
  else if (ctx->current_typed)
    {
      GIIrNodeField *field;

      field = (GIIrNodeField *)ctx->current_typed;
      field->callback = function;
    }
  else
    switch (CURRENT_NODE (ctx)->type)
      {
      case GI_IR_NODE_INTERFACE:
      case GI_IR_NODE_OBJECT:
        {
          GIIrNodeInterface *iface;

          iface = (GIIrNodeInterface *)CURRENT_NODE (ctx);
          iface->members = g_list_append (iface->members, function);
        }
        break;
      case GI_IR_NODE_BOXED:
        {
          GIIrNodeBoxed *boxed;

          boxed = (GIIrNodeBoxed *)CURRENT_NODE (ctx);
          boxed->members = g_list_append (boxed->members, function);
        }
        break;
      case GI_IR_NODE_STRUCT:
        {
          GIIrNodeStruct *struct_;

          struct_ = (GIIrNodeStruct *)CURRENT_NODE (ctx);
          struct_->members = g_list_append (struct_->members, function);                }
        break;
      case GI_IR_NODE_UNION:
        {
          GIIrNodeUnion *union_;

          union_ = (GIIrNodeUnion *)CURRENT_NODE (ctx);
          union_->members = g_list_append (union_->members, function);
        }
        break;
      case GI_IR_NODE_ENUM:
      case GI_IR_NODE_FLAGS:
        {
          GIIrNodeEnum *enum_;

          enum_ = (GIIrNodeEnum *)CURRENT_NODE (ctx);
          enum_->methods = g_list_append (enum_->methods, function);
        }
        break;
      default:
        g_assert_not_reached ();
      }

  push_node(ctx, (GIIrNode *)function);

  return TRUE;
}

static void
parse_property_transfer (GIIrNodeProperty *property,
                         const char       *transfer,
                         ParseContext     *ctx)
{
  if (transfer == NULL)
  {
#if 0
    GIIrNodeInterface *iface = (GIIrNodeInterface *)CURRENT_NODE (ctx);

    g_debug ("required attribute 'transfer-ownership' is missing from "
             "property '%s' in type '%s.%s'. Assuming 'none'",
             property->node.name, ctx->namespace, iface->node.name);
#endif
    transfer = "none";
  }
  if (strcmp (transfer, "none") == 0)
    {
      property->transfer = FALSE;
      property->shallow_transfer = FALSE;
    }
  else if (strcmp (transfer, "container") == 0)
    {
      property->transfer = FALSE;
      property->shallow_transfer = TRUE;
    }
  else if (strcmp (transfer, "full") == 0)
    {
      property->transfer = TRUE;
      property->shallow_transfer = FALSE;
    }
  else
    {
      GIIrNodeInterface *iface = (GIIrNodeInterface *)CURRENT_NODE (ctx);

      g_warning ("Unknown transfer-ownership value: '%s' for property '%s' in "
                 "type '%s.%s'", transfer, property->node.name, ctx->namespace,
                 iface->node.name);
    }
}

static gboolean
parse_param_transfer (GIIrNodeParam *param, const char *transfer, const char *name,
                      GError **error)
{
  if (transfer == NULL)
  {
    g_set_error (error, G_MARKUP_ERROR,
                 G_MARKUP_ERROR_INVALID_CONTENT,
                 "required attribute 'transfer-ownership' missing");
    return FALSE;
  }
  else if (strcmp (transfer, "none") == 0)
    {
      param->transfer = FALSE;
      param->shallow_transfer = FALSE;
    }
  else if (strcmp (transfer, "container") == 0)
    {
      param->transfer = FALSE;
      param->shallow_transfer = TRUE;
    }
  else if (strcmp (transfer, "full") == 0)
    {
      param->transfer = TRUE;
      param->shallow_transfer = FALSE;
    }
  else
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   "invalid value for 'transfer-ownership': %s", transfer);
      return FALSE;
    }
  return TRUE;
}

static gboolean
start_instance_parameter (GMarkupParseContext  *context,
                          const char           *element_name,
                          const char          **attribute_names,
                          const char          **attribute_values,
                          ParseContext         *ctx,
                          GError              **error)
{
  const char *transfer;
  gboolean transfer_full;

  if (!(strcmp (element_name, "instance-parameter") == 0 &&
        ctx->state == STATE_FUNCTION_PARAMETERS))
    return FALSE;

  transfer = find_attribute ("transfer-ownership", attribute_names, attribute_values);

  state_switch (ctx, STATE_PASSTHROUGH);

  if (g_strcmp0 (transfer, "full") == 0)
    transfer_full = TRUE;
  else if (g_strcmp0 (transfer, "none") == 0)
    transfer_full = FALSE;
  else
    {
      g_set_error (error, G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   "invalid value for 'transfer-ownership' for instance parameter: %s", transfer);
      return FALSE;
    }

  switch (CURRENT_NODE (ctx)->type)
    {
    case GI_IR_NODE_FUNCTION:
    case GI_IR_NODE_CALLBACK:
      {
        GIIrNodeFunction *func;

        func = (GIIrNodeFunction *)CURRENT_NODE (ctx);
        func->instance_transfer_full = transfer_full;
      }
      break;
    case GI_IR_NODE_SIGNAL:
      {
        GIIrNodeSignal *signal;

        signal = (GIIrNodeSignal *)CURRENT_NODE (ctx);
        signal->instance_transfer_full = transfer_full;
      }
      break;
    case GI_IR_NODE_VFUNC:
      {
        GIIrNodeVFunc *vfunc;

        vfunc = (GIIrNodeVFunc *)CURRENT_NODE (ctx);
        vfunc->instance_transfer_full = transfer_full;
      }
      break;
    default:
      g_assert_not_reached ();
    }

  return TRUE;
}

static gboolean
start_parameter (GMarkupParseContext  *context,
                 const char           *element_name,
                 const char          **attribute_names,
                 const char          **attribute_values,
                 ParseContext         *ctx,
                 GError              **error)
{
  const char *name;
  const char *direction;
  const char *retval;
  const char *optional;
  const char *caller_allocates;
  const char *allow_none;
  const char *transfer;
  const char *scope;
  const char *closure;
  const char *destroy;
  const char *skip;
  const char *nullable;
  GIIrNodeParam *param;

  if (!(strcmp (element_name, "parameter") == 0 &&
        ctx->state == STATE_FUNCTION_PARAMETERS))
    return FALSE;

  name = find_attribute ("name", attribute_names, attribute_values);
  direction = find_attribute ("direction", attribute_names, attribute_values);
  retval = find_attribute ("retval", attribute_names, attribute_values);
  optional = find_attribute ("optional", attribute_names, attribute_values);
  allow_none = find_attribute ("allow-none", attribute_names, attribute_values);
  caller_allocates = find_attribute ("caller-allocates", attribute_names, attribute_values);
  transfer = find_attribute ("transfer-ownership", attribute_names, attribute_values);
  scope = find_attribute ("scope", attribute_names, attribute_values);
  closure = find_attribute ("closure", attribute_names, attribute_values);
  destroy = find_attribute ("destroy", attribute_names, attribute_values);
  skip = find_attribute ("skip", attribute_names, attribute_values);
  nullable = find_attribute ("nullable", attribute_names, attribute_values);

  if (name == NULL)
    name = "unknown";

  param = (GIIrNodeParam *)gi_ir_node_new (GI_IR_NODE_PARAM,
                                           ctx->current_module);

  ctx->current_typed = (GIIrNode*) param;
  ctx->current_typed->name = g_strdup (name);

  state_switch (ctx, STATE_FUNCTION_PARAMETER);

  if (direction && strcmp (direction, "out") == 0)
    {
      param->in = FALSE;
      param->out = TRUE;
      if (caller_allocates == NULL)
        param->caller_allocates = FALSE;
      else
        param->caller_allocates = strcmp (caller_allocates, "1") == 0;
    }
  else if (direction && strcmp (direction, "inout") == 0)
    {
      param->in = TRUE;
      param->out = TRUE;
      param->caller_allocates = FALSE;
    }
  else
    {
      param->in = TRUE;
      param->out = FALSE;
      param->caller_allocates = FALSE;
    }

  if (retval && strcmp (retval, "1") == 0)
    param->retval = TRUE;
  else
    param->retval = FALSE;

  if (optional && strcmp (optional, "1") == 0)
    param->optional = TRUE;
  else
    param->optional = FALSE;

  if (nullable && strcmp (nullable, "1") == 0)
    param->nullable = TRUE;
  else
    param->nullable = FALSE;

  if (allow_none && strcmp (allow_none, "1") == 0)
    {
      if (param->out)
        param->optional = TRUE;
      else
        param->nullable = TRUE;
    }

  if (skip && strcmp (skip, "1") == 0)
    param->skip = TRUE;
  else
    param->skip = FALSE;

  if (!parse_param_transfer (param, transfer, name, error))
    return FALSE;

  if (scope && strcmp (scope, "call") == 0)
    param->scope = GI_SCOPE_TYPE_CALL;
  else if (scope && strcmp (scope, "async") == 0)
    param->scope = GI_SCOPE_TYPE_ASYNC;
  else if (scope && strcmp (scope, "notified") == 0)
    param->scope = GI_SCOPE_TYPE_NOTIFIED;
  else if (scope && strcmp (scope, "forever") == 0)
    param->scope = GI_SCOPE_TYPE_FOREVER;
  else
    param->scope = GI_SCOPE_TYPE_INVALID;

  param->closure = closure ? atoi (closure) : -1;
  param->destroy = destroy ? atoi (destroy) : -1;

  switch (CURRENT_NODE (ctx)->type)
    {
    case GI_IR_NODE_FUNCTION:
    case GI_IR_NODE_CALLBACK:
      {
        GIIrNodeFunction *func;

        func = (GIIrNodeFunction *)CURRENT_NODE (ctx);
        func->parameters = g_list_append (func->parameters, param);
      }
      break;
    case GI_IR_NODE_SIGNAL:
      {
        GIIrNodeSignal *signal;

        signal = (GIIrNodeSignal *)CURRENT_NODE (ctx);
        signal->parameters = g_list_append (signal->parameters, param);
      }
      break;
    case GI_IR_NODE_VFUNC:
      {
        GIIrNodeVFunc *vfunc;

        vfunc = (GIIrNodeVFunc *)CURRENT_NODE (ctx);
        vfunc->parameters = g_list_append (vfunc->parameters, param);
      }
      break;
    default:
      g_assert_not_reached ();
    }

  return TRUE;
}

static gboolean
start_field (GMarkupParseContext  *context,
             const char           *element_name,
             const char          **attribute_names,
             const char          **attribute_values,
             ParseContext         *ctx,
             GError              **error)
{
  const char *name;
  const char *readable;
  const char *writable;
  const char *bits;
  const char *branch;
  GIIrNodeField *field;
  ParseState target_state;
  gboolean introspectable;
  guint64 parsed_bits;

  switch (ctx->state)
    {
    case STATE_CLASS:
      target_state = STATE_CLASS_FIELD;
      break;
    case STATE_BOXED:
      target_state = STATE_BOXED_FIELD;
      break;
    case STATE_STRUCT:
      target_state = STATE_STRUCT_FIELD;
      break;
    case STATE_UNION:
      target_state = STATE_UNION_FIELD;
      break;
    case STATE_INTERFACE:
      target_state = STATE_INTERFACE_FIELD;
      break;
    default:
      return FALSE;
    }

  if (strcmp (element_name, "field") != 0)
    return FALSE;

  g_assert (ctx->state != STATE_PASSTHROUGH);

  /* We handle introspectability specially here; we replace with just gpointer
   * for the type.
   */
  introspectable = introspectable_prelude (context, attribute_names, attribute_values, ctx, target_state);

  name = find_attribute ("name", attribute_names, attribute_values);
  readable = find_attribute ("readable", attribute_names, attribute_values);
  writable = find_attribute ("writable", attribute_names, attribute_values);
  bits = find_attribute ("bits", attribute_names, attribute_values);
  branch = find_attribute ("branch", attribute_names, attribute_values);

  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }

  field = (GIIrNodeField *)gi_ir_node_new (GI_IR_NODE_FIELD,
                                           ctx->current_module);
  if (introspectable)
    {
      ctx->current_typed = (GIIrNode*) field;
    }
  else
    {
      field->type = parse_type (ctx, "gpointer", NULL);
      g_assert (field->type != NULL);  /* parsing `gpointer` should never fail */
    }

  ((GIIrNode *)field)->name = g_strdup (name);
  /* Fields are assumed to be read-only.
   * (see also girwriter.py and generate.c)
   */
  field->readable = readable == NULL || strcmp (readable, "0") == 0;
  field->writable = writable != NULL && strcmp (writable, "1") == 0;

  if (bits == NULL)
    field->bits = 0;
  else if (g_ascii_string_to_unsigned (bits, 10, 0, G_MAXUINT, &parsed_bits, error))
    field->bits = parsed_bits;
  else
    {
      gi_ir_node_free ((GIIrNode *) field);
      return FALSE;
    }

  switch (CURRENT_NODE (ctx)->type)
    {
    case GI_IR_NODE_OBJECT:
      {
        GIIrNodeInterface *iface;

        iface = (GIIrNodeInterface *)CURRENT_NODE (ctx);
        iface->members = g_list_append (iface->members, field);
      }
      break;
    case GI_IR_NODE_INTERFACE:
      {
        GIIrNodeInterface *iface;

        iface = (GIIrNodeInterface *)CURRENT_NODE (ctx);
        iface->members = g_list_append (iface->members, field);
      }
      break;
    case GI_IR_NODE_BOXED:
      {
        GIIrNodeBoxed *boxed;

        boxed = (GIIrNodeBoxed *)CURRENT_NODE (ctx);
                boxed->members = g_list_append (boxed->members, field);
      }
      break;
    case GI_IR_NODE_STRUCT:
      {
        GIIrNodeStruct *struct_;

        struct_ = (GIIrNodeStruct *)CURRENT_NODE (ctx);
        struct_->members = g_list_append (struct_->members, field);
      }
      break;
    case GI_IR_NODE_UNION:
      {
        GIIrNodeUnion *union_;

        union_ = (GIIrNodeUnion *)CURRENT_NODE (ctx);
        union_->members = g_list_append (union_->members, field);
        if (branch)
          {
            GIIrNodeConstant *constant;

            constant = (GIIrNodeConstant *) gi_ir_node_new (GI_IR_NODE_CONSTANT,
                                                            ctx->current_module);
            ((GIIrNode *)constant)->name = g_strdup (name);
            constant->value = g_strdup (branch);
            constant->type = union_->discriminator_type;
            constant->deprecated = FALSE;

            union_->discriminators = g_list_append (union_->discriminators, constant);
          }
      }
      break;
    default:
      g_assert_not_reached ();
    }

  return TRUE;
}

static gboolean
start_alias (GMarkupParseContext  *context,
             const char           *element_name,
             const char          **attribute_names,
             const char          **attribute_values,
             ParseContext         *ctx,
             GError              **error)
{
  const char *name;

  name = find_attribute ("name", attribute_names, attribute_values);
  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }

  ctx->current_alias = g_strdup (name);
  state_switch (ctx, STATE_ALIAS);

  return TRUE;
}

static gboolean
start_enum (GMarkupParseContext  *context,
            const char           *element_name,
            const char          **attribute_names,
            const char          **attribute_values,
            ParseContext         *ctx,
            GError              **error)
{
  const char *name;
  const char *typename;
  const char *typeinit;
  const char *deprecated;
  const char *error_domain;
  GIIrNodeEnum *enum_;

  if (!((strcmp (element_name, "enumeration") == 0 && ctx->state == STATE_NAMESPACE) ||
        (strcmp (element_name, "bitfield") == 0 && ctx->state == STATE_NAMESPACE)))
    return FALSE;

  if (!introspectable_prelude (context, attribute_names, attribute_values, ctx, STATE_ENUM))
    return TRUE;

  name = find_attribute ("name", attribute_names, attribute_values);
  typename = find_attribute ("glib:type-name", attribute_names, attribute_values);
  typeinit = find_attribute ("glib:get-type", attribute_names, attribute_values);
  error_domain = find_attribute ("glib:error-domain", attribute_names, attribute_values);
  deprecated = find_attribute ("deprecated", attribute_names, attribute_values);

  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }

  if (strcmp (element_name, "enumeration") == 0)
    enum_ = (GIIrNodeEnum *) gi_ir_node_new (GI_IR_NODE_ENUM,
                                             ctx->current_module);
  else
    enum_ = (GIIrNodeEnum *) gi_ir_node_new (GI_IR_NODE_FLAGS,
                                             ctx->current_module);
  ((GIIrNode *)enum_)->name = g_strdup (name);
  enum_->gtype_name = g_strdup (typename);
  enum_->gtype_init = g_strdup (typeinit);
  enum_->error_domain = g_strdup (error_domain);

  if (deprecated)
    enum_->deprecated = TRUE;
  else
    enum_->deprecated = FALSE;

  push_node (ctx, (GIIrNode *) enum_);
  ctx->current_module->entries =
    g_list_append (ctx->current_module->entries, enum_);

  return TRUE;
}

static gboolean
start_property (GMarkupParseContext  *context,
                const char           *element_name,
                const char          **attribute_names,
                const char          **attribute_values,
                ParseContext         *ctx,
                GError              **error)
{
  ParseState target_state;
  const char *name;
  const char *readable;
  const char *writable;
  const char *construct;
  const char *construct_only;
  const char *transfer;
  const char *setter;
  const char *getter;
  GIIrNodeProperty *property;
  GIIrNodeInterface *iface;

  if (!(strcmp (element_name, "property") == 0 &&
        (ctx->state == STATE_CLASS ||
         ctx->state == STATE_INTERFACE)))
    return FALSE;

  if (ctx->state == STATE_CLASS)
    target_state = STATE_CLASS_PROPERTY;
  else if (ctx->state == STATE_INTERFACE)
    target_state = STATE_INTERFACE_PROPERTY;
  else
    g_assert_not_reached ();

  if (!introspectable_prelude (context, attribute_names, attribute_values, ctx, target_state))
    return TRUE;


  name = find_attribute ("name", attribute_names, attribute_values);
  readable = find_attribute ("readable", attribute_names, attribute_values);
  writable = find_attribute ("writable", attribute_names, attribute_values);
  construct = find_attribute ("construct", attribute_names, attribute_values);
  construct_only = find_attribute ("construct-only", attribute_names, attribute_values);
  transfer = find_attribute ("transfer-ownership", attribute_names, attribute_values);
  setter = find_attribute ("setter", attribute_names, attribute_values);
  getter = find_attribute ("getter", attribute_names, attribute_values);

  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }

  property = (GIIrNodeProperty *) gi_ir_node_new (GI_IR_NODE_PROPERTY,
                                                  ctx->current_module);
  ctx->current_typed = (GIIrNode*) property;

  ((GIIrNode *)property)->name = g_strdup (name);

  /* Assume properties are readable */
  if (readable == NULL || strcmp (readable, "1") == 0)
    property->readable = TRUE;
  else
    property->readable = FALSE;
  if (writable && strcmp (writable, "1") == 0)
    property->writable = TRUE;
  else
    property->writable = FALSE;
  if (construct && strcmp (construct, "1") == 0)
    property->construct = TRUE;
  else
    property->construct = FALSE;
  if (construct_only && strcmp (construct_only, "1") == 0)
    property->construct_only = TRUE;
  else
    property->construct_only = FALSE;

  property->setter = g_strdup (setter);
  property->getter = g_strdup (getter);

  parse_property_transfer (property, transfer, ctx);

  iface = (GIIrNodeInterface *)CURRENT_NODE (ctx);
  iface->members = g_list_append (iface->members, property);

  return TRUE;
}

static int64_t
parse_value (const char *str)
{
  const char *shift_op;

  /* FIXME just a quick hack */
  shift_op = strstr (str, "<<");

  if (shift_op)
    {
      int64_t base, shift;

      base = g_ascii_strtoll (str, NULL, 10);
      shift = g_ascii_strtoll (shift_op + 3, NULL, 10);

      return base << shift;
    }
  else
    return g_ascii_strtoll (str, NULL, 10);

  return 0;
}

static gboolean
start_member (GMarkupParseContext  *context,
              const char           *element_name,
              const char          **attribute_names,
              const char          **attribute_values,
              ParseContext         *ctx,
              GError              **error)
{
  const char *name;
  const char *value;
  const char *deprecated;
  const char *c_identifier;
  GIIrNodeEnum *enum_;
  GIIrNodeValue *value_;

  if (!(strcmp (element_name, "member") == 0 &&
        ctx->state == STATE_ENUM))
    return FALSE;

  name = find_attribute ("name", attribute_names, attribute_values);
  value = find_attribute ("value", attribute_names, attribute_values);
  deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
  c_identifier = find_attribute ("c:identifier", attribute_names, attribute_values);

  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }

  value_ = (GIIrNodeValue *) gi_ir_node_new (GI_IR_NODE_VALUE,
                                             ctx->current_module);

  ((GIIrNode *)value_)->name = g_strdup (name);

  value_->value = parse_value (value);

  if (deprecated)
    value_->deprecated = TRUE;
  else
    value_->deprecated = FALSE;

  g_hash_table_insert (((GIIrNode *)value_)->attributes,
                       g_strdup ("c:identifier"),
                       g_strdup (c_identifier));

  enum_ = (GIIrNodeEnum *)CURRENT_NODE (ctx);
  enum_->values = g_list_append (enum_->values, value_);

  return TRUE;
}

static gboolean
start_constant (GMarkupParseContext  *context,
                const char           *element_name,
                const char          **attribute_names,
                const char          **attribute_values,
                ParseContext         *ctx,
                GError              **error)
{
  ParseState prev_state;
  ParseState target_state;
  const char *name;
  const char *value;
  const char *deprecated;
  GIIrNodeConstant *constant;

  if (!(strcmp (element_name, "constant") == 0 &&
        (ctx->state == STATE_NAMESPACE ||
         ctx->state == STATE_CLASS ||
         ctx->state == STATE_INTERFACE)))
    return FALSE;

  switch (ctx->state)
    {
    case STATE_NAMESPACE:
      target_state = STATE_NAMESPACE_CONSTANT;
      break;
    case STATE_CLASS:
      target_state = STATE_CLASS_CONSTANT;
      break;
    case STATE_INTERFACE:
      target_state = STATE_INTERFACE_CONSTANT;
      break;
    default:
      g_assert_not_reached ();
    }

  prev_state = ctx->state;

  if (!introspectable_prelude (context, attribute_names, attribute_values, ctx, target_state))
    return TRUE;

  name = find_attribute ("name", attribute_names, attribute_values);
  value = find_attribute ("value", attribute_names, attribute_values);
  deprecated = find_attribute ("deprecated", attribute_names, attribute_values);

  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }
  else if (value == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "value");
      return FALSE;
    }

  constant = (GIIrNodeConstant *) gi_ir_node_new (GI_IR_NODE_CONSTANT,
                                                  ctx->current_module);

  ((GIIrNode *)constant)->name = g_strdup (name);
  constant->value = g_strdup (value);

  ctx->current_typed = (GIIrNode*) constant;

  if (deprecated)
    constant->deprecated = TRUE;
  else
    constant->deprecated = FALSE;

  if (prev_state == STATE_NAMESPACE)
    {
      push_node (ctx, (GIIrNode *) constant);
      ctx->current_module->entries =
        g_list_append (ctx->current_module->entries, constant);
    }
  else
    {
      GIIrNodeInterface *iface;

      iface = (GIIrNodeInterface *)CURRENT_NODE (ctx);
      iface->members = g_list_append (iface->members, constant);
    }

  return TRUE;
}

static gboolean
start_interface (GMarkupParseContext  *context,
                 const char           *element_name,
                 const char          **attribute_names,
                 const char          **attribute_values,
                 ParseContext         *ctx,
                 GError              **error)
{
  const char *name;
  const char *typename;
  const char *typeinit;
  const char *deprecated;
  const char *glib_type_struct;
  GIIrNodeInterface *iface;

  if (!(strcmp (element_name, "interface") == 0 &&
        ctx->state == STATE_NAMESPACE))
    return FALSE;

  if (!introspectable_prelude (context, attribute_names, attribute_values, ctx, STATE_INTERFACE))
    return TRUE;

  name = find_attribute ("name", attribute_names, attribute_values);
  typename = find_attribute ("glib:type-name", attribute_names, attribute_values);
  typeinit = find_attribute ("glib:get-type", attribute_names, attribute_values);
  glib_type_struct = find_attribute ("glib:type-struct", attribute_names, attribute_values);
  deprecated = find_attribute ("deprecated", attribute_names, attribute_values);

  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }
  else if (typename == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "glib:type-name");
      return FALSE;
    }
  else if (typeinit == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "glib:get-type");
      return FALSE;
    }

  iface = (GIIrNodeInterface *) gi_ir_node_new (GI_IR_NODE_INTERFACE,
                                                ctx->current_module);
  ((GIIrNode *)iface)->name = g_strdup (name);
  iface->gtype_name = g_strdup (typename);
  iface->gtype_init = g_strdup (typeinit);
  iface->glib_type_struct = g_strdup (glib_type_struct);
  if (deprecated)
    iface->deprecated = TRUE;
  else
    iface->deprecated = FALSE;

  push_node (ctx, (GIIrNode *) iface);
  ctx->current_module->entries =
    g_list_append (ctx->current_module->entries, iface);

  return TRUE;
}

static gboolean
start_class (GMarkupParseContext  *context,
             const char           *element_name,
             const char          **attribute_names,
             const char          **attribute_values,
             ParseContext         *ctx,
             GError              **error)
{
  const char *name;
  const char *parent;
  const char *glib_type_struct;
  const char *typename;
  const char *typeinit;
  const char *deprecated;
  const char *abstract;
  const char *fundamental;
  const char *final;
  const char *ref_func;
  const char *unref_func;
  const char *set_value_func;
  const char *get_value_func;
  GIIrNodeInterface *iface;

  if (!(strcmp (element_name, "class") == 0 &&
        ctx->state == STATE_NAMESPACE))
    return FALSE;

  if (!introspectable_prelude (context, attribute_names, attribute_values, ctx, STATE_CLASS))
    return TRUE;

  name = find_attribute ("name", attribute_names, attribute_values);
  parent = find_attribute ("parent", attribute_names, attribute_values);
  glib_type_struct = find_attribute ("glib:type-struct", attribute_names, attribute_values);
  typename = find_attribute ("glib:type-name", attribute_names, attribute_values);
  typeinit = find_attribute ("glib:get-type", attribute_names, attribute_values);
  deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
  abstract = find_attribute ("abstract", attribute_names, attribute_values);
  final = find_attribute ("final", attribute_names, attribute_values);
  fundamental = find_attribute ("glib:fundamental", attribute_names, attribute_values);
  ref_func = find_attribute ("glib:ref-func", attribute_names, attribute_values);
  unref_func = find_attribute ("glib:unref-func", attribute_names, attribute_values);
  set_value_func = find_attribute ("glib:set-value-func", attribute_names, attribute_values);
  get_value_func = find_attribute ("glib:get-value-func", attribute_names, attribute_values);

  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }
  else if (typename == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "glib:type-name");
      return FALSE;
    }
  else if (typeinit == NULL && strcmp (typename, "GObject"))
    {
      MISSING_ATTRIBUTE (context, error, element_name, "glib:get-type");
      return FALSE;
    }

  iface = (GIIrNodeInterface *) gi_ir_node_new (GI_IR_NODE_OBJECT,
                                                ctx->current_module);
  ((GIIrNode *)iface)->name = g_strdup (name);
  iface->gtype_name = g_strdup (typename);
  iface->gtype_init = g_strdup (typeinit);
  iface->parent = g_strdup (parent);
  iface->glib_type_struct = g_strdup (glib_type_struct);
  if (deprecated)
    iface->deprecated = TRUE;
  else
    iface->deprecated = FALSE;

  iface->abstract = abstract && strcmp (abstract, "1") == 0;
  iface->final_ = final && strcmp (final, "1") == 0;

  if (fundamental)
    iface->fundamental = TRUE;
  if (ref_func)
    iface->ref_func = g_strdup (ref_func);
  if (unref_func)
    iface->unref_func = g_strdup (unref_func);
  if (set_value_func)
    iface->set_value_func = g_strdup (set_value_func);
  if (get_value_func)
    iface->get_value_func = g_strdup (get_value_func);

  push_node (ctx, (GIIrNode *) iface);
  ctx->current_module->entries =
    g_list_append (ctx->current_module->entries, iface);

  return TRUE;
}

static gboolean
start_type (GMarkupParseContext  *context,
            const char           *element_name,
            const char          **attribute_names,
            const char          **attribute_values,
            ParseContext        *ctx,
            GError              **error)
{
  const char *name;
  const char *ctype;
  gboolean in_alias = FALSE;
  gboolean is_array;
  gboolean is_varargs;
  GIIrNodeType *typenode;

  is_array = strcmp (element_name, "array") == 0;
  is_varargs = strcmp (element_name, "varargs") == 0;

  if (!(is_array || is_varargs || (strcmp (element_name, "type") == 0)))
    return FALSE;

  if (ctx->state == STATE_TYPE)
    {
      ctx->type_depth++;
      ctx->type_stack = g_list_prepend (ctx->type_stack, ctx->type_parameters);
      ctx->type_parameters = NULL;
    }
  else if (ctx->state == STATE_FUNCTION_PARAMETER ||
           ctx->state == STATE_FUNCTION_RETURN ||
           ctx->state == STATE_STRUCT_FIELD ||
           ctx->state == STATE_UNION_FIELD ||
           ctx->state == STATE_CLASS_PROPERTY ||
           ctx->state == STATE_CLASS_FIELD ||
           ctx->state == STATE_INTERFACE_FIELD ||
           ctx->state == STATE_INTERFACE_PROPERTY ||
           ctx->state == STATE_BOXED_FIELD ||
           ctx->state == STATE_NAMESPACE_CONSTANT ||
           ctx->state == STATE_CLASS_CONSTANT ||
           ctx->state == STATE_INTERFACE_CONSTANT ||
           ctx->state == STATE_ALIAS
           )
    {
      if (ctx->state == STATE_ALIAS)
        in_alias = TRUE;
      state_switch (ctx, STATE_TYPE);
      ctx->type_depth = 1;
      ctx->type_stack = NULL;
      ctx->type_parameters = NULL;
    }

  name = find_attribute ("name", attribute_names, attribute_values);

  if (in_alias && ctx->current_alias)
    {
      char *key;
      char *value;

      if (name == NULL)
        {
          MISSING_ATTRIBUTE (context, error, element_name, "name");
          return FALSE;
        }

      key = g_strdup_printf ("%s.%s", ctx->namespace, ctx->current_alias);
      if (!strchr (name, '.'))
        {
          const BasicTypeInfo *basic = parse_basic (name);
          if (!basic)
            {
              /* For non-basic types, re-qualify the interface */
              value = g_strdup_printf ("%s.%s", ctx->namespace, name);
            }
          else
            {
              value = g_strdup (name);
            }
        }
      else
        value = g_strdup (name);

      g_hash_table_replace (ctx->aliases, key, value);

      return TRUE;
    }
  else if (!ctx->current_module || in_alias)
    return TRUE;

  if (!ctx->current_typed)
    {
      g_set_error (error,
                   G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   "The element <type> is invalid here");
      return FALSE;
    }

  if (is_varargs)
    return TRUE;

  if (is_array)
    {
      const char *zero;
      const char *len;
      const char *size;

      typenode = (GIIrNodeType *)gi_ir_node_new (GI_IR_NODE_TYPE,
                                                 ctx->current_module);

      typenode->tag = GI_TYPE_TAG_ARRAY;
      typenode->is_pointer = TRUE;
      typenode->is_array = TRUE;

      if (name && strcmp (name, "GLib.Array") == 0) {
        typenode->array_type = GI_ARRAY_TYPE_ARRAY;
      } else if (name && strcmp (name, "GLib.ByteArray") == 0) {
        typenode->array_type = GI_ARRAY_TYPE_BYTE_ARRAY;
      } else if (name && strcmp (name, "GLib.PtrArray") == 0) {
        typenode->array_type = GI_ARRAY_TYPE_PTR_ARRAY;
      } else {
        typenode->array_type = GI_ARRAY_TYPE_C;
      }

      if (typenode->array_type == GI_ARRAY_TYPE_C) {
          guint64 parsed_uint;

          zero = find_attribute ("zero-terminated", attribute_names, attribute_values);
          len = find_attribute ("length", attribute_names, attribute_values);
          size = find_attribute ("fixed-size", attribute_names, attribute_values);

          typenode->has_length = len != NULL;
          if (typenode->has_length &&
              g_ascii_string_to_unsigned (len, 10, 0, G_MAXUINT, &parsed_uint, error))
            typenode->length = parsed_uint;
          else if (typenode->has_length)
            {
              gi_ir_node_free ((GIIrNode *) typenode);
              return FALSE;
            }

          typenode->has_size = size != NULL;
          if (typenode->has_size &&
              g_ascii_string_to_unsigned (size, 10, 0, G_MAXSIZE, &parsed_uint, error))
            typenode->size = parsed_uint;
          else if (typenode->has_size)
            {
              gi_ir_node_free ((GIIrNode *) typenode);
              return FALSE;
            }

          if (zero)
            typenode->zero_terminated = strcmp(zero, "1") == 0;
          else
            /* If neither zero-terminated nor length nor fixed-size is given, assume zero-terminated. */
            typenode->zero_terminated = !(typenode->has_length || typenode->has_size);

          if (typenode->has_size && ctx->current_typed->type == GI_IR_NODE_FIELD)
            typenode->is_pointer = FALSE;
        } else {
          typenode->zero_terminated = FALSE;
          typenode->has_length = FALSE;
          typenode->has_size = FALSE;
        }
    }
  else
    {
      int pointer_depth;

      if (name == NULL)
        {
          MISSING_ATTRIBUTE (context, error, element_name, "name");
          return FALSE;
        }

      pointer_depth = 0;
      ctype = find_attribute ("c:type", attribute_names, attribute_values);
      if (ctype != NULL)
        {
          const char *cp = ctype + strlen(ctype) - 1;
          while (cp > ctype && *cp-- == '*')
            pointer_depth++;

          if (g_str_has_prefix (ctype, "gpointer")
              || g_str_has_prefix (ctype, "gconstpointer"))
            pointer_depth++;
        }

      if (ctx->current_typed->type == GI_IR_NODE_PARAM &&
          ((GIIrNodeParam *)ctx->current_typed)->out &&
          pointer_depth > 0)
        pointer_depth--;

      typenode = parse_type (ctx, name, error);
      if (typenode == NULL)
        return FALSE;

      /* A "pointer" structure is one where the c:type is a typedef that
       * to a pointer to a structure; we used to call them "disguised"
       * structures as well.
       */
      if (typenode->tag == GI_TYPE_TAG_INTERFACE)
        {
          gboolean is_pointer = FALSE;
          gboolean is_disguised = FALSE;

          is_pointer_or_disguised_structure (ctx, typenode->giinterface,
                                             &is_pointer,
                                             &is_disguised);

          if (is_pointer || is_disguised)
            pointer_depth++;
        }

      if (pointer_depth > 0)
        typenode->is_pointer = TRUE;
    }

  ctx->type_parameters = g_list_append (ctx->type_parameters, typenode);

  return TRUE;
}

static void
end_type_top (ParseContext *ctx)
{
  GIIrNodeType *typenode;

  if (!ctx->type_parameters)
    goto out;

  typenode = (GIIrNodeType *) g_steal_pointer (&ctx->type_parameters->data);

  /* Default to pointer for unspecified containers */
  if (typenode->tag == GI_TYPE_TAG_ARRAY ||
      typenode->tag == GI_TYPE_TAG_GLIST ||
      typenode->tag == GI_TYPE_TAG_GSLIST)
    {
      if (typenode->parameter_type1 == NULL)
        typenode->parameter_type1 = parse_type (ctx, "gpointer", NULL);
      g_assert (typenode->parameter_type1 != NULL);  /* parsing `gpointer` should never fail */
    }
  else if (typenode->tag == GI_TYPE_TAG_GHASH)
    {
      if (typenode->parameter_type1 == NULL)
        {
          typenode->parameter_type1 = parse_type (ctx, "gpointer", NULL);
          g_assert (typenode->parameter_type1 != NULL);  /* parsing `gpointer` should never fail */
          typenode->parameter_type2 = parse_type (ctx, "gpointer", NULL);
          g_assert (typenode->parameter_type2 != NULL);  /* same */
        }
    }

  switch (ctx->current_typed->type)
    {
    case GI_IR_NODE_PARAM:
      {
        GIIrNodeParam *param = (GIIrNodeParam *)ctx->current_typed;
        param->type = g_steal_pointer (&typenode);
      }
      break;
    case GI_IR_NODE_FIELD:
      {
        GIIrNodeField *field = (GIIrNodeField *)ctx->current_typed;
        field->type = g_steal_pointer (&typenode);
      }
      break;
    case GI_IR_NODE_PROPERTY:
      {
        GIIrNodeProperty *property = (GIIrNodeProperty *) ctx->current_typed;
        property->type = g_steal_pointer (&typenode);
      }
      break;
    case GI_IR_NODE_CONSTANT:
      {
        GIIrNodeConstant *constant = (GIIrNodeConstant *)ctx->current_typed;
        constant->type = g_steal_pointer (&typenode);
      }
      break;
    default:
      g_printerr("current node is %d\n", CURRENT_NODE (ctx)->type);
      g_assert_not_reached ();
    }
  g_list_free_full (ctx->type_parameters, (GDestroyNotify) gi_ir_node_free);

 out:
  ctx->type_depth = 0;
  ctx->type_parameters = NULL;
  ctx->current_typed = NULL;
}

static void
end_type_recurse (ParseContext *ctx)
{
  GIIrNodeType *parent;
  GIIrNodeType *param = NULL;

  parent = (GIIrNodeType *) ((GList*)ctx->type_stack->data)->data;
  if (ctx->type_parameters)
    param = (GIIrNodeType *) g_steal_pointer (&ctx->type_parameters->data);

  if (parent->tag == GI_TYPE_TAG_ARRAY ||
      parent->tag == GI_TYPE_TAG_GLIST ||
      parent->tag == GI_TYPE_TAG_GSLIST)
    {
      g_assert (param != NULL);

      if (parent->parameter_type1 == NULL)
        parent->parameter_type1 = g_steal_pointer (&param);
      else
        g_assert_not_reached ();
    }
  else if (parent->tag == GI_TYPE_TAG_GHASH)
    {
      g_assert (param != NULL);

      if (parent->parameter_type1 == NULL)
        parent->parameter_type1 = g_steal_pointer (&param);
      else if (parent->parameter_type2 == NULL)
        parent->parameter_type2 = g_steal_pointer (&param);
      else
        g_assert_not_reached ();
    }

  if (param != NULL)
    gi_ir_node_free ((GIIrNode *) param);
  param = NULL;

  g_list_free_full (ctx->type_parameters, (GDestroyNotify) gi_ir_node_free);
  ctx->type_parameters = (GList *)ctx->type_stack->data;
  ctx->type_stack = g_list_delete_link (ctx->type_stack, ctx->type_stack);
}

static void
end_type (ParseContext *ctx)
{
  if (ctx->type_depth == 1)
    {
      end_type_top (ctx);
      state_switch (ctx, ctx->prev_state);
    }
  else
    {
      end_type_recurse (ctx);
      ctx->type_depth--;
    }
}

static gboolean
start_attribute (GMarkupParseContext  *context,
                 const char           *element_name,
                 const char          **attribute_names,
                 const char          **attribute_values,
                 ParseContext        *ctx,
                 GError              **error)
{
  const char *name;
  const char *value;
  GIIrNode *curnode;

  if (strcmp (element_name, "attribute") != 0 || ctx->node_stack == NULL)
    return FALSE;

  name = find_attribute ("name", attribute_names, attribute_values);
  value = find_attribute ("value", attribute_names, attribute_values);

  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }
  if (value == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "value");
      return FALSE;
    }

  state_switch (ctx, STATE_ATTRIBUTE);

  curnode = CURRENT_NODE (ctx);

  if (ctx->current_typed && ctx->current_typed->type == GI_IR_NODE_PARAM)
    {
      g_hash_table_insert (ctx->current_typed->attributes, g_strdup (name), g_strdup (value));
    }
  else
    {
      g_hash_table_insert (curnode->attributes, g_strdup (name), g_strdup (value));
    }

  return TRUE;
}

static gboolean
start_return_value (GMarkupParseContext  *context,
                    const char           *element_name,
                    const char          **attribute_names,
                    const char          **attribute_values,
                    ParseContext        *ctx,
                    GError              **error)
{
  GIIrNodeParam *param;
  const char *transfer;
  const char *skip;
  const char *nullable;

  if (!(strcmp (element_name, "return-value") == 0 &&
        ctx->state == STATE_FUNCTION))
    return FALSE;

  param = (GIIrNodeParam *)gi_ir_node_new (GI_IR_NODE_PARAM,
                                           ctx->current_module);
  param->in = FALSE;
  param->out = FALSE;
  param->retval = TRUE;

  ctx->current_typed = (GIIrNode*) param;

  state_switch (ctx, STATE_FUNCTION_RETURN);

  skip = find_attribute ("skip", attribute_names, attribute_values);
  if (skip && strcmp (skip, "1") == 0)
    param->skip = TRUE;
  else
    param->skip = FALSE;

  transfer = find_attribute ("transfer-ownership", attribute_names, attribute_values);
  if (!parse_param_transfer (param, transfer, NULL, error))
    return FALSE;

  nullable = find_attribute ("nullable", attribute_names, attribute_values);
  if (nullable && g_str_equal (nullable, "1"))
    param->nullable = TRUE;

  switch (CURRENT_NODE (ctx)->type)
    {
    case GI_IR_NODE_FUNCTION:
    case GI_IR_NODE_CALLBACK:
      {
        GIIrNodeFunction *func = (GIIrNodeFunction *)CURRENT_NODE (ctx);
        func->result = param;
      }
      break;
    case GI_IR_NODE_SIGNAL:
      {
        GIIrNodeSignal *signal = (GIIrNodeSignal *)CURRENT_NODE (ctx);
        signal->result = param;
      }
      break;
    case GI_IR_NODE_VFUNC:
      {
        GIIrNodeVFunc *vfunc = (GIIrNodeVFunc *)CURRENT_NODE (ctx);
        vfunc->result = param;
      }
      break;
    default:
      g_assert_not_reached ();
    }

  return TRUE;
}

static gboolean
start_implements (GMarkupParseContext  *context,
                  const char           *element_name,
                  const char          **attribute_names,
                  const char          **attribute_values,
                  ParseContext        *ctx,
                  GError              **error)
{
  GIIrNodeInterface *iface;
  const char *name;

  if (strcmp (element_name, "implements") != 0 ||
      !(ctx->state == STATE_CLASS))
    return FALSE;

  state_switch (ctx, STATE_IMPLEMENTS);

  name = find_attribute ("name", attribute_names, attribute_values);
  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }

  iface = (GIIrNodeInterface *)CURRENT_NODE (ctx);
  iface->interfaces = g_list_append (iface->interfaces, g_strdup (name));

  return TRUE;
}

static gboolean
start_glib_signal (GMarkupParseContext  *context,
                   const char           *element_name,
                   const char          **attribute_names,
                   const char          **attribute_values,
                   ParseContext        *ctx,
                   GError              **error)
{
  const char *name;
  const char *when;
  const char *no_recurse;
  const char *detailed;
  const char *action;
  const char *no_hooks;
  const char *has_class_closure;
  GIIrNodeInterface *iface;
  GIIrNodeSignal *signal;

  if (!(strcmp (element_name, "glib:signal") == 0 &&
        (ctx->state == STATE_CLASS ||
         ctx->state == STATE_INTERFACE)))
    return FALSE;

  if (!introspectable_prelude (context, attribute_names, attribute_values, ctx, STATE_FUNCTION))
    return TRUE;

  name = find_attribute ("name", attribute_names, attribute_values);
  when = find_attribute ("when", attribute_names, attribute_values);
  no_recurse = find_attribute ("no-recurse", attribute_names, attribute_values);
  detailed = find_attribute ("detailed", attribute_names, attribute_values);
  action = find_attribute ("action", attribute_names, attribute_values);
  no_hooks = find_attribute ("no-hooks", attribute_names, attribute_values);
  has_class_closure = find_attribute ("has-class-closure", attribute_names, attribute_values);

  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }
  signal = (GIIrNodeSignal *)gi_ir_node_new (GI_IR_NODE_SIGNAL,
                                             ctx->current_module);

  ((GIIrNode *)signal)->name = g_strdup (name);

  signal->run_first = FALSE;
  signal->run_last = FALSE;
  signal->run_cleanup = FALSE;
  if (when == NULL || g_ascii_strcasecmp (when, "LAST") == 0)
    signal->run_last = TRUE;
  else if (g_ascii_strcasecmp (when, "FIRST") == 0)
    signal->run_first = TRUE;
  else
    signal->run_cleanup = TRUE;

  if (no_recurse && strcmp (no_recurse, "1") == 0)
    signal->no_recurse = TRUE;
  else
    signal->no_recurse = FALSE;
  if (detailed && strcmp (detailed, "1") == 0)
    signal->detailed = TRUE;
  else
    signal->detailed = FALSE;
  if (action && strcmp (action, "1") == 0)
    signal->action = TRUE;
  else
    signal->action = FALSE;
  if (no_hooks && strcmp (no_hooks, "1") == 0)
    signal->no_hooks = TRUE;
  else
    signal->no_hooks = FALSE;
  if (has_class_closure && strcmp (has_class_closure, "1") == 0)
    signal->has_class_closure = TRUE;
  else
    signal->has_class_closure = FALSE;

  iface = (GIIrNodeInterface *)CURRENT_NODE (ctx);
  iface->members = g_list_append (iface->members, signal);

  push_node (ctx, (GIIrNode *)signal);

  return TRUE;
}

static gboolean
start_vfunc (GMarkupParseContext  *context,
             const char           *element_name,
             const char          **attribute_names,
             const char          **attribute_values,
             ParseContext        *ctx,
             GError              **error)
{
  const char *name;
  const char *must_chain_up;
  const char *override;
  const char *is_class_closure;
  const char *offset;
  const char *invoker;
  const char *throws;
  const char *is_static;
  const char *finish_func;
  const char *async_func;
  const char *sync_func;
  GIIrNodeInterface *iface;
  GIIrNodeVFunc *vfunc;
  guint64 parsed_offset;

  if (!(strcmp (element_name, "virtual-method") == 0 &&
        (ctx->state == STATE_CLASS ||
         ctx->state == STATE_INTERFACE)))
    return FALSE;

  if (!introspectable_prelude (context, attribute_names, attribute_values, ctx, STATE_FUNCTION))
    return TRUE;

  name = find_attribute ("name", attribute_names, attribute_values);
  must_chain_up = find_attribute ("must-chain-up", attribute_names, attribute_values);
  override = find_attribute ("override", attribute_names, attribute_values);
  is_class_closure = find_attribute ("is-class-closure", attribute_names, attribute_values);
  offset = find_attribute ("offset", attribute_names, attribute_values);
  invoker = find_attribute ("invoker", attribute_names, attribute_values);
  throws = find_attribute ("throws", attribute_names, attribute_values);
  is_static = find_attribute ("glib:static", attribute_names, attribute_values);
  finish_func = find_attribute ("glib:finish-func", attribute_names, attribute_values);
  sync_func = find_attribute ("glib:sync-func", attribute_names, attribute_values);
  async_func = find_attribute ("glib:async-func", attribute_names, attribute_values);

  if (name == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }

  vfunc = (GIIrNodeVFunc *)gi_ir_node_new (GI_IR_NODE_VFUNC,
                                           ctx->current_module);

  ((GIIrNode *)vfunc)->name = g_strdup (name);

  if (must_chain_up && strcmp (must_chain_up, "1") == 0)
    vfunc->must_chain_up = TRUE;
  else
    vfunc->must_chain_up = FALSE;

  if (override && strcmp (override, "always") == 0)
    {
      vfunc->must_be_implemented = TRUE;
      vfunc->must_not_be_implemented = FALSE;
    }
  else if (override && strcmp (override, "never") == 0)
    {
      vfunc->must_be_implemented = FALSE;
      vfunc->must_not_be_implemented = TRUE;
    }
  else
    {
      vfunc->must_be_implemented = FALSE;
      vfunc->must_not_be_implemented = FALSE;
    }

  if (is_class_closure && strcmp (is_class_closure, "1") == 0)
    vfunc->is_class_closure = TRUE;
  else
    vfunc->is_class_closure = FALSE;

  if (throws && strcmp (throws, "1") == 0)
    vfunc->throws = TRUE;
  else
    vfunc->throws = FALSE;

  if (is_static && strcmp (is_static, "1") == 0)
    vfunc->is_static = TRUE;
  else
    vfunc->is_static = FALSE;

  if (offset == NULL)
    vfunc->offset = 0xFFFF;
  else if (g_ascii_string_to_unsigned (offset, 10, 0, G_MAXSIZE, &parsed_offset, error))
    vfunc->offset = parsed_offset;
  else
    {
      gi_ir_node_free ((GIIrNode *) vfunc);
      return FALSE;
    }

  vfunc->is_async = FALSE;
  vfunc->async_func = NULL;
  vfunc->sync_func = NULL;
  vfunc->finish_func = NULL;

  // Only asynchronous functions have a glib:sync-func defined
  if (sync_func != NULL)
    {
      if (G_UNLIKELY (async_func != NULL))
        {
          INVALID_ATTRIBUTE (context, error, element_name, "glib:sync-func", "glib:sync-func should only be defined with asynchronous "
                 "functions");
        
          return FALSE;
        }

      vfunc->is_async = TRUE;
      vfunc->sync_func = g_strdup (sync_func);
    }

  // Only synchronous functions have a glib:async-func defined
  if (async_func != NULL)
    {
      if (G_UNLIKELY (sync_func != NULL))
        {
          INVALID_ATTRIBUTE (context, error, element_name, "glib:async-func", "glib:async-func should only be defined with synchronous "
                 "functions");
        
          return FALSE;
        }

      vfunc->is_async = FALSE;
      vfunc->async_func = g_strdup (async_func);
    }

  if (finish_func != NULL)
    {
      if (G_UNLIKELY (async_func != NULL))
        {
          INVALID_ATTRIBUTE (context, error, element_name, "glib:finish-func", "glib:finish-func should only be defined with asynchronous "
                 "functions");
        
          return FALSE;
        }

      vfunc->is_async = TRUE;
      vfunc->finish_func = g_strdup (finish_func);
    }


  vfunc->invoker = g_strdup (invoker);

  iface = (GIIrNodeInterface *)CURRENT_NODE (ctx);
  iface->members = g_list_append (iface->members, vfunc);

  push_node (ctx, (GIIrNode *)vfunc);

  return TRUE;
}

static gboolean
start_struct (GMarkupParseContext  *context,
              const char           *element_name,
              const char          **attribute_names,
              const char          **attribute_values,
              ParseContext         *ctx,
              GError               **error)
{
  const char *name;
  const char *deprecated;
  const char *disguised;
  const char *opaque;
  const char *pointer;
  const char *gtype_name;
  const char *gtype_init;
  const char *gtype_struct;
  const char *foreign;
  const char *copy_func;
  const char *free_func;
  GIIrNodeStruct *struct_;

  if (!(strcmp (element_name, "record") == 0 &&
        (ctx->state == STATE_NAMESPACE ||
         ctx->state == STATE_UNION ||
         ctx->state == STATE_STRUCT ||
         ctx->state == STATE_CLASS)))
    return FALSE;

  if (!introspectable_prelude (context, attribute_names, attribute_values, ctx, STATE_STRUCT))
    return TRUE;

  name = find_attribute ("name", attribute_names, attribute_values);
  deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
  disguised = find_attribute ("disguised", attribute_names, attribute_values);
  pointer = find_attribute ("pointer", attribute_names, attribute_values);
  opaque = find_attribute ("opaque", attribute_names, attribute_values);
  gtype_name = find_attribute ("glib:type-name", attribute_names, attribute_values);
  gtype_init = find_attribute ("glib:get-type", attribute_names, attribute_values);
  gtype_struct = find_attribute ("glib:is-gtype-struct-for", attribute_names, attribute_values);
  foreign = find_attribute ("foreign", attribute_names, attribute_values);
  copy_func = find_attribute ("copy-function", attribute_names, attribute_values);
  free_func = find_attribute ("free-function", attribute_names, attribute_values);

  if (name == NULL && ctx->node_stack == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }
  if ((gtype_name == NULL && gtype_init != NULL))
    {
      MISSING_ATTRIBUTE (context, error, element_name, "glib:type-name");
      return FALSE;
    }
  if ((gtype_name != NULL && gtype_init == NULL))
    {
      MISSING_ATTRIBUTE (context, error, element_name, "glib:get-type");
      return FALSE;
    }

  struct_ = (GIIrNodeStruct *) gi_ir_node_new (GI_IR_NODE_STRUCT,
                                               ctx->current_module);

  ((GIIrNode *)struct_)->name = g_strdup (name ? name : "");
  if (deprecated)
    struct_->deprecated = TRUE;
  else
    struct_->deprecated = FALSE;

  if (g_strcmp0 (disguised, "1") == 0)
    struct_->disguised = TRUE;

  if (g_strcmp0 (pointer, "1") == 0)
    struct_->pointer = TRUE;

  if (g_strcmp0 (opaque, "1") == 0)
    struct_->opaque = TRUE;

  struct_->is_gtype_struct = gtype_struct != NULL;

  struct_->gtype_name = g_strdup (gtype_name);
  struct_->gtype_init = g_strdup (gtype_init);

  struct_->foreign = (g_strcmp0 (foreign, "1") == 0);

  struct_->copy_func = g_strdup (copy_func);
  struct_->free_func = g_strdup (free_func);

  if (ctx->node_stack == NULL)
    ctx->current_module->entries =
      g_list_append (ctx->current_module->entries, struct_);
  push_node (ctx, (GIIrNode *)struct_);
  return TRUE;
}

static gboolean
start_union (GMarkupParseContext  *context,
             const char           *element_name,
             const char          **attribute_names,
             const char          **attribute_values,
             ParseContext        *ctx,
             GError              **error)
{
  const char *name;
  const char *deprecated;
  const char *typename;
  const char *typeinit;
  const char *copy_func;
  const char *free_func;
  GIIrNodeUnion *union_;

  if (!(strcmp (element_name, "union") == 0 &&
        (ctx->state == STATE_NAMESPACE ||
         ctx->state == STATE_UNION ||
         ctx->state == STATE_STRUCT ||
         ctx->state == STATE_CLASS)))
    return FALSE;

  if (!introspectable_prelude (context, attribute_names, attribute_values, ctx, STATE_UNION))
    return TRUE;

  name = find_attribute ("name", attribute_names, attribute_values);
  deprecated = find_attribute ("deprecated", attribute_names, attribute_values);
  typename = find_attribute ("glib:type-name", attribute_names, attribute_values);
  typeinit = find_attribute ("glib:get-type", attribute_names, attribute_values);
  copy_func = find_attribute ("copy-function", attribute_names, attribute_values);
  free_func = find_attribute ("free-function", attribute_names, attribute_values);

  if (name == NULL && ctx->node_stack == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "name");
      return FALSE;
    }

  union_ = (GIIrNodeUnion *) gi_ir_node_new (GI_IR_NODE_UNION,
                                             ctx->current_module);

  ((GIIrNode *)union_)->name = g_strdup (name ? name : "");
  union_->gtype_name = g_strdup (typename);
  union_->gtype_init = g_strdup (typeinit);
  union_->copy_func = g_strdup (copy_func);
  union_->free_func = g_strdup (free_func);
  if (deprecated)
    union_->deprecated = TRUE;
  else
    union_->deprecated = FALSE;

  if (ctx->node_stack == NULL)
    ctx->current_module->entries =
      g_list_append (ctx->current_module->entries, union_);
  push_node (ctx, (GIIrNode *)union_);
  return TRUE;
}

static gboolean
start_discriminator (GMarkupParseContext  *context,
                     const char           *element_name,
                     const char          **attribute_names,
                     const char          **attribute_values,
                     ParseContext         *ctx,
                     GError              **error)
{
  const char *type;
  const char *offset;
  guint64 parsed_offset;
  GIIrNodeType *discriminator_type = NULL;

  if (!(strcmp (element_name, "discriminator") == 0 &&
        ctx->state == STATE_UNION))
    return FALSE;

  type = find_attribute ("type", attribute_names, attribute_values);
  offset = find_attribute ("offset", attribute_names, attribute_values);
  if (type == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "type");
      return FALSE;
    }
  else if (offset == NULL)
    {
      MISSING_ATTRIBUTE (context, error, element_name, "offset");
      return FALSE;
    }

  discriminator_type = parse_type (ctx, type, error);
  if (discriminator_type == NULL)
    return FALSE;

  ((GIIrNodeUnion *)CURRENT_NODE (ctx))->discriminator_type = g_steal_pointer (&discriminator_type);

  if (g_ascii_string_to_unsigned (offset, 10, 0, G_MAXSIZE, &parsed_offset, error))
    ((GIIrNodeUnion *)CURRENT_NODE (ctx))->discriminator_offset = parsed_offset;
  else
    return FALSE;

  return TRUE;
}

static gboolean
parse_include (GMarkupParseContext *context,
               ParseContext        *ctx,
               const char          *name,
               const char          *version)
{
  GError *error = NULL;
  char *buffer;
  gsize length;
  char *girpath, *girname;
  GIIrModule *module;
  GList *l;

  for (l = ctx->parser->parsed_modules; l; l = l->next)
    {
      GIIrModule *m = l->data;

      if (strcmp (m->name, name) == 0)
        {
          if (strcmp (m->version, version) == 0)
            {
              ctx->include_modules = g_list_prepend (ctx->include_modules, m);

              return TRUE;
            }
          else
            {
              g_printerr ("Module '%s' imported with conflicting versions '%s' and '%s'\n",
                          name, m->version, version);
              return FALSE;
            }
        }
    }

  girname = g_strdup_printf ("%s-%s.gir", name, version);
  girpath = locate_gir (ctx->parser, girname);

  if (girpath == NULL)
    {
      g_printerr ("Could not find GIR file '%s'; check XDG_DATA_DIRS or use --includedir\n",
                   girname);
      g_free (girname);
      return FALSE;
    }
  g_free (girname);

  g_debug ("Parsing include %s", girpath);

  if (!g_file_get_contents (girpath, &buffer, &length, &error))
    {
      g_printerr ("%s: %s\n", girpath, error->message);
      g_clear_error (&error);
      g_free (girpath);
      return FALSE;
    }

  if (length > G_MAXSSIZE)
    {
      g_printerr ("Input file ‘%s’ too big\n", girpath);
      g_free (girpath);
      return FALSE;
    }

  module = gi_ir_parser_parse_string (ctx->parser, name, girpath, buffer, (gssize) length, &error);
  g_free (buffer);
  if (error != NULL)
    {
      int line_number, char_number;
      g_markup_parse_context_get_position (context, &line_number, &char_number);
      g_printerr ("%s:%d:%d: error: %s\n", girpath, line_number, char_number, error->message);
      g_clear_error (&error);
      g_free (girpath);
      return FALSE;
    }
  g_free (girpath);

  ctx->include_modules = g_list_append (ctx->include_modules,
                                        module);

  return TRUE;
}

static void
start_element_handler (GMarkupParseContext  *context,
                       const char           *element_name,
                       const char          **attribute_names,
                       const char          **attribute_values,
                       gpointer              user_data,
                       GError              **error)
{
  ParseContext *ctx = user_data;

  if (ctx->parser->logged_levels & G_LOG_LEVEL_DEBUG)
    {
      GString *tags = g_string_new ("");
      int i;
      for (i = 0; attribute_names[i]; i++)
        g_string_append_printf (tags, "%s=\"%s\" ",
                                attribute_names[i],
                                attribute_values[i]);

      if (i)
        {
          g_string_insert_c (tags, 0, ' ');
          g_string_truncate (tags, tags->len - 1);
        }
      g_debug ("<%s%s>", element_name, tags->str);
      g_string_free (tags, TRUE);
    }

  if (ctx->state == STATE_PASSTHROUGH)
    {
      ctx->unknown_depth += 1;
      return;
    }

  switch (element_name[0])
    {
    case 'a':
      if (ctx->state == STATE_NAMESPACE && strcmp (element_name, "alias") == 0)
        {
          state_switch (ctx, STATE_ALIAS);
          goto out;
        }
      if (start_type (context, element_name,
                      attribute_names, attribute_values,
                      ctx, error))
        goto out;
      else if (start_attribute (context, element_name,
                                attribute_names, attribute_values,
                                ctx, error))
        goto out;
      break;
    case 'b':
      if (start_enum (context, element_name,
                      attribute_names, attribute_values,
                      ctx, error))
        goto out;
      break;
    case 'c':
      if (start_function (context, element_name,
                          attribute_names, attribute_values,
                          ctx, error))
        goto out;
      else if (start_constant (context, element_name,
                               attribute_names, attribute_values,
                               ctx, error))
        goto out;
      else if (start_class (context, element_name,
                            attribute_names, attribute_values,
                            ctx, error))
        goto out;
      break;

    case 'd':
      if (start_discriminator (context, element_name,
                               attribute_names, attribute_values,
                               ctx, error))
    goto out;
      if (strcmp ("doc", element_name) == 0 || strcmp ("doc-deprecated", element_name) == 0 ||
          strcmp ("doc-stability", element_name) == 0 || strcmp ("doc-version", element_name) == 0 ||
          strcmp ("docsection", element_name) == 0)
        {
          state_switch (ctx, STATE_PASSTHROUGH);
          goto out;
        }
      else if (strcmp ("doc:format", element_name) == 0)
        {
          state_switch (ctx, STATE_DOC_FORMAT);
          goto out;
        }
      break;

    case 'e':
      if (start_enum (context, element_name,
                      attribute_names, attribute_values,
                      ctx, error))
        goto out;
      break;

    case 'f':
      if (strcmp ("function-macro", element_name) == 0 ||
          strcmp ("function-inline", element_name) == 0)
        {
          state_switch (ctx, STATE_PASSTHROUGH);
          goto out;
        }
      else if (start_function (context, element_name,
                          attribute_names, attribute_values,
                          ctx, error))
        goto out;
      else if (start_field (context, element_name,
                            attribute_names, attribute_values,
                            ctx, error))
        goto out;
      break;

    case 'g':
      if (start_glib_boxed (context, element_name,
                            attribute_names, attribute_values,
                            ctx, error))
        goto out;
      else if (start_glib_signal (context, element_name,
                             attribute_names, attribute_values,
                             ctx, error))
        goto out;
      break;

    case 'i':
      if (strcmp (element_name, "include") == 0 &&
          ctx->state == STATE_REPOSITORY)
        {
          const char *name;
          const char *version;

          name = find_attribute ("name", attribute_names, attribute_values);
          version = find_attribute ("version", attribute_names, attribute_values);

          if (name == NULL)
            {
              MISSING_ATTRIBUTE (context, error, element_name, "name");
              break;
            }
          if (version == NULL)
            {
              MISSING_ATTRIBUTE (context, error, element_name, "version");
              break;
            }

          if (!parse_include (context, ctx, name, version))
            {
              g_set_error (error,
                           G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           "Failed to parse included gir %s-%s",
                           name,
                           version);
              return;
            }

          g_ptr_array_insert (ctx->dependencies, 0,
                              g_strdup_printf ("%s-%s", name, version));

          state_switch (ctx, STATE_INCLUDE);
          goto out;
        }
      if (start_interface (context, element_name,
                           attribute_names, attribute_values,
                           ctx, error))
        goto out;
      else if (start_implements (context, element_name,
                                 attribute_names, attribute_values,
                                 ctx, error))
        goto out;
      else if (start_instance_parameter (context, element_name,
                                attribute_names, attribute_values,
                                ctx, error))
        goto out;
      else if (strcmp (element_name, "c:include") == 0)
        {
          state_switch (ctx, STATE_C_INCLUDE);
          goto out;
        }
      break;

    case 'm':
      if (strcmp (element_name, "method-inline") == 0)
        {
          state_switch (ctx, STATE_PASSTHROUGH);
          goto out;
        }
      else if (start_function (context, element_name,
                          attribute_names, attribute_values,
                          ctx, error))
        goto out;
      else if (start_member (context, element_name,
                          attribute_names, attribute_values,
                          ctx, error))
        goto out;
      break;

    case 'n':
      if (strcmp (element_name, "namespace") == 0 && ctx->state == STATE_REPOSITORY)
        {
          const char *name, *version, *shared_library, *cprefix;

          if (ctx->current_module != NULL)
            {
              g_set_error (error,
                           G_MARKUP_ERROR,
                           G_MARKUP_ERROR_INVALID_CONTENT,
                           "Only one <namespace/> element is currently allowed per <repository/>");
              goto out;
            }

          name = find_attribute ("name", attribute_names, attribute_values);
          version = find_attribute ("version", attribute_names, attribute_values);
          shared_library = find_attribute ("shared-library", attribute_names, attribute_values);
          cprefix = find_attribute ("c:identifier-prefixes", attribute_names, attribute_values);
          /* Backwards compatibility; vala currently still generates this */
          if (cprefix == NULL)
            cprefix = find_attribute ("c:prefix", attribute_names, attribute_values);

          if (name == NULL)
            MISSING_ATTRIBUTE (context, error, element_name, "name");
          else if (version == NULL)
            MISSING_ATTRIBUTE (context, error, element_name, "version");
          else
            {
              GList *l;

              if (strcmp (name, ctx->namespace) != 0)
                g_set_error (error,
                             G_MARKUP_ERROR,
                             G_MARKUP_ERROR_INVALID_CONTENT,
                             "<namespace/> name element '%s' doesn't match file name '%s'",
                             name, ctx->namespace);

              ctx->current_module = gi_ir_module_new (name, version, shared_library, cprefix);

              ctx->current_module->aliases = ctx->aliases;
              ctx->aliases = NULL;
              ctx->current_module->disguised_structures = ctx->disguised_structures;
              ctx->current_module->pointer_structures = ctx->pointer_structures;
              ctx->disguised_structures = NULL;
              ctx->pointer_structures = NULL;

              for (l = ctx->include_modules; l; l = l->next)
                gi_ir_module_add_include_module (ctx->current_module, l->data);

              g_list_free (ctx->include_modules);
              ctx->include_modules = NULL;

              ctx->modules = g_list_append (ctx->modules, ctx->current_module);

              if (ctx->current_module->dependencies != ctx->dependencies)
                {
                  g_clear_pointer (&ctx->current_module->dependencies, g_ptr_array_unref);
                  ctx->current_module->dependencies = g_ptr_array_ref (ctx->dependencies);
                }

              state_switch (ctx, STATE_NAMESPACE);
              goto out;
            }
        }
      break;

    case 'p':
      if (start_property (context, element_name,
                          attribute_names, attribute_values,
                          ctx, error))
        goto out;
      else if (strcmp (element_name, "parameters") == 0 &&
               ctx->state == STATE_FUNCTION)
        {
          state_switch (ctx, STATE_FUNCTION_PARAMETERS);

          goto out;
        }
      else if (start_parameter (context, element_name,
                                attribute_names, attribute_values,
                                ctx, error))
        goto out;
      else if (strcmp (element_name, "prerequisite") == 0 &&
               ctx->state == STATE_INTERFACE)
        {
          const char *name;

          name = find_attribute ("name", attribute_names, attribute_values);

          state_switch (ctx, STATE_PREREQUISITE);

          if (name == NULL)
            MISSING_ATTRIBUTE (context, error, element_name, "name");
          else
            {
              GIIrNodeInterface *iface;

              iface = (GIIrNodeInterface *)CURRENT_NODE(ctx);
              iface->prerequisites = g_list_append (iface->prerequisites, g_strdup (name));
            }
          goto out;
        }
      else if (strcmp (element_name, "package") == 0 &&
          ctx->state == STATE_REPOSITORY)
        {
          state_switch (ctx, STATE_PACKAGE);
          goto out;
        }
      break;

    case 'r':
      if (strcmp (element_name, "repository") == 0 && ctx->state == STATE_START)
        {
          const char *version;

          version = find_attribute ("version", attribute_names, attribute_values);

          if (version == NULL)
            MISSING_ATTRIBUTE (context, error, element_name, "version");
          else if (strcmp (version, SUPPORTED_GIR_VERSION) != 0)
            g_set_error (error,
                         G_MARKUP_ERROR,
                         G_MARKUP_ERROR_INVALID_CONTENT,
                         "Unsupported version '%s'",
                         version);
          else
            state_switch (ctx, STATE_REPOSITORY);

          goto out;
        }
      else if (start_return_value (context, element_name,
                                   attribute_names, attribute_values,
                                   ctx, error))
        goto out;
      else if (start_struct (context, element_name,
                             attribute_names, attribute_values,
                             ctx, error))
        goto out;
      break;

    case 's':
      if (strcmp ("source-position", element_name) == 0)
      {
          state_switch (ctx, STATE_PASSTHROUGH);
          goto out;
      }
      break;
    case 'u':
      if (start_union (context, element_name,
                       attribute_names, attribute_values,
                       ctx, error))
        goto out;
      break;

    case 't':
      if (start_type (context, element_name,
                      attribute_names, attribute_values,
                      ctx, error))
        goto out;
      break;

    case 'v':
      if (start_vfunc (context, element_name,
                       attribute_names, attribute_values,
                       ctx, error))
        goto out;
      if (start_type (context, element_name,
                      attribute_names, attribute_values,
                      ctx, error))
        goto out;
      break;
    default:
      break;
    }

  if (*error == NULL && ctx->state != STATE_PASSTHROUGH)
    {
      int line_number, char_number;
      g_markup_parse_context_get_position (context, &line_number, &char_number);
      if (!g_str_has_prefix (element_name, "c:"))
        g_printerr ("%s:%d:%d: warning: element %s from state %d is unknown, ignoring\n",
                    ctx->file_path, line_number, char_number, element_name,
                    ctx->state);
      state_switch (ctx, STATE_PASSTHROUGH);
    }

 out:
  if (*error)
    {
      int line_number, char_number;
      g_markup_parse_context_get_position (context, &line_number, &char_number);

      g_printerr ("%s:%d:%d: error: %s\n", ctx->file_path, line_number, char_number, (*error)->message);
    }
}

static gboolean
require_one_of_end_elements (GMarkupParseContext  *context,
                             ParseContext         *ctx,
                             const char           *actual_name,
                             GError              **error,
                             ...)
{
  va_list args;
  int line_number, char_number;
  const char *expected;
  gboolean matched = FALSE;

  va_start (args, error);

  while ((expected = va_arg (args, const char*)) != NULL)
    {
      if (strcmp (expected, actual_name) == 0)
        {
          matched = TRUE;
          break;
        }
    }

  va_end (args);

  if (matched)
    return TRUE;

  g_markup_parse_context_get_position (context, &line_number, &char_number);
  g_set_error (error,
               G_MARKUP_ERROR,
               G_MARKUP_ERROR_INVALID_CONTENT,
               "Unexpected end tag '%s' on line %d char %d; current state=%d (prev=%d)",
               actual_name,
               line_number, char_number, ctx->state, ctx->prev_state);
  return FALSE;
}

static gboolean
state_switch_end_struct_or_union (GMarkupParseContext  *context,
                                  ParseContext         *ctx,
                                  const char           *element_name,
                                  GError              **error)
{
  GIIrNode *node = pop_node (ctx);

  if (ctx->node_stack == NULL)
    {
      state_switch (ctx, STATE_NAMESPACE);
    }
  else
    {
      /* In this case the node was not tracked by any other node, so we need
       * to free the node, or we'd leak.
       */
      g_clear_pointer (&node, gi_ir_node_free);

      if (CURRENT_NODE (ctx)->type == GI_IR_NODE_STRUCT)
        state_switch (ctx, STATE_STRUCT);
      else if (CURRENT_NODE (ctx)->type == GI_IR_NODE_UNION)
        state_switch (ctx, STATE_UNION);
      else if (CURRENT_NODE (ctx)->type == GI_IR_NODE_OBJECT)
        state_switch (ctx, STATE_CLASS);
      else
        {
          int line_number, char_number;
          g_markup_parse_context_get_position (context, &line_number, &char_number);
          g_set_error (error,
                       G_MARKUP_ERROR,
                       G_MARKUP_ERROR_INVALID_CONTENT,
                       "Unexpected end tag '%s' on line %d char %d",
                       element_name,
                       line_number, char_number);
          return FALSE;
        }
    }
  return TRUE;
}

static gboolean
require_end_element (GMarkupParseContext  *context,
                     ParseContext         *ctx,
                     const char           *expected_name,
                     const char           *actual_name,
                     GError              **error)
{
  return require_one_of_end_elements (context, ctx, actual_name, error, expected_name, NULL);
}

static void
end_element_handler (GMarkupParseContext  *context,
                     const char           *element_name,
                     gpointer              user_data,
                     GError              **error)
{
  ParseContext *ctx = user_data;

  g_debug ("</%s>", element_name);

  switch (ctx->state)
    {
    case STATE_START:
    case STATE_END:
      /* no need to GError here, GMarkup already catches this */
      break;

    case STATE_REPOSITORY:
      state_switch (ctx, STATE_END);
      break;

    case STATE_INCLUDE:
      if (require_end_element (context, ctx, "include", element_name, error))
        {
          state_switch (ctx, STATE_REPOSITORY);
        }
      break;

    case STATE_C_INCLUDE:
      if (require_end_element (context, ctx, "c:include", element_name, error))
        {
          state_switch (ctx, STATE_REPOSITORY);
        }
      break;

    case STATE_PACKAGE:
      if (require_end_element (context, ctx, "package", element_name, error))
        {
          state_switch (ctx, STATE_REPOSITORY);
        }
      break;

    case STATE_NAMESPACE:
      if (require_end_element (context, ctx, "namespace", element_name, error))
        {
          ctx->current_module = NULL;
          state_switch (ctx, STATE_REPOSITORY);
        }
      break;

    case STATE_ALIAS:
      if (require_end_element (context, ctx, "alias", element_name, error))
        {
          g_free (ctx->current_alias);
          ctx->current_alias = NULL;
          state_switch (ctx, STATE_NAMESPACE);
        }
      break;

    case STATE_FUNCTION_RETURN:
      if (strcmp ("type", element_name) == 0)
        break;
      if (require_end_element (context, ctx, "return-value", element_name, error))
        {
          state_switch (ctx, STATE_FUNCTION);
        }
      break;

    case STATE_FUNCTION_PARAMETERS:
      if (require_end_element (context, ctx, "parameters", element_name, error))
        {
          state_switch (ctx, STATE_FUNCTION);
        }
      break;

    case STATE_FUNCTION_PARAMETER:
      if (strcmp ("type", element_name) == 0)
        break;
      if (require_end_element (context, ctx, "parameter", element_name, error))
        {
          state_switch (ctx, STATE_FUNCTION_PARAMETERS);
        }
      break;

    case STATE_FUNCTION:
      {
        pop_node (ctx);
        if (ctx->node_stack == NULL)
          {
            state_switch (ctx, STATE_NAMESPACE);
          }
        else
          {
            g_debug("case STATE_FUNCTION %d", CURRENT_NODE (ctx)->type);
            if (ctx->in_embedded_state != STATE_NONE)
              {
                state_switch (ctx, ctx->in_embedded_state);
                ctx->in_embedded_state = STATE_NONE;
              }
            else if (CURRENT_NODE (ctx)->type == GI_IR_NODE_INTERFACE)
              state_switch (ctx, STATE_INTERFACE);
            else if (CURRENT_NODE (ctx)->type == GI_IR_NODE_OBJECT)
              state_switch (ctx, STATE_CLASS);
            else if (CURRENT_NODE (ctx)->type == GI_IR_NODE_BOXED)
              state_switch (ctx, STATE_BOXED);
            else if (CURRENT_NODE (ctx)->type == GI_IR_NODE_STRUCT)
              state_switch (ctx, STATE_STRUCT);
            else if (CURRENT_NODE (ctx)->type == GI_IR_NODE_UNION)
              state_switch (ctx, STATE_UNION);
            else if (CURRENT_NODE (ctx)->type == GI_IR_NODE_ENUM ||
                     CURRENT_NODE (ctx)->type == GI_IR_NODE_FLAGS)
              state_switch (ctx, STATE_ENUM);
            else
              {
                int line_number, char_number;
                g_markup_parse_context_get_position (context, &line_number, &char_number);
                g_set_error (error,
                             G_MARKUP_ERROR,
                             G_MARKUP_ERROR_INVALID_CONTENT,
                             "Unexpected end tag '%s' on line %d char %d",
                             element_name,
                             line_number, char_number);
              }
          }
      }
      break;

    case STATE_CLASS_FIELD:
      if (strcmp ("type", element_name) == 0)
        break;
      if (require_end_element (context, ctx, "field", element_name, error))
        {
          state_switch (ctx, STATE_CLASS);
        }
      break;

    case STATE_CLASS_PROPERTY:
      if (strcmp ("type", element_name) == 0)
        break;
      if (require_end_element (context, ctx, "property", element_name, error))
        {
          state_switch (ctx, STATE_CLASS);
        }
      break;

    case STATE_CLASS:
      if (require_end_element (context, ctx, "class", element_name, error))
        {
          pop_node (ctx);
          state_switch (ctx, STATE_NAMESPACE);
        }
      break;

    case STATE_INTERFACE_PROPERTY:
      if (strcmp ("type", element_name) == 0)
        break;
      if (require_end_element (context, ctx, "property", element_name, error))
        {
          state_switch (ctx, STATE_INTERFACE);
        }
      break;

    case STATE_INTERFACE_FIELD:
      if (strcmp ("type", element_name) == 0)
        break;
      if (require_end_element (context, ctx, "field", element_name, error))
        {
          state_switch (ctx, STATE_INTERFACE);
        }
      break;

    case STATE_INTERFACE:
      if (require_end_element (context, ctx, "interface", element_name, error))
        {
          pop_node (ctx);
          state_switch (ctx, STATE_NAMESPACE);
        }
      break;

    case STATE_ENUM:
      if (strcmp ("member", element_name) == 0)
        break;
      else if (strcmp ("function", element_name) == 0)
        break;
      else if (require_one_of_end_elements (context, ctx,
                                            element_name, error, "enumeration",
                                            "bitfield", NULL))
        {
          pop_node (ctx);
          state_switch (ctx, STATE_NAMESPACE);
        }
      break;

    case STATE_BOXED:
      if (require_end_element (context, ctx, "glib:boxed", element_name, error))
        {
          pop_node (ctx);
          state_switch (ctx, STATE_NAMESPACE);
        }
      break;

    case STATE_BOXED_FIELD:
      if (strcmp ("type", element_name) == 0)
        break;
      if (require_end_element (context, ctx, "field", element_name, error))
        {
          state_switch (ctx, STATE_BOXED);
        }
      break;

    case STATE_STRUCT_FIELD:
      if (strcmp ("type", element_name) == 0)
        break;
      if (require_end_element (context, ctx, "field", element_name, error))
        {
          state_switch (ctx, STATE_STRUCT);
        }
      break;

    case STATE_STRUCT:
      if (require_end_element (context, ctx, "record", element_name, error))
        {
          state_switch_end_struct_or_union (context, ctx, element_name, error);
        }
      break;

    case STATE_UNION_FIELD:
      if (strcmp ("type", element_name) == 0)
        break;
      if (require_end_element (context, ctx, "field", element_name, error))
        {
          state_switch (ctx, STATE_UNION);
        }
      break;

    case STATE_UNION:
      if (require_end_element (context, ctx, "union", element_name, error))
        {
          state_switch_end_struct_or_union (context, ctx, element_name, error);
        }
      break;
    case STATE_IMPLEMENTS:
      if (strcmp ("interface", element_name) == 0)
        break;
      if (require_end_element (context, ctx, "implements", element_name, error))
        state_switch (ctx, STATE_CLASS);
      break;
    case STATE_PREREQUISITE:
      if (require_end_element (context, ctx, "prerequisite", element_name, error))
        state_switch (ctx, STATE_INTERFACE);
      break;
    case STATE_NAMESPACE_CONSTANT:
    case STATE_CLASS_CONSTANT:
    case STATE_INTERFACE_CONSTANT:
      if (strcmp ("type", element_name) == 0)
        break;
      if (require_end_element (context, ctx, "constant", element_name, error))
        {
          switch (ctx->state)
            {
            case STATE_NAMESPACE_CONSTANT:
                    pop_node (ctx);
              state_switch (ctx, STATE_NAMESPACE);
              break;
            case STATE_CLASS_CONSTANT:
              state_switch (ctx, STATE_CLASS);
              break;
            case STATE_INTERFACE_CONSTANT:
              state_switch (ctx, STATE_INTERFACE);
              break;
            default:
              g_assert_not_reached ();
              break;
            }
        }
      break;
    case STATE_TYPE:
      if ((strcmp ("type", element_name) == 0) || (strcmp ("array", element_name) == 0) ||
          (strcmp ("varargs", element_name) == 0))
        {
          end_type (ctx);
        }
      break;
    case STATE_ATTRIBUTE:
      if (strcmp ("attribute", element_name) == 0)
        {
          state_switch (ctx, ctx->prev_state);
        }
      break;
    case STATE_DOC_FORMAT:
      if (require_end_element (context, ctx, "doc:format", element_name, error))
        state_switch (ctx, STATE_REPOSITORY);
      break;

    case STATE_PASSTHROUGH:
      ctx->unknown_depth -= 1;
      g_assert (ctx->unknown_depth >= 0);
      if (ctx->unknown_depth == 0)
        state_switch (ctx, ctx->prev_state);
      break;
    default:
      g_error ("Unhandled state %d in end_element_handler", ctx->state);
    }
}

static void
text_handler (GMarkupParseContext  *context,
              const char           *text,
              gsize                 text_len,
              gpointer              user_data,
              GError              **error)
{
  /* FIXME warn about non-whitespace text */
}

static void
cleanup (GMarkupParseContext *context,
         GError              *error,
         void                *user_data)
{
  ParseContext *ctx = user_data;
  GList *m;

  g_clear_slist (&ctx->node_stack, NULL);

  for (m = ctx->modules; m; m = m->next)
    gi_ir_module_free (m->data);
  g_list_free (ctx->modules);
  ctx->modules = NULL;

  ctx->current_module = NULL;
}

/**
 * gi_ir_parser_parse_string:
 * @parser: a #GIIrParser
 * @namespace: the namespace of the string
 * @filename: (nullable) (type filename): Path to parsed file, or `NULL`
 * @buffer: (array length=length): the data containing the XML
 * @length: length of the data, in bytes, or `-1` if nul terminated
 * @error: return location for a [type@GLib.Error], or `NULL`
 *
 * Parse a string that holds a complete GIR XML file, and return a list of a
 * a [class@GIRepository.IrModule] for each `<namespace/>` element within the
 * file.
 *
 * Returns: (transfer none): a new [class@GIRepository.IrModule]
 * Since: 2.80
 */
GIIrModule *
gi_ir_parser_parse_string (GIIrParser   *parser,
                           const char   *namespace,
                           const char   *filename,
                           const char   *buffer,
                           gssize        length,
                           GError      **error)
{
  ParseContext ctx = { 0 };
  GMarkupParseContext *context;
  GIIrModule *module = NULL;

  ctx.parser = parser;
  ctx.state = STATE_START;
  ctx.file_path = filename;
  ctx.namespace = namespace;
  ctx.include_modules = NULL;
  ctx.aliases = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, g_free);
  ctx.disguised_structures = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  ctx.pointer_structures = g_hash_table_new_full (g_str_hash, g_str_equal, g_free, NULL);
  ctx.type_depth = 0;
  ctx.dependencies = g_ptr_array_new_with_free_func (g_free);
  ctx.current_module = NULL;

  context = g_markup_parse_context_new (&firstpass_parser, 0, &ctx, NULL);

  if (!g_markup_parse_context_parse (context, buffer, length, error))
    goto out;

  if (!g_markup_parse_context_end_parse (context, error))
    goto out;

  g_markup_parse_context_free (context);

  ctx.state = STATE_START;
  context = g_markup_parse_context_new (&markup_parser, 0, &ctx, NULL);
  if (!g_markup_parse_context_parse (context, buffer, length, error))
    goto out;

  if (!g_markup_parse_context_end_parse (context, error))
    goto out;

  if (ctx.modules)
    module = ctx.modules->data;

  parser->parsed_modules = g_list_concat (g_steal_pointer (&ctx.modules),
                                          parser->parsed_modules);

 out:

  if (module == NULL)
    {
      /* An error occurred before we created a module, so we haven't
       * transferred ownership of these hash tables to the module.
       */
      g_clear_pointer (&ctx.aliases, g_hash_table_unref);
      g_clear_pointer (&ctx.disguised_structures, g_hash_table_unref);
      g_clear_pointer (&ctx.pointer_structures, g_hash_table_unref);
      g_clear_list (&ctx.modules, (GDestroyNotify) gi_ir_module_free);
      g_list_free (ctx.include_modules);
    }

  g_clear_slist (&ctx.node_stack, NULL);
  g_clear_pointer (&ctx.dependencies, g_ptr_array_unref);
  g_markup_parse_context_free (context);

  if (module)
    return module;

  if (error && *error == NULL)
    g_set_error (error,
                 G_MARKUP_ERROR,
                 G_MARKUP_ERROR_INVALID_CONTENT,
                 "Expected namespace element in the gir file");
  return NULL;
}

/**
 * gi_ir_parser_parse_file:
 * @parser: a #GIIrParser
 * @filename: (type filename): filename to parse
 * @error: return location for a [type@GLib.Error], or `NULL`
 *
 * Parse the given GIR XML file, and return a list of a
 * [class@GIRepository.IrModule] for each `<namespace/>` element within the
 * file.
 *
 * Returns: (transfer container): a newly allocated list of
 *   [class@GIRepository.IrModule]s. The modules themselves
 *   are owned by the `GIIrParser` and will be freed along with the parser.
 * Since: 2.80
 */
GIIrModule *
gi_ir_parser_parse_file (GIIrParser   *parser,
                         const char   *filename,
                         GError      **error)
{
  char *buffer;
  gsize length;
  GIIrModule *module;
  char *dash;
  char *namespace;

  if (!g_str_has_suffix (filename, ".gir"))
    {
      g_set_error (error,
                   G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   "Expected filename to end with '.gir'");
      return NULL;
    }

  g_debug ("[parsing] filename %s", filename);

  namespace = g_path_get_basename (filename);
  namespace[strlen(namespace)-4] = '\0';

  /* Remove version */
  dash = strstr (namespace, "-");
  if (dash != NULL)
    *dash = '\0';

  if (!g_file_get_contents (filename, &buffer, &length, error))
    {
      g_free (namespace);

      return NULL;
    }

  if (length > G_MAXSSIZE)
    {
      g_free (namespace);
      g_free (buffer);

      g_set_error (error,
                   G_MARKUP_ERROR,
                   G_MARKUP_ERROR_INVALID_CONTENT,
                   "Input file too big");
      return NULL;
    }

  module = gi_ir_parser_parse_string (parser, namespace, filename, buffer, (gssize) length, error);

  g_free (namespace);

  g_free (buffer);

  return module;
}


