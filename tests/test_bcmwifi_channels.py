#!/usr/bin/env python3
"""Unit tests for Broadcom chanspec encoding/decoding and validation (bcmwifi_channels.c)."""
import ctypes
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
INVCHANSPEC = 0


class BcmwifiChannelsTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix="nexmon-bcmwifi-")
        path = Path(cls.tmp.name)
        so_path = path / "libbcmwifi_test.so"
        channels_c = ROOT / "utilities/nexutil/bcmwifi_channels.c"
        utils_c = ROOT / "utilities/nexutil/bcmutils.c"
        inc_nexutil = ROOT / "utilities/nexutil/include"
        inc_patches = ROOT / "patches/include"

        subprocess.run(
            [
                "gcc", "-shared", "-fPIC", "-std=gnu11", "-O2",
                str(channels_c), str(utils_c),
                f"-I{inc_nexutil}", f"-I{inc_patches}",
                "-o", str(so_path)
            ],
            check=True,
            timeout=30
        )

        cls.lib = ctypes.CDLL(str(so_path))

        # Function signatures
        cls.lib.wf_chspec_aton.argtypes = [ctypes.c_char_p]
        cls.lib.wf_chspec_aton.restype = ctypes.c_uint16

        cls.lib.wf_chspec_ntoa_ex.argtypes = [ctypes.c_uint16, ctypes.c_char_p]
        cls.lib.wf_chspec_ntoa_ex.restype = ctypes.c_char_p

        cls.lib.wf_chspec_is_6g_psc.argtypes = [ctypes.c_uint16]
        cls.lib.wf_chspec_is_6g_psc.restype = ctypes.c_bool

        cls.lib.wf_chspec_malformed.argtypes = [ctypes.c_uint16]
        cls.lib.wf_chspec_malformed.restype = ctypes.c_bool

        cls.lib.wf_chspec_valid.argtypes = [ctypes.c_uint16]
        cls.lib.wf_chspec_valid.restype = ctypes.c_bool

        cls.lib.wf_chanspec2mhz.argtypes = [ctypes.c_uint16]
        cls.lib.wf_chanspec2mhz.restype = ctypes.c_int

        cls.lib.wf_mhz2channel.argtypes = [ctypes.c_uint, ctypes.c_uint]
        cls.lib.wf_mhz2channel.restype = ctypes.c_int

        cls.lib.wf_chspec_overlap.argtypes = [ctypes.c_uint16, ctypes.c_uint16]
        cls.lib.wf_chspec_overlap.restype = ctypes.c_bool

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def _ntoa(self, cs: int) -> str:
        buf = ctypes.create_string_buffer(64)
        res = self.lib.wf_chspec_ntoa_ex(cs, buf)
        return res.decode() if res else ""

    def test_2g_channel_parsing_and_roundtrip(self):
        """Test standard 2.4 GHz channels (1-14) parse correctly and round-trip."""
        for ch in range(1, 15):
            ch_str = str(ch).encode()
            cs = self.lib.wf_chspec_aton(ch_str)
            self.assertNotEqual(cs, INVCHANSPEC, f"Failed to parse 2.4GHz channel {ch}")
            self.assertFalse(self.lib.wf_chspec_malformed(cs), f"Channel {ch} marked malformed")
            self.assertTrue(self.lib.wf_chspec_valid(cs), f"Channel {ch} marked invalid")
            out_str = self._ntoa(cs)
            self.assertEqual(out_str, str(ch), f"Round-trip failed for channel {ch}")

    def test_2g_40mhz_sidebands(self):
        """Test 2.4 GHz 40 MHz upper and lower sideband channels."""
        for ch, sb in [(1, b"1l"), (6, b"6u"), (6, b"6l"), (11, b"11u")]:
            cs = self.lib.wf_chspec_aton(sb)
            self.assertNotEqual(cs, INVCHANSPEC, f"Failed to parse 2.4GHz 40MHz channel {sb.decode()}")
            self.assertFalse(self.lib.wf_chspec_malformed(cs))
            self.assertTrue(self.lib.wf_chspec_valid(cs))

    def test_5g_channel_parsing(self):
        """Test 5 GHz channels at 20, 40, 80, and 160 MHz bandwidths."""
        cases = [
            (b"36", 5180),
            (b"40", 5200),
            (b"44", 5220),
            (b"48", 5240),
            (b"149", 5745),
            (b"157", 5785),
            (b"36/40", 5190),
            (b"36/80", 5210),
            (b"36/160", 5250),
        ]
        for spec_str, expected_mhz in cases:
            cs = self.lib.wf_chspec_aton(spec_str)
            self.assertNotEqual(cs, INVCHANSPEC, f"Failed to parse {spec_str.decode()}")
            self.assertFalse(self.lib.wf_chspec_malformed(cs))
            self.assertTrue(self.lib.wf_chspec_valid(cs))
            mhz = self.lib.wf_chanspec2mhz(cs)
            self.assertEqual(mhz, expected_mhz, f"Frequency mismatch for {spec_str.decode()}: got {mhz}, want {expected_mhz}")

    def test_6g_channel_and_psc_detection(self):
        """Test 6 GHz channels and Preferred Scanning Channel (PSC) detection."""
        # 6 GHz PSC channels: 5, 21, 37, 53, 69, 85, 101, 117, 133, 149, 165, 181, 197, 213, 229
        psc_channels = [5, 21, 37, 53, 69, 85, 101, 117, 133, 149, 165, 181, 197, 213, 229]
        non_psc_channels = [1, 9, 17, 25, 33, 41, 49, 57, 65, 73, 81]

        for ch in psc_channels:
            spec = f"6g{ch}".encode()
            cs = self.lib.wf_chspec_aton(spec)
            self.assertNotEqual(cs, INVCHANSPEC, f"Failed to parse PSC channel {spec.decode()}")
            self.assertFalse(self.lib.wf_chspec_malformed(cs))
            self.assertTrue(self.lib.wf_chspec_valid(cs))
            self.assertTrue(self.lib.wf_chspec_is_6g_psc(cs), f"Channel {ch} should be identified as PSC")

        for ch in non_psc_channels:
            spec = f"6g{ch}".encode()
            cs = self.lib.wf_chspec_aton(spec)
            self.assertNotEqual(cs, INVCHANSPEC, f"Failed to parse non-PSC channel {spec.decode()}")
            self.assertFalse(self.lib.wf_chspec_malformed(cs))
            self.assertTrue(self.lib.wf_chspec_valid(cs))
            self.assertFalse(self.lib.wf_chspec_is_6g_psc(cs), f"Channel {ch} should NOT be identified as PSC")

    def test_malformed_and_invalid_chanspecs(self):
        """Test rejection of malformed or invalid chanspecs."""
        self.assertTrue(self.lib.wf_chspec_malformed(0x0000))
        self.assertTrue(self.lib.wf_chspec_malformed(0xFFFF))
        self.assertFalse(self.lib.wf_chspec_valid(0x0000))
        self.assertFalse(self.lib.wf_chspec_valid(0xFFFF))

        # Invalid strings
        invalid_strings = [
            b"",
            b"invalid",
            b"-1",
            b"0",
            b"999",
            b"15/80",
            b"6g999",
            b"200/160",
        ]
        for inv in invalid_strings:
            cs = self.lib.wf_chspec_aton(inv)
            self.assertEqual(cs, INVCHANSPEC, f"Invalid string '{inv.decode()}' should parse to INVCHANSPEC (0)")

    def test_frequency_to_channel_conversions(self):
        """Test frequency to channel conversions."""
        # 2.4 GHz start factor = 0
        self.assertEqual(self.lib.wf_mhz2channel(2412, 0), 1)
        self.assertEqual(self.lib.wf_mhz2channel(2437, 0), 6)
        self.assertEqual(self.lib.wf_mhz2channel(2462, 0), 11)
        self.assertEqual(self.lib.wf_mhz2channel(2484, 0), 14)

    def test_channel_overlap(self):
        """Test detection of overlapping channel allocations."""
        cs_ch1 = self.lib.wf_chspec_aton(b"1")
        cs_ch6 = self.lib.wf_chspec_aton(b"6")
        cs_36_80 = self.lib.wf_chspec_aton(b"36/80")
        cs_40_20 = self.lib.wf_chspec_aton(b"40")
        cs_52_80 = self.lib.wf_chspec_aton(b"52/80")

        # 36/80 covers channels 36, 40, 44, 48 -> overlaps with channel 40
        self.assertTrue(self.lib.wf_chspec_overlap(cs_36_80, cs_40_20))
        # 36/80 does not overlap with 52/80
        self.assertFalse(self.lib.wf_chspec_overlap(cs_36_80, cs_52_80))
        # 2.4 GHz channel 1 does not overlap with 5 GHz channel 36
        self.assertFalse(self.lib.wf_chspec_overlap(cs_ch1, cs_36_80))


if __name__ == "__main__":
    unittest.main(verbosity=2)
