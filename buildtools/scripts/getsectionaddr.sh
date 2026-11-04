#!/bin/bash
set -euo pipefail
if [ -z "${CC:-}" ]; then CC="objdump"; fi
if [ $# -ne 2 ]; then echo "getsectionaddr.sh: usage: $0 <section> <elf>" >&2; exit 1; fi
# Use CC's objdump if CC is gcc, else try ${CC%gcc}objdump or plain objdump
OBJDUMP="${CC%gcc}objdump"
if ! command -v "$OBJDUMP" >/dev/null 2>&1; then
    OBJDUMP="objdump"
fi
ADDR=$("$OBJDUMP" -h "$2" 2>/dev/null | awk -v sec="$1" '$2 == sec { print $4 }' | head -n 1)
if [ -n "$ADDR" ]; then
    printf "%s\n" "$ADDR"
fi
