#!/bin/bash
# SPDX-License-Identifier: GPL-2.0-or-later
#
# Check if PNGs in a commit can be compressed.
#
# Manual: invoke with a commit ID, e.g. `tools/pre-commit HEAD~`; relative paths
# work if you start inside the tree.

COMMIT_IDS=HEAD
[ $# -gt 0 ] && COMMIT_IDS="$*"

UNAME=$(uname -a)

case "$UNAME" in
    *\ Msys)
        pyvar="pythonw.exe"
        ;;
    *)
        pyvar="python3"
        ;;
esac

PYBIN="${WS_GITHOOK_PYTHON:-$pyvar}"

# Establish absolute tools directory
TOPDIR="$(git rev-parse --show-toplevel)"
TOOLS_DIR="${TOPDIR}/tools"

# Always start in the root directory of the source tree, this allows for
# invocations via relative paths (such as ../tools/pre-commit):
if ! cd "$TOPDIR"; then
    echo "Can't change to the top-level source directory."
    exit 1
fi

compress_pngs="${TOOLS_DIR}/compress-pngs.py"
exit_status=0

for COMMIT_ID in $COMMIT_IDS; do
    PNG_FILES=()
    while read -r line; do
        PNG_FILES+=("$line");
    done < <(git diff-index --cached --name-status "$COMMIT_ID" | grep -v "^D" | cut -f2 | grep "\\.png$")

    if [ -n "${PNG_FILES}" ]; then
        "$PYBIN" "$compress_pngs" "${PNG_FILES[@]}"
    fi

    git diff --exit-code || exit_status=1
done

exit $exit_status
