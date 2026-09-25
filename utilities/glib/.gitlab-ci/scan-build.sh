#!/usr/bin/env bash
#
# Copyright 2024 GNOME Foundation, Inc.
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# Original author: Philip Withnall

set -eu

# This script just exists so that we can set the scan-build flags in
# .gitlab-ci.yml and pass them into Meson’s `scan-build` target.

# shellcheck disable=SC2086
exec scan-build ${SCAN_BUILD_FLAGS:-} "$@"