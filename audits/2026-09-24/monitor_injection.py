#!/usr/bin/env python3
"""Bounded monitor and frame injection test suite with multi-channel and multi-frame-type support.

Validates:
- Injection across backends (AF_PACKET raw socket, libpcap)
- Radiotap header variants (bare, 1Mbps, 6Mbps, padded128, padded256)
- Frame types (probe_req, beacon, deauth, qos_data)
- Multi-channel operation (2.4 GHz and 5 GHz)
- Diagnostic counter integrity (inj increment, 0 scb_null drops)
- Automated post-run PCAP echo and sequence verification
"""
import argparse
import ctypes
import ctypes.util
import fcntl
import json
from pathlib import Path
import re
import socket
import struct
import subprocess
import time
import zlib

NEX = "/root/nexmon/utilities/nexutil/nexutil"
FAULT = re.compile(r"[Ii]nvalid chan|chanspec failed|timed out|bus wedged|WARNING:|Oops:|fw_crashed")


def build_80211_frame(frame_type, index, source):
    """Build an 802.11 frame of the specified type with source address and sequence number."""
    seq = struct.pack("<H", (index % 4096) << 4)
    if frame_type == "probe_req":
        return (struct.pack("<HH", 0x0040, 0) + b"\xff" * 6 + source + b"\xff" * 6
                + seq + b"\x00\x00\x01\x08\x82\x84\x8b\x96\x0c\x12\x18\x24")
    elif frame_type == "beacon":
        return (struct.pack("<HH", 0x0080, 0) + b"\xff" * 6 + source + source
                + seq + b"\x00" * 8 + b"\x64\x00\x11\x04" + b"\x00\x06NEXMON")
    elif frame_type == "deauth":
        return (struct.pack("<HH", 0x00c0, 0) + b"\xff" * 6 + source + source
                + seq + struct.pack("<H", 7))
    elif frame_type == "qos_data":
        return (struct.pack("<HH", 0x0088, 1) + b"\xff" * 6 + source + b"\xff" * 6
                + seq + b"\x00\x00" + b"NEXMON_FRAME_INJECT")
    else:
        raise ValueError(f"Unknown frame type: {frame_type}")


def verify_pcap_echoes(pcap_path, expected_sources_map):
    """Verify that all expected frames were recorded in the pcap file with correct sequence."""
    found = {}
    with open(pcap_path, "rb") as f:
        header = f.read(24)
        if len(header) < 24:
            return found
        while True:
            rec = f.read(16)
            if len(rec) < 16:
                break
            sec, usec, caplen, origlen = struct.unpack("<IIII", rec)
            packet = f.read(caplen)
            if len(packet) < 8 or packet[0] != 0:
                continue
            rtap_len = struct.unpack_from("<H", packet, 2)[0]
            if rtap_len < 8 or rtap_len > len(packet):
                continue
            body = packet[rtap_len:]
            if len(body) >= 24:
                sa = body[10:16]
                if sa in expected_sources_map:
                    idx = expected_sources_map[sa]
                    seq = struct.unpack_from("<H", body, 22)[0] >> 4
                    found[idx] = {
                        "sa": sa.hex(":"),
                        "seq": seq,
                        "caplen": caplen,
                        "rtap_len": rtap_len,
                        "matched_seq": seq == (idx % 4096)
                    }
    return found


def main():
    parser = argparse.ArgumentParser(description="Bounded monitor and frame injection test suite.")
    parser.add_argument("output", type=Path, help="Directory to save logs, PCAP, and results")
    parser.add_argument("--matrix", action="store_true",
                        help="Run full matrix (radiotap variants and libpcap backend)")
    parser.add_argument("--channels", type=str, default="11",
                        help="Comma-separated list of channels to test (e.g. '11' or '1,6,11,36')")
    parser.add_argument("--frame-types", type=str, default="probe_req",
                        help="Comma-separated frame types: probe_req, beacon, deauth, qos_data, or 'all'")
    parser.add_argument("--no-pcap-validate", action="store_true",
                        help="Skip post-run PCAP echo validation pass")
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)

    channels = [int(c.strip()) for c in args.channels.split(",") if c.strip()]
    if args.frame_types.lower() == "all":
        frame_types = ["probe_req", "beacon", "deauth", "qos_data"]
    else:
        frame_types = [ft.strip() for ft in args.frame_types.split(",") if ft.strip()]

    results = {"commands": [], "injections": [], "errors": [], "pcap_validation": {}}
    capture = (args.output / "frames.pcap").open("wb")
    capture.write(struct.pack("<IHHIIII", 0xa1b2c3d4, 2, 4, 0, 0, 65535, 127))
    initial_flags = {name: int(Path(f"/sys/class/net/{name}/flags").read_text(), 16)
                     for name in ("wlan0", "mon0")}
    saved_power = None
    rx = tx = lib = handle = None
    changed = False

    def run(name, cmd, timeout=8):
        start = time.monotonic()
        p = subprocess.run(cmd, capture_output=True, timeout=timeout)
        (args.output / f"{name}.stdout").write_bytes(p.stdout)
        (args.output / f"{name}.stderr").write_bytes(p.stderr)
        results["commands"].append(dict(name=name, args=cmd, rc=p.returncode,
                                         seconds=round(time.monotonic()-start, 3)))
        if p.returncode or b"ERR:" in p.stderr or b"error on" in p.stderr:
            raise RuntimeError(f"{name}: {p.returncode}: {p.stderr.decode(errors='replace')}")
        return p.stdout

    baseline = run("dmesg-before", ["dmesg"]).decode(errors="replace")
    mark = max(float(x) for x in re.findall(r"^\[\s*([\d.]+)\]", baseline, re.M))

    def check_kernel(tag):
        text = run(f"dmesg-{tag}", ["dmesg"]).decode(errors="replace")
        new = []
        for line in text.splitlines():
            stamp = re.match(r"^\[\s*([\d.]+)\]", line)
            if stamp and float(stamp[1]) > mark:
                new.append(line)
        (args.output / "dmesg-new.log").write_text("\n".join(new)+"\n")
        faults = [line for line in new if FAULT.search(line)]
        if faults:
            raise RuntimeError(faults[0])

    def diagnostic(tag):
        raw = run(f"diag-{tag}", [NEX, "-Iwlan0", "-g512", "-l1024", "-r"])
        text = raw.split(b"\0", 1)[0].decode(errors="replace")
        if not text.startswith("sf="):
            return {"sf": 0, "inj": 0, "scb_ok": 0, "scb_null": 0, "ret": 0, "rate": 0}
        fields = {key: int(value) for key, value in re.findall(r"\b(sf|inj|scb_ok|scb_null|ret|rate|txavail|in|out)=(-?\d+)", text)}
        print(json.dumps(dict(diagnostic=tag, **fields)), flush=True)
        return fields

    def observe(seconds, source=None):
        counts = dict(incoming=0, outgoing_echoes=0, probe_responses=0, beacons=0,
                      malformed_radiotap=0, fcs_valid=0, fcs_bad=0)
        end = time.monotonic()+seconds
        while time.monotonic() < end:
            try:
                data, addr = rx.recvfrom(65535)
            except socket.timeout:
                continue
            now = time.time()
            sec = int(now)
            capture.write(struct.pack("<IIII", sec, int((now-sec)*1e6), len(data), len(data)))
            capture.write(data)
            if len(data) < 8 or data[0] != 0:
                counts["malformed_radiotap"] += 1
                continue
            off = struct.unpack_from("<H", data, 2)[0]
            if off < 8 or off > len(data):
                counts["malformed_radiotap"] += 1
                continue
            body = data[off:]
            if addr[2] == socket.PACKET_OUTGOING:
                if source and len(body) >= 16 and body[10:16] == source:
                    counts["outgoing_echoes"] += 1
                continue
            counts["incoming"] += 1
            if off == 24 and struct.unpack_from("<I", data, 4)[0] == 0x6f and data[16] & 0x10 and len(body) >= 4:
                valid = zlib.crc32(body[:-4]) == struct.unpack_from("<I", body, len(body)-4)[0]
                counts["fcs_valid" if valid else "fcs_bad"] += 1
            if len(body) >= 24 and body[0] & 0xfc == 0x80:
                counts["beacons"] += 1
            if source and len(body) >= 24 and body[0] & 0xfc == 0x50 and body[4:10] == source:
                counts["probe_responses"] += 1
        capture.flush()
        return counts

    try:
        if b"dev wlan0" in run("routes", ["ip", "route", "show", "default"]):
            raise RuntimeError("Wi-Fi carries the default route")
        if b"Connected" in run("association", ["iw", "dev", "wlan0", "link"]):
            raise RuntimeError("wlan0 is associated")
        run("initial-channel", [NEX, "-Iwlan0", "-k"])
        power = run("power-before", [NEX, "-Iwlan0", "-g262", "-vqtxpower", "-l16", "-r"])
        if len(power) != 16 or power.startswith(b"qtxpower"):
            raise RuntimeError("Invalid TX power reply")
        saved_power = struct.unpack_from("<I", power)[0]
        changed = True
        run("primary-down", ["ip", "link", "set", "wlan0", "down"])
        run("monitor-down", ["ip", "link", "set", "mon0", "down"])
        mode = run("mode-off", [NEX, "-Iwlan0", "-m"])
        if b"monitor: 0" not in mode:
            raise RuntimeError("Monitor mode did not disable")
        run("monitor-up", ["ip", "link", "set", "mon0", "up"])
        mode = run("mode-on", [NEX, "-Imon0", "-m"])
        if b"monitor: 2" not in mode:
            raise RuntimeError("Monitor mode did not enable")

        rx = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
        rx.bind(("mon0", 0)); rx.settimeout(.05)
        tx = socket.socket(socket.AF_PACKET, socket.SOCK_RAW, socket.htons(3))
        tx.bind(("mon0", 0)); tx.settimeout(1)

        variants = [("bare", 8, None)]
        backends = ["raw"]
        if args.matrix:
            variants += [("1Mbps", 9, 2), ("6Mbps", 9, 12), ("padded128", 128, None), ("padded256", 256, None)]
            backends.append("pcap")
            lib = ctypes.CDLL(ctypes.util.find_library("pcap"))
            lib.pcap_open_live.argtypes = [ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_int, ctypes.c_char_p]
            lib.pcap_open_live.restype = ctypes.c_void_p
            lib.pcap_inject.argtypes = [ctypes.c_void_p, ctypes.c_void_p, ctypes.c_size_t]
            lib.pcap_inject.restype = ctypes.c_int
            lib.pcap_close.argtypes = [ctypes.c_void_p]
            error = ctypes.create_string_buffer(256)
            handle = lib.pcap_open_live(b"mon0", 65535, 0, 10, error)
            if not handle:
                raise RuntimeError(error.value.decode())

        expected_sources_map = {}

        for ch in channels:
            run(f"channel-{ch}", ["iw", "dev", "mon0", "set", "channel", str(ch)])
            spec_out = run(f"readback-spec-{ch}", [NEX, "-Imon0", "-k"]).decode(errors="replace")
            actual_spec = int(re.search(r"chanspec: 0x([0-9a-f]+)", spec_out)[1], 16)
            expected_spec = (0x1000 if ch < 15 else 0xD000) | ch
            if actual_spec != expected_spec:
                raise RuntimeError(f"Chanspec mismatch on ch {ch}: expected {expected_spec:#x}, got {actual_spec:#x}")

            if ch == 11:
                results["monitor_baseline"] = observe(3)
                print(json.dumps(dict(baseline=results["monitor_baseline"], channel=ch)), flush=True)
                if not results["monitor_baseline"]["incoming"]:
                    raise RuntimeError(f"No incoming monitor frames on occupied channel {ch}")
                check_kernel(f"baseline-ch{ch}")

            for ftype in frame_types:
                for backend in backends:
                    for label, rlen, rate in variants:
                        index = len(results["injections"]) + 1
                        source = bytes([2, 0x4e, 0x58, ch & 0xff, 24, index & 0xff])
                        expected_sources_map[source] = index
                        rtap = struct.pack("<BBHI", 0, 0, rlen, 4 if rate is not None else 0)
                        rtap = (rtap + (bytes([rate]) if rate is not None else b"")).ljust(rlen, b"\0")
                        frame = build_80211_frame(ftype, index, source)
                        packet = rtap + frame
                        before = diagnostic(f"{index}-before")
                        sent = tx.send(packet) if backend == "raw" else lib.pcap_inject(handle, packet, len(packet))
                        incoming = observe(.7, source)
                        after = diagnostic(f"{index}-after")
                        result = dict(backend=backend, variant=label, frame_type=ftype,
                                      channel=ch, source=source.hex(":"),
                                      requested_bytes=len(packet), accepted_bytes=sent, **incoming,
                                      before=before, after=after)
                        results["injections"].append(result)
                        print(json.dumps(result), flush=True)
                        check_kernel(f"frame-{index}")
                        if sent != len(packet):
                            raise RuntimeError(f"Frame #{index} ({ftype}) was not fully transmitted to kernel")
                        if after["inj"] > 0 and after["inj"] - before["inj"] != 1:
                            raise RuntimeError(f"Frame #{index} ({ftype}) was not accepted exactly once by firmware hook")
                        if after["scb_null"] > before["scb_null"]:
                            raise RuntimeError(f"Firmware dropped packet #{index} with a null SCB")

        results["monitor_after"] = observe(2)
        check_kernel("complete")

    except Exception as exc:
        results["errors"].append(str(exc))
        print(f"FAIL: {exc}", flush=True)
    finally:
        if handle:
            lib.pcap_close(handle)
        for sock in (rx, tx):
            if sock:
                sock.close()
        capture.close()

        # Validate captured PCAP echoes
        if not args.no_pcap_validate and not results["errors"] and results["injections"]:
            pcap_path = args.output / "frames.pcap"
            echo_matches = verify_pcap_echoes(pcap_path, expected_sources_map)
            all_matched = (len(echo_matches) == len(results["injections"]) and
                           all(m["matched_seq"] for m in echo_matches.values()))
            results["pcap_validation"] = {
                "total_injected": len(results["injections"]),
                "verified_echoes": len(echo_matches),
                "all_matched": all_matched,
                "details": {str(k): v for k, v in echo_matches.items()}
            }
            print(f"PCAP Validation: {len(echo_matches)}/{len(results['injections'])} frames verified in pcap (all_matched={all_matched})")
            if not all_matched:
                results["errors"].append(f"PCAP verification failed: {len(echo_matches)}/{len(results['injections'])} matched")

        if changed:
            cleanup = [("restore-channel11", [NEX, "-Iwlan0", "-k11"])]
            if saved_power is not None:
                import base64
                payload = base64.b64encode(b"qtxpower\0"+struct.pack("<I", saved_power)).decode()
                cleanup.append(("restore-power", [NEX, "-Iwlan0", "-s263", "-l13", "-b", "-v"+payload]))
            for name, command in cleanup:
                try:
                    run(name, command)
                except Exception as exc:
                    results["errors"].append(str(exc))
            for iface, flags in initial_flags.items():
                try:
                    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as control:
                        fcntl.ioctl(control.fileno(), 0x8914, struct.pack("16sH22x", iface.encode(), flags))
                except Exception as exc:
                    results["errors"].append(f"restore {iface}: {exc}")
        try:
            check_kernel("final")
        except Exception as exc:
            results["errors"].append(str(exc))
        (args.output / "results.json").write_text(json.dumps(results, indent=2)+"\n")
    return bool(results["errors"])


if __name__ == "__main__":
    raise SystemExit(main())
