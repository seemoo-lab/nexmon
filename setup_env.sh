export ARCH=arm
export SUBARCH=arm

export NEXMON_ROOT=$(pwd)

export CC=$NEXMON_ROOT/buildtools/gcc-arm-none-eabi-5_4-2016q2-linux-x86/bin/arm-none-eabi-
export CCPLUGIN=$NEXMON_ROOT/buildtools/gcc-nexmon-plugin/nexmon.so

export Q=@

export NEXMON_SETUP_ENV=1