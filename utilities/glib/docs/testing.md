Testing policy
===

Aims
---

 * Maintainers should be able to make a release of GLib at any time, confident
   that it will not contain regressions or obvious bugs with new functionality
 * Speed up review of submitted changes by deferring some of the review effort
   to automated testing
 * Allow fast detection of bugs in new or changed code, particularly if they are
   only present on platforms not regularly used by the maintainers
 * Allow easy dynamic and static analysis of a significant proportion of the
   GLib code
 * Statistics on tests (such as pass/failure) should be easily and mechanically
   collectable to allow analysis and highlight problems
 * Code for tests and code for production should be easily separable so that
   statistics on them can be grouped separately
 * Performance measurement tools for GLib should be reusable over time to allow
   comparable measurements to be collected and to discourage use of lower
   quality and throwaway tests when prototyping improvements to GLib

Policy
---

 * Tests must be written for all new code, and any existing code which is being
   non-trivially modified (for example to fix a bug), to give confidence to the
   author and reviewer of the changes that they are correct for all platforms
   that GLib runs CI on.
 * Tests live in the `{glib,gobject,gio}/tests` directories. This allows their
   code to be counted separately when analysing statistics such as code
   coverage.
   - Performance tests live in `{glib,gobject,gio}/tests/performance`, as they
     are executed and results interpreted differently due to giving a result on
     a continuous scale rather than a pass/fail result.
 * All tests must use the GTest framework, as it supports
   [structured output](https://testanything.org/) which exposes test results to
   the test runner for analysis.
   - Use `g_test_bug()` and `g_test_summary()` in unit tests to link them to
     contextual information in bug reports, and to provide a summary of what
     each test checks and how it goes about doing those checks. Sometimes a
     test’s behaviour can be quite complex, and needs to be explained so that
     future developers can understand and build on such tests in future.
   - Use the `g_assert_*()` functions inside unit tests, and do not use
     `g_assert()`. The latter is compiled out when GLib is built with
     `G_DISABLE_ASSERT`, and the former are not. The `g_assert_*()` functions
     also give more helpful error messages on test failure.
 * Performance tests must be able to be run unattended. In this mode they must
   choose default argument values which check that the performance test
   functions (i.e. without crashing) and doesn’t take too long to complete. This
   is used to automatically verify that performance tests still work, as they
   are typically used infrequently and are subject to bitrot.
 * Code coverage reports must be used to demonstrate that unit tests reach all
   newly submitted or significantly modified code, reaching all lines of code
   and a significant majority of branches. If this is not enforced, code ends up
   never being tested.
 * Code should be structured to be testable, which is typically only possible by
   writing tests at the same time as the code. Otherwise it is easy to design
   APIs which cannot easily be unit tested, and once those APIs are stable it is
   hard to retrofit tests to them.
 * Parsers, network-facing code or code which handles untrusted user input must
   have fuzz tests added, in the `fuzzing` directory. These are run by
   [oss-fuzz](https://github.com/google/oss-fuzz/) and are very effective at
   catching exploitable security issues. See the
   [fuzzing README](../fuzzing/README.md) for more details.
 * When fixing bugs in existing code, regression tests must be added when it is
   straightforward to do so. If it’s difficult to do so (such as if the code
   needs to be significantly restructured or APIs need to be changed), adding
   the regression tests can be deferred to a follow-up issue so as not to slow
   down bug fixing. In that case, the bug fix must be carefully manually tested
   before being merged.
