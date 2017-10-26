#!/bin/bash
make -C /lib/modules/$(uname -r)/build M=$(pwd)/brcmfmac_kernel49 -j2 V=1
cp brcmfmac_kernel49/brcmfmac.ko /home/pi/P4wnP1/nexmon/brcmfmac.ko
chown pi:pi /home/pi/P4wnP1/nexmon/brcmfmac.ko
#source /home/pi/P4wnP1/boot/init_wifi_nexmon.sh
#WIFI_activate_nexmon
#/home/pi/P4wnP1/nexmon/nexutil -m6
