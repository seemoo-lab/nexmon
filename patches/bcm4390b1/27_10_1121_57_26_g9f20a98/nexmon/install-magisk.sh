#!/usr/bin/env bash

C_GREEN="\033[0;32m"
C_RED="\033[0;31m"
C_NONE="\033[0m"

if [[ -z "$1" ]]; then
    printf "\n${C_RED}!${C_NONE} Missing argument 1: <magisk module archive>!\n\n"
    exit 1
fi

if [[ ! -f "$1" ]]; then
    printf "\n${C_RED}!${C_NONE} Error in argument 1: <magisk module archive> %s is not a file!\n\n" "$1"
    exit 1
fi

printf "\n${C_GREEN}#${C_NONE} Installing Magisk module\n\n"
adb start-server
DEVICES=$(adb devices | head -n-1 | tail -n+2 | sed -r 's/^(([^\s])+)\s.*$/\1/g' | tr '\n' ' ')

if [[ -z "${DEVICES}" ]]; then
    printf "\n${C_RED}!${C_NONE} No devices found by ADB!\n\n"
    exit 1
fi

FNAME=$(basename $1)

for DEVICE in ${DEVICES}; do
    VENDOR=$(adb -s "${DEVICE}" shell getprop ro.product.manufacturer)
    MODEL=$(adb -s "${DEVICE}" shell getprop ro.product.model)
    read -p "Install on device ${VENDOR} ${MODEL} (${DEVICE})? (Y/n): " user_install
    if [[ -z "${user_install}" || "${user_install,,}" == "y" || "${user_install,,}" == "yes" ]]; then
        printf "${C_GREEN}+${C_NONE} Install on %s\n" "${DEVICE}"
        printf "${C_GREEN}+${C_NONE} Copy to device: %s => %s\n" "$1" "/sdcard/$FNAME"
        adb -s "${DEVICE}" push "$1" "/sdcard/$FNAME"
        printf "${C_GREEN}+${C_NONE} Install %s with magisk\n" "/sdcard/$FNAME"
        adb -s "${DEVICE}" shell 'su -c "magisk --install-module /sdcard/'''$FNAME'''"'
        printf "${C_RED}+ Device reboot required!${C_NONE}\n"
        read -p "Reboot ${DEVICE} now? (Y/n): " user_reboot
        if [[ -z "${user_reboot}" || "${user_reboot,,}" == "y" || "${user_reboot,,}" == "yes" ]]; then
            adb -s "${DEVICE}" reboot
        fi
    else
        printf "${C_GREEN}-${C_NONE} Skipping install on %s\n" "${DEVICE}"
    fi
done

printf "\n${C_GREEN}### DONE ###${C_NONE}\n\n"
