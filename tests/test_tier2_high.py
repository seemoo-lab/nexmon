#!/usr/bin/env python3
"""Comprehensive regression test suite for Tier 2 High audit fixes (H-01 through H-18)."""

import os
import socket
import struct
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys_py = str(ROOT / "python")
if sys_py not in os.sys.path:
    os.sys.path.insert(0, sys_py)

from pynexmon.csi import (
    CSIReceiver,
    parse_csi_payload,
    NEXMON_CSI_MAGIC,
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
    iq_data = bytearray()
    for sc in range(num_subcarriers):
        i_val = sc * 10
        q_val = -sc * 5
        iq_data.extend(struct.pack("<hh", i_val, q_val))

    return bytes(header + iq_data)


class TestTier2High(unittest.TestCase):

    # -------------------------------------------------------------------------
    # H-01: Netlink skb linearize & WLC_{GET,SET}_PROMISC/CHANNEL Primary-BSS Redirect
    # -------------------------------------------------------------------------
    def test_h01_driver_promisc_channel_redirect_and_netlink_copy(self):
        core_files = list(ROOT.glob("patches/driver/brcmfmac_*.y-nexmon/core.c"))
        self.assertGreaterEqual(len(core_files), 13, "Expected at least 13 brcmfmac core.c files")
        for cf in core_files:
            text = cf.read_text(encoding="utf-8", errors="replace")
            # Verify primary-BSS redirect includes promisc (9/10) and channel (29/30)
            self.assertIn("WLC_SET_PROMISC", text, f"{cf.name} missing WLC_SET_PROMISC redirect")
            self.assertIn("WLC_GET_PROMISC", text, f"{cf.name} missing WLC_GET_PROMISC redirect")
            self.assertIn("WLC_SET_CHANNEL", text, f"{cf.name} missing WLC_SET_CHANNEL redirect")
            self.assertIn("WLC_GET_CHANNEL", text, f"{cf.name} missing WLC_GET_CHANNEL redirect")
            # Verify Netlink copy linearizes skb
            self.assertIn("skb_linearize", text, f"{cf.name} missing skb_linearize")

    # -------------------------------------------------------------------------
    # H-02: BCDC DCMD Response Bounds Validation
    # -------------------------------------------------------------------------
    def test_h02_driver_bcdc_bounds_validation(self):
        bcdc_files = list(ROOT.glob("patches/driver/brcmfmac_*.y-nexmon/bcdc.c"))
        self.assertGreaterEqual(len(bcdc_files), 13, "Expected at least 13 brcmfmac bcdc.c files")
        for bf in bcdc_files:
            text = bf.read_text(encoding="utf-8", errors="replace")
            self.assertIn("sizeof(struct brcmf_proto_bcdc_dcmd)", text, f"{bf.name} missing header size validation")
            self.assertIn("le32_to_cpu(msg->cmd) != cmd", text, f"{bf.name} missing cmd match check")
            self.assertIn("wire_payload", text, f"{bf.name} missing wire payload bounds clamping")

    # -------------------------------------------------------------------------
    # H-03: udptunnel vsnprintf Clamping & Const Header
    # -------------------------------------------------------------------------
    def test_h03_udptunnel_hardening(self):
        udp_c = ROOT / "patches/common/udptunnel.c"
        text = udp_c.read_text(encoding="utf-8", errors="replace")
        self.assertIn("static const struct ethernet_ip_udp_header ethernet_ipv4_udp_header", text)
        self.assertIn("if ((unsigned int) len >= n)", text)
        self.assertIn("len = (int) (n - 1);", text)

    # -------------------------------------------------------------------------
    # H-04: Radiotap Parser & Namespace Checks
    # -------------------------------------------------------------------------
    def test_h04_radiotap_parser_and_namespaces(self):
        rtap_c = ROOT / "patches/common/radiotap.c"
        text = rtap_c.read_text(encoding="utf-8", errors="replace")
        # Verify index 18 (XCHANNEL) is present and not omitted
        self.assertIn("[IEEE80211_RADIOTAP_XCHANNEL]", text)
        # Verify iterator initializes _next_ns_data
        self.assertIn("iterator->_next_ns_data = 0", text)
        # Verify vnslen overflow-safe check
        self.assertIn("(unsigned long)iterator->_max_length - used - (unsigned long)size <", text)
        self.assertIn("(unsigned long)vnslen", text)

        rtap_h = ROOT / "patches/include/ieee80211_radiotap.h"
        text_h = rtap_h.read_text(encoding="utf-8", errors="replace")
        self.assertIn("IEEE80211_RADIOTAP_HE_MU", text_h)
        self.assertIn("IEEE80211_RADIOTAP_EHT", text_h)
        self.assertIn("#define ENOENT", text_h)
        self.assertIn("#define EINVAL", text_h)

    # -------------------------------------------------------------------------
    # H-05: helper.c Safety Checks
    # -------------------------------------------------------------------------
    def test_h05_helper_memory_and_pointer_safety(self):
        helper_c = ROOT / "patches/common/helper.c"
        text = helper_c.read_text(encoding="utf-8", errors="replace")
        self.assertIn("buff[0] = '\\0';", text)
        self.assertIn("if (len <= 0 || pc == 0)", text)
        self.assertIn("if (!delayed_task)", text)
        self.assertIn("hndrte_free_timer(task);", text)

    # -------------------------------------------------------------------------
    # H-06: Firmware Bounds & RX Underflow Checks
    # -------------------------------------------------------------------------
    def test_h06_firmware_bounds_and_monitormode(self):
        for fw_ver in ["7_45_265-28bca26-CY", "7_45_206", "7_45_189", "7_45_154"]:
            mon_c = ROOT / f"patches/bcm43455c0/{fw_ver}/nexmon/src/monitormode.c"
            text = mon_c.read_text(encoding="utf-8", errors="replace")
            self.assertIn("p->len <= PLCP_HDR_LEN", text, f"{fw_ver} monitormode.c missing PLCP guard")

            inj_c = ROOT / f"patches/bcm43455c0/{fw_ver}/nexmon/src/injection.c"
            text_inj = inj_c.read_text(encoding="utf-8", errors="replace")
            self.assertIn("rtap_len < (int) p->len", text_inj, f"{fw_ver} injection.c missing payload check")

        sf_c = ROOT / "patches/bcm43455c0/7_45_265-28bca26-CY/nexmon/src/sendframe.c"
        text_sf = sf_c.read_text(encoding="utf-8", errors="replace")
        self.assertIn("dump_len = p->len < sizeof(dbg_txh_dump) ? p->len : sizeof(dbg_txh_dump)", text_sf)

    # -------------------------------------------------------------------------
    # H-07: Restored Chip Targets & Conflicting Timer Symbol Fix
    # -------------------------------------------------------------------------
    def test_h07_chip_targets_and_wrapper_symbols(self):
        for target in ["patches/bcm4373a0", "patches/bcm4378b1", "patches/bcm4383a3", "patches/bcm4390b1"]:
            self.assertTrue((ROOT / target).is_dir(), f"Target {target} should be present")

        wrapper_c = ROOT / "patches/common/wrapper.c"
        text = wrapper_c.read_text(encoding="utf-8", errors="replace")
        self.assertIn("AT(CHIP_VER_BCM4356, FW_VER_ALL, 0x8240)", text)
        # Should not have duplicate BCM4358 on 0x8240
        self.assertNotIn("AT(CHIP_VER_BCM4358, FW_VER_ALL, 0x8240)", text)

    # -------------------------------------------------------------------------
    # H-08: libnexio Vendor Command & Error Handling
    # -------------------------------------------------------------------------
    def test_h08_libnexio_error_handling(self):
        libnexio_c = ROOT / "utilities/libnexio/libnexio.c"
        text = libnexio_c.read_text(encoding="utf-8", errors="replace")
        self.assertIn("nl_cb_err", text)
        self.assertIn("len > 0 && buf == NULL", text)
        self.assertIn("nl_recvmsgs", text)

    # -------------------------------------------------------------------------
    # H-09: LD_PRELOAD ioctl & fakeioctl Returns
    # -------------------------------------------------------------------------
    def test_h09_fakeioctl_and_nexmon_preload(self):
        fakeioctl_c = ROOT / "utilities/libfakeioctl/fakeioctl.c"
        text_f = fakeioctl_c.read_text(encoding="utf-8", errors="replace")
        self.assertIn("ret = 0;", text_f)
        self.assertIn("request_t", text_f)

        nexmon_c = ROOT / "utilities/libnexmon/nexmon.c"
        text_n = nexmon_c.read_text(encoding="utf-8", errors="replace")
        self.assertIn("SOCK_CLOEXEC", text_n)
        self.assertIn("func_ioctl", text_n)

    # -------------------------------------------------------------------------
    # H-10: rawproxy 16-byte PCAP Wire Header & dhdutil Checks
    # -------------------------------------------------------------------------
    def test_h10_rawproxy_wire_format_and_dhdutil(self):
        rawproxy_c = ROOT / "utilities/rawproxy/rawproxy.c"
        text_r = rawproxy_c.read_text(encoding="utf-8", errors="replace")
        self.assertIn("struct pcap_pkthdr32", text_r)
        self.assertIn("rtap_len < 8", text_r)

        dhdu_linux_c = ROOT / "utilities/dhdutil/dhdu_linux.c"
        text_d = dhdu_linux_c.read_text(encoding="utf-8", errors="replace")
        self.assertIn("buf == NULL", text_d)

    # -------------------------------------------------------------------------
    # H-11: Android Helpers & JNI Ref Lifecycle
    # -------------------------------------------------------------------------
    def test_h11_android_helpers_safety(self):
        logger_c = ROOT / "app/app/src/main/external/src/libandroidlogger/android-logger.c"
        text_l = logger_c.read_text(encoding="utf-8", errors="replace")
        self.assertIn("buf[rdsz] = '\\0'", text_l)
        self.assertNotIn("buf[rdsz - 1] = 0", text_l)

        sender_c = ROOT / "app/app/src/main/external/src/libsender/src/sender.c"
        text_s = sender_c.read_text(encoding="utf-8", errors="replace")
        self.assertIn("NewGlobalRef", text_s)

    # -------------------------------------------------------------------------
    # H-12: validate_firmware.py Enhancements
    # -------------------------------------------------------------------------
    def test_h12_validate_firmware_tightening(self):
        vf_py = ROOT / "buildtools/scripts/validate_firmware.py"
        text = vf_py.read_text(encoding="utf-8", errors="replace")
        self.assertIn("is_append = k.endswith(\"+\")", text)
        self.assertIn("linker.err", text)
        self.assertIn("decomp != raw_ucode", text)
        self.assertIn("e_entry == 0", text)

    # -------------------------------------------------------------------------
    # H-13: Parallel make Races & Multi-ABI DAG Prerequisites
    # -------------------------------------------------------------------------
    def test_h13_parallel_make_and_dag(self):
        gd_sh = ROOT / "buildtools/scripts/getdefine.sh"
        text_gd = gd_sh.read_text(encoding="utf-8", errors="replace")
        self.assertIn("mktemp", text_gd)
        self.assertIn("mv -f", text_gd)

        util_mk = ROOT / "utilities/Makefile"
        text_u = util_mk.read_text(encoding="utf-8", errors="replace")
        self.assertIn("libfakeioctl/. libnexmon/. nexutil/.: libnexio/.", text_u)
        self.assertIn("rawproxy/. rawproxyreverse/. mdk4/. tcpdump/.: libpcap/.", text_u)

        ws_sh = ROOT / "utilities/wireshark/build-android.sh"
        text_ws = ws_sh.read_text(encoding="utf-8", errors="replace")
        self.assertIn('ABI="$ANDROID_ABI"', text_ws)

    # -------------------------------------------------------------------------
    # H-14: Flashpatches Portability, Section Addr & b43 Assembler
    # -------------------------------------------------------------------------
    def test_h14_flashpatch_portability_and_b43(self):
        fp_awk = ROOT / "buildtools/scripts/flashpatches.mk.awk"
        text_fp = fp_awk.read_text(encoding="utf-8", errors="replace")
        self.assertIn("wc -c < gen/section.bin", text_fp)
        self.assertIn("bs=1 conv=notrunc", text_fp)
        self.assertNotIn("oflag=seek_bytes", text_fp)

        sec_sh = ROOT / "buildtools/scripts/getsectionaddr.sh"
        text_sec = sec_sh.read_text(encoding="utf-8", errors="replace")
        self.assertIn("${CC%gcc}objdump", text_sec)
        self.assertIn('"$OBJDUMP" -h "$2"', text_sec)

        b43_main = ROOT / "buildtools/b43-unified/assembler/main.c"
        text_b43 = b43_main.read_text(encoding="utf-8", errors="replace")
        self.assertIn("insn_count_limit = 65536;", text_b43)

        b43_iv = ROOT / "buildtools/b43-unified/assembler/initvals.c"
        text_iv = b43_iv.read_text(encoding="utf-8", errors="replace")
        self.assertIn("size == SIZE_16BIT && value > 0xFFFF", text_iv)

    # -------------------------------------------------------------------------
    # H-15: nexfirmware.py Modes, Multi-chip, Permissions & Rollback
    # -------------------------------------------------------------------------
    def test_h15_nexfirmware_improvements(self):
        nf_py = ROOT / "nexfirmware.py"
        text_nf = nf_py.read_text(encoding="utf-8", errors="replace")
        self.assertIn("os.chmod(temporary, 0o644)", text_nf)
        self.assertIn('is_nexmon="nexmon" in name.lower()', text_nf)
        self.assertIn("brcmfmac_wcc", text_nf)
        self.assertIn("actual_ram_start", text_nf)

    # -------------------------------------------------------------------------
    # H-16: monitor-mode.sh Uplink Guard & Non-blocking Crash Check
    # -------------------------------------------------------------------------
    def test_h16_monitor_mode_script_hardening(self):
        mm_sh = ROOT / "monitor-mode.sh"
        text_mm = mm_sh.read_text(encoding="utf-8", errors="replace")
        self.assertIn("for family in -4 -6", text_mm)
        self.assertIn('iflag=nonblock', text_mm)
        self.assertIn('[ -n "$mark" ] || { echo 0; return 0; }', text_mm)

    # -------------------------------------------------------------------------
    # H-17: pynexmon.csi Magic, Offset, Buffer Size & Timeout Handling
    # -------------------------------------------------------------------------
    def test_h17_pynexmon_csi_packet_handling(self):
        # 1. Negative offset rejection
        with self.assertRaises(ValueError):
            parse_csi_payload(b"\x11" * 32, offset=-1)

        # 2. Magic mismatch rejection
        bad_magic_payload = create_synthetic_csi_frame(magic=0xDEADBEEF)
        with self.assertRaises(ValueError) as ctx:
            parse_csi_payload(bad_magic_payload)
        self.assertIn("Invalid Nexmon CSI magic", str(ctx.exception))

        # 3. Buffer size default is 65535
        rx = CSIReceiver()
        self.assertEqual(rx.buffer_size, 65535)
        rx.close()

        # 4. Stream timeout loop breaks on socket timeout
        with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as s:
            s.bind(("127.0.0.1", 0))
            idle_port = s.getsockname()[1]

        rx_idle = CSIReceiver(host="127.0.0.1", port=idle_port)
        try:
            packets = list(rx_idle.stream(max_packets=5, timeout=0.05))
            self.assertEqual(len(packets), 0, "Idle stream should break on timeout without spinning forever")
        finally:
            rx_idle.close()

    # -------------------------------------------------------------------------
    # H-18: CI Sanitizer Builds & PEP 621 pyproject.toml
    # -------------------------------------------------------------------------
    def test_h18_ci_sanitizers_and_packaging(self):
        ci_yml = ROOT / ".github/workflows/ci.yml"
        text_ci = ci_yml.read_text(encoding="utf-8", errors="replace")
        self.assertIn("make -C utilities/rawproxy sanitize", text_ci)
        self.assertIn("make -C utilities/rawproxyreverse sanitize", text_ci)
        self.assertIn("make -C utilities/libnexio sanitize", text_ci)

        pyproject = ROOT / "pyproject.toml"
        text_pp = pyproject.read_text(encoding="utf-8", errors="replace")
        self.assertIn("[build-system]", text_pp)
        self.assertIn("[project]", text_pp)
        self.assertIn('name = "pynexmon"', text_pp)
        self.assertIn('where = ["python"]', text_pp)


if __name__ == "__main__":
    unittest.main()
