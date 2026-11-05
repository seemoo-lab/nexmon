# Confirmed route and technical findings

## Exact target

iPhone8,4 (2016 SE, A1723), iOS15.7/19H12, BCM43452a3, firmware7.64.133.15. Compatibility hashes, toolchain identities and the partial live-ROM boundaries are in [target metadata](metadata/target.json). The live ROM view includes installed overlays and is not a virgin mask-ROM dump. Leading32bytes are unavailable; addresses and contracts must not be ported to a different chip/build without verification.

## Why ordinary monitor capture was insufficient

The BSD/BPF RFMON path previously caused watchdog resets and is not used. A native socket owner activates firmware Monitor Mode with GET_MONITOR107/SET_MONITOR108 and restores it when stopped. Ordinary BPF initially omitted split payloads. Bounded asynchronous reassembly allowed complete raw frames through the normal monitor-copy path, but later external-client tests still lacked required AP-to-client handshake bytes at the host boundary. The old unmatched137/193-byte fingerprints are not retroactively labelled M1/M3: they had insufficient original bytes to establish identity.

The working route copies selected complete EAPOL frames **before** the host monitor-copy boundary, preserving the original RX/fetch ownership path. This bypass establishes raw reception of the required frames; it does not prove that ordinary BPF now receives them. Original FCS is checked before retention. No PHY signal strength, Radiotap timestamp, channel-hopping coverage or universal packet-reception claim follows.

## Two-region composition

The primary reservation begins at0x199c00:488-byte fetch helper,448-byte capture module at0x199e00,24-byte ABI wrapper at0x199fc0, ending0x199fd8. The secondary reservation is the768-byte alignment region0x1ed100..0x1ed400, already RX/RO in the examined late MPU configuration. Its528-byte control handler and44-byte initializer end at0x1ed33c. Both known heap donors, modeled constructor/reclaim/rodata clearing and the examined upload path preserve the reservations. Live startup, protocol, frame capture and a separate five-minute service test pass on this target. These checks do not exhaust every possible dynamic firmware reference or prove lifetime on other devices.

The new `nxrawv1` state starts at0x1eeb30:64fixed bytes plus an allocated4096-byte ring, eight512-byte slots. Each slot contains16metadata bytes and up to496original frame bytes including FCS. Configuration contains one AP and two excluded own addresses, all supplied at runtime. Epoch is at offset52; an earlier draft at offset40 overlapped own-address storage and was corrected before any load. The separate magic is0x31574152/version1. Initialization is disarmed. Coherent64-byte reads reconstruct4160bytes; consumers reject changing/odd sequence, scope/epoch mismatch, bad ordinals, invalid lengths/FCS and unsuitable EAPOL payloads. Rearming resets the ring and advances epoch.

## Actual external-client result

After receiver readiness was verified, the owner reconnected a second authorized iPhone once using its Wi-Fi Settings toggle. The selected AP and both fresh SE addresses were configured; the initial80MHz channel specification was preserved.114coherent snapshots retained eight frames, two each of M1/M2/M3/M4, with zero observed ordinal gaps or busy reads. One non-SE station supplied two complete ordered exchanges with matching replay/ANonce/SNonce relationships, directions, descriptor/version, exact EAPOL lengths and WPA2-PSK context.

An independent parser, not importing the collection parser, matched every exported PCAP frame to original bytes with declared FCS removal. PCAP length1372bytes. Four HC22000 records,1600bytes, match actual M2 MIC/EAPOL and same-peer M1/M3 ANonce/replay fields. No synthetic packet headers, invented FCS, cross-peer pairing, password recovery or known-password MIC recomputation was used. Timestamps represent snapshot observations, not hardware arrival times.

After explicit stop: actual worker PID absent, radio lock free, stock image/digest/autojoin/authorized association restored, Monitor0, same boot. The earlier own-supplicant Ethernet tap remains a separate result requiring saved valid credentials; it must not be substituted for this passive result.

## Application integration and limits

The private phone app1.8.0build17 bundles the reader and exact-profile edit descriptions, derives the image from a locally verified stock input, uses a scoped on-device session and a bounded durable mobile spool, and exports stored evidence through authenticated AP details. The backend is the only database writer. The actual retained eight frames passed host/phone SQLite import/export checks; production downloads reproduce the original bytes/line set. An additive ordinal field preserves equal-observation-time ordering.23authentication tests and22product/transport tests pass.

The five-minute product trial performed674coherent empty snapshots with stable19890176-byte peak RSS and verified explicit-stop recovery; it is not another authentication exchange. Mobile offline repair restored a deliberately missing project-owned reader and rederived image without a firmware load. UI opening without a crash was confirmed. The packaged app subsequently installed successfully. A physical reboot/re-jailbreak cycle, another clean target device, long locked-screen coverage and universal reception remain unverified. The selected-AP raw capture is bounded; ordinary continuous Monitor Mode is a separate operation. The installed build17 package subsequently passed a separate fifteen-minute/2011-snapshot empty-buffer control with stable19939328-byte peak RSS, no observed gap/busy/error, explicit stop and same-boot stock/digest/autojoin/Monitor0 restoration. The fully public-source standalone builder reproduced the exact image; five rebuilt native helpers self-tested, and a thirty-second standalone CLI control completed79coherent snapshots with verified recovery. Neither later control is another handshake capture. See the aggregate evidence and external dependency sources.
