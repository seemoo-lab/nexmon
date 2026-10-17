#!/usr/bin/env python3
"""Unit tests for Base64 encoding/decoding used in nexutil (b64-encode.c, b64-decode.c)."""
import ctypes
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class B64TranscoderTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.tmp = tempfile.TemporaryDirectory(prefix="nexmon-b64-")
        path = Path(cls.tmp.name)
        so_path = path / "libb64_test.so"
        enc_c = ROOT / "utilities/nexutil/b64-encode.c"
        dec_c = ROOT / "utilities/nexutil/b64-decode.c"
        inc_nexutil = ROOT / "utilities/nexutil/include"

        subprocess.run(
            [
                "gcc", "-shared", "-fPIC", "-std=gnu11", "-O2",
                str(enc_c), str(dec_c),
                f"-I{inc_nexutil}",
                "-o", str(so_path)
            ],
            check=True,
            timeout=20
        )

        cls.lib = ctypes.CDLL(str(so_path))
        cls.libc = ctypes.CDLL(None)
        cls.libc.free.argtypes = [ctypes.c_void_p]

        cls.lib.b64_encode.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
        cls.lib.b64_encode.restype = ctypes.c_void_p

        cls.lib.b64_decode.argtypes = [ctypes.c_char_p, ctypes.c_size_t]
        cls.lib.b64_decode.restype = ctypes.c_void_p

        cls.lib.b64_decode_ex.argtypes = [ctypes.c_char_p, ctypes.c_size_t, ctypes.POINTER(ctypes.c_size_t)]
        cls.lib.b64_decode_ex.restype = ctypes.c_void_p

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def _encode(self, data: bytes) -> str:
        ptr = self.lib.b64_encode(data, len(data))
        if not ptr:
            return ""
        try:
            return ctypes.string_at(ptr).decode("ascii")
        finally:
            self.libc.free(ptr)

    def _decode_ex(self, b64_str: str) -> bytes:
        raw = b64_str.encode("ascii")
        dec_len = ctypes.c_size_t(0)
        ptr = self.lib.b64_decode_ex(raw, len(raw), ctypes.byref(dec_len))
        if not ptr:
            return b""
        try:
            return ctypes.string_at(ptr, dec_len.value)
        finally:
            self.libc.free(ptr)

    def test_null_inputs(self):
        """Test NULL handling on both encode and decode."""
        self.assertIsNone(self.lib.b64_encode(None, 10))
        dec_len = ctypes.c_size_t(99)
        self.assertIsNone(self.lib.b64_decode_ex(None, 10, ctypes.byref(dec_len)))
        self.assertEqual(dec_len.value, 0)

    def test_empty_string(self):
        """Test encoding and decoding of empty byte sequence."""
        res = self._encode(b"")
        self.assertEqual(res, "")
        dec = self._decode_ex("")
        self.assertEqual(dec, b"")

    def test_padding_variations(self):
        """Test base64 outputs with 0, 1, and 2 padding '=' characters."""
        # 1 byte -> 4 chars (2 pad): "YQ=="
        self.assertEqual(self._encode(b"a"), "YQ==")
        self.assertEqual(self._decode_ex("YQ=="), b"a")

        # 2 bytes -> 4 chars (1 pad): "YWI="
        self.assertEqual(self._encode(b"ab"), "YWI=")
        self.assertEqual(self._decode_ex("YWI="), b"ab")

        # 3 bytes -> 4 chars (0 pad): "YWJj"
        self.assertEqual(self._encode(b"abc"), "YWJj")
        self.assertEqual(self._decode_ex("YWJj"), b"abc")

    def test_binary_frame_with_null_bytes(self):
        """Test encoding/decoding raw 802.11 binary payloads containing embedded NULL bytes."""
        payload = b"\x80\x00\x00\x00\xff\xff\xff\xff\xff\xff\x12\x34\x56\x78\x9a\xbc\x00\x01\x02\x03\x00\x00\xde\xad\xbe\xef"
        encoded = self._encode(payload)
        self.assertTrue(len(encoded) > 0)
        decoded = self._decode_ex(encoded)
        self.assertEqual(decoded, payload)

    def test_large_frame_roundtrip(self):
        """Test round-trip of MTU-sized and A-MSDU-sized frame buffers."""
        import os
        for size in [256, 1500, 2304, 4096]:
            sample = os.urandom(size)
            enc = self._encode(sample)
            dec = self._decode_ex(enc)
            self.assertEqual(dec, sample, f"Round-trip failed for size {size}")


if __name__ == "__main__":
    unittest.main(verbosity=2)
