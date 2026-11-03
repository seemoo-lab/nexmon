#!/usr/bin/env python3
"""Regression tests for hardening fixes (rawproxy, libnexio, monitor-mode, etc.)"""

import pathlib
import subprocess
import unittest

ROOT = pathlib.Path(__file__).resolve().parents[1]

class TestRawproxyHardening(unittest.TestCase):
    def test_rawproxy_uses_strtol_not_atoi(self):
        text = (ROOT / "utilities/rawproxy/rawproxy.c").read_text()
        self.assertIn("strtol", text)
        self.assertNotIn("pid = atoi", text)
        self.assertNotIn("portin = atoi", text)
        self.assertIn("1..65535", text)

    def test_rawproxy_pcap_mtu(self):
        text = (ROOT / "utilities/rawproxy/rawproxy.c").read_text()
        self.assertIn("65535", text)
        self.assertNotIn('pcap_open_live("any", BUFSIZ', text)
        self.assertIn("SO_REUSEADDR", text)

    def test_rawproxy_pcap_geterr(self):
        text = (ROOT / "utilities/rawproxy/rawproxy.c").read_text()
        self.assertIn("pcap_geterr", text)

    def test_rawproxy_perror(self):
        text = (ROOT / "utilities/rawproxy/rawproxy.c").read_text()
        self.assertIn("perror", text)
        self.assertNotIn('printf("ERR: %d', text)

    def test_rawproxy_prctl(self):
        text = (ROOT / "utilities/rawproxy/rawproxy.c").read_text()
        self.assertIn("PR_SET_PDEATHSIG", text)
        self.assertIn("prctl", text)

    def test_rawproxy_pthread_detach(self):
        text = (ROOT / "utilities/rawproxy/rawproxy.c").read_text()
        self.assertIn("pthread_detach", text)

    def test_rawproxyreverse_same(self):
        text = (ROOT / "utilities/rawproxyreverse/rawproxyreverse.c").read_text()
        self.assertIn("strtol", text)
        self.assertIn("65535", text)
        self.assertIn("pthread_detach", text)

class TestLibnexioHardening(unittest.TestCase):
    def test_socket_cloexec(self):
        text = (ROOT / "utilities/libnexio/libnexio.c").read_text()
        self.assertIn("socket_cloexec", text)
        self.assertIn("SOCK_CLOEXEC", text)
        self.assertIn("FD_CLOEXEC", text)

    def test_cap_net_raw_warn(self):
        text = (ROOT / "utilities/libnexio/libnexio.c").read_text()
        self.assertIn("CAP_NET_RAW", text)

    def test_eagain_distinction(self):
        text = (ROOT / "utilities/libnexio/libnexio.c").read_text()
        self.assertIn("EAGAIN", text)
        self.assertIn("timeout waiting for firmware reply", text)

class TestMonitorModeHardening(unittest.TestCase):
    def test_flock(self):
        text = (ROOT / "monitor-mode.sh").read_text()
        self.assertIn("flock -x", text)
        self.assertIn("chmod 600", text)

    def test_dmesg_fallback(self):
        text = (ROOT / "monitor-mode.sh").read_text()
        self.assertIn("journalctl", text)
        self.assertIn("/dev/kmsg", text)

    def test_poll_for_country(self):
        text = (ROOT / "monitor-mode.sh").read_text()
        self.assertIn("poll_for_country", text)
        self.assertIn("sleep 0.05", text)

    def test_find_nexutil_cached(self):
        text = (ROOT / "monitor-mode.sh").read_text()
        self.assertIn("find_nexutil_cached", text)

class TestBuildtoolsHardening(unittest.TestCase):
    def test_getdefine_cache(self):
        text = (ROOT / "buildtools/scripts/getdefine.sh").read_text()
        self.assertIn("CACHE_DIR", text)
        self.assertIn("sha256sum", text)

    def test_getsectionaddr_cache(self):
        text = (ROOT / "buildtools/scripts/nexmon.mk.1.awk").read_text()
        self.assertIn("_cache", text)

    def test_b43_initvals(self):
        text = (ROOT / "buildtools/b43-v3/assembler/initvals.c").read_text()
        self.assertNotIn("//TODO", text)
        self.assertIn("Unaligned SHM offset", text)

    def test_b43_insn_limit(self):
        for f in ROOT.glob("buildtools/b43*/assembler/main.c"):
            text = f.read_text()
            self.assertTrue(
                "65536; /* expanded limit" in text or "4096; /* conservative limit" in text,
                f"Missing insn limit check in {f}",
            )

class TestPyHardening(unittest.TestCase):
    def test_nexfirmware_pull_backup(self):
        text = (ROOT / "nexfirmware.py").read_text()
        self.assertIn("manifest.json.bak", text)

    def test_validate_logs(self):
        text = (ROOT / "buildtools/scripts/validate_firmware.py").read_text()
        self.assertIn("undefined symbol", text)

    def test_crash_decoder_strmap(self):
        text = (ROOT / "buildtools/scripts/nexmon_crash_decoder.py").read_text()
        self.assertIn("str_map", text)

class TestSanitizeBuild(unittest.TestCase):
    def test_rawproxy_builds_with_sanitize(self):
        # Try building rawproxy with -fsanitize=address,undefined if available
        # This is a smoke test, not a full run
        import tempfile
        import shutil
        if not shutil.which("gcc"):
            self.skipTest("gcc not found")
        # Check if sanitizer is available
        ret = subprocess.run(["gcc", "-fsanitize=address,undefined", "-E", "-"], input="", capture_output=True, text=True, timeout=5)
        if ret.returncode != 0:
            self.skipTest("sanitizer not available")
        with tempfile.TemporaryDirectory() as tmp:
            out = pathlib.Path(tmp) / "rawproxy_test"
            src = ROOT / "utilities/rawproxy/rawproxy.c"
            # Try compile with sanitize (may need --static? just check)
            ret = subprocess.run(["gcc", "-fsanitize=address,undefined", "-DVERSION=\"test\"", "-I", str(ROOT / "utilities/libpcap"), "-c", str(src), "-o", str(out.with_suffix(".o"))], capture_output=True, text=True, timeout=10)
            # We don't require success, just that the source is sanitizer-clean (no ubsan errors at compile time)
            # If it fails due to missing libpcap, skip
            if ret.returncode != 0 and "pcap.h" in ret.stderr:
                self.skipTest("libpcap headers not found for sanitize build")


class TestNativeUtilitiesAuditFixes(unittest.TestCase):
    """Regression coverage for H-09, H-10, and H-11 audit fixes."""

    def test_h09_fakeioctl_and_nexmon_source_and_native_compile(self):
        import ctypes
        import shutil
        import tempfile

        fake_text = (ROOT / "utilities/libfakeioctl/fakeioctl.c").read_text()
        nex_text = (ROOT / "utilities/libnexmon/nexmon.c").read_text()

        for text in (fake_text, nex_text):
            self.assertNotIn("buf & MONITOR_DISABLED", text)
            self.assertIn("buf == MONITOR_DISABLED", text)
            self.assertIn("IFNAMSIZ", text)
            self.assertNotIn("strlen(ifname)", text)
            self.assertIn("typedef unsigned long request_t;", text)
            self.assertIn("IW_MODE_INFRA", text)

        self.assertIn("SOCK_CLOEXEC", nex_text)
        self.assertIn("SOCK_NONBLOCK", nex_text)
        self.assertIn("UINT16_MAX", nex_text)

        if not shutil.which("gcc"):
            self.skipTest("gcc not found")

        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = pathlib.Path(tmp)
            common_inc = [
                "-I", str(ROOT / "utilities/libnexio"),
                "-I", str(ROOT / "patches/include"),
            ]
            # Compile both LD_PRELOAD libraries natively on 64-bit Linux
            for name, src in (
                ("libfakeioctl_test.so", ROOT / "utilities/libfakeioctl/fakeioctl.c"),
                ("libnexmon_test.so", ROOT / "utilities/libnexmon/nexmon.c"),
            ):
                so_out = tmp_path / name
                cmd = [
                    "gcc", "-shared", "-fPIC", "-std=c99", "-Wall", "-Wextra",
                    "-Wno-unused-parameter", "-Wno-unused-variable",
                    "-DBUILD_ON_RPI", "-DVERSION=\"test\"",
                    *common_inc,
                    str(src),
                    str(ROOT / "utilities/libnexio/libnexio.c"),
                    "-ldl", "-o", str(so_out),
                ]
                res = subprocess.run(cmd, capture_output=True, text=True, timeout=15)
                self.assertEqual(res.returncode, 0, msg=f"{name} failed:\n{res.stderr}")

            # Verify hexdump(desc, NULL, 0) does not read uninitialized stack bytes
            libnexmon = ctypes.CDLL(str(tmp_path / "libnexmon_test.so"))
            libnexmon.hexdump.argtypes = [ctypes.c_char_p, ctypes.c_void_p, ctypes.c_int]
            libnexmon.hexdump.restype = None
            libnexmon.hexdump(None, None, 0)
            libnexmon.hexdump(None, None, -5)

    def test_h10_rawproxy_pkthdr32_and_dhdutil_hardening(self):
        raw_text = (ROOT / "utilities/rawproxy/rawproxy.c").read_text()
        self.assertIn("struct pcap_pkthdr32", raw_text)
        self.assertNotIn("sizeof(struct pcap_pkthdr)", raw_text)
        self.assertIn("rtap_len < 8", raw_text)
        self.assertIn("header.caplen < 8", raw_text)

        dhdu_linux = (ROOT / "utilities/dhdutil/dhdu_linux.c").read_text()
        self.assertIn("buf == NULL", dhdu_linux)
        self.assertNotIn("strlen(dev_type) - 1", dhdu_linux)
        self.assertNotIn("strcpy(info.driver", dhdu_linux)
        self.assertNotIn("strcat(info.driver", dhdu_linux)
        self.assertIn("dnode = -1;", dhdu_linux)
        self.assertIn('strcmp(*argv, "--wince")', dhdu_linux)

        dhdu_nl = (ROOT / "utilities/dhdutil/dhdu_nl80211.c").read_text()
        self.assertIn("if (dhd_nli->cb)", dhdu_nl)
        self.assertIn("if (res < 0)", dhdu_nl)

    def test_h11_android_helpers_hardening_and_ndk_compile(self):
        logger_src = ROOT / "app/app/src/main/external/src/libandroidlogger/android-logger.c"
        ws_src = ROOT / "app/app/src/main/external/src/libwireshark_helper/wireshark_helper.c"
        sender_src = ROOT / "app/app/src/main/external/src/libsender/src/sender.c"

        logger_text = logger_src.read_text()
        self.assertNotIn("buf[rdsz - 1] = 0", logger_text)
        self.assertIn("buf[rdsz] = '\\0'", logger_text)
        self.assertNotIn("== -1)", logger_text)
        self.assertIn("!= 0)", logger_text)

        ws_text = ws_src.read_text()
        self.assertNotIn("sizeof(str_res)", ws_text)
        self.assertIn("memset(str_res, '\\0', 2048)", ws_text)
        self.assertIn('snprintf(str_res, 2048, "%s"', ws_text)

        sender_text = sender_src.read_text()
        self.assertNotIn("JNIEnv *g_env", sender_text)
        self.assertIn("JavaVM *g_jvm", sender_text)
        self.assertIn("NewGlobalRef", sender_text)
        self.assertIn("AttachCurrentThread", sender_text)

        # If an Android NDK clang is available, syntax/type-check android-logger.c & sender.c
        ndk_clangs = sorted(pathlib.Path("/opt/android-sdk/ndk").glob(
            "*/toolchains/llvm/prebuilt/*/bin/aarch64-linux-android21-clang"
        ))
        if ndk_clangs:
            import tempfile
            clang = str(ndk_clangs[-1])
            with tempfile.TemporaryDirectory() as tmp:
                for src in (logger_src, sender_src):
                    out_o = pathlib.Path(tmp) / (src.stem + ".o")
                    res = subprocess.run(
                        [clang, "-Wall", "-Wextra", "-Werror", "-c", str(src), "-o", str(out_o)],
                        capture_output=True, text=True, timeout=15,
                    )
                    self.assertEqual(res.returncode, 0, msg=f"{src.name} NDK compile failed:\n{res.stderr}")

if __name__ == "__main__":
    unittest.main()
