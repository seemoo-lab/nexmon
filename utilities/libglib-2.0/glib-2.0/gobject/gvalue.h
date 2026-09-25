/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 1997-1999, 2000-2001 Tim Janik and Red Hat, Inc.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * gvalue.h: generic GValue functions
 */
#ifndef __G_VALUE_H__
#define __G_VALUE_H__

#if !defined (__GLIB_GOBJECT_H_INSIDE__) && !defined (GOBJECT_COMPILATION)
#error "Only <glib-object.h> can be included directly."
#endif

#include	<gobject/gtype.h>

G_BEGIN_DECLS

/* --- type macros --- */
/**
 * G_TYPE_IS_VALUE:
 * @type: a type
 * 
 * Checks whether the passed in type ID can be used for
 * [method@GObject.Value.init].
 *
 * That is, this macro checks whether this type provides an implementation
 * of the [struct@GObject.TypeValueTable] functions required to be able to
 * create a [struct@GObject.Value] instance.
 * 
 * Returns: Whether @type is suitable as a [struct@GObject.Value] type.
 */
#define	G_TYPE_IS_VALUE(type)		(g_type_check_is_value_type (type))

/**
 * G_IS_VALUE:
 * @value: a [struct@GObject.Value] structure
 * 
 * Checks if @value is a valid and initialized [struct@GObject.Value] structure.
 *
 * Returns: true on success; false otherwise
 */
#define	G_IS_VALUE(value)		(G_TYPE_CHECK_VALUE (value))

/**
 * G_VALUE_TYPE:
 * @value: a [struct@GObject.Value] structure
 *
 * Get the type identifier of @value.
 *
 * Returns: the type ID
 */
#define	G_VALUE_TYPE(value)		(((GValue*) (value))->g_type)

/**
 * G_VALUE_TYPE_NAME:
 * @value: a [struct@GObject.Value] structure
 *
 * Gets the name of the type of @value.
 *
 * Returns: the type name
 */
#define	G_VALUE_TYPE_NAME(value)	(g_type_name (G_VALUE_TYPE (value)))

/**
 * G_VALUE_HOLDS:
 * @value: (not nullable): a [struct@GObject.Value] structure
 * @type: a [type@GObject.Type]
 *
 * Checks if @value holds a value of @type.
 *
 * This macro will also check for `value != NULL` and issue a
 * warning if the check fails.
 *
 * Returns: true if @value is non-`NULL` and holds a value of the given @type;
 *   false otherwise
 */
#define G_VALUE_HOLDS(value,type)	(G_TYPE_CHECK_VALUE_TYPE ((value), (type)))


/* --- typedefs & structures --- */
/**
 * GValueTransform:
 * @src_value: source value
 * @dest_value: target value
 * 
 * The type of value transformation functions which can be registered with
 * [func@GObject.Value.register_transform_func].
 *
 * @dest_value will be initialized to the correct destination type.
 */
typedef void (*GValueTransform) (const GValue *src_value,
				 GValue       *dest_value);

/**
 * GValue:
 * 
 * An opaque structure used to hold different types of values.
 *
 * Before it can be used, a `GValue` has to be initialized to a specific type by
 * calling [method@GObject.Value.init] on it.
 *
 * Many types which are stored within a `GValue` need to allocate data on the
 * heap, so [method@GObject.Value.unset] must always be called on a `GValue` to
 * free any such data once you’re finished with the `GValue`, even if the
 * `GValue` itself is stored on the stack.
 *
 * The data within the structure has protected scope: it is accessible only
 * to functions within a [struct@GObject.TypeValueTable] structure, or
 * implementations of the `g_value_*()` API. That is, code which implements new
 * fundamental types.
 *
 * `GValue` users cannot make any assumptions about how data is stored
 * within the 2 element @data union, and the @g_type member should
 * only be accessed through the [func@GObject.VALUE_TYPE] macro and related
 * macros.
 */
struct _GValue
{
  /*< private >*/
  GType		g_type;

  /* public for GTypeValueTable methods */
  union {
    gint	v_int;
    guint	v_uint;
    glong	v_long;
    gulong	v_ulong;
    gint64      v_int64;
    guint64     v_uint64;
    gfloat	v_float;
    gdouble	v_double;
    gpointer	v_pointer;
  } data[2];
};


/* --- prototypes --- */
GOBJECT_AVAILABLE_IN_ALL
GValue*         g_value_init	   	(GValue       *value,
					 GType         g_type);
GOBJECT_AVAILABLE_IN_ALL
void            g_value_copy    	(const GValue *src_value,
					 GValue       *dest_value);
GOBJECT_AVAILABLE_IN_ALL
GValue*         g_value_reset   	(GValue       *value);
GOBJECT_AVAILABLE_IN_ALL
void            g_value_unset   	(GValue       *value);
GOBJECT_AVAILABLE_IN_ALL
void		g_value_set_instance	(GValue	      *value,
					 gpointer      instance);
GOBJECT_AVAILABLE_IN_2_42
void            g_value_init_from_instance   (GValue       *value,
                                              gpointer      instance);


/* --- private --- */
GOBJECT_AVAILABLE_IN_ALL
gboolean	g_value_fits_pointer	(const GValue *value);
GOBJECT_AVAILABLE_IN_ALL
gpointer	g_value_peek_pointer	(const GValue *value);


/* --- implementation details --- */
GOBJECT_AVAILABLE_IN_ALL
gboolean g_value_type_compatible	(GType		 src_type,
					 GType		 dest_type);
GOBJECT_AVAILABLE_IN_ALL
gboolean g_value_type_transformable	(GType           src_type,
					 GType           dest_type);
GOBJECT_AVAILABLE_IN_ALL
gboolean g_value_transform		(const GValue   *src_value,
					 GValue         *dest_value);
GOBJECT_AVAILABLE_IN_ALL
void	g_value_register_transform_func	(GType		 src_type,
					 GType		 dest_type,
					 GValueTransform transform_func);

/**
 * G_VALUE_NOCOPY_CONTENTS:
 *
 * Flag to indicate that allocated data in a [struct@GObject.Value] shouldn’t be
 * copied.
 *
 * If passed to [func@GObject.VALUE_COLLECT], allocated data won’t be copied
 * but used verbatim. This does not affect ref-counted types like objects.
 *
 * This does not affect usage of [method@GObject.Value.copy]: the data will
 * be copied if it is not ref-counted.
 *
 * This flag should be checked by implementations of
 * [callback@GObject.TypeValueFreeFunc], [callback@GObject.TypeValueCollectFunc]
 * and [callback@GObject.TypeValueLCopyFunc].
 */
#define G_VALUE_NOCOPY_CONTENTS (1 << 27)

/**
 * G_VALUE_INTERNED_STRING:
 *
 * Flag to indicate that a string in a [struct@GObject.Value] is canonical and
 * will exist for the duration of the process.
 *
 * See [method@GObject.Value.set_interned_string].
 *
 * This flag should be checked by implementations of
 * [callback@GObject.TypeValueFreeFunc], [callback@GObject.TypeValueCollectFunc]
 * and [callback@GObject.TypeValueLCopyFunc].
 *
 * Since: 2.66
 */
#define G_VALUE_INTERNED_STRING (1 << 28) GOBJECT_AVAILABLE_MACRO_IN_2_66

/**
 * G_VALUE_INIT:
 *
 * Clears a [struct@GObject.Value] to zero at declaration time.
 *
 * A [struct@GObject.Value] must be cleared and then initialized before it can
 * be used. This macro can be assigned to a variable instead of an explicit
 * `{ 0 }` when declaring it, but it cannot be assigned to a variable after
 * declaration time.
 *
 * After the [struct@GObject.Value] is cleared, it must be initialized by
 * calling [method@GObject.Value.init] on it before any other methods can be
 * called on it.
 *
 * ```c
 *   GValue value = G_VALUE_INIT;
 *
 *   g_value_init (&value, SOME_G_TYPE);
 *   …
 *   g_value_unset (&value);
 * ```
 *
 * Since: 2.30
 */
#define G_VALUE_INIT  { 0, { { 0 } } }


G_END_DECLS

#endif /* __G_VALUE_H__ */
