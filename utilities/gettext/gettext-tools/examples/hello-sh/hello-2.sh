#!/bin/sh
# Example for use of GNU gettext.
# This file is in the public domain.
#
# Source code of the POSIX sh program that uses a POSIX:2024 compliant 'printf'.

TEXTDOMAIN=hello-sh
export TEXTDOMAIN
TEXTDOMAINDIR='@localedir@'
export TEXTDOMAINDIR

gettext "Hello, world!"; echo

pid=$$
env printf "`gettext \"This program is running as process number %u.\"`"'\n' $pid
