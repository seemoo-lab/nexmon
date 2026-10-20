#!/usr/bin/env bash
#
# Copyright 2022 Canonical Limited
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Original author: Marco Trevisan

if [ ! -d "$1" ]; then
    echo "Usage $(basename "$0") UCD-directory [version]"
    exit 1
fi

ucd=$(realpath "$1")
version=$2
glib_dir=$(git -C "$(dirname "$0")" rev-parse --show-toplevel)

# shellcheck disable=SC2144 # we only want to match a file like this
if ! [ -f "$ucd"/UnicodeData*.txt ] || ! [ -f "$ucd"/CaseFolding.*txt ]; then
    echo "'$ucd' does not look like an Unicode Database directory";
fi

if [ -z "$version" ]; then
    readme=("$ucd"/ReadMe*.txt)
    version=$(sed -n "s,.*Version \([0-9.]\+\) of the Unicode Standard.*,\1,p" \
        "${readme[@]}")

    if [ -z "$version" ]; then
        echo "Invalid version found"
        exit 1
    fi
fi

cd "$glib_dir" || exit 1

echo "Updating generated code to Unicode version $version"
set -xe

(cd glib && ./gen-unicode-tables.pl -both "$version" "$ucd")
glib/tests/gen-casefold-txt.py "$version" \
    "$ucd"/CaseFolding*.txt > glib/tests/casefold.txt
glib/tests/gen-casemap-txt.py "$version" \
    "$ucd"/UnicodeData*.txt \
    "$ucd"/SpecialCasing*.txt > glib/tests/casemap.txt
cp "$ucd"/NormalizationTest.txt glib/tests/
