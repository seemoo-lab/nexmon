OLD_PWD=$(pwd)
cd $(dirname ${BASH_SOURCE[0]})

export ARCH=arm
export SUBARCH=arm
export KERNEL=kernel7

export HOSTUNAME=$(uname -s)
export PLATFORMUNAME=$(uname -m)

export NEXMON_ROOT=$(pwd)

if [ $HOSTUNAME == "Darwin" ]; then
    export CC=$NEXMON_ROOT/buildtools/gcc-arm-none-eabi-5_4-2016q2-osx/bin/arm-none-eabi-
    export CCPLUGIN=$NEXMON_ROOT/buildtools/gcc-nexmon-plugin-osx/nexmon.so
    export ZLIBFLATE="openssl zlib"
else if [ $HOSTUNAME == "Linux" ] && [ $PLATFORMUNAME == "x86_64" ]; then
    export CC=$NEXMON_ROOT/buildtools/gcc-arm-none-eabi-5_4-2016q2-linux-x86/bin/arm-none-eabi-
    export CCPLUGIN=$NEXMON_ROOT/buildtools/gcc-nexmon-plugin/nexmon.so
    export ZLIBFLATE="zlib-flate -compress"
else if [[ $HOSTUNAME == "Linux" ]] && [[ $PLATFORMUNAME == "armv7l" || $PLATFORMUNAME == "armv6l" ]]; then
    export CC=$NEXMON_ROOT/buildtools/gcc-arm-none-eabi-5_4-2016q2-linux-armv7l/bin/arm-none-eabi-
    export CCPLUGIN=$NEXMON_ROOT/buildtools/gcc-nexmon-plugin-arm/nexmon.so
    export ZLIBFLATE="zlib-flate -compress"
else
    echo "Platform not supported!"
fi
fi
fi

export Q=@
export NEXMON_SETUP_ENV=1

cd "$OLD_PWD"
