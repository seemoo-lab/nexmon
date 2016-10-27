#!/bin/bash
printf "#include \"%s/patches/include/firmware_version.h\"\n%s\n" $NEXMON_ROOT $1 | gcc -E -x c - | tail -n 1
