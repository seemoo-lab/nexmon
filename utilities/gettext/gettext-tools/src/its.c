/* Internationalization Tag Set (ITS) handling
   Copyright (C) 2015-2026 Free Software Foundation, Inc.

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

/* Written by Daiki Ueno and Bruno Haible.  */

#include <config.h>

/* Specification.  */
#include "its.h"

#include <assert.h>
#include <errno.h>
#include <setjmp.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>

#include <libxml/xmlversion.h>
#include <libxml/xmlerror.h>
#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xmlwriter.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#define SB_NO_APPENDF
#include <error.h>
#include "mem-hash-map.h"
#include "trim.h"
#include "xalloc.h"
#include "xvasprintf.h"
#include "string-buffer.h"
#include "xstring-desc.h"
#include "c-ctype.h"
#include "unistr.h"
#include "bcp47.h"
#include "gettext.h"

#define _(str) gettext (str)

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))


/* The Internationalization Tag Set (ITS) 2.0 standard is available at:
   https://www.w3.org/TR/its20/

   This implementation supports only a few data categories, useful for
   gettext-based projects.  Other data categories can be added by
   extending the its_rule_class_ty class and registering it in
   init_classes().

   The message extraction is performed in three steps.  In the first
   step, its_rule_list_apply() assigns values to nodes in an XML
   document.  In the second step, its_rule_list_extract_nodes() marks
   translatable nodes.  In the final step,
   its_rule_list_extract_text() extracts text contents from the marked
   nodes.

   The values assigned to a node are represented as an array of
   key-value pairs, where both keys and values are string.  The array
   is stored in node->_private.  To retrieve the values for a node,
   use its_rule_list_eval().  */

#define ITS_NS "http://www.w3.org/2005/11/its"
#define XML_NS "http://www.w3.org/XML/1998/namespace"
#define GT_NS "https://www.gnu.org/s/gettext/ns/its/extensions/1.0"


/* =================== Common API for xgettext and msgfmt =================== */

/* ----------------------------- Error handling ----------------------------- */

/* The non-local exit to invoke when a non-fatal libxml error occurs.  */
static jmp_buf xml_error_exit;

static void
/* Adapt to API change in libxml 2.12.0.
   See <https://gitlab.gnome.org/GNOME/libxml2/-/issues/622>.  */
#if LIBXML_VERSION >= 21200
structured_error (void *data, const xmlError *err)
#else
structured_error (void *data, xmlError *err)
#endif
{
  error (err->level == XML_ERR_FATAL ? EXIT_FAILURE : 0, 0,
         _("%s error: %s"), "libxml2", err->message);
  longjmp (xml_error_exit, 1);
}

/* Generic errors are marked "deprecated" in the documentation, but functions
   like xmlDocFormatDump actually do produce them.  */
static void generic_error (void *data, const char *message, ...)
     LIBXML_ATTR_FORMAT (2, 3);
static void
generic_error (void *data, const char *message, ...)
{
  va_list args;
  va_start (args, message);
  vfprintf (stderr, message, args);
  va_end (args);
  longjmp (xml_error_exit, 1);
}

/* --------------------------------- Values --------------------------------- */

struct its_value_ty
{
  char *name;
  char *value;
};

struct its_value_list_ty
{
  struct its_value_ty *items;
  size_t nitems;
  size_t nitems_max;
};

static void
its_value_list_append (struct its_value_list_ty *values,
                       const char *name,
                       const char *value)
{
  struct its_value_ty _value;
  _value.name = xstrdup (name);
  _value.value = xstrdup (value);

  if (values->nitems == values->nitems_max)
    {
      values->nitems_max = 2 * values->nitems_max + 1;
      values->items =
        xrealloc (values->items,
                  sizeof (struct its_value_ty) * values->nitems_max);
    }
  memcpy (&values->items[values->nitems++], &_value,
          sizeof (struct its_value_ty));
}

static const char *
its_value_list_get_value (struct its_value_list_ty *values,
                          const char *name)
{
  for (size_t i = 0; i < values->nitems; i++)
    {
      struct its_value_ty *value = &values->items[i];
      if (strcmp (value->name, name) == 0)
        return value->value;
    }
  return NULL;
}

static void
its_value_list_set_value (struct its_value_list_ty *values,
                          const char *name,
                          const char *value)
{
  size_t i;
  for (i = 0; i < values->nitems; i++)
    {
      struct its_value_ty *_value = &values->items[i];
      if (strcmp (_value->name, name) == 0)
        {
          free (_value->value);
          _value->value = xstrdup (value);
          break;
        }
    }

  if (i == values->nitems)
    its_value_list_append (values, name, value);
}

static void
its_value_list_merge (struct its_value_list_ty *values,
                      struct its_value_list_ty *other)
{
  for (size_t i = 0; i < other->nitems; i++)
    {
      struct its_value_ty *other_value = &other->items[i];

      size_t j;
      for (j = 0; j < values->nitems; j++)
        {
          struct its_value_ty *value = &values->items[j];

          if (strcmp (value->name, other_value->name) == 0
              && strcmp (value->value, other_value->value) != 0)
            {
              free (value->value);
              value->value = xstrdup (other_value->value);
              break;
            }
        }

      if (j == values->nitems)
        its_value_list_append (values, other_value->name, other_value->value);
    }
}

static void
its_value_list_destroy (struct its_value_list_ty *values)
{
  for (size_t i = 0; i < values->nitems; i++)
    {
      free (values->items[i].name);
      free (values->items[i].value);
    }
  free (values->items);
}

struct its_pool_ty
{
  struct its_value_list_ty *items;
  size_t nitems;
  size_t nitems_max;
};

static struct its_value_list_ty *
its_pool_alloc_value_list (struct its_pool_ty *pool)
{
  if (pool->nitems == pool->nitems_max)
    {
      pool->nitems_max = 2 * pool->nitems_max + 1;
      pool->items =
        xrealloc (pool->items,
                  sizeof (struct its_value_list_ty) * pool->nitems_max);
    }

  struct its_value_list_ty *values = &pool->items[pool->nitems++];
  memset (values, 0, sizeof (struct its_value_list_ty));
  return values;
}

static const char *
its_pool_get_value_for_node (struct its_pool_ty *pool, xmlNode *node,
                              const char *name)
{
  intptr_t index = (intptr_t) node->_private;
  if (index > 0)
    {
      assert (index <= pool->nitems);
      struct its_value_list_ty *values = &pool->items[index - 1];

      return its_value_list_get_value (values, name);
    }
  return NULL;
}

static void
its_pool_destroy (struct its_pool_ty *pool)
{
  for (size_t i = 0; i < pool->nitems; i++)
    its_value_list_destroy (&pool->items[i]);
  free (pool->items);
}


/* ----------------------------- Lists of nodes ----------------------------- */

struct its_node_list_ty
{
  xmlNode **items;
  size_t nitems;
  size_t nitems_max;
};

static void
its_node_list_append (struct its_node_list_ty *nodes,
                      xmlNode *node)
{
  if (nodes->nitems == nodes->nitems_max)
    {
      nodes->nitems_max = 2 * nodes->nitems_max + 1;
      nodes->items =
        xrealloc (nodes->items, sizeof (xmlNode *) * nodes->nitems_max);
    }
  nodes->items[nodes->nitems++] = node;
}


/* ---------------------------- Rule base class ---------------------------- */

struct its_rule_ty;

/* Base class representing an ITS rule in global definition.  */
struct its_rule_class_ty
{
  /* How many bytes to malloc for an instance of this class.  */
  size_t size;

  /* What to do immediately after the instance is malloc()ed.  */
  void (*constructor) (struct its_rule_ty *rule, xmlNode *node);

  /* What to do immediately before the instance is free()ed.  */
  void (*destructor) (struct its_rule_ty *rule);

  /* How to apply the rule to all elements in DOC.  */
  void (* apply) (struct its_rule_ty *rule, struct its_pool_ty *pool,
                  xmlDoc *doc);

  /* How to evaluate the value of NODE according to the rule.  */
  struct its_value_list_ty *(* eval) (struct its_rule_ty *rule,
                                      struct its_pool_ty *pool, xmlNode *node);
};

#define ITS_RULE_TY                             \
  struct its_rule_class_ty *methods;            \
  char *selector;                               \
  struct its_value_list_ty values;              \
  xmlNs **namespaces;

struct its_rule_ty
{
  ITS_RULE_TY
};

static void
its_rule_destructor (struct its_rule_ty *rule)
{
  free (rule->selector);
  its_value_list_destroy (&rule->values);
  if (rule->namespaces)
    {
      for (size_t i = 0; rule->namespaces[i] != NULL; i++)
        xmlFreeNs (rule->namespaces[i]);
      free (rule->namespaces);
    }
}

static void
its_rule_apply (struct its_rule_ty *rule, struct its_pool_ty *pool, xmlDoc *doc)
{
  if (!rule->selector)
    {
      error (0, 0, _("selector is not specified"));
      return;
    }

  xmlXPathContext *context = xmlXPathNewContext (doc);
  if (!context)
    {
      error (0, 0, _("cannot create XPath context"));
      return;
    }

  if (rule->namespaces)
    {
      for (size_t i = 0; rule->namespaces[i] != NULL; i++)
        {
          xmlNs *ns = rule->namespaces[i];
          xmlXPathRegisterNs (context, ns->prefix, ns->href);
        }
    }

  xmlXPathObject *object = xmlXPathEval (BAD_CAST rule->selector, context);
  if (!object)
    {
      xmlXPathFreeContext (context);
      error (0, 0, _("cannot evaluate XPath expression: %s"), rule->selector);
      return;
    }

  if (object->nodesetval)
    {
      xmlNodeSet *nodes = object->nodesetval;

      for (size_t i = 0; i < nodes->nodeNr; i++)
        {
          xmlNode *node = nodes->nodeTab[i];

          /* We can't store VALUES in NODE, since the address can
             change when realloc()ed.  */
          intptr_t index = (intptr_t) node->_private;
          assert (index <= pool->nitems);

          struct its_value_list_ty *values;
          if (index > 0)
            values = &pool->items[index - 1];
          else
            {
              values = its_pool_alloc_value_list (pool);
              node->_private = (void *) pool->nitems;
            }

          its_value_list_merge (values, &rule->values);
        }
    }

  xmlXPathFreeObject (object);
  xmlXPathFreeContext (context);
}

static char *
_its_get_attribute (xmlNode *node, const char *attr, const char *namespace)
{
  xmlChar *value = xmlGetNsProp (node, BAD_CAST attr, BAD_CAST namespace);

  char *result = xstrdup ((const char *) value);
  xmlFree (value);

  return result;
}

static char *
normalize_whitespace (const char *text, enum its_whitespace_type_ty whitespace)
{
  switch (whitespace)
    {
    case ITS_WHITESPACE_PRESERVE:
      return xstrdup (text);

    case ITS_WHITESPACE_TRIM:
      return trim (text);

    case ITS_WHITESPACE_NORMALIZE_PARAGRAPH:
      /* Normalize whitespaces within the text, keeping paragraph
         boundaries.  */
      {
        char *result = xstrdup (text);
        /* Go through the string, shrinking it, reading from *p++
           and writing to *out++.  (result <= out <= p.)  */
        char *out = result;
        for (const char *start_of_paragraph = result; *start_of_paragraph != '\0';)
          {
            const char *end_of_paragraph;
            const char *next_paragraph;

            /* Find the next paragraph boundary.  */
            for (const char *p = start_of_paragraph;;)
              {
                const char *nl = strchrnul (p, '\n');
                if (*nl == '\0')
                  {
                    end_of_paragraph = nl;
                    next_paragraph = end_of_paragraph;
                    break;
                  }
                p = nl + 1;
                {
                  const char *past_whitespace = p + strspn (p, " \t\n");
                  if (memchr (p, '\n', past_whitespace - p) != NULL)
                    {
                      end_of_paragraph = nl;
                      next_paragraph = past_whitespace;
                      break;
                    }
                  p = past_whitespace;
                }
              }

            /* Normalize whitespaces in the paragraph.  */
            {
              const char *p;

              /* Remove whitespace at the beginning of the paragraph.  */
              for (p = start_of_paragraph; p < end_of_paragraph; p++)
                if (!(*p == ' ' || *p == '\t' || *p == '\n'))
                  break;

              for (; p < end_of_paragraph;)
                {
                  if (*p == ' ' || *p == '\t' || *p == '\n')
                    {
                      /* Normalize whitespace inside the paragraph, and
                         remove whitespace at the end of the paragraph.  */
                      do
                        p++;
                      while (p < end_of_paragraph
                             && (*p == ' ' || *p == '\t' || *p == '\n'));
                      if (p < end_of_paragraph)
                        *out++ = ' ';
                    }
                  else
                    *out++ = *p++;
                }
            }

            if (*next_paragraph != '\0')
              {
                memcpy (out, "\n\n", 2);
                out += 2;
              }
            start_of_paragraph = next_paragraph;
          }
        *out = '\0';
        return result;
      }
    default:
      /* Normalize whitespaces within the text, but do not eliminate whitespace
         at the beginning nor the end of the text.  */
      {
        char *result = xstrdup (text);

        char *out = result;
        for (const char *p = result; *p != '\0';)
          {
            if (*p == ' ' || *p == '\t' || *p == '\n')
              {
                do
                  p++;
                while (*p == ' ' || *p == '\t' || *p == '\n');
                *out++ = ' ';
              }
            else
              *out++ = *p++;
          }
        *out = '\0';
        return result;
      }
    }
}

static char *
_its_encode_special_chars (const char *content, bool is_attribute)
{
  size_t amount = 0;
  for (const char *str = content; *str != '\0'; str++)
    {
      switch (*str)
        {
        case '&':
          amount += sizeof ("&amp;");
          break;
        case '<':
          amount += sizeof ("&lt;");
          break;
        case '>':
          amount += sizeof ("&gt;");
          break;
        case '"':
          if (is_attribute)
            amount += sizeof ("&quot;");
          else
            amount += 1;
          break;
        default:
          amount += 1;
          break;
        }
    }

  char *result = XNMALLOC (amount + 1, char);
  {
    char *p = result;
    for (const char *str = content; *str != '\0'; str++)
      {
        switch (*str)
          {
          case '&':
            p = stpcpy (p, "&amp;");
            break;
          case '<':
            p = stpcpy (p, "&lt;");
            break;
          case '>':
            p = stpcpy (p, "&gt;");
            break;
          case '"':
            if (is_attribute)
              p = stpcpy (p, "&quot;");
            else
              *p++ = '"';
            break;
          default:
            *p++ = *str;
            break;
          }
      }
    *p = '\0';
  }
  return result;
}

static char *
_its_collect_text_content (xmlNode *node,
                           enum its_whitespace_type_ty whitespace,
                           bool do_escape)
{
  struct string_buffer buffer;
  sb_init (&buffer);

  for (xmlNode *n = node->children; n; n = n->next)
    {
      char *content = NULL;

      switch (n->type)
        {
        case XML_TEXT_NODE:
        case XML_CDATA_SECTION_NODE:
          {
            xmlChar *xcontent = xmlNodeGetContent (n);

            /* We can't expect xmlTextWriterWriteString() encode
               special characters as we write text outside of the
               element.  */
            char *econtent;
            if (do_escape)
              econtent =
                _its_encode_special_chars ((const char *) xcontent,
                                           node->type == XML_ATTRIBUTE_NODE);
            else
              econtent = xstrdup ((const char *) xcontent);
            xmlFree (xcontent);

            /* Skip whitespaces at the beginning of the text, if this
               is the first node.  */
            const char *ccontent = econtent;
            if (whitespace == ITS_WHITESPACE_NORMALIZE && !n->prev)
              ccontent = ccontent + strspn (ccontent, " \t\n");
            content =
              normalize_whitespace (ccontent, whitespace);
            free (econtent);

            /* Skip whitespaces at the end of the text, if this
               is the last node.  */
            if (whitespace == ITS_WHITESPACE_NORMALIZE && !n->next)
              {
                char *p = content + strlen (content);
                for (; p > content; p--)
                  {
                    int c = *(p - 1);
                    if (!(c == ' ' || c == '\t' || c == '\n'))
                      {
                        *p = '\0';
                        break;
                      }
                  }
              }
          }
          break;

        case XML_ELEMENT_NODE:
          {
            xmlOutputBuffer *obuffer = xmlAllocOutputBuffer (NULL);
            xmlTextWriter *writer = xmlNewTextWriter (obuffer);
            char *p = _its_collect_text_content (n, whitespace, do_escape);

            xmlTextWriterStartElement (writer, BAD_CAST n->name);
            if (n->properties)
              {
                xmlAttr *attr = n->properties;
                for (; attr; attr = attr->next)
                  {
                    xmlChar *prop = xmlGetProp (n, attr->name);
                    xmlTextWriterWriteAttribute (writer,
                                                 attr->name,
                                                 prop);
                    xmlFree (prop);
                  }
              }
            if (*p != '\0')
              xmlTextWriterWriteRaw (writer, BAD_CAST p);
            xmlTextWriterEndElement (writer);

            const char *ccontent = (const char *) xmlOutputBufferGetContent (obuffer);
            content = normalize_whitespace (ccontent, whitespace);
            xmlFreeTextWriter (writer);
            free (p);
          }
          break;

        case XML_ENTITY_REF_NODE:
          content = xasprintf ("&%s;", (const char *) n->name);
          break;

        default:
          break;
        }

      if (content != NULL)
        sb_xappend_c (&buffer, content);
      free (content);
    }

  return sb_xdupfree_c (&buffer);
}

static void
_its_error_missing_attribute (xmlNode *node, const char *attribute)
{
  error (0, 0, _("\"%s\" node does not contain \"%s\""),
         node->name, attribute);
}


/* ---------------------------- <translateRule> ---------------------------- */

/* Implementation of Translate data category.  */
static void
its_translate_rule_constructor (struct its_rule_ty *rule, xmlNode *node)
{
  if (!xmlHasProp (node, BAD_CAST "selector"))
    {
      _its_error_missing_attribute (node, "selector");
      return;
    }

  if (!xmlHasProp (node, BAD_CAST "translate"))
    {
      _its_error_missing_attribute (node, "translate");
      return;
    }

  {
    char *prop = _its_get_attribute (node, "selector", NULL);
    if (prop)
      rule->selector = prop;
  }
  {
    char *prop = _its_get_attribute (node, "translate", NULL);
    its_value_list_append (&rule->values, "translate", prop);
    free (prop);
  }
}

static struct its_value_list_ty *
its_translate_rule_eval (struct its_rule_ty *rule, struct its_pool_ty *pool,
                         xmlNode *node)
{
  /* Evaluation rules,
     as specified in <https://www.w3.org/TR/its20/#datacategories-defaults-etc>:
     - Local usage: Yes
     - Global, rule-based selection: Yes
     - Default values: translate="yes" for elements,
                       translate="no" for attributes.
     - Inheritance for element nodes: Textual content of element,
       including content of child elements, but excluding attributes.  */
  struct its_value_list_ty *result = XCALLOC (1, struct its_value_list_ty);

  switch (node->type)
    {
    case XML_ATTRIBUTE_NODE:
      /* Attribute nodes don't inherit from the parent elements.  */
      {
        const char *value =
          its_pool_get_value_for_node (pool, node, "translate");
        if (value != NULL)
          {
            its_value_list_set_value (result, "translate", value);
            return result;
          }

        /* The default value is translate="no".  */
        its_value_list_append (result, "translate", "no");
      }
      break;

    case XML_ELEMENT_NODE:
      /* Inherit from the parent elements.  */
      {
        /* A local attribute overrides the global rule.  */
        if (xmlHasNsProp (node, BAD_CAST "translate", BAD_CAST ITS_NS))
          {
            char *prop = _its_get_attribute (node, "translate", ITS_NS);
            its_value_list_append (result, "translate", prop);
            free (prop);
            return result;
          }

        /* Check value for the current node.  */
        const char *value = its_pool_get_value_for_node (pool, node, "translate");
        if (value != NULL)
          {
            its_value_list_set_value (result, "translate", value);
            return result;
          }

        /* Recursively check value for the parent node.  */
        if (node->parent == NULL
            || node->parent->type != XML_ELEMENT_NODE)
          /* The default value is translate="yes".  */
          its_value_list_append (result, "translate", "yes");
        else
          {
            struct its_value_list_ty *values =
              its_translate_rule_eval (rule, pool, node->parent);
            its_value_list_merge (result, values);
            its_value_list_destroy (values);
            free (values);
          }
      }
      break;

    default:
      break;
    }

  return result;
}

static struct its_rule_class_ty its_translate_rule_class =
  {
    sizeof (struct its_rule_ty),
    its_translate_rule_constructor,
    its_rule_destructor,
    its_rule_apply,
    its_translate_rule_eval,
  };


/* ----------------------------- <locNoteRule> ----------------------------- */

/* Implementation of Localization Note data category.  */
static void
its_localization_note_rule_constructor (struct its_rule_ty *rule, xmlNode *node)
{
  if (!xmlHasProp (node, BAD_CAST "selector"))
    {
      _its_error_missing_attribute (node, "selector");
      return;
    }

  if (!xmlHasProp (node, BAD_CAST "locNoteType"))
    {
      _its_error_missing_attribute (node, "locNoteType");
      return;
    }

  {
    char *prop = _its_get_attribute (node, "selector", NULL);
    if (prop)
      rule->selector = prop;
  }

  xmlNode *n;
  for (n = node->children; n; n = n->next)
    {
      if (n->type == XML_ELEMENT_NODE
          && xmlStrEqual (n->name, BAD_CAST "locNote")
          && xmlStrEqual (n->ns->href, BAD_CAST ITS_NS))
        break;
    }

  {
    char *prop = _its_get_attribute (node, "locNoteType", NULL);
    if (prop)
      its_value_list_append (&rule->values, "locNoteType", prop);
    free (prop);
  }

  if (n)
    {
      /* FIXME: Respect space attribute.  */
      char *content = _its_collect_text_content (n, ITS_WHITESPACE_NORMALIZE,
                                                 false);
      its_value_list_append (&rule->values, "locNote", content);
      free (content);
    }
  else if (xmlHasProp (node, BAD_CAST "locNotePointer"))
    {
      char *prop = _its_get_attribute (node, "locNotePointer", NULL);
      its_value_list_append (&rule->values, "locNotePointer", prop);
      free (prop);
    }
  /* FIXME: locNoteRef and locNoteRefPointer */
}

static struct its_value_list_ty *
its_localization_note_rule_eval (struct its_rule_ty *rule,
                                 struct its_pool_ty *pool,
                                 xmlNode *node)
{
  /* Evaluation rules,
     as specified in <https://www.w3.org/TR/its20/#datacategories-defaults-etc>:
     - Local usage: Yes
     - Global, rule-based selection: Yes
     - Default values: none
     - Inheritance for element nodes: Textual content of element,
       including content of child elements, but excluding attributes.  */
  struct its_value_list_ty *result = XCALLOC (1, struct its_value_list_ty);

  switch (node->type)
    {
    case XML_ATTRIBUTE_NODE:
      /* Attribute nodes don't inherit from the parent elements.  */
      {
        const char *value = its_pool_get_value_for_node (pool, node, "locNoteType");
        if (value != NULL)
          its_value_list_set_value (result, "locNoteType", value);
      }
      {
        const char *value = its_pool_get_value_for_node (pool, node, "locNote");
        if (value != NULL)
          {
            its_value_list_set_value (result, "locNote", value);
            return result;
          }
      }
      {
        const char *value = its_pool_get_value_for_node (pool, node, "locNotePointer");
        if (value != NULL)
          {
            its_value_list_set_value (result, "locNotePointer", value);
            return result;
          }
      }
      break;

    case XML_ELEMENT_NODE:
      /* Inherit from the parent elements.  */
      {
        /* Local attributes overrides the global rule.  */
        if (xmlHasNsProp (node, BAD_CAST "locNote", BAD_CAST ITS_NS)
            || xmlHasNsProp (node, BAD_CAST "locNoteRef", BAD_CAST ITS_NS)
            || xmlHasNsProp (node, BAD_CAST "locNoteType", BAD_CAST ITS_NS))
          {
            if (xmlHasNsProp (node, BAD_CAST "locNote", BAD_CAST ITS_NS))
              {
                char *prop = _its_get_attribute (node, "locNote", ITS_NS);
                its_value_list_append (result, "locNote", prop);
                free (prop);
              }

            /* FIXME: locNoteRef */

            if (xmlHasNsProp (node, BAD_CAST "locNoteType", BAD_CAST ITS_NS))
              {
                char *prop = _its_get_attribute (node, "locNoteType", ITS_NS);
                its_value_list_append (result, "locNoteType", prop);
                free (prop);
              }

            return result;
          }

        /* Check value for the current node.  */
        {
          const char *value = its_pool_get_value_for_node (pool, node, "locNoteType");
          if (value != NULL)
            its_value_list_set_value (result, "locNoteType", value);
        }
        {
          const char *value = its_pool_get_value_for_node (pool, node, "locNote");
          if (value != NULL)
            {
              its_value_list_set_value (result, "locNote", value);
              return result;
            }
        }
        {
          const char *value = its_pool_get_value_for_node (pool, node, "locNotePointer");
          if (value != NULL)
            {
              its_value_list_set_value (result, "locNotePointer", value);
              return result;
            }
        }

        /* Recursively check value for the parent node.  */
        if (node->parent == NULL
            || node->parent->type != XML_ELEMENT_NODE)
          return result;
        else
          {
            struct its_value_list_ty *values =
              its_localization_note_rule_eval (rule, pool, node->parent);
            its_value_list_merge (result, values);
            its_value_list_destroy (values);
            free (values);
          }
      }
      break;

    default:
      break;
    }

  /* The default value is None.  */
  return result;
}

static struct its_rule_class_ty its_localization_note_rule_class =
  {
    sizeof (struct its_rule_ty),
    its_localization_note_rule_constructor,
    its_rule_destructor,
    its_rule_apply,
    its_localization_note_rule_eval,
  };


/* ---------------------------- <withinTextRule> ---------------------------- */

/* Implementation of Element Within Text data category.  */
static void
its_element_within_text_rule_constructor (struct its_rule_ty *rule,
                                          xmlNode *node)
{
  if (!xmlHasProp (node, BAD_CAST "selector"))
    {
      _its_error_missing_attribute (node, "selector");
      return;
    }

  if (!xmlHasProp (node, BAD_CAST "withinText"))
    {
      _its_error_missing_attribute (node, "withinText");
      return;
    }

  {
    char *prop = _its_get_attribute (node, "selector", NULL);
    if (prop)
      rule->selector = prop;
  }
  {
    char *prop = _its_get_attribute (node, "withinText", NULL);
    its_value_list_append (&rule->values, "withinText", prop);
    free (prop);
  }
}

static struct its_value_list_ty *
its_element_within_text_rule_eval (struct its_rule_ty *rule,
                                   struct its_pool_ty *pool,
                                   xmlNode *node)
{
  /* Evaluation rules,
     as specified in <https://www.w3.org/TR/its20/#datacategories-defaults-etc>:
     - Local usage: Yes
     - Global, rule-based selection: Yes
     - Default values: withinText="no"
     - Inheritance for element nodes: none  */
  struct its_value_list_ty *result = XCALLOC (1, struct its_value_list_ty);

  if (node->type != XML_ELEMENT_NODE)
    return result;

  /* A local attribute overrides the global rule.  */
  if (xmlHasNsProp (node, BAD_CAST "withinText", BAD_CAST ITS_NS))
    {
      char *prop = _its_get_attribute (node, "withinText", ITS_NS);
      its_value_list_append (result, "withinText", prop);
      free (prop);
      return result;
    }

  /* Doesn't inherit from the parent elements, and the default value
     is "no".  */
  {
    const char *value = its_pool_get_value_for_node (pool, node, "withinText");
    if (value != NULL)
      its_value_list_set_value (result, "withinText", value);
  }

  return result;
}

static struct its_rule_class_ty its_element_within_text_rule_class =
  {
    sizeof (struct its_rule_ty),
    its_element_within_text_rule_constructor,
    its_rule_destructor,
    its_rule_apply,
    its_element_within_text_rule_eval,
  };


/* -------------------------- <preserveSpaceRule> -------------------------- */

/* Implementation of Preserve Space data category.  */
static void
its_preserve_space_rule_constructor (struct its_rule_ty *rule,
                                     xmlNode *node)
{
  if (!xmlHasProp (node, BAD_CAST "selector"))
    {
      _its_error_missing_attribute (node, "selector");
      return;
    }

  if (!xmlHasProp (node, BAD_CAST "space"))
    {
      _its_error_missing_attribute (node, "space");
      return;
    }

  {
    char *prop = _its_get_attribute (node, "selector", NULL);
    if (prop)
      rule->selector = prop;
  }
  {
    char *prop = _its_get_attribute (node, "space", NULL);
    if (prop
        && !(strcmp (prop, "preserve") ==0
             || strcmp (prop, "default") == 0
             /* gettext extension: remove leading/trailing whitespaces only.  */
             || (node->ns && xmlStrEqual (node->ns->href, BAD_CAST GT_NS)
                 && strcmp (prop, "trim") == 0)
             /* gettext extension: same as default except keeping
                paragraph boundaries.  */
             || (node->ns && xmlStrEqual (node->ns->href, BAD_CAST GT_NS)
                 && strcmp (prop, "paragraph") == 0)))
      {
        error (0, 0, _("invalid attribute value \"%s\" for \"%s\""),
               prop, "space");
        free (prop);
        return;
      }

    its_value_list_append (&rule->values, "space", prop);
    free (prop);
  }
}

static struct its_value_list_ty *
its_preserve_space_rule_eval (struct its_rule_ty *rule,
                              struct its_pool_ty *pool,
                              xmlNode *node)
{
  /* Evaluation rules,
     as specified in <https://www.w3.org/TR/its20/#datacategories-defaults-etc>:
     - Local usage: Yes
     - Global, rule-based selection: Yes
     - Default values: space="default"
     - Inheritance for element nodes: Textual content of element,
       including attributes and child elements.  */
  struct its_value_list_ty *result = XCALLOC (1, struct its_value_list_ty);

  if (node->type != XML_ELEMENT_NODE)
    return result;

  /* A local attribute overrides the global rule.  */
  if (xmlHasNsProp (node, BAD_CAST "space", BAD_CAST XML_NS))
    {
      char *prop;

      prop = _its_get_attribute (node, "space", XML_NS);
      its_value_list_append (result, "space", prop);
      free (prop);
      return result;
    }

  /* Check value for the current node.  */
  {
    const char *value = its_pool_get_value_for_node (pool, node, "space");
    if (value != NULL)
      {
        its_value_list_set_value (result, "space", value);
        return result;
      }
  }

  if (node->parent == NULL
      || node->parent->type != XML_ELEMENT_NODE)
    {
      /* The default value is space="default".  */
      its_value_list_append (result, "space", "default");
      return result;
    }

  /* Recursively check value for the parent node.  */
  struct its_value_list_ty *values =
    its_preserve_space_rule_eval (rule, pool, node->parent);
  its_value_list_merge (result, values);
  its_value_list_destroy (values);
  free (values);

  return result;
}

static struct its_rule_class_ty its_preserve_space_rule_class =
  {
    sizeof (struct its_rule_ty),
    its_preserve_space_rule_constructor,
    its_rule_destructor,
    its_rule_apply,
    its_preserve_space_rule_eval,
  };


/* ----------------------------- <contextRule> ----------------------------- */

/* Implementation of Context data category.  */
static void
its_extension_context_rule_constructor (struct its_rule_ty *rule, xmlNode *node)
{
  if (!xmlHasProp (node, BAD_CAST "selector"))
    {
      _its_error_missing_attribute (node, "selector");
      return;
    }

  if (!xmlHasProp (node, BAD_CAST "contextPointer"))
    {
      _its_error_missing_attribute (node, "contextPointer");
      return;
    }

  {
    char *prop = _its_get_attribute (node, "selector", NULL);
    if (prop)
      rule->selector = prop;
  }
  {
    char *prop = _its_get_attribute (node, "contextPointer", NULL);
    its_value_list_append (&rule->values, "contextPointer", prop);
    free (prop);
  }

  if (xmlHasProp (node, BAD_CAST "textPointer"))
    {
      char *prop = _its_get_attribute (node, "textPointer", NULL);
      its_value_list_append (&rule->values, "textPointer", prop);
      free (prop);
    }
}

static struct its_value_list_ty *
its_extension_context_rule_eval (struct its_rule_ty *rule,
                                 struct its_pool_ty *pool,
                                 xmlNode *node)
{
  /* Evaluation rules:
     - Local usage: No
     - Global, rule-based selection: Yes
     - Default values: none
     - Inheritance for element nodes: none  */
  struct its_value_list_ty *result = XCALLOC (1, struct its_value_list_ty);

  /* Doesn't inherit from the parent elements, and the default value
     is None.  */
  {
    const char *value = its_pool_get_value_for_node (pool, node, "contextPointer");
    if (value != NULL)
      its_value_list_set_value (result, "contextPointer", value);
  }
  {
    const char *value = its_pool_get_value_for_node (pool, node, "textPointer");
    if (value != NULL)
      its_value_list_set_value (result, "textPointer", value);
  }

  return result;
}

static struct its_rule_class_ty its_extension_context_rule_class =
  {
    sizeof (struct its_rule_ty),
    its_extension_context_rule_constructor,
    its_rule_destructor,
    its_rule_apply,
    its_extension_context_rule_eval,
  };


/* ------------------------------ <escapeRule> ------------------------------ */

/* Implementation of Escape Special Characters data category.  */
static void
its_extension_escape_rule_constructor (struct its_rule_ty *rule, xmlNode *node)
{
  if (!xmlHasProp (node, BAD_CAST "selector"))
    {
      _its_error_missing_attribute (node, "selector");
      return;
    }

  if (!xmlHasProp (node, BAD_CAST "escape"))
    {
      _its_error_missing_attribute (node, "escape");
      return;
    }

  {
    char *prop = _its_get_attribute (node, "selector", NULL);
    if (prop)
      rule->selector = prop;
  }
  {
    char *prop = _its_get_attribute (node, "escape", NULL);
    its_value_list_append (&rule->values, "escape", prop);
    free (prop);
  }

  if (xmlHasProp (node, BAD_CAST "unescape-if"))
    {
      char *prop = _its_get_attribute (node, "unescape-if", NULL);
      its_value_list_append (&rule->values, "unescape-if", prop);
      free (prop);
    }
}

static struct its_value_list_ty *
its_extension_escape_rule_eval (struct its_rule_ty *rule,
                                struct its_pool_ty *pool,
                                xmlNode *node)
{
  /* Evaluation rules:
     - Local usage: Yes
     - Global, rule-based selection: Yes
     - Default values: escape="no" unescape-if="no" (handled in the caller)
     - Inheritance for element nodes: Textual content of element,
       including content of child elements, but excluding attributes.  */
  struct its_value_list_ty *result = XCALLOC (1, struct its_value_list_ty);

  switch (node->type)
    {
    case XML_ATTRIBUTE_NODE:
      /* Attribute nodes don't inherit from the parent elements.  */
      {
        const char *value =
          its_pool_get_value_for_node (pool, node, "escape");
        if (value != NULL)
          {
            its_value_list_set_value (result, "escape", value);
            return result;
          }
      }
      break;

    case XML_ELEMENT_NODE:
      /* Inherit from the parent elements.  */
      {
        /* A local attribute overrides the global rule.  */
        if (xmlHasNsProp (node, BAD_CAST "escape", BAD_CAST GT_NS)
            || xmlHasNsProp (node, BAD_CAST "unescape-if", BAD_CAST GT_NS))
          {
            if (xmlHasNsProp (node, BAD_CAST "escape", BAD_CAST GT_NS))
              {
                char *prop = _its_get_attribute (node, "escape", GT_NS);
                if (strcmp (prop, "yes") == 0 || strcmp (prop, "no") == 0)
                  {
                    its_value_list_append (result, "escape", prop);
                    if (strcmp (prop, "no") != 0)
                      {
                        free (prop);
                        return result;
                      }
                  }
                free (prop);
              }

            if (xmlHasNsProp (node, BAD_CAST "unescape-if", BAD_CAST GT_NS))
              {
                char *prop = _its_get_attribute (node, "unescape-if", GT_NS);
                if (strcmp (prop, "xml") == 0
                    || strcmp (prop, "xhtml") == 0
                    || strcmp (prop, "html") == 0
                    || strcmp (prop, "no") == 0)
                  {
                    its_value_list_append (result, "unescape-if", prop);
                    if (strcmp (prop, "no") != 0)
                      {
                        free (prop);
                        return result;
                      }
                  }
                free (prop);
              }
          }

        /* Check value for the current node.  */
        {
          const char *value = its_pool_get_value_for_node (pool, node, "unescape-if");
          if (value != NULL)
            its_value_list_set_value (result, "unescape-if", value);
        }

        {
          const char *value = its_pool_get_value_for_node (pool, node, "escape");
          if (value != NULL)
            {
              its_value_list_set_value (result, "escape", value);
              return result;
            }
        }

        /* Recursively check value for the parent node.  */
        if (node->parent != NULL
            && node->parent->type == XML_ELEMENT_NODE)
          {
            struct its_value_list_ty *values =
              its_extension_escape_rule_eval (rule, pool, node->parent);
            its_value_list_merge (result, values);
            its_value_list_destroy (values);
            free (values);
          }
      }
      break;

    default:
      break;
    }

  return result;
}

static struct its_rule_class_ty its_extension_escape_rule_class =
  {
    sizeof (struct its_rule_ty),
    its_extension_escape_rule_constructor,
    its_rule_destructor,
    its_rule_apply,
    its_extension_escape_rule_eval,
  };


/* ---------------------------- Rules in general ---------------------------- */

static hash_table classes;

static struct its_rule_ty *
its_rule_alloc (struct its_rule_class_ty *method_table, xmlNode *node)
{
  struct its_rule_ty *rule = (struct its_rule_ty *) xcalloc (1, method_table->size);
  rule->methods = method_table;
  if (method_table->constructor)
    method_table->constructor (rule, node);
  return rule;
}

static struct its_rule_ty *
its_rule_parse (xmlDoc *doc, xmlNode *node)
{
  const char *name = (const char *) node->name;

  void *value;
  if (hash_find_entry (&classes, name, strlen (name), &value) == 0)
    {
      struct its_rule_ty *result =
        its_rule_alloc ((struct its_rule_class_ty *) value, node);

      xmlNs **namespaces = xmlGetNsList (doc, node);
      if (namespaces)
        {
          {
            size_t i;
            for (i = 0; namespaces[i] != NULL; i++)
              ;
            result->namespaces = XCALLOC (i + 1, xmlNs *);
          }
          for (size_t i = 0; namespaces[i] != NULL; i++)
            result->namespaces[i] = xmlCopyNamespace (namespaces[i]);
        }
      xmlFree (namespaces);

      return result;
    }

  return NULL;
}

static void
its_rule_destroy (struct its_rule_ty *rule)
{
  if (rule->methods->destructor)
    rule->methods->destructor (rule);
}

static void
init_classes (void)
{
#define ADD_RULE_CLASS(n, c) \
  hash_insert_entry (&classes, n, strlen (n), &c);

  ADD_RULE_CLASS ("translateRule", its_translate_rule_class);
  ADD_RULE_CLASS ("locNoteRule", its_localization_note_rule_class);
  ADD_RULE_CLASS ("withinTextRule", its_element_within_text_rule_class);
  ADD_RULE_CLASS ("preserveSpaceRule", its_preserve_space_rule_class);
  ADD_RULE_CLASS ("contextRule", its_extension_context_rule_class);
  ADD_RULE_CLASS ("escapeRule", its_extension_escape_rule_class);

#undef ADD_RULE_CLASS
}


/* --------------------------- Loading the rules --------------------------- */

struct its_rule_list_ty
{
  struct its_rule_ty **items;
  size_t nitems;
  size_t nitems_max;

  struct its_pool_ty pool;
};

struct its_rule_list_ty *
its_rule_list_alloc (void)
{
  if (classes.table == NULL)
    {
      hash_init (&classes, 10);
      init_classes ();
    }

  struct its_rule_list_ty *result = XCALLOC (1, struct its_rule_list_ty);
  return result;
}

void
its_rule_list_free (struct its_rule_list_ty *rules)
{
  for (size_t i = 0; i < rules->nitems; i++)
    {
      its_rule_destroy (rules->items[i]);
      free (rules->items[i]);
    }
  free (rules->items);
  its_pool_destroy (&rules->pool);
}

static bool
its_rule_list_add_from_doc (struct its_rule_list_ty *rules,
                            xmlDoc *doc)
{
  xmlNode *root = xmlDocGetRootElement (doc);
  if (!(xmlStrEqual (root->name, BAD_CAST "rules")
        && xmlStrEqual (root->ns->href, BAD_CAST ITS_NS)))
    {
      error (0, 0, _("the root element is not \"rules\""
                     " under namespace %s"),
             ITS_NS);
      xmlFreeDoc (doc);
      return false;
    }

  for (xmlNode *node = root->children; node; node = node->next)
    {
      struct its_rule_ty *rule = its_rule_parse (doc, node);
      if (rule != NULL)
        {
          if (rules->nitems == rules->nitems_max)
            {
              rules->nitems_max = 2 * rules->nitems_max + 1;
              rules->items =
                xrealloc (rules->items,
                          sizeof (struct its_rule_ty *) * rules->nitems_max);
            }
          rules->items[rules->nitems++] = rule;
        }
    }

  return true;
}

bool
its_rule_list_add_from_file (struct its_rule_list_ty *rules,
                             const char *filename)
{
  xmlDoc *doc = xmlReadFile (filename, "utf-8",
                             XML_PARSE_NONET
                             | XML_PARSE_NOWARNING
                             | XML_PARSE_NOBLANKS
                             | XML_PARSE_NOERROR);
  if (doc == NULL)
    {
      const xmlError *err = xmlGetLastError ();
      error (err->level == XML_ERR_FATAL ? EXIT_FAILURE : 0, 0,
             _("cannot read %s: %s"), filename, err->message);
      return false;
    }

  bool result;
  if (setjmp (xml_error_exit) == 0)
    {
      xmlSetStructuredErrorFunc (NULL, structured_error);
      xmlSetGenericErrorFunc (NULL, generic_error);

      result = its_rule_list_add_from_doc (rules, doc);
    }
  else
    /* Caught a libxml error.  */
    result = false;

  xmlFreeDoc (doc);

  xmlSetGenericErrorFunc (NULL, NULL);
  xmlSetStructuredErrorFunc (NULL, NULL);
  return result;
}

bool
its_rule_list_add_from_string (struct its_rule_list_ty *rules,
                               const char *rule)
{
  xmlDoc *doc = xmlReadMemory (rule, strlen (rule),
                               "(internal)",
                               NULL,
                               XML_PARSE_NONET
                               | XML_PARSE_NOWARNING
                               | XML_PARSE_NOBLANKS
                               | XML_PARSE_NOERROR);
  if (doc == NULL)
    {
      const xmlError *err = xmlGetLastError ();
      error (err->level == XML_ERR_FATAL ? EXIT_FAILURE : 0, 0,
             _("cannot read %s: %s"), "(internal)", err->message);
      return false;
    }

  bool result;
  if (setjmp (xml_error_exit) == 0)
    {
      xmlSetStructuredErrorFunc (NULL, structured_error);
      xmlSetGenericErrorFunc (NULL, generic_error);

      result = its_rule_list_add_from_doc (rules, doc);
    }
  else
    /* Caught a libxml error.  */
    result = false;

  xmlFreeDoc (doc);

  xmlSetGenericErrorFunc (NULL, NULL);
  xmlSetStructuredErrorFunc (NULL, NULL);
  return result;
}

static void
its_rule_list_apply (struct its_rule_list_ty *rules, xmlDoc *doc)
{
  for (size_t i = 0; i < rules->nitems; i++)
    {
      struct its_rule_ty *rule = rules->items[i];
      rule->methods->apply (rule, &rules->pool, doc);
    }
}

static struct its_value_list_ty *
its_rule_list_eval (its_rule_list_ty *rules, xmlNode *node)
{
  struct its_value_list_ty *result = XCALLOC (1, struct its_value_list_ty);

  for (size_t i = 0; i < rules->nitems; i++)
    {
      struct its_rule_ty *rule = rules->items[i];
      struct its_value_list_ty *values = rule->methods->eval (rule, &rules->pool, node);
      its_value_list_merge (result, values);
      its_value_list_destroy (values);
      free (values);
    }

  return result;
}

static bool
its_rule_list_is_translatable (its_rule_list_ty *rules,
                               xmlNode *node,
                               int depth)
{
  if (node->type != XML_ELEMENT_NODE
      && node->type != XML_ATTRIBUTE_NODE)
    return false;

  struct its_value_list_ty *values = its_rule_list_eval (rules, node);

  /* Check if NODE has translate="yes".  */
  {
    const char *value = its_value_list_get_value (values, "translate");
    if (!(value && strcmp (value, "yes") == 0))
      {
        its_value_list_destroy (values);
        free (values);
        return false;
      }
  }

  /* Check if NODE has withinText="yes", if NODE is not top-level.  */
  if (depth > 0)
    {
      const char *value = its_value_list_get_value (values, "withinText");
      if (!(value && strcmp (value, "yes") == 0))
        {
          its_value_list_destroy (values);
          free (values);
          return false;
        }
    }

  its_value_list_destroy (values);
  free (values);

  for (xmlNode *n = node->children; n; n = n->next)
    {
      switch (n->type)
        {
        case XML_ELEMENT_NODE:
          if (!its_rule_list_is_translatable (rules, n, depth + 1))
            return false;
          break;

        case XML_TEXT_NODE:
        case XML_CDATA_SECTION_NODE:
        case XML_ENTITY_REF_NODE:
        case XML_COMMENT_NODE:
          break;

        default:
          return false;
        }
    }

  return true;
}

static void
its_rule_list_extract_nodes (its_rule_list_ty *rules,
                             struct its_node_list_ty *nodes,
                             xmlNode *node)
{
  if (node->type == XML_ELEMENT_NODE)
    {
      if (node->properties)
        {
          for (xmlAttr *attr = node->properties; attr; attr = attr->next)
            {
              xmlNode *n = (xmlNode *) attr;
              if (its_rule_list_is_translatable (rules, n, 0))
                its_node_list_append (nodes, n);
            }
        }

      if (its_rule_list_is_translatable (rules, node, 0))
        its_node_list_append (nodes, node);
      else
        {
          for (xmlNode *n = node->children; n; n = n->next)
            its_rule_list_extract_nodes (rules, nodes, n);
        }
    }
}

static char *
_its_get_content (struct its_rule_list_ty *rules, xmlNode *node,
                  const char *pointer,
                  enum its_whitespace_type_ty whitespace,
                  bool do_escape)
{
  xmlXPathContext *context = xmlXPathNewContext (node->doc);
  if (!context)
    {
      error (0, 0, _("cannot create XPath context"));
      return NULL;
    }

  for (size_t i = 0; i < rules->nitems; i++)
    {
      struct its_rule_ty *rule = rules->items[i];
      if (rule->namespaces)
        {
          for (size_t j = 0; rule->namespaces[j] != NULL; j++)
            {
              xmlNs *ns = rule->namespaces[j];
              xmlXPathRegisterNs (context, ns->prefix, ns->href);
            }
        }
    }

  xmlXPathSetContextNode (node, context);
  xmlXPathObject *object = xmlXPathEvalExpression (BAD_CAST pointer, context);
  if (!object)
    {
      xmlXPathFreeContext (context);
      error (0, 0, _("cannot evaluate XPath location path: %s"),
             pointer);
      return NULL;
    }

  char *result = NULL;
  switch (object->type)
    {
    case XPATH_NODESET:
      {
        xmlNodeSet *nodes = object->nodesetval;
        string_list_ty sl;
        string_list_init (&sl);
        for (size_t i = 0; i < nodes->nodeNr; i++)
          {
            char *content = _its_collect_text_content (nodes->nodeTab[i],
                                                       whitespace,
                                                       do_escape);
            string_list_append (&sl, content);
            free (content);
          }
        result = string_list_concat (&sl);
        string_list_destroy (&sl);
      }
      break;

    case XPATH_STRING:
      result = xstrdup ((const char *) object->stringval);
      break;

    default:
      break;
    }

  xmlXPathFreeObject (object);
  xmlXPathFreeContext (context);

  return result;
}


/* ========================= API only for xgettext ========================= */

static void
_its_comment_append (string_list_ty *comments, const char *data)
{
  /* Split multiline comment into lines, and remove leading and trailing
     whitespace.  */
  char *copy = xstrdup (data);
  char *p;
  char *q;

  for (p = copy; (q = strchr (p, '\n')) != NULL; p = q + 1)
    {
      while (p[0] == ' ' || p[0] == '\t')
        p++;
      while (q > p && (q[-1] == ' ' || q[-1] == '\t'))
        q--;
      *q = '\0';
      string_list_append (comments, p);
    }
  q = p + strlen (p);
  while (p[0] == ' ' || p[0] == '\t')
    p++;
  while (q > p && (q[-1] == ' ' || q[-1] == '\t'))
    q--;
  *q = '\0';
  string_list_append (comments, p);
  free (copy);
}

static void
its_rule_list_extract_text (its_rule_list_ty *rules,
                            xmlNode *node,
                            const char *logical_filename,
                            message_list_ty *mlp,
                            its_extract_callback_ty callback)
{
  if (node->type == XML_ELEMENT_NODE
      || node->type == XML_ATTRIBUTE_NODE)
    {
      struct its_value_list_ty *values = its_rule_list_eval (rules, node);

      bool do_escape;
      {
        const char *value = its_value_list_get_value (values, "escape");
        do_escape = value != NULL && strcmp (value, "yes") == 0;
      }

      bool do_escape_during_extract = do_escape;
      /* But no, during message extraction (i.e. what xgettext does), we do
         *not* want escaping to be done.  The contents of the POT file is meant
         for translators, and
           - the messages are not labelled as requiring XML content syntax,
           - it is better for the translators if they can write various
             characters such as & < > without escaping them.
         Escaping needs to happen in the message merge phase (i.e. what msgfmt
         does) instead.  */
      do_escape_during_extract = false;

      char *comment = NULL;
      {
        const char *value = its_value_list_get_value (values, "locNote");
        if (value)
          comment = xstrdup (value);
        else
          {
            value = its_value_list_get_value (values, "locNotePointer");
            if (value)
              comment = _its_get_content (rules, node, value, ITS_WHITESPACE_TRIM,
                                          do_escape_during_extract);
          }
      }

      if (comment != NULL && *comment != '\0')
        {
          string_list_ty comments;
          string_list_init (&comments);
          _its_comment_append (&comments, comment);
          char *tmp = string_list_join (&comments, "\n", '\0', false);
          free (comment);
          comment = tmp;
        }
      else
        /* Extract comments preceding the node.  */
        {
          string_list_ty comments;
          string_list_init (&comments);

          xmlNode *sibling;
          for (sibling = node->prev; sibling; sibling = sibling->prev)
            if (sibling->type != XML_COMMENT_NODE || sibling->prev == NULL)
              break;
          if (sibling)
            {
              if (sibling->type != XML_COMMENT_NODE)
                sibling = sibling->next;
              for (; sibling && sibling->type == XML_COMMENT_NODE;
                   sibling = sibling->next)
                {
                  xmlChar *content = xmlNodeGetContent (sibling);
                  _its_comment_append (&comments, (const char *) content);
                  xmlFree (content);
                }
              free (comment);
              comment = string_list_join (&comments, "\n", '\0', false);
              string_list_destroy (&comments);
            }
        }

      enum its_whitespace_type_ty whitespace;
      {
        const char *value = its_value_list_get_value (values, "space");
        if (value && strcmp (value, "preserve") == 0)
          whitespace = ITS_WHITESPACE_PRESERVE;
        else if (value && strcmp (value, "trim") == 0)
          whitespace = ITS_WHITESPACE_TRIM;
        else if (value && strcmp (value, "paragraph") == 0)
          whitespace = ITS_WHITESPACE_NORMALIZE_PARAGRAPH;
        else
          whitespace = ITS_WHITESPACE_NORMALIZE;
      }

      char *msgctxt = NULL;
      {
        const char *value = its_value_list_get_value (values, "contextPointer");
        if (value)
          msgctxt = _its_get_content (rules, node, value, ITS_WHITESPACE_PRESERVE,
                                      do_escape_during_extract);
      }

      char *msgid = NULL;
      {
        const char *value = its_value_list_get_value (values, "textPointer");
        if (value)
          msgid = _its_get_content (rules, node, value, ITS_WHITESPACE_PRESERVE,
                                    do_escape_during_extract);
      }

      its_value_list_destroy (values);
      free (values);

      if (msgid == NULL)
        msgid = _its_collect_text_content (node, whitespace,
                                           do_escape_during_extract);
      if (*msgid != '\0')
        {
          lex_pos_ty pos;
          pos.file_name = xstrdup (logical_filename);
          pos.line_number = xmlGetLineNo (node);

          char *marker;
          if (node->type == XML_ELEMENT_NODE)
            {
              assert (node->parent);
              marker = xasprintf ("%s/%s", node->parent->name, node->name);
            }
          else
            {
              assert (node->parent && node->parent->parent);
              marker = xasprintf ("%s/%s@%s",
                                  node->parent->parent->name,
                                  node->parent->name,
                                  node->name);
            }

          if (msgctxt != NULL && *msgctxt == '\0')
            {
              free (msgctxt);
              msgctxt = NULL;
            }

          callback (mlp, msgctxt, msgid, &pos, comment, marker, whitespace);
          free (marker);
        }
      free (msgctxt);
      free (msgid);
      free (comment);
    }
}

void
its_rule_list_extract (its_rule_list_ty *rules,
                       FILE *fp, const char *real_filename,
                       const char *logical_filename,
                       msgdomain_list_ty *mdlp,
                       its_extract_callback_ty callback)
{
  xmlDoc *doc = xmlReadFd (fileno (fp), logical_filename, NULL,
                           XML_PARSE_NONET
                           | XML_PARSE_NOWARNING
                           | XML_PARSE_NOBLANKS
                           | XML_PARSE_NOERROR);
  if (doc == NULL)
    {
      const xmlError *err = xmlGetLastError ();
      error (err->level == XML_ERR_FATAL ? EXIT_FAILURE : 0, 0,
             _("cannot read %s: %s"), logical_filename, err->message);
      return;
    }

  if (setjmp (xml_error_exit) == 0)
    {
      xmlSetStructuredErrorFunc (NULL, structured_error);
      xmlSetGenericErrorFunc (NULL, generic_error);

      its_rule_list_apply (rules, doc);

      struct its_node_list_ty nodes;
      memset (&nodes, 0, sizeof (struct its_node_list_ty));
      its_rule_list_extract_nodes (rules,
                                   &nodes,
                                   xmlDocGetRootElement (doc));

      for (size_t i = 0; i < nodes.nitems; i++)
        its_rule_list_extract_text (rules, nodes.items[i],
                                    logical_filename,
                                    mdlp->item[0]->messages,
                                    callback);

      free (nodes.items);
    }
  else
    /* Caught a libxml error.  */
    ;

  xmlFreeDoc (doc);

  xmlSetGenericErrorFunc (NULL, NULL);
  xmlSetStructuredErrorFunc (NULL, NULL);
}


/* ========================== API only for msgfmt ========================== */

struct its_merge_context_ty
{
  its_rule_list_ty *rules;
  xmlDoc *doc;
  struct its_node_list_ty nodes;
};

/* Copies an element node and its attributes, but not its children nodes,
   for inserting at a sibling position in the document tree.  */
static xmlNode *
_its_copy_node_with_attributes (xmlNode *node)
{
  xmlNode *copy;

#if 0
  /* Not suitable here, because it adds namespace declaration attributes,
     which is overkill here.  */
  copy = xmlCopyNode (node, 2);
#elif 0
  /* Not suitable here either, for the same reason.  */
  copy = xmlNewNode (node->ns, node->name);
  copy->properties = xmlCopyPropList (copy, node->properties);
#else
  copy = xmlNewNode (node->ns, node->name);

  for (xmlAttr *attributes = node->properties;
       attributes != NULL;
       attributes = attributes->next)
    {
      const xmlChar *attr_name = attributes->name;
      if (strcmp ((const char *) attr_name, "id") != 0)
        {
          xmlNs *attr_ns = attributes->ns;
          xmlChar *attr_value =
            xmlGetNsProp (node, attr_name,
                          attr_ns != NULL ? attr_ns->href : NULL);
          xmlNewNsProp (copy, attr_ns, attr_name, attr_value);
          xmlFree (attr_value);
        }
    }
#endif

  return copy;
}

/* Returns true if S starts with a character reference.
   If so, and if UCS_P is non-NULL, it returns the Unicode code point
   in *UCS_P.  */
static bool
starts_with_character_reference (const char *s, unsigned int *ucs_p)
{
  /* <https://www.w3.org/TR/xml/#NT-CharRef> defines
     CharRef ::= '&#' [0-9]+ ';' | '&#x' [0-9a-fA-F]+ ';'  */
  if (*s == '&')
    {
      s++;
      if (*s == '#')
        {
          s++;
          if (*s >= '0' && *s <= '9')
            {
              bool overflow = false;
              unsigned int value = 0;
              do
                {
                  value = 10 * value + (*s - '0');
                  if (value >= 0x110000)
                    overflow = true;
                  s++;
                }
              while (*s >= '0' && *s <= '9');
              if (*s == ';')
                {
                  if (ucs_p != NULL)
                    *ucs_p = (overflow || (value >= 0xD800 && value <= 0xDFFF)
                              ? 0xFFFD
                              : value);
                  return true;
                }
              else
                return false;
            }
          if (*s == 'x')
            {
              s++;
              if ((*s >= '0' && *s <= '9')
                  || (*s >= 'A' && *s <= 'F')
                  || (*s >= 'a' && *s <= 'f'))
                {
                  bool overflow = false;
                  unsigned int value = 0;
                  do
                    {
                      value = 16 * value
                              + (*s >= '0' && *s <= '9' ? *s - '0' :
                                 *s >= 'A' && *s <= 'F' ? *s - 'A' + 10 :
                                 *s >= 'a' && *s <= 'f' ? *s - 'a' + 10 :
                                 0);
                      if (value >= 0x110000)
                        overflow = true;
                      s++;
                    }
                  while ((*s >= '0' && *s <= '9')
                         || (*s >= 'A' && *s <= 'F')
                         || (*s >= 'a' && *s <= 'f'));
                  if (*s == ';')
                    {
                      if (ucs_p != NULL)
                        *ucs_p = (overflow || (value >= 0xD800 && value <= 0xDFFF)
                                  ? 0xFFFD
                                  : value);
                      return true;
                    }
                  else
                    return false;
                }
            }
        }
    }
  return false;
}

static char *
_its_encode_special_chars_for_merge (const char *content)
{
  size_t amount = 0;
  for (const char *str = content; *str != '\0'; str++)
    {
      if (*str == '&' && starts_with_character_reference (str, NULL))
        amount += sizeof ("&amp;");
      else if (*str == '<')
        amount += sizeof ("&lt;");
      else if (*str == '>')
        amount += sizeof ("&gt;");
      else
        amount += 1;
    }

  char *result = XNMALLOC (amount + 1, char);
  {
    char *p = result;
    for (const char *str = content; *str != '\0'; str++)
      {
        if (*str == '&' && starts_with_character_reference (str, NULL))
          p = stpcpy (p, "&amp;");
        else if (*str == '<')
          p = stpcpy (p, "&lt;");
        else if (*str == '>')
          p = stpcpy (p, "&gt;");
        else
          *p++ = *str;
      }
    *p = '\0';
  }
  return result;
}

/* Attempts to set the document's encoding to UTF-8.
   Returns true if successful, or false if it failed.  */
static bool
set_doc_encoding_utf8 (xmlDoc *doc)
{
  if (doc->encoding == NULL)
    {
      doc->encoding = BAD_CAST xstrdup ("UTF-8");
      return true;
    }
  string_desc_t enc = sd_from_c ((char *) doc->encoding);
  if (sd_c_casecmp (enc, sd_from_c ("UTF-8")) == 0
      || sd_c_casecmp (enc, sd_from_c ("UTF8")) == 0)
    return true;
  /* The document's encoding is not UTF-8.  Conversion would be expensive.  */
  return false;
}

/* Parses CONTENTS as a piece of simple well-formed generalized XML
   ("simple" meaning without comments, CDATA, and other gobbledygook),
   with markup being limited to ASCII tags only.
   IGNORE_CASE means to ignore the case of tags (like in HTML).
   VALID_ELEMENT is a test whether to accept a given element name,
   or NULL to accept any element name.
   NO_END_ELEMENT is a test whether a given element name is one that is an
   empty element without needing an end tag (like e.g. <br> in HTML), or NULL
   for none.
   ADD_TO_NODE is the node (of type XML_ELEMENT_NODE) to which to add the
   contents in form of XML_TEXT_NODE and XML_ELEMENT_NODE nodes, or NULL
   for parsing without constructing the tree.
   Returns true if the parsing succeeded.
   Returns false with partially allocated children nodes (under ADD_TO_NODE,
   to be freed by the caller) if the parsing failed.  */
static bool
_its_is_valid_simple_gen_xml (const char *contents,
                              bool ignore_case,
                              bool (*valid_element) (string_desc_t tag),
                              bool (*no_end_element) (string_desc_t tag),
                              xmlNode *add_to_node)
{
  /* Specification:
     https://www.w3.org/TR/xml/  */

  xmlNode *parent_node = add_to_node;

  /* Stack of open elements.  */
  string_desc_t open_elements[100];
  size_t open_elements_count = 0;
  const size_t open_elements_max = SIZEOF (open_elements);

  const char *p = contents;
  const char *curr_text_segment_start = p;

  for (;;)
    {
      char c;

      c = *p;
      if (c == '\0')
        {
          if (open_elements_count > 0)
            return false;
          break;
        }
      if (c == '<')
        {
          if (add_to_node != NULL && curr_text_segment_start < p)
            {
              xmlNode *text_node = xmlNewDocTextLen (add_to_node->doc, NULL, 0);
              xmlNodeSetContentLen (text_node,
                                    BAD_CAST curr_text_segment_start,
                                    p - curr_text_segment_start);
              xmlAddChild (parent_node, text_node);
            }

          c = *++p;
          if (c == '\0')
            return false;

          bool slash_before_tag = false;
          if (c == '/')
            {
              slash_before_tag = true;
              c = *++p;
              if (c == '\0')
                return false;
            }

          /* Parse a name.
             <https://www.w3.org/TR/xml/#NT-Name>  */
          if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                || c == '_' || c == ':'))
            return false;
          const char *name_start = p;
          do
            {
              c = *++p;
              if (c == '\0')
                return false;
            }
          while ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                 || c == '_' || c == ':'
                 || (c >= '0' && c <= '9') || c == '-' || c == '.');
          const char *name_end = p;

          xmlNode *current_node = NULL;
          if (add_to_node != NULL && !slash_before_tag)
            {
              string_desc_t name =
                sd_new_addr (name_end - name_start, name_start);
              char *name_c = xsd_c (name);
              if (ignore_case)
                {
                  /* Convert the name to lower case.  */
                  for (char *np = name_c; *np != '\0'; np++)
                    *np = c_tolower (*np);
                }
              current_node =
                xmlNewDocNodeEatName (add_to_node->doc, NULL, BAD_CAST name_c,
                                      NULL);
              xmlAddChild (parent_node, current_node);
            }

          /* Skip over whitespace.
             <https://www.w3.org/TR/xml/#sec-common-syn>  */
          while (c == ' ' || c == '\t' || c == '\n' || c == '\r')
            {
              c = *++p;
              if (c == '\0')
                return false;
            }

          bool slash_after_tag = false;
          if (!slash_before_tag)
            {
              /* Parse a sequence of attributes.
                 <https://www.w3.org/TR/xml/#NT-Attribute>  */
              for (;;)
                {
                  if (!((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                        || c == '_' || c == ':'))
                    break;
                  const char *attr_name_start = p;
                  do
                    {
                      c = *++p;
                      if (c == '\0')
                        return false;
                    }
                  while ((c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z')
                         || c == '_' || c == ':'
                         || (c >= '0' && c <= '9') || c == '-' || c == '.');
                  const char *attr_name_end = p;
                  /* Skip over whitespace before '='.  */
                  while (c == ' ' || c == '\t' || c == '\n' || c == '\r')
                    {
                      c = *++p;
                      if (c == '\0')
                        return false;
                    }
                  /* Expect '='.  */
                  if (c != '=')
                    return false;
                  /* Skip over whitespace after '='.  */
                  do
                    {
                      c = *++p;
                      if (c == '\0')
                        return false;
                    }
                  while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
                  /* Skip over an attribute value.  */
                  const char *attr_value_start = NULL;
                  const char *attr_value_end = NULL;
                  if (c == '"')
                    {
                      attr_value_start = p + 1;
                      do
                        {
                          c = *++p;
                          if (c == '\0')
                            return false;
                        }
                      while (c != '"');
                      attr_value_end = p;
                    }
                  else if (c == '\'')
                    {
                      attr_value_start = p + 1;
                      do
                        {
                          c = *++p;
                          if (c == '\0')
                            return false;
                        }
                      while (c != '\'');
                      attr_value_end = p;
                    }
                  else
                    return false;
                  if (add_to_node != NULL)
                    {
                      string_desc_t attr_name =
                        sd_new_addr (attr_name_end - attr_name_start,
                                     attr_name_start);
                      string_desc_t attr_value =
                        sd_new_addr (attr_value_end - attr_value_start,
                                     attr_value_start);
                      char *attr_name_c = xsd_c (attr_name);
                      char *attr_value_c = xsd_c (attr_value);
                      xmlAttr *attr =
                        xmlNewProp (current_node, BAD_CAST attr_name_c,
                                    BAD_CAST attr_value_c);
                      if (attr == NULL)
                        xalloc_die ();
                      free (attr_value_c);
                      free (attr_name_c);
                    }
                  /* Skip over whitespace after the attribute value.  */
                  c = *++p;
                  if (c == '\0')
                    return false;
                  if (!(c == ' ' || c == '\t' || c == '\n' || c == '\r'))
                    break;
                  do
                    {
                      c = *++p;
                      if (c == '\0')
                        return false;
                    }
                  while (c == ' ' || c == '\t' || c == '\n' || c == '\r');
                }
              if (c == '/')
                {
                  slash_after_tag = true;
                  c = *++p;
                  if (c == '\0')
                    return false;
                }
            }
          if (c != '>')
            return false;
          /* Seen a complete <...> element start/end.  */

          /* Verify that the tag is allowed.  */
          string_desc_t tag = sd_new_addr (name_end - name_start, name_start);
          if (!(valid_element == NULL || valid_element (tag)))
            return false;
          if (slash_after_tag || (no_end_element != NULL && no_end_element (tag)))
            {
              /* Seen an empty element.  */
            }
          else if (!slash_before_tag)
            {
              /* Seen the start of an element.  */
              if (open_elements_count == open_elements_max)
                /* Nesting depth too high.  */
                return false;
              open_elements[open_elements_count++] = tag;
              if (add_to_node != NULL)
                parent_node = current_node;
            }
          else
            {
              /* Seen the end of an element.
                 Verify that the tag matches the one of the start.  */
              if (open_elements_count == 0)
                /* The end of an element without a corresponding start.  */
                return false;
              if ((ignore_case
                   ? sd_c_casecmp (open_elements[open_elements_count - 1], tag)
                   : sd_cmp (open_elements[open_elements_count - 1], tag))
                  != 0)
                return false;
              open_elements_count--;
              if (add_to_node != NULL)
                parent_node = parent_node->parent;
            }
          curr_text_segment_start = p + 1;
        }
      else if (c == '>')
        {
          /* Stray '>'.
             We could allow it, but better not.  */
          return false;
        }
      else if (c == '&')
        {
          /* Allow a character reference as a whole.
             Also allow a single '&', as it does not much harm.  */
          unsigned int ucs;
          if (starts_with_character_reference (p, &ucs))
            {
              const char *semicolon = strchr (p, ';');
              if (add_to_node != NULL)
                {
                  if (curr_text_segment_start < p)
                    {
                      xmlNode *text_node =
                        xmlNewDocTextLen (add_to_node->doc, NULL, 0);
                      xmlNodeSetContentLen (text_node,
                                            BAD_CAST curr_text_segment_start,
                                            p - curr_text_segment_start);
                      xmlAddChild (parent_node, text_node);
                    }
                  xmlNode *text_node =
                    xmlNewDocTextLen (add_to_node->doc, NULL, 0);
                  if (set_doc_encoding_utf8 (add_to_node->doc))
                    {
                      uint8_t buf[6];
                      int nbytes = u8_uctomb (buf, ucs, SIZEOF (buf));
                      if (nbytes <= 0)
                        abort ();
                      xmlNodeSetContentLen (text_node, BAD_CAST buf, nbytes);
                    }
                  else
                    xmlNodeSetContentLen (text_node, BAD_CAST p,
                                          semicolon + 1 - p);
                  /* Here it is useful that xmlAddChild merges adjacent text
                     nodes.  */
                  xmlAddChild (parent_node, text_node);
                }
              curr_text_segment_start = semicolon + 1;
              p = semicolon;
            }
        }
      p++;
    }

  if (add_to_node != NULL && curr_text_segment_start < p)
    {
      xmlNode *text_node = xmlNewDocTextLen (add_to_node->doc, NULL, 0);
      xmlNodeSetContentLen (text_node,
                            BAD_CAST curr_text_segment_start,
                            p - curr_text_segment_start);
      xmlAddChild (parent_node, text_node);
    }
  return true;
}

/* Returns true if CONTENTS is a piece of simple well-formed XML
   ("simple" meaning without comments, CDATA, and other gobbledygook),
   with markup being limited to ASCII tags only.  */
static bool
_its_is_valid_simple_xml (const char *contents)
{
  return _its_is_valid_simple_gen_xml (contents, false, NULL, NULL, NULL);
}

static bool
is_valid_xhtml_element (string_desc_t tag)
{
  /* Specification:
     https://www.w3.org/TR/xhtml1/
     https://www.w3.org/TR/xhtml1/dtds.html  */
  /* Sorted list of allowed tags.  */
  static const char allowed[41][12] =
    {
      "a", /* anchor */
      "abbr", /* abbreviation */
      "acronym", /* acronym */
      "address", /* address */
      "b", /* bold font style */
      "bdo", /* bidi override */
      "big", /* bigger font */
      "blockquote", /* block-like quote */
      "br", /* forced line break */
      "cite", /* citation */
      "code", /* program code */
      "dd", /* definition list item */
      "del", /* deleted text */
      "dfn", /* definitional */
      "dl", /* definition list */
      "dt", /* definition list item */
      "em", /* emphasis */
      "h1", /* heading */
      "h2", /* heading */
      "h3", /* heading */
      "h4", /* heading */
      "h5", /* heading */
      "h6", /* heading */
      "hr", /* horizontal rule */
      "i", /* italic font style */
      "ins", /* inserted text */
      "kbd", /* user typed */
      "li", /* list item */
      "ol", /* list */
      "p", /* paragraph */
      "pre", /* preformatted text */
      "q", /* inlined quote */
      "samp", /* sample */
      "small", /* smaller font */
      "span", /* generic container */
      "strong", /* strong emphasis */
      "sub", /* subscript */
      "sup", /* superscript */
      "tt", /* fixed-width font */
      "ul", /* list */
      "var" /* variable */
#if 0 /* I don't think it is appropriate for a translator to use these.  */
      "div", /* generic container */
      "script", /* only used in head */
      "object", /* embedded object */
      "param", /* parameter for object */
      "img", /* image */
      "map", /* image map */
      "area", /* image map */
      "form", /* form */
      "label", /* form element */
      "input", /* form control */
      "select", /* form control */
      "optgroup", /* form element */
      "option", /* form element */
      "textarea", /* user input */
      "fieldset", /* form element */
      "legend", /* form element */
      "button", /* form element */
      "table", /* table */
      "caption", /* table */
      "thead", /* table */
      "tfoot", /* table */
      "tbody", /* table */
      "colgroup", /* table */
      "col", /* table */
      "tr", /* table */
      "th", /* table */
      "td", /* table */
#endif
    };
  /* Use binary search.  */
  size_t lo = 0;
  size_t hi = SIZEOF (allowed);
  while (lo < hi)
    {
      /* Invariant:
         If tag occurs in the table, it is at an index >= lo, < hi.  */
      size_t i = (lo + hi) / 2; /* >= lo, < hi */
      int cmp = sd_cmp (tag, sd_from_c (allowed[i]));
      if (cmp == 0)
        return true;
      if (cmp < 0)
        hi = i;
      else
        lo = i + 1;
    }
  return false;
}

/* Returns true if the argument is a piece of simple well-formed XHTML
   ("simple" meaning without comments, CDATA, and other gobbledygook),
   with markup being limited to ASCII tags only.  */
static bool
_its_is_valid_simple_xhtml (const char *contents)
{
  return _its_is_valid_simple_gen_xml (contents, false,
                                       is_valid_xhtml_element, NULL, NULL);
}

static bool
is_valid_html_element (string_desc_t tag)
{
  /* Specification:
     https://html.spec.whatwg.org/
     sections
     4.3 Sections
     4.4 Grouping content
     4.5 Text-level semantics
     4.6 Links
     4.7 Edits
     I don't think it is appropriate for a translator to use elements from
     the other sections of chapter 4.  */
  /* Sorted list of allowed tags.  */
  static const char allowed[52][12] =
    {
      "a", /* anchor */
      "abbr", /* abbreviation */
      "acronym", /* acronym (removed in HTML 5) */
      "address", /* address */
      "b", /* bold font style */
      "bdi", /* bidi isolation */
      "bdo", /* bidi override */
      "big", /* bigger font (removed in HTML 5) */
      "blockquote", /* block-like quote */
      "br", /* forced line break */
      "cite", /* citation */
      "code", /* program code */
      "dd", /* definition list item */
      "del", /* deleted text */
      "dfn", /* definitional */
      "dl", /* definition list */
      "dt", /* definition list item */
      "em", /* emphasis */
      "figcaption",
      "figure",
      "h1", /* heading */
      "h2", /* heading */
      "h3", /* heading */
      "h4", /* heading */
      "h5", /* heading */
      "h6", /* heading */
      "hr", /* horizontal rule */
      "i", /* italic font style */
      "ins", /* inserted text */
      "kbd", /* user typed */
      "li", /* list item */
      "mark", /* marked */
      "menu", /* toolbar */
      "ol", /* list */
      "p", /* paragraph */
      "pre", /* preformatted text */
      "q", /* inlined quote */
      "rp", /* ruby */
      "rt", /* ruby */
      "ruby", /* ruby annotations */
      "s", /* strikethrough */
      "samp", /* sample */
      "small", /* smaller font */
      "span", /* generic container */
      "strong", /* strong emphasis */
      "sub", /* subscript */
      "sup", /* superscript */
      "tt", /* fixed-width font (removed in HTML 5) */
      "u", /* unarticulated */
      "ul", /* list */
      "var", /* variable */
      "wbr" /* possible line break */
    };
  /* Use binary search.  */
  size_t lo = 0;
  size_t hi = SIZEOF (allowed);
  while (lo < hi)
    {
      /* Invariant:
         If tag occurs in the table, it is at an index >= lo, < hi.  */
      size_t i = (lo + hi) / 2; /* >= lo, < hi */
      int cmp = sd_cmp (tag, sd_from_c (allowed[i]));
      if (cmp == 0)
        return true;
      if (cmp < 0)
        hi = i;
      else
        lo = i + 1;
    }
  return false;
}

static bool
is_no_end_html_element (string_desc_t tag)
{
  /* Specification:
     https://html.spec.whatwg.org/
     Search for "Tag omission in text/html: No end tag."  */
  return sd_cmp (tag, sd_from_c ("br")) == 0
         || sd_cmp (tag, sd_from_c ("hr")) == 0;
}

/* Returns true if the argument is a piece of simple well-formed HTML
   ("simple" meaning without comments, CDATA, and other gobbledygook),
   with markup being limited to ASCII tags only.  */
static bool
_its_is_valid_simple_html (const char *contents)
{
  /* Specification:
     https://html.spec.whatwg.org/  */
  return _its_is_valid_simple_gen_xml (contents, true,
                                       is_valid_html_element,
                                       is_no_end_html_element,
                                       NULL);
}

static bool
_its_set_simple_xml_content (xmlNode *node, const char *contents)
{
  /* This works fine for "xml" and "xhtml", but not for "html", due to
     elements with no end, such as <br>.  xmlParseInNodeContext returns error
     XML_ERR_NOT_WELL_BALANCED in this situation.  */
  xmlNode *newChildNodes = NULL;
  xmlParserErrors errors =
    xmlParseInNodeContext (node, contents, strlen (contents),
                           XML_PARSE_NONET | XML_PARSE_NOWARNING
                           | XML_PARSE_NOBLANKS | XML_PARSE_NOERROR,
                           &newChildNodes);
  if (errors == XML_ERR_OK)
    {
      if (newChildNodes != NULL)
        xmlAddChildList (node, newChildNodes);
      return true;
    }
  else
    return false;
}

static bool
_its_set_simple_html_content (xmlNode *node, const char *contents)
{
  if (_its_is_valid_simple_gen_xml (contents, true,
                                    is_valid_html_element,
                                    is_no_end_html_element,
                                    node))
    return true;
  else
    {
      xmlNodeSetContent (node, NULL);
      return false;
    }
}

static void
its_merge_context_merge_node (struct its_merge_context_ty *context,
                              xmlNode *node,
                              const char *language,
                              message_list_ty *mlp,
                              bool replace_text)
{
  if (node->type == XML_ELEMENT_NODE)
    {
      struct its_value_list_ty *values = its_rule_list_eval (context->rules, node);

      bool do_escape;
      {
        const char *value = its_value_list_get_value (values, "escape");
        do_escape = value != NULL && strcmp (value, "yes") == 0;
      }

      bool do_escape_during_extract = do_escape;
      /* Like above, in its_rule_list_extract_text.  */
      do_escape_during_extract = false;

      bool do_escape_during_merge = do_escape;

      const char *do_unescape_if = its_value_list_get_value (values, "unescape-if");

      enum its_whitespace_type_ty whitespace;
      {
        const char *value = its_value_list_get_value (values, "space");
        if (value && strcmp (value, "preserve") == 0)
          whitespace = ITS_WHITESPACE_PRESERVE;
        else if (value && strcmp (value, "trim") == 0)
          whitespace = ITS_WHITESPACE_TRIM;
        else if (value && strcmp (value, "paragraph") == 0)
          whitespace = ITS_WHITESPACE_NORMALIZE_PARAGRAPH;
        else
          whitespace = ITS_WHITESPACE_NORMALIZE;
      }

      char *msgctxt = NULL;
      {
        const char *value = its_value_list_get_value (values, "contextPointer");
        if (value)
          msgctxt = _its_get_content (context->rules, node, value,
                                      ITS_WHITESPACE_PRESERVE,
                                      do_escape_during_extract);
      }

      char *msgid = NULL;
      {
        const char *value = its_value_list_get_value (values, "textPointer");
        if (value)
          msgid = _its_get_content (context->rules, node, value,
                                    ITS_WHITESPACE_PRESERVE,
                                    do_escape_during_extract);
      }

      if (msgid == NULL)
        msgid = _its_collect_text_content (node, whitespace,
                                           do_escape_during_extract);
      if (*msgid != '\0')
        {
          message_ty *mp = message_list_search (mlp, msgctxt, msgid);
          if (mp && *mp->msgstr != '\0')
            {
              xmlNode *translated;
              if (replace_text)
                {
                  /* Reuse the node.  But first, clear its text content and all
                     its children nodes (except the attributes).  */
                  xmlNodeSetContent (node, NULL);
                  translated = node;
                }
              else
                {
                  /* Create a new element node, of the same name, with the same
                     attributes.  */
                  translated = _its_copy_node_with_attributes (node);
                }

              /* Set the xml:lang attribute.
                 <https://www.w3.org/International/questions/qa-when-xmllang.en.html>
                 says: "The value of the xml:lang attribute is a language tag
                 defined by BCP 47."  */
              char language_bcp47[BCP47_MAX];
              xpg_to_bcp47 (language_bcp47, language);
              xmlSetProp (translated, BAD_CAST "xml:lang", BAD_CAST language_bcp47);

              const char *msgstr = mp->msgstr;
              /* libxml2 offers two functions for setting the content of an
                 element: xmlNodeSetContent and xmlNodeAddContent.  They differ
                 in the amount of escaping they do:
                 - xmlNodeSetContent does no escaping, at the risk of creating
                   malformed XML.
                 - xmlNodeAddContent escapes all of & < >, which always produces
                   well-formed XML but is not the right thing for entity
                   references.
                 We need a middle ground between both, that is adapted to what
                 translators will usually produce.

                 translated       | no escaping | middle-ground | full escaping
                                  | SetContent  |               | AddContent
                 -----------------+-------------+---------------+--------------
                 &                | &           | &             | &amp;
                 &quot;           | &quot;      | &quot;        | &amp;quot;
                 &amp;            | &amp;       | &amp;         | &amp;amp;
                 <                | <           | &lt;          | &lt;
                 >                | >           | &gt;          | &gt;
                 &lt;             | &lt;        | &lt;          | &amp;lt;
                 &gt;             | &gt;        | &gt;          | &amp;gt;
                 &#xa9;           | &#xa9;      | &amp;#xa9;    | &amp;#xa9;
                 &copy;           | &copy;      | &copy;        | &amp;copy;
                 -----------------+-------------+---------------+--------------

                 The function _its_encode_special_chars_for_merge implements
                 this middle-ground.  But we allow full escaping to be requested
                 through a gt:escape="yes" attribute.  */

              if (do_escape_during_merge)
                {
                  /* These three are equivalent:
                     xmlNodeAddContent (translated, BAD_CAST msgstr);
                     xmlNodeSetContent (translated, xmlEncodeEntitiesReentrant (context->doc, BAD_CAST msgstr));
                     xmlNodeSetContent (translated, xmlEncodeSpecialChars (context->doc, BAD_CAST msgstr));  */
                  xmlNodeAddContent (translated, BAD_CAST msgstr);
                }
              else
                {
                  bool done_unescape = false;

                  if (do_unescape_if != NULL
                       && ((strcmp (do_unescape_if, "xml") == 0
                           && _its_is_valid_simple_xml (msgstr))
                          || (strcmp (do_unescape_if, "xhtml") == 0
                              && _its_is_valid_simple_xhtml (msgstr))
                          || (strcmp (do_unescape_if, "html") == 0
                              && _its_is_valid_simple_html (msgstr))))
                    {
                      /* It looks like the translator has provided a syntactically
                         valid XML or HTML markup.
                         Note: This is only a simple test; we don't check the XML
                         or XHTML schema or HTML DTD here.  Therefore in theory the
                         result may be invalid.  But this should be rare, since
                         translators most often only preserve the markup that was
                         present in the msgid; if they do this, the result will be
                         valid.  */
                      if (strcmp (do_unescape_if, "xml") == 0
                          || strcmp (do_unescape_if, "xhtml") == 0)
                        {
                          if (_its_set_simple_xml_content (translated, msgstr))
                            done_unescape = true;
                        }
                      else
                        {
                          /* For "html", we create the children nodes ourselves,
                             in order to deal with elements with no end, such as
                             <br>.  For "xml" and "xhtml", on the other hand,
                             this code would not work well, due to insufficient
                             handling of namespaces.  */
                          if (_its_set_simple_html_content (translated, msgstr))
                            done_unescape = true;
                        }
                    }
                  if (!done_unescape)
                    {
                      char *middle_ground = _its_encode_special_chars_for_merge (msgstr);
                      xmlNodeSetContent (translated, BAD_CAST middle_ground);
                      free (middle_ground);
                    }
                }

              if (!replace_text)
                xmlAddNextSibling (node, translated);
            }
        }
      free (msgid);
      free (msgctxt);
      its_value_list_destroy (values);
      free (values);
    }
  /* FIXME: If replace_text, we should handle nodes of type XML_ATTRIBUTE_NODE,
     because at least the "translatable" and "escape" properties are applicable
     to them.  */
}

void
its_merge_context_merge (its_merge_context_ty *context,
                         const char *language,
                         message_list_ty *mlp,
                         bool replace_text)
{
  if (setjmp (xml_error_exit) == 0)
    {
      xmlSetStructuredErrorFunc (NULL, structured_error);
      xmlSetGenericErrorFunc (NULL, generic_error);

      for (size_t i = 0; i < context->nodes.nitems; i++)
        its_merge_context_merge_node (context, context->nodes.items[i],
                                      language,
                                      mlp,
                                      replace_text);
    }
  else
    /* Caught a libxml error.  */
    ;

  xmlSetGenericErrorFunc (NULL, NULL);
  xmlSetStructuredErrorFunc (NULL, NULL);
}

struct its_merge_context_ty *
its_merge_context_alloc (its_rule_list_ty *rules,
                         const char *filename)
{
  xmlDoc *doc = xmlReadFile (filename, NULL,
                             XML_PARSE_NONET
                             | XML_PARSE_NOWARNING
                             | XML_PARSE_NOBLANKS
                             | XML_PARSE_NOERROR);
  if (doc == NULL)
    {
      const xmlError *err = xmlGetLastError ();
      error (err->level == XML_ERR_FATAL ? EXIT_FAILURE : 0, 0,
             _("cannot read %s: %s"), filename, err->message);
      return NULL;
    }

  struct its_merge_context_ty *result;
  if (setjmp (xml_error_exit) == 0)
    {
      xmlSetStructuredErrorFunc (NULL, structured_error);
      xmlSetGenericErrorFunc (NULL, generic_error);

      its_rule_list_apply (rules, doc);

      result = XMALLOC (struct its_merge_context_ty);
      result->rules = rules;
      result->doc = doc;

      /* Collect translatable nodes.  */
      memset (&result->nodes, 0, sizeof (struct its_node_list_ty));
      its_rule_list_extract_nodes (result->rules,
                                   &result->nodes,
                                   xmlDocGetRootElement (result->doc));
    }
  else
    {
      /* Caught a libxml error.  */
      if (result != NULL)
        free (result);
      result = NULL;
      xmlFreeDoc (doc);
    }

  xmlSetGenericErrorFunc (NULL, NULL);
  xmlSetStructuredErrorFunc (NULL, NULL);
  return result;
}

void
its_merge_context_write (struct its_merge_context_ty *context,
                         FILE *fp)
{
  if (setjmp (xml_error_exit) == 0)
    {
      xmlSetStructuredErrorFunc (NULL, structured_error);
      xmlSetGenericErrorFunc (NULL, generic_error);

      xmlDocFormatDump (fp, context->doc, 1);
    }
  else
    /* Caught a libxml error.  */
    ;

  xmlSetGenericErrorFunc (NULL, NULL);
  xmlSetStructuredErrorFunc (NULL, NULL);
}

void
its_merge_context_free (struct its_merge_context_ty *context)
{
  xmlFreeDoc (context->doc);
  free (context->nodes.items);
  free (context);
}
