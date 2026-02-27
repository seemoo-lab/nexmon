#!/bin/sh
# Usage: mmsmallpo.sh hello-foo ll

set -e

test $# = 2  || { echo "Usage: mmsmallpo.sh hello-foo ll" 1>&2; exit 1; }
directory=$1
language=$2

msgmerge --quiet --force-po $language.po $directory.pot -o - | \
msgattrib --no-obsolete | \
sed -e "s, $directory/, ,g" | sed -e "s,gettext-examples,$directory," | \
sed -e '/^"POT-Creation-Date: .*"$/{
x
s/P/P/
ta
g
d
bb
:a
x
:b
}' \
  > ../$directory/po/$language.po
