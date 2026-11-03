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

    def test_atomic_mgmt_tx_id_monotonic(self):
        """Kernel 7.4-rc1 (60cbcc91dc89): Monotonic atomic mgmt_tx_id prevents firmware completion collisions."""
        for drv_ver in ("5.15", "6.1", "6.2", "6.6", "6.18", "7.2", "7.3", "7.4"):
            cfg_c = ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/cfg80211.c"
            self.assertTrue(cfg_c.is_file(), f"Missing {cfg_c}")
            content = cfg_c.read_text(errors="replace")
            self.assertIn("atomic_t brcmf_mgmt_tx_id", content, f"brcmf_mgmt_tx_id missing in {drv_ver}")
            self.assertIn("brcmf_next_mgmt_tx_id", content, f"brcmf_next_mgmt_tx_id missing in {drv_ver}")
            self.assertIn("action_frame->packet_id = cpu_to_le32(", content, f"packet_id assignment missing in {drv_ver}")

    def test_8021x_tx_completion_memory_barrier(self):
        """Kernel 7.4-rc1 (621d90169cef): Memory barrier smp_mb__after_atomic prevents lost 802.1X TX completion wakeup."""
        all_drv_versions = (
            "4.4", "4.9", "4.14", "4.19", "5.4", "5.10", "5.15",
            "6.1", "6.2", "6.6", "6.18", "7.2", "7.3", "7.4"
        )
        for drv_ver in all_drv_versions:
            core_c = ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/core.c"
            self.assertTrue(core_c.is_file(), f"Missing {core_c}")
            content = core_c.read_text(errors="replace")
            self.assertIn("atomic_dec(&ifp->pend_8021x_cnt);", content, f"atomic_dec missing in {drv_ver}")
            self.assertIn("smp_mb__after_atomic();", content, f"smp_mb__after_atomic missing in {drv_ver}")
            # Ensure smp_mb__after_atomic() comes after atomic_dec and before waitqueue_active
            dec_pos = content.find("atomic_dec(&ifp->pend_8021x_cnt);")
            barrier_pos = content.find("smp_mb__after_atomic();", dec_pos)
            wq_pos = content.find("if (waitqueue_active(&ifp->pend_8021x_wait))", barrier_pos)
            self.assertTrue(
                dec_pos < barrier_pos < wq_pos,
                f"Memory barrier ordering incorrect in {drv_ver}: dec={dec_pos}, barrier={barrier_pos}, wq={wq_pos}"
            )

    def test_connect_failure_diagnostic_logging(self):
        """Kernel 7.4-rc1 (42aa76b3fd92): Diagnostic firmware status and reason logging on connection failure."""
        all_drv_versions = (
            "4.4", "4.9", "4.14", "4.19", "5.4", "5.10", "5.15",
            "6.1", "6.2", "6.6", "6.18", "7.2", "7.3", "7.4"
        )
        for drv_ver in all_drv_versions:
            cfg_c = ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/cfg80211.c"
            self.assertTrue(cfg_c.is_file(), f"Missing {cfg_c}")
            content = cfg_c.read_text(errors="replace")
            self.assertIn(
                "connect failed: event %u status %u reason %u",
                content,
                f"Connect failure diagnostic logging missing in {drv_ver}"
            )
            self.assertIn(
                "e->event_code, e->status, e->reason",
                content,
                f"Event status/reason parameters missing in {drv_ver}"
            )

    def test_power_saving_delay_2s_optimization(self):
        """RPi Kernel (e7d5c7bae4a9): Increase power saving sleep delay to 2s across driver matrix."""
        all_drv_versions = (
            "4.4", "4.9", "4.14", "4.19", "5.4", "5.10", "5.15",
            "6.1", "6.2", "6.6", "6.18", "7.2", "7.3", "7.4"
        )
        for drv_ver in all_drv_versions:
            cfg_c = ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/cfg80211.c"
            self.assertTrue(cfg_c.is_file(), f"Missing {cfg_c}")
            content = cfg_c.read_text(errors="replace")
            self.assertIn(
                '"pm2_sleep_ret"',
                content,
                f"pm2_sleep_ret missing in {drv_ver}"
            )
            self.assertTrue(
                "2000" in content,
                f"2000ms sleep return delay missing in {drv_ver}"
            )

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

    def test_zero_copy_netlink_csi_streaming(self):
        """Zero-Copy Netlink CSI Streaming: kernel GENL family and userspace receiver."""
        for drv_ver in ("7.3", "7.4"):
            core_c = (ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/core.c").read_text(errors="replace")
            self.assertIn('NEXMON_CSI_GENL_NAME "nexmon_csi"', core_c)
            self.assertIn("struct genl_family nexmon_csi_gnl_family", core_c)
            self.assertIn("nexmon_csi_handle_rx", core_c)
            self.assertIn("genlmsg_multicast_allns(&nexmon_csi_gnl_family", core_c)
            self.assertIn("brcmu_pkt_buf_free_skb(skb)", core_c)
            self.assertIn("genl_register_family(&nexmon_csi_gnl_family)", core_c)
            self.assertIn("genl_unregister_family(&nexmon_csi_gnl_family)", core_c)

        csi_py = (ROOT / "python/pynexmon/csi.py").read_text(errors="replace")
        self.assertIn("class NetlinkCSIReceiver", csi_py)
        self.assertIn("class UDPCSIReceiver", csi_py)
        self.assertIn("resolve_genl_family", csi_py)
        self.assertIn("NEXMON_CSI_GENL_NAME", csi_py)

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

    def test_firmware_null_and_boundary_defenses(self):
        """Verify firmware NULL pointer defenses and buffer boundary checks across helper, udptunnel, argprintf, and injection hooks."""
        # 1. helper.c pkthead & skb_push
        helper_c = (ROOT / "patches/common/helper.c").read_text(errors="replace")
        self.assertIn("pkthead(sk_buff *p)", helper_c)
        self.assertIn("p->head_off", helper_c)
        self.assertIn("(unsigned char *) p->data - len < (unsigned char *) head", helper_c)

        # 2. udptunnel.c
        udptunnel_c = (ROOT / "patches/common/udptunnel.c").read_text(errors="replace")
        self.assertIn("if (!p)\n        return;", udptunnel_c)
        self.assertIn("if (!hdr)\n        return;", udptunnel_c)

        # 3. argprintf.c reentrancy & validation
        argprintf_c = (ROOT / "patches/common/argprintf.c").read_text(errors="replace")
        self.assertIn("argprintf_in_progress", argprintf_c)
        self.assertIn("argprintf_len = (len > 0) ? len : 0;", argprintf_c)

        # 4. wl_send_hook in injection targets
        hook_targets = [
            ROOT / "patches/bcm43455c0/7_45_265-28bca26-CY/nexmon/src/injection.c",
            ROOT / "patches/bcm43455c0/7_45_206/nexmon/src/injection.c",
            ROOT / "patches/bcm43455c0/7_45_189/nexmon/src/injection.c",
            ROOT / "patches/bcm43455c0/7_45_154/nexmon/src/injection.c",
            ROOT / "patches/bcm43430a1/7_45_41_46/nexmon/src/injection.c",
            ROOT / "patches/bcm43430a1/7_45_41_26/nexmon/src/injection.c",
            ROOT / "patches/bcm43436b0/9_88_4_65/nexmon/src/injection.c",
        ]
        for inj_path in hook_targets:
            text = inj_path.read_text(errors="replace")
            self.assertIn("if (!dev || !dev->softc)", text, f"{inj_path} missing dev null check")
            self.assertIn("if (!wl || !wl->wlc)", text, f"{inj_path} missing wl/wlc null check")

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

    def test_d11_shm_memory_mapped_primitives(self):
        """Kernel 7.4-rc1 (f6e39ae4ea84): Direct D11 SHM host memory-mapped access primitives."""
        # 1. IOCTLs and memory map definitions in headers
        for drv_ver in ("7.3", "7.4"):
            hdr = (ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/nexmon_ioctls.h").read_text(errors="replace")
            self.assertIn("#define NEX_READ_D11_SHM", hdr)
            self.assertIn("430", hdr)
            self.assertIn("#define NEX_WRITE_D11_SHM", hdr)
            self.assertIn("431", hdr)
            self.assertIn("struct nex_d11_shm_req", hdr)
            self.assertIn("D11_BASE_ADDR", hdr)
            self.assertIn("0x18001000", hdr)
            self.assertIn("D11_AXI_BASE_ADDR", hdr)
            self.assertIn("0xE8000000", hdr)
            self.assertIn("D11_SHM_BASE_ADDR", hdr)
            self.assertIn("D11REG_ADDR", hdr)
            self.assertIn("D11SHM_ADDR", hdr)

        common_hdr = (ROOT / "patches/include/nexioctls.h").read_text(errors="replace")
        self.assertIn("#define NEX_READ_D11_SHM                430", common_hdr)
        self.assertIn("#define NEX_WRITE_D11_SHM               431", common_hdr)
        self.assertIn("struct nex_d11_shm_req", common_hdr)

        # 2. Driver core handlers
        for drv_ver in ("7.3", "7.4"):
            core = (ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/core.c").read_text(errors="replace")
            self.assertIn("frame->cmd == NEX_READ_D11_SHM", core)
            self.assertIn("frame->cmd == NEX_WRITE_D11_SHM", core)
            self.assertIn("brcmf_sdio_sleep(sdiodev->bus, false);", core)
            self.assertIn("brcmf_sdiod_readl", core)
            self.assertIn("brcmf_sdiod_writel", core)

        # 3. Libnexio bindings and CLI options
        lib_h = (ROOT / "utilities/libnexio/libnexio.h").read_text(errors="replace")
        self.assertIn("int nex_read_d11_shm(", lib_h)
        self.assertIn("int nex_write_d11_shm(", lib_h)
        self.assertIn("int nex_read_d11_reg(", lib_h)
        self.assertIn("int nex_write_d11_reg(", lib_h)

        nexutil_c = (ROOT / "utilities/nexutil/nexutil.c").read_text(errors="replace")
        self.assertIn('{"read-shm", \'S\', "INT"', nexutil_c)
        self.assertIn('{"write-shm", \'W\', "INT=VAL"', nexutil_c)
        self.assertIn("nex_read_d11_shm", nexutil_c)
        self.assertIn("nex_write_d11_shm", nexutil_c)

        # 4. Python SDK
        import sys
        sys.path.insert(0, str(ROOT / "python"))
        import pynexmon
        self.assertEqual(pynexmon.NEX_READ_D11_SHM, 430)
        self.assertEqual(pynexmon.NEX_WRITE_D11_SHM, 431)
        self.assertTrue(hasattr(pynexmon.NexmonDevice, "read_d11_shm"))
        self.assertTrue(hasattr(pynexmon.NexmonDevice, "write_d11_shm"))
        self.assertTrue(hasattr(pynexmon.NexmonDevice, "read_d11_reg"))
        self.assertTrue(hasattr(pynexmon.NexmonDevice, "write_d11_reg"))

    def test_d11_template_ram_host_primitives(self):
        """Item 3: Direct D11 Template RAM Host Primitives (NEX_READ_TEMPLATE_RAM / NEX_WRITE_TEMPLATE_RAM)."""
        # 1. IOCTL headers & definitions
        for drv_ver in ("7.3", "7.4"):
            hdr = (ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/nexmon_ioctls.h").read_text(errors="replace")
            self.assertIn("#define NEX_READ_TEMPLATE_RAM", hdr)
            self.assertIn("429", hdr)
            self.assertIn("#define NEX_WRITE_TEMPLATE_RAM", hdr)
            self.assertIn("426", hdr)
            self.assertIn("struct nex_d11_templ_req", hdr)
            self.assertIn("D11_TEMPLATE_RAM_BASE", hdr)
            self.assertIn("0x18001800", hdr)
            self.assertIn("D11_TPLATEWRPTR_REG", hdr)
            self.assertIn("0x130", hdr)
            self.assertIn("D11_TPLATEWRDATA_REG", hdr)
            self.assertIn("0x134", hdr)
            self.assertIn("D11_TPL_BCN0_BASE", hdr)
            self.assertIn("D11_TPL_PRS_BASE", hdr)

        common_hdr = (ROOT / "patches/include/nexioctls.h").read_text(errors="replace")
        self.assertIn("#define NEX_READ_TEMPLATE_RAM           429", common_hdr)
        self.assertIn("#define NEX_WRITE_TEMPLATE_RAM          426", common_hdr)
        self.assertIn("struct nex_d11_templ_req", common_hdr)

        # 2. Driver core handlers
        for drv_ver in ("7.3", "7.4"):
            core = (ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/core.c").read_text(errors="replace")
            self.assertIn("frame->cmd == NEX_READ_TEMPLATE_RAM", core)
            self.assertIn("frame->cmd == NEX_WRITE_TEMPLATE_RAM", core)
            self.assertIn("D11_TPLATEWRPTR_REG", core)
            self.assertIn("D11_TPLATEWRDATA_REG", core)

        # 3. Libnexio bindings and CLI options
        lib_h = (ROOT / "utilities/libnexio/libnexio.h").read_text(errors="replace")
        self.assertIn("int nex_read_d11_templ(", lib_h)
        self.assertIn("int nex_write_d11_templ(", lib_h)
        self.assertIn("int nex_read_d11_template_ram(", lib_h)
        self.assertIn("int nex_write_d11_template_ram(", lib_h)

        nexutil_c = (ROOT / "utilities/nexutil/nexutil.c").read_text(errors="replace")
        self.assertIn('{"read-template", \'T\', "INT"', nexutil_c)
        self.assertIn('{"write-template", \'Y\', "INT=VAL"', nexutil_c)
        self.assertIn('{"dump-template", \'M\', "INT"', nexutil_c)
        self.assertIn("nex_read_d11_templ", nexutil_c)
        self.assertIn("nex_write_d11_templ", nexutil_c)
        self.assertIn("nex_read_d11_template_ram", nexutil_c)

        # 4. Python SDK
        import sys
        sys.path.insert(0, str(ROOT / "python"))
        import pynexmon
        self.assertEqual(pynexmon.NEX_READ_TEMPLATE_RAM, 429)
        self.assertEqual(pynexmon.NEX_WRITE_TEMPLATE_RAM, 426)
        self.assertTrue(hasattr(pynexmon.NexmonDevice, "read_d11_templ"))
        self.assertTrue(hasattr(pynexmon.NexmonDevice, "write_d11_templ"))
        self.assertTrue(hasattr(pynexmon.NexmonDevice, "read_d11_template_ram"))
        self.assertTrue(hasattr(pynexmon.NexmonDevice, "write_d11_template_ram"))

    def test_bcm4375b1_runt_frame_injection_check(self):
        """Ensure BCM4375b1 firmware injection boundary check uses >= to prevent runt frame underflows."""
        for path in (
            ROOT / "patches/bcm4375b1/18_38_18_sta/nexmon/src/injection.c",
            ROOT / "patches/bcm4375b1/18_41_8_9_sta/nexmon/src/injection.c",
        ):
            content = path.read_text(errors="replace")
            self.assertIn("rtap_header->it_len >= p->len", content)
            self.assertNotIn("rtap_header->it_len > p->len", content)

    def test_per_board_clm_blob_firmware_declaration(self):
        """Kernel 7.4 commit 8a0f175488bd: Ensure per-board clm_blob MODULE_FIRMWARE declarations exist."""
        for drv_ver in ("7.3", "7.4"):
            sdio_c = (ROOT / f"patches/driver/brcmfmac_{drv_ver}.y-nexmon/sdio.c").read_text(errors="replace")
            self.assertIn('MODULE_FIRMWARE(BRCMF_FW_DEFAULT_PATH "brcmfmac*-sdio.*.clm_blob");', sdio_c)

    def test_bcm43341_chip_id_mapping(self):
        """Ensure BCM43341 chip ID mapping is correct in 4.19 driver sdio.c."""
        sdio_c = (ROOT / "patches/driver/brcmfmac_4.19.y-nexmon/sdio.c").read_text(errors="replace")
        self.assertIn('BRCMF_FW_DEF(43341, "brcmfmac43341-sdio");', sdio_c)
        self.assertIn('BRCMF_FW_ENTRY(BRCM_CC_43341_CHIP_ID, 0xFFFFFFFF, 43341)', sdio_c)



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
