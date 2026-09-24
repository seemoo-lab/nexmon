#!/bin/bash

C_GREEN="\033[0;32m"
C_RED="\033[0;31m"
C_NONE="\033[0m"

if [[ -z "$1" ]]; then
    printf "\n${C_RED}!${C_NONE} Missing argument 1: <firmware file>!\n\n"
    exit 1
fi

if [[ ! -f "$1" ]]; then
    printf "\n${C_RED}!${C_NONE} Error in argument 1: %s is not a file!\n\n" "$1"
    exit 1
fi

printf "\n${C_GREEN}#${C_NONE} Installing firmware\n\n"
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
        printf "${C_GREEN}+${C_NONE} Install per bind mount: /sdcard/%s => /data/local/tmp/%s => /vendor/firmware/%s\n" "$FNAME" "$FNAME" "$FNAME"
        adb -s "${DEVICE}" shell 'su -c "umount /vendor/firmware/'''$FNAME''' 2>/dev/null"'
        adb -s "${DEVICE}" shell 'su -c "mv /sdcard/'''$FNAME''' /data/local/tmp/'''$FNAME'''"'
        adb -s "${DEVICE}" shell 'su -c "chown root:root /data/local/tmp/'''$FNAME'''"'
        adb -s "${DEVICE}" shell 'su -c "chmod 644 /data/local/tmp/'''$FNAME'''"'
        adb -s "${DEVICE}" shell 'su -c "chcon u:object_r:vendor_fw_file:s0 /data/local/tmp/'''$FNAME'''"'
        adb -s "${DEVICE}" shell 'su -c "mount -o bind /data/local/tmp/'''$FNAME''' /vendor/firmware/'''$FNAME'''"'
        sleep 1
        adb -s "${DEVICE}" shell 'su -c "rmmod bcmdhd4383 && insmod /vendor/lib/modules/bcmdhd4383.ko dhd_msg_level=0x450001"'
        adb -s "${DEVICE}" shell 'su -c "sleep 2 && ifconfig wlan0 up"'
    else
        printf "${C_GREEN}-${C_NONE} Skipping install on %s\n" "${DEVICE}"
    fi
done
printf "\n${C_GREEN}### DONE ###${C_NONE}\n\n"
