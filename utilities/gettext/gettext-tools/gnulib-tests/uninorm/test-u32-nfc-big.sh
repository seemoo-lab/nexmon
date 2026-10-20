#!/bin/sh
exec ${CHECKER} ./test-u32-nfc-big${EXEEXT} "$srcdir/uninorm/NormalizationTest.txt"
