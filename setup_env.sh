OLD_PWD=$(pwd)
cd $(dirname ${BASH_SOURCE[0]})

export ARCH=arm
export SUBARCH=arm
export KERNEL=kernel7

export HOSTUNAME=$(uname -s)
export PLATFORMUNAME=$(uname -m)

export NEXMON_ROOT=$(pwd)

if [ "$HOSTUNAME" == "Darwin" ]; then
    export CC=$NEXMON_ROOT/buildtools/gcc-arm-none-eabi-5_4-2016q2-osx/bin/arm-none-eabi-
    export CCPLUGIN=$NEXMON_ROOT/buildtools/gcc-nexmon-plugin-osx/nexmon.so
    export ZLIBFLATE="openssl zlib"
elif [ "$HOSTUNAME" == "Linux" ] && [ "$PLATFORMUNAME" == "x86_64" ]; then
    export CC=$NEXMON_ROOT/buildtools/gcc-arm-none-eabi-5_4-2016q2-linux-x86/bin/arm-none-eabi-
    export CCPLUGIN=$NEXMON_ROOT/buildtools/gcc-nexmon-plugin/nexmon.so
    export ZLIBFLATE="zlib-flate -compress"
elif [ "$HOSTUNAME" == "Linux" ] && { [ "$PLATFORMUNAME" == "armv7l" ] || [ "$PLATFORMUNAME" == "armv6l" ] || [ "$PLATFORMUNAME" == "aarch64" ]; }; then
    export CC=$NEXMON_ROOT/buildtools/gcc-arm-none-eabi-5_4-2016q2-linux-armv7l/bin/arm-none-eabi-
    export CCPLUGIN=$NEXMON_ROOT/buildtools/gcc-nexmon-plugin-arm/nexmon.so
    export ZLIBFLATE="zlib-flate -compress"
    # This cross-compiler is an armv7l/armhf ELF binary. On an aarch64 host it
    # runs through the armhf compat runtime and needs armhf builds of
    # libisl/libmpfr that are not on the default loader path; ship them in
    # buildtools/armhf-compat-libs so cc1 can find them.
    if [ -d "$NEXMON_ROOT/buildtools/armhf-compat-libs" ]; then
        export LD_LIBRARY_PATH="$NEXMON_ROOT/buildtools/armhf-compat-libs${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
    fi
else
    echo "Platform not supported! ($HOSTUNAME/$PLATFORMUNAME)" >&2
    # Do NOT mark the environment as initialised: leaving NEXMON_SETUP_ENV unset
    # makes the build's check-nexmon-setup-env guard fire with a clear message
    # instead of proceeding with an unset CC and failing obscurely later.
    cd "$OLD_PWD"
    return 1 2>/dev/null || exit 1
fi

export Q=@
export NEXMON_SETUP_ENV=1

if [ -z "$NDK_ROOT" ]; then
    if [ -n "$ANDROID_NDK_ROOT" ] && [ -d "$ANDROID_NDK_ROOT" ]; then
        export NDK_ROOT="$ANDROID_NDK_ROOT"
    elif [ -n "$ANDROID_NDK_HOME" ] && [ -d "$ANDROID_NDK_HOME" ]; then
        export NDK_ROOT="$ANDROID_NDK_HOME"
    elif [ -n "$ANDROID_NDK" ] && [ -d "$ANDROID_NDK" ]; then
        export NDK_ROOT="$ANDROID_NDK"
    elif [ -d "/opt/android-sdk/ndk" ]; then
        export NDK_ROOT="$(find /opt/android-sdk/ndk -mindepth 1 -maxdepth 1 -type d | sort -V | tail -n 1)"
    elif command -v ndk-build >/dev/null 2>&1; then
        export NDK_ROOT="$(dirname "$(command -v ndk-build)")"
    fi
fi

if [ -n "$NDK_ROOT" ] && [[ ":$PATH:" != *":$NDK_ROOT:"* ]]; then
    export PATH="$PATH:$NDK_ROOT"
fi

# The NDK has no linux-aarch64 host toolchain, so on an ARM64 host ndk-build
# runs its x86_64 clang under qemu: slow, and it aborts at random with
# "libc++abi: Pure virtual function called!". buildtools/ndk-native-aarch64.sh
# installs a native one alongside it; use it when it is there.
if [ -n "$NDK_ROOT" ] && [ "$PLATFORMUNAME" == "aarch64" ]; then
    if [ -x "$NDK_ROOT/toolchains/llvm/prebuilt/linux-aarch64/bin/clang" ]; then
        export NDK_NATIVE_HOST_ARCH=aarch64
        export GNUMAKE="$NEXMON_ROOT/buildtools/ndk-make-native"
    else
        echo "NOTE: no native aarch64 NDK toolchain; utilities will build under"
        echo "      qemu-x86_64. Run buildtools/ndk-native-aarch64.sh to fix."
    fi
fi

cd "$OLD_PWD"
