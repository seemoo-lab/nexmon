#!/bin/sh
# Cross-build wireshark 4.6.8 (static libs only: wsutil, wiretap, epan) and
# its dependencies for Android with the NDK:
#
#   libgpg-error 1.61 -> libgcrypt 1.12.3   (autotools, from ../libgpg-error, ../libgcrypt)
#   c-ares 1.34.8, pcre2 10.48, libxml2 2.15.3 (autotools, from ../c-ares, ../pcre2, ../libxml2)
#   glib 2.89.4 from the ../libglib-2.0 build (its prefix is consumed)
#   wireshark itself via CMake with the NDK Android toolchain
#
# The resulting archives are published where the old ndk-build wrappers used
# to put them:
#   ../libwsutil/local/<abi>/libwsutil.a
#   ../libwiretap/local/<abi>/libwiretap.a
#   ../libwireshark/local/<abi>/libwireshark.a
#
# Usage: ./build-android.sh [APP_ABI]        (default: arm64-v8a)
#        APP_ABI=all builds only arm64-v8a, the archives' canonical ABI.
#        With all three archives present and newer than the stamp, the
#        script is a no-op; `make clean` in the wrappers forces a rebuild.
#
# Requirements: NDK_ROOT (or an NDK under /opt/android-sdk/ndk), cmake,
# ninja, pkg-config.

set -eu

HERE=$(cd "$(dirname "$0")" && pwd)
API=28

ABI="${1:-${APP_ABI:-arm64-v8a}}"
if [ "$ABI" = "all" ]; then
    echo "note: APP_ABI=all builds the arm64-v8a archives (the canonical ABI)"
    ABI=arm64-v8a
fi
case "$ABI" in
  arm64-v8a|arm64|aarch64)
    ANDROID_ABI=arm64-v8a
    AUTOTOOLS_HOST=aarch64-linux-android ;;
  armeabi-v7a|armv7|arm|armeabi)
    ANDROID_ABI=armeabi-v7a
    AUTOTOOLS_HOST=arm-linux-androideabi ;;
  x86)
    ANDROID_ABI=x86
    AUTOTOOLS_HOST=i686-linux-android ;;
  x86_64|x64)
    ANDROID_ABI=x86_64
    AUTOTOOLS_HOST=x86_64-linux-android ;;
  *)
    echo "unsupported APP_ABI: $ABI (supported: arm64-v8a armeabi-v7a x86 x86_64)" >&2
    exit 1 ;;
esac

PUB_WSUTIL="$HERE/../libwsutil/local/$ABI/libwsutil.a"
PUB_WIRETAP="$HERE/../libwiretap/local/$ABI/libwiretap.a"
PUB_WIRESHARK="$HERE/../libwireshark/local/$ABI/libwireshark.a"

WORK="$HERE/local/build-$ABI"
STAMP="$WORK/.stamp"
# Serialize concurrent runs (e.g. two `make all-arm64` at once): without the
# lock they rm -rf / regenerate the same build dirs and corrupt each other.
mkdir -p "$HERE/local"
exec 9> "$HERE/local/.build-$ABI.lock"
flock 9
# The dependency prefix must live OUTSIDE this source directory: CMake refuses
# interface include paths that are prefixed in the source tree.
DEPROOT="$HERE/../.wsdeps/$ABI"
if [ -f "$STAMP" ] && [ -f "$PUB_WSUTIL" ] && [ -f "$PUB_WIRETAP" ] && [ -f "$PUB_WIRESHARK" ] \
   && [ "$PUB_WSUTIL" -nt "$STAMP" ] && [ "$PUB_WIRETAP" -nt "$STAMP" ] && [ "$PUB_WIRESHARK" -nt "$STAMP" ]; then
    echo "  wireshark ($ABI) archives up to date"
    exit 0
fi

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
TOOLCHAIN_FILE="$NDK_ROOT/build/cmake/android.toolchain.cmake"
if [ ! -x "$TC/clang" ] || [ ! -f "$TOOLCHAIN_FILE" ]; then
    echo "NDK toolchain incomplete at $TC" >&2
    exit 1
fi
CC="$TC/${AUTOTOOLS_HOST}${API}-clang"
CXX="$TC/${AUTOTOOLS_HOST}${API}-clang++"

command -v cmake >/dev/null 2>&1 || { echo "cmake not found" >&2; exit 1; }
command -v ninja >/dev/null 2>&1 || { echo "ninja not found" >&2; exit 1; }
command -v pkg-config >/dev/null 2>&1 || { echo "pkg-config not found" >&2; exit 1; }

# --- glib (from the libglib-2.0 build) ---------------------------------
GLIB_PREFIX="$HERE/../libglib-2.0/local/build-$ABI/prefix"
if [ ! -f "$GLIB_PREFIX/lib/pkgconfig/glib-2.0.pc" ]; then
    echo "  glib prefix missing; running ../libglib-2.0/build-android.sh $ABI"
    "$HERE/../libglib-2.0/build-android.sh" "$ABI"
fi

PREFIX="$DEPROOT/prefix"
mkdir -p "$PREFIX" "$WORK"

autotools_build() {
    # $1 = source dir, $2 = build dir, $3... = extra configure args
    src=$1; bdir=$2; shift 2
    mkdir -p "$bdir"
    # (Re)configure when there is no Makefile yet, or when the vendored source
    # has been bumped since this build dir was last configured. Without the
    # version check a dependency update is compiled against stale generated
    # Makefiles / config.h from the previous version (e.g. missing new
    # AC_DEFINEs), which fails in confusing ways.
    want_ver=$(sed -n "s/^PACKAGE_VERSION='\\(.*\\)'.*/\\1/p" "$src/configure" 2>/dev/null | head -n1)
    have_ver=$(sed -n 's/.*define PACKAGE_VERSION "\\(.*\\)".*/\\1/p' "$bdir/config.h" 2>/dev/null | head -n1)
    if [ ! -f "$bdir/Makefile" ] || { [ -n "$want_ver" ] && [ "$want_ver" != "$have_ver" ]; }; then
        rm -rf "$bdir"
        mkdir -p "$bdir"
        (
            cd "$bdir"
            "$src/configure" --host="$AUTOTOOLS_HOST" --prefix="$PREFIX" \
                --disable-shared --enable-static \
                CC="$CC" CXX="$CXX" AR="$TC/llvm-ar" \
                RANLIB="$TC/llvm-ranlib" STRIP="$TC/llvm-strip" \
                "$@" > configure.log 2>&1
        )
    fi
    make -C "$bdir" -j"$(nproc)" > "$bdir/make.log" 2>&1
    make -C "$bdir" install > "$bdir/install.log" 2>&1
}

# --- dependencies ------------------------------------------------------
echo "  BUILDING libgpg-error ($ABI)"
autotools_build "$HERE/../libgpg-error" "$DEPROOT/libgpg-error" \
    --disable-doc --disable-tests --disable-l10n

echo "  BUILDING libgcrypt ($ABI)"
autotools_build "$HERE/../libgcrypt" "$DEPROOT/libgcrypt" \
    --disable-doc --with-libgpg-error-prefix="$PREFIX"

echo "  BUILDING c-ares ($ABI)"
autotools_build "$HERE/../c-ares" "$DEPROOT/c-ares"

echo "  BUILDING pcre2 ($ABI)"
autotools_build "$HERE/../pcre2" "$DEPROOT/pcre2"

echo "  BUILDING libxml2 ($ABI)"
autotools_build "$HERE/../libxml2" "$DEPROOT/libxml2" \
    --without-python --without-zlib --without-icu

# --- host build tool: lemon (parser generator) -------------------------
# Cross-compiling needs lemon to run on the BUILD HOST. wireshark's cmake
# (cmake/modules/UseLemon.cmake) uses a system lemon iff
# /usr/share/lemon/lempar.c exists and `lemon` is on PATH; otherwise it
# compiles its bundled lemon for the TARGET, which cannot run on the host.
# Build wireshark's own lemon natively once so versions match.
if [ ! -f /usr/share/lemon/lempar.c ] || ! command -v lemon >/dev/null 2>&1; then
    echo "  BUILDING host lemon"
    HOSTTOOLS="$HERE/local/hosttools"
    mkdir -p "$HOSTTOOLS"
    cc -O2 -o "$HOSTTOOLS/lemon" "$HERE/tools/lemon/lemon.c"
    mkdir -p /usr/share/lemon
    cp "$HERE/tools/lemon/lempar.c" /usr/share/lemon/lempar.c
    cp "$HOSTTOOLS/lemon" /usr/local/bin/lemon
fi

# --- wireshark ---------------------------------------------------------
echo "  BUILDING wireshark ($ABI)"
export PKG_CONFIG_LIBDIR="$GLIB_PREFIX/lib/pkgconfig:$PREFIX/lib/pkgconfig"
export PKG_CONFIG_SYSROOT_DIR=/

# The NDK ships no linux-aarch64 host toolchain, so both the NDK toolchain
# file and CMake's Android platform modules fall back to the x86_64 host
# binaries, which run under qemu-x86_64 on this machine: an order of
# magnitude slower and prone to random mid-compile aborts (see
# buildtools/ndk-native-aarch64.sh). When the native aarch64 prebuilts
# exist, wrap the NDK toolchain file and re-point the host tools at them.
USE_TOOLCHAIN_FILE="$TOOLCHAIN_FILE"
NATIVE_TC="$NDK_ROOT/toolchains/llvm/prebuilt/linux-aarch64"
if [ "$(uname -m)" = "aarch64" ] && [ -x "$NATIVE_TC/bin/clang" ]; then
    USE_TOOLCHAIN_FILE="$WORK/android-native.toolchain.cmake"
    cat > "$USE_TOOLCHAIN_FILE" <<EOF
include("$TOOLCHAIN_FILE")
set(ANDROID_HOST_TAG linux-aarch64)
set(ANDROID_TOOLCHAIN_ROOT "$NATIVE_TC")
set(CMAKE_SYSROOT "$NATIVE_TC/sysroot")
set(CMAKE_C_COMPILER "$NATIVE_TC/bin/clang")
set(CMAKE_CXX_COMPILER "$NATIVE_TC/bin/clang++")
set(CMAKE_AR "$NATIVE_TC/bin/llvm-ar")
set(CMAKE_RANLIB "$NATIVE_TC/bin/llvm-ranlib")
set(CMAKE_STRIP "$NATIVE_TC/bin/llvm-strip")
EOF
fi

if [ ! -f "$WORK/wireshark/CMakeCache.txt" ]; then
    cmake -G Ninja -S "$HERE" -B "$WORK/wireshark" \
        -DCMAKE_TOOLCHAIN_FILE="$USE_TOOLCHAIN_FILE" \
        -DANDROID_ABI="$ANDROID_ABI" \
        -DANDROID_PLATFORM="android-$API" \
        -DCMAKE_BUILD_TYPE=Release \
        -DBUILD_SHARED_LIBS=OFF \
        -DCMAKE_PREFIX_PATH="$GLIB_PREFIX;$PREFIX" \
        -DCMAKE_FIND_ROOT_PATH="$GLIB_PREFIX;$PREFIX" \
        -DCMAKE_INSTALL_PREFIX="$WORK/install" \
        -DBUILD_wireshark=OFF -DBUILD_stratoshark=OFF -DBUILD_tshark=OFF \
        -DBUILD_strato=OFF -DBUILD_tfshark=OFF -DBUILD_rawshark=OFF \
        -DBUILD_dumpcap=OFF -DBUILD_text2pcap=OFF -DBUILD_mergecap=OFF \
        -DBUILD_reordercap=OFF -DBUILD_editcap=OFF -DBUILD_capinfos=OFF \
        -DBUILD_captype=OFF -DBUILD_randpkt=OFF -DBUILD_dftest=OFF \
        -DBUILD_corbaidl2wrs=OFF -DBUILD_dcerpcidl2wrs=OFF -DBUILD_xxx2deb=OFF \
        -DBUILD_androiddump=OFF -DBUILD_sshdump=OFF -DBUILD_ciscodump=OFF \
        -DBUILD_dpauxmon=OFF -DBUILD_randpktdump=OFF -DBUILD_wifidump=OFF \
        -DBUILD_etwdump=OFF -DBUILD_sdjournal=OFF -DBUILD_udpdump=OFF \
        -DBUILD_falcodump=OFF -DBUILD_sshdig=OFF -DBUILD_sharkd=OFF \
        -DBUILD_mmdbresolve=OFF -DBUILD_fuzzshark=OFF \
        -DENABLE_PCAP=OFF -DENABLE_LZ4=OFF -DENABLE_BROTLI=OFF \
        -DENABLE_SNAPPY=OFF -DENABLE_ZSTD=OFF -DENABLE_NGHTTP2=OFF \
        -DENABLE_NGHTTP3=OFF -DENABLE_LUA=OFF -DENABLE_SMI=OFF \
        -DENABLE_GNUTLS=OFF -DENABLE_KERBEROS=OFF -DENABLE_SBC=OFF \
        -DENABLE_SPANDSP=OFF -DENABLE_BCG729=OFF -DENABLE_AMRNB=OFF \
        -DENABLE_ILBC=OFF -DENABLE_OPUS=OFF -DENABLE_SINSP=OFF \
        -DENABLE_PLUGINS=OFF -DENABLE_MINIZIP=OFF -DENABLE_CAP=OFF \
        -DENABLE_NETLINK=OFF -DENABLE_WERROR=OFF \
        > "$WORK/cmake-config.log" 2>&1
fi
ninja -C "$WORK/wireshark" -j"$(nproc)" wsutil wiretap epan > "$WORK/ninja.log" 2>&1

# --- publish -----------------------------------------------------------
for spec in "run/libwsutil.a:$PUB_WSUTIL" \
            "run/libwiretap.a:$PUB_WIRETAP" \
            "run/libwireshark.a:$PUB_WIRESHARK"; do
    from="$WORK/wireshark/${spec%%:*}"
    to="${spec##*:}"
    mkdir -p "$(dirname "$to")"
    cp "$from" "$to"
done
touch "$STAMP"
echo "  DONE: wireshark $(sed -n 's/^#define VERSION "\(.*\)"/\1/p' "$WORK/wireshark/config.h") ($ABI) libraries published"
