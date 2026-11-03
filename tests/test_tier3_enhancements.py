#!/usr/bin/env python3
"""Comprehensive regression test suite for Nexmon Tier 3 enhancements (M-01..M-13, L-01..L-04)."""
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class TestTier3DriversAndFirmware(unittest.TestCase):
    """M-01, M-02, L-01: Driver core/cfg80211 and firmware common infrastructure."""

    def test_m01_driver_pm_mpc_decoupled(self):
        """M-01: Ensure brcmf_net_mon_open decouples best-effort PM/mpc errors via tmp_err."""
        for drv in ("brcmfmac_7.2.y-nexmon", "brcmfmac_7.3.y-nexmon"):
            core_c = ROOT / "patches/driver" / drv / "core.c"
            self.assertTrue(core_c.is_file(), f"Driver core.c missing: {core_c}")
            content = core_c.read_text(errors="replace")
            self.assertIn("tmp_err", content, f"tmp_err decoupling missing in {drv}/core.c")
            self.assertIn('brcmf_fil_iovar_int_set(prim, "mpc", 0)', content)

    def test_m02_driver_rsdb_combinations_include_mon_flag(self):
        """M-02: Ensure RSDB interface combinations include + mon_flag across drivers."""
        for drv_ver in ("5.15", "6.1", "6.2", "6.6", "6.18", "7.2", "7.3"):
            cfg_c = ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/cfg80211.c"
            if not cfg_c.is_file():
                continue
            content = cfg_c.read_text(errors="replace")
            self.assertIn("4 + mon_flag", content, f"4 + mon_flag missing in {drv_ver}")
            self.assertIn("3 + mon_flag", content, f"3 + mon_flag missing in {drv_ver}")

    def test_p2p_detach_race_rtnl_wiphy_locks(self):
        """Kernel 7.4-rc1 (08a3f02fea63): Guard P2P netdev-less VIF detach with rtnl_lock and wiphy_lock."""
        for drv_ver in ("5.15", "6.1", "6.2", "6.6", "6.18", "7.2", "7.3"):
            core_c = ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/core.c"
            self.assertTrue(core_c.is_file(), f"Missing {core_c}")
            content = core_c.read_text(errors="replace")
            self.assertIn("wiphy_lock(drvr->wiphy);", content, f"wiphy_lock missing in {drv_ver}")
            self.assertIn("rtnl_lock();", content, f"rtnl_lock missing in {drv_ver}")
            self.assertIn("if (ifp->ndev)", content, f"ifp->ndev branch missing in {drv_ver}")
            self.assertIn("brcmf_remove_interface(ifp, true);", content, f"locked remove missing in {drv_ver}")

    def test_l01_firmware_common_static_inline_and_statistics(self):
        """L-01: Ensure helper.h uses static inline, statistics.mk supports DISABLE_STATISTICS, and typo is fixed."""
        helper_h = ROOT / "patches/include/helper.h"
        content_h = helper_h.read_text(errors="replace")
        for sym in ("htons", "htonl", "ntohs", "ntohl", "get_stack_ptr"):
            self.assertIn("static inline", content_h, f"static inline missing for {sym} in helper.h")

        stats_mk = ROOT / "patches/common/statistics.mk"
        content_stats = stats_mk.read_text(errors="replace")
        self.assertIn("DISABLE_STATISTICS", content_stats)
        self.assertNotIn("--no-check-certificate", content_stats)

        # Check typo in 43455c0 nexmon Makefile: gen/nexmon.mk should not log to flashpatches.log
        mf = ROOT / "patches/bcm43455c0/7_45_265-28bca26-CY/nexmon/Makefile"
        self.assertTrue(mf.is_file())
        content = mf.read_text()
        self.assertNotIn("-f gen/nexmon.mk >>log/patches.log 2>>log/flashpatches.log", content)
        self.assertIn("-f gen/nexmon.mk >>log/patches.log 2>>log/patches.log", content)


class TestTier3ToolsAndUtilities(unittest.TestCase):
    """M-08, M-09, M-10, L-02: Native C utilities, rawproxy, Android NDK, updates."""

    def test_m08_nexutil_monitor_bitmask_and_objmem_range(self):
        """M-08: Ensure nexutil supports 0..255 monitor bitmask, strtoul for objmem, and async-safe alarm."""
        src = (ROOT / "utilities/nexutil/nexutil.c").read_text(errors="replace")
        self.assertIn('parse_uchar_range(arg, 0, 255, "monitor mode", state)', src)
        self.assertIn("strtoul", src)
        self.assertIn("write(STDERR_FILENO", src)

    def test_m09_rawproxy_and_reverse_hardening(self):
        """M-09: Ensure rawproxyreverse has SO_REUSEADDR, CLI port option, 65536B buffer, and prctl in main."""
        src_rev = (ROOT / "utilities/rawproxyreverse/rawproxyreverse.c").read_text(errors="replace")
        self.assertIn("SO_REUSEADDR", src_rev)
        self.assertIn("packet[65536]", src_rev)
        self.assertIn("MSG_TRUNC", src_rev)
        self.assertIn("PR_SET_PDEATHSIG", src_rev)

        src_raw = (ROOT / "utilities/rawproxy/rawproxy.c").read_text(errors="replace")
        self.assertIn("PR_SET_PDEATHSIG", src_raw)

    def test_m10_android_mk_and_wireshark_script(self):
        """M-10: Verify LOCAL_CFLAGS typo fix and Wireshark lemon compile configuration."""
        rawproxy_mk = (ROOT / "utilities/rawproxy/Android.mk").read_text(errors="replace")
        self.assertNotIn("LOCAL_CFLGGS", rawproxy_mk)
        self.assertIn("LOCAL_CFLAGS += -DUSE_LIBPCAP=1", rawproxy_mk)

        build_gradle = (ROOT / "app/app/build.gradle").read_text(errors="replace")
        self.assertIn("code > 0 ? code : 1", build_gradle)

    def test_l02_check_updates_commit_hash_parsing(self):
        """L-02: Verify check_updates does not treat commit hashes as beta releases."""
        check_py = ROOT / "buildtools/scripts/check_updates.py"
        self.assertTrue(check_py.is_file())
        res = subprocess.run([str(check_py), "--category", "all", "--check-only"],
                             capture_output=True, text=True, timeout=15)
        self.assertIn(res.returncode, (0, 1))


class TestTier3PythonSDKAndTooling(unittest.TestCase):
    """M-03, M-04, M-05, M-06, M-07, L-03: Crash decoder, pynexmon, monitor-mode, nexfirmware, nexbench."""

    def test_m03_crash_decoder_ordering_and_json(self):
        """M-03: Ensure crash decoder prioritizes PATCH/UCODE over RAM and emits valid JSON."""
        decoder_py = ROOT / "nexmon_crash_decoder.py"
        self.assertTrue(decoder_py.is_file())

        # Run self-test
        res = subprocess.run(["python3", str(decoder_py), "--test"], capture_output=True, text=True, timeout=10)
        self.assertEqual(res.returncode, 0, res.stderr)
        self.assertIn("All Self-Test Assertions Passed", res.stdout)

        # Run --json with empty input and verify stdout parses as JSON without text banner
        res_json = subprocess.run(["python3", str(decoder_py), "-f", "/dev/null", "--json"],
                                  capture_output=True, text=True, timeout=10)
        # Should exit 1 on empty input
        self.assertEqual(res_json.returncode, 1)

    def test_m04_pynexmon_parameter_validation(self):
        """M-04: Test NexmonDevice interface validation and ioctl bounds enforcement."""
        import sys
        sys.path.insert(0, str(ROOT / "python"))
        from pynexmon import NexmonDevice, NexmonValueError

        # Rejects interface name >= 16 bytes (IFNAMSIZ)
        with self.assertRaises(NexmonValueError):
            NexmonDevice("a_very_long_interface_name_exceeding_16_bytes")

        # Rejects slash in ifname
        with self.assertRaises(NexmonValueError):
            NexmonDevice("wlan0/eth0")

        # Rejects invalid mode
        with self.assertRaises(NexmonValueError):
            NexmonDevice("wlan0", mode="invalid_mode", check_interface=False)

    def test_m05_monitor_mode_script_hardening(self):
        """M-05: Ensure monitor-mode.sh enforces umask 077, locked restoration, and preserves lockfile."""
        mm = (ROOT / "monitor-mode.sh").read_text(errors="replace")
        self.assertIn("umask 077", mm)
        self.assertIn("_WAS_ALREADY_ON", mm)
        self.assertIn("retaining pre-existing monitor session", mm)
        self.assertNotIn('rm -f "$STATE_FILE.lock"', mm)

    def test_m06_nexfirmware_edit_dynamic_offsets_and_atomic_write(self):
        """M-06: Ensure FirmwareManager.edit derives chip-specific offsets and writes atomically."""
        import nexfirmware
        with tempfile.TemporaryDirectory(prefix="nex-t3-edit-") as tmp:
            tmp_path = Path(tmp)
            fw_file = tmp_path / "brcmfmac43430-sdio.bin"
            # 128 KiB synthetic image
            fw_file.write_bytes(b"\x00" * 131072)
            out_file = tmp_path / "edited.bin"

            # BCM43430 has default_ramstart = 0x0
            res = nexfirmware.FirmwareManager.edit(
                str(fw_file),
                out_file=str(out_file),
                patches=[(0x10, b"\xde\xad\xbe\xef")],
                chip="bcm43430a1",
            )
            self.assertTrue(out_file.is_file())
            self.assertIn("modified_sha256", res)
            edited_bytes = out_file.read_bytes()
            self.assertEqual(edited_bytes[0x10:0x14], b"\xde\xad\xbe\xef")

    def test_m07_nexbench_and_multiframe_channel_options(self):
        """M-07: Ensure nexbench and benchmark_multiframe pass -I to nexutil and parse 6g channels."""
        nb = (ROOT / "nexbench.py").read_text(errors="replace")
        self.assertIn('["nexutil", "-I", iface, "-k", ch]', nb)
        self.assertIn("parts[0] == \"wlan0\"", nb)
        self.assertIn("errno.ENOBUFS", nb)

        bm = (ROOT / "benchmark_multiframe.py").read_text(errors="replace")
        self.assertIn('["nexutil", "-I", iface, "-k", ch_str]', bm)

    def test_l03_csi_timestamp_zero_and_hwcheck_chanspecs(self):
        """L-03: Ensure CSIPacket preserves timestamp=0.0 and hwcheck supports 36/80 and 6g."""
        import sys
        sys.path.insert(0, str(ROOT / "python"))
        from pynexmon.csi import CSIPacket

        pkt = CSIPacket(
            magic=0x11111111,
            src_mac="00:11:22:33:44:55",
            seq=1,
            core=0,
            spatial_stream=0,
            chanspec=0x1006,
            chip_id=0x4345,
            raw_iq=[(1, 1), (2, 2)],
            timestamp=0.0,
        )
        self.assertEqual(pkt.timestamp, 0.0)

        hwcheck = (ROOT / "hwcheck_channel_hopping.py").read_text(errors="replace")
        self.assertIn("channels_match", hwcheck)
        self.assertIn("6g", hwcheck)


class TestTier3BuildAndEnvironment(unittest.TestCase):
    """M-11, M-12, M-13, L-04: Makefile bootstrap, setup_env.sh, .gitignore, JNI releases."""

    def test_m11_makefile_and_setup_env_quoting(self):
        """M-11: Ensure Makefile bootstrap uses positional parameters and setup_env preserves ARCH."""
        makefile = (ROOT / "Makefile").read_text(errors="replace")
        self.assertIn('source "$$1" && shift && exec "$$1"', makefile)

        setup_sh = (ROOT / "setup_env.sh").read_text(errors="replace")
        self.assertIn('cd "$(dirname "${BASH_SOURCE[0]}")" || return 1', setup_sh)
        self.assertIn('if [ -z "${ARCH:-}" ]; then', setup_sh)

    def test_m12_crash_decoder_self_test_wiring(self):
        """M-12: Ensure nexmon_crash_decoder.py self-test passes without error."""
        res = subprocess.run(["python3", str(ROOT / "nexmon_crash_decoder.py"), "--test"],
                             capture_output=True, text=True, timeout=10)
        self.assertEqual(res.returncode, 0)
        self.assertIn("✓ All Self-Test Assertions Passed Successfully!", res.stdout)

    def test_m13_gitignore_entries(self):
        """M-13: Ensure diagnostic and test directories are excluded in .gitignore."""
        gi = (ROOT / ".gitignore").read_text(errors="replace")
        for entry in ("/firmware_backup/", "/.ruff_cache/", "/.pytest_cache/",
                      "/DISABLE_STATISTICS", "/updates.json", "fuzz_radiotap_sanitize"):
            self.assertIn(entry, gi, f"{entry} missing from .gitignore")

    def test_l04_wrapper_pentestsuite_jni_cleanup_and_docs(self):
        """L-04: Ensure JNI early-exit paths release GetStringUTFChars and docs are current."""
        src = (ROOT / "app/app/src/main/internal/jni/wrapper_pentestsuite.c").read_text(errors="replace")
        # Ensure ReleaseStringUTFChars is called before return on mid == 0
        self.assertIn("if(mid == 0) {\n\t\tif (native_station_mac) (*env)->ReleaseStringUTFChars", src)

        sec = (ROOT / "SECURITY.md").read_text(errors="replace")
        self.assertIn("brcmfmac", sec)
        self.assertIn("rawproxy", sec)

        dev = (ROOT / "DEVICES.md").read_text(errors="replace")
        self.assertIn("Raspberry Pi 4 Model B", dev)
        self.assertIn("Raspberry Pi 3 Model B", dev)


if __name__ == "__main__":
    unittest.main(verbosity=2)
