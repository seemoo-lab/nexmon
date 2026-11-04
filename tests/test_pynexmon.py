#!/usr/bin/env python3
"""Unit tests for pynexmon Python SDK."""
import sys
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))

from pynexmon import Chanspec, NexmonDevice, NexmonError


class PyNexmonTests(unittest.TestCase):
    def test_chanspec_helpers(self):
        """Test Chanspec static utility methods."""
        # 2.4 GHz channel 6
        cs_6 = Chanspec.aton("6")
        self.assertEqual(cs_6, 0x1006)
        self.assertEqual(Chanspec.ntoa(cs_6), "6")
        self.assertEqual(Chanspec.to_mhz(cs_6), 2437)

        # 5 GHz channel 36 / 80 MHz
        cs_36_80 = Chanspec.aton("36/80")
        self.assertEqual(cs_36_80, 0xe02a)
        self.assertEqual(Chanspec.ntoa(cs_36_80), "36/80")
        self.assertEqual(Chanspec.to_mhz(cs_36_80), 5210)

        # 6 GHz channel 37 (PSC)
        cs_6g37 = Chanspec.aton("6g37")
        self.assertTrue(Chanspec.is_6g_psc(cs_6g37))
        self.assertEqual(Chanspec.to_mhz(cs_6g37), 6135)

        # 6 GHz channel 1 (non-PSC)
        cs_6g1 = Chanspec.aton("6g1")
        self.assertFalse(Chanspec.is_6g_psc(cs_6g1))

        # Invalid chanspec string raises ValueError
        with self.assertRaises(ValueError):
            Chanspec.aton("invalid_spec")

    def test_device_initialization_on_invalid_interface(self):
        """Test device error when initializing a non-existent network interface."""
        with self.assertRaises(NexmonError):
            NexmonDevice("nonexistent_iface_9999")


if __name__ == "__main__":
    unittest.main(verbosity=2)
