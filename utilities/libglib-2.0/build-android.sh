#!/bin/sh
# Cross-build glib 2.89.4 and its dependencies for Android with the NDK,
# producing the static archives checked into this directory:
#
#   libglib-2.0.a   glib core (PCRE2 for GRegex included)
#   libgobject-2.0.a libffi.a  gobject (needs libffi at final link)
#   libgmodule-2.0.a libgio-2.0.a libgthread-2.0.a
#   libintl.a       gettext stub (proxy-libintl, NLS disabled)
#
# Sources: ../glib (glib 2.89.4, meson), ../libffi (autotools), ../pcre2 (autotools).
# The fresh headers are installed into ./glib-2.0/.
#
# Usage: ./build-android.sh [APP_ABI]        (default: arm64-v8a)
#        APP_ABI=all builds only arm64-v8a, the archives' canonical ABI.
#
# Requirements: NDK_ROOT (or an NDK under /opt/android-sdk/ndk), meson,
# ninja, pkg-config.

set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
GLIB_SRC="$HERE/../glib"
FFI_SRC="$HERE/../libffi"
PCRE2_SRC="$HERE/../pcre2"
API=28

ABI="${1:-${APP_ABI:-arm64-v8a}}"
if [ "$ABI" = "all" ]; then
    echo "note: APP_ABI=all builds the arm64-v8a archives (the canonical ABI)"
    ABI=arm64-v8a
fi
case "$ABI" in
  arm64-v8a|arm64|aarch64)
    CLANG_TARGET=aarch64-linux-android$API
    AUTOTOOLS_HOST=aarch64-linux-android
    MESON_CPU_FAMILY=aarch64; MESON_CPU=aarch64 ;;
  armeabi-v7a|armv7|arm|armeabi)
    CLANG_TARGET=armv7a-linux-androideabi$API
    AUTOTOOLS_HOST=arm-linux-androideabi
    MESON_CPU_FAMILY=arm; MESON_CPU=armv7a ;;
  x86)
    CLANG_TARGET=i686-linux-android$API
    AUTOTOOLS_HOST=i686-linux-android
    MESON_CPU_FAMILY=x86; MESON_CPU=i686 ;;
  x86_64|x64)
    CLANG_TARGET=x86_64-linux-android$API
    AUTOTOOLS_HOST=x86_64-linux-android
    MESON_CPU_FAMILY=x86_64; MESON_CPU=x86_64 ;;
  *)
    echo "unsupported APP_ABI: $ABI (supported: arm64-v8a armeabi-v7a x86 x86_64)" >&2
    exit 1 ;;
esac

# --- NDK ---------------------------------------------------------------
if [ -z "${NDK_ROOT:-}" ]; then
    for cand in "$ANDROID_NDK_ROOT" "$ANDROID_NDK_HOME" "$ANDROID_NDK"; do
        [ -n "${cand:-}" ] && [ -d "$cand" ] && NDK_ROOT="$cand" && break
    done
fi
if [ -z "${NDK_ROOT:-}" ] && [ -d /opt/android-sdk/ndk ]; then
    NDK_ROOT=$(find /opt/android-sdk/ndk -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)
fi
if [ -z "${NDK_ROOT:-}" ] && command -v ndk-build >/dev/null 2>&1; then
    NDK_ROOT=$(dirname "$(command -v ndk-build)")
fi
if [ -z "${NDK_ROOT:-}" ]; then
    echo "NDK not found; set NDK_ROOT" >&2
    exit 1
fi

case "$(uname -m)" in
  x86_64)  HOST_TAG=linux-x86_64 ;;
  aarch64) HOST_TAG=linux-aarch64 ;;
  *) echo "unsupported host arch: $(uname -m)" >&2; exit 1 ;;
esac
TC="$NDK_ROOT/toolchains/llvm/prebuilt/$HOST_TAG/bin"
if [ ! -x "$TC/clang" ]; then
    echo "no clang at $TC" >&2
    exit 1
fi

# --- meson/ninja -------------------------------------------------------
MESON=$(command -v meson || true)
[ -n "$MESON" ] || MESON=/opt/meson-venv/bin/meson
if [ ! -x "$MESON" ]; then
    echo "meson not found (pip install meson)" >&2
    exit 1
fi
if ! command -v ninja >/dev/null 2>&1; then
    echo "ninja not found" >&2
    exit 1
fi

WORK="$HERE/local/build-$ABI"
PREFIX="$WORK/prefix"
# Serialize concurrent runs (e.g. two `make all-arm64` at once): without the
# lock they rm -rf / regenerate the same build dirs and corrupt each other.
mkdir -p "$HERE/local"
exec 9> "$HERE/local/.build-$ABI.lock"
flock 9
rm -rf "$WORK"
mkdir -p "$PREFIX" "$WORK/bin"

# The NDK clang driver defaults --unwindlib=libunwind, which is unused for
# -c compiles; meson's probes add -Werror=unused-command-line-argument
# *after* user c_args, so the only way to keep the probes green is to
# append the -Wno- to every invocation.
for tool in clang clang++; do
    shim="$WORK/bin/${tool}-android.sh"
    printf '#!/bin/sh\nexec "%s/%s" --target=%s "$@" -Wno-unused-command-line-argument\n' \
        "$TC" "$tool" "$CLANG_TARGET" > "$shim"
    chmod +x "$shim"
done

cat > "$WORK/android.cross" <<EOF
[binaries]
c = '$WORK/bin/clang-android.sh'
cpp = '$WORK/bin/clang++-android.sh'
ar = '$TC/llvm-ar'
strip = '$TC/llvm-strip'
pkg-config = '$(command -v pkg-config)'

[host_machine]
system = 'android'
cpu_family = '$MESON_CPU_FAMILY'
cpu = '$MESON_CPU'
endian = 'little'

[properties]
pkg_config_libdir = '$PREFIX/lib/pkgconfig'
EOF

# --- libffi ------------------------------------------------------------
echo "  BUILDING libffi ($ABI)"
mkdir -p "$WORK/libffi"
(
    cd "$WORK/libffi"
    "$FFI_SRC/configure" --host="$AUTOTOOLS_HOST" --prefix="$PREFIX" \
        --disable-shared --enable-static --disable-docs \
        CC="$TC/$CLANG_TARGET-clang" CXX="$TC/$CLANG_TARGET-clang++" \
        AR="$TC/llvm-ar" RANLIB="$TC/llvm-ranlib" STRIP="$TC/llvm-strip" \
        > configure.log 2>&1
    make -j"$(nproc)" > make.log 2>&1
    make install > install.log 2>&1
)

# --- pcre2 -------------------------------------------------------------
echo "  BUILDING pcre2 ($ABI)"
mkdir -p "$WORK/pcre2"
(
    cd "$WORK/pcre2"
    "$PCRE2_SRC/configure" --host="$AUTOTOOLS_HOST" --prefix="$PREFIX" \
        --disable-shared --enable-static \
        CC="$TC/$CLANG_TARGET-clang" CXX="$TC/$CLANG_TARGET-clang++" \
        AR="$TC/llvm-ar" RANLIB="$TC/llvm-ranlib" STRIP="$TC/llvm-strip" \
        > configure.log 2>&1
    make -j"$(nproc)" > make.log 2>&1
    make install > install.log 2>&1
)

# --- glib --------------------------------------------------------------
echo "  BUILDING glib ($ABI)"
"$MESON" setup "$WORK/glib" "$GLIB_SRC" \
    --cross-file "$WORK/android.cross" \
    --default-library=static \
    --prefix="$PREFIX" \
    -Dnls=disabled \
    -Dlibmount=disabled \
    -Dselinux=disabled \
    -Dxattr=false \
    -Dman-pages=disabled \
    -Ddocumentation=false \
    -Dinstalled_tests=false \
    -Dtests=false \
    -Ddtrace=disabled \
    -Dsystemtap=disabled \
    -Dsysprof=disabled \
    -Dintrospection=disabled \
    -Dlibelf=disabled > "$WORK/meson-setup.log" 2>&1
ninja -C "$WORK/glib" -j"$(nproc)" > "$WORK/ninja.log" 2>&1
DESTDIR="$WORK/stage" ninja -C "$WORK/glib" install > "$WORK/ninja-install.log" 2>&1
# Also install into the plain prefix so downstream builds (e.g. wireshark)
# can consume glib-2.0.pc, headers and archives from it.
ninja -C "$WORK/glib" install >> "$WORK/ninja-install.log" 2>&1

# --- artifacts ---------------------------------------------------------
cp "$WORK/glib/glib/libglib-2.0.a" \
   "$WORK/glib/gobject/libgobject-2.0.a" \
   "$WORK/glib/gmodule/libgmodule-2.0.a" \
   "$WORK/glib/gthread/libgthread-2.0.a" \
   "$WORK/glib/gio/libgio-2.0.a" \
   "$WORK"/glib/subprojects/proxy-libintl*/libintl.a \
   "$PREFIX/lib/libffi.a" \
   "$HERE/"

rm -rf "$HERE/glib-2.0"
cp -a "$WORK/stage$PREFIX/include/glib-2.0" "$HERE/glib-2.0"
# glibconfig.h is arch-specific, so meson installs it under
# libdir/glib-2.0/include; consumers here include it from glib-2.0/.
cp "$WORK/glib/glib/glibconfig.h" "$HERE/glib-2.0/glibconfig.h"
cp "$WORK/glib/glib/glibconfig.h" "$HERE/glibconfig.h"

echo "  DONE: glib $(awk '/GLIB_(MAJOR|MINOR|MICRO)_VERSION/{printf "%s%s", sep, $3; sep="."}' "$HERE/glib-2.0/glibconfig.h") ($ABI) installed in $HERE"
