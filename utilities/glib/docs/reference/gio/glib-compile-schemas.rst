.. _glib-compile-schemas(1):
.. meta::
   :copyright: Copyright 2010, 2011, 2012, 2015 Red Hat, Inc.
   :copyright: Copyright 2012 Allison Karlitskaya
   :copyright: Copyright 2016 Sam Thursfield
   :license: LGPL-2.1-or-later
..
   This has to be duplicated from above to make it machine-readable by `reuse`:
   SPDX-FileCopyrightText: 2010, 2011, 2012, 2015 Red Hat, Inc.
   SPDX-FileCopyrightText: 2012 Allison Karlitskaya
   SPDX-FileCopyrightText: 2016 Sam Thursfield
   SPDX-License-Identifier: LGPL-2.1-or-later

====================
glib-compile-schemas
====================

-------------------------
GSettings schema compiler
-------------------------

SYNOPSIS
--------

|  **glib-compile-schemas** [*OPTION*…] *DIRECTORY*

DESCRIPTION
-----------

``glib-compile-schemas`` compiles all the GSettings XML schema files in
``DIRECTORY`` into a binary file with the name ``gschemas.compiled`` that can be
used by ``GSettings``. The XML schema files must have the filename extension
``.gschema.xml``. For a detailed description of the XML file format, see the
``GSettings`` documentation.

At runtime, GSettings looks for schemas in the ``glib-2.0/schemas``
subdirectories of all directories specified in the ``XDG_DATA_DIRS`` and
``XDG_DATA_HOME`` environment variables. The usual location to install schema
files is ``/usr/share/glib-2.0/schemas``.

In addition to schema files, ``glib-compile-schemas`` reads ‘vendor override’
files, which are key files that can override default values for keys in
the schemas. The group names in the key files are the schema ID, and the
values are written in serialized GVariant form.
Vendor override files must have the filename extension
``.gschema.override``.

By convention, vendor override files begin with ``nn_`` where ``nn`` is a number
from 00 to 99.  Higher numbered files have higher priority (e.g. if the same
override is made in a file numbered 10 and then again in a file numbered 20, the
override from 20 will take precedence).

OPTIONS
-------

``-h``, ``--help``

  Print help and exit.

``--version``

  Print program version and exit.

``--targetdir <TARGET>``

  Store ``gschemas.compiled`` in the ``TARGET`` directory instead of
  ``DIRECTORY``.

``--strict``

  Abort on any errors in schemas. Without this option, faulty schema files are
  simply omitted from the resulting compiled schema.

``--dry-run``

  Don’t write ``gschemas.compiled``. This option can be used to check
  ``.gschema.xml`` sources for errors.

``--allow-any-name``

  Do not enforce restrictions on key names. Note that this option is purely
  to facility the transition from GConf, and will be removed at some time
  in the future.