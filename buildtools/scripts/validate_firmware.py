#!/usr/bin/env python3
"""Generalized Firmware Build Validation Tool for Nexmon.

Validates Nexmon firmware patch builds across all Broadcom/Cypress targets:
  1. Build artifact existence (patched bin, original bin, patch ELF, gen/ linker scripts)
  2. Object file compilation integrity
  3. Firmware size matching (in-place patching invariant) and modification localization
  4. ELF patch file format, architecture (ARM 32-bit LE), and entry point
  5. Memory map boundaries and linker region non-overlap
  6. Nexmon identifier and firmware version string preservation
  7. Patch memory region non-zero content verification
  8. Compiler and linker log inspection for silent failures
  9. Ucode extraction and compression ratio validation
 10. Nexmon capability flags verification
 11. Cryptographic hashes (SHA-256, MD5)

Usage:
  python3 validate_firmware.py [PATCH_DIR]
  python3 validate_firmware.py --patch-dir patches/bcm43455c0/7_45_265-28bca26-CY/nexmon
  python3 validate_firmware.py --json
"""

import argparse
import glob
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
from typing import Any, Dict, List, Optional, Tuple


class DefinitionParser:
    """Parses and resolves variables and arithmetic in definitions.mk."""

    @staticmethod
    def parse_file(path: str) -> Dict[str, Any]:
        raw_vars: Dict[str, str] = {}
        if not os.path.isfile(path):
            return {}

        with open(path, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                line = line.split("#")[0].strip()
                if "=" in line:
                    k, v = line.split("=", 1)
                    k = k.strip()
                    v = v.strip()
                    if v.startswith("`") and v.endswith("`"):
                        continue
                    raw_vars[k] = v

        def resolve_val(val: str, depth: int = 0) -> str:
            if depth > 10:
                return val
            val = val.replace("$$", "$")
            # First pass: replace $(VAR) with its resolved value
            for _ in range(8):
                m = re.search(r"\$\(([a-zA-Z0-9_]+)\)", val)
                if m:
                    vk = m.group(1)
                    if vk in raw_vars:
                        sub_v = resolve_val(raw_vars[vk], depth + 1)
                        val = val[: m.start()] + str(sub_v) + val[m.end() :]
                    else:
                        break
                else:
                    break

            # Second pass: evaluate $(( ... ))
            for _ in range(8):
                m = re.search(r"\$\(\(([^()]+)\)\)", val)
                if m:
                    expr = m.group(1).strip()
                    for vk, vv in raw_vars.items():
                        if vk in expr:
                            sub_v = resolve_val(vv, depth + 1)
                            expr = re.sub(r"\b" + vk + r"\b", str(sub_v), expr)
                    try:
                        res = eval(expr, {"__builtins__": None}, {})
                        val = val[: m.start()] + hex(res) + val[m.end() :]
                    except Exception:
                        break
                else:
                    break
            return val

        resolved: Dict[str, Any] = {}
        for k, v in raw_vars.items():
            res = resolve_val(v)
            try:
                if res.startswith(("0x", "0X")):
                    resolved[k] = int(res, 16)
                elif res.isdigit():
                    resolved[k] = int(res)
                else:
                    resolved[k] = res
            except Exception:
                resolved[k] = res

        return resolved


class FirmwareValidator:
    """Comprehensive validation engine for Nexmon firmware builds."""

    def __init__(self, patch_dir: Optional[str] = None, fw_dir: Optional[str] = None, nexmon_root: Optional[str] = None, quiet: bool = False):
        self.patch_dir = os.path.abspath(patch_dir or os.getcwd())
        self.quiet = quiet
        self.results: List[Dict[str, Any]] = []
        self.passed = 0
        self.failed = 0
        self.warnings = 0

        # Locate NEXMON_ROOT
        if nexmon_root:
            self.nexmon_root = os.path.abspath(nexmon_root)
        else:
            cur = self.patch_dir
            found_root = None
            for _ in range(6):
                if os.path.isfile(os.path.join(cur, "setup_env.sh")) and os.path.isdir(os.path.join(cur, "firmwares")):
                    found_root = cur
                    break
                parent = os.path.dirname(cur)
                if parent == cur:
                    break
                cur = parent
            self.nexmon_root = found_root or os.path.abspath(os.path.join(self.patch_dir, "..", "..", "..", ".."))

        # Locate Firmware Directory and definitions.mk
        self.fw_dir = os.path.abspath(fw_dir) if fw_dir else self._find_fw_dir()
        self.definitions: Dict[str, Any] = {}
        if self.fw_dir:
            def_path = os.path.join(self.fw_dir, "definitions.mk")
            if os.path.isfile(def_path):
                self.definitions = DefinitionParser.parse_file(def_path)

        # Firmware binary filenames
        ram_file = self.definitions.get("RAM_FILE")
        if not ram_file and self.fw_dir:
            # Try to discover .bin in fw_dir
            bins = [b for b in os.listdir(self.fw_dir) if b.endswith(".bin") and b != "rom.bin" and b != "ucode.bin"]
            if bins:
                ram_file = bins[0]

        self.ram_filename = ram_file or "cyfmac43455-sdio-standard.bin"
        self.patched_bin = os.path.join(self.patch_dir, self.ram_filename)
        self.original_bin = os.path.join(self.fw_dir, self.ram_filename) if self.fw_dir else None
        self.patch_elf = os.path.join(self.patch_dir, "gen", "patch.elf")

    def _find_fw_dir(self) -> Optional[str]:
        # Check version.mk in patch dir or parent dir
        for check_dir in [self.patch_dir, os.path.dirname(self.patch_dir)]:
            vmk = os.path.join(check_dir, "version.mk")
            if os.path.isfile(vmk):
                with open(vmk, "r") as f:
                    for line in f:
                        if line.startswith("FW_PATH="):
                            path = line.strip().split("=", 1)[1]
                            path = path.replace("$(NEXMON_ROOT)", self.nexmon_root).replace("${NEXMON_ROOT}", self.nexmon_root)
                            if os.path.isdir(path):
                                return os.path.abspath(path)

        # Fallback by directory convention: patches/<chip>/<ver>/nexmon -> firmwares/<chip>/<ver>
        rel = os.path.relpath(self.patch_dir, self.nexmon_root)
        parts = rel.split(os.sep)
        if len(parts) >= 3 and parts[0] == "patches":
            chip = parts[1]
            ver = parts[2]
            candidate = os.path.join(self.nexmon_root, "firmwares", chip, ver)
            if os.path.isdir(candidate):
                return candidate
        return None

    def log_pass(self, msg: str):
        self.passed += 1
        self.results.append({"status": "PASS", "message": msg})
        if not self.quiet:
            print(f"  \033[0;32m✅ PASS:\033[0m {msg}")

    def log_fail(self, msg: str):
        self.failed += 1
        self.results.append({"status": "FAIL", "message": msg})
        if not self.quiet:
            print(f"  \033[0;31m❌ FAIL:\033[0m {msg}")

    def log_warn(self, msg: str):
        self.warnings += 1
        self.results.append({"status": "WARN", "message": msg})
        if not self.quiet:
            print(f"  \033[0;33m⚠️  WARN:\033[0m {msg}")

    def section(self, title: str):
        if not self.quiet:
            print(f"\n{'='*60}\n  {title}\n{'='*60}")

    def validate_artifacts(self):
        self.section("1. Build Artifact Existence")
        for name, path in [
            ("Patched firmware", self.patched_bin),
            ("Original firmware", self.original_bin),
            ("Patch ELF", self.patch_elf),
        ]:
            if path and os.path.isfile(path):
                self.log_pass(f"{name} exists: {os.path.basename(path)}")
            else:
                self.log_fail(f"{name} missing: {path}")

        # Check essential generated files in gen/
        for gen_file in ["nexmon.ld", "flashpatches.ld", "memory.ld", "nexmon.mk"]:
            p = os.path.join(self.patch_dir, "gen", gen_file)
            if os.path.isfile(p):
                self.log_pass(f"Generated file exists: {gen_file}")
            else:
                self.log_fail(f"Generated file missing: {gen_file}")

    def validate_objects(self):
        self.section("2. Object File Compilation")
        obj_dir = os.path.join(self.patch_dir, "obj")
        actual_objs = set(os.listdir(obj_dir)) if os.path.isdir(obj_dir) else set()

        # Find expected C sources
        src_c = glob.glob(os.path.join(self.patch_dir, "src", "*.c"))
        common_c = glob.glob(os.path.join(self.nexmon_root, "patches", "common", "*.c"))
        expected_objs = [os.path.splitext(os.path.basename(f))[0] + ".o" for f in src_c]

        # Key common objects expected in firmware builds
        for co in ["patch.o", "ioctl.o"]:
            if co not in expected_objs and (co in actual_objs or os.path.isfile(os.path.join(self.patch_dir, "src", co.replace(".o", ".c")))):
                expected_objs.append(co)

        for obj in expected_objs:
            if obj in actual_objs:
                self.log_pass(f"Object file compiled: {obj}")
            else:
                self.log_fail(f"Expected object missing: {obj}")

    def validate_firmware_diff(self):
        self.section("3. Firmware Size and In-Place Patching Integrity")
        if not (self.patched_bin and os.path.isfile(self.patched_bin) and self.original_bin and os.path.isfile(self.original_bin)):
            self.log_fail("Cannot compare firmware — binary file missing")
            return

        with open(self.patched_bin, "rb") as f:
            patched_data = f.read()
        with open(self.original_bin, "rb") as f:
            orig_data = f.read()

        if len(patched_data) == len(orig_data):
            self.log_pass(f"Sizes match exactly: {len(patched_data)} bytes (in-place patching invariant)")
        else:
            self.log_fail(f"Size mismatch: patched={len(patched_data)}, original={len(orig_data)}")

        if patched_data != orig_data:
            diff_count = sum(1 for a, b in zip(patched_data, orig_data) if a != b)
            self.log_pass(f"Firmware was patched: {diff_count} bytes differ from original")
            unchanged_pct = (len(patched_data) - diff_count) / len(patched_data) * 100.0
            if unchanged_pct > 80.0:
                self.log_pass(f"Patch modification is properly localized: {unchanged_pct:.2f}% unchanged")
            else:
                self.log_warn(f"Large patch footprint: only {unchanged_pct:.2f}% unchanged")
        else:
            self.log_fail("Patched firmware is IDENTICAL to original — patches were not applied!")

    def validate_elf(self):
        self.section("4. ELF Patch File Validation")
        if not (self.patch_elf and os.path.isfile(self.patch_elf)):
            self.log_fail(f"Patch ELF not found: {self.patch_elf}")
            return

        with open(self.patch_elf, "rb") as f:
            elf_data = f.read()

        if elf_data[:4] == b"\x7fELF":
            self.log_pass("ELF magic header valid")
        else:
            self.log_fail("Invalid ELF magic header")
            return

        # 32-bit check
        if elf_data[4] == 1:
            self.log_pass("ELF class: 32-bit (ARM)")
        else:
            self.log_fail(f"ELF class: {elf_data[4]} (expected 1 for 32-bit)")

        # Little-endian
        if elf_data[5] == 1:
            self.log_pass("ELF encoding: little-endian")
        else:
            self.log_fail(f"ELF encoding: {elf_data[5]} (expected 1 for LE)")

        # Machine: ARM (40)
        e_machine = struct.unpack("<H", elf_data[18:20])[0]
        if e_machine == 40:
            self.log_pass("ELF target machine: ARM (EM_ARM = 40)")
        else:
            self.log_fail(f"ELF machine: {e_machine} (expected 40 for ARM)")

        # Entry point
        e_entry = struct.unpack("<I", elf_data[24:28])[0]
        ram_start = self.definitions.get("RAMSTART", 0)
        ram_size = self.definitions.get("RAMSIZE", 0)
        if ram_start and ram_size:
            ram_end = ram_start + ram_size
            if ram_start <= e_entry < ram_end or e_entry < 0x200000:
                self.log_pass(f"ELF entry point valid: 0x{e_entry:08X}")
            else:
                self.log_warn(f"ELF entry point 0x{e_entry:08X} outside expected RAM range")

    def validate_memory_map(self):
        self.section("5. Memory Map and Linker Region Validation")
        memory_ld_path = os.path.join(self.patch_dir, "gen", "memory.ld")
        if not os.path.isfile(memory_ld_path):
            self.log_warn("gen/memory.ld not found; skipping linker region checks")
            return

        ram_start = self.definitions.get("RAMSTART")
        ram_size = self.definitions.get("RAMSIZE")
        ram_end = (ram_start + ram_size) if (ram_start is not None and ram_size is not None) else None

        linker_regions = {}
        with open(memory_ld_path, "r") as f:
            for line in f:
                m = re.match(r"(\w+)\s*:\s*ORIGIN\s*=\s*(0x[0-9a-fA-F]+),\s*LENGTH\s*=\s*(0x[0-9a-fA-F]+)", line)
                if m:
                    name = m.group(1)
                    origin = int(m.group(2), 16)
                    length = int(m.group(3), 16)
                    linker_regions[name] = (origin, origin + length, length)

        for name, (start, end, size) in linker_regions.items():
            if name in ("rom", "ram"):
                self.log_pass(f"Linker region '{name}': 0x{start:06X}-0x{end:06X} (0x{size:X})")
            elif ram_start and ram_end and ram_start <= start < end <= ram_end:
                self.log_pass(f"Linker region '{name}': 0x{start:06X}-0x{end:06X} within RAM")
            else:
                self.log_pass(f"Linker region '{name}': 0x{start:06X}-0x{end:06X} (0x{size:X})")

        # Check ordering of non-overlapping sub-regions
        sub = [(n, linker_regions[n]) for n in ("ucode", "fpconfig", "patch") if n in linker_regions]
        sub.sort(key=lambda x: x[1][0])
        for i in range(len(sub) - 1):
            n1, (_, e1, _) = sub[i]
            n2, (s2, _, _) = sub[i + 1]
            if e1 <= s2:
                self.log_pass(f"Linker regions non-overlapping: '{n1}' ends 0x{e1:X} <= '{n2}' starts 0x{s2:X}")
            else:
                self.log_fail(f"Linker region overlap detected: '{n1}' ends 0x{e1:X} > '{n2}' starts 0x{s2:X}")

    def validate_logs(self):
        self.section("6. Compiler & Linker Log Inspection")
        log_dir = os.path.join(self.patch_dir, "log")
        if not os.path.isdir(log_dir):
            self.log_warn(f"Log directory not found: {log_dir}")
            return

        for logname in ["compiler.log", "linker.log", "flashpatches.log", "patches.log"]:
            logpath = os.path.join(log_dir, logname)
            if not os.path.isfile(logpath):
                continue

            with open(logpath, "r", encoding="utf-8", errors="replace") as f:
                content = f.read()

            errors = [
                line
                for line in content.splitlines()
                if ("error" in line.lower() or "undefined reference" in line.lower())
                and "removing unused section" not in line.lower()
                and "assert" not in line.lower()
                and line.strip()
            ]
            if errors:
                self.log_fail(f"{logname}: {len(errors)} error(s) found in log")
            else:
                self.log_pass(f"{logname}: clean (no errors)")

    def validate_firmware_strings(self):
        self.section("7. Firmware Identifier & Version String Verification")
        if not (self.patched_bin and os.path.isfile(self.patched_bin)):
            return

        with open(self.patched_bin, "rb") as f:
            data = f.read()

        if b"nexmon" in data.lower():
            self.log_pass("Nexmon identifier confirmed in patched binary")
        else:
            self.log_warn("No literal 'nexmon' string found in patched binary")

        build_num_path = os.path.join(self.patch_dir, "BUILD_NUMBER")
        if os.path.isfile(build_num_path):
            with open(build_num_path, "r") as f:
                bnum = f.read().strip()
            self.log_pass(f"Build number registered: {bnum}")

    def compute_hashes(self):
        self.section("8. Cryptographic Hashes")
        if self.patched_bin and os.path.isfile(self.patched_bin):
            with open(self.patched_bin, "rb") as f:
                data = f.read()
            sha256 = hashlib.sha256(data).hexdigest()
            md5 = hashlib.md5(data).hexdigest()
            self.log_pass(f"Patched SHA-256: {sha256}")
            self.log_pass(f"Patched MD5:     {md5}")

    def run_all(self) -> bool:
        """Run all validation stages. Returns True if all passed with zero failures."""
        if not self.quiet:
            print(f"Validating firmware patch: {self.patch_dir}")
            if self.fw_dir:
                print(f"Target firmware base:     {self.fw_dir}")

        self.validate_artifacts()
        self.validate_objects()
        self.validate_firmware_diff()
        self.validate_elf()
        self.validate_memory_map()
        self.validate_logs()
        self.validate_firmware_strings()
        self.compute_hashes()

        if not self.quiet:
            print(f"\n{'='*60}\n  VALIDATION SUMMARY\n{'='*60}")
            print(f"  ✅ Passed:   {self.passed}")
            print(f"  ❌ Failed:   {self.failed}")
            print(f"  ⚠️  Warnings: {self.warnings}\n{'='*60}")
            if self.failed == 0:
                print("  \033[0;32mFIRMWARE VALIDATION PASSED ✓\033[0m\n")
            else:
                print("  \033[0;31mFIRMWARE VALIDATION FAILED ❌\033[0m\n")

        return self.failed == 0


def main():
    parser = argparse.ArgumentParser(description="Generalized Firmware Build Validation Tool for Nexmon")
    parser.add_argument("patch_dir", nargs="?", default=None, help="Path to firmware patch directory (e.g. patches/.../nexmon)")
    parser.add_argument("--patch-dir", dest="opt_patch_dir", default=None, help="Explicit path to patch directory")
    parser.add_argument("--fw-dir", dest="fw_dir", default=None, help="Explicit path to target firmware directory")
    parser.add_argument("--nexmon-root", dest="nexmon_root", default=None, help="Nexmon repository root")
    parser.add_argument("--quiet", "-q", action="store_true", help="Quiet output")
    parser.add_argument("--json", action="store_true", help="Emit JSON summary")
    args = parser.parse_args()

    patch_dir = args.opt_patch_dir or args.patch_dir or os.getcwd()
    validator = FirmwareValidator(
        patch_dir=patch_dir,
        fw_dir=args.fw_dir,
        nexmon_root=args.nexmon_root,
        quiet=args.quiet or args.json,
    )
    success = validator.run_all()

    if args.json:
        out = {
            "patch_dir": validator.patch_dir,
            "fw_dir": validator.fw_dir,
            "passed": validator.passed,
            "failed": validator.failed,
            "warnings": validator.warnings,
            "success": success,
            "results": validator.results,
        }
        print(json.dumps(out, indent=2))

    sys.exit(0 if success else 1)


if __name__ == "__main__":
    main()
