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

export TEST_NAME=flake8
export TEST_REQUIRES_TOOLS="flake8 git"

run_lint () {
    # Disable formatting warnings in flake8, as we use `black` to handle that.
    local formatting_warnings=E101,E111,E114,E115,E116,E117,E12,E13,E2,E3,E401,E5,E70,W1,W2,W3,W5

    # shellcheck disable=SC2046
    flake8 \
        --max-line-length=88 --ignore="$formatting_warnings" \
        $(git ls-files '*.py' 'tests/lib/*.py.in')
}

# shellcheck source=tests/lint-common.sh
. "$G_TEST_SRCDIR/lint-common.sh"
