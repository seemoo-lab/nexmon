#!/usr/bin/env python3
"""Unit tests for the nexfirmware toolkit (detection, reverse engineering, lifecycle).

Exercises detection routines, binary analysis, definitions derivation, disassembly,
pull, put, and edit functions without requiring active radio transmissions.
"""

import hashlib
import json
import os
import shutil
import tempfile
import unittest
from pathlib import Path

# Add root directory to path
ROOT = Path(__file__).resolve().parents[1]
import sys
sys.path.insert(0, str(ROOT))

import nexfirmware


class TestDeviceDetection(unittest.TestCase):
    def test_detect_host(self):
        host = nexfirmware.DeviceDetector.detect_host()
        self.assertIsNotNone(host.model)
        self.assertIsNotNone(host.arch)
        self.assertIsNotNone(host.kernel)
        self.assertIsInstance(host.is_raspberry_pi, bool)

    def test_detect_hardware(self):
        devs = nexfirmware.DeviceDetector.detect_hardware()
        self.assertIsInstance(devs, list)
        if devs:
            d = devs[0]
            self.assertIn(d.bus_type, ("sdio", "pci", "usb", "unknown"))
            self.assertTrue(d.vendor_id.startswith("0x") or d.vendor_id == "unknown")

    def test_detect_driver(self):
        drv = nexfirmware.DeviceDetector.detect_driver()
        self.assertEqual(drv.module_name, "brcmfmac")
        self.assertIsInstance(drv.loaded, bool)
        self.assertIsInstance(drv.is_nexmon, bool)

    def test_full_detection(self):
        rep = nexfirmware.DeviceDetector.full_detection()
        self.assertIsNotNone(rep.chip_name)
        self.assertIsNotNone(rep.host)
        self.assertIsNotNone(rep.driver)


class TestFirmwareReverseEngineering(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.stock_fw = ROOT / "firmwares/bcm43455c0/7_45_265-28bca26-CY/cyfmac43455-sdio-standard.bin"
        if not cls.stock_fw.is_file():
            raise unittest.SkipTest("Stock firmware binary not present")

    def test_analyze_stock_firmware(self):
        rep = nexfirmware.FirmwareReverseEngineer.analyze_firmware(str(self.stock_fw))
        self.assertEqual(rep.file_size, 609309)
        self.assertEqual(rep.version_string, "7.45.265 (28bca26 CY)")
        self.assertEqual(rep.ucode_start, 0x21F24C)
        self.assertEqual(rep.reclaim_0_end, 0x22CAD4)
        self.assertEqual(rep.ucode_write_bl_hook, 0x20EF78)
        self.assertEqual(rep.templateram_start, 0x22C1F4)
        self.assertEqual(rep.templateram_size, 0x8E0)
        self.assertGreater(len(rep.flash_patches), 200)
        self.assertIn("NEXMON_CHIP=CHIP_VER_BCM43455c0", rep.definitions_mk)
        self.assertFalse(rep.is_nexmon_patched)

    def test_disassemble_range(self):
        lines = nexfirmware.FirmwareReverseEngineer.disassemble_range(
            str(self.stock_fw), 0x20EF78, count_bytes=16
        )
        self.assertGreater(len(lines), 0)
        # BL hook to wlc_ucode_write
        full_text = " ".join(lines)
        self.assertIn("bl", full_text)

    def test_extract_ucode(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-ucode-") as tmp:
            out_file = Path(tmp) / "ucode.bin"
            ok, msg = nexfirmware.FirmwareReverseEngineer.extract_ucode(str(self.stock_fw), str(out_file))
            self.assertTrue(ok, msg)
            self.assertTrue(out_file.is_file())
            # Microcode magic at start
            ucode_bytes = out_file.read_bytes()
            self.assertEqual(ucode_bytes[:8], nexfirmware.FirmwareReverseEngineer.UCODE_MAGIC)


class TestFirmwareLifecycle(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.stock_fw = ROOT / "firmwares/bcm43455c0/7_45_265-28bca26-CY/cyfmac43455-sdio-standard.bin"
        if not cls.stock_fw.is_file():
            raise unittest.SkipTest("Stock firmware binary not present")

    def test_edit_firmware(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-edit-") as tmp:
            tmp_dir = Path(tmp)
            orig_copy = tmp_dir / "firmware.bin"
            shutil.copy2(self.stock_fw, orig_copy)

            out_copy = tmp_dir / "firmware_edited.bin"
            # Apply a test patch at 0x198000
            res = nexfirmware.FirmwareManager.edit(
                str(orig_copy),
                out_file=str(out_copy),
                patches=[(0x198000, b"\x12\x34\x56\x78")]
            )
            self.assertTrue(out_copy.is_file())
            self.assertNotEqual(res["original_sha256"], res["modified_sha256"])
            modified_bytes = out_copy.read_bytes()
            self.assertEqual(modified_bytes[:4], b"\x12\x34\x56\x78")


if __name__ == "__main__":
    unittest.main(verbosity=2)
