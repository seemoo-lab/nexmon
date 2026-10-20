.. _gio-querymodules(1):
.. meta::
   :copyright: Copyright 2010, 2012 Red Hat, Inc.
   :license: LGPL-2.1-or-later
..
   This has to be duplicated from above to make it machine-readable by `reuse`:
   SPDX-FileCopyrightText: 2010, 2012 Red Hat, Inc.
   SPDX-License-Identifier: LGPL-2.1-or-later

================
gio-querymodules
================

-------------------------
GIO module cache creation
-------------------------

SYNOPSIS
--------

|  **gio-querymodules** *DIRECTORY*…

DESCRIPTION
-----------

``gio-querymodules`` creates a ``giomodule.cache`` file in the listed
directories. This file lists the implemented extension points for each module
that has been found. It is used by GIO at runtime to avoid opening all modules
just to find out which extension points they are implementing.

GIO modules are usually installed in the ``gio/modules`` subdirectory of libdir.