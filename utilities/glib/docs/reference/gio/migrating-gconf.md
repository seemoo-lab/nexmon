Title: Migrating from GConf to GSettings
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2010, 2012 Matthias Clasen
SPDX-FileCopyrightText: 2010 Allison Lortie
SPDX-FileCopyrightText: 2011 Ray Strode

# Migrating from GConf to GSettings

## Before you start

Converting individual applications and their settings from GConf to
GSettings can be done at will. But desktop-wide settings like font or theme
settings often have consumers in multiple modules. Therefore, some
consideration has to go into making sure that all users of a setting are
converted to GSettings at the same time or that the program responsible for
configuring that setting continues to update the value in both places.

It is always a good idea to have a look at how others have handled similar
problems before.

## Conceptual differences

Conceptually, GConf and GSettings are fairly similar. Both have a concept of
pluggable backends. Both keep information about keys and their types in
schemas. Both have a concept of mandatory values, which lets you implement
lock-down.

There are some differences in the approach to schemas. GConf installs the
schemas into the database and has API to handle schema information
(`gconf_client_get_default_from_schema()`, `gconf_value_get_schema()`, etc).
GSettings on the other hand assumes that an application knows its own
schemas, and does not provide API to handle schema information at runtime.
GSettings is also more strict about requiring a schema whenever you want to
read or write a key. To deal with more free-form information that would
appear in schema-less entries in GConf, GSettings allows for schemas to be
'relocatable'.

One difference in the way applications interact with their settings is that
with GConf you interact with a tree of settings (ie the keys you pass to
functions when reading or writing values are actually paths with the actual
name of the key as the last element. With GSettings, you create a GSettings
object which has an implicit prefix that determines where the settings get
stored in the global tree of settings, but the keys you pass when reading or
writing values are just the key names, not the full path.

## GConfClient (and GConfBridge) API conversion

Most people use GConf via the high-level `GConfClient` API. The
corresponding API is the [class@Gio.Settings] object. While not every
`GConfClient` function has a direct GSettings equivalent, many do:

| GConfClient | GSettings |
|-------------|-----------|
| `gconf_client_get_default()` | no direct equivalent, instead you call [`ctor@Gio.Settings.new`] for the schemas you use |
| `gconf_client_set()` | [`method@Gio.Settings.set`] |
| `gconf_client_get()` | `g_settings_get()` |
| `gconf_client_get_bool()` | `g_settings_get_boolean()` |
| `gconf_client_set_bool()` | `g_settings_set_boolean()` |
| `gconf_client_get_int()` | `g_settings_get_int()` |
| `gconf_client_set_int()` | `g_settings_set_int()` |
| `gconf_client_get_float()` | `g_settings_get_double()` |
| `gconf_client_set_float()` | `g_settings_set_double()` |
| `gconf_client_get_string()` | `g_settings_get_string()` |
| `gconf_client_set_string()` | `g_settings_set_string()` |
| `gconf_client_get_list()` | for string lists, see `g_settings_get_strv()`, else see `g_settings_get_value()` and GVariant API |
| `gconf_client_set_list()` | for string lists, see `g_settings_set_strv()`, else see `g_settings_set_value()` and GVariant API |
| `gconf_entry_get_is_writable()` | `g_settings_is_writable()` |
| `gconf_client_notify_add()` | not required, the “changed” signal is emitted automatically |
| `gconf_client_add_dir()` | not required, each GSettings instance automatically watches all keys in its path |
| `GConfChangeSet` | `g_settings_delay()`, `g_settings_apply()` |
| `gconf_client_get_default_from_schema()` | no equivalent, applications are expected to know their schema |
| `gconf_client_all_entries()` | no equivalent, applications are expected to know their schema, and GSettings does not allow schema-less entries |
| `gconf_client_get_without_default()` | no equivalent |
| `gconf_bridge_bind_property()` | `g_settings_bind()` |
| `gconf_bridge_bind_property_full()` | `g_settings_bind_with_mapping()` |

GConfBridge was a third-party library that used GConf to bind an object
property to a particular configuration key. GSettings offers this service
itself.

There is a pattern that is sometimes used for GConf, where a setting can
have explicit 'value A', explicit 'value B' or 'use the system default'.
With GConf, 'use the system default' is sometimes implemented by unsetting
the user value. This is not possible in GSettings, since it does not have
API to determine if a value is the default and does not let you unset
values. The recommended way (and much clearer) way in which this can be
implemented in GSettings is to have a separate 'use-system-default' boolean
setting.

## Change notification

GConf requires you to call `gconf_client_add_dir()` and
`gconf_client_notify_add()` to get change notification. With GSettings, this
is not necessary; signals get emitted automatically for every change.

The [signal@Gio.Settings::changed] signal is emitted for each changed key.
There is also a [`signal@Gio.Settings::change-event`] signal that you can
handle if you need to see groups of keys that get changed at the same time.

GSettings also notifies you about changes in writability of keys, with the
[signal@Gio.Settings::writable-changed] signal (and the
[signal@Gio.Settings::writable-change-event] signal).

## Change sets

GConf has a concept of a set of changes which can be applied or reverted at
once: `GConfChangeSet` (GConf doesn't actually apply changes atomically,
which is one of its shortcomings).

Instead of a separate object to represent a change set, GSettings has a
'delayed-apply' mode, which can be turned on for a [class@Gio.Settings]
object by calling [method@Gio.Settings.delay]. In this mode, changes done to
the GSettings object are not applied - they are still visible when calling
[method@Gio.Settings.get] on the same object, but not to other GSettings
instances or even other processes.

To apply the pending changes all at once (GSettings does atomicity here),
call [method@Gio.Settings.apply]. To revert the pending changes, call
[method@Gio.Settings.revert] or just drop the reference to the GSettings
object.

## Schema conversion

If you are porting your application from GConf, most likely you already have
a GConf schema. GConf comes with a commandline tool
`gsettings-schema-convert` that can help with the task of converting a GConf
schema into an equivalent GSettings schema. The tool is not perfect and may
need assistance in some cases.

### An example for using gsettings-schema-convert

Running `gsettings-schema-convert --gconf --xml --schema-id
"org.gnome.font-rendering" --output org.gnome.font-rendering.gschema.xml
desktop_gnome_font_rendering.schemas` on the following
`desktop_gnome_font_rendering.schemas` file:

```xml
<?xml version="1.0"?>
<gconfschemafile>
    <schemalist>
        <schema>
            <key>/schemas/desktop/gnome/font_rendering/dpi</key>
            <applyto>/desktop/gnome/font_rendering/dpi</applyto>
            <owner>gnome</owner>
            <type>int</type>
            <default>96</default>
            <locale name="C">
                <short>DPI</short>
                <long>The resolution used for converting font sizes to pixel sizes, in dots per inch.</long>
            </locale>
        </schema>
    </schemalist>
</gconfschemafile>
```

produces an `org.gnome.font-rendering.gschema.xml` file with the following content:

```xml
<schemalist>
  <schema id="org.gnome.font-rendering" path="/desktop/gnome/font_rendering/">
    <key name="dpi" type="i">
      <default>96</default>
      <summary>DPI</summary>
      <description>The resolution used for converting font sizes to pixel sizes, in dots per inch.</description>
    </key>
  </schema>
</schemalist>
```

GSettings schemas are identified at runtime by their id (as specified in the
XML source file). It is recommended to use a dotted name as schema id,
similar in style to a D-Bus bus name, e.g. "org.gnome.SessionManager". In
cases where the settings are general and not specific to one application,
the id should not use StudlyCaps, e.g. "org.gnome.font-rendering". The
filename used for the XML schema source is immaterial, but schema compiler
expects the files to have the extension `.gschema.xml`. It is recommended to
simply use the schema id as the filename, followed by this extension, e.g.
`org.gnome.SessionManager.gschema.xml`.

The XML source file for your GSettings schema needs to get installed into
`$datadir/glib-2.0/schemas`, and needs to be compiled into a binary form. At
runtime, GSettings looks for compiled schemas in the `glib-2.0/schemas`
subdirectories of all `XDG_DATA_DIRS` directories, so if you install your
schema in a different location, you need to set the `XDG_DATA_DIRS`
environment variable appropriately.

Schemas are compiled into binary form by the `glib-compile-schemas` utility.
GIO provides a `glib_compile_schemas` variable in its pkg-config file
pointing to the schema compiler binary.

### Using schemas with Meson

You should use `install_data()` to install the `.gschema.xml` file in the
correct directory, e.g.

```
install_data('my.app.gschema.xml', install_dir: get_option('datadir') / 'glib-2.0/schemas')
```

Schema compilation is done at installation time; if you are using Meson 0.57 or newer, you can use the `gnome.post_install()` function from the GNOME module:

```
gnome.post_install(glib_compile_schemas: true)
```

Alternatively, you can use `meson.add_install_script()` and the following
Python script:

```py
#!/usr/bin/env python3
# build-aux/compile-schemas.py

import os
import subprocess

install_prefix = os.environ['MESON_INSTALL_PREFIX']
schemadir = os.path.join(install_prefix, 'share', 'glib-2.0', 'schemas')

if not os.environ.get('DESTDIR'):
    print('Compiling gsettings schemas...')
    subprocess.call(['glib-compile-schemas', schemadir])
```

```
meson.add_install_script('build-aux/compile-schemas.py')
```

### Using schemas with Autotools

GLib provides m4 macros for hiding the various complexities and reduce the
chances of getting things wrong.

To handle schemas in your Autotools build, start by adding this to your
`configure.ac`:

```
GLIB_GSETTINGS
```

Then add this fragment to your `Makefile.am`:

```
# gsettings_SCHEMAS is a list of all the schemas you want to install
gsettings_SCHEMAS = my.app.gschema.xml

# include the appropriate makefile rules for schema handling
@GSETTINGS_RULES@
```

This is not sufficient on its own. You need to mention what the source of
the `my.app.gschema.xml` file is. If the schema file is distributed directly
with your project's tarball then a mention in `EXTRA_DIST` is appropriate. If
the schema file is generated from another source then you will need the
appropriate rule for that, plus probably an item in `EXTRA_DIST` for the
source files used by that rule.

One possible pitfall in doing schema conversion is that the default values
in GSettings schemas are parsed by the GVariant parser. This means that
strings need to include quotes in the XML. Also note that the types are now
specified as GVariant type strings.

```xml
<type>string</type>
<default>rgb</default>
```

becomes

```xml
<key name="rgba-order" type="s">
  <default>'rgb'</default> <!-- note quotes -->
</key>
```

Another possible complication is that GConf specifies full paths for each
key, while a GSettings schema has a 'path' attribute that contains the
prefix for all the keys in the schema, and individual keys just have a
simple name. So

```xml
<key>/schemas/desktop/gnome/font_rendering/antialiasing</key>
```

becomes

```xml
<schema id="org.gnome.font" path="/desktop/gnome/font_rendering/">
  <key name="antialiasing" type="s">
```


Default values can be localized in both GConf and GSettings schemas, but
GSettings uses gettext for the localization. You can specify the gettext
domain to use in the gettext-domain attribute. Therefore, when converting
localized defaults in GConf,

```xml
<key>/schemas/apps/my_app/font_size</key>
  <locale name="C">
    <default>18</default>
  </locale>
  <locale name="be">
    <default>24</default>
  </locale>
</key>
```

becomes

```xml
<schema id="..." gettext-domain="your-domain">
 ...
<key name="font-size" type="i">
  <default l10n="messages" context="font_size">18</default>
</key>
```

GSettings uses gettext for translation of default values. The string that is
translated is exactly the string that appears inside of the `<default>`
element. This includes the quotation marks that appear around strings.
Default values must be marked with the l10n attribute in the `<default>` tag,
which should be set as equal to 'messages' or 'time' depending on the
desired category. An optional translation context can also be specified with
the context attribute, as in the example. This is usually recommended, since
the string "18" is not particularly easy to translate without context. The
translated version of the default value should be stored in the specified
gettext-domain. Care must be taken during translation to ensure that all
translated values remain syntactically valid; mistakes here will cause
runtime errors.

GSettings schemas have optional `<summary>` and `<description>` elements for
each key which correspond to the `<short>` and `<long>` elements in the
GConf schema and can be used in the same way by a GUI editor, so you should
use the same conventions for them: The summary is just a short label with no
punctuation, the description can be one or more complete sentences. If
multiple paragraphs are desired for the description, the paragraphs should
be separated by a completely empty line.

Translations for these strings will also be handled via gettext, so you
should arrange for these strings to be extracted into your gettext catalog.
Gettext supports GSettings schemas natively since version 0.19, so all you
have to do is add the XML schema file to the list of translatable files
inside your `POTFILES.in`.

GSettings is a bit more restrictive about key names than GConf. Key names in
GSettings can be at most 32 characters long, and must only consist of
lowercase characters, numbers and dashes, with no consecutive dashes. The
first character must not be a number or dash, and the last character cannot
be '-'.

If you are using the GConf backend for GSettings during the transition, you
may want to keep your key names the same they were in GConf, so that
existing settings in the users GConf database are preserved. You can achieve
this by using the `--allow-any-name` with the `glib-compile-schemas` schema
compiler. Note that this option is only meant to ease the process of porting
your application, allowing parts of your application to continue to access
GConf and parts to use GSettings. By the time you have finished porting your
application you must ensure that all key names are valid.

## Data conversion

GConf comes with a GSettings backend that can be used to facility the
transition to the GSettings API until you are ready to make the jump to a
different backend (most likely dconf). To use it, you need to set the
`GSETTINGS_BACKEND` to 'gconf', e.g. by using

```c
g_setenv ("GSETTINGS_BACKEND", "gconf", TRUE);
```

early on in your program. Note that this backend is meant purely as a
transition tool, and should not be used in production.

GConf also comes with a utility called `gsettings-data-convert`, which is
designed to help with the task of migrating user settings from GConf into
another GSettings backend. It can be run manually, but it is designed to be
executed automatically, every time a user logs in. It keeps track of the
data migrations that it has already done, and it is harmless to run it more
than once.

To make use of this utility, you must install a keyfile in the directory
`/usr/share/GConf/gsettings` which lists the GSettings keys and GConf paths
to map to each other, for each schema that you want to migrate user data
for.

Here is an example:

```
[org.gnome.fonts]
antialiasing = /desktop/gnome/font_rendering/antialiasing
dpi = /desktop/gnome/font_rendering/dpi
hinting = /desktop/gnome/font_rendering/hinting
rgba-order = /desktop/gnome/font_rendering/rgba_order

[apps.myapp:/path/to/myapps/]
some-odd-key1 = /apps/myapp/some_ODD-key1
```

The last key demonstrates that it may be necessary to modify the key name to
comply with stricter GSettings key name rules. Of course, that means your
application must use the new key names when looking up settings in
GSettings.

The last group in the example also shows how to handle the case of
'relocatable' schemas, which don't have a fixed path. You can specify the
path to use in the group name, separated by a colon.

There are some limitations: `gsettings-data-convert` does not do any
transformation of the values. And it does not handle complex GConf types
other than lists of strings or integers.

**Don't forget to require GConf 2.31.1 or newer in your configure script if
you are making use of the GConf backend or the conversion utility.**

If, as an application developer, you are interested in manually ensuring
that `gsettings-data-convert` has been invoked (for example, to deal with the
case where the user is logged in during a distribution upgrade or for
non-XDG desktop environments which do not run the command as an autostart)
you may invoke it manually during your program initialisation. This is not
recommended for all application authors -- it is your choice if this use
case concerns you enough.

Internally, `gsettings-data-convert` uses a keyfile to track which settings
have been migrated. The following code fragment will check that keyfile to
see if your data conversion script has been run yet and, if not, will
attempt to invoke the tool to run it. You should adapt it to your
application as you see fit.

```c
static void
ensure_migrated (const gchar *name)
{
  gboolean needed = TRUE;
  GKeyFile *kf;
  gchar **list;
  gsize i, n;

  kf = g_key_file_new ();

  g_key_file_load_from_data_dirs (kf, "gsettings-data-convert",
                                  NULL, G_KEY_FILE_NONE, NULL);
  list = g_key_file_get_string_list (kf, "State", "converted", &n, NULL);

  if (list)
    {
      for (i = 0; i < n; i++)
        if (strcmp (list[i], name) == 0)
          {
            needed = FALSE;
            break;
          }

      g_strfreev (list);
    }

  g_key_file_free (kf);

  if (needed)
    g_spawn_command_line_sync ("gsettings-data-convert",
                               NULL, NULL, NULL, NULL);
}
```

Although there is the possibility that the `gsettings-data-convert` script
will end up running multiple times concurrently with this approach, it is
believed that this is safe.
