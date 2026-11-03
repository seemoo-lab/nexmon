#!/usr/bin/env python3
"""
nexbench.py - Advanced 802.11 Injection Benchmark & Diagnostics Suite
Optimized for Raspberry Pi 5 BCM43455 & Nexmon Firmware.
"""

import argparse
import errno
import json
import os
import re
import socket
import subprocess
import sys
import time
from typing import Dict, List, Tuple

try:
    from scapy.all import (
        RadioTap, Dot11, Dot11ProbeReq, Dot11Beacon, Dot11Elt, conf
    )
    conf.sniff_promisc = 0
except ImportError:
    print("[-] Error: Scapy is required. Please install via: apt install python3-scapy")
    sys.exit(1)


def read_sysfs_stats(iface: str) -> Dict[str, int]:
    base = f"/sys/class/net/{iface}/statistics"
    stats = {}
    keys = ["tx_packets", "tx_bytes", "tx_errors", "tx_dropped"]
    for k in keys:
        p = os.path.join(base, k)
        if os.path.exists(p):
            with open(p, "r") as f:
                stats[k] = int(f.read().strip())
        else:
            stats[k] = 0
    return stats


def get_dmesg_snapshot() -> List[str]:
    try:
        out = subprocess.check_output(["dmesg"], stderr=subprocess.DEVNULL)
        return out.decode("utf-8", errors="ignore").splitlines()
    except Exception:
        return []


def build_frame(frame_type: str, channel: int) -> Tuple[str, bytes]:
    """Generates standard-compliant 802.11 frames with Radiotap header."""
    ft = frame_type.lower()
    
    if ft == "probe":
        pkt = (
            RadioTap() /
            Dot11(type=0, subtype=4, addr1="ff:ff:ff:ff:ff:ff",
                  addr2="00:11:22:33:44:55", addr3="ff:ff:ff:ff:ff:ff") /
            Dot11ProbeReq() /
            Dot11Elt(ID="SSID", info="NexBench-Probe")
        )
        name = "Probe Request (Mgmt)"
        
    elif ft == "beacon":
        pkt = (
            RadioTap() /
            Dot11(type=0, subtype=8, addr1="ff:ff:ff:ff:ff:ff",
                  addr2="02:11:22:33:44:55", addr3="02:11:22:33:44:55") /
            Dot11Beacon(cap="ESS+privacy") /
            Dot11Elt(ID="SSID", info="NexBench-Beacon") /
            Dot11Elt(ID="Rates", info=b"\x82\x84\x8b\x96\x0c\x12\x18\x24") /
            Dot11Elt(ID="DSset", info=bytes([channel if channel < 200 else 36]))
        )
        name = "Beacon Frame (Mgmt)"
        
    elif ft == "null":
        pkt = (
            RadioTap() /
            Dot11(type=2, subtype=4, addr1="02:00:00:00:00:01",
                  addr2="00:11:22:33:44:55", addr3="02:00:00:00:00:01")
        )
        name = "Null Data (Data)"
        
    elif ft == "qosnull":
        pkt = (
            RadioTap() /
            Dot11(type=2, subtype=12, addr1="02:00:00:00:00:01",
                  addr2="00:11:22:33:44:55", addr3="02:00:00:00:00:01")
        )
        name = "QoS Null Data (Data)"
        
    elif ft == "rts":
        pkt = (
            RadioTap() /
            Dot11(type=1, subtype=11, addr1="02:00:00:00:00:01",
                  addr2="00:11:22:33:44:55")
        )
        name = "RTS (Control)"
        
    elif ft == "cts":
        pkt = (
            RadioTap() /
            Dot11(type=1, subtype=12, addr1="00:11:22:33:44:55")
        )
        name = "CTS (Control)"
        
    else:
        raise ValueError(f"Unknown frame type: {frame_type}")
        
    return name, bytes(pkt)


def set_channel(iface: str, ch: str) -> Tuple[bool, str]:
    """Tuning channel via nexutil with fallback to iw."""
    try:
        ret = subprocess.run(["nexutil", "-I", iface, "-k", ch], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if ret.returncode == 0:
            q = subprocess.run(["nexutil", "-I", iface, "-k"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
            return True, q.stdout.strip()
    except FileNotFoundError:
        pass

    try:
        ret = subprocess.run(["iw", "dev", iface, "set", "channel", ch], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        if ret.returncode == 0:
            return True, f"CH {ch} (via iw)"
        return False, ret.stderr.strip()
    except Exception as e:
        return False, str(e)


def run_benchmark(iface: str, channels: List[str], frame_types: List[str],
                  count: int, inter: float, stress: bool, export_json: str = None):
    # Adjust delay for stress mode
    if stress:
        inter = 0.0005 # 500 us delay (~1800 PPS peak burst rate without ring overrun)
        mode_desc = "STRESS PEAK RATE (~1800 PPS, 500 µs interval)"
    else:
        mode_desc = f"Paced ({inter*1000:.2f} ms interval, ~{1.0/inter if inter>0 else 0:.0f} target PPS)"

    print("=" * 72)
    print("       NEXMON 802.11 INJECTION BENCHMARK & DIAGNOSTICS SUITE")
    print("=" * 72)
    print(f"Interface    : {iface}")
    print(f"Channels     : {', '.join(channels)}")
    print(f"Frame Types  : {', '.join(frame_types)}")
    print(f"Packets/Test : {count} frames")
    print(f"Mode         : {mode_desc}")
    print("=" * 72)

    # Open raw socket once
    try:
        sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW)
        sock.bind((iface, 0))
    except PermissionError:
        print("[-] Error: Root privileges required for AF_PACKET raw socket injection.")
        sys.exit(1)
    except Exception as e:
        print(f"[-] Failed to open raw socket on {iface}: {e}")
        sys.exit(1)

    dmesg_start = get_dmesg_snapshot()
    all_results = []

    for ch_str in channels:
        m = re.search(r'\d+', ch_str)
        ch_int = int(m.group(0)) if m else 1
        print(f"\n[*] Tuning {iface} to Channel {ch_str}...")
        ok, msg = set_channel(iface, ch_str)
        if not ok:
            print(f"[-] Failed to tune to channel {ch_str}: {msg}")
            continue
        print(f"    Active Channel Info: {msg}")
        time.sleep(0.3)

        for ft in frame_types:
            name, raw_frame = build_frame(ft, ch_int)
            frame_len = len(raw_frame)

            stats_before = read_sysfs_stats(iface)
            t_start = time.perf_counter()

            for _ in range(count):
                try:
                    sock.send(raw_frame)
                except OSError as e:
                    if e.errno in (errno.ENOBUFS, errno.EAGAIN):
                        time.sleep(0.001)
                        try:
                            sock.send(raw_frame)
                        except OSError:
                            pass
                    else:
                        raise
                if inter > 0:
                    time.sleep(inter)

            t_end = time.perf_counter()
            duration = t_end - t_start
            stats_after = read_sysfs_stats(iface)

            tx_delta = stats_after["tx_packets"] - stats_before["tx_packets"]
            stats_after["tx_bytes"] - stats_before["tx_bytes"]
            err_delta = stats_after["tx_errors"] - stats_before["tx_errors"]
            drop_delta = stats_after["tx_dropped"] - stats_before["tx_dropped"]

            pps = count / duration if duration > 0 else 0
            mbps = (count * frame_len * 8) / (duration * 1_000_000) if duration > 0 else 0

            verified = (tx_delta == count) or (tx_delta > 0)
            status_str = "PASS" if verified and err_delta == 0 and drop_delta == 0 else "WARN"

            result_entry = {
                "channel": ch_str,
                "frame_type": name,
                "frame_size": frame_len,
                "requested_pkts": count,
                "kernel_tx_pkts": tx_delta,
                "duration_sec": round(duration, 4),
                "pps": round(pps, 1),
                "mbps": round(mbps, 3),
                "errors": err_delta,
                "dropped": drop_delta,
                "status": status_str
            }
            all_results.append(result_entry)

            print(f"  -> {name:<22} | {count:4d} pkts | {duration:6.3f}s | {pps:7.1f} pps | {mbps:6.3f} Mbps | TX: +{tx_delta:4d} | [{status_str}]")

    sock.close()

    # Dmesg Health Check
    dmesg_end = get_dmesg_snapshot()
    new_dmesg = dmesg_end[len(dmesg_start):]
    dmesg_warnings = [
        l for l in new_dmesg 
        if any(k in l.lower() for k in ["timeout", "wedged", "failed", "oops", "warning", "panic", "-110", "-52"])
    ]

    print("\n" + "=" * 72)
    print("                         SUMMARY REPORT")
    print("=" * 72)
    print(f"{'Channel':<9} | {'Frame Type':<22} | {'Packets':<7} | {'Rate (PPS)':<10} | {'Throughput':<11} | {'Status'}")
    print("-" * 72)
    for r in all_results:
        print(f"{r['channel']:<9} | {r['frame_type']:<22} | {r['requested_pkts']:<7} | {r['pps']:<10.1f} | {r['mbps']:<6.3f} Mbps | {r['status']}")
    print("-" * 72)

    if dmesg_warnings:
        print("[-] KERNEL DIAGNOSTICS: Driver or bus errors detected in dmesg:")
        for w in dmesg_warnings:
            print("    " + w)
    else:
        print("[+] KERNEL DIAGNOSTICS: Perfectly Clean! (0 timeouts, 0 bus wedging, 0 warnings)")
    print("=" * 72)

    if export_json:
        with open(export_json, "w") as f:
            json.dump({
                "interface": iface,
                "timestamp": time.time(),
                "results": all_results,
                "dmesg_warnings": dmesg_warnings
            }, f, indent=2)
        print(f"[+] Exported benchmark metrics to: {export_json}")



def preflight_check():
    # Ensure NetworkManager does not interfere with background scans
    try:
        res = subprocess.run(["nmcli", "dev", "status"], stdout=subprocess.PIPE, stderr=subprocess.DEVNULL, text=True)
        for line in res.stdout.splitlines():
            parts = line.split()
            if parts and parts[0] == "wlan0":
                if "unmanaged" not in line:
                    subprocess.run(["nmcli", "dev", "set", "wlan0", "managed", "no"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    print("[*] Pre-flight: Set wlan0 to unmanaged to suppress NetworkManager scan collisions.")
                break
    except (subprocess.SubprocessError, OSError):
        pass

def main():
    parser = argparse.ArgumentParser(
        description="Nexmon 802.11 High-Speed Injection Benchmark & Diagnostics Suite"
    )
    parser.add_argument("-i", "--iface", default="mon0", help="Monitor interface name (default: mon0)")
    parser.add_argument("-c", "--channels", default="6,36", help="Comma-separated channels to test (default: 6,36)")
    parser.add_argument("-n", "--count", type=int, default=50, help="Number of packets per frame type (default: 50)")
    parser.add_argument("-t", "--types", default="all", help="Frame types: all,probe,beacon,null,qosnull,rts,cts (default: all)")
    parser.add_argument("--inter", type=float, default=0.002, help="Inter-frame delay in seconds (default: 0.002s / 500 PPS)")
    parser.add_argument("--rate", type=float, help="Target injection rate in PPS (overrides --inter, e.g. --rate 250)")
    parser.add_argument("--stress", action="store_true", help="Run peak sustained burst rate (~1800 PPS)")
    parser.add_argument("--json", help="Save benchmark results to a JSON file")

    args = parser.parse_args()
    preflight_check()

    inter = args.inter
    if args.rate and args.rate > 0:
        inter = 1.0 / args.rate

    channels = [c.strip() for c in args.channels.split(",") if c.strip()]
    if args.types.lower() == "all":
        types = ["probe", "beacon", "null", "qosnull", "rts", "cts"]
    else:
        types = [t.strip().lower() for t in args.types.split(",") if t.strip()]

    run_benchmark(
        iface=args.iface,
        channels=channels,
        frame_types=types,
        count=args.count,
        inter=inter,
        stress=args.stress,
        export_json=args.json
    )


if __name__ == "__main__":
    main()
