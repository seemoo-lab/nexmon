# nexmon brcmfmac driver — kernel 6.18 port

Base: jayofelony/nexmon@dev `brcmfmac_6.12.y-nexmon` (monolithic; drops the
upstream fwvid vendor-split, so a single brcmfmac.ko carries all vendor logic).

## Changes made to port 6.12 -> 6.18
- Makefile: include paths 6.6 -> 6.18.
- cfg80211.c timer API (removed in 6.16):
    del_timer_sync  -> timer_delete_sync   (also btcoex.c, sdio.c)
    from_timer      -> timer_container_of  (also btcoex.c, sdio.c)
- cfg80211_ops signature changes (6.16-6.18 multi-radio / MLO):
    set_wiphy_params:     + int radio_idx
    set_tx_power:         + int radio_idx
    get_tx_power:         + int radio_idx, unsigned int link_id
    set_monitor_channel:  + struct net_device *dev
- sdio.c / bcmsdh.c: SDIO_DEVICE_ID_BROADCOM_CYPRESS_43752 -> SDIO_DEVICE_ID_BROADCOM_43752 (renamed upstream)

## Build (aarch64)
    source setup_env.sh
    ARCH=arm64 make -C /lib/modules/$(uname -r)/build \
        M=/root/nexmon/patches/driver/brcmfmac_6.18.y-nexmon modules
Or via nexmon: `make -C patches/bcm43455c0/7_45_234_4ca95bb_CY/nexmon brcmfmac.ko`

## vermagic note
The kernel build tree /root/linux generated UTS_RELEASE without the trailing "+".
Running kernel is 6.18.46-v8-16k+. Fixed include/generated/utsrelease.h and
include/config/kernel.release to include "+" so module vermagic matches. This is
consistent with .scmversion (="+") and how the kernel itself derives the suffix.

Verified: module insmods cleanly into the running 6.18.46-v8-16k+ kernel.
