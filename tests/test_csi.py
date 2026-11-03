#!/usr/bin/env python3
"""Unit tests for python/pynexmon/csi.py (Nexmon CSI pipeline)."""

import math
import os
import socket
import struct
import sys
import unittest

NEXMON_ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, os.path.join(NEXMON_ROOT, "python"))

from pynexmon.csi import (
    CSIReceiver,
    NetlinkCSIReceiver,
    UDPCSIReceiver,
    parse_csi_payload,
    resolve_genl_family,
    NEXMON_CSI_MAGIC,
    NEXMON_CSI_CMD_SAMPLE,
    NEXMON_CSI_ATTR_RAW,
    NEXMON_CSI_ATTR_TSFT,
    NEXMON_CSI_ATTR_IFINDEX,
)


def create_synthetic_csi_frame(
    magic=NEXMON_CSI_MAGIC,
    mac=b"\x00\x11\x22\x33\x44\x55",
    seq=42,
    core=1,
    stream=0,
    chanspec=0x1809,
    chip_id=0x4345,
    num_subcarriers=64,
):
    """Generate a synthetic Nexmon CSI UDP payload."""
    core_spatial = (stream << 8) | (core & 0xFF)
    header = struct.pack(
        "<I6sHHHH",
        magic,
        mac,
        seq,
        core_spatial,
        chanspec,
        chip_id,
    )
    # Generate test I/Q pairs: I = subcarrier index, Q = -subcarrier index
    iq_data = bytearray()
    for sc in range(num_subcarriers):
        i_val = sc * 10
        q_val = -sc * 5
        iq_data.extend(struct.pack("<hh", i_val, q_val))

    return bytes(header + iq_data)


class TestCSIPipeline(unittest.TestCase):
    def test_parse_csi_payload(self):
        payload = create_synthetic_csi_frame(num_subcarriers=64)
        pkt = parse_csi_payload(payload)

        self.assertEqual(pkt.magic, NEXMON_CSI_MAGIC)
        self.assertEqual(pkt.src_mac, "00:11:22:33:44:55")
        self.assertEqual(pkt.seq, 42)
        self.assertEqual(pkt.core, 1)
        self.assertEqual(pkt.spatial_stream, 0)
        self.assertEqual(pkt.chanspec, 0x1809)
        self.assertEqual(pkt.chip_id, 0x4345)
        self.assertEqual(pkt.num_subcarriers, 64)
        self.assertEqual(pkt.bandwidth_mhz, 20)

        # Check first and last subcarrier I/Q values
        self.assertEqual(pkt.raw_iq[0], (0, 0))
        self.assertEqual(pkt.raw_iq[1], (10, -5))
        self.assertEqual(pkt.complex_csi[1], complex(10, -5))
        self.assertAlmostEqual(pkt.amplitudes[1], math.sqrt(100 + 25))

    def test_bandwidth_estimation(self):
        pkt20 = parse_csi_payload(create_synthetic_csi_frame(num_subcarriers=64))
        pkt40 = parse_csi_payload(create_synthetic_csi_frame(num_subcarriers=128))
        pkt80 = parse_csi_payload(create_synthetic_csi_frame(num_subcarriers=256))
        pkt160 = parse_csi_payload(create_synthetic_csi_frame(num_subcarriers=512))

        self.assertEqual(pkt20.bandwidth_mhz, 20)
        self.assertEqual(pkt40.bandwidth_mhz, 40)
        self.assertEqual(pkt80.bandwidth_mhz, 80)
        self.assertEqual(pkt160.bandwidth_mhz, 160)

    def test_payload_too_short(self):
        with self.assertRaises(ValueError):
            parse_csi_payload(b"\x11\x11\x11\x11\x00")

    def test_serialization(self):
        payload = create_synthetic_csi_frame(num_subcarriers=64)
        pkt = parse_csi_payload(payload)
        d = pkt.to_dict()
        self.assertEqual(d["src_mac"], "00:11:22:33:44:55")
        self.assertEqual(d["num_subcarriers"], 64)
        self.assertIn("raw_iq", d)

        j = pkt.to_json()
        self.assertIn('"num_subcarriers": 64', j)
        self.assertIn('"src_mac": "00:11:22:33:44:55"', j)

    def test_numpy_conversion(self):
        try:
            import numpy as np
        except ImportError:
            self.skipTest("NumPy not installed")

        payload = create_synthetic_csi_frame(num_subcarriers=64)
        pkt = parse_csi_payload(payload)

        arr = pkt.to_numpy()
        self.assertEqual(arr.shape, (64,))
        self.assertEqual(arr.dtype, np.complex128)
        self.assertEqual(arr[1], np.complex128(10 - 5j))

        amp = pkt.amplitudes_numpy()
        self.assertEqual(amp.shape, (64,))
        self.assertAlmostEqual(float(amp[1]), math.sqrt(125))

        ph = pkt.phases_numpy()
        self.assertEqual(ph.shape, (64,))
        self.assertAlmostEqual(float(ph[1]), math.atan2(-5, 10))

    def test_udp_receiver_loopback(self):
        # Pick an unused port for loopback testing
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.bind(("127.0.0.1", 0))
            test_port = s.getsockname()[1]

        rx = CSIReceiver(host="127.0.0.1", port=test_port)
        with rx:
            # Send synthetic packet to loopback
            payload = create_synthetic_csi_frame(seq=999, num_subcarriers=64)
            sender = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            sender.sendto(payload, ("127.0.0.1", test_port))
            sender.close()

            pkt = rx.recv(timeout=1.0)
            self.assertIsNotNone(pkt)
            assert pkt is not None
            self.assertEqual(pkt.seq, 999)
            self.assertEqual(pkt.num_subcarriers, 64)

    def test_resolve_genl_family(self):
        try:
            fid, groups = resolve_genl_family("nl80211")
            self.assertIsInstance(fid, int)
            self.assertGreater(fid, 0)
            self.assertIsInstance(groups, dict)
        except OSError:
            self.skipTest("AF_NETLINK not available on this platform")

        with self.assertRaises(RuntimeError):
            resolve_genl_family("definitely_nonexistent_family_12345")

    def test_netlink_csi_receiver_synthetic_decode(self):
        raw_csi = create_synthetic_csi_frame(seq=1234, num_subcarriers=64)
        tsft_us = 9876543210
        ifindex = 3

        # Attr 1 (RAW)
        nla1_len = 4 + len(raw_csi)
        pad1 = (4 - (len(raw_csi) % 4)) % 4
        nla1 = struct.pack("<HH", nla1_len, NEXMON_CSI_ATTR_RAW) + raw_csi + (b"\0" * pad1)

        # Attr 2 (TSFT: u64)
        nla2 = struct.pack("<HHQ", 12, NEXMON_CSI_ATTR_TSFT, tsft_us)

        # Attr 3 (IFINDEX: u32)
        nla3 = struct.pack("<HHI", 8, NEXMON_CSI_ATTR_IFINDEX, ifindex)

        attrs = nla1 + nla2 + nla3
        genl_hdr = struct.pack("<BBH", NEXMON_CSI_CMD_SAMPLE, 1, 0)
        family_id = 99
        nl_len = 16 + len(genl_hdr) + len(attrs)
        nl_hdr = struct.pack("<IHHII", nl_len, family_id, 0, 1, 0)

        packet_bytes = nl_hdr + genl_hdr + attrs

        rx = NetlinkCSIReceiver()
        rx._family_id = family_id

        class MockSocket:
            def settimeout(self, t):
                pass
            def recv(self, bufsize):
                return packet_bytes

        rx._sock = MockSocket()
        pkt = rx.recv(timeout=1.0)
        self.assertIsNotNone(pkt)
        assert pkt is not None
        self.assertEqual(pkt.seq, 1234)
        self.assertEqual(pkt.tsft_us, tsft_us)
        self.assertEqual(pkt.ifindex, ifindex)
        self.assertAlmostEqual(pkt.timestamp, tsft_us / 1e6)
        self.assertEqual(pkt.num_subcarriers, 64)

    def test_csi_receiver_mode_selection(self):
        rx_udp = CSIReceiver(mode="udp", port=5500)
        self.assertEqual(rx_udp.mode, "udp")
        rx_udp.start()
        self.assertEqual(rx_udp.backend, "udp")
        rx_udp.close()

        rx_custom_port = CSIReceiver(mode="auto", port=5599)
        rx_custom_port.start()
        self.assertEqual(rx_custom_port.backend, "udp")
        rx_custom_port.close()

        rx_bad_nl = CSIReceiver(mode="netlink", family_name="nonexistent_family_xyz")
        with self.assertRaises(Exception):
            rx_bad_nl.start()


if __name__ == "__main__":
    unittest.main()
