#!/bin/bash

C_GREEN="\033[0;32m"
C_RED="\033[0;31m"
C_NONE="\033[0m"

if [[ -z "$1" ]]; then
    printf "\n${C_RED}!${C_NONE} Missing argument 1: <binary>!\n\n"
    exit 1
fi

if [[ ! -f "$1" ]]; then
    printf "\n${C_RED}!${C_NONE} Error in argument 1: binary %s is not a file!\n\n" "$1"
    exit 1
fi

BINNAME=$(basename $1)

printf "\n${C_GREEN}#${C_NONE} Installing util %s\n\n" "$BINNAME"
adb start-server
DEVICES=$(adb devices | head -n-1 | tail -n+2 | sed -r 's/^(([^\s])+)\s.*$/\1/g' | tr '\n' ' ')

if [[ -z "${DEVICES}" ]]; then
    printf "\n${C_RED}!${C_NONE} No devices found by ADB!\n\n"
    exit 1
fi


for DEVICE in ${DEVICES}; do
    VENDOR=$(adb -s "${DEVICE}" shell getprop ro.product.manufacturer)
    MODEL=$(adb -s "${DEVICE}" shell getprop ro.product.model)
    read -p "Install on device ${VENDOR} ${MODEL} (${DEVICE})? (Y/n): " user_install
    if [[ -z "${user_install}" || "${user_install,,}" == "y" || "${user_install,,}" == "yes" ]]; then
        printf "${C_GREEN}+${C_NONE} Install on %s\n" "${DEVICE}"
        printf "${C_GREEN}+${C_NONE} Copy to device: %s => %s\n" "$1" "/sdcard/$BINNAME"
        adb -s "${DEVICE}" push "$1" "/sdcard/$BINNAME"
        printf "${C_GREEN}+${C_NONE} Installing /sdcard/%s => /system_ext/bin/%s\n" "$BINNAME" "$BINNAME"
        adb -s "${DEVICE}" shell 'su -c "mount -o remount,rw /system_ext/bin"'
        adb -s "${DEVICE}" shell 'su -c "mv /sdcard/'''$BINNAME''' /system_ext/bin/"'
        adb -s "${DEVICE}" shell 'su -c "chown root:root /system_ext/bin/'''$BINNAME'''"'
        adb -s "${DEVICE}" shell 'su -c "chmod 755 /system_ext/bin/'''$BINNAME'''"'
        adb -s "${DEVICE}" shell 'su -c "mount -o remount,ro /system_ext/bin"'
    else
        printf "${C_GREEN}-${C_NONE} Skipping install on %s\n" "${DEVICE}"
    fi
done
printf "\n${C_GREEN}### DONE ###${C_NONE}\n\n"
