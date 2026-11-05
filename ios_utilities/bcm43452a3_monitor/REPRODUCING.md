# Reproduction from zero on another matching phone

Only operate an owned or explicitly authorized network/device. A watchdog reset is possible. Use USB or another independent control path and retain backups. Do not use the previously failing host-BPF RFMON path. The procedure assumes a matching, already installed iOS and active Dopamine; it does not promise that Apple permits a downgrade to that build today.

## 1. Obtain the exact inputs

Check out this contribution and work in this directory. Review [DEPENDENCIES.md](DEPENDENCIES.md) and [target metadata](metadata/target.json). Copy the unmodified stock `hans.trx` from your compatible phone. Keep it outside the source tree. Obtain Zig0.13.0 from its official distribution. No compiled object, ROM dump or private research archive is required for the fixed-profile build.

On the phone, check iPhone8,4, iOS15.7/19H12, the stock image/loader hashes and Dopamine3.0.11's library hash. The standalone CLI enforces these again. Do not replace system inputs with reference binaries to bypass a mismatch. Stock resets firmware on reboot; manually restore Dopamine if required before resuming. Installation/re-jailbreak on a second clean device has not been tested by the authors.

## 2. Build the exact firmware image on a host

Use a private working directory and a separately installed Python3.9+ environment. POSIX shell examples follow; adapt environment-variable syntax for your host.

```sh
python3 -m venv "$NEXMON_WORK/venv"
"$NEXMON_WORK/venv/bin/python" -m pip install -r requirements.txt
"$NEXMON_WORK/venv/bin/python" -B test_reproduction.py
"$NEXMON_WORK/venv/bin/python" -B build.py --stock "$NEXMON_STOCK" --zig "$NEXMON_ZIG_013" --output "$NEXMON_WORK/image"
```

Set `NEXMON_WORK` to a new private directory, `NEXMON_STOCK` to your copied stock file, and `NEXMON_ZIG_013` to the Zig0.13.0 executable first. The builder checks the stock SHA256, compiler version, narrow ELF relocations, section bounds and final full-image SHA256. It must produce `0245b815b9ff0a5d7e7a3b4f42b491338d2dbeefd8f5bf66cf8a86a83fb00f1f`. A different compiler or chip is not an accepted substitute. It writes private candidate/object/edit/proof files to the new output directory; never commit or publish those binary outputs. The fixed-image hash binds this reproduction to the examined layout; this builder does not redo every ROM/MPU/reclaim experiment.

## 3. Build and sign phone helpers

Copy this source directory and the privately rebuilt `candidate.trx` to an owner-controlled location on the matching phone. Obtain lawful SDK/compiler/signing dependencies from the providers in [DEPENDENCIES.md](DEPENDENCIES.md). Do not redistribute the SDK. Run as root with Python3, Clang and ldid from the compatible jailbreak environment:

```sh
python3 -I -B build_native.py --sdk "$NEXMON_SDK" --cc clang --ldid ldid --output "$NEXMON_TOOLS"
python3 -I -B trust_native.py --tools "$NEXMON_TOOLS"
```

Set `NEXMON_SDK` to your SDK and `NEXMON_TOOLS` to a new root-owned private output directory. Signing permissions are emitted and read back by the builder. The manifest records all five compiled helpers; the trust step only accepts that exact inventory, root ownership/permissions, matching hashes and the pinned Dopamine library. It bootstraps `MemoryLauncher` before its first execution. No firmware or mode command runs during build/trust. Native object identity across different SDKs is not assumed; enforce the ABI and zero-radio self-tests. The tested reference build used Clang16 and an independently held SDK labelled15.6.

## 4. Start one bounded authorized capture

Stop other Wi-Fi scanners/monitor sessions, including Wi-Fi Map recording if installed. Connect the SE normally to the authorized AP first. The tested route preserves the complete initial20/40/80MHz channel specification. The CLI discovers the current AP/SSID and fresh own addresses; it does not reconnect the SE or attack another client. Choose a fresh private run directory:

```sh
"$NEXMON_TOOLS/MemoryLauncher" 128 root /var/jb/usr/bin/python3 -I -B capture.py --tools "$NEXMON_TOOLS" --candidate "$NEXMON_WORK/candidate.trx" --output "$NEXMON_WORK/run-001" --seconds 540
```

For an explicit additional scope check, pass `--bssid` with your authorized AP; it must equal the current association. Never paste that real value into a public report. Duration is5..7140seconds and retention caps1024frames. The process ignores SSH hangup, owns its radio locks and exits/restores on SIGINT/SIGTERM or the timer. Do not start a second instance to recover from an observation timeout.

The transaction verifies stock/loader/library/candidate, temporarily updates the pinned kernel **data** digest, calls Apple's stock firmware loader, then restores/readbacks the original digest. It does not permanently replace the system firmware, change boot arguments or install a kernel-code patch. The socket holder uses firmware commands107/108, not BPF RFMON. Runtime arm scope is one AP with both SE addresses excluded.

## 5. Establish readiness, then trigger one normal client event

From another local shell, inspect the private run's `state.json`. Require phase `capturing`, `ready:true`, a live recorded PID and at least one successful coherent snapshot. Only then reconnect a second owned/authorized client once using its normal Wi-Fi Settings off/on. The original experiment waited five seconds between off/on. Do not inject/deauthenticate or fabricate missing messages. No reconnect means no new handshake is guaranteed on an otherwise idle connected network.

The foreground CLI can be stopped with Ctrl-C. If detached, signal only its verified recorded PID; do not kill arbitrary Python processes. The bounded native owner is additional protection, not permission to reload firmware while a request is pending.

## 6. Verify cleanup and inspect private results

Require actual PID absence, released radio locks, phase `stopped`, `blocked:false`, Monitor0 and verified stock/digest/autojoin restoration. A still-running native request/loader, uncertain digest or nonzero Monitor blocks reload/retry. Preserve private logs and stop; an observed reboot requires a fresh owner instruction after jailbreak recovery.

Files `frames.ndjson`, `private-target.json`, PCAP and HC22000 contain private network/client/authentication material. The run directory is0700 and these files are0600. Keep them private. Successful exported pairs require original full bytes, valid FCS, appropriate WPA2 context, same-peer replay/nonce agreement and matching EAPOL fields. No pair/password is invented when required bytes are missing. Observation times are not hardware/Radiotap timestamps.

This standalone utility has no SQLite/frontend dependency. The separately tested private app stores compressed authentication frames linked to AP/capture IDs and exposes authenticated exports, but its application/DEB/user database are not required to reproduce this low-level result. See [the report](REPORT.md) for hardware evidence and limits.
