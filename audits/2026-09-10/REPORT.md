# Monitor mode and frame injection audit — 10 September 2026

**Overall: FAIL for usable monitor reception; over-the-air injection remains unverified.** The starting configuration loads the stock driver, which rejects monitor-interface creation. A temporary Nexmon driver correction makes interface control responsive, but monitor captures still contain zero incoming frames despite nearby access points being visible in managed mode.

The original stock driver and network configuration were restored after testing. No installed firmware, installed kernel module, or repository source file was replaced. The tested correction is provided separately in [driver-wakeup-fix.patch](driver-wakeup-fix.patch).

## Device and configuration

| Item | Observed value |
|---|---|
| Device | Raspberry Pi 5 Model B Rev 1.1 |
| Wireless hardware | BCM43455 family; driver identifies BCM4345/6, core revision 54 |
| Kernel | `7.3.0-rc2-v8-16k+`, aarch64, 16 KiB pages |
| Starting driver | Stock `brcmfmac`, srcversion `911E1F2235E9B184BD0A5EA`, with `brcmfmac_cyw` |
| Repository driver | `7.3.0-nexmon`, matching kernel vermagic; original srcversion `C590D14546B368EC9A62285` |
| Temporary corrected driver | Srcversion `40F476C1ECBE9A2AE55DF54` |
| Loaded firmware | `7.45.265 (28bca26 CY nexmon.org: d6b6-dirty-11)`, FWID `01-b677b91b` |
| Firmware target | `/usr/lib/firmware/nexmon/cyfmac43455-sdio-standard-265.bin` |
| Connectivity | Default route via Ethernet; Wi-Fi initially disconnected and managed by NetworkManager |
| Radios available | One physical Wi-Fi radio; no independent injection receiver |

Exact file hashes are in [artifact-hashes.json](artifact-hashes.json). Tests used the installed firmware throughout. The repository firmware binary is a different build, `e79e4-13`, and was not loaded.

## Findings

### 1. The starting driver does not support Nexmon monitor interfaces

`iw phy phy2 interface add mon0 type monitor` failed with `Operation not supported (-95)`. The original wiphy capabilities do not advertise monitor mode. Having patched firmware installed is insufficient with this loaded stock driver.

Evidence: [starting capabilities](evidence/phy-before.txt), [restored driver identity](evidence/final-driver.stdout).

### 2. The repository Nexmon driver has a confirmed wait/wake mismatch

In `patches/driver/brcmfmac_7.3.y-nexmon/sdio.c`, the control-response wait uses `TASK_KILLABLE`, and the control-transmit wait uses `wait_event_killable_timeout()`. Their completion functions still call `wake_up_interruptible()`. Kernel definitions show that this wake-up only selects `TASK_INTERRUPTIBLE`; killable waits include `TASK_UNINTERRUPTIBLE` instead.

The original module took several minutes to initialize. Its worker repeatedly waited in `brcmf_sdio_bus_txctl()` while registering the wiphy, also delaying other network-configuration operations. Reading monitor state took **2.564 seconds**; reading capabilities took **5.120 seconds**.

A copy of the driver was built under `/tmp` with just the two completion calls changed to `wake_up()`. The matching-kernel module build succeeded. With that candidate loaded, the same reads took **5 ms and 1 ms**, and monitor setup took **1.195 seconds**. Subsequent channel changes completed in milliseconds. This validates the correction for the observed control-path latency; it does not establish working monitor reception or radio transmission.

Evidence: [worker stack](evidence/original-driver-worker-stack.txt), [build log](evidence/driver-candidate-build.log), [command timings](evidence/commands.jsonl), [proposed patch](driver-wakeup-fix.patch).

### 3. Monitor reception fails with the installed firmware and corrected driver

All recorded monitor captures contain **zero incoming packets**:

| Check | Result |
|---|---|
| Channels 1, 6, 11, 13, 36, 40, 44, 48 | 8 seconds each; zero packets on every channel |
| Primary `wlan0` also up, explicit promiscuous mode | 8 seconds; zero packets |
| Passive monitor capture on channel 100 | 8 seconds; zero packets |
| Independent tshark capture on occupied channel 11 | 10 seconds; zero packets |
| Capture during 60 cross-band channel changes | Zero packets |
| `airodump-ng --band abg` | 45-second limit; zero discovered APs |

Monitor mode read back as `2`; promiscuous mode as `1`; scan suppression as `0`. Incoming tcpdump captures used `-Q in`, which excludes host transmit echoes. The independent tshark capture did not depend on that direction filter.

Managed-mode scanning with the same corrected driver and installed firmware received an AP on channel 11 at **−43 dBm**, with a fresh result reported as 0 ms old. It also saw an AP on channel 100 at **−49 dBm**. After returning to the stock driver, a fresh scan received the channel-11 AP at **−52 dBm**. Thus the monitor result cannot be explained simply by an absence of nearby radio traffic.

The exact firmware/driver cause of failed monitor delivery remains unresolved. Radiotap field accuracy, captured FCS, RSSI reporting, and frame completeness cannot be validated without incoming monitor frames.

Evidence: [capture counts and summary](summary.json), [managed scan with corrected driver](evidence/candidate-passive-recheck.stdout), [stock scan](evidence/stock-scan-dump.stdout), [tshark result](evidence/occupied-tshark.stderr). Packet captures are retained in `evidence/`.

### 4. Host injection submission works; successful radio injection is not demonstrated

The raw-socket and libpcap matrix used three channels, two APIs, six radiotap variants, and three repetitions: **108 probe-request frames** in total.

| Dimension | Coverage |
|---|---|
| Channels | 1, 11, 36; channel 11 independently confirmed occupied |
| APIs | `AF_PACKET` raw socket and `pcap_inject()` |
| Headers | Bare 8-byte header; rate-bearing 9-byte headers; valid padded 128- and 256-byte headers |
| Requested rates | Default, 1 Mbps on 2.4 GHz, 6 Mbps, 54 Mbps |
| Submission results | 108/108 calls returned the full requested byte count |
| Host echoes | 108 outgoing echoes identified using `PACKET_OUTGOING` |
| Incoming probe responses | **0** |

`aireplay-ng -9 mon0` was also run on channels 1, 11, and 36. Each reported `No Answer...` and `Found 0 APs`. Its exit status was zero despite that result, so exit status alone is not treated as an injection pass.

The first corrected-driver session recorded 78 SDIO data submissions, consistent with its 72 custom probe requests plus six aireplay broadcast probes, with zero SDIO transmit errors. That supports host-to-device submission, not proof of over-the-air delivery. The failed monitor receive path and absence of an independent receiver prevent separating a transmit failure from an inability to observe replies. Header parsing, actual on-air rates, ACK behavior, and reliable injection remain unverified.

Evidence: [channel 1 matrix](evidence/raw-injection-ch1.json), [channel 11 matrix](evidence/raw-injection-ch11.json), [channel 36 matrix](evidence/raw-injection-ch36.json), [occupied-channel aireplay output](evidence/aireplay-ch11.stdout), [SDIO counters](evidence/candidate-final-stats.stdout).

### 5. Additional compatibility and audit findings

- **Control operations pass with the correction:** 60/60 cross-band hops succeeded. 40 MHz and 80 MHz channel settings read back correctly. Disabled channel 14 was rejected with `EINVAL`. These are configuration checks, not reception/transmission bandwidth validation.
- **Lifecycle checks pass:** three consecutive monitor sessions, plus the final occupied-channel session, enabled mode `2` and tore down successfully. The first teardown restored `mpc=1` and the saved `qtxpower` value. No new firmware crash, kernel oops, or SDIO control timeout was recorded during the corrected-driver tests. The first session recorded 645 successful TX and RX control operations, with zero control errors.
- **The helper suppresses obsolete power-setting errors:** `monitor-mode.sh` sends legacy DCMD 66 twice during each activation. Both calls generate `-52` errors in the kernel log while setup still succeeds. The driver uses the working `qtxpower` iovar; the stale helper calls should be removed or corrected. The audit explicitly set test power to 10 dBm for the first and final injection sessions.
- **MAC-address handling differs:** the Nexmon driver used the fixed address in the generic NVRAM file (`b8:27:eb:74:f2:6c`), while the stock driver used the device address (`88:a2:9e:37:4d:fd`). This should be addressed before treating the driver as a complete Pi 5 port.
- **Source and installed firmware differ:** current repository source and its built firmware cannot automatically be used to explain the exact behavior of the older installed `d6b6-dirty-11` image. The repository monitor source also explicitly bypasses A-MSDU processing; complete aggregate-frame capture is not established.
- **Transient scan errors were resolved:** the first managed scan was aborted, and initial scans after stock reload returned `EBUSY`. Quiescing the supplicant and repeating the scan produced fresh AP results. These initial attempts are retained in the command ledger rather than counted as successful scans.
- **Airodump output limitation:** when captured through a pipe, its terminal display redrew excessively, producing about 1.06 GB of repetitive ANSI output. This is retained losslessly as `airodump-hopping.stdout.gz`, with a verified uncompressed SHA-256. That run checks driver survival under the hopping workload; it does not establish normal interactive CPU or display behavior.

## Restoration and remaining work

Final checks confirmed the original driver srcversion, original hardware MAC, no `mon0`, Wi-Fi managed and disconnected with power saving on, both NetworkManager and wpa_supplicant active, and the unchanged Ethernet default route. The stock driver rejected a manual channel-36 setting in managed mode; the disconnected interface is again under NetworkManager's channel control, so the instantaneous channel is not pinned to the initial value.

The two-line wake-up correction is ready for review but is **not installed or applied to repository source**. The next repair work is to trace monitor frame delivery using the exact installed firmware image, reconcile the Pi 5 NVRAM/MAC handling, and remove the helper's unsupported power ioctls. Final injection certification needs an independent monitor receiver or a functioning receive path with correlated responses from a test AP.

All feasible checks above were executed. Destructive traffic, association attacks, throughput flooding, and malformed-frame fuzzing were not part of this functional audit. Only probe requests were injected; channel 100 was receive-only. Existing workspace changes were preserved.

The [command ledger](evidence/commands.jsonl) records individual arguments, elapsed times, and return codes. Exit `124` on bounded captures/airodump represents the planned time limit. Supporting stdout, stderr, scan data, counters, kernel snapshots, and captures are in [evidence](evidence/). The [test helper](run_checks.py) implements the capture and probe matrices; privileged setup and restoration commands are recorded separately in the ledger.
