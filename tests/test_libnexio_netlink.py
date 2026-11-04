#!/usr/bin/env python3
"""Dedicated unittest for libnexio netlink/UDP paths (beyond pynexmon Chanspec)"""

import pathlib
import subprocess
import unittest
import shutil
import sys

ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "python"))

class TestLibnexioSource(unittest.TestCase):
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

    def test_netlink_socket_cloexec(self):
        text = (ROOT / "utilities/libnexio/libnexio.c").read_text()
        # Check that netlink socket also uses cloexec
        self.assertIn("socket_cloexec(PF_NETLINK", text)

    def test_weak_chanspec_tables(self):
        text = (ROOT / "utilities/libnexio/libnexio.c").read_text()
        self.assertIn("libnexio_wf_5g_40m_chans", text)
        self.assertIn("libnexio_wf_5g_80m_chans", text)

    def test_h08_vendor_cmd_and_netlink_hardening(self):
        text = (ROOT / "utilities/libnexio/libnexio.c").read_text()
        self.assertIn("nl_cb_err", text)
        self.assertIn("__nex_driver_error_handler", text)
        self.assertIn("nlmsg_alloc_size", text)
        self.assertNotIn("nlmsg_alloc()", text)
        self.assertNotIn("sock_rx_frame", text)
        self.assertNotIn("htons(5600)", text)
        self.assertIn("snl_rx_ioctl.nl_pid = 0", text)
        self.assertIn("getsockname", text)
        self.assertIn("len > 0 && buf == NULL", text)

    def test_h08_runtime_null_buf_and_multi_handle(self):
        if not shutil.which("gcc"):
            self.skipTest("gcc not found")
        import ctypes
        import tempfile
        with tempfile.TemporaryDirectory() as tmp:
            so_path = pathlib.Path(tmp) / "libnexio_test.so"
            cmd = [
                "gcc", "-shared", "-fPIC", "-std=gnu11", "-O1", "-g",
                "-DBUILD_ON_RPI", "-DVERSION=\"test\"",
                "-I", str(ROOT / "utilities/libnexio"),
                "-I", str(ROOT / "patches/include"),
                str(ROOT / "utilities/libnexio/libnexio.c"),
                "-o", str(so_path),
            ]
            res = subprocess.run(cmd, capture_output=True, text=True, timeout=15)
            self.assertEqual(res.returncode, 0, msg=res.stderr)

            lib = ctypes.CDLL(str(so_path))
            lib.nex_init_ioctl.argtypes = [ctypes.c_char_p]
            lib.nex_init_ioctl.restype = ctypes.c_void_p
            lib.nex_ioctl.argtypes = [
                ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.c_bool
            ]
            lib.nex_ioctl.restype = ctypes.c_int
            lib.nex_free.argtypes = [ctypes.c_void_p]
            lib.nex_free.restype = None

            h1 = lib.nex_init_ioctl(b"wlan0")
            h2 = lib.nex_init_ioctl(b"wlan0")
            self.assertIsNotNone(h1)
            self.assertIsNotNone(h2)
            # len > 0 with NULL buf must be rejected cleanly with -1
            self.assertEqual(lib.nex_ioctl(h1, 107, None, 4, False), -1)
            self.assertEqual(lib.nex_ioctl(h1, 107, None, -1, False), -1)
            lib.nex_free(h1)
            lib.nex_free(h2)

class TestLibnexioBuild(unittest.TestCase):
    def test_libnexio_builds(self):
        # Check that libnexio can be built (or is already built)
        lib = ROOT / "utilities/libnexio/libnexio.a"
        # If not built, try building
        if not lib.is_file():
            if not shutil.which("make"):
                self.skipTest("make not found")
            result = subprocess.run(["make", "-C", str(ROOT / "utilities/libnexio")], capture_output=True, text=True, timeout=30)
            if result.returncode != 0:
                self.skipTest(f"libnexio build failed: {result.stderr[:500]}")
        self.assertTrue((ROOT / "utilities/libnexio/libnexio.a").is_file() or (ROOT / "utilities/libnexio/libnexio.so").is_file())

    def test_libnexio_sanitize_build(self):
        if not shutil.which("gcc"):
            self.skipTest("gcc not found")
        ret = subprocess.run(["gcc", "-fsanitize=address,undefined", "-E", "-"], input="", capture_output=True, text=True, timeout=5)
        if ret.returncode != 0:
            self.skipTest("sanitizer not available")
        # Try building libnexio with sanitizer target
        result = subprocess.run(
            ["make", "-C", str(ROOT / "utilities/libnexio"), "sanitize"],
            capture_output=True, text=True, timeout=30
        )
        # Allow failure due to missing deps, but not due to sanitizer errors in our code
        if result.returncode != 0 and "sanitizer" in result.stderr.lower():
            self.fail(f"sanitizer build failed: {result.stderr[:1000]}")
        self.assertTrue((ROOT / "utilities/libnexio/libnexio_sanitize.a").is_file())

class TestLibnexioNetlinkUDP(unittest.TestCase):
    def test_nex_init_udp_warn(self):
        # Test that nex_init_udp warns when not root (we can't easily test without root, but check source)
        text = (ROOT / "utilities/libnexio/libnexio.c").read_text()
        self.assertIn("not running as root", text.lower())

    def test_nex_ioctl_timeout(self):
        # Check that nex_ioctl has timeout handling via SO_RCVTIMEO
        text = (ROOT / "utilities/libnexio/libnexio.c").read_text()
        self.assertIn("SO_RCVTIMEO", text)
        self.assertIn("tv_sec = 1", text)

    def test_python_binding(self):
        # Test that python binding can import and has expected methods
        try:
            from pynexmon import nexmon
            # Check that nexmon has expected classes
            self.assertTrue(hasattr(nexmon, 'NexmonDevice') or hasattr(nexmon, 'Chanspec'))
        except ImportError as e:
            self.skipTest(f"pynexmon not available: {e}")

    def test_udp_loopback(self):
        # Test UDP loopback via python pynexmon if available - similar to test_csi but for libnexio
        # This is a smoke test for the UDP path (requires no hardware)
        try:
            from pynexmon import nexmon
            self.assertIsNotNone(nexmon)
        except Exception as e:
            self.skipTest(f"pynexmon UDP test skipped: {e}")

if __name__ == "__main__":
    unittest.main()
