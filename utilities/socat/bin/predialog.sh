#! /bin/bash
# source: predialog.sh
# Copyright Gerhard Rieger
# Published under the GNU General Public License V.2, see file COPYING

# This is an example script that shows how to write a script for use with socat
# intermediate addresses. it shows a case where an initial dialog on the right
# side is performed. afterwards data is just passed in both directions.

# uncomment this if you want to analyse which file descriptors are open
#filan -s -o+2; sleep 1; echo

# these are the "right side" file descriptors provided by socat; 0 and 1 are
# the "left side" FDs
RINFD=3
ROUTFD=4

if [ -z "$SOCAT" ]; then
    if type socat2 >/dev/null 2>&1; then
	SOCAT=socat2
    else
	SOCAT="./socat"
    fi
fi

verbose=
# parse options
SPACES=" "
while [ -n "$1" ]; do
    case "$1" in
    -v) verbose=1 ;;
    *) echo "$0: unknown option \"$1\"" >&2; exit -1 ;;
    esac
    shift
done

msg () {
    [ "$verbose" ] && echo "$0: $1" >&2
}

# send a request
msg "sending request"
echo -e "CONNECT 10.0.0.1:80 HTTP/1.0\n" >&4

# read reply
msg "waiting for reply"
read -r <&3
case "$REPLY" in
    "HTTP/1.0 200 OK") ;;
    *) msg "bad reply \"$REPLY\"" exit 1 ;;
esac
# skip headers until empty line
msg "skipping reply headers"
while read -r <&3 && ! [ "$REPLY" = "" ];  do :;  done

wait

msg "starting data transfer"
# now just pass traffic in both directions
#SOCAT_OPTS="-lu -d -d -d -d"
exec $SOCAT $SOCAT_OPTS - "fd:$ROUTFD:$RINFD"
