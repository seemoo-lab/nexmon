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

    def test_unpadded_valid_remainders(self):
        """Test 2-char (1 byte) and 3-char (2 byte) unpadded base64 remainders."""
        self.assertEqual(self._decode_ex("YQ"), b"a")
        self.assertEqual(self._decode_ex("YWI"), b"ab")

    def test_malformed_single_char_quantum_no_double_free(self):
        """Regression test for C-06: 1-char trailing quantum must return NULL without double-free."""
        for bad in [b"A", b"A=", b"A===", b"YWJjA", b"YWJjA==="]:
            dec_len = ctypes.c_size_t(123)
            ptr = self.lib.b64_decode_ex(bad, len(bad), ctypes.byref(dec_len))
            self.assertIsNone(ptr, f"Expected NULL for invalid base64 {bad!r}")
            self.assertEqual(dec_len.value, 0)

    def test_non_ascii_high_bytes_safe(self):
        """Regression test for C-06: signed-char negative bytes (>= 0x80) must not trigger UB or table corruption."""
        for payload in [b"\xff\xfe\xfd\xfc", b"YQ==\xff", b"\x80"]:
            dec_len = ctypes.c_size_t(77)
            ptr = self.lib.b64_decode_ex(payload, len(payload), ctypes.byref(dec_len))
            if ptr:
                try:
                    out = ctypes.string_at(ptr, dec_len.value)
                    if payload.startswith(b"YQ=="):
                        self.assertEqual(out, b"a")
                    else:
                        self.assertEqual(out, b"")
                finally:
                    self.libc.free(ptr)

    def test_asan_ubsan_malformed_inputs(self):
        """Compile b64-decode.c with ASan+UBSan and hammer 1-char and high-byte inputs."""
        check = subprocess.run(
            ["gcc", "-fsanitize=address,undefined", "-E", "-"],
            input="", text=True, capture_output=True, timeout=5
        )
        if check.returncode != 0:
            self.skipTest("ASan/UBSan not available")

        with tempfile.TemporaryDirectory(prefix="nexmon-b64-asan-") as tmp:
            exe = Path(tmp) / "b64_asan"
            harness = Path(tmp) / "harness.c"
            harness.write_text(
                '#include <stdlib.h>\n'
                '#include <string.h>\n'
                '#include <assert.h>\n'
                '#include "b64.h"\n'
                'int main(void) {\n'
                '    size_t sz = 99;\n'
                '    assert(b64_decode_ex("A", 1, &sz) == NULL && sz == 0);\n'
                '    sz = 99;\n'
                '    assert(b64_decode_ex("A===", 4, &sz) == NULL && sz == 0);\n'
                '    sz = 99;\n'
                '    assert(b64_decode_ex("YWJjZ", 5, &sz) == NULL && sz == 0);\n'
                '    unsigned char *ok = b64_decode_ex("YWJj", 4, &sz);\n'
                '    assert(ok != NULL && sz == 3 && memcmp(ok, "abc", 3) == 0);\n'
                '    free(ok);\n'
                '    ok = b64_decode_ex("\\xff\\xfe", 2, &sz);\n'
                '    assert(ok != NULL && sz == 0);\n'
                '    free(ok);\n'
                '    return 0;\n'
                '}\n'
            )
            subprocess.run(
                [
                    "gcc", "-std=gnu11", "-O1", "-g", "-fsanitize=address,undefined",
                    "-fno-omit-frame-pointer",
                    str(harness),
                    str(ROOT / "utilities/nexutil/b64-decode.c"),
                    str(ROOT / "utilities/nexutil/b64-encode.c"),
                    f"-I{ROOT / 'utilities/nexutil'}",
                    f"-I{ROOT / 'utilities/nexutil/include'}",
                    "-o", str(exe),
                ],
                check=True,
                timeout=20,
            )
            res = subprocess.run([str(exe)], capture_output=True, text=True, timeout=10)
            self.assertEqual(res.returncode, 0, res.stderr)


if __name__ == "__main__":
    unittest.main(verbosity=2)
