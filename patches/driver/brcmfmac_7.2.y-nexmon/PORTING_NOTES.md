# nexmon brcmfmac driver — kernel 7.2 port

Base: `brcmfmac_6.18.y-nexmon` as of jayofelony/nexmon@dev (28d42c0). That tree
is already version-guarded with `LINUX_VERSION_CODE` for 6.11.2 → 6.18, so this
port only adds two more rungs to the same ladder. It stays monolithic (no
upstream fwvid vendor split): one `brcmfmac.ko` carries all vendor logic, and
the stock `brcmfmac-{cyw,bca,wcc}.ko` are neither needed nor requested.

## Changes made to port 6.18 -> 7.2

### Kernel 7.1: cfg80211_ops wdev-ification
7.1 changed a group of ops from taking the interface's `struct net_device *` to
taking its `struct wireless_dev *`, and moved `cfg80211_new_sta()` /
`cfg80211_del_sta()` over with them. Affected ops:

    get_station  dump_station  add_key  del_key  get_key
    set_default_mgmt_key  del_station  change_station

Rather than duplicate eight signatures behind `#if`, `cfg80211.c` defines:

    BRCMF_OP_NDEV_ARG        the op's 2nd parameter (wdev on >=7.1, ndev below)
    BRCMF_OP_NDEV            resolves that parameter to a struct net_device *
    BRCMF_OP_NDEV_PASS(ndev) the reverse, for handing a netdev to those callees

Each affected function takes `BRCMF_OP_NDEV_ARG` and opens with
`struct net_device *ndev = BRCMF_OP_NDEV;`, so every function body is unchanged
and identical on both sides of the version boundary. Two internal callers
(`add_key` -> `del_key`, `dump_station` -> `get_station`) and the two
`cfg80211_{new,del}_sta()` event call sites go through `BRCMF_OP_NDEV_PASS()`.

### Kernel 7.2: remain_on_channel gained rx_addr
`remain_on_channel` takes a trailing `const u8 *rx_addr` (the MLO per-link
receive address). This driver has no per-link address to honour, so
`brcmf_p2p_remain_on_channel()` accepts and ignores it; the pre-7.2 signature is
kept behind `#if LINUX_VERSION_CODE < KERNEL_VERSION(7,2,0)` (p2p.c and p2p.h).

### Kernel 7.2: strncpy() removed
`strncpy()` is gone from the kernel as of 7.2 (present through 7.1). Three call
sites, all the same `ifp->ndev->name` copy, replaced with `strscpy()` — which is
what upstream brcmfmac does, and which is available on every kernel this tree
supports, so no guard is needed:

    cfg80211.c  brcmf_cfg80211_add_iface()   (station/AP path)
    cfg80211.c  the NEXMON monitor-interface path
    p2p.c       brcmf_p2p_add_vif()

`set_monitor_channel` is deliberately *not* touched: 7.2 still passes it a
`struct net_device *`, so the existing nexmon hook signature is already correct.

## Build (aarch64)

    ARCH=arm64 make -C /lib/modules/$(uname -r)/build \
        M=/root/nexmon/patches/driver/brcmfmac_7.2.y-nexmon modules

Or via nexmon, which selects this directory automatically from `uname -r`:
`make -C patches/bcm43455c0/7_45_234_4ca95bb_CY/nexmon brcmfmac.ko`

The Makefile uses `-I$(src)` rather than a hardcoded `$(NEXMON_ROOT)` path, so
the directory can be copied to a new kernel version without editing it.

## vermagic note
Unlike the 6.18 port, no `include/generated/utsrelease.h` fixup was needed: the
7.2 build tree at /root/linux-rpi-7.2.y already produces `7.2.2-v8-16k+`, which
matches the running kernel, so the module's vermagic matches as built.
