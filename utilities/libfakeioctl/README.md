$ adb shell su -c "LD_PRELOAD=libfakeioctl.so tcpdump -i wlan0"
####################################################
## nexmon ioctl hook active
## sa_family = ARPHRD_IEEE80211_RADIOTAP
## to change sa_family, set NEXMON_SA_FAMILY
## environment variable to ARPHRD_IEEE80211
####################################################
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on wlan0, link-type IEEE802_11_RADIO (802.11 plus radiotap header), capture size 262144 bytes

$ adb shell su -c "LD_PRELOAD=libfakeioctl.so NEXMON_SA_FAMILY=ARPHRD_IEEE80211 tcpdump -i wlan0"
####################################################
## nexmon ioctl hook active
## sa_family = ARPHRD_IEEE80211
## to change sa_family, set NEXMON_SA_FAMILY
## environment variable to ARPHRD_IEEE80211
####################################################
tcpdump: verbose output suppressed, use -v or -vv for full protocol decode
listening on wlan0, link-type IEEE802_11 (802.11), capture size 262144 bytes