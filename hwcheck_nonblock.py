#!/usr/bin/env python3
"""Hardware check: non-blocking TX throughput on a monitor interface.

TRANSMITS. Sends `count` probe requests on the monitor interface and reports
achievable packet rate plus how often the socket buffer was full. Requires root
and a monitor vif brought up with monitor-mode.sh.

Renamed from test_nonblock.py and given a __main__ guard: the previous version
bound an AF_PACKET socket and transmitted 500 frames at import time, so merely
collecting it with pytest put frames on air.
"""
import argparse
import select
import socket
import sys
import time

from scapy.all import Dot11, Dot11Elt, Dot11ProbeReq, RadioTap

DEFAULT_IFACE = "mon0"
DEFAULT_COUNT = 500
DEFAULT_TIMEOUT = 0.02


def main():
    parser = argparse.ArgumentParser(description="Nexmon monitor-mode non-blocking TX throughput check")
    parser.add_argument("-i", "--iface", default=DEFAULT_IFACE, help=f"Monitor interface (default: {DEFAULT_IFACE})")
    parser.add_argument("-c", "--count", type=int, default=DEFAULT_COUNT,
                        help=f"Number of frames to transmit (default: {DEFAULT_COUNT})")
    parser.add_argument("-t", "--timeout", type=float, default=DEFAULT_TIMEOUT,
                        help=f"Select timeout in seconds if buffer full (default: {DEFAULT_TIMEOUT})")

    args = parser.parse_args()

    pkt = (RadioTap()
           / Dot11(type=0, subtype=4,
                   addr1="ff:ff:ff:ff:ff:ff",
                   addr2="00:11:22:33:44:55",
                   addr3="ff:ff:ff:ff:ff:ff")
           / Dot11ProbeReq()
           / Dot11Elt(ID="SSID", info="TestNonblock"))
    raw_data = bytes(pkt)

    sock = socket.socket(socket.AF_PACKET, socket.SOCK_RAW)
    sent = 0
    dropped_full = 0
    try:
        sock.bind((args.iface, 0))
        sock.setblocking(False)

        t0 = time.time()
        for _ in range(args.count):
            # wait up to args.timeout if the socket buffer is temporarily full
            _, w, _ = select.select([], [sock], [], args.timeout)
            if w:
                try:
                    sock.send(raw_data)
                    sent += 1
                except BlockingIOError:
                    dropped_full += 1
            else:
                dropped_full += 1
        dt = time.time() - t0
    finally:
        sock.close()

    pps = sent / dt if dt > 0 else 0
    print(f"Sent {sent}/{args.count} frames on {args.iface} in {dt:.3f}s "
          f"({pps:.1f} pps, buffer_full={dropped_full})")

    if sent == 0:
        print("[-] nothing was accepted by the driver", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
