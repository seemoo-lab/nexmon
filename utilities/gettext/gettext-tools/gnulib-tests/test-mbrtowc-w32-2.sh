#!/bin/sh

# Test a CP1256 locale.
./test-mbrtowc-w32${EXEEXT} "Arabic_Saudi Arabia" 1256
