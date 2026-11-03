#!/usr/bin/env python3
"""Regression tests for Tier 1 Critical audit fixes (C-01 through C-07)."""

import os
from pathlib import Path
import re
import shutil
import struct
import subprocess
import tempfile
import textwrap
import unittest


ROOT = Path(__file__).resolve().parents[1]


class TestC01DriverNetlinkHardening(unittest.TestCase):
    """C-01: Verify Netlink security, bounds, and lifetime hardening across all 14 brcmfmac trees."""

    def test_all_drivers_enforce_netlink_hardening(self):
        drivers = sorted(ROOT.glob("patches/driver/brcmfmac_*/core.c"))
        self.assertEqual(len(drivers), 14)
        for core_c in drivers:
            ver = core_c.parent.name
            text = core_c.read_text()
            self.assertNotIn("ndev_global", text, f"{ver} still uses raw ndev_global")
            self.assertIn("netlink_capable(skb, CAP_NET_ADMIN)", text, ver)
            self.assertIn("nlmsg_ok(nlh, skb->len)", text, ver)
            self.assertIn("BRCMF_DCMD_MAXLEN", text, ver)
            self.assertIn("skb_is_nonlinear(skb)", text, ver)
            self.assertIn("kmalloc(frame_len, GFP_KERNEL)", text, ver)
            self.assertIn("nexmon_default_ifindex", text, ver)
            self.assertIn("dev_get_by_index(&init_net, ifindex)", text, ver)
            self.assertIn("dev_hold(target_ndev)", text, ver)
            self.assertIn("dev_put(ndev)", text, ver)
            self.assertIn("NETLINK_CB(skb).portid", text, ver)
            self.assertNotIn("nlmsg_new(4, 0)", text, ver)
            self.assertNotIn("nlh->nlmsg_pid", text, ver)


class TestC02ObjmemAlignmentAndMask(unittest.TestCase):
    """C-02: Verify wlc_bmac_write_objmem32 and wlc_bmac_write_objmem32_objaddr handle odd DWORDs."""

    @unittest.skipUnless(shutil.which("gcc"), "gcc required")
    def test_objmem32_even_and_odd_dword_writes(self):
        with tempfile.TemporaryDirectory(prefix="nexmon-objmem-") as tmp:
            tmp_path = Path(tmp)
            harness_c = tmp_path / "objmem_harness.c"
            exe = tmp_path / "objmem_test"

            objmem_body = (ROOT / "patches/common/objmem.c").read_text()
            # Strip firmware-only includes/pragmas so we can test the exact C functions against a register model.
            objmem_body = re.sub(r"^\s*#(?:include|pragma)\b.*$", "", objmem_body, flags=re.M)

            harness_c.write_text(textwrap.dedent("""
                #include <stdint.h>
                #include <stdio.h>
                #include <string.h>
                #include <assert.h>

                /* Hardware simulation: d11 object memory latches QWORD reads/writes on odd objaddr accesses,
                 * or direct 32-bit reads at objaddr. We intercept objaddr/objdata accesses via a simple struct
                 * where tests verify the exact sequence of (objaddr, objdata) reads & writes! */
                struct d11regs {
                    uint32_t objaddr;
                    uint32_t objdata;
                };
                struct wlc_hw_info {
                    volatile struct d11regs *regs;
                };
            """) + "\n" + objmem_body + "\n" + textwrap.dedent("""
                /* Note: real Broadcom d11 hardware advances/selects backing RAM word on each objaddr write.
                 * Since C cannot overload `regs->objdata = x` on plain memory, we inspect source invariants AND
                 * trace register-level calls by wrapping read_objmem64_objaddr / write_objmem64_objaddr! */
                int main(void) {
                    struct d11regs r = {0, 0};
                    struct wlc_hw_info hw = { &r };
                    unsigned int val = 0;
                    wlc_bmac_read_objmem32(&hw, 4, &val, 0x00020000);
                    assert(r.objaddr == (0x00020000U | 1U));
                    return 0;
                }
            """))

            # Also build a trace harness that replaces the 64-bit register accessors with a 64-word backing array
            # so we test wlc_bmac_write_objmem32_objaddr, wlc_bmac_write_objmem32, and wlc_bmac_write_objmem_byte end-to-end!
            trace_c = tmp_path / "objmem_trace.c"
            fn_split = objmem_body.split("wlc_bmac_write_objmem32_objaddr", 1)[1]
            trace_c.write_text(textwrap.dedent("""
                #include <stdint.h>
                #include <assert.h>

                static uint32_t mem[64];
                struct wlc_hw_info { int dummy; };

                void wlc_bmac_read_objmem32_objaddr(struct wlc_hw_info *wlc_hw, unsigned int objaddr, unsigned int *val) {
                    (void)wlc_hw;
                    unsigned int idx = objaddr & 0x3fU;
                    *val = mem[idx];
                }
                void wlc_bmac_read_objmem32(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned int *val, int sel) {
                    wlc_bmac_read_objmem32_objaddr(wlc_hw, sel | ((offset & 0xfffffffc) >> 2), val);
                }
                void wlc_bmac_read_objmem64_objaddr(struct wlc_hw_info *wlc_hw, unsigned int objaddr, unsigned int *val_low, unsigned int *val_high) {
                    (void)wlc_hw;
                    assert((objaddr & 1U) == 0); /* Hardware requires QWORD-aligned even objaddr! */
                    unsigned int idx = objaddr & 0x3fU;
                    *val_low = mem[idx];
                    *val_high = mem[idx + 1];
                }
                void wlc_bmac_read_objmem64(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned int *val_low, unsigned int *val_high, int sel) {
                    wlc_bmac_read_objmem64_objaddr(wlc_hw, sel | ((offset & 0xfffffff8) >> 2), val_low, val_high);
                }
                void wlc_bmac_write_objmem64_objaddr(struct wlc_hw_info *wlc_hw, unsigned int objaddr, unsigned int val_low, unsigned int val_high) {
                    (void)wlc_hw;
                    assert((objaddr & 1U) == 0); /* Hardware requires QWORD-aligned even objaddr! */
                    unsigned int idx = objaddr & 0x3fU;
                    mem[idx] = val_low;
                    mem[idx + 1] = val_high;
                }
                void wlc_bmac_write_objmem64(struct wlc_hw_info *wlc_hw, unsigned int offset, unsigned int val_low, unsigned int val_high, int sel) {
                    wlc_bmac_write_objmem64_objaddr(wlc_hw, sel | ((offset & 0xfffffff8) >> 2), val_low, val_high);
                }

                void
                wlc_bmac_write_objmem32_objaddr""") + fn_split + textwrap.dedent("""

                int main(void) {
                    struct wlc_hw_info hw = {0};
                    for (int i = 0; i < 16; i++)
                        mem[i] = 0x11110000U + (unsigned int)i;

                    /* Write even DWORD (offset 0 -> objaddr 0) */
                    wlc_bmac_write_objmem32(&hw, 0, 0xdeadbeefU, 0x00020000);
                    assert(mem[0] == 0xdeadbeefU);
                    assert(mem[1] == 0x11110001U);

                    /* Write odd DWORD (offset 4 -> objaddr 1): previously overwrote mem[0] or corrupted mem[2]! */
                    wlc_bmac_write_objmem32(&hw, 4, 0xcafebabeU, 0x00020000);
                    assert(mem[0] == 0xdeadbeefU);
                    assert(mem[1] == 0xcafebabeU);
                    assert(mem[2] == 0x11110002U);

                    /* Direct odd objaddr write (objaddr 3 -> mem[3]) */
                    wlc_bmac_write_objmem32_objaddr(&hw, 0x00020003U, 0x8badf00dU);
                    assert(mem[2] == 0x11110002U);
                    assert(mem[3] == 0x8badf00dU);
                    assert(mem[4] == 0x11110004U);

                    unsigned int rd = 0;
                    wlc_bmac_read_objmem32(&hw, 4, &rd, 0x00020000);
                    assert(rd == 0xcafebabeU);
                    wlc_bmac_read_objmem32(&hw, 12, &rd, 0x00020000);
                    assert(rd == 0x8badf00dU);
                    return 0;
                }
            """))

            for src in (harness_c, trace_c):
                subprocess.run(["gcc", "-std=gnu11", "-Wall", "-Wextra", "-O2", str(src), "-o", str(exe)],
                               check=True, timeout=15)
                subprocess.run([str(exe)], check=True, timeout=5)


class TestC03C04GccPluginAndAwkPipeline(unittest.TestCase):
    """C-03 & C-04: Verify GCC plugin AST validation, alignment clamping, and REGION awk pipeline."""

    def _find_cross_gcc_and_plugin(self):
        aarch64_gcc = ROOT / "buildtools/arm-gnu-toolchain-15.3.rel1-aarch64-arm-none-eabi/bin/arm-none-eabi-gcc"
        aarch64_plugin = ROOT / "buildtools/gcc-nexmon-plugin-aarch64/nexmon.so"
        if os.uname().machine == "aarch64" and aarch64_gcc.is_file() and aarch64_plugin.is_file():
            return aarch64_gcc, aarch64_plugin
        x86_gcc = ROOT / "buildtools/gcc-arm-none-eabi-5_4-2016q2-linux-x86/bin/arm-none-eabi-gcc"
        x86_plugin = ROOT / "buildtools/gcc-nexmon-plugin/nexmon.so"
        if os.uname().machine == "x86_64" and x86_gcc.is_file() and x86_plugin.is_file():
            try:
                probe = subprocess.run([str(x86_gcc), f"-fplugin={x86_plugin}", "-v"], capture_output=True, timeout=5)
                if probe.returncode == 0:
                    return x86_gcc, x86_plugin
            except Exception:
                pass
            return None, None
        return None, None

    def test_plugin_rejects_invalid_at_arguments_and_clamps_alignment(self):
        gcc, plugin = self._find_cross_gcc_and_plugin()
        if not gcc:
            self.skipTest("matching host arm-none-eabi-gcc + nexmon.so not available")
        ld = gcc.parent / "arm-none-eabi-ld"
        objdump = gcc.parent / "arm-none-eabi-objdump"

        with tempfile.TemporaryDirectory(prefix="nexmon-plugin-") as tmp:
            tmp_path = Path(tmp)

            # 1. Invalid non-constant argument must fail compilation and emit no 0x00000000 entry
            bad_c = tmp_path / "bad.c"
            bad_pre = tmp_path / "bad.pre"
            bad_o = tmp_path / "bad.o"
            bad_c.write_text("extern int dynamic_addr;\n__attribute__((at(dynamic_addr))) int bad_sym = 1;\n")
            res = subprocess.run(
                [
                    str(gcc), "-mthumb", "-mcpu=cortex-m3", "-O2",
                    f"-fplugin={plugin}",
                    f"-fplugin-arg-nexmon-objfile={bad_o}",
                    f"-fplugin-arg-nexmon-prefile={bad_pre}",
                    "-c", str(bad_c), "-o", str(bad_o),
                ],
                capture_output=True, text=True, timeout=15,
            )
            self.assertNotEqual(res.returncode, 0)
            self.assertIn("at", res.stderr)
            if bad_pre.exists():
                self.assertNotIn("0x00000000", bad_pre.read_text())

            # 2. Unaligned 16-bit, 32-bit, and 64-bit patches + REGION placement
            good_c = tmp_path / "good.c"
            good_pre = tmp_path / "good.pre"
            good_o = tmp_path / "good.o"
            good_c.write_text(textwrap.dedent("""
                #include <stdint.h>
                __attribute__((at(0x180001), section(".text.p16"))) uint16_t p16 = 0x1234;
                __attribute__((at(0x180006), section(".text.p32"))) uint32_t p32 = 0xdeadbeef;
                __attribute__((at(0x18000c), section(".text.p64"))) uint64_t p64 = 0x1122334455667788ULL;
                __attribute__((at("PATCH"), section(".text.in_region"))) uint32_t in_region = 0xcafebabe;
            """))
            subprocess.run(
                [
                    str(gcc), "-mthumb", "-mcpu=cortex-m3", "-O2", "-ffunction-sections", "-fdata-sections",
                    f"-fplugin={plugin}",
                    f"-fplugin-arg-nexmon-objfile={good_o}",
                    f"-fplugin-arg-nexmon-prefile={good_pre}",
                    "-c", str(good_c), "-o", str(good_o),
                ],
                check=True, capture_output=True, text=True, timeout=15,
            )

            pre_text = good_pre.read_text()
            self.assertIn("PATCH REGION", pre_text)

            # Generate linker script via nexmon.ld.awk and link ELF
            ld_body = subprocess.check_output(
                ["gawk", "-f", str(ROOT / "buildtools/scripts/nexmon.ld.awk"), str(good_pre)],
                text=True, timeout=5,
            )
            full_ld = tmp_path / "test.ld"
            full_ld.write_text("MEMORY { PATCH : ORIGIN = 0x181000, LENGTH = 0x1000 }\nSECTIONS {\n" + ld_body + "\n}\n")
            elf = tmp_path / "patch.elf"
            subprocess.run([str(ld), "-T", str(full_ld), "-o", str(elf)], check=True, timeout=10)

            # Verify exact unaligned VMAs in linked ELF (no alignment padding shifts!)
            hdrs = subprocess.check_output([str(objdump), "-h", str(elf)], text=True, timeout=5)
            self.assertRegex(hdrs, r"\.text\.p16\s+00000002\s+00180001")
            self.assertRegex(hdrs, r"\.text\.p32\s+00000004\s+00180006")
            self.assertRegex(hdrs, r"\.text\.p64\s+00000008\s+0018000c")
            self.assertRegex(hdrs, r"\.text\.in_region\s+00000004\s+00181000")

            # Run nexmon.mk.1.awk -> nexmon.mk.2.awk and verify REGION rule uses .text.in_region and 0x00181000!
            env = dict(os.environ, NEXMON_ROOT=str(ROOT), CC=str(gcc.parent / "arm-none-eabi-"))
            mk1_out = subprocess.check_output(
                ["gawk", "-v", f"src_file={elf}", "-f", str(ROOT / "buildtools/scripts/nexmon.mk.1.awk"), str(good_pre)],
                text=True, env=env, timeout=5,
            )
            self.assertIn("00181000", mk1_out)
            mk2_out = subprocess.check_output(
                ["gawk", "-v", "ramstart=0x180000", "-f", str(ROOT / "buildtools/scripts/nexmon.mk.2.awk")],
                input=mk1_out, text=True, env=env, timeout=5,
            )
            self.assertIn("-j .text.in_region", mk2_out)
            self.assertIn("seek=$$((0x00181000 - 0x180000))", mk2_out)


class TestC05FpextAndUcodeextHardening(unittest.TestCase):
    """C-05: Verify bounds validation and off-by-one fixes in fpext and ucodeext."""

    def test_fpext_valid_and_oob_rejection(self):
        fpext = ROOT / "buildtools/flash_patch_extractor/fpext"
        if not fpext.is_file():
            self.skipTest("fpext binary missing (build with make -C buildtools/flash_patch_extractor)")

        with tempfile.TemporaryDirectory(prefix="nexmon-fpext-") as tmp:
            tmp_path = Path(tmp)
            ram_file = tmp_path / "ram.bin"
            rom_in = tmp_path / "rom_in.bin"
            rom_out = tmp_path / "rom_out.bin"

            # Put patch data right at ram_start (0x180000) to exercise the `addr >= ram_start` off-by-one fix!
            # 4-byte Thumb-2 branch `b.w +0` = 0xf000 0xb800, followed by fp_config at offset 0x10.
            ram = bytearray(0x40)
            ram[0:4] = b"\x00\xf0\x00\xb8"
            # struct fp_config { target_addr=0x10, size=4, data_ptr=0x180000 }
            struct.pack_into("<III", ram, 0x10, 0x10, 4, 0x180000)
            ram_file.write_bytes(ram)
            rom_in.write_bytes(b"\xff" * 0x40)

            res = subprocess.run(
                [
                    str(fpext), "-r", str(ram_file), "-s", "0x180000",
                    "-b", "0x180010", "-e", "0x18001c",
                    "-i", str(rom_in), "-o", str(rom_out), "-t", "0x0",
                ],
                capture_output=True, text=True, timeout=10,
            )
            self.assertEqual(res.returncode, 0, res.stderr)
            self.assertIn('__attribute__((at(0x00000010, "flashpatch")))', res.stdout)
            patched = rom_out.read_bytes()
            self.assertEqual(patched[0x10:0x14], b"\x00\xf0\x00\xb8")

            # OOB: fp_config_end < fp_config_base (previously caused unsigned underflow infinite loop / crash!)
            bad1 = subprocess.run(
                [str(fpext), "-r", str(ram_file), "-s", "0x180000", "-b", "0x180020", "-e", "0x180010"],
                capture_output=True, text=True, timeout=5,
            )
            self.assertNotEqual(bad1.returncode, 0)

            # OOB: fp_config_end past end of RAM
            bad2 = subprocess.run(
                [str(fpext), "-r", str(ram_file), "-s", "0x180000", "-b", "0x180010", "-e", "0x190000"],
                capture_output=True, text=True, timeout=5,
            )
            self.assertNotEqual(bad2.returncode, 0)

            # OOB: corrupt data_ptr outside RAM
            struct.pack_into("<III", ram, 0x10, 0x10, 4, 0x199999)
            ram_file.write_bytes(ram)
            bad3 = subprocess.run(
                [str(fpext), "-r", str(ram_file), "-s", "0x180000", "-b", "0x180010", "-e", "0x18001c"],
                capture_output=True, text=True, timeout=5,
            )
            self.assertNotEqual(bad3.returncode, 0)

    def test_ucodeext_valid_trailing_and_oob_rejection(self):
        ucodeext = ROOT / "buildtools/ucode_extractor/ucodeext"
        if not ucodeext.is_file():
            self.skipTest("ucodeext binary missing (build with make -C buildtools/ucode_extractor)")

        with tempfile.TemporaryDirectory(prefix="nexmon-ucodeext-") as tmp:
            tmp_path = Path(tmp)
            ram_file = tmp_path / "ram.bin"
            out_file = tmp_path / "ucode.bin"

            # 7-byte ucode instruction with high bit >= 0x80 at p[3] (previously signed-shift UB!)
            # plus 3 trailing bytes to test non-multiple-of-7 safety
            ram_file.write_bytes(b"\xaa\xbb\xcc\x81\x23\x45\x67" + b"\xde\xad\xbe")
            res = subprocess.run(
                [str(ucodeext), "-r", str(ram_file), "-b", "0", "-l", "7", "-o", str(out_file)],
                capture_output=True, text=True, timeout=5,
            )
            self.assertEqual(res.returncode, 0, res.stderr)
            a, b = struct.unpack("<II", out_file.read_bytes())
            self.assertEqual(a, 0x81234567)
            self.assertEqual(b, 0x00aabbcc)

            # Length = 10 (not a multiple of 7): must only emit 1 full instruction (8 bytes) without OOB read!
            res_rem = subprocess.run(
                [str(ucodeext), "-r", str(ram_file), "-b", "0", "-l", "10", "-o", str(out_file)],
                capture_output=True, text=True, timeout=5,
            )
            self.assertEqual(res_rem.returncode, 0, res_rem.stderr)
            self.assertEqual(len(out_file.read_bytes()), 8)

            # Missing -o must fail (previously exited EXIT_SUCCESS!)
            bad_no_out = subprocess.run(
                [str(ucodeext), "-r", str(ram_file), "-b", "0", "-l", "7"],
                capture_output=True, text=True, timeout=5,
            )
            self.assertNotEqual(bad_no_out.returncode, 0)

            # OOB length past end of RAM must fail
            bad_oob = subprocess.run(
                [str(ucodeext), "-r", str(ram_file), "-b", "4", "-l", "14", "-o", str(out_file)],
                capture_output=True, text=True, timeout=5,
            )
            self.assertNotEqual(bad_oob.returncode, 0)


class TestC07AndroidJniHardening(unittest.TestCase):
    """C-07: Verify 64-bit JNI handle types, 512-byte buffer fix, and wrapper_pentestsuite safety."""

    def test_wireshark_helper_and_packet_java_64bit_handles_and_bounds(self):
        packet_java = (ROOT / "app/app/src/main/java/de/tu_darmstadt/seemoo/nexmon/sharky/Packet.java").read_text()
        ws_c = (ROOT / "app/app/src/main/external/src/libwireshark_helper/wireshark_helper.c").read_text()
        wrap_c = (ROOT / "app/app/src/main/internal/jni/wrapper_pentestsuite.c").read_text()

        # Packet.java uses 64-bit long handles everywhere
        self.assertIn("public long _dissection_ptr;", packet_java)
        self.assertIn("in.readLong()", packet_java)
        self.assertIn("out.writeLong(-1L)", packet_java)
        self.assertIn("public native static synchronized long dissectPacket(", packet_java)
        self.assertIn("public native static synchronized void dissectCleanup(long dissect_ptr);", packet_java)
        self.assertIn("public native static synchronized String wiresharkGet(long dissect_ptr, String param);", packet_java)
        self.assertIn("public native static synchronized String[] wiresharkGetAll(long dissect_ptr);", packet_java)

        # wireshark_helper.c uses intptr_t / jlong and allocates 512 bytes inside dfilter_string != NULL
        self.assertIn("void dissectCleanup(intptr_t ptr);", ws_c)
        self.assertIn("intptr_t dissectPacket(char *pHeader, char *pData, int encap);", ws_c)
        self.assertIn("item->str = malloc(512);", ws_c)
        self.assertNotIn("item->str = malloc(256);", ws_c)
        self.assertIn("(chop_len <= dlen)", ws_c)

        # wrapper_pentestsuite.c returns jstring via NewStringUTF, heap-allocates frameBytes, and zero-inits my_message
        self.assertNotIn('return "nothing";', wrap_c)
        self.assertIn('NewStringUTF(pEnv, "nothing")', wrap_c)
        self.assertNotIn("jbyte frameBytes[len];", wrap_c)
        self.assertIn("memset(&my_message, 0, sizeof(my_message));", wrap_c)


if __name__ == "__main__":
    unittest.main(verbosity=2)
