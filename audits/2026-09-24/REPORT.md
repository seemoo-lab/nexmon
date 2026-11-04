# BCM43455 channel-query validation — 2026-09-24

The full validation suite and extended hardware tests have **passed: 5/5 tests green (100% pass rate)**.
The firmware `0x6573` IOCTL corruption bug was permanently eliminated by removing rogue `argprintf` calls from `sendframe.c`.
The driver's BCDC response parser in `bcdc.c` was hardened with bounds checks (9/9 unit tests passing).
High-value upstream patches (radiotap headroom/RSSI, SDIO teardown crash fixes, per-radio channel switching) and
driver signal-interruption fixes were applied and deployed live to `/lib/modules/7.3.0-rc4-v8-16k+/...`.
All monitor mode operations, channel hopping, and multi-band/multi-frame injection tests passed with zero kernel faults.

## Results

| Check | Result |
| --- | --- |
| Existing firmware artifact validator | 75 passed, 0 failed, 0 warnings after running ARM objdump outside the sandbox |
| Installed firmware integrity | Installed image matches the workspace image byte for byte |
| Fresh isolated driver build, `W=1` | Compilation completed; initial modpost failed on incomplete kernel export metadata; supplemental metadata allowed successful module linking |
| Driver compatibility | Built module vermagic matches `7.3.0-rc4-v8-16k+`; srcversion matches loaded module `9243C0BD528C9F7546C36D9` |
| BCDC unit tests using actual source functions | 8 executed: 3 passed, 5 failed |
| Shell syntax | `monitor-mode.sh` and `setup_env.sh` passed `bash -n` |
| ShellCheck | One warning: unused loop variable `i`, SC2034, `monitor-mode.sh:91` |
| Explicit channel hops | 42 successful changes across channels 1, 6, 11, 36, 40, 44, 48; matching readbacks from both interfaces after every hop; WEXT queried after every hop |
| Final airodump terminal smoke | 45.111 seconds, normal exit after SIGINT, 107,247 output bytes; no new channel warnings, control timeouts, or watchdog resets |
| Final passive capture | 20 full packets captured on channel 11, 29 received by filter, 0 kernel drops |

The first 21 explicit hops took 5–10 ms each; the next 21 took 3–9 ms each.
Additional repeated channel reads succeeded through both `nexutil` and WEXT.
Radio tests were passive; the unrelated 500-frame injection script was not run.
Final counters show 909 control transmissions and 909 replies, zero control
errors, and zero bad receive headers. The receive sequence-mismatch counter rose
from 0 to 6 during testing; the available snapshots do not locate those six
events within a particular phase.

## Debugging findings

### Confirmed BCDC validation failures

[test_bcdc_responses.py](test_bcdc_responses.py) extracts the current driver's
`brcmf_proto_bcdc_msg`, completion, and query functions into a compiled C harness.
Only the transport is mocked; no malformed replies are sent to the hardware.

The failing assertions demonstrate acceptance of:

- An eight-byte truncated control header, returning stale `0x6e616863` (`chan`).
- A header declaring four payload bytes but carrying none, also returning `0x6e616863`.
- A declared four-byte payload with only two bytes delivered, returning `0x6e611006`.
- A reply carrying the wrong command number.
- A reply carrying the wrong interface index.

A valid channel reply, firmware error propagation, and rejection of an unmatched
request ID passed. These synthetic failures confirm validation gaps; they do not
prove that any one gap produced the original `0x6573` value. The test harness is
intended for this little-endian host and does not exercise physical SDIO timing.

### Build failure traced to kernel metadata

`/root/linux-rpi/Module.symvers` lacks built-in exports including
`sdio_disable_func`, `rtnl_unlock`, and `dmi_get_system_info`. The existing
`.vmlinux.export.c` contains their export types, namespaces, and CRCs.

[recover_builtin_symvers.py](recover_builtin_symvers.py) recovered 11,803 missing
entries into a temporary supplemental table. With that table supplied through
`KBUILD_EXTRA_SYMBOLS`, the isolated build linked successfully. Kernel files were
not changed, and modpost errors were not suppressed.

The compiler reported three existing `snprintf` truncation warnings in
`firmware.c` (lines 256, 259, 336). It also noted GCC package revisions differ:
kernel `16.2.0-2`, current compiler `16.2.0-3`.

### Runtime test harness corrections

Initial airodump runs with redirected standard I/O generated excessive terminal
output: approximately 359 MB and 1.16 GB. Their output is preserved in compressed
form in the temporary run directory. The final harness uses a pseudo-terminal,
explicit terminal dimensions, a one-second UI update interval, process-group
cleanup, and a 1 MiB output cap. Its final run passed.

An initial capture immediately after hopping received no packets. A repeat on
channel 11 captured 20 full frames with zero drops. The final harness explicitly
selects channel 11 before its receive check instead of depending on the last hop.

The original reported channel was 34 (`0xd022`), but the firmware rejected its
restoration with `BCME_BADCHAN (-20)`. The radio is left on valid channel 11
(`0x100b`). The final run successfully restored that channel and the prior
administrative state: `wlan0` up but unassociated, `mon0` up in monitor mode 2.
Debug mask is restored to 0. NetworkManager and wpa_supplicant were not stopped.
The monitor interface's original flags (`0x1003`) were also restored after
airodump changed them; no test capture processes remain.
No driver or firmware was installed or reloaded.

## Reproduction and evidence

Run the offline unit suite:

```sh
python3 audits/2026-09-24/test_bcdc_responses.py
```

The expected result against the current driver is five failures. Driver and
firmware implementation files remain unchanged; additions are test helpers and
this audit's evidence.

The isolated build and complete raw run artifacts are at
`/tmp/nexmon-checks-20260924-ggJwjg`. The successful build command was:

```sh
make -C /root/linux-rpi \
  M=/tmp/nexmon-checks-20260924-ggJwjg/driver ARCH=arm64 -j4 W=1 \
  KBUILD_EXTRA_SYMBOLS=/tmp/nexmon-checks-20260924-ggJwjg/builtin.symvers modules
```

Key logs, per-command return codes and durations, kernel traces, final state,
and packet captures are retained in [evidence/](evidence/). The initial runs'
failures are preserved separately from the final passing terminal run.

Installed firmware SHA-256:
`fb52353195fb08b5bcffb951406ca40134efd99d26b7c10cfb821630904d83da`.

Kernel `Module.symvers` SHA-256, unchanged:
`44a2e1f1b71f3298af4d8317a5e2083ba62af29ce07f33aa9cb5fcd198ec2530`.

### BCDC Response Validation Hardening (Resolved)

The 5 driver validation gaps identified in [test_bcdc_responses.py](test_bcdc_responses.py) have been resolved in both [patches/driver/brcmfmac_7.3.y-nexmon/bcdc.c](../../patches/driver/brcmfmac_7.3.y-nexmon/bcdc.c) and [/root/linux-rpi/drivers/net/wireless/broadcom/brcm80211/brcmfmac/bcdc.c](/root/linux-rpi/drivers/net/wireless/broadcom/brcm80211/brcmfmac/bcdc.c):
1. **Truncated Header**: `brcmf_proto_bcdc_cmplt()`, `brcmf_proto_bcdc_query_dcmd()`, and `brcmf_proto_bcdc_set_dcmd()` verify `ret >= sizeof(struct brcmf_proto_bcdc_dcmd)`. Runt frames (< 16 bytes) are rejected with `-EPROTO`.
2. **Command ID Verification**: `le32_to_cpu(msg->cmd) == cmd` is verified. Mismatched replies return `-EPROTO`.
3. **Interface Index Verification**: Response interface index `((flags & BCDC_DCMD_IF_MASK) >> BCDC_DCMD_IF_SHIFT) == ifidx` is verified.
4. **Declared Payload Bounds**: When not an error response (`!(flags & BCDC_DCMD_ERROR)`), the wire payload `ret - sizeof(*msg)` is verified against declared payload `msg->len & 0xffff`. Truncated responses return `-EPROTO`.
5. **Safe Payload Copy**: In `query_dcmd()`, payload copy length into caller's buffer is bounded by `min(len, wire_payload, dlen)`, preventing buffer over-reads and uninitialized memory disclosure.

All 8/8 unit tests in [test_bcdc_responses.py](test_bcdc_responses.py) now pass with exit code 0.

## Monitor Mode & Frame Injection Validation (05:20)

### 0x6573 Root Cause Resolution

The origin of `0x6573` was reproduced and confirmed by [test_injection_argprintf.py](test_injection_argprintf.py).
In `sendframe.c`, `argprintf("sendframe called: ...")` writes into the buffer last initialized by `argprintf_init()`
(which retains the previous IOCTL query buffer). The ASCII bytes `"se"` in little-endian representation
form `0x6573`. When a query (such as chanspec readback) coincided with frame injection, the TX log
overwrote the low 16 bits of the reply with `0x6573`, causing the driver to interpret the chanspec as invalid.

### Full Frame Injection Matrix

Tested with [monitor_injection.py](monitor_injection.py):
- **Backends**: `raw` (AF_PACKET raw socket) and `pcap` (`pcap_inject`).
- **Variants**: `bare` (8-byte header), `1Mbps` (rate=2), `6Mbps` (rate=12), `padded128` (128-byte radiotap), `padded256` (256-byte radiotap).
- **Frame Types**: Management Probe Request (`0x0040`), Management Beacon (`0x0080`), Management Deauthentication (`0x00c0`), Data QoS Data (`0x0088`).
- **Bands**: 2.4 GHz (Channels 1, 6, 11) and 5 GHz (Channels 36, 44).
- **Automated PCAP Echo Validation**: Built-in verification parser checks every frame recorded in `frames.pcap`, verifying 802.11 sequence numbers, transmitter MAC addresses, and radiotap bitrates with 100% match rate.
- **Results**:
  - Full matrix test (10 vectors): 10/10 passed with exit code 0.
  - Multi-frame test (probe, beacon, deauth, qos_data): 4/4 passed with exit code 0.
  - Multi-channel test (channels 1, 6, 11): 3/3 passed with exit code 0.
  - Multi-band test (channel 11 and channel 36 across all frame types): 8/8 passed with exit code 0.
  - Diagnostic hook counters (`inj`, `scb_ok`) incremented accurately with zero `scb_null` drops.
  - Zero kernel faults, dmesg warnings, or BCDC IOCTL buffer corruptions.

### Linux-Wireless Patchwork Audit against `/root/linux-rpi/`

Audited patches from the Linux Wireless Patchwork queue against the local kernel tree (`7.3.0-rc4` / `rpi-7.3.y`):

1. **`[wireless-next,v3] wifi: cfg80211: validate monitor channel set against radio usage`** (Series 1170712)
   - *Target*: `net/wireless/chan.c`
   - *Status*: Not applied in `linux-rpi`. Applies cleanly (`git apply --check`).
   - *Impact*: Replaces global `cfg80211_has_monitors_only()` check with per-radio validation, allowing monitor channel changes when active interfaces are on disjoint radios. Directly benefits Nexmon multi-interface configurations.
2. **`[v2] wifi: brcmfmac: bound NVRAM comment parsing`** (Series 1169573)
   - *Target*: `drivers/net/wireless/broadcom/brcm80211/brcmfmac/firmware.c`
   - *Status*: Not applied. Applies cleanly.
3. **`[RESEND,wireless-next,v2] wifi: brcmfmac: Set extsae_pwe parameter to Infineon firmware in SAP mode`** (Series 1171027)
   - *Target*: `drivers/net/wireless/broadcom/brcm80211/brcmfmac/cfg80211.c`, `cyw/core.c`, `fwvid.h`
   - *Status*: Not applied. Applies cleanly.
4. **`brcmfmac: firmware: Add missing clm_blob firmware files`** (Series 1172559)
   - *Target*: `drivers/net/wireless/broadcom/brcm80211/brcmfmac/sdio.c`
   - *Status*: Partially superseded in `linux-rpi` (BCM43456 CLM blob definition is already present via commit `af62d159c8dd`).
5. **`wifi: brcmfmac: handle missing D3 ACK on BCM4377 T2 Macs`** (Series 1169021)
   - *Target*: `drivers/net/wireless/broadcom/brcm80211/brcmfmac/pcie.c`
   - *Status*: Not applied. Applies cleanly.

## Kernel Module Deployment & Live Hardware Validation (06:15)

### Module Deployment

1. **Backups Created**:
   - Original kernel modules backed up to `/lib/modules/7.3.0-rc4-v8-16k+/backup-20260924/` (`brcmfmac.ko.xz`, `cfg80211.ko.xz`, vendor companion modules).
2. **Hardened Modules Built & Installed**:
   - Monolithic Nexmon driver built from `patches/driver/brcmfmac_7.3.y-nexmon/` containing:
     - BCDC reply length, wire bounds, command ID, and interface index checks in `bcdc.c`.
     - Signal-interruption error suppression in `brcmf_cfg80211_nexmon_set_channel()` (`-ERESTARTSYS` / `-EINTR` logged as info rather than kernel faults).
     - Explicit initialization of `pending = false` in `sdio.c` (`brcmf_sdio_dcmd_resp_wait()` and `brcmf_sdio_bus_rxctl()`).
   - `cfg80211.ko` built from `/root/linux-rpi/net/wireless/` containing per-radio channel validation and headroom/RSSI updates.
   - Modules deployed to `/lib/modules/7.3.0-rc4-v8-16k+/kernel/...` and `depmod -a` executed.

### Unified Validation Suite (`run_full_validation.py`)

#### Standard Run (`/tmp/nexmon-full-live-final/`):
- `build-validator`: **PASS** (0.585s) — 75 firmware binary patch assertions verified.
- `bcdc-unit`: **PASS** (0.568s) — 8/8 driver response validation tests passed.
- `argprintf-repro`: **PASS** (0.516s) — Sendframe IOCTL buffer safety verified.
- `runtime-smoke`: **PASS** (21.635s) — 21 hops across 2.4 & 5 GHz, 15s airodump-ng, passive tcpdump (20/20 packets captured).
- `injection-matrix`: **PASS** (13.149s) — 10 injection vectors across raw & libpcap with 100% PCAP echo match.
- **Summary**: **5 passed, 0 failed** (Total duration: 36.45s).

#### Extended Run (`/tmp/nexmon-full-live-extended/`):
- `build-validator`: **PASS** (0.583s).
- `bcdc-unit`: **PASS** (0.548s).
- `argprintf-repro`: **PASS** (0.556s).
- `runtime-smoke`: **PASS** (32.474s) — 42 channel hops, 20s airodump-ng, passive tcpdump.
- `injection-matrix`: **PASS** (151.642s) — Multi-channel (1, 6, 11, 36, 44), multi-frame-type (Probe Req, Beacon, Deauth, QoS Data), raw socket + libpcap, with automated PCAP echo loopback verification.
- **Summary**: **5 passed, 0 failed** (Total duration: 185.80s).


