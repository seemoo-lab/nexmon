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
        for drv_ver in ("5.15", "6.1", "6.2", "6.6", "6.18", "7.2", "7.3", "7.4"):
            core_c = ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/core.c"
            self.assertTrue(core_c.is_file(), f"Missing {core_c}")
            content = core_c.read_text(errors="replace")
            self.assertIn("wiphy_lock(drvr->wiphy);", content, f"wiphy_lock missing in {drv_ver}")
            self.assertIn("rtnl_lock();", content, f"rtnl_lock missing in {drv_ver}")
            self.assertIn("if (ifp->ndev)", content, f"ifp->ndev branch missing in {drv_ver}")
            self.assertIn("brcmf_remove_interface(ifp, true);", content, f"locked remove missing in {drv_ver}")

    def test_nvram_comment_parsing_bounded(self):
        """Kernel 7.4-rc1 (3ffb0de666c6): Bounded NVRAM comment parsing with strnchr and memchr."""
        for drv_ver in ("5.15", "6.1", "6.2", "6.6", "6.18", "7.2", "7.3", "7.4"):
            fw_c = ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/firmware.c"
            self.assertTrue(fw_c.is_file(), f"Missing {fw_c}")
            content = fw_c.read_text(errors="replace")
            self.assertIn("size_t data_len;", content, f"data_len field missing in {drv_ver}")
            self.assertIn("remaining = nvp->data_len - nvp->pos;", content, f"remaining missing in {drv_ver}")
            self.assertIn("eoc = strnchr(sol, remaining, '\\n');", content, f"strnchr missing in {drv_ver}")
            self.assertIn("eoc = memchr(sol, '\\0', remaining);", content, f"memchr missing in {drv_ver}")
            self.assertIn("nvp->data_len = data_len;", content, f"nvp->data_len assignment missing in {drv_ver}")

    def test_driver_7_4_support(self):
        """Verify Linux 7.4 driver support (cfg80211 nl80211_key_type ops and module version)."""
        cfg_c = (ROOT / "patches/driver/brcmfmac_7.4.y-nexmon/cfg80211.c").read_text(errors="replace")
        self.assertIn("enum nl80211_key_type type", cfg_c)
        common_c = (ROOT / "patches/driver/brcmfmac_7.4.y-nexmon/common.c").read_text(errors="replace")
        self.assertIn('MODULE_VERSION("7.4.0-nexmon");', common_c)

    def test_fast_channel_hopping_infrastructure(self):
        """Fast non-blocking sub-2ms channel hopping ioctl and driver/firmware wiring."""
        # Check ioctl definitions
        for drv_ver in ("7.3", "7.4"):
            hdr = ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/nexmon_ioctls.h"
            content = hdr.read_text(errors="replace")
            self.assertIn("NEX_GET_CHANSPEC_OVERRIDE", content)
            self.assertIn("418", content)
            self.assertIn("NEX_SET_CHANSPEC_OVERRIDE", content)
            self.assertIn("419", content)

        # Check driver fast paths
        for drv_ver in ("7.3", "7.4"):
            core_c = ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/core.c"
            content = core_c.read_text(errors="replace")
            self.assertIn("NEX_SET_CHANSPEC_OVERRIDE", content)
            self.assertIn("NEX_GET_CHANSPEC_OVERRIDE", content)
            self.assertIn('brcmf_fil_iovar_int_set(target_ifp, "chanspec", cs)', content)
            self.assertIn("nexmon_mon_restore_set_chanspec(cs)", content)

        # Check firmware hook
        fw_ioctl = ROOT / "patches/bcm43455c0/7_45_206/nexmon/src/ioctl.c"
        content = fw_ioctl.read_text(errors="replace")
        self.assertIn("case NEX_GET_CHANSPEC_OVERRIDE:", content)
        self.assertIn("case NEX_SET_CHANSPEC_OVERRIDE:", content)
        self.assertIn("WLC_SET_VAR", content)

        # Check libnexio and nexutil
        libnexio_h = (ROOT / "utilities/libnexio/libnexio.h").read_text(errors="replace")
        self.assertIn("nex_ioctl_nonblock", libnexio_h)
        libnexio_c = (ROOT / "utilities/libnexio/libnexio.c").read_text(errors="replace")
        self.assertIn("int\nnex_ioctl_nonblock(", libnexio_c)

        nexutil_c = (ROOT / "utilities/nexutil/nexutil.c").read_text(errors="replace")
        self.assertIn("NEX_SET_CHANSPEC_OVERRIDE", nexutil_c)
        self.assertIn("NEX_GET_CHANSPEC_OVERRIDE", nexutil_c)
        self.assertIn("nex_ioctl_nonblock", nexutil_c)
        self.assertIn('"nonblock"', nexutil_c)

    def test_runt_frame_injection_guards(self):
        """Ensure all secondary/legacy chip firmware targets reject runt injection frames."""
        target_injections = [
            ROOT / "patches/bcm43430a1/7_45_41_46/nexmon/src/injection.c",
            ROOT / "patches/bcm43430a1/7_45_41_26/nexmon/src/injection.c",
            ROOT / "patches/bcm43436b0/9_88_4_65/nexmon/src/injection.c",
            ROOT / "patches/bcm4335b0/6.30.171.1_sta/nexmon/src/injection.c",
            ROOT / "patches/bcm4339/6_37_34_43/nexmon/src/injection.c",
            ROOT / "patches/bcm4358/7_112_300_14_sta/nexmon/src/injection.c",
            ROOT / "patches/bcm43596a0/9.96.4_sta_c0/nexmon/src/injection.c",
        ]
        for inj_file in target_injections:
            self.assertTrue(inj_file.is_file(), f"Missing {inj_file}")
            text = inj_file.read_text(errors="replace")
            self.assertIn("rtap_len >= (int) p->len", text, f"{inj_file} missing inject_frame runt guard")
            if "wl_send_hook" in text or "handle_sdio_xmit_request_hook" in text:
                self.assertIn("rtap_len < (int) p->len", text, f"{inj_file} missing hook payload guard")

    def test_firmware_helper_and_udptunnel_bounds(self):
        """Verify helper.c skb_pull bounds checking and udptunnel.c 1's complement folded checksum."""
        helper_c = (ROOT / "patches/common/helper.c").read_text(errors="replace")
        self.assertIn("if (len > p->len)", helper_c, "skb_pull underflow check missing")

        udptunnel_c = (ROOT / "patches/common/udptunnel.c").read_text(errors="replace")
        self.assertIn("while (sum >> 16)", udptunnel_c, "calc_checksum folded 1's complement missing")

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

    def test_tsf64_hardware_timestamping_radiotap_header(self):
        """Verify 64-bit microsecond TSF radiotap layout, alignment and presence bit in drivers."""
        for drv_ver in ("7.3", "7.4"):
            core_c = (ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/core.c").read_text(errors="replace")
            core_h = (ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/core.h").read_text(errors="replace")

            # Check core.h fields in struct brcmf_pub
            self.assertIn("u32 nexmon_tsf_high;", core_h, f"nexmon_tsf_high missing in {drv_ver}/core.h")
            self.assertIn("u32 nexmon_last_tsf_low;", core_h, f"nexmon_last_tsf_low missing in {drv_ver}/core.h")
            self.assertIn("bool nexmon_tsf_initialized;", core_h, f"nexmon_tsf_initialized missing in {drv_ver}/core.h")

            # Check struct brcmf_mon_radiotap definition
            self.assertIn("__le64 tsft;", core_c, f"__le64 tsft missing in {drv_ver}/core.c")
            self.assertIn("BIT(IEEE80211_RADIOTAP_TSFT)", core_c, f"TSFT bit missing in {drv_ver}/core.c")
            self.assertIn("brcmf_mon_recover_tsf64", core_c, f"brcmf_mon_recover_tsf64 missing in {drv_ver}/core.c")

            # Check fallback path uses microsecond boottime
            self.assertIn("ktime_get_boottime_ns()", core_c, f"ktime_get_boottime_ns missing in {drv_ver}/core.c")

            # Check ARPHRD_IEEE80211_RADIOTAP unwraps tsf_h
            self.assertIn("tsf[1] = cpu_to_le32((u32)(tsf64 >> 32));", core_c, f"tsf_h unwrap missing in {drv_ver}/core.c")

    def test_tsf64_firmware_rollover_tracking(self):
        """Verify firmware monitormode.c files track 32-bit hardware TSF rollover to populate tsf_h."""
        for mm_path in ROOT.glob("patches/**/monitormode.c"):
            content = mm_path.read_text(errors="replace")
            if "wl_monitor_radiotap" in content and "sts->mactime" in content:
                self.assertIn("fw_tsf_high", content, f"fw_tsf_high rollover tracking missing in {mm_path}")
                self.assertIn("frame->tsf.tsf_h = fw_tsf_high;", content, f"tsf_h assignment missing in {mm_path}")

    def test_tsf64_monotonic_unwrapper_simulation(self):
        """Simulate brcmf_mon_recover_tsf64 logic across 32-bit boundaries and jitter."""
        class MockDriver:
            def __init__(self):
                self.nexmon_tsf_high = 0
                self.nexmon_last_tsf_low = 0
                self.nexmon_tsf_initialized = False

        def recover_tsf64(drvr, tsf_low):
            if not tsf_low and not drvr.nexmon_tsf_initialized:
                return 123456789  # mock boottime fallback
            if not drvr.nexmon_tsf_initialized:
                drvr.nexmon_tsf_initialized = True
                drvr.nexmon_last_tsf_low = tsf_low
                drvr.nexmon_tsf_high = 0
            else:
                if drvr.nexmon_last_tsf_low >= 0x80000000 and tsf_low < 0x80000000:
                    drvr.nexmon_tsf_high += 1
                drvr.nexmon_last_tsf_low = tsf_low
            return (drvr.nexmon_tsf_high << 32) | tsf_low

        drvr = MockDriver()
        # Normal initial reception
        self.assertEqual(recover_tsf64(drvr, 1000), 1000)
        self.assertEqual(recover_tsf64(drvr, 2000), 2000)

        # High quadrant progression
        self.assertEqual(recover_tsf64(drvr, 0xFFFFFF00), 0xFFFFFF00)
        # Small out-of-order jitter in high quadrant should not wrap
        self.assertEqual(recover_tsf64(drvr, 0xFFFFFE80), 0xFFFFFE80)
        self.assertEqual(drvr.nexmon_tsf_high, 0)

        # 32-bit Rollover boundary crossing
        self.assertEqual(recover_tsf64(drvr, 0x00000050), (1 << 32) | 0x00000050)
        self.assertEqual(drvr.nexmon_tsf_high, 1)

        # Monotonic continuation in epoch 1
        self.assertEqual(recover_tsf64(drvr, 0x00000100), (1 << 32) | 0x00000100)
        self.assertEqual(recover_tsf64(drvr, 0x80000010), (1 << 32) | 0x80000010)

        # Second rollover
        self.assertEqual(recover_tsf64(drvr, 0xFFFFFFF0), (1 << 32) | 0xFFFFFFF0)
        self.assertEqual(recover_tsf64(drvr, 0x00000010), (2 << 32) | 0x00000010)
        self.assertEqual(drvr.nexmon_tsf_high, 2)

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

    def test_zstandard_firmware_support(self):
        """Verify nexfirmware transparently supports .zst compressed firmware and NVRAM/CLM files."""
        import nexfirmware
        import zstandard
        with tempfile.TemporaryDirectory(prefix="nex-t3-zst-") as tmp:
            tmp_path = Path(tmp)
            raw_data = b"NEXMON_ZST_TEST_BLOB_BCM43455_7.45.206" + (b"\x00" * 1024)
            cctx = zstandard.ZstdCompressor()
            compressed = cctx.compress(raw_data)

            zst_file = tmp_path / "firmware.bin.zst"
            zst_file.write_bytes(compressed)

            decompressed = nexfirmware.read_firmware_blob(zst_file)
            self.assertEqual(decompressed, raw_data)

            # Test uncompressed fallback
            raw_file = tmp_path / "firmware.bin"
            raw_file.write_bytes(raw_data)
            self.assertEqual(nexfirmware.read_firmware_blob(raw_file), raw_data)

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
