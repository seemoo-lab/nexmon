#! /bin/bash
# source: cat2.sh
# Copyright Gerhard Rieger 2009
# Published under the GNU General Public License V.2, see file COPYING

# This is an example script that shows how to write a script for use with socat
# intermediate addresses. it shows a simple case consisting of two
# unidirectional programs. 
# note how the n>&- and n<&- controls are used to close FDs on sub processes
# to make half close possible.

# uncomment this if you want to analyse which file descriptors are open
#filan -s -o+2; sleep 1; echo

# these are the "right side" file descriptors provided by socat; 0 and 1 are
# the "left side" FDs
RINFD=3
ROUTFD=4

if true; then

    # this is a typical example.
    #these work fine
    socat -u - - <&3 3<&- 4>&- &	# right (3) to left (1)
    socat -u - - <&0 >&4 4>&- 3<&- &	# left (0) to right (4)
    #strace -o /tmp/cat.strace -v -tt -f -F -x -s 1024 cat <&3 3<&- 4>&- &		# right (3) to left (1)
    #cat <&0 1>&- >&4 4>&- 3<&-	&	# left (0) to right (4)
    exec 1>&- 4>&-
    exec 0<&- 3<&-

elif false; then

    # works except close in reverse direction
    cat <&3 3<&- 4>&- &	# right (3) to left (1)
    exec cat <&0 1>&- >&4 4>&- 3<&-	# left (0) to right (4)

elif true; then

    # works - forw, rev
    #socat $SOCAT_OPTS -u fd:$RINFD  -  </dev/null 4>&- &	# right to left
    #socat $SOCAT_OPTS -u - fd:$ROUTFD  >/dev/null 3<&- &	# left to right
    exec 1>&- 4>&-
    exec 0<&- 3<&-

elif false; then

    # works - forw, rev
    socat -u - - <&3 3<&- 4>&- &	# right (3) to left (1)
    exec socat -u - - <&0 1>&- >&4 4>&- 3<&-	# left (0) to right (4)

else

    # works  except close in reverse
    #filan -s -o+2 <&3 3<&- 4>&- &	# right (3) to left (1)
     cat           <&3 3<&- 4>&- &	# right (3) to left (1)
    #socat -d -d -d -d -u - -  <&3 3<&- 4>&- &	# right (3) to left (1)

    #sleep 1; echo >&2;  filan -s -o+2 <&0 >&4 4>&- 3<&- &	# left (0) to right (4)
    #cat          <&0 >&4 4>&- 3<&- &	# left (0) to right (4)
     socat -u - - <&0 >&4 4>&- 3<&- &	# left (0) to right (4)

    exec 1>&- 4>&-
    exec 0<&- 3<&-
    #sleep 1; echo >&2;  filan -s -o+2

fi

wait
