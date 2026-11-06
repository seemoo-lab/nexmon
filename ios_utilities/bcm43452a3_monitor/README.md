# Experimental iPhone SE monitor mode and passive WPA2 capture

Exact target: **iPhone SE2016/iPhone8,4, iOS15.7 build19H12, BCM43452a3 firmware7.64.133.15**. This standalone research utility reproduces the tested firmware-monitor route without the private Wi-Fi Map application. It is not support for arbitrary iPhones, a generic Nexmon target port, a password recovery tool or a Radiotap/injection implementation.

The confirmed capture retained eight original-FCS-valid raw802.11 frames: two complete M1-M4 exchanges from one authorized other station. Independent byte auditing matched the1372-byte PCAP and four HC22000 records to observed fields. No injection, deauthentication, forced remote disconnect or own-SE reconnect was used. No password was recovered or known-password MIC recomputation performed.

## Ready binary patch

A [prebuilt own-code binary patch](prebuilt/README.md) now generates the exact tested firmware from your own pinned stock image, using Python standard library only. No firmware compiler is required for this route. Full Apple/Broadcom firmware and ROM dumps are not redistributed. Native helpers still require the documented source build.

## Contents

- [Reproduce from source](REPRODUCING.md): exact build, signing/trust, scoped capture and recovery commands.
- [External dependencies](DEPENDENCIES.md): official download links, device-file paths, required versus research-only inputs and hashes.
- [Technical report](REPORT.md): missing host delivery, split-RX ownership, two-region composition, raw mirror and limitations.
- [Integration contract](INTEGRATION.md), [provenance](PROVENANCE.md), [target metadata](metadata/target.json), [GPLv3](LICENSE.txt).
- `build.py`: strict standalone source builder, verified to reproduce the exact live-tested image hash. No ROM dump or precompiled object is needed.
- `build_native.py`, `trust_native.py`, `capture.py`: build five own native helpers, trust them through pinned Dopamine, and run a bounded passive selected-AP session.
- `test_reproduction.py`:18 synthetic transport/FCS/ownership/recovery tests, with no radio or phone required.
- `evidence/`: aggregate actual-capture, storage, native build, offline repair, five/fifteen-minute service and standalone-reproduction results. Private packet bytes are intentionally absent.

## Validation scope

The firmware was rebuilt from these public sources using Zig0.13.0 and the owner's exact stock input, with full-image SHA256 equality. All five native helpers were rebuilt from the included sources on the SE, signed and self-tested. The standalone CLI then completed79coherent snapshots in one30-second control, followed by actual PID absence, free radio lock, stock/digest/autojoin restoration and Monitor0. That control contained no new EAPOL event. The earlier eight-frame exchange is separate original-byte evidence. The installed application passed a distinct15-minute/2011-snapshot empty-buffer lifecycle check with stable peak RSS.

Physical reboot/re-jailbreak, a second clean matching phone, locked-screen endurance and universal reception are unverified. Ordinary continuous Monitor Mode and the application's database/frontend are separate from this bounded standalone capture. Read the limits before operating the exact-target loader transaction.

`python3 -B test_reproduction.py` runs the portable tests. `python3 -B verify_bundle.py` verifies the allowlisted source distribution, privacy patterns, local links, manifest and any supplied archive. Neither command changes a phone or contacts the network.
