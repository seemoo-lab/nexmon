/* GObject - GLib Type, Object, Parameter and Signal Library
 * Copyright (C) 2006 Imendio AB
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
 */

#include <glib-object.h>

/* --- MySingleton class --- */

struct _MySingleton {
  GObject parent_instance;
  int myint;
};

#define MY_TYPE_SINGLETON my_singleton_get_type ()
G_DECLARE_FINAL_TYPE (MySingleton, my_singleton, MY, SINGLETON, GObject)
G_DEFINE_FINAL_TYPE (MySingleton, my_singleton, G_TYPE_OBJECT)

static MySingleton *the_one_and_only = NULL;

/* --- methods --- */
static GObject*
my_singleton_constructor (GType                  type,
                          guint                  n_construct_properties,
                          GObjectConstructParam *construct_properties)
{
  if (the_one_and_only)
    return g_object_ref (G_OBJECT (the_one_and_only));
  else
    return G_OBJECT_CLASS (my_singleton_parent_class)->constructor (type, n_construct_properties, construct_properties);
}

static void
my_singleton_finalize (GObject *object)
{
  g_assert ((GObject *) the_one_and_only == object);
  the_one_and_only = NULL;

  G_OBJECT_CLASS (my_singleton_parent_class)->finalize (object);
}

static void
my_singleton_init (MySingleton *self)
{
  g_assert_null (the_one_and_only);
  the_one_and_only = self;
}

static void
my_singleton_set_property (GObject      *gobject,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
  MySingleton *self = (MySingleton *) gobject;

  g_assert (prop_id == 1);

  self->myint = g_value_get_int (value);
}

static void
my_singleton_get_property (GObject    *gobject,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
  MySingleton *self = (MySingleton *) gobject;

  g_assert (prop_id == 1);

  g_value_set_int (value, self->myint);
}

static void
my_singleton_class_init (MySingletonClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->constructor = my_singleton_constructor;
  object_class->finalize = my_singleton_finalize;
  object_class->set_property = my_singleton_set_property;
  object_class->get_property = my_singleton_get_property;

  g_object_class_install_property (G_OBJECT_CLASS (klass), 1,
                                   g_param_spec_int ("foo", NULL, NULL,
                                                     0, G_MAXINT, 0,
                                                     G_PARAM_READWRITE | G_PARAM_CONSTRUCT | G_PARAM_STATIC_STRINGS));
}

static void
test_singleton_construction (void)
{
  MySingleton *singleton, *obj;

  /* create the singleton */
  singleton = g_object_new (MY_TYPE_SINGLETON, NULL);
  g_assert_nonnull (singleton);

  /* assert _singleton_ creation */
  obj = g_object_new (MY_TYPE_SINGLETON, NULL);
  g_assert_true (singleton == obj);
  g_object_unref (obj);

  /* shutdown */
  g_object_unref (singleton);
}

static void
test_singleton_construct_property (void)
{
  MySingleton *singleton;

  g_test_summary ("Test that creating a singleton with a construct-time property works");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/2666");

  /* create the singleton and set a property at construction time */
  singleton = g_object_new (MY_TYPE_SINGLETON, "foo", 1, NULL);
  g_assert_nonnull (singleton);

  /* shutdown */
  g_object_unref (singleton);
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/gobject/singleton/construction", test_singleton_construction);
  g_test_add_func ("/gobject/singleton/construct-property", test_singleton_construct_property);

  return g_test_run ();
}
