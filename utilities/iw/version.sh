#!/bin/sh

VERSION="6.17"
OUT="$1"

# get the absolute path for the OUT file
OUT_NAME=$(basename ${OUT})
OUT_DIR=$(cd $(dirname ${OUT}); pwd)
OUT="${OUT_DIR}/${OUT_NAME}"

# the version check should be under the source directory
# where this script is located, instead of the currect directory
# where this script is excuted.
SRC_DIR=$(dirname $0)
SRC_DIR=$(cd ${SRC_DIR}; pwd)
cd "${SRC_DIR}"

v=""
if [ -d .git ] && head=`git rev-parse --verify HEAD 2>/dev/null`; then
    git update-index --refresh --unmerged > /dev/null
    descr=$(git describe --match=v* 2>/dev/null)
    if [ $? -eq 0 ]; then
        # on git builds check that the version number above
        # is correct...
        if [ "${descr%%-*}" = "v$VERSION" ]; then
            v="${descr#v}"
            if git diff-index --name-only HEAD | read dummy ; then
                v="$v"-dirty
            fi
        fi
    fi
fi

# set to the default version when failed to get the version
# information with git
if [ -z "${v}" ]; then
    v="$VERSION"
fi

echo '#include "iw.h"' > "$OUT"
echo "const char iw_version[] = \"$v\";" >> "$OUT"
