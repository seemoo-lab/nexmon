#!/bin/sh

set -e

echo "## Scripts regression tests"

if [ -n "$1" ]; then
    xmllint=$1
else
    xmllint=./xmllint
fi

exitcode=0

for i in test/scripts/*.script ; do
    name=$(basename $i .script)
    xml="./test/scripts/$name.xml"

    if [ -f $xml ] ; then
        if [ ! -f result/scripts/$name ] ; then
            echo "New test file $name"

            $xmllint --shell $xml < $i \
                > result/scripts/$name \
                2> result/scripts/$name.err
        else
            $xmllint --shell $xml < $i > shell.out 2> shell.err || true

            if [ -f result/scripts/$name.err ]; then
                resulterr="result/scripts/$name.err"
            else
                resulterr=/dev/null
            fi

            log=$(
                diff -u result/scripts/$name shell.out || true;
                diff -u $resulterr shell.err || true
            )

            if [ -n "$log" ] ; then
                echo $name result
                echo "$log"
                exitcode=1
            fi

            rm shell.out shell.err
        fi
    fi
done

exit $exitcode
