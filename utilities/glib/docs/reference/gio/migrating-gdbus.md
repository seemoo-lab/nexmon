Title: Migrating to GDBus
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2010 Matthias Clasen
SPDX-FileCopyrightText: 2010, 2011 David Zeuthen

# Migrating to GDBus

## Conceptual differences

The central concepts of D-Bus are modelled in a very similar way in
dbus-glib and GDBus. Both have objects representing connections, proxies and
method invocations. But there are some important differences:

- dbus-glib uses the libdbus reference implementation, GDBus doesn't.
  Instead, it relies on GIO streams as transport layer, and has its own
  implementation for the D-Bus connection setup and authentication. Apart
  from using streams as transport, avoiding libdbus also lets GDBus avoid
  some thorny multithreading issues.
- dbus-glib uses the GObject type system for method arguments and return
  values, including a homegrown container specialization mechanism. GDBus
  relies on the GVariant type system which is explicitly designed to match
  D-Bus types.
- dbus-glib models only D-Bus interfaces and does not provide any types for
  objects. GDBus models both D-Bus interfaces (via the GDBusInterface,
  GDBusProxy and GDBusInterfaceSkeleton types) and objects (via the
  GDBusObject, GDBusObjectSkeleton and GDBusObjectProxy types).
- GDBus includes native support for the org.freedesktop.DBus.Properties (via
  the GDBusProxy type) and org.freedesktop.DBus.ObjectManager D-Bus
  interfaces, dbus-glib doesn't.
- The typical way to export an object in dbus-glib involves generating glue
  code from XML introspection data using dbus-binding-tool. GDBus provides a
  similar tool called gdbus-codegen that can also generate Docbook D-Bus
  interface documentation.
- dbus-glib doesn't provide any convenience API for owning and watching bus
  names, GDBus provides the `g_bus_own_name()` and `g_bus_watch_name()`
  family of convenience functions.
- GDBus provides API to parse, generate and work with Introspection XML,
  dbus-glib doesn't.
- GTestDBus provides API to create isolated unit tests

## API comparison

| dbus-glib | GDBus |
|-----------|-------|
| `DBusGConnection` | `GDBusConnection` |
| `DBusGProxy` | `GDBusProxy`, `GDBusInterface` - also see `GDBusObjectProxy` |
| `DBusGObject` | `GDBusInterfaceSkeleton`, `GDBusInterface` - also see `GDBusObjectSkeleton` |
| `DBusGMethodInvocation` | `GDBusMethodInvocation` |
| `dbus_g_bus_get()` | `g_bus_get_sync()`, also see `g_bus_get()` |
| `dbus_g_proxy_new_for_name()` | `g_dbus_proxy_new_sync()` and `g_dbus_proxy_new_for_bus_sync()`, also see `g_dbus_proxy_new()` |
| `dbus_g_proxy_add_signal()` | not needed, use the generic “g-signal” |
| `dbus_g_proxy_connect_signal()` | use `g_signal_connect()` with “g-signal” |
| `dbus_g_connection_register_g_object()` | `g_dbus_connection_register_object()` - also see `g_dbus_object_manager_server_export()` |
| `dbus_g_connection_unregister_g_object()` | `g_dbus_connection_unregister_object()` - also see `g_dbus_object_manager_server_unexport()` |
| `dbus_g_object_type_install_info()` | introspection data is installed while registering an object, see `g_dbus_connection_register_object()` |
| `dbus_g_proxy_begin_call()` | `g_dbus_proxy_call()` |
| `dbus_g_proxy_end_call()` | `g_dbus_proxy_call_finish()` |
| `dbus_g_proxy_call()` | `g_dbus_proxy_call_sync()` |
| `dbus_g_error_domain_register()` | `g_dbus_error_register_error_domain()` |
| `dbus_g_error_has_name()` | no direct equivalent, see `g_dbus_error_get_remote_error()` |
| `dbus_g_method_return()` | `g_dbus_method_invocation_return_value()` |
| `dbus_g_method_return_error()` | `g_dbus_method_invocation_return_error()` and variants |
| `dbus_g_method_get_sender()` | `g_dbus_method_invocation_get_sender()` |

## Owning bus names

Using dbus-glib, you typically call RequestName manually to own a name, like in the following excerpt:

```c
error = NULL;
res = dbus_g_proxy_call (system_bus_proxy,
                         "RequestName",
                         &error,
                         G_TYPE_STRING, NAME_TO_CLAIM,
                         G_TYPE_UINT,   DBUS_NAME_FLAG_ALLOW_REPLACEMENT,
                         G_TYPE_INVALID,
                         G_TYPE_UINT,   &result,
                         G_TYPE_INVALID);
if (!res)
  {
    if (error != NULL)
      {
        g_warning ("Failed to acquire %s: %s",
                   NAME_TO_CLAIM, error->message);
        g_error_free (error);
      }
    else
      {
        g_warning ("Failed to acquire %s", NAME_TO_CLAIM);
      }
    goto out;
  }

if (result != DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER)
  {
    if (error != NULL)
      {
        g_warning ("Failed to acquire %s: %s",
                   NAME_TO_CLAIM, error->message);
        g_error_free (error);
      }
    else
      {
        g_warning ("Failed to acquire %s", NAME_TO_CLAIM);
      }
    exit (1);
  }

dbus_g_proxy_add_signal (system_bus_proxy, "NameLost",
                         G_TYPE_STRING, G_TYPE_INVALID);
dbus_g_proxy_connect_signal (system_bus_proxy, "NameLost",
                             G_CALLBACK (on_name_lost), NULL, NULL);

/* further setup ... */
```

While you can do things this way with GDBus too, using
`g_dbus_proxy_call_sync()`, it is much nicer to use the high-level API for
this:

```c
static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  /* further setup ... */
}

/* ... */

  owner_id = g_bus_own_name (G_BUS_TYPE_SYSTEM,
                             NAME_TO_CLAIM,
                             G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT,
                             on_bus_acquired,
                             on_name_acquired,
                             on_name_lost,
                             NULL,
                             NULL);

  g_main_loop_run (loop);

  g_bus_unown_name (owner_id);
```

Note that `g_bus_own_name()` works asynchronously and requires you to enter
your mainloop to await the `on_name_acquired()` callback. Also note that in
order to avoid race conditions (e.g. when your service is activated by a
method call), you have to export your manager object before acquiring the
name. The `on_bus_acquired()` callback is the right place to do such
preparations.

## Creating proxies for well-known names

dbus-glib lets you create proxy objects for well-known names, like the following example:

```c
proxy = dbus_g_proxy_new_for_name (system_bus_connection,
                                   "org.freedesktop.Accounts",
                                   "/org/freedesktop/Accounts",
                                   "org.freedesktop.Accounts");
```

For a DBusGProxy constructed like this, method calls will be sent to the current owner of the name, and that owner can change over time.

The same can be achieved with GDBusProxy:

```c
error = NULL;
proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
                                       G_DBUS_PROXY_FLAGS_NONE,
                                       NULL, /* GDBusInterfaceInfo */
                                       "org.freedesktop.Accounts",
                                       "/org/freedesktop/Accounts",
                                       "org.freedesktop.Accounts",
                                       NULL, /* GCancellable */
                                       &error);
```

For an added layer of safety, you can specify what D-Bus interface the proxy
is expected to conform to by using the GDBusInterfaceInfo type.
Additionally, GDBusProxy loads, caches and tracks changes to the D-Bus
properties on the remote object. It also sets up match rules so D-Bus
signals from the remote object are delivered locally.

The GDBusProxy type normally isn't used directly - instead proxies
subclassing GDBusProxy generated by `gdbus-codegen` is used, see the section
called “Using gdbus-codegen”.

## Generating code and docs

### Using gdbus-codegen

dbus-glib comes with dbus-binding-tool, which can produce somewhat nice
client- and server-side wrappers for a D-Bus interface. With GDBus,
gdbus-codegen is used and like its counterpart, it also takes D-Bus
Introspection XML as input:

#### Example D-Bus Introspection XML

```xml
<node>
  <!-- org.gtk.GDBus.Example.ObjectManager.Animal:
       @short_description: Example docs generated by gdbus-codegen
       @since: 2.30

       This D-Bus interface is used to describe a simple animal.
    -->
  <interface name="org.gtk.GDBus.Example.ObjectManager.Animal">
    <!-- Mood: The mood of the animal.
         @since: 2.30

         Known values for this property include
         <literal>Happy</literal> and <literal>Sad</literal>. Use the
         org.gtk.GDBus.Example.ObjectManager.Animal.Poke() method to
         change this property.

         This property influences how often the animal jumps up and
         down, see the
         #org.gtk.GDBus.Example.ObjectManager.Animal::Jumped signal
         for more details.
    -->
    <property name="Mood" type="s" access="read"/>

    <!--
        Poke:
        @make_sad: Whether to make the animal sad.
        @make_happy: Whether to make the animal happy.
        @since: 2.30

        Method used to changing the mood of the animal. See also the
        #org.gtk.GDBus.Example.ObjectManager.Animal:Mood property.
      -->
    <method name="Poke">
      <arg direction="in" type="b" name="make_sad"/>
      <arg direction="in" type="b" name="make_happy"/>
    </method>

    <!--
        Jumped:
        @height: Height, in meters, that the animal jumped.
        @since: 2.30

        Emitted when the animal decides to jump.
      -->
    <signal name="Jumped">
      <arg type="d" name="height"/>
    </signal>

    <!--
        Foo:
        Property with no <quote>since</quote> annotation (should inherit the 2.30 from its containing interface).
      -->
    <property name="Foo" type="s" access="read"/>

    <!--
        Bar:
        @since: 2.36
        Property with a later <quote>since</quote> annotation.
      -->
    <property name="Bar" type="s" access="read"/>
  </interface>

  <!-- org.gtk.GDBus.Example.ObjectManager.Cat:
       @short_description: More example docs generated by gdbus-codegen

       This D-Bus interface is used to describe a cat. Right now there
       are no properties, methods or signals associated with this
       interface so it is essentially a <ulink
       url="http://en.wikipedia.org/wiki/Marker_interface_pattern">Marker
       Interface</ulink>.

       Note that D-Bus objects implementing this interface also
       implement the #org.gtk.GDBus.Example.ObjectManager.Animal
       interface.
    -->
  <interface name="org.gtk.GDBus.Example.ObjectManager.Cat">
  </interface>
</node>
```

If this XML is processed like this

```
gdbus-codegen --interface-prefix org.gtk.GDBus.Example.ObjectManager. \
              --generate-c-code generated-code	                      \
              --c-namespace Example 				      \
              --c-generate-object-manager			      \
              --generate-docbook generated-docs                       \
              gdbus-example-objectmanager.xml
```

then two files generated-code.h and generated-code.c are generated.
Additionally, two XML files
generated-docs-org.gtk.GDBus.Example.ObjectManager.Animal and
generated-docs-org.gtk.GDBus.Example.ObjectManager.Cat with Docbook XML are
generated.

While the contents of `generated-code.h` and `generated-code.c` are best described by the `gdbus-codegen` manual page, here's a brief example of how this generated code can be used:

```c
#include "gdbus-object-manager-example/objectmanager-gen.h"

/* ---------------------------------------------------------------------------------------------------- */

static GDBusObjectManagerServer *manager = NULL;

static gboolean
on_animal_poke (ExampleAnimal          *animal,
                GDBusMethodInvocation  *invocation,
                gboolean                make_sad,
                gboolean                make_happy,
                gpointer                user_data)
{
  if ((make_sad && make_happy) || (!make_sad && !make_happy))
    {
      g_dbus_method_invocation_return_dbus_error (invocation,
                                                  "org.gtk.GDBus.Examples.ObjectManager.Error.Failed",
                                                  "Exactly one of make_sad or make_happy must be TRUE");
      goto out;
    }

  if (make_sad)
    {
      if (g_strcmp0 (example_animal_get_mood (animal), "Sad") == 0)
        {
          g_dbus_method_invocation_return_dbus_error (invocation,
                                                      "org.gtk.GDBus.Examples.ObjectManager.Error.SadAnimalIsSad",
                                                      "Sad animal is already sad");
          goto out;
        }

      example_animal_set_mood (animal, "Sad");
      example_animal_complete_poke (animal, invocation);
      goto out;
    }

  if (make_happy)
    {
      if (g_strcmp0 (example_animal_get_mood (animal), "Happy") == 0)
        {
          g_dbus_method_invocation_return_dbus_error (invocation,
                                                      "org.gtk.GDBus.Examples.ObjectManager.Error.HappyAnimalIsHappy",
                                                      "Happy animal is already happy");
          goto out;
        }

      example_animal_set_mood (animal, "Happy");
      example_animal_complete_poke (animal, invocation);
      goto out;
    }

  g_assert_not_reached ();

 out:
  return G_DBUS_METHOD_INVOCATION_HANDLED;
}


static void
on_bus_acquired (GDBusConnection *connection,
                 const gchar     *name,
                 gpointer         user_data)
{
  ExampleObjectSkeleton *object;
  guint n;

  g_print ("Acquired a message bus connection\n");

  /* Create a new org.freedesktop.DBus.ObjectManager rooted at /example/Animals */
  manager = g_dbus_object_manager_server_new ("/example/Animals");

  for (n = 0; n < 10; n++)
    {
      gchar *s;
      ExampleAnimal *animal;

      /* Create a new D-Bus object at the path /example/Animals/N where N is 000..009 */
      s = g_strdup_printf ("/example/Animals/%03d", n);
      object = example_object_skeleton_new (s);
      g_free (s);

      /* Make the newly created object export the interface
       * org.gtk.GDBus.Example.ObjectManager.Animal (note
       * that @object takes its own reference to @animal).
       */
      animal = example_animal_skeleton_new ();
      example_animal_set_mood (animal, "Happy");
      example_object_skeleton_set_animal (object, animal);
      g_object_unref (animal);

      /* Cats are odd animals - so some of our objects implement the
       * org.gtk.GDBus.Example.ObjectManager.Cat interface in addition
       * to the .Animal interface
       */
      if (n % 2 == 1)
        {
          ExampleCat *cat;
          cat = example_cat_skeleton_new ();
          example_object_skeleton_set_cat (object, cat);
          g_object_unref (cat);
        }

      /* Handle Poke() D-Bus method invocations on the .Animal interface */
      g_signal_connect (animal,
                        "handle-poke",
                        G_CALLBACK (on_animal_poke),
                        NULL); /* user_data */

      /* Export the object (@manager takes its own reference to @object) */
      g_dbus_object_manager_server_export (manager, G_DBUS_OBJECT_SKELETON (object));
      g_object_unref (object);
    }

  /* Export all objects */
  g_dbus_object_manager_server_set_connection (manager, connection);
}

static void
on_name_acquired (GDBusConnection *connection,
                  const gchar     *name,
                  gpointer         user_data)
{
  g_print ("Acquired the name %s\n", name);
}

static void
on_name_lost (GDBusConnection *connection,
              const gchar     *name,
              gpointer         user_data)
{
  g_print ("Lost the name %s\n", name);
}


gint
main (gint argc, gchar *argv[])
{
  GMainLoop *loop;
  guint id;

  loop = g_main_loop_new (NULL, FALSE);

  id = g_bus_own_name (G_BUS_TYPE_SESSION,
                       "org.gtk.GDBus.Examples.ObjectManager",
                       G_BUS_NAME_OWNER_FLAGS_ALLOW_REPLACEMENT |
                       G_BUS_NAME_OWNER_FLAGS_REPLACE,
                       on_bus_acquired,
                       on_name_acquired,
                       on_name_lost,
                       loop,
                       NULL);

  g_main_loop_run (loop);

  g_bus_unown_name (id);
  g_main_loop_unref (loop);

  return 0;
}
```

This, on the other hand, is a client-side application using generated code:

```c
#include "gdbus-object-manager-example/objectmanager-gen.h"

/* ---------------------------------------------------------------------------------------------------- */

static void
print_objects (GDBusObjectManager *manager)
{
  GList *objects;
  GList *l;

  g_print ("Object manager at %s\n", g_dbus_object_manager_get_object_path (manager));
  objects = g_dbus_object_manager_get_objects (manager);
  for (l = objects; l != NULL; l = l->next)
    {
      ExampleObject *object = EXAMPLE_OBJECT (l->data);
      GList *interfaces;
      GList *ll;
      g_print (" - Object at %s\n", g_dbus_object_get_object_path (G_DBUS_OBJECT (object)));

      interfaces = g_dbus_object_get_interfaces (G_DBUS_OBJECT (object));
      for (ll = interfaces; ll != NULL; ll = ll->next)
        {
          GDBusInterface *interface = G_DBUS_INTERFACE (ll->data);
          g_print ("   - Interface %s\n", g_dbus_interface_get_info (interface)->name);

          /* Note that @interface is really a GDBusProxy instance - and additionally also
           * an ExampleAnimal or ExampleCat instance - either of these can be used to
           * invoke methods on the remote object. For example, the generated function
           *
           *  void example_animal_call_poke_sync (ExampleAnimal  *proxy,
           *                                      gboolean        make_sad,
           *                                      gboolean        make_happy,
           *                                      GCancellable   *cancellable,
           *                                      GError        **error);
           *
           * can be used to call the Poke() D-Bus method on the .Animal interface.
           * Additionally, the generated function
           *
           *  const gchar *example_animal_get_mood (ExampleAnimal *object);
           *
           * can be used to get the value of the :Mood property.
           */
        }
      g_list_free_full (interfaces, g_object_unref);
    }
  g_list_free_full (objects, g_object_unref);
}

static void
on_object_added (GDBusObjectManager *manager,
                 GDBusObject        *object,
                 gpointer            user_data)
{
  gchar *owner;
  owner = g_dbus_object_manager_client_get_name_owner (G_DBUS_OBJECT_MANAGER_CLIENT (manager));
  g_print ("Added object at %s (owner %s)\n", g_dbus_object_get_object_path (object), owner);
  g_free (owner);
}

static void
on_object_removed (GDBusObjectManager *manager,
                   GDBusObject        *object,
                   gpointer            user_data)
{
  gchar *owner;
  owner = g_dbus_object_manager_client_get_name_owner (G_DBUS_OBJECT_MANAGER_CLIENT (manager));
  g_print ("Removed object at %s (owner %s)\n", g_dbus_object_get_object_path (object), owner);
  g_free (owner);
}

static void
on_notify_name_owner (GObject    *object,
                      GParamSpec *pspec,
                      gpointer    user_data)
{
  GDBusObjectManagerClient *manager = G_DBUS_OBJECT_MANAGER_CLIENT (object);
  gchar *name_owner;

  name_owner = g_dbus_object_manager_client_get_name_owner (manager);
  g_print ("name-owner: %s\n", name_owner);
  g_free (name_owner);
}

static void
on_interface_proxy_properties_changed (GDBusObjectManagerClient *manager,
                                       GDBusObjectProxy         *object_proxy,
                                       GDBusProxy               *interface_proxy,
                                       GVariant                 *changed_properties,
                                       const gchar *const       *invalidated_properties,
                                       gpointer                  user_data)
{
  GVariantIter iter;
  const gchar *key;
  GVariant *value;
  gchar *s;

  g_print ("Properties Changed on %s:\n", g_dbus_object_get_object_path (G_DBUS_OBJECT (object_proxy)));
  g_variant_iter_init (&iter, changed_properties);
  while (g_variant_iter_next (&iter, "{&sv}", &key, &value))
    {
      s = g_variant_print (value, TRUE);
      g_print ("  %s -> %s\n", key, s);
      g_variant_unref (value);
      g_free (s);
    }
}

gint
main (gint argc, gchar *argv[])
{
  GDBusObjectManager *manager;
  GMainLoop *loop;
  GError *error;
  gchar *name_owner;

  manager = NULL;
  loop = NULL;

  loop = g_main_loop_new (NULL, FALSE);

  error = NULL;
  manager = example_object_manager_client_new_for_bus_sync (G_BUS_TYPE_SESSION,
                                                            G_DBUS_OBJECT_MANAGER_CLIENT_FLAGS_NONE,
                                                            "org.gtk.GDBus.Examples.ObjectManager",
                                                            "/example/Animals",
                                                            NULL, /* GCancellable */
                                                            &error);
  if (manager == NULL)
    {
      g_printerr ("Error getting object manager client: %s", error->message);
      g_error_free (error);
      goto out;
    }

  name_owner = g_dbus_object_manager_client_get_name_owner (G_DBUS_OBJECT_MANAGER_CLIENT (manager));
  g_print ("name-owner: %s\n", name_owner);
  g_free (name_owner);

  print_objects (manager);

  g_signal_connect (manager,
                    "notify::name-owner",
                    G_CALLBACK (on_notify_name_owner),
                    NULL);
  g_signal_connect (manager,
                    "object-added",
                    G_CALLBACK (on_object_added),
                    NULL);
  g_signal_connect (manager,
                    "object-removed",
                    G_CALLBACK (on_object_removed),
                    NULL);
  g_signal_connect (manager,
                    "interface-proxy-properties-changed",
                    G_CALLBACK (on_interface_proxy_properties_changed),
                    NULL);

  g_main_loop_run (loop);

 out:
  if (manager != NULL)
    g_object_unref (manager);
  if (loop != NULL)
    g_main_loop_unref (loop);

  return 0;
}
```
