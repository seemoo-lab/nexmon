#!/bin/sh
# Emit a KBUILD_EXTRA_SYMBOLS file for a kernel tree whose Module.symvers is
# missing the built-in exports, or nothing at all when the tree is healthy.
#
# Some kernel trees ship a Module.symvers that only carries the exports of
# modules built during that build, not the vmlinux built-ins. modpost then
# fails an out-of-tree module with hundreds of "undefined" symbols, starting
# with module_layout - which every module needs - even though the module
# compiles cleanly. The symbols are still present in the kernel's own
# .vmlinux.export.c, so they can be recovered without touching the tree.
#
# Usage: kernel-symvers-supplement.sh <kernel-build-dir>
# Prints the path to a supplemental symvers file, or nothing.
set -u

KB="${1:-}"
[ -n "$KB" ] || exit 0
[ -d "$KB" ] || exit 0

SYMVERS="$KB/Module.symvers"
EXPORT_C="$KB/.vmlinux.export.c"

# Healthy tree: nothing to do. module_layout is the canary because every module
# references it, so if it is present the built-in exports made it into the file.
if [ -f "$SYMVERS" ] && grep -qE "^[0-9a-fx]+[[:space:]]+module_layout[[:space:]]" "$SYMVERS"; then
	exit 0
fi

# No export artifact to recover from: let modpost fail with its own message
# rather than masking the real problem.
[ -f "$EXPORT_C" ] || exit 0

HERE=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
OUT="/tmp/nexmon-symvers-$(uname -r)"

# Reuse a previously generated table when it is newer than the kernel artifact.
if [ -f "$OUT" ] && [ "$OUT" -nt "$EXPORT_C" ]; then
	echo "$OUT"
	exit 0
fi

if python3 "$HERE/recover_builtin_symvers.py" "$KB" "$OUT" >/dev/null 2>&1 && [ -s "$OUT" ]; then
	echo "$OUT"
fi
