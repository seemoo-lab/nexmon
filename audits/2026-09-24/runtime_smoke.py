#!/usr/bin/env python3
"""Bounded passive RX/channel smoke test with logs and state restoration."""
import argparse
import fcntl
import json
import os
from pathlib import Path
import pty
import re
import select
import signal
import socket
import struct
import subprocess
import termios
import time

NEX = "/root/nexmon/utilities/nexutil/nexutil"
FAULT = re.compile(r"[Ii]nvalid chan|chanspec failed|chanspec empty|Set Channel failed(?!: chspec=\d+, -(?:512|4)\b)|timed out|bus wedged|WARNING:|Oops:|fw_crashed")


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("output", type=Path)
    parser.add_argument("--airodump-seconds", type=int, default=15)
    parser.add_argument("--debug", type=lambda value: int(value, 0), default=0x11090)
    parser.add_argument("--hops", type=int, default=21)
    args = parser.parse_args()
    if not 1 <= args.airodump_seconds <= 60:
        parser.error("--airodump-seconds must be between 1 and 60")
    if not 0 <= args.hops <= 42:
        parser.error("--hops must be between 0 and 42")
    args.output.mkdir(parents=True, exist_ok=True)
    ledger = []

    def run(name, command, timeout=8, accepted=(0,)):
        start = time.monotonic()
        try:
            result = subprocess.run(command, capture_output=True, text=True, timeout=timeout)
            rc, out, err = result.returncode, result.stdout, result.stderr
        except subprocess.TimeoutExpired as exc:
            rc = 124
            out = (exc.stdout or b"").decode(errors="replace")
            err = (exc.stderr or b"").decode(errors="replace")
        (args.output / f"{name}.stdout").write_text(out)
        (args.output / f"{name}.stderr").write_text(err)
        item = dict(name=name, command=command, rc=rc,
                    seconds=round(time.monotonic() - start, 3))
        ledger.append(item)
        print(json.dumps(item), flush=True)
        if rc not in accepted or re.search(r"ERR:|error on.*ioctl", err, re.I):
            raise RuntimeError(f"{name}: rc={rc}, {err.strip()}")
        return out

    def airodump():
        command = ["airodump-ng", "--update", "1", "--channel", "1,6,11,36,40,44,48", "mon0"]
        master, slave = pty.openpty()
        fcntl.ioctl(slave, termios.TIOCSWINSZ, struct.pack("HHHH", 24, 100, 0, 0))
        start = time.monotonic()
        proc = subprocess.Popen(command, stdin=slave, stdout=slave, stderr=slave,
                                start_new_session=True)
        os.close(slave)
        sent_stop = False
        total = 0
        try:
            with (args.output / "airodump.terminal").open("wb") as logfile:
                while proc.poll() is None:
                    elapsed = time.monotonic() - start
                    if elapsed >= args.airodump_seconds and not sent_stop:
                        os.killpg(proc.pid, signal.SIGINT)
                        sent_stop = True
                    if elapsed >= args.airodump_seconds + 3:
                        os.killpg(proc.pid, signal.SIGKILL)
                        break
                    if select.select([master], [], [], .1)[0]:
                        try:
                            data = os.read(master, 65536)
                        except OSError:
                            break
                        # Preserve at most 1 MiB even if the UI misbehaves.
                        logfile.write(data[:max(0, 1048576 - total)])
                        total += len(data)
            rc = proc.wait(timeout=5)
        finally:
            if proc.poll() is None:
                os.killpg(proc.pid, signal.SIGKILL)
                proc.wait(timeout=5)
            os.close(master)
        item = dict(name="airodump-pty", command=command, rc=rc,
                    seconds=round(time.monotonic() - start, 3), output_bytes=total,
                    stopped_at_deadline=sent_stop)
        ledger.append(item)
        print(json.dumps(item), flush=True)
        if not sent_stop or rc not in (0, -signal.SIGINT, 130):
            raise RuntimeError(f"airodump terminal run ended unexpectedly: {item}")

    before = run("dmesg-before", ["dmesg"])
    mark = max(float(x) for x in re.findall(r"^\[\s*([\d.]+)\]", before, re.M))
    original_up = int(Path("/sys/class/net/wlan0/flags").read_text(), 16) & 1
    original_mon_flags = int(Path("/sys/class/net/mon0/flags").read_text(), 16)
    original_debug = Path("/sys/module/brcmfmac/parameters/debug").read_text()
    original_spec = None
    changed = False
    failures = []

    def check_log(name):
        log = run(name, ["dmesg"])
        lines = []
        for line in log.splitlines():
            stamp = re.match(r"^\[\s*([\d.]+)\]", line)
            if stamp and float(stamp[1]) > mark:
                lines.append(line)
        (args.output / "dmesg-new.log").write_text("\n".join(lines) + "\n")
        errors = [line for line in lines if FAULT.search(line)]
        if errors:
            raise RuntimeError("New kernel fault: " + errors[0])

    try:
        routes = run("routes", ["ip", "route", "show", "default"])
        if "dev wlan0" in routes:
            raise RuntimeError("wlan0 carries the default route")
        if "Connected" in run("association", ["iw", "dev", "wlan0", "link"]):
            raise RuntimeError("wlan0 is associated")
        out = run("original-channel", [NEX, "-Iwlan0", "-k"])
        original_spec = re.search(r"chanspec: (0x[0-9a-f]+)", out)[1]
        # Control payload/firmware-interface tracing, scoped to this test.
        Path("/sys/module/brcmfmac/parameters/debug").write_text(str(args.debug))
        for i in range(5):
            for iface in ("wlan0", "mon0"):
                run(f"read-{i}-{iface}", [NEX, f"-I{iface}", "-k"])
            run(f"wext-{i}", ["iwconfig", "mon0"])
        check_log("dmesg-queries")
        changed = True
        run("primary-down", ["ip", "link", "set", "wlan0", "down"])
        for i in range(args.hops):
            channel = [1, 6, 11, 36, 40, 44, 48][i % 7]
            run(f"hop-{i}-{channel}", ["iw", "dev", "mon0", "set", "channel", str(channel)])
            for iface in ("wlan0", "mon0"):
                out = run(f"readback-{i}-{iface}", [NEX, f"-I{iface}", "-k"])
                actual = int(re.search(r"chanspec: 0x([0-9a-f]+)", out)[1], 16)
                expected = (0x1000 if channel < 15 else 0xD000) | channel
                if actual != expected:
                    raise RuntimeError(f"{iface}: expected {expected:#x}, got {actual:#x}")
            run(f"wext-hop-{i}", ["iwconfig", "mon0"])
            check_log(f"dmesg-hop-{i}")
            time.sleep(.2)
        # Receive only; bounded airodump exercises its WEXT channel reads/hops.
        airodump()
        check_log("dmesg-airodump")
        run("capture-channel", ["iw", "dev", "mon0", "set", "channel", "11"])
        run("passive-capture", ["timeout", "-s", "INT", "-k", "3", "8", "tcpdump",
                                "-i", "mon0", "-Q", "in", "-nn", "-s", "0",
                                "-c", "20", "-w", str(args.output / "passive.pcap")],
            timeout=14, accepted=(0, 124))
        captured = re.search(r"(\d+) packets captured",
                             (args.output / "passive-capture.stderr").read_text())
        if not captured or int(captured[1]) == 0:
            raise RuntimeError("Passive capture received no packets")
        check_log("dmesg-capture")
    except Exception as exc:
        failures.append(str(exc))
        print(f"FAIL: {exc}", flush=True)
    finally:
        if changed:
            # A watchdog reset may briefly remove both interfaces.
            for _ in range(10):
                if Path("/sys/class/net/wlan0").exists():
                    break
                time.sleep(1)
            try:
                run("restore-channel", [NEX, "-Iwlan0", "-i", f"-k{original_spec}"])
            except Exception:
                try:
                    run("restore-fallback-ch11", [NEX, "-Iwlan0", "-i", "-k0x100b"])
                except Exception as exc:
                    failures.append(str(exc))
            try:
                run("restore-primary", ["ip", "link", "set", "wlan0", "up" if original_up else "down"])
            except Exception as exc:
                failures.append(str(exc))
        try:
            Path("/sys/module/brcmfmac/parameters/debug").write_text(original_debug)
            with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as control:
                request = struct.pack("16sH22x", b"mon0", original_mon_flags)
                fcntl.ioctl(control.fileno(), 0x8914, request)  # SIOCSIFFLAGS
            check_log("dmesg-after")
        except Exception as exc:
            failures.append(str(exc))
        for name, command in [("interfaces-after", ["iw", "dev"]),
                              ("channel-after", [NEX, "-Iwlan0", "-k"])]:
            try:
                run(name, command)
            except Exception as exc:
                failures.append(str(exc))
        (args.output / "commands.json").write_text(json.dumps(ledger, indent=2) + "\n")
        (args.output / "result.json").write_text(json.dumps(dict(failures=failures), indent=2) + "\n")
    return bool(failures)


if __name__ == "__main__":
    raise SystemExit(main())
