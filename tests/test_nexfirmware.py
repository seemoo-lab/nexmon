#!/usr/bin/env python3
"""Unit tests for the nexfirmware toolkit (detection, reverse engineering, lifecycle).

Exercises detection routines, binary analysis, definitions derivation, disassembly,
pull, put, and edit functions without requiring active radio transmissions.
"""

import shutil
import subprocess
import tempfile
import unittest
from pathlib import Path
from types import SimpleNamespace
from unittest import mock

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

    def test_strings_tool_uses_offsets_and_minimum_length(self):
        completed = subprocess.CompletedProcess(
            ["strings"], 0, stdout="10 firmware-version\n", stderr=""
        )
        with (
            mock.patch.object(
                nexfirmware.shutil, "which", return_value="/usr/bin/strings"
            ),
            mock.patch.object(
                nexfirmware.subprocess, "run", return_value=completed
            ) as run,
        ):
            result = nexfirmware.FirmwareReverseEngineer.extract_strings(
                str(self.stock_fw), min_length=8
            )

        command = run.call_args.args[0]
        self.assertEqual(command[:6], ["/usr/bin/strings", "-a", "-t", "x", "-n", "8"])
        self.assertEqual(result["output"], "10 firmware-version\n")

    def test_binwalk_report_can_be_saved(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-binwalk-") as tmp:
            output = Path(tmp) / "reports" / "binwalk.txt"
            completed = subprocess.CompletedProcess(
                ["binwalk"], 0, stdout="DECIMAL HEX DESCRIPTION\n", stderr=""
            )
            with (
                mock.patch.object(
                    nexfirmware.shutil, "which", return_value="/usr/bin/binwalk"
                ),
                mock.patch.object(
                    nexfirmware.subprocess, "run", return_value=completed
                ),
            ):
                result = nexfirmware.FirmwareReverseEngineer.scan_binwalk(
                    str(self.stock_fw), output_file=str(output)
                )

            self.assertEqual(output.read_text(), "DECIMAL HEX DESCRIPTION\n")
            self.assertEqual(result["output_path"], str(output))
            self.assertIsNone(result["output"])

    def test_requested_missing_analysis_tool_is_an_error(self):
        with mock.patch.object(nexfirmware.shutil, "which", return_value=None):
            with self.assertRaisesRegex(RuntimeError, "binwalk is not installed"):
                nexfirmware.FirmwareReverseEngineer.scan_binwalk(str(self.stock_fw))


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

    def test_edit_rejects_negative_patch_offset(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-edit-negative-") as tmp:
            source = Path(tmp) / "firmware.bin"
            source.write_bytes(b"abcdefgh")

            with self.assertRaisesRegex(ValueError, "non-negative"):
                nexfirmware.FirmwareManager.edit(
                    str(source), patches=[(-1, b"ZZ")]
                )

            self.assertEqual(source.read_bytes(), b"abcdefgh")

    def test_edit_rejects_patch_past_end_without_writing_output(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-edit-bounds-") as tmp:
            source = Path(tmp) / "firmware.bin"
            output = Path(tmp) / "edited.bin"
            source.write_bytes(b"abcdefgh")

            with self.assertRaisesRegex(ValueError, "exceeds file size"):
                nexfirmware.FirmwareManager.edit(
                    str(source), out_file=str(output), patches=[(7, b"ZZ")]
                )

            self.assertEqual(source.read_bytes(), b"abcdefgh")
            self.assertFalse(output.exists())

    def test_put_rejects_non_basename_destination(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-put-name-") as tmp:
            source = Path(tmp) / "firmware.bin"
            source.write_bytes(b"firmware")

            for invalid_name in (
                "",
                "../escaped.bin",
                "/tmp/escaped.bin",
                "sub/fw.bin",
                "..",
            ):
                with self.subTest(name=invalid_name):
                    with self.assertRaisesRegex(ValueError, "must be a basename"):
                        nexfirmware.FirmwareManager.put(
                            str(source),
                            dest_name=invalid_name,
                            update_alternatives=False,
                        )

    def test_put_atomically_replaces_existing_firmware(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-put-atomic-") as tmp:
            root = Path(tmp)
            install_dir = root / "install"
            install_dir.mkdir()
            source = root / "firmware.bin"
            source.write_bytes(b"N" * 100001)
            target = install_dir / "installed.bin"
            target.write_bytes(b"old firmware")
            detection = SimpleNamespace(active_firmware=None)

            with (
                mock.patch.object(nexfirmware.os, "geteuid", return_value=0),
                mock.patch.object(
                    nexfirmware.FirmwareManager, "INSTALL_DIR", install_dir
                ),
                mock.patch.object(
                    nexfirmware.DeviceDetector,
                    "full_detection",
                    return_value=detection,
                ),
            ):
                result = nexfirmware.FirmwareManager.put(
                    str(source),
                    dest_name="installed.bin",
                    update_alternatives=False,
                    backup_current=False,
                )

            self.assertEqual(target.read_bytes(), source.read_bytes())
            self.assertEqual(result["installed_path"], str(target))
            self.assertEqual(list(install_dir.glob(".*.tmp")), [])

    def test_put_keeps_existing_target_when_atomic_publish_fails(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-put-failure-") as tmp:
            root = Path(tmp)
            install_dir = root / "install"
            install_dir.mkdir()
            source = root / "firmware.bin"
            source.write_bytes(b"N" * 100001)
            target = install_dir / "installed.bin"
            target.write_bytes(b"old firmware")
            detection = SimpleNamespace(active_firmware=None)

            with (
                mock.patch.object(nexfirmware.os, "geteuid", return_value=0),
                mock.patch.object(
                    nexfirmware.FirmwareManager, "INSTALL_DIR", install_dir
                ),
                mock.patch.object(
                    nexfirmware.DeviceDetector,
                    "full_detection",
                    return_value=detection,
                ),
                mock.patch.object(
                    nexfirmware.os, "replace", side_effect=OSError("publish failed")
                ),
            ):
                with self.assertRaisesRegex(OSError, "publish failed"):
                    nexfirmware.FirmwareManager.put(
                        str(source),
                        dest_name="installed.bin",
                        update_alternatives=False,
                        backup_current=False,
                    )

            self.assertEqual(target.read_bytes(), b"old firmware")
            self.assertEqual(list(install_dir.glob(".*.tmp")), [])

    def test_put_propagates_activation_failure(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-put-activate-") as tmp:
            root = Path(tmp)
            install_dir = root / "install"
            source = root / "firmware.bin"
            source.write_bytes(b"N" * 100001)
            detection = SimpleNamespace(active_firmware=None)

            with (
                mock.patch.object(nexfirmware.os, "geteuid", return_value=0),
                mock.patch.object(
                    nexfirmware.FirmwareManager, "INSTALL_DIR", install_dir
                ),
                mock.patch.object(
                    nexfirmware.DeviceDetector,
                    "full_detection",
                    return_value=detection,
                ),
                mock.patch.object(
                    nexfirmware.shutil,
                    "which",
                    return_value="/usr/bin/update-alternatives",
                ),
                mock.patch.object(
                    nexfirmware.subprocess,
                    "run",
                    side_effect=subprocess.CalledProcessError(
                        1, ["update-alternatives"], stderr="activation failed"
                    ),
                ),
            ):
                with self.assertRaisesRegex(RuntimeError, "activation failed"):
                    nexfirmware.FirmwareManager.put(
                        str(source),
                        update_alternatives=True,
                        backup_current=False,
                    )

    def test_put_propagates_driver_reload_failure(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-put-reload-") as tmp:
            root = Path(tmp)
            install_dir = root / "install"
            source = root / "firmware.bin"
            source.write_bytes(b"N" * 100001)
            detection = SimpleNamespace(active_firmware=None)

            with (
                mock.patch.object(nexfirmware.os, "geteuid", return_value=0),
                mock.patch.object(
                    nexfirmware.FirmwareManager, "INSTALL_DIR", install_dir
                ),
                mock.patch.object(
                    nexfirmware.DeviceDetector,
                    "full_detection",
                    return_value=detection,
                ),
                mock.patch.object(
                    nexfirmware.shutil, "which", return_value="/usr/sbin/modprobe"
                ),
                mock.patch.object(
                    nexfirmware.subprocess,
                    "run",
                    side_effect=subprocess.CalledProcessError(
                        1, ["modprobe"], stderr="reload failed"
                    ),
                ),
            ):
                with self.assertRaisesRegex(RuntimeError, "reload failed"):
                    nexfirmware.FirmwareManager.put(
                        str(source),
                        update_alternatives=False,
                        reload_driver=True,
                        backup_current=False,
                    )

    @staticmethod
    def _restore_detection(active_path: Path):
        active_data = active_path.read_bytes()
        return SimpleNamespace(
            chip_name="BCM43455c0",
            chip_num=0x4345,
            chip_rev=6,
            active_firmware=SimpleNamespace(
                active_path=str(active_path),
                symlink_path="/etc/alternatives/cyfmac43455-sdio.bin",
                file_size=len(active_data),
                version_string=nexfirmware.extract_firmware_version(active_data),
                is_nexmon=b"nexmon" in active_data.lower(),
            ),
        )

    def test_find_original_prefers_registered_stock_and_rejects_nexmon(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-find-original-") as tmp:
            root = Path(tmp)
            active = root / "active-nexmon.bin"
            active.write_bytes(b"7.45.265 (28bca26 CY) nexmon.org" + b"\0" * 100000)
            stock = root / "cyfmac43455-sdio-standard.bin"
            stock.write_bytes(b"7.45.265 (28bca26 CY) stock" + b"\0" * 100000)
            patched_candidate = root / "cyfmac43455-sdio-minimal.bin"
            patched_candidate.write_bytes(b"NEXMON" + b"\0" * 100000)
            detection = self._restore_detection(active)

            with (
                mock.patch.object(
                    nexfirmware.FirmwareManager,
                    "_list_alternatives",
                    return_value=[patched_candidate, stock],
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager, "SYSTEM_FIRMWARE_DIRS", ()
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager,
                    "REPO_FIRMWARE_DIR",
                    root / "repository",
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager, "INSTALL_DIR", root / "installed"
                ),
            ):
                found = nexfirmware.FirmwareManager.find_original_firmware(detection)

            self.assertEqual(found["selected"]["path"], str(stock.resolve()))
            self.assertEqual(found["selected"]["origin"], "alternatives")
            self.assertNotIn(
                str(patched_candidate.resolve()),
                [candidate["path"] for candidate in found["candidates"]],
            )

    def test_restore_dry_run_finds_original_without_root_or_writes(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-restore-plan-") as tmp:
            root = Path(tmp)
            active = root / "active.bin"
            active_bytes = b"7.45.265 (28bca26 CY) nexmon.org" + b"\0" * 100000
            active.write_bytes(active_bytes)
            stock = root / "cyfmac43455-sdio-standard.bin"
            stock.write_bytes(b"7.45.265 (28bca26 CY) stock" + b"\0" * 100000)
            detection = self._restore_detection(active)

            with (
                mock.patch.object(
                    nexfirmware.DeviceDetector,
                    "full_detection",
                    return_value=detection,
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager,
                    "_list_alternatives",
                    return_value=[stock],
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager, "SYSTEM_FIRMWARE_DIRS", ()
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager,
                    "REPO_FIRMWARE_DIR",
                    root / "repository",
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager, "INSTALL_DIR", root / "installed"
                ),
                mock.patch.object(nexfirmware.os, "geteuid", return_value=1000),
                mock.patch.object(
                    nexfirmware.shutil,
                    "which",
                    return_value="/usr/bin/update-alternatives",
                ),
            ):
                result = nexfirmware.FirmwareManager.restore(dry_run=True)

            self.assertTrue(result["dry_run"])
            self.assertEqual(result["activation"], "update-alternatives")
            self.assertEqual(active.read_bytes(), active_bytes)
            self.assertFalse((root / "installed").exists())

    def test_restore_activates_registered_original_and_backs_up_current(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-restore-alt-") as tmp:
            root = Path(tmp)
            install_dir = root / "installed"
            active = root / "active.bin"
            active_bytes = b"7.45.265 (28bca26 CY) nexmon.org" + b"\0" * 100000
            active.write_bytes(active_bytes)
            stock = root / "cyfmac43455-sdio-standard.bin"
            stock.write_bytes(b"7.45.265 (28bca26 CY) stock" + b"\0" * 100000)
            detection = self._restore_detection(active)

            with (
                mock.patch.object(
                    nexfirmware.DeviceDetector,
                    "full_detection",
                    return_value=detection,
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager,
                    "_list_alternatives",
                    return_value=[stock],
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager, "SYSTEM_FIRMWARE_DIRS", ()
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager,
                    "REPO_FIRMWARE_DIR",
                    root / "repository",
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager, "INSTALL_DIR", install_dir
                ),
                mock.patch.object(nexfirmware.os, "geteuid", return_value=0),
                mock.patch.object(
                    nexfirmware.shutil,
                    "which",
                    return_value="/usr/bin/update-alternatives",
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager, "_run_required"
                ) as run_required,
            ):
                result = nexfirmware.FirmwareManager.restore()

            self.assertEqual(result["activation"], "update-alternatives")
            backup = Path(result["backup_created"])
            self.assertEqual(backup.read_bytes(), active_bytes)
            run_required.assert_called_once_with(
                [
                    "/usr/bin/update-alternatives",
                    "--set",
                    "cyfmac43455-sdio.bin",
                    str(stock),
                ],
                "original firmware activation",
            )

    def test_restore_falls_back_to_atomic_replacement(self):
        with tempfile.TemporaryDirectory(prefix="nex-test-restore-copy-") as tmp:
            root = Path(tmp)
            active = root / "active.bin"
            active.write_bytes(b"7.45.265 (28bca26 CY) nexmon.org" + b"\0" * 100000)
            stock = root / "cyfmac43455-sdio-standard.bin"
            stock_data = b"7.45.265 (28bca26 CY) stock" + b"\0" * 100000
            stock.write_bytes(stock_data)
            detection = self._restore_detection(active)

            with (
                mock.patch.object(
                    nexfirmware.DeviceDetector,
                    "full_detection",
                    return_value=detection,
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager,
                    "_list_alternatives",
                    return_value=[],
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager,
                    "SYSTEM_FIRMWARE_DIRS",
                    (root,),
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager,
                    "REPO_FIRMWARE_DIR",
                    root / "repository",
                ),
                mock.patch.object(
                    nexfirmware.FirmwareManager, "INSTALL_DIR", root / "installed"
                ),
                mock.patch.object(nexfirmware.os, "geteuid", return_value=0),
                mock.patch.object(nexfirmware.shutil, "which", return_value=None),
            ):
                result = nexfirmware.FirmwareManager.restore(backup_current=False)

            self.assertEqual(result["activation"], "atomic-replacement")
            self.assertEqual(active.read_bytes(), stock_data)


class TestFirmwareCli(unittest.TestCase):
    def test_reverse_tool_options(self):
        args = nexfirmware.build_parser().parse_args(
            ["reverse", "firmware.bin", "--strings", "--binwalk", "scan.txt"]
        )
        self.assertEqual(args.strings, "-")
        self.assertEqual(args.binwalk, "scan.txt")

    def test_restore_options(self):
        args = nexfirmware.build_parser().parse_args(
            ["restore", "--source", "stock.bin", "--reload", "--dry-run"]
        )
        self.assertEqual(args.source, "stock.bin")
        self.assertTrue(args.reload)
        self.assertTrue(args.dry_run)


if __name__ == "__main__":
    unittest.main(verbosity=2)
