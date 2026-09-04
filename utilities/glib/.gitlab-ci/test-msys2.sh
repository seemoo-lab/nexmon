#!/bin/bash

set -ex

pacman --noconfirm -Suy

if [[ "${MSYSTEM}" != "MINGW32" ]]; then
    pacman --noconfirm -S --needed \
        base-devel \
        lcov \
        "${MINGW_PACKAGE_PREFIX}"-ccache \
        "${MINGW_PACKAGE_PREFIX}"-gettext \
        "${MINGW_PACKAGE_PREFIX}"-gi-docgen \
        "${MINGW_PACKAGE_PREFIX}"-gobject-introspection \
        "${MINGW_PACKAGE_PREFIX}"-libffi \
        "${MINGW_PACKAGE_PREFIX}"-meson \
        "${MINGW_PACKAGE_PREFIX}"-pcre2 \
        "${MINGW_PACKAGE_PREFIX}"-python3 \
        "${MINGW_PACKAGE_PREFIX}"-python-docutils \
        "${MINGW_PACKAGE_PREFIX}"-python-pip \
        "${MINGW_PACKAGE_PREFIX}"-toolchain \
        "${MINGW_PACKAGE_PREFIX}"-zlib \
        "${MINGW_PACKAGE_PREFIX}"-libelf \
        "${MINGW_PACKAGE_PREFIX}"-gdb
else
    pacman --noconfirm -S --needed \
        base-devel \
        "${MINGW_PACKAGE_PREFIX}"-ccache \
        "${MINGW_PACKAGE_PREFIX}"-meson \
        "${MINGW_PACKAGE_PREFIX}"-toolchain
fi

mkdir -p _coverage
mkdir -p _ccache
CCACHE_BASEDIR="$(pwd)"
CCACHE_DIR="${CCACHE_BASEDIR}/_ccache"
export CCACHE_BASEDIR CCACHE_DIR

PATH="$(cygpath "$USERPROFILE")/.local/bin:$HOME/.local/bin:$PATH"
DIR="$(pwd)"
export PATH CFLAGS

if [[ -v INTROSPECTION ]]; then
    # If msys2 doesn’t provide a new enough gobject-introspection package, build it
    # ourselves.
    # See https://gitlab.gnome.org/GNOME/glib/-/merge_requests/3746#note_2161354
    if [[ $(vercmp "$(pacman -Qi "${MINGW_PACKAGE_PREFIX}"-gobject-introspection | grep -Po '^Version\s*: \K.+')" "${GOBJECT_INTROSPECTION_TAG}") -lt 0 ]]; then
        mkdir -p gobject-introspection
        git clone --branch "${GOBJECT_INTROSPECTION_TAG}" https://gitlab.gnome.org/GNOME/gobject-introspection.git gobject-introspection
        meson setup gobject-introspection gobject-introspection/build --prefix "${MINGW_PREFIX}"
        meson install -C gobject-introspection/build
    fi
    introspection=enabled
else
    introspection=disabled
fi

if [[ -v DOCUMENTATION ]]; then
    # If msys2 doesn’t provide a new enough gi-docgen package, download the subproject
    GI_DOCGEN_TAG=$(grep "^revision[[:space:]]*=" subprojects/gi-docgen.wrap | tr -d ' ' | cut -d'=' -f2)
    if [[ $(vercmp "$(pacman -Qi "${MINGW_PACKAGE_PREFIX}"-gi-docgen | grep -Po '^Version\s*: \K.+')" "${GI_DOCGEN_TAG}") -lt 0 ]]; then
        meson subprojects download gi-docgen
    fi
    documentation=true
else
    documentation=false
fi

if [[ -v COVERAGE ]]; then
    coverage=true
else
    coverage=false
fi

if [[ "${MSYSTEM}" != "MINGW32" ]]; then
    wrap_mode=nodownload
else
    wrap_mode=forcefallback
fi

# FIXME: We can’t use ${MESON_COMMON_OPTIONS} here because this script installs
# Meson 1.3. See the comment in .gitlab-ci.yml about the same problem on
# FreeBSD.
# shellcheck disable=SC2086
meson setup \
    --buildtype=debug \
    --wrap-mode=$wrap_mode \
    -Dwerror=false \
    -D:werror=true \
    -Db_coverage=$coverage \
    -Ddocumentation=$documentation \
    -Dman-pages=$documentation \
    -Dintrospection=$introspection \
    _build

meson compile -C _build

if [[ -v COVERAGE ]]; then
    lcov \
        --quiet \
        --config-file "${DIR}"/.lcovrc \
        --directory "${DIR}/_build" \
        --capture \
        --initial \
        --output-file "${DIR}/_coverage/${CI_JOB_NAME}-baseline.lcov"
fi

meson test -C _build -v --timeout-multiplier "${MESON_TEST_TIMEOUT_MULTIPLIER}"
meson test -C _build -v --timeout-multiplier "${MESON_TEST_TIMEOUT_MULTIPLIER}" \
    --setup=unstable_tests --suite=failing --suite=flaky || true

if [[ -v COVERAGE ]]; then
    lcov \
        --quiet \
        --config-file "${DIR}"/.lcovrc \
        --directory "${DIR}/_build" \
        --capture \
        --output-file "${DIR}/_coverage/${CI_JOB_NAME}.lcov"
fi

if [[ -v DOCUMENTATION ]]; then
    # Copy the built documentation to an artifact directory. The build for docs.gtk.org
    # can then pull it from there — see https://gitlab.gnome.org/GNOME/gtk/-/blob/docs-gtk-org/README.md
    mkdir -p _reference/
    mv _build/docs/reference/glib/glib-win32-2.0/ _reference/glib-win32/
    mv _build/docs/reference/gio/gio-win32-2.0/ _reference/gio-win32/
fi