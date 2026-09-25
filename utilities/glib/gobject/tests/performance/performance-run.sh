#!/bin/bash

# Copyright (C) 2024 Red Hat, Inc.
#
# SPDX-License-Identifier: LGPL-2.1-or-later

# Run ./build/gobject/tests/performance/performance for several commits
# and compare the result.
#
# Howto:
#
# 1) configure the build. For example run
#     $ git clean -fdx
#     $ meson build -Dprefix=/tmp/glib/ -Db_lto=true --buildtype release -Ddebug=false
#    Beware, that running the script will check out other commits,
#    build the tree and run `ninja install`. Don't have important
#    work there.
#
#    Consider setting `echo performance | tee /sys/devices/system/cpu/cpu*/cpufreq/scaling_governor`
#    and check `watch cat /sys/devices/system/cpu/cpu[0-9]*/cpufreq/scaling_cur_freq`.
#
# 2) run the script. Set $COMMITS to the list of commit sha's to test.
#    Environment variables:
#      COMMITS: list of git references to test.
#      PATCH: if set, after checking out each commit, run `git cherry-pick $PATCH`
#        before building.
#      PREPARE_CMD: if set, invoke $PREPARE_CMD after checking out commit. Can be used
#        to patch the sources.
#      PERF: if set, run performance test via perf. Set for example, `PERF="perf stat -r 10 -B"`
#        When setting `PERF`, you probably also want to set GLIB_PERFORMANCE_FACTOR,
#        which depends on the test (run the test in verbose mode to find a suitable factor).
#      STRIP: if set to 1, call `strip` on the library and binary before running.
#   Arguments: arguments are directly passed to performance. For example try "-s 1".
#
# Example:
#
#    # once:
#    git clean -fdx
#    meson build -Dprefix=/tmp/glib/ -Db_lto=true --buildtype release -Ddebug=false
#
#    # test:
#    COMMIT_END=my-test-branch
#    COMMIT_START="$(git merge-base origin/main "$COMMIT_END")"
#    PERF="" PATCH="" COMMITS=" $COMMIT_START $COMMIT_END " /tmp/performance-run.sh -q -s 5
#    PERF="" PATCH="" COMMITS=" $COMMIT_START $( git log --reverse --pretty=%H "$COMMIT_START..$COMMIT_END" ) " /tmp/performance-run.sh -q -s 5
#
#    GLIB_PERFORMANCE_FACTOR=1 PERF='perf stat -r 3 -B' PATCH="" COMMITS=" $COMMIT_START $COMMIT_END " /tmp/performance-run.sh -q -s 1 property-get

set -e

usage() {
    sed -n '/^# Run /,/^$/ s/^#\( \(.*\)\|\)$/\2/p' "$0"
}

if [[ "$#" -eq 1 && ( "$1" == "-h" || "$1" == "--help" ) ]]; then
    usage
    exit 0
fi

die() {
    printf "%s\n" "$*"
    exit 1
}

die_with_last_cmd() {
    cat /tmp/glib-performance-last-cmd || :
    die "$@"
}

read -ra COMMITS_V -d '' <<<"$COMMITS" || :

[ "${#COMMITS_V[@]}" -gt 0 ] || die "Must set \$COMMITS"

COMMITS_SHA=()
for commit in "${COMMITS_V[@]}" ; do
    c="$(git rev-parse --verify "$commit^{commit}")" || die "invalid git reference \"$commit\""
    COMMITS_SHA+=( "$c" )
done

_list_commits() {
    local PREFIX="$1"
    local idx
    local I

    I=0
    for idx in "${!COMMITS_V[@]}" ; do
        I=$((I+1))
        echo "${PREFIX}[$I] $(git log -n1 --oneline "${COMMITS_SHA[$idx]}") (${COMMITS_V[$idx]})"
    done
}

cat << EOF

Testing commits:

$(_list_commits "    ")

  \$ meson build -Dprefix=/tmp/glib/ -Db_lto=true --buildtype release -Ddebug=false
  \$ PATCH=$(printf '%q' "$PATCH") \\
    PREPARE_CMD=$(printf '%q' "$PREPARE_CMD") \\
    PERF=$(printf '%q' "$PERF") \\
    GLIB_PERFORMANCE_FACTOR=$(printf '%q' "$GLIB_PERFORMANCE_FACTOR") \\
    STRIP=$(printf '%q' "$STRIP") \\
    COMMITS=$(printf '%q' "$COMMITS") \\
       $0$(printf ' %q' "$@")

EOF

_get_timing() {
    local LINE_NUM="$1"
    local LINE_PREFIX="$2"
    local FILE="$3"

    sed -n "$LINE_NUM s/^$LINE_PREFIX: //p" "$FILE"
}

_perc() {
    awk -v n1="$1" -v n2="$2" 'BEGIN { printf("%+0.3g%\n", (n2 - n1) / n1 * 100.0); }'
}

_cmd() {
    # printf '   $' ; printf ' %s' "$@" ; printf '\n'
    "$@"
}


I=0
for idx in "${!COMMITS_V[@]}" ; do
    commit="${COMMITS_V[$idx]}"
    commit_sha="${COMMITS_SHA[$idx]}"
    I=$((I+1))
    echo -n ">>> [$I] test [ $(git log -n1 --oneline "$commit") ]"
    git checkout -f "$commit_sha" &> /tmp/glib-performance-last-cmd || (echo ...; die_with_last_cmd "Failure to checkout \"$commit\"")
    touch glib/gdataset.c gobject/gobject.c gobject/tests/performance/performance.c
    if [ "$PATCH" != "" ] ; then
        read -ra PATCH_ARR -d '' <<<"$PATCH" || :
        for p in "${PATCH_ARR[@]}" ; do
            git cherry-pick "$p" &> /tmp/glib-performance-last-cmd || (echo ...; die_with_last_cmd "Failure to cherry-pick \"$PATCH\"")
        done
    fi
    if [ "$PREPARE_CMD" != "" ] ; then
        I="$I" COMMIT="$commit" "$PREPARE_CMD" &> /tmp/glib-performance-last-cmd || (echo ...; die_with_last_cmd "\$PREPARE_CMD failed")
    fi
    echo " commit $(git rev-parse --verify HEAD) > /tmp/glib-performance-output.$I"
    ( ninja -C build || ninja -C build ) &> /tmp/glib-performance-last-cmd || die_with_last_cmd "Failure to build with \`ninja -C build\`"
    ninja -C build install &> /tmp/glib-performance-last-cmd || die_with_last_cmd "FAilure to install with \`ninja -C build install\`"
    if [ "$STRIP" = 1 ] ; then
        strip ./build/gobject/libgobject-2.0.so ./build/glib/libglib-2.0.so.0 ./build/gobject/tests/performance/performance
    fi

    # ls -lad -L \
    #     ./build/gobject/libgobject-2.0.so \
    #     ./build/glib/libglib-2.0.so \
    #     ./build/gobject/tests/performance/performance
    # ldd ./build/gobject/tests/performance/performance

    sleep 2
    (
        if [ -z "$PERF" ] ; then
            TESTRUN="$I" TESTCOMMIT="$commit" ./build/gobject/tests/performance/performance "$@"
        else
            TESTRUN="$I" TESTCOMMIT="$commit" $PERF ./build/gobject/tests/performance/performance "$@"
        fi
    ) |& tee "/tmp/glib-performance-output.$I" || :
    [ "${PIPESTATUS[0]}" -eq 0 ] || die "Performance test failed"
done

merge_output() {
    (
        declare -A RES
        LINE_NUM=0
        OLD_IFS="$IFS"
        while IFS=$'\n' read -r LINE ; do
            LINE="${LINE:1}"
            LINE_NUM=$((LINE_NUM+1))
            line_regex='^([a-zA-Z0-9].*): +([0-9.]+)$'

            if [[ "$LINE" =~ $line_regex ]] ; then
                LINE_PREFIX="${BASH_REMATCH[1]}"
                T1="${BASH_REMATCH[2]}"
                echo -n "$LINE_PREFIX: $T1"
                RES["$LINE_PREFIX"]="${RES[$LINE_PREFIX]} $T1"
                for J in $(seq 2 "${#COMMITS_V[@]}") ; do
                    T="$(_get_timing "$LINE_NUM" "$LINE_PREFIX" "/tmp/glib-performance-output.$J")"
                    echo -n "	$T ($(_perc "$T1" "$T"))"
                    RES["$LINE_PREFIX"]="${RES[$LINE_PREFIX]} $T"
                done
                echo
                continue
            fi
            if [ -n "$PERF" ] ; then
                if [[ "$LINE" == *"seconds time elapsed"* ]] ; then
                    echo "[1] $LINE"
                    for J in $(seq 2 "${#COMMITS_V[@]}") ; do
                        echo "[$J] $(sed -n "$LINE_NUM s/seconds time elapsed/\\0/p" "/tmp/glib-performance-output.$J")"
                    done
                    continue
                fi
            fi
            echo "$LINE"
        done < <( sed 's/^/x/' "/tmp/glib-performance-output.1" )

        IFS="$OLD_IFS"
        if [ -n "$PERF" ] ; then
            for LINE_PREFIX in "${!RES[@]}"; do
                # The values are interleaved. To cumbersome to process in bash.
                read -ra T -d '' <<<"${RES[$LINE_PREFIX]}" || :
                read -ra T_SORTED -d '' < <(printf '%s\n' "${T[@]}" | awk -v C="${#COMMITS_V[@]}" '{ print (1 + (NR-1) % C) " " $0 }' | sort -n -s | sed 's/^[0-9]\+ *//' ) || :
                NGROUP="$(( "${#T_SORTED[@]}" / "${#COMMITS_V[@]}" ))"
                echo -n "$LINE_PREFIX: "
                SEP=''
                for J in $(seq 0 "${#T_SORTED[@]}") ; do
                    echo -n "$SEP${T_SORTED[$J]}"
                    if [ $(( (J+1) % NGROUP )) = 0 ]; then
                        echo -n "  ;  "
                        SEP=''
                    else
                        SEP=" , "
                    fi
                done
                echo
            done
        fi
    ) | tee "/tmp/glib-performance-output.all"
}

echo ">>> combined result > /tmp/glib-performance-output.all"
merge_output
