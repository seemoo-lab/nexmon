#!/usr/bin/env python3
"""Exercise the actual BCDC query functions with a mocked SDIO transport.

No kernel module is loaded and no radio traffic is sent. Rejection tests are
intentionally ordinary assertions, so vulnerable source produces a failing run.
"""
from pathlib import Path
import ctypes
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]
SOURCE = ROOT / "patches/driver/brcmfmac_7.3.y-nexmon/bcdc.c"

PREAMBLE = r"""
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint32_t __le32;
typedef unsigned int uint;
#define cpu_to_le32(x) (x)
#define le32_to_cpu(x) (x)
#define BRCMF_DCMD_MAXLEN 8192
#define BRCMF_TX_IOCTL_MAX_MSG_SIZE 8192
#define brcmf_dbg(...) ((void)0)
#define bphy_err(...) ((void)0)
#define bphy_err_ratelimited(...) ((void)0)
struct brcmf_proto { void *pd; };
struct brcmf_pub { struct brcmf_proto *proto; void *bus_if; };
static unsigned char response[128];
static int response_len, response_reads;
static int brcmf_bus_txctl(void *bus, unsigned char *msg, uint len) { return 0; }
static int brcmf_bus_rxctl(void *bus, unsigned char *msg, uint len) {
    if (response_reads++) return -ETIMEDOUT;
    memcpy(msg, response, len < (uint)response_len ? len : (uint)response_len);
    return response_len;
}
"""

HARNESS = r"""
int query(int wire_len, int payload_len, int reply_cmd, int reply_ifidx,
          int reply_id, int error, unsigned int *value, int *fwerr) {
    struct brcmf_bcdc bcdc = {0};
    struct brcmf_proto proto = {.pd = &bcdc};
    struct brcmf_pub drvr = {.proto = &proto};
    struct brcmf_proto_bcdc_dcmd hdr = {
        .cmd = reply_cmd, .len = payload_len,
        .flags = ((u32)reply_id << 16) | ((u32)reply_ifidx << 12) | (error ? 1 : 0),
        .status = error
    };
    unsigned char buf[13] = "chanspec";
    u32 channel = 0x1006;
    memset(response, 0, sizeof(response));
    memcpy(response, &hdr, sizeof(hdr));
    memcpy(response + sizeof(hdr), &channel, sizeof(channel));
    response_len = wire_len;
    response_reads = 0;
    int ret = brcmf_proto_bcdc_query_dcmd(&drvr, 1, 262, buf, sizeof(buf), fwerr);
    memcpy(value, buf, sizeof(*value));
    return ret;
}
"""


class BcdcResponseTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        if __import__("sys").byteorder != "little":
            raise unittest.SkipTest("Harness requires a little-endian host")
        source = SOURCE.read_text()
        definitions = source[source.index("struct brcmf_proto_bcdc_dcmd {"):
                             source.index("struct brcmf_fws_info *drvr_to_fws")]
        functions = source[source.index("static int\nbrcmf_proto_bcdc_msg("):
                           source.index("static int\nbrcmf_proto_bcdc_set_dcmd(")]
        cls.tmp = tempfile.TemporaryDirectory(prefix="nexmon-bcdc-unit-")
        cfile = Path(cls.tmp.name) / "query.c"
        libfile = Path(cls.tmp.name) / "query.so"
        cfile.write_text(PREAMBLE + definitions + functions + HARNESS)
        subprocess.run(["gcc", "-shared", "-fPIC", "-O0", "-Wall", "-Wextra",
                        "-Wno-unused-parameter", "-Wno-sign-compare", "-Werror",
                        str(cfile), "-o", str(libfile)], check=True, timeout=30)
        cls.lib = ctypes.CDLL(str(libfile))
        cls.lib.query.argtypes = [ctypes.c_int] * 6 + [
            ctypes.POINTER(ctypes.c_uint), ctypes.POINTER(ctypes.c_int)]
        cls.lib.query.restype = ctypes.c_int

    @classmethod
    def tearDownClass(cls):
        cls.tmp.cleanup()

    def query(self, wire_len=20, payload_len=4, cmd=262, iface=1, reqid=1, error=0):
        value, fwerr = ctypes.c_uint(), ctypes.c_int()
        ret = self.lib.query(wire_len, payload_len, cmd, iface, reqid, error,
                             ctypes.byref(value), ctypes.byref(fwerr))
        return ret, fwerr.value, value.value

    def assert_rejected(self, **kwargs):
        ret, fwerr, value = self.query(**kwargs)
        self.assertTrue(ret < 0 or fwerr < 0,
                        f"malformed reply accepted: ret={ret}, fwerr={fwerr}, value=0x{value:08x}")

    def test_valid_channel_reply(self):
        self.assertEqual(self.query(), (0, 0, 0x1006))

    def test_firmware_error_propagated(self):
        self.assertEqual(self.query(error=-23)[:2], (0, -23))

    def test_wrong_request_id_rejected(self):
        self.assert_rejected(reqid=2)

    def test_truncated_header_rejected(self):
        self.assert_rejected(wire_len=8)

    def test_missing_declared_payload_rejected(self):
        self.assert_rejected(wire_len=16)

    def test_truncated_declared_payload_rejected(self):
        self.assert_rejected(wire_len=18)

    def test_wrong_command_rejected(self):
        self.assert_rejected(cmd=263)

    def test_wrong_interface_rejected(self):
        self.assert_rejected(iface=2)

    def test_primary_interface_reply_accepted(self):
        # Broadcom firmware answers radio-level queries (e.g. chanspec) on primary ifidx 0
        ret, fwerr, val = self.query(iface=0)
        self.assertEqual((ret, fwerr, val), (0, 0, 0x1006))


if __name__ == "__main__":
    unittest.main(verbosity=2)
