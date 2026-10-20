#!/usr/bin/env bash
# Copyright 2016-2018 Simon McVittie
# Copyright 2018-2024 Collabora Ltd.
# SPDX-License-Identifier: LGPL-2.1-or-later

set -eu

skip_all () {
    echo "1..0 # SKIP $*"
    exit 0
}

main () {
    local need_git=
    local tool

    cd "$G_TEST_SRCDIR/.."
    echo "TAP version 13"

    # shellcheck disable=SC2046
    for tool in ${TEST_REQUIRES_TOOLS-}; do
        command -v "$tool" >/dev/null || skip_all "$tool not found"
        if [ "$tool" = git ]; then
            need_git=1
        fi
    done

    if [ -n "${need_git-}" ] && ! test -e .git; then
        skip_all "not a git checkout"
    fi

    echo "1..1"

    if run_lint >&2; then
        echo "ok 1"
        exit 0
    elif [ -n "${LINT_WARNINGS_ARE_ERRORS-}" ]; then
        echo "not ok 1 - warnings from ${TEST_NAME-"lint tool"}"
        exit 1
    else
        echo "not ok 1 # TO""DO warnings from ${TEST_NAME-"lint tool"}"
        exit 0
    fi
}

main
