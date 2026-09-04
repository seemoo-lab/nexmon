.. _gio(1):
.. meta::
   :copyright: Copyright 2024 Collabora Ltd.
   :license: LGPL-2.1-or-later
..
   This has to be duplicated from above to make it machine-readable by `reuse`:
   SPDX-FileCopyrightText: 2024 Collabora Ltd.
   SPDX-License-Identifier: LGPL-2.1-or-later

==================
gi-inspect-typelib
==================

-----------------------
Typelib inspection tool
-----------------------

SYNOPSIS
--------

| **gi-inspect-typelib** [*OPTION*\ …] **--print-shlibs** *NAMESPACE*
| **gi-inspect-typelib** [*OPTION*\ …] **--print-typelibs** *NAMESPACE*

DESCRIPTION
-----------

**gi-inspect-typelib** displays information about GObject-Introspection
binary typelib files.

OPTIONS
-------

``--print-shlibs``
    Show the shared libraries that implement the *NAMESPACE*.

``--print-typelibs``
    Show the other typelibs that the *NAMESPACE* depends on.

``--typelib-version``
    The version of each *NAMESPACE* to inspect.
    For example, the version of ``Gio-2.0.typelib`` is ``2.0``.
    If not specified, use the newest available version if there is more
    than one installed.

EXAMPLE
-------

On Linux, the ``Gio-2.0`` typelib is implemented by ``libgio-2.0.so.0``::

    $ gi-inspect-typelib --typelib-version 2.0 --print-shlibs Gio
    shlib: libgio-2.0.so.0

and it depends on GObject-2.0, GLib-2.0 and GModule-2.0::

    $ gi-inspect-typelib --typelib-version 2.0 --print-typelibs Gio
    typelib: GObject-2.0
    typelib: GLib-2.0
    typelib: GModule-2.0
