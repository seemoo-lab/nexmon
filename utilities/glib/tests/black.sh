#!/usr/bin/env bash
# Copyright 2020 Frederic Martinsons
# Copyright 2020 Endless OS Foundation, LLC
# Copyright 2024 Collabora Ltd.
# SPDX-License-Identifier: LGPL-2.1-or-later

set -eu

if [ -z "${G_TEST_SRCDIR-}" ]; then
    me="$(readlink -f "$0")"
    G_TEST_SRCDIR="${me%/*}"
fi

export TEST_NAME=black
export TEST_REQUIRES_TOOLS="black git"

run_lint () {
    # shellcheck disable=SC2046
    black --diff --check $(git ls-files '*.py' 'tests/lib/*.py.in')
}

# shellcheck source=tests/lint-common.sh
. "$G_TEST_SRCDIR/lint-common.sh"
