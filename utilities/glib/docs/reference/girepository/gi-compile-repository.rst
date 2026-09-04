.. _gi-compile-repository(1):
.. meta::
   :copyright: Copyright 2010 Johan Dahlin
   :copyright: Copyright 2015 Ben Boeckel
   :copyright: Copyright 2013, 2015 Dieter Verfaillie
   :copyright: Copyright 2018 Emmanuele Bassi
   :copyright: Copyright 2018 Tomasz Miąsko
   :copyright: Copyright 2018 Christoph Reiter
   :copyright: Copyright 2020 Jan Tojnar
   :copyright: Copyright 2024 Collabora Ltd.
   :license: LGPL-2.1-or-later
..
   This has to be duplicated from above to make it machine-readable by `reuse`:
   SPDX-FileCopyrightText: 2010 Johan Dahlin
   SPDX-FileCopyrightText: 2015 Ben Boeckel
   SPDX-FileCopyrightText: 2013, 2015 Dieter Verfaillie
   SPDX-FileCopyrightText: 2018 Emmanuele Bassi
   SPDX-FileCopyrightText: 2018 Tomasz Miąsko
   SPDX-FileCopyrightText: 2018 Christoph Reiter
   SPDX-FileCopyrightText: 2020 Jan Tojnar
   SPDX-FileCopyrightText: 2024 Collabora Ltd.
   SPDX-License-Identifier: LGPL-2.1-or-later

=====================
gi-compile-repository
=====================

----------------
Typelib compiler
----------------

:Manual section: 1


SYNOPSIS
========

**gi-compile-repository** [*OPTION*…] *GIRFILE*


DESCRIPTION
===========

gi-compile-repository converts one or more GIR files into one or more typelibs.
The output will be written to standard output unless the ``--output`` is
specified.


OPTIONS
=======

``--help``
    Show help options.

``--output`` *FILENAME*, ``-o`` *FILENAME*
    Save the resulting output in *FILENAME*.

``--verbose``
    Show verbose messages.

``--debug``
    Show debug messages.

``--includedir`` *DIRECTORY*
    Add *DIRECTORY* to the search path for GIR XML.
    This option can be used more than once.
    The first *DIRECTORY* on the command-line will be searched first
    (highest precedence).

``--shared-library`` *FILENAME*, ``-l`` *FILENAME*
    Specifies the shared library where the symbols in the typelib can be
    found. The name of the library should not contain the ending shared
    library suffix.
    This option can be used more than once, for typelibs that describe
    more than one shared library.

``--version``
    Show program’s version number and exit.


EXAMPLE
=======

::
    $ gi-compile-repository -o Gio-2.0.typelib /usr/share/gir-1.0/Gio-2.0.gir


BUGS
====

Report bugs at https://gitlab.gnome.org/GNOME/glib/-/issues


HOMEPAGE and CONTACT
====================

https://gi.readthedocs.io/


AUTHORS
=======

Matthias Clasen
