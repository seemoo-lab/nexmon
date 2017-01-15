#! /bin/bash
# source: proxy.sh
# Copyright Gerhard Rieger 2003-2004
# Published under the GNU General Public License V.2, see file COPYING

# perform primitive simulation of a proxy server.
# accepts and answers correct HTTP CONNECT requests on stdio, and tries to
# establish the connection to the given server.
# it is required for socats test.sh
# for TCP, use this script as:
# socat tcp-l:8080,reuseaddr,fork exec:"proxy.sh",nofork

if [ -z "$SOCAT" ]; then
    if type socat >/dev/null 2>&1; then
	SOCAT=socat
    else
	SOCAT="./socat"
    fi
fi

if   [ $(echo "x\c") = "x" ]; then E=""
elif [ $(echo -e "x\c") = "x" ]; then E="-e"
else
    echo "cannot suppress trailing newline on echo" >&2
    exit 1
fi
ECHO="echo $E"
CR=$($ECHO "\r")
#echo "CR=$($ECHO "$CR\c" |od -c)" >&2

case `uname` in
HP-UX|OSF1)
    # their cats are too stupid to work with unix domain sockets
    CAT="$SOCAT -u stdin stdout"
    ;;
*)
    CAT=cat
    ;;
esac

SPACES=" "
while [ -n "$1" ]; do
    case "$1" in
    -w) n="$2"; while [ "$n" -gt 0 ]; do SPACES="$SPACES "; n=$((n-1)); done
	shift ;;
    #-s) STAT="$2"; shift ;;
    esac
    shift
done

# read and parse HTTP request
read l
if echo "$l" |egrep '^CONNECT +[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+:[0-9]+ +HTTP/1.[01]' >/dev/null
then
    : go on below
else
    $ECHO "HTTP/1.0${SPACES}500 Bad Request$CR"
    $ECHO "$CR"
    exit
fi

# extract target server name/address
s=`echo $l |awk '{print($2);}'`

# read more headers until empty line
while [ "$l" != "$CR" ]; do
    read l
done

# send status
$ECHO "HTTP/1.0${SPACES}200 OK$CR"
# send empty line
$ECHO "$CR"

# perform proxy (relay) function
exec $SOCAT $SOCAT_OPTS - tcp:$s
