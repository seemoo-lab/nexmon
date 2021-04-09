#! /bin/sh

# Add a .gdb_index section to a file.

# Copyright (C) 2010-2019 Free Software Foundation, Inc.
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

# This program assumes gdb and objcopy are in $PATH.
# If not, or you want others, pass the following in the environment
GDB=${GDB:=gdb}
OBJCOPY=${OBJCOPY:=objcopy}

myname="${0##*/}"

dwarf5=""
if [ "$1" = "-dwarf-5" ]; then
    dwarf5="$1"
    shift
fi

if test $# != 1; then
    echo "usage: $myname [-dwarf-5] FILE" 1>&2
    exit 1
fi

file="$1"

if test ! -r "$file"; then
    echo "$myname: unable to access: $file" 1>&2
    exit 1
fi

dir="${file%/*}"
test "$dir" = "$file" && dir="."
index4="${file}.gdb-index"
index5="${file}.debug_names"
debugstr="${file}.debug_str"
debugstrmerge="${file}.debug_str.merge"
debugstrerr="${file}.debug_str.err"

rm -f $index4 $index5 $debugstr $debugstrmerge $debugstrerr
# Ensure intermediate index file is removed when we exit.
trap "rm -f $index4 $index5 $debugstr $debugstrmerge $debugstrerr" 0

$GDB --batch -nx -iex 'set auto-load no' \
    -ex "file $file" -ex "save gdb-index $dwarf5 $dir" || {
    # Just in case.
    status=$?
    echo "$myname: gdb error generating index for $file" 1>&2
    exit $status
}

# In some situations gdb can exit without creating an index.  This is
# not an error.
# E.g., if $file is stripped.  This behaviour is akin to stripping an
# already stripped binary, it's a no-op.
status=0

if test -f "$index4" -a -f "$index5"; then
    echo "$myname: Both index types were created for $file" 1>&2
    status=1
elif test -f "$index4" -o -f "$index5"; then
    if test -f "$index4"; then
	index="$index4"
	section=".gdb_index"
    else
	index="$index5"
	section=".debug_names"
    fi
    debugstradd=false
    debugstrupdate=false
    if test -s "$debugstr"; then
	if ! $OBJCOPY --dump-section .debug_str="$debugstrmerge" "$file" /dev/null \
		 2>$debugstrerr; then
	    cat >&2 $debugstrerr
	    exit 1
	fi
	if grep -q "can't dump section '.debug_str' - it does not exist" \
		  $debugstrerr; then
	    debugstradd=true
	else
	    debugstrupdate=true
	    cat >&2 $debugstrerr
	fi
	cat "$debugstr" >>"$debugstrmerge"
    fi

    $OBJCOPY --add-section $section="$index" \
	--set-section-flags $section=readonly \
	$(if $debugstradd; then \
	      echo --add-section .debug_str="$debugstrmerge"; \
	      echo --set-section-flags .debug_str=readonly; \
	  fi; \
	  if $debugstrupdate; then \
	      echo --update-section .debug_str="$debugstrmerge"; \
	  fi) \
	"$file" "$file"

    status=$?
else
    echo "$myname: No index was created for $file" 1>&2
    echo "$myname: [Was there no debuginfo? Was there already an index?]" 1>&2
fi

exit $status
