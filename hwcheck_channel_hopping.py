#!/usr/bin/env python3
"""Hardware check: monitor-mode channel-hopping stability.

RECONFIGURES THE RADIO. Hops across 2.4 and 5 GHz channels and then scans dmesg
for firmware timeouts, bus wedges and kernel warnings. Requires root and a
monitor vif brought up with monitor-mode.sh.

Renamed from test_channel_hopping.py and given a __main__ guard: the previous
version performed all 21 hops at import time, so merely collecting it with pytest
retuned the radio out from under any running capture.

Also fixed while here:
  - nexutil was invoked without -I, so the hops went to nexutil's default
    interface rather than the $iface this script reports on.
  - the readback was labelled "Verify current channel" but only printed; it is
    now compared against the requested channel and counted as a failure.
  - the script exited 0 even after printing "[-] Dmesg logged warnings or
    errors", so it could never fail a caller. It now returns non-zero.

Caveat: the dmesg window is taken by line-count slicing, which is wrong if the
kernel ring buffer wraps mid-run. That is much less likely now that the
per-frame BCDC/SDIO printk instrumentation was removed from the 7.3 driver.
"""
import re
import subprocess
import sys
import time

CHANNELS = ["1", "6", "11", "36", "40", "44", "48"]
ITERATIONS = 3
IFACE = "mon0"
NEXUTIL = "nexutil"
DWELL_S = 0.3
ERROR_KEYWORDS = ("timeout", "wedged", "failed", "oops", "warning",
                  "panic", "-110", "-52")


def nexutil(*args):
    return subprocess.run([NEXUTIL, "-I", IFACE, *args],
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          text=True)


def dmesg_lines():
    return subprocess.check_output(["dmesg"]).decode("utf-8", errors="ignore").splitlines()


def readback_channel(text):
    """Pull the control channel out of 'chanspec: 0xd024, 36'."""
    m = re.search(r",\s*(\d+)", text)
    return m.group(1) if m else None


def main():
    print(f"[*] Starting channel-hopping stability check on {IFACE}...")
    print(f"[*] Channels: {', '.join(CHANNELS)} | Iterations: {ITERATIONS}")

    dmesg_before = dmesg_lines()
    start_time = time.time()
    hop_count = 0
    mismatches = 0

    for i in range(ITERATIONS):
        print(f"\n--- Pass {i+1}/{ITERATIONS} ---")
        for ch in CHANNELS:
            t0 = time.time()
            ret = nexutil("-k", ch)
            if ret.returncode != 0:
                print(f"[-] ERROR switching to CH {ch}: "
                      f"{(ret.stderr or ret.stdout).strip()}")
                return 1

            curr = nexutil("-k").stdout.strip()
            dt = (time.time() - t0) * 1000
            got = readback_channel(curr)
            flag = "" if got == ch else "  <-- MISMATCH"
            if got != ch:
                mismatches += 1
            print(f"  -> Hop to CH {ch:2s} ({dt:5.1f} ms) | readback: {curr}{flag}")

            time.sleep(DWELL_S)
            hop_count += 1

    total_time = time.time() - start_time
    avg = (total_time / hop_count * 1000) if hop_count else 0.0
    print(f"\n[+] Completed {hop_count} hops across 2.4 GHz & 5 GHz "
          f"in {total_time:.2f}s (avg {avg:.1f}ms/hop)")

    new_lines = dmesg_lines()[len(dmesg_before):]
    errors = [l for l in new_lines
              if any(k in l.lower() for k in ERROR_KEYWORDS)]

    if errors:
        print("[-] Dmesg logged warnings or errors during channel-hopping:")
        for err in errors:
            print("    " + err)
    else:
        print("[+] DMESG CLEAN: 0 timeouts, 0 wedged bus errors, "
              "0 kernel warnings during entire hop test!")

    if mismatches:
        print(f"[-] {mismatches} hop(s) read back a different channel than requested")

    return 1 if (errors or mismatches) else 0


if __name__ == "__main__":
    sys.exit(main())
