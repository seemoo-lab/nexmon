#!/bin/sh

set -e

echo "## SGML catalog regression tests"

if [ -n "$1" ]; then
    xmlcatalog=$1
else
    xmlcatalog=./xmlcatalog
fi

exitcode=0

for i in test/catalogs/*.script ; do
    name=$(basename $i .script)
    sgml="./test/catalogs/$name.sgml"

    if [ -f $sgml ] ; then
        if [ ! -f result/catalogs/$name ] ; then
            echo New test file $name
            $xmlcatalog --shell $sgml < $i > result/catalogs/$name
        else
            $xmlcatalog --shell $sgml < $i > catalog_sgml.out
            log=$(diff result/catalogs/$name catalog_sgml.out)
            if [ -n "$log" ] ; then
                echo $name result
                echo "$log"
                exitcode=1
            fi
            rm catalog_sgml.out
        fi
    fi
done

exit $exitcode
