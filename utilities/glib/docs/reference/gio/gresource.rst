.. _gresource(1):
.. meta::
   :copyright: Copyright 2012 Red Hat, Inc.
   :license: LGPL-2.1-or-later
..
   This has to be duplicated from above to make it machine-readable by `reuse`:
   SPDX-FileCopyrightText: 2012 Red Hat, Inc.
   SPDX-License-Identifier: LGPL-2.1-or-later

=========
gresource
=========

--------------
GResource tool
--------------

SYNOPSIS
--------

|  **gresource** [--section *SECTION*] list *FILE* [*PATH*]
|  **gresource** [--section *SECTION*] details *FILE* [*PATH*]
|  **gresource** [--section *SECTION*] extract *FILE* *PATH*
|  **gresource** sections *FILE*
|  **gresource** help [*COMMAND*]

DESCRIPTION
-----------

``gresource`` offers a simple commandline interface to ``GResource``. It lets
you list and extract resources that have been compiled into a resource file or
included in an ELF file (a binary or a shared library).

The file to operate on is specified by the ``FILE`` argument.

If an ELF file includes multiple sections with resources, it is possible to
select which one to operate on with the ``--section`` option. Use the
``sections`` command to find available sections.

COMMANDS
--------

``list``

  Lists resources. If ``SECTION`` is given, only list resources in this section.
  If ``PATH`` is given, only list matching resources.

``details``

  Lists resources with details. If ``SECTION`` is given, only list resources in
  this section. If ``PATH`` is given, only list matching resources. Details
  include the section, size and compression of each resource.

``extract``

  Extracts the resource named by ``PATH`` to stdout. Note that resources may
  contain binary data.

``sections``

  Lists sections containing resources. This is only interesting if ``FILE`` is
  an ELF file.

``help``

  Prints help and exits.