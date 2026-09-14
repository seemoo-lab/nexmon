.. _gi-decompile-typelib(1):
.. meta::
   :copyright: Copyright 2008, 2010 Johan Dahlin
   :copyright: Copyright 2014 Robert Roth
   :copyright: Copyright 2015 Dieter Verfaillie
   :copyright: Copyright 2018 Tomasz Miąsko
   :copyright: Copyright 2018 Christoph Reiter
   :copyright: Copyright 2020 Jan Tojnar
   :copyright: Copyright 2024 Collabora Ltd.
   :license: LGPL-2.1-or-later
..
   This has to be duplicated from above to make it machine-readable by `reuse`:
   SPDX-FileCopyrightText: 2008, 2010 Johan Dahlin
   SPDX-FileCopyrightText: 2014 Robert Roth
   SPDX-FileCopyrightText: 2015 Dieter Verfaillie
   SPDX-FileCopyrightText: 2018 Tomasz Miąsko
   SPDX-FileCopyrightText: 2018 Christoph Reiter
   SPDX-FileCopyrightText: 2020 Jan Tojnar
   SPDX-FileCopyrightText: 2024 Collabora Ltd.
   SPDX-License-Identifier: LGPL-2.1-or-later

====================
gi-decompile-typelib
====================

------------------
Typelib decompiler
------------------

:Manual section: 1


SYNOPSIS
========

**gi-decompile-typelib** [*OPTION*…] *TYPELIB* [*TYPELIB*\ …]


DESCRIPTION
===========

gi-decompile-typelib is a GIR decompiler, using the repository API.
It generates GIR XML files from the compiled binary typelib format.
The output will be written to standard output unless the ``--output``
is specified.

The binary typelib format stores a subset of the information available
in GIR XML, so not all typelibs can be decompiled in this way, and the
resulting GIR XML might be incomplete.

Normally, GIR XML should be generated from source code, headers and
shared libraries using `g-ir-scanner(1) <man:g-ir-scanner(1)>`_
instead of using this tool.


OPTIONS
=======

``--help``
    Show help options.

``--output`` *FILENAME*, ``-o`` *FILENAME*
    Save the resulting output in *FILENAME*.

``--includedir`` *DIRECTORY*
    Add *DIRECTORY* to the search path for typelibs.
    This option can be used more than once.
    The first *DIRECTORY* on the command-line will be searched first
    (highest precedence).

``--all``
    Show all available information.

``--version``
    Show program’s version number and exit.


EXAMPLE
=======

::
    $ libdir=/usr/lib/x86_64-linux-gnu     # or /usr/lib64 or similar
    $ gi-decompile-typelib -o Gio-2.0.gir \
      $libdir/girepository-1.0/Gio-2.0.typelib

    $ diff -u /usr/share/gir-1.0/Gio-2.0.gir Gio-2.0.gir

You will see that the original GIR XML contains much more information
than the decompiled typelib.


BUGS
====

Report bugs at https://gitlab.gnome.org/GNOME/glib/-/issues


HOMEPAGE and CONTACT
====================

https://gi.readthedocs.io/


AUTHORS
=======

Matthias Clasen
