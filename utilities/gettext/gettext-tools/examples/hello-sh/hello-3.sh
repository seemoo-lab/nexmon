#!/bin/sh
# Example for use of GNU gettext.
# This file is in the public domain.
#
# Source code of the POSIX sh program that uses the 'printf_gettext' program.

TEXTDOMAIN=hello-sh
export TEXTDOMAIN
TEXTDOMAINDIR='@localedir@'
export TEXTDOMAINDIR

gettext "Hello, world!"; echo

pid=$$
printf_gettext 'This program is running as process number %u.' $pid; echo
