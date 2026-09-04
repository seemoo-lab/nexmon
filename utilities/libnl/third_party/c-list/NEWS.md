# c-list - Circular Intrusive Double Linked List Collection

## CHANGES WITH 3.1.0:

        * The minimum required meson version is now 0.60.0.

        * New function c_list_split() is added. It reverses c_list_splice()
          and thus allows to split a list in half.

        Contributions from: David Rheinsberg, Michele Dionisio

        - Brno, 2022-06-22

## CHANGES WITH 3:

        * API break: The c_list_loop_*() symbols were removed, since we saw
                     little use for them. No user was known at the time, so
                     all projects should build with the new API version
                     unchanged.
                     Since c-list does not distribute any compiled code, there
                     is no ABI issue with this change.

        * Two new symbols c_list_length() and c_list_contains(). They are meant
          for debugging purposes, to easily verify list integrity. Since they
          run in O(n) time, they are not recommended for any other use than
          debugging.

        * New symbol c_list_init() is provided as alternative to the verbose
          C_LIST_INIT assignment.

        * The c-list API is extended to work well with `const CList` objects.
          That is, any read-only accessor function allows constant objects as
          input now.
          Note that this does not propagate into other members linked in the
          list. Using `const` for CList members is of little practical use.
          However, it might be of use for its embedding objects, so we now
          allow it in the CList API as well.

        * The c_list_splice() call now clears the source list, rather than
          returning with stale pointers. Technically, this is also an API
          break, but unlikely to affect any existing code.

        Contributions from: David Herrmann, Thomas Haller

        - Berlin, 2017-08-13

## CHANGES WITH 2:

        * Adjust project-name in build-system to reflect the actual project. The
          previous releases incorrectly claimed to be c-rbtree in the build
          system.

        * Add c_list_swap() that swaps two lists given their head pointers.

        * Add c_list_splice() that moves a list.

        * Add LGPL2.1+ as license so c-list can be imported into GPL2 projects.
          It is now officially dual-licensed.

        * As usual a bunch of fixes, additional tests, and documentation
          updates.

        Contributions from: David Herrmann, Tom Gundersen

        - Lund, 2017-05-03

## CHANGES WITH 1:

        * Initial release of c-list.

        * This project provides an implementation of a circular double linked
          list in standard ISO-C11. License is ASL-2.0 and the build system
          used is `Meson'.

        Contributions from: David Herrmann, Tom Gundersen

        - Berlin, 2017-03-03
