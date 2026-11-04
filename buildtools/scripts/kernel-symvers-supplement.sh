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
KB=$(CDPATH='' cd -- "$KB" 2>/dev/null && pwd -P) || exit 0

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

HERE=$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)
# Per-user 0700 cache dir instead of a predictable world-writable /tmp path.
CACHE_BASE=${XDG_CACHE_HOME:-"${HOME:-${TMPDIR:-/tmp}/nexmon-cache-$(id -u)}/.cache"}
CACHE="$CACHE_BASE/nexmon"
mkdir -p "$CACHE" && chmod 700 "$CACHE" || { echo "kernel-symvers: cannot create cache $CACHE" >&2; exit 1; }

hash_file() {
	if command -v sha256sum >/dev/null 2>&1; then
		sha256sum "$1" | awk '{print $1}'
	else
		cksum "$1" | awk '{print $1 "-" $2}'
	fi
}

hash_stream() {
	if command -v sha256sum >/dev/null 2>&1; then
		sha256sum | awk '{print $1}'
	else
		cksum | awk '{print $1 "-" $2}'
	fi
}

EXPORT_HASH=$(hash_file "$EXPORT_C") || exit 0
if [ -f "$SYMVERS" ]; then
	SYMVERS_HASH=$(hash_file "$SYMVERS") || exit 0
else
	SYMVERS_HASH=missing
fi

# The cache identity includes the canonical input tree and both relevant file
# contents. This prevents reuse across kernel trees or after Module.symvers is
# changed without a helpful timestamp relationship.
CACHE_KEY=$(
	{
		printf '%s\n' "$KB"
		printf '%s\n' "$EXPORT_HASH"
		printf '%s\n' "$SYMVERS_HASH"
	} | hash_stream
) || exit 0
OUT="$CACHE/symvers-$CACHE_KEY"

if [ ! -s "$OUT" ]; then
	umask 077
	TEMP_OUT=$(mktemp "$CACHE/.symvers-$CACHE_KEY.XXXXXX") || { echo "kernel-symvers: mktemp failed in $CACHE" >&2; exit 1; }
	trap 'rm -f "$TEMP_OUT"' EXIT HUP INT TERM
	if python3 "$HERE/recover_builtin_symvers.py" "$KB" "$TEMP_OUT" >/dev/null 2>&1 && [ -s "$TEMP_OUT" ]; then
		chmod 600 "$TEMP_OUT" 2>/dev/null || true
		mv -f "$TEMP_OUT" "$OUT" || { echo "kernel-symvers: mv failed" >&2; exit 1; }
	fi
	rm -f "$TEMP_OUT"
	trap - EXIT HUP INT TERM
fi

[ -s "$OUT" ] && printf '%s\n' "$OUT"
