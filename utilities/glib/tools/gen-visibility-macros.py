#!/usr/bin/env python3
#
# Copyright © 2022 Collabora Inc.
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Original author: Xavier Claessens <xclaesse@gmail.com>

import argparse
import textwrap
from pathlib import Path


# Disable line length warnings as wrapping the C code templates would be hard
# flake8: noqa: E501


def gen_versions_macros(args, current_minor_version):
    with args.out_path.open("w", encoding="utf-8") as ofile, args.in_path.open(
        "r", encoding="utf-8"
    ) as ifile:
        for line in ifile.readlines():
            if "@GLIB_VERSIONS@" in line:
                for minor in range(2, current_minor_version + 2, 2):
                    ofile.write(
                        textwrap.dedent(
                            f"""\
                        /**
                        * GLIB_VERSION_2_{minor}:
                        *
                        * A macro that evaluates to the 2.{minor} version of GLib, in a format
                        * that can be used by the C pre-processor.
                        *
                        * Since: 2.{max(minor, 32)}
                        */
                        #define GLIB_VERSION_2_{minor}       (G_ENCODE_VERSION (2, {minor}))
                        """
                        )
                    )
            else:
                ofile.write(line)


def gen_doc_sections(args, current_minor_version):
    with args.out_path.open("w", encoding="utf-8") as ofile, args.in_path.open(
        "r", encoding="utf-8"
    ) as ifile:
        for line in ifile.readlines():
            if "@GLIB_VERSIONS@" in line:
                for minor in range(2, current_minor_version + 2, 2):
                    ofile.write(
                        textwrap.dedent(
                            f"""\
                        GLIB_VERSION_2_{minor}
                        """
                        )
                    )
            else:
                ofile.write(line)


def gen_visibility_macros(args, current_minor_version):
    """
    Generates a set of macros for each minor stable version of GLib

    - GLIB_VAR

    - GLIB_DEPRECATED
    - GLIB_DEPRECATED_IN_…
    - GLIB_DEPRECATED_MACRO_IN_…
    - GLIB_DEPRECATED_ENUMERATOR_IN_…
    - GLIB_DEPRECATED_TYPE_IN_…

    - GLIB_AVAILABLE_IN_ALL
    - GLIB_AVAILABLE_IN_…
    - GLIB_AVAILABLE_STATIC_INLINE_IN_…
    - GLIB_AVAILABLE_MACRO_IN_…
    - GLIB_AVAILABLE_ENUMERATOR_IN_…
    - GLIB_AVAILABLE_TYPE_IN_…

    - GLIB_UNAVAILABLE(maj,min)
    - GLIB_UNAVAILABLE_STATIC_INLINE(maj,min)

    The GLIB namespace can be replaced with one of GOBJECT, GIO, GMODULE, GI.
    """

    ns = args.namespace
    with args.out_path.open("w", encoding="utf-8") as f:
        f.write(
            textwrap.dedent(
                f"""\
            #pragma once

            #if (defined(_WIN32) || defined(__CYGWIN__)) && !defined({ns}_STATIC_COMPILATION)
            #  define _{ns}_EXPORT __declspec(dllexport)
            #  define _{ns}_IMPORT __declspec(dllimport)
            #elif __GNUC__ >= 4
            #  define _{ns}_EXPORT __attribute__((visibility("default")))
            #  define _{ns}_IMPORT
            #else
            #  define _{ns}_EXPORT
            #  define _{ns}_IMPORT
            #endif
            #ifdef {ns}_COMPILATION
            #  define _{ns}_API _{ns}_EXPORT
            #else
            #  define _{ns}_API _{ns}_IMPORT
            #endif

            #define _{ns}_EXTERN _{ns}_API extern

            #define {ns}_VAR _{ns}_EXTERN
            #define {ns}_AVAILABLE_IN_ALL _{ns}_EXTERN

            #ifdef GLIB_DISABLE_DEPRECATION_WARNINGS
            #define {ns}_DEPRECATED _{ns}_EXTERN
            #define {ns}_DEPRECATED_FOR(f) _{ns}_EXTERN
            #define {ns}_UNAVAILABLE(maj,min) _{ns}_EXTERN
            #define {ns}_UNAVAILABLE_STATIC_INLINE(maj,min)
            #else
            #define {ns}_DEPRECATED G_DEPRECATED _{ns}_EXTERN
            #define {ns}_DEPRECATED_FOR(f) G_DEPRECATED_FOR(f) _{ns}_EXTERN
            #define {ns}_UNAVAILABLE(maj,min) G_UNAVAILABLE(maj,min) _{ns}_EXTERN
            #define {ns}_UNAVAILABLE_STATIC_INLINE(maj,min) G_UNAVAILABLE(maj,min)
            #endif
            """
            )
        )
        for minor in range(26, current_minor_version + 2, 2):
            f.write(
                textwrap.dedent(
                    f"""
                #if GLIB_VERSION_MIN_REQUIRED >= GLIB_VERSION_2_{minor}
                #define {ns}_DEPRECATED_IN_2_{minor} {ns}_DEPRECATED
                #define {ns}_DEPRECATED_IN_2_{minor}_FOR(f) {ns}_DEPRECATED_FOR (f)
                #define {ns}_DEPRECATED_MACRO_IN_2_{minor} GLIB_DEPRECATED_MACRO
                #define {ns}_DEPRECATED_MACRO_IN_2_{minor}_FOR(f) GLIB_DEPRECATED_MACRO_FOR (f)
                #define {ns}_DEPRECATED_ENUMERATOR_IN_2_{minor} GLIB_DEPRECATED_ENUMERATOR
                #define {ns}_DEPRECATED_ENUMERATOR_IN_2_{minor}_FOR(f) GLIB_DEPRECATED_ENUMERATOR_FOR (f)
                #define {ns}_DEPRECATED_TYPE_IN_2_{minor} GLIB_DEPRECATED_TYPE
                #define {ns}_DEPRECATED_TYPE_IN_2_{minor}_FOR(f) GLIB_DEPRECATED_TYPE_FOR (f)
                #else
                #define {ns}_DEPRECATED_IN_2_{minor} _{ns}_EXTERN
                #define {ns}_DEPRECATED_IN_2_{minor}_FOR(f) _{ns}_EXTERN
                #define {ns}_DEPRECATED_MACRO_IN_2_{minor}
                #define {ns}_DEPRECATED_MACRO_IN_2_{minor}_FOR(f)
                #define {ns}_DEPRECATED_ENUMERATOR_IN_2_{minor}
                #define {ns}_DEPRECATED_ENUMERATOR_IN_2_{minor}_FOR(f)
                #define {ns}_DEPRECATED_TYPE_IN_2_{minor}
                #define {ns}_DEPRECATED_TYPE_IN_2_{minor}_FOR(f)
                #endif

                #if GLIB_VERSION_MAX_ALLOWED < GLIB_VERSION_2_{minor}
                #define {ns}_AVAILABLE_IN_2_{minor} {ns}_UNAVAILABLE (2, {minor})
                #define {ns}_AVAILABLE_STATIC_INLINE_IN_2_{minor} GLIB_UNAVAILABLE_STATIC_INLINE (2, {minor})
                #define {ns}_AVAILABLE_MACRO_IN_2_{minor} GLIB_UNAVAILABLE_MACRO (2, {minor})
                #define {ns}_AVAILABLE_ENUMERATOR_IN_2_{minor} GLIB_UNAVAILABLE_ENUMERATOR (2, {minor})
                #define {ns}_AVAILABLE_TYPE_IN_2_{minor} GLIB_UNAVAILABLE_TYPE (2, {minor})
                #else
                #define {ns}_AVAILABLE_IN_2_{minor} _{ns}_EXTERN
                #define {ns}_AVAILABLE_STATIC_INLINE_IN_2_{minor}
                #define {ns}_AVAILABLE_MACRO_IN_2_{minor}
                #define {ns}_AVAILABLE_ENUMERATOR_IN_2_{minor}
                #define {ns}_AVAILABLE_TYPE_IN_2_{minor}
                #endif
                """
                )
            )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("glib_version", help="Current GLib version")
    subparsers = parser.add_subparsers()

    versions_parser = subparsers.add_parser(
        "versions-macros", help="Generate versions macros"
    )
    versions_parser.add_argument("in_path", help="input file", type=Path)
    versions_parser.add_argument("out_path", help="output file", type=Path)
    versions_parser.set_defaults(func=gen_versions_macros)

    doc_parser = subparsers.add_parser(
        "doc-sections", help="Generate glib-sections.txt"
    )
    doc_parser.add_argument("in_path", help="input file", type=Path)
    doc_parser.add_argument("out_path", help="output file", type=Path)
    doc_parser.set_defaults(func=gen_doc_sections)

    visibility_parser = subparsers.add_parser(
        "visibility-macros", help="Generate visibility macros"
    )
    visibility_parser.add_argument("namespace", help="Macro namespace")
    visibility_parser.add_argument("out_path", help="output file", type=Path)
    visibility_parser.set_defaults(func=gen_visibility_macros)

    args = parser.parse_args()
    version = [int(i) for i in args.glib_version.split(".")]
    assert version[0] == 2
    args.func(args, version[1])


if __name__ == "__main__":
    main()
