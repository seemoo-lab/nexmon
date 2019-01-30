OLD_PWD=$(pwd)
shell_path=$(echo $SHELL)
script_path=''
re_zsh="zsh"
re_bash="bash"

echo -e "Setting up Nexmon build environment..."
# Get the path of this script relative to the calling path
# This is done using a shells' functionality, therefore we have to 
# check which shell is currently running / sourcing this script
# https://stackoverflow.com/questions/9901210/bash-source0-equivalent-in-zsh
if [[ $script_path =~ $re_szh ]];
then
  echo -e "[*] Detected ZSH"
  script_path=${(%):-%x}
elif [[ $script_path =~ $re_bash ]];
then
  script_path=${BASH_SOURCE[0]}
  echo -e "[*] Detected Bash"
else
  echo -e "\033[0;31m[!!] Unsupported shell! Please use Bash or ZSH."
  exit
fi

echo -e "[*] Script path: $script_path"
cd $(dirname $script_path)

export ARCH=arm
echo "[*] Exported ARCH=$ARCH"
export SUBARCH=arm
echo "[*] Exported SUBARCH=$SUBARCH"
export KERNEL=kernel7
echo "[*] Exported KERNEL=$KERNEL"

export HOSTUNAME=$(uname -s)
export PLATFORMUNAME=$(uname -m)

export NEXMON_ROOT=$(pwd)
echo "[*] Exported NEXMON_ROOT=$NEXMON_ROOT"
alias cd-nex='cd $NEXMON_ROOT'
echo "[*] Created alias to quickly cd to $NEXMON_ROOT: cd-nex"
export NEXMON_FW_PATH=$(pwd)/firmwares
echo "[*] Exported NEXMON_FW_PATH=$NEXMON_FW_PATH"



if [ $HOSTUNAME = "Darwin" ]; 
then
    export CC=$NEXMON_ROOT/buildtools/gcc-arm-none-eabi-5_4-2016q2-osx/bin/arm-none-eabi-
    export CCPLUGIN=$NEXMON_ROOT/buildtools/gcc-nexmon-plugin-osx/nexmon.so
    export ZLIBFLATE="openssl zlib"
elif [ $HOSTUNAME = "Linux" ] && [ $PLATFORMUNAME = "x86_64" ]; 
then
    export CC=$NEXMON_ROOT/buildtools/gcc-arm-none-eabi-5_4-2016q2-linux-x86/bin/arm-none-eabi-
    export CCPLUGIN=$NEXMON_ROOT/buildtools/gcc-nexmon-plugin/nexmon.so
    export ZLIBFLATE="zlib-flate -compress"
elif [[ $HOSTUNAME = "Linux" ]] && [[ $PLATFORMUNAME = "armv7l" || $PLATFORMUNAME = "armv6l" ]]; 
then
    export CC=$NEXMON_ROOT/buildtools/gcc-arm-none-eabi-5_4-2016q2-linux-armv7l/bin/arm-none-eabi-
    export CCPLUGIN=$NEXMON_ROOT/buildtools/gcc-nexmon-plugin-arm/nexmon.so
    export ZLIBFLATE="zlib-flate -compress"
else
    echo -e "\033[1;31m[!!]Platform not supported!"
    exit
fi

echo -e "[*] Detected $HOSTUNAME $PLATFORMUNAME"
echo "[*] Exported CC=$CC"
echo "[*] Exported CCPLUGIN=$CCPLUGIN"
echo "[*] Exported ZLIBFLATE=$ZLIBFLATE"

export Q=@
export NEXMON_SETUP_ENV=1

echo -e "\033[0;32mSuccessfully set up Nexmon build environment.\033[0m"
cd "$OLD_PWD"
