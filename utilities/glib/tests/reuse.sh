#!/usr/bin/env bash
#
# Copyright 2022 Endless OS Foundation, LLC
# Copyright 2024 Collabora Ltd.
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Original author: Philip Withnall

set -eu

if [ -z "${G_TEST_SRCDIR-}" ]; then
    me="$(readlink -f "$0")"
    G_TEST_SRCDIR="${me%/*}"
fi

skip_all () {
    echo "1..0 # SKIP $*"
    exit 0
}

cd "$G_TEST_SRCDIR/.."
echo "TAP version 13"

command -v git >/dev/null || skip_all "git not found"
command -v reuse >/dev/null || skip_all "reuse not found"
test -e .git || skip_all "not a git checkout"

echo "1..1"

# We need to make sure the submodules are up to date, or `reuse lint` will fail
# when it tries to run `git status` internally
git submodule update --init >&2

# Run `reuse lint` on the code base and see if the number of files without
# suitable copyright/licensing information has increased from a baseline
# FIXME: Eventually this script can check whether *any* files are missing
# information. But for now, let’s slowly improve the baseline.
files_without_copyright_information_max=336
files_without_license_information_max=386

# The || true is because `reuse lint` will exit with status 1 if the project is not compliant
# FIXME: Once https://github.com/fsfe/reuse-tool/issues/512 or
# https://github.com/fsfe/reuse-tool/issues/183 land, we can check only files
# which have changed in this merge request, and confidently parse structured
# output rather than the current human-readable output.
lint_output="$(reuse lint || true)"

files_with_copyright_information="$(echo "${lint_output}" | awk '/^\* [fF]iles with copyright information: / { print $6 }')"
files_with_license_information="$(echo "${lint_output}" | awk '/^\* [fF]iles with license information: / { print $6 }')"
total_files="$(echo "${lint_output}" | awk '/^\* [fF]iles with copyright information: / { print $8 }')"
error=0

files_without_copyright_information="$(( total_files - files_with_copyright_information ))"
files_without_license_information="$(( total_files - files_with_license_information ))"

if [ "${files_without_copyright_information}" -gt "${files_without_copyright_information_max}" ] || \
   [ "${files_without_license_information}" -gt "${files_without_license_information_max}" ]; then
  echo "${lint_output}" >&2
fi

if [ "${files_without_copyright_information}" -gt "${files_without_copyright_information_max}" ]; then
  echo "" >&2
  echo "Error: New files added without REUSE-compliant copyright information" >&2
  echo "Please make sure that all files added in this branch/merge request have correct copyright information" >&2
  error=1
fi

if [ "${files_without_license_information}" -gt "${files_without_license_information_max}" ]; then
  echo "" >&2
  echo "Error: New files added without REUSE-compliant licensing information" >&2
  echo "Please make sure that all files added in this branch/merge request have correct license information" >&2
  error=1
fi

if [ "${error}" -eq "1" ]; then
  echo "" >&2
  echo "See https://reuse.software/tutorial/#step-2 for information on how to add REUSE information" >&2
  echo "Also see https://gitlab.gnome.org/GNOME/glib/-/issues/1415" >&2
fi

if [ "${error}" -eq 0 ]; then
    echo "ok 1"
    exit 0
elif [ -n "${LINT_WARNINGS_ARE_ERRORS-}" ]; then
    echo "not ok 1 - warnings from reuse"
    exit "${error}"
else
    echo "not ok 1 # TO""DO warnings from reuse"
    exit 0
fi
