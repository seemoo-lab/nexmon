.. _gtester(1):
.. meta::
   :copyright: Copyright 2008 Matthias Clasen
   :copyright: Copyright 2011 Collabora, Ltd.
   :copyright: Copyright 2011 Carlos Garcia Campos
   :copyright: Copyright 2012 Red Hat, Inc.
   :copyright: Copyright 2019 Endless Mobile, Inc.
   :license: LGPL-2.1-or-later
..
   This has to be duplicated from above to make it machine-readable by `reuse`:
   SPDX-FileCopyrightText: 2008 Matthias Clasen
   SPDX-FileCopyrightText: 2011 Collabora, Ltd.
   SPDX-FileCopyrightText: 2011 Carlos Garcia Campos
   SPDX-FileCopyrightText: 2012 Red Hat, Inc.
   SPDX-FileCopyrightText: 2019 Endless Mobile, Inc.
   SPDX-License-Identifier: LGPL-2.1-or-later

=======
gtester
=======

--------------------
test running utility
--------------------

SYNOPSIS
--------

|  **gtester** [*OPTION*…] *test-program*

DESCRIPTION
-----------

``gtester`` is a utility to run unit tests that have been written using the GLib
test framework.

Since GLib 2.62, ``gtester-report`` is deprecated. Use TAP for reporting test
results instead, and feed it to the test harness provided by your build system.

When called with the ``-o`` option, ``gtester`` writes an XML report of the test
results, which can be converted into HTML using the ``gtester-report`` utility.

OPTIONS
-------

``-h``, ``--help``

  Print help and exit.

``-v``, ``--verbose``

  Print version information and exit.

``--g-fatal-warnings``

  Make warnings fatal.

``-k``, ``--keep-going``

  Continue running after tests failed.

``-l``

  List paths of available test cases.

``-m=<MODE>``

  Run test cases in ``MODE``, which can be one of:

  * ``perf``

    Run performance tests.

  * ``slow``, ``thorough``

    Run slow tests, or repeat non-deterministic tests more often.

  * ``quick``

    Do not run slow or performance tests, or do extra repeats of
    non-deterministic tests (default).

  * ``undefined``

    Run test cases that deliberately provoke checks or assertion failures, if
    implemented (default).

  * ``no-undefined``

    Do not run test cases that deliberately provoke checks or assertion
    failures.

``-p=<TEST-PATH>``

  Only run test cases matching ``TEST-PATH``.

``-s=<TEST-PATH>``

  Skip test cases matching ``TEST-PATH``.

``--seed=<SEED-STRING>``

  Run all test cases with random number seed ``SEED-STRING``.

``-o=<LOG-FILE>``

  Write the test log to ``LOG-FILE``.

``-q``, ``--quiet``

  Suppress per-test-binary output.

``--verbose``

  Report success per testcase.

SEE ALSO
--------

`gtester-report(1) <man:gtester-report(1)>`_