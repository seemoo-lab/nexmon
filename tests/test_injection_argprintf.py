#!/usr/bin/env python3
"""Reproduce TX logging corruption with the actual argprintf implementation."""
import ctypes
from pathlib import Path
import re
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class InjectionLoggingTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        source = (ROOT / "patches/common/argprintf.c").read_text()
        source = source[source.index("static int argprintf_written"):]
        source = source[:source.index("void\narghexdump")] + source[source.index("void\nargprintf_init"):]
        sendframe = (ROOT / "patches/bcm43455c0/7_45_265-28bca26-CY/nexmon/src/sendframe.c").read_text()
        match = re.search(r'^\s*argprintf\("sendframe called:[^\n]+', sendframe, re.M)
        cls.tx_log = match[0].strip() if match else ""
        cls.tmp = tempfile.TemporaryDirectory(prefix="nexmon-argprintf-")
        path = Path(cls.tmp.name)
        cfile = path / "test.c"
        cfile.write_text("#include <stdarg.h>\n#include <stdio.h>\n#include <stdbool.h>\n#include <string.h>\n"
                         + source + "\nvoid tx_log(void) { void *wlc=(void*)0x1234, *p=(void*)0x5678; "
                         "unsigned int fifo=1, rate=2; " + cls.tx_log + " }\n"
                         "int written(void) { return argprintf_written; }\n")
        subprocess.run(["gcc", "-shared", "-fPIC", "-O0", str(cfile), "-o", str(path / "test.so")],
                       check=True, timeout=20)
        cls.lib = ctypes.CDLL(str(path / "test.so"))
        cls.lib.argprintf_init.argtypes = [ctypes.c_void_p, ctypes.c_int]
        cls.lib.written.restype = ctypes.c_int

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def test_tx_logging_does_not_overwrite_ioctl_reply(self):
        buf = ctypes.create_string_buffer(4096)
        buf.raw = b"\x0b\x10\x00\x00" + b"Q" * 4092
        self.lib.argprintf_init(buf, 13)
        original = buf.raw[:13]
        self.lib.tx_log()
        value = int.from_bytes(buf.raw[:2], "little")
        self.assertEqual(buf.raw[:13], original,
                         f"TX log overwrote IOCTL reply; low chanspec is now {value:#06x}")

    def test_formatter_cursor_stays_inside_buffer(self):
        buf = ctypes.create_string_buffer(4096)
        self.lib.argprintf_init(buf, 13)
        # The extra backing allocation makes observation safe without allowing
        # a subsequent call to write beyond the declared 13-byte reply.
        self.lib.tx_log()
        self.assertLess(self.lib.written(), 13,
                        "vsnprintf's would-have-written length escapes the reply buffer")


if __name__ == "__main__":
    unittest.main(verbosity=2)
