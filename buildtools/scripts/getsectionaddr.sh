#!/bin/bash
${CC}objdump -j "$1" -t "$2" | awk -v sec="$1" '$0 ~ sec { print $1; exit }'
