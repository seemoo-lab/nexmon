.. _glib-gettextize(1):
.. meta::
   :copyright: Copyright 2003 Matthias Clasen
   :copyright: Copyright 2012 Red Hat, Inc.
   :license: LGPL-2.1-or-later
..
   This has to be duplicated from above to make it machine-readable by `reuse`:
   SPDX-FileCopyrightText: 2003 Matthias Clasen
   SPDX-FileCopyrightText: 2012 Red Hat, Inc.
   SPDX-License-Identifier: LGPL-2.1-or-later

===============
glib-gettextize
===============

------------------------------------
gettext internationalization utility
------------------------------------

SYNOPSIS
--------

|  **glib-gettextize** [*OPTION*…] [*DIRECTORY*]

DESCRIPTION
-----------

``glib-gettextize`` helps to prepare a source package for being
internationalized through `gettext <https://www.gnu.org/software/gettext/>`_.
It is a variant of the ``gettextize`` that ships with gettext.

``glib-gettextize`` differs from ``gettextize`` in that it doesn’t create an
``intl/`` subdirectory and doesn’t modify ``po/ChangeLog`` (note that newer
versions of ``gettextize`` behave like this when called with the
``--no-changelog`` option).

OPTIONS
-------

``--help``

  Print help and exit.

``--version``

  Print version information and exit.

``-c``, ``--copy``

  Copy files instead of making symlinks.

``-f``, ``--force``

  Force writing of new files even if old ones exist.

SEE ALSO
--------

`gettextize(1) <man:gettextize(1)>`_