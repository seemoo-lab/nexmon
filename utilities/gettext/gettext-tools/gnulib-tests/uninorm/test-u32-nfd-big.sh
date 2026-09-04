#!/bin/sh
exec ${CHECKER} ./test-u32-nfd-big${EXEEXT} "$srcdir/uninorm/NormalizationTest.txt"
