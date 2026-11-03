# nexmon brcmfmac driver — kernel 7.3 port

Base: `brcmfmac_7.2.y-nexmon`. Stays monolithic (no upstream fwvid vendor
split): one `brcmfmac.ko` carries all vendor logic, and the stock
`brcmfmac-{cyw,bca,wcc}.ko` are neither needed nor requested.

## Changes made to port 7.2 -> 7.3

### Kernel 7.3: cfg80211 cookie output to input conversion
Kernel 7.3 converted the `u64 *cookie` output parameter to a plain `u64 cookie`
input parameter in `remain_on_channel` and `mgmt_tx` ops across `cfg80211_ops`,
as cfg80211 now pre-assigns the cookie value before invoking drivers.

- `cfg80211.c`: `brcmf_cfg80211_mgmt_tx()` accepts `u64 cookie` (version-guarded
  with `#if LINUX_VERSION_CODE >= KERNEL_VERSION(7,3,0)`).
- `p2p.c` / `p2p.h`: `brcmf_p2p_remain_on_channel()` accepts `u64 cookie`
  instead of `u64 *cookie` on `>= 7.3.0` and stores it into
  `p2p->remain_on_channel_cookie`.

### Kernel 7.3 upstream driver fixes & updates
- `sdio.c`: Declared CLM blob for 43456 (`BRCMF_FW_CLM_DEF(43456, "brcmfmac43456-sdio")`)
  matching upstream modinfo metadata.
- `sdio.c`: Fixed potential buffer leak in `brcmf_sdio_read_control()` on error paths
  by calling `vfree(buf)`.
- `p2p.c`: Handled action frame abort gracefully when device vif is not yet
  allocated (`vif = p2p->bss_idx[P2PAPI_BSSCFG_PRIMARY].vif` fallback), and guarded
  against NULL dereference of `saved_ie` in `brcmf_p2p_send_action_frame()`.
- `msgbuf.c`: Validated `flow_ring_id` boundaries via `brcmf_msgbuf_get_flowid()`
  to prevent array underflow/overflow, and properly stored DMA direction in
  `brcmf_msgbuf_init_pktids()`.
- `common.c`: Updated `MODULE_VERSION` to `"7.3.0-nexmon"`.

### SDPCM flow control & control frame deadlock fixes
- `sdio.c`: Unconditionally reserved `TXCTL_CREDITS` (2) in `data_ok()` so bulk data
  transmissions and raw packet injection cannot starve the control path of SDPCM sequence window credits.
- `sdio.c`: In `brcmf_sdio_sendfromq()`, deducted `TXCTL_CREDITS` from available credits when
  computing `pkt_num` for TX glomming (`bus->txglom`), preventing batch aggregation from overdrawing credits.
- `sdio.c`: In `brcmf_sdio_dpc()`, added active polling of `brcmf_sdio_intr_rstatus()` and pending frames
  when a control frame is queued with 0 credits (`!txctl_ok`), immediately reclaiming credit updates from the dongle.
- `sdio.c`: Re-triggered DPC while `bus->ctrl_frame_stat` is set so control operations are never stalled.
- `sdio.c`: In `brcmf_sdio_bus_watchdog()`, guarded the idle bus sleep condition with
  `!bus->ctrl_frame_stat` to ensure the watchdog thread cannot put the bus to sleep or gate clocks while
  a control frame is pending.

### Monitor mode auto-recovery & channel state preservation
- `core.c`: In `brcmf_detach()`, synchronized netdev-less P2P device VIF removal under
  `rtnl_lock()` and `wiphy_lock(drvr->wiphy)` (upstream fix `08a3f02fea63`), preventing
  fatal `LIST_POISON2` page faults when `wpa_supplicant` exits concurrently during monitor
  teardown or module removal.
- `core.c`: Retained monitor interface name and active chanspec in `nexmon_mon_restore` across SDIO
  bus watchdog resets (`brcmf_sdio_ctl_watchdog`).
- `core.c`: Replayed monitor session via `brcmf_core_mon_restore` workqueue upon `brcmf_bus_started()`,
  avoiding loss of packet capture on transient bus errors.
- `cfg80211.c`: In `brcmf_mon_restore_vif()`, automatically took down the primary station interface (`wlan0`)
  if re-upped by udev/NetworkManager during re-probe, preventing cfg80211 `-EBUSY` rejections during channel hops.
- `cfg80211.c`: Restored the previously active channel `chanspec` via `brcmf_fil_iovar_int_set(pri_ifp, "chanspec", chspec)`
  upon recreating the monitor netdev.
- `cfg80211.c`: Pre-validated requested channel in `brcmf_cfg80211_nexmon_set_channel()` against regulatory
  disabled flags (`IEEE80211_CHAN_DISABLED`), logging explicit diagnostic messages when rejected early.

### Compiler & format warning fixes
- `firmware.c`: Expanded `pci_path`, `pcie_path`, and `prefix` buffers from 9/10 bytes to 32 bytes
  to prevent GCC `-Werror=format-truncation` warnings when formatting `u16` bus and domain numbers.

## Build (aarch64)

    ARCH=arm64 make -C /lib/modules/$(uname -r)/build \
        M=/root/nexmon/patches/driver/brcmfmac_7.3.y-nexmon modules

Or via nexmon, which selects this directory automatically from `uname -r`:
`make -C patches/bcm43455c0/7_45_234_4ca95bb_CY/nexmon brcmfmac.ko`

## vermagic note
Matches the running `7.3.0-rc2-v8-16k+` kernel tree.
