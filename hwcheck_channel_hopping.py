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
import argparse
import re
import subprocess
import sys
import time

DEFAULT_CHANNELS = ["1", "6", "11", "36", "40", "44", "48"]
DEFAULT_ITERATIONS = 3
DEFAULT_IFACE = "mon0"
DEFAULT_NEXUTIL = "nexutil"
DEFAULT_DWELL_S = 0.3
ERROR_KEYWORDS = ("timeout", "wedged", "failed", "oops", "warning",
                  "panic", "-110", "-52")


def nexutil(cmd, iface, nex_bin, *args):
    return subprocess.run([nex_bin, "-I", iface, *args],
                          stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                          text=True)


def dmesg_lines():
    return subprocess.check_output(["dmesg"]).decode("utf-8", errors="ignore").splitlines()


def readback_channel(text):
    """Pull the control channel/chanspec out of 'chanspec: 0xd024, 36' or '36/80' or '6g37'."""
    m = re.search(r",\s*([0-9a-zA-Z_/]+)", text)
    if m:
        return m.group(1).strip()
    m2 = re.search(r"(6g\d+|\d+(?:/\d+)?)", text)
    return m2.group(1) if m2 else None


def channels_match(requested: str, readback: str) -> bool:
    """Compare requested channel against readback allowing bandwidth suffixes and 6g prefixes."""
    if not readback:
        return False
    if requested.lower() == readback.lower():
        return True
    req_base = requested.split("/")[0].lower()
    rb_base = readback.split("/")[0].lower()
    if req_base == rb_base:
        return True
    if req_base.lstrip("6g") == rb_base.lstrip("6g"):
        return True
    return False


def get_regulatory(iface, nex_bin):
    try:
        iw_out = subprocess.check_output(["iw", "reg", "get"], stderr=subprocess.DEVNULL, text=True)
        m = re.search(r"^country\s+([A-Z0-9]+):", iw_out, re.M)
        cfg_cc = m.group(1) if m else "unknown"
    except Exception:
        cfg_cc = "unknown"

    try:
        nex_out = subprocess.run([nex_bin, "-I", iface, "-g", "262", "-v", "country", "-l", "16", "-r"],
                                 stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, errors="ignore").stdout[:2]
        fw_cc = nex_out if re.match(r"^[A-Z0-9]{2}$", nex_out) else "unknown"
    except Exception:
        fw_cc = "unknown"
    return cfg_cc, fw_cc


def parse_channel_list(arg):
    chans = []
    for part in arg.split(","):
        part = part.strip()
        if ".." in part or "-" in part:
            sep = ".." if ".." in part else "-"
            start, end = part.split(sep, 1)
            prefix = "6g" if start.startswith("6g") else ""
            m_start = re.search(r'\d+', start)
            m_end = re.search(r'\d+', end)
            if m_start and m_end:
                s_num = int(m_start.group(0))
                e_num = int(m_end.group(0))
                chans.extend(f"{prefix}{x}" for x in range(s_num, e_num + 1))
            else:
                chans.append(part)
        elif part:
            chans.append(part)
    return chans


def main():
    parser = argparse.ArgumentParser(description="Nexmon monitor-mode channel-hopping stability check")
    parser.add_argument("-i", "--iface", default=DEFAULT_IFACE, help=f"Monitor interface (default: {DEFAULT_IFACE})")
    parser.add_argument("-c", "--channels", default=None,
                        help="Comma-separated list or range of channels (e.g. '1,6,11' or '36-48')")
    parser.add_argument("-n", "--iterations", type=int, default=DEFAULT_ITERATIONS,
                        help=f"Number of passes across channels (default: {DEFAULT_ITERATIONS})")
    parser.add_argument("-d", "--dwell", type=float, default=DEFAULT_DWELL_S,
                        help=f"Dwell time per channel in seconds (default: {DEFAULT_DWELL_S})")
    parser.add_argument("--nexutil", default=DEFAULT_NEXUTIL, help=f"Path to nexutil (default: {DEFAULT_NEXUTIL})")

    args = parser.parse_args()

    channels = parse_channel_list(args.channels) if args.channels else DEFAULT_CHANNELS
    iface = args.iface
    iterations = args.iterations
    dwell_s = args.dwell
    nex_bin = args.nexutil

    cfg_cc, fw_cc = get_regulatory(iface, nex_bin)
    print(f"[*] Starting channel-hopping stability check on {iface}...")
    print(f"[*] Regulatory Domain: cfg80211={cfg_cc} | firmware={fw_cc}" +
          ("" if cfg_cc == fw_cc else " (DESYNC WARNING - check 'monitor-mode.sh status')"))
    print(f"[*] Channels: {', '.join(channels)} | Iterations: {iterations} | Dwell: {dwell_s}s")

    dmesg_before = dmesg_lines()
    start_time = time.time()
    hop_count = 0
    mismatches = 0

    for i in range(iterations):
        print(f"\n--- Pass {i+1}/{iterations} ---")
        for ch in channels:
            t0 = time.time()
            ret = subprocess.run([nex_bin, "-I", iface, "-k", ch],
                                 stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                 text=True)
            if ret.returncode != 0:
                print(f"[-] ERROR switching to CH {ch}: "
                      f"{(ret.stderr or ret.stdout).strip()}")
                return 1

            curr = subprocess.run([nex_bin, "-I", iface, "-k"],
                                  stdout=subprocess.PIPE, stderr=subprocess.PIPE,
                                  text=True).stdout.strip()
            dt = (time.time() - t0) * 1000
            got = readback_channel(curr)
            is_match = channels_match(ch, got)
            flag = "" if is_match else "  <-- MISMATCH"
            if not is_match:
                mismatches += 1
            print(f"  -> Hop to CH {ch:2s} ({dt:5.1f} ms) | readback: {curr}{flag}")

            time.sleep(dwell_s)
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
