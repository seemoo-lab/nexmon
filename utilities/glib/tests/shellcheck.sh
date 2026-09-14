#!/usr/bin/env bash
# Copyright 2020 Frederic Martinsons
# Copyright 2024 Collabora Ltd.
# SPDX-License-Identifier: LGPL-2.1-or-later

set -eu

if [ -z "${G_TEST_SRCDIR-}" ]; then
    me="$(readlink -f "$0")"
    G_TEST_SRCDIR="${me%/*}"
fi

export TEST_NAME=shellcheck
export TEST_REQUIRES_TOOLS="git shellcheck"

run_lint () {
    # Ignoring third-party directories that we don't want to parse
    # shellcheck disable=SC2046
    shellcheck $(git ls-files '*.sh' 'gio/completion/' | grep -Ev "glib/libcharset|glib/dirent|gitignore")
}

# shellcheck source=tests/lint-common.sh
. "$G_TEST_SRCDIR/lint-common.sh"
