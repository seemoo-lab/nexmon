#!/bin/sh

echo "## Catalog regression tests"

if [ -n "$1" ]; then
    xmlcatalog=$1
else
    xmlcatalog=./xmlcatalog
fi

exitcode=0

# Test xmlcatalog --shell command line
# Case 1: Really long argument (470 chars)
input=""; for i in {1..470}; do input="${input}A"; done
echo $input | $xmlcatalog --shell test/catalogs/dockbook.xml || exit 1
# Case 2: public + long argument
input="public "; for i in {1..470}; do input="${input}A"; done
echo $input | $xmlcatalog --shell test/catalogs/dockbook.xml || exit 1
# Case 3: public + lots of args
input="public "; for i in {1..80}; do input="${input} x"; done
echo $input | $xmlcatalog --shell test/catalogs/dockbook.xml || exit 1

for i in test/catalogs/*.script ; do
    name=$(basename $i .script)
    xml="./test/catalogs/$name.xml"

    if [ -f $xml ] ; then
        if [ ! -f result/catalogs/$name ] ; then
            echo New test file $name
            $xmlcatalog --shell $xml < $i 2>&1 > result/catalogs/$name
        else
            $xmlcatalog --shell $xml < $i 2>&1 > catalog.out
            log=$(diff result/catalogs/$name catalog.out)
            if [ -n "$log" ] ; then
                echo $name result
                echo "$log"
                exitcode=1
            fi
            rm catalog.out
        fi
    fi
done

# Add and del operations on XML Catalogs

$xmlcatalog --create --noout mycatalog
$xmlcatalog --noout --add public Pubid sysid mycatalog
$xmlcatalog --noout --add public Pubid2 sysid2 mycatalog
$xmlcatalog --noout --add public Pubid3 sysid3 mycatalog
diff result/catalogs/mycatalog.full mycatalog
$xmlcatalog --noout --del sysid mycatalog
$xmlcatalog --noout --del sysid3 mycatalog
$xmlcatalog --noout --del sysid2 mycatalog
diff result/catalogs/mycatalog.empty mycatalog
rm -f mycatalog

exit $exitcode
