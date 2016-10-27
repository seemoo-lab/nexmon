#!/bin/bash
$CC"objdump" -j $1 -t $2 | sed -e '5q;d' | awk '{ print $1; }'
