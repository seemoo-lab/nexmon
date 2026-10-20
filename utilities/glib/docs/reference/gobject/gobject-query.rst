.. _gobject-query(1):
.. meta::
   :copyright: Copyright 2003 Matthias Clasen
   :copyright: Copyright 2012 Red Hat, Inc.
   :license: LGPL-2.1-or-later
..
   This has to be duplicated from above to make it machine-readable by `reuse`:
   SPDX-FileCopyrightText: 2003 Matthias Clasen
   SPDX-FileCopyrightText: 2012 Red Hat, Inc.
   SPDX-License-Identifier: LGPL-2.1-or-later

=============
gobject-query
=============

-----------------------
display a tree of types
-----------------------

SYNOPSIS
--------

|  **gobject-query** froots [*OPTION*…]
|  **gobject-query** tree [*OPTION*…]

DESCRIPTION
-----------

``gobject-query`` is a small utility that draws a tree of types.

It takes a mandatory argument that specifies whether it should iterate over the
fundamental types or print a type tree.

COMMANDS
--------

``froots``

  Iterate over fundamental roots.

``tree``

  Print type tree.

OPTIONS
-------

``-r <TYPE>``

  Specify the root type.

``-n``

  Don’t descend type tree.

``-b <STRING>``

  Specify indent string.

``-i <STRING>``

  Specify incremental indent string.

``-s <NUMBER>``

  Specify line spacing.

``-h``, ``--help``

  Print brief help and exit.

``-v``, ``--version``

  Print version and exit.