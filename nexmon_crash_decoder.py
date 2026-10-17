#!/usr/bin/env python3
"""
Nexmon Firmware Crash Telemetry Extractor & Decoder
===================================================

An automated utility to extract, parse, decode, and diagnose Broadcom WiFi
firmware crash dumps and trap frames from kernel dmesg, console traces, or log files.

Key Features:
- Parses `brcmfmac` SDIO trap frames (`brcmf_sdio_trap_info`), DHD dumps,
  assertion failures, and firmware console crash logs.
- Automatically resolves Program Counters (`EPC`, `PC`), Link Registers (`LR`),
  Stack Pointers (`SP`), and general-purpose registers to function and symbol names.
- Decodes memory regions: stock ROM, RAM code, Nexmon patch area, microcode,
  flashpatch config tables, and stack/heap.
- Unpacks ARM CPSR/SPSR status registers (Thumb mode, processor modes, condition codes).
- Synthesizes intelligent root-cause diagnostics (NULL pointer dereference, misaligned
  access, invalid function pointer call, stack smashing, MPU violation).
- Heuristic stack trace / backtrace reconstruction from memory dump words.
- Auto-detects target chip and firmware version from kernel logs or allows manual selection.
- Outputs human-readable terminal reports or machine-parseable JSON (`-j` / `--json`).
- Built-in self-test mode (`--test`) for automated regression verification.

Copyright (c) 2026 Nexmon Project
Licensed under Apache 2.0 / GPLv2.
"""

import argparse
import glob
import json
import os
import re
import struct
import subprocess
import sys
from dataclasses import dataclass, field
from typing import Dict, List, Optional, Tuple, Any


# -----------------------------------------------------------------------------
# ANSI Color Palette
# -----------------------------------------------------------------------------

class Colors:
    HEADER = "\033[95m"
    BLUE = "\033[94m"
    CYAN = "\033[96m"
    GREEN = "\033[92m"
    YELLOW = "\033[93m"
    RED = "\033[91m"
    BOLD = "\033[1m"
    DIM = "\033[2m"
    RESET = "\033[0m"

    @classmethod
    def disable(cls):
        cls.HEADER = ""
        cls.BLUE = ""
        cls.CYAN = ""
        cls.GREEN = ""
        cls.YELLOW = ""
        cls.RED = ""
        cls.BOLD = ""
        cls.DIM = ""
        cls.RESET = ""


# -----------------------------------------------------------------------------
# Data Models: Memory Regions & Symbols
# -----------------------------------------------------------------------------

@dataclass
class MemoryRegion:
    name: str
    start: int
    end: int
    desc: str

    def contains(self, addr: int) -> bool:
        return self.start <= addr < self.end


@dataclass
class Symbol:
    name: str
    address: int
    size: int = 0
    source: str = "unknown"  # wrapper.c, elf, flashpatches, src_annotation


@dataclass
class ResolvedAddress:
    raw_addr: int
    clean_addr: int
    is_thumb: bool
    region: Optional[MemoryRegion]
    symbol: Optional[Symbol]
    offset: int = 0

    def format_inline(self, color: bool = True) -> str:
        b = Colors.BOLD if color else ""
        c = Colors.CYAN if color else ""
        r = Colors.RESET if color else ""
        y = Colors.YELLOW if color else ""
        dim = Colors.DIM if color else ""

        region_str = f"[{self.region.name}]" if self.region else "[UNKNOWN]"
        thumb_str = f" (Thumb)" if self.is_thumb else ""

        if self.symbol:
            if self.offset == 0:
                sym_str = f"{c}{self.symbol.name}{r}{thumb_str}"
            else:
                sym_str = f"{c}{self.symbol.name}{r}+{y}0x{self.offset:x}{r}{thumb_str}"
            return f"0x{self.raw_addr:08x} <{sym_str}> {dim}{region_str}{r}"
        else:
            return f"0x{self.raw_addr:08x} <unknown> {dim}{region_str}{r}"


# -----------------------------------------------------------------------------
# Pure Python ELF Parser (32-bit & 64-bit LE)
# -----------------------------------------------------------------------------

def parse_elf_symbols(elf_path: str) -> List[Symbol]:
    """Parse symbols from ELF binary without requiring external tools."""
    if not os.path.isfile(elf_path):
        return []

    symbols: List[Symbol] = []
    try:
        with open(elf_path, "rb") as f:
            data = f.read()

        if len(data) < 52 or data[:4] != b"\x7fELF":
            return []

        ei_class = data[4]  # 1 = 32-bit, 2 = 64-bit
        ei_data = data[5]   # 1 = LE, 2 = BE
        if ei_data != 1:    # We only deal with little-endian Broadcom ELF
            return []

        if ei_class == 1:
            e_shoff = struct.unpack_from("<I", data, 0x20)[0]
            e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", data, 0x2E)
            sections = [
                struct.unpack_from("<10I", data, e_shoff + i * e_shentsize)
                for i in range(e_shnum)
            ]
            for sec in sections:
                sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link = sec[:7]
                if sh_type in (2, 11):  # SHT_SYMTAB or SHT_DYNSYM
                    if sh_link < len(sections):
                        strtab_sec = sections[sh_link]
                        strtab_off = strtab_sec[4]
                        strtab_len = strtab_sec[5]
                        strtab = data[strtab_off : strtab_off + strtab_len]
                        sym_count = sh_size // 16
                        for s in range(sym_count):
                            st_name, st_value, st_size, st_info = struct.unpack_from("<IIIB", data, sh_offset + s * 16)
                            if st_name < len(strtab) and st_value != 0:
                                end = strtab.find(b"\x00", st_name)
                                sym_name = strtab[st_name:end].decode("utf-8", errors="replace")
                                if sym_name and not sym_name.startswith("$"):
                                    # Clear Thumb bit on symbol addresses if present
                                    symbols.append(Symbol(name=sym_name, address=st_value & ~1, size=st_size, source="patch.elf"))

        elif ei_class == 2:
            e_shoff = struct.unpack_from("<Q", data, 0x28)[0]
            e_shentsize, e_shnum, e_shstrndx = struct.unpack_from("<HHH", data, 0x3A)
            sections = [
                struct.unpack_from("<2I4Q2I2Q", data, e_shoff + i * e_shentsize)
                for i in range(e_shnum)
            ]
            for sec in sections:
                sh_name, sh_type, sh_flags, sh_addr, sh_offset, sh_size, sh_link = sec[:7]
                if sh_type in (2, 11):
                    if sh_link < len(sections):
                        strtab_sec = sections[sh_link]
                        strtab_off = strtab_sec[4]
                        strtab_len = strtab_sec[5]
                        strtab = data[strtab_off : strtab_off + strtab_len]
                        sym_count = sh_size // 24
                        for s in range(sym_count):
                            st_name, st_info, st_other, st_shndx, st_value, st_size = struct.unpack_from("<IBBHQQ", data, sh_offset + s * 24)
                            if st_name < len(strtab) and st_value != 0:
                                end = strtab.find(b"\x00", st_name)
                                sym_name = strtab[st_name:end].decode("utf-8", errors="replace")
                                if sym_name and not sym_name.startswith("$"):
                                    symbols.append(Symbol(name=sym_name, address=st_value & ~1, size=st_size, source="elf"))
    except Exception:
        pass

    return symbols


# -----------------------------------------------------------------------------
# Target Database & Symbol Resolution
# -----------------------------------------------------------------------------

class ChipTarget:
    """Represents a specific WiFi chip and firmware target with its memory map & symbols."""

    def __init__(self, chip: str, fw: str, repo_root: str):
        self.chip = chip
        self.fw = fw
        self.repo_root = repo_root
        self.regions: List[MemoryRegion] = []
        self.symbols: List[Symbol] = []
        self._symbols_sorted: List[Symbol] = []
        self.definitions: Dict[str, Any] = {}
        self._load_memory_map()
        self._load_symbols()

    def _load_memory_map(self):
        """Parse definitions.mk to establish memory regions."""
        mk_path = os.path.join(self.repo_root, "firmwares", self.chip, self.fw, "definitions.mk")
        if not os.path.isfile(mk_path):
            # Fallback to search in patches
            pattern = os.path.join(self.repo_root, "patches", self.chip, self.fw, "definitions.mk")
            matches = glob.glob(pattern)
            if matches:
                mk_path = matches[0]

        if os.path.isfile(mk_path):
            with open(mk_path, "r", encoding="utf-8", errors="replace") as f:
                for line in f:
                    line = line.strip()
                    if "=" in line and not line.startswith("#"):
                        key, val = [x.strip() for x in line.split("=", 1)]
                        # Try parsing hex values
                        val_hex = re.findall(r"0x[0-9a-fA-F]+", val)
                        if val_hex:
                            try:
                                self.definitions[key] = int(val_hex[0], 0)
                            except ValueError:
                                self.definitions[key] = val
                        else:
                            self.definitions[key] = val

        # Configure regions based on parsed definitions or default ARM layouts
        rom_start = self.definitions.get("ROMSTART", 0x0)
        rom_size = self.definitions.get("ROMSIZE", 0xB0000)
        self.regions.append(MemoryRegion("ROM", rom_start, rom_start + rom_size, "Stock Broadcom ROM Microcode & Base Functions"))

        ram_start = self.definitions.get("RAMSTART", 0x198000)
        ram_size = self.definitions.get("RAMSIZE", 0xC8000)
        self.regions.append(MemoryRegion("RAM", ram_start, ram_start + ram_size, "Firmware SRAM / Dynamic Memory"))

        patch_start = self.definitions.get("PATCHSTART")
        patch_size = self.definitions.get("PATCHSIZE", 0x4000)
        if patch_start and isinstance(patch_start, int):
            self.regions.append(MemoryRegion("PATCH", patch_start, patch_start + patch_size, "Custom Nexmon Patch Code"))

        ucode_start = self.definitions.get("UCODESTART")
        ucode_size = self.definitions.get("UCODESIZE")
        if ucode_start and ucode_size and isinstance(ucode_start, int) and isinstance(ucode_size, int):
            self.regions.append(MemoryRegion("UCODE", ucode_start, ucode_start + ucode_size, "D11 MAC Microcode Engine"))

        # Peripheral & MMIO Regions
        self.regions.append(MemoryRegion("PERIPHERALS", 0x18000000, 0x18100000, "Broadcom Core MMIO / ChipCommon / D11 Registers"))

    def _load_symbols(self):
        """Aggregate symbols from wrapper.c, patch.elf, flashpatches.c, and source annotations."""
        symbols_map: Dict[int, Symbol] = {}

        # 1. Parse wrapper.c
        wrapper_path = os.path.join(self.repo_root, "patches", "common", "wrapper.c")
        if os.path.isfile(wrapper_path):
            self._parse_wrapper(wrapper_path, symbols_map)

        # 2. Parse flashpatches.c
        fp_path = os.path.join(self.repo_root, "firmwares", self.chip, self.fw, "flashpatches.c")
        if os.path.isfile(fp_path):
            self._parse_flashpatches(fp_path, symbols_map)

        # 3. Parse source files for @at and BPatch
        src_dir = os.path.join(self.repo_root, "patches", self.chip, self.fw, "nexmon", "src")
        if os.path.isdir(src_dir):
            self._parse_src_annotations(src_dir, symbols_map)

        # 4. Parse patch.elf if built
        elf_path = os.path.join(self.repo_root, "patches", self.chip, self.fw, "nexmon", "gen", "patch.elf")
        if os.path.isfile(elf_path):
            for s in parse_elf_symbols(elf_path):
                symbols_map[s.address] = s

        # 5. Add key definition symbols
        if "VERSION_PTR" in self.definitions and isinstance(self.definitions["VERSION_PTR"], int):
            symbols_map[self.definitions["VERSION_PTR"]] = Symbol("nexmon_version_str", self.definitions["VERSION_PTR"], 0, "definitions.mk")
        if "WLC_UCODE_WRITE_BL_HOOK_ADDR" in self.definitions and isinstance(self.definitions["WLC_UCODE_WRITE_BL_HOOK_ADDR"], int):
            symbols_map[self.definitions["WLC_UCODE_WRITE_BL_HOOK_ADDR"]] = Symbol("wlc_ucode_write_bl_hook", self.definitions["WLC_UCODE_WRITE_BL_HOOK_ADDR"], 0, "definitions.mk")

        self.symbols = list(symbols_map.values())
        self._symbols_sorted = sorted(self.symbols, key=lambda s: s.address)

    def _parse_wrapper(self, path: str, symbols_map: Dict[int, Symbol]):
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            text = f.read()

        at_regex = re.compile(r"AT\s*\(\s*([A-Za-z0-9_]+)\s*,\s*([A-Za-z0-9_]+)\s*,\s*(0x[0-9a-fA-F]+|[0-9]+)\s*\)")
        fn_regex = re.compile(r"([A-Za-z0-9_]+)\s*\(")

        chip_norm = self.chip.lower().replace("-", "").replace("_", "")
        # Aliases
        is_43430 = "43430" in chip_norm or "43438" in chip_norm
        is_43455 = "43455" in chip_norm

        current_ats: List[Tuple[str, str, int]] = []
        for line in text.splitlines():
            line_str = line.strip()
            m = at_regex.search(line_str)
            if m:
                current_ats.append((m.group(1), m.group(2), int(m.group(3), 0)))
            elif current_ats and not line_str.startswith("//") and not line_str.startswith("/*"):
                fn_m = fn_regex.search(line_str)
                if fn_m:
                    fname = fn_m.group(1)
                    if fname not in ("void", "int", "char", "unsigned", "uint", "bool", "struct", "RETURN_DUMMY", "VOID_DUMMY"):
                        for c_macro, fw_macro, addr in current_ats:
                            c_macro_norm = c_macro.lower()
                            # Match chip
                            match_chip = (
                                (chip_norm in c_macro_norm) or
                                (is_43430 and ("43430" in c_macro_norm or "43438" in c_macro_norm)) or
                                (is_43455 and "43455" in c_macro_norm) or
                                (c_macro == "CHIP_VER_ALL")
                            )
                            # Match FW (check if FW string appears or FW_VER_ALL)
                            fw_norm = self.fw.lower().replace("-", "_").replace(".", "_")
                            match_fw = (fw_macro == "FW_VER_ALL") or (fw_norm in fw_macro.lower())

                            if match_chip and match_fw:
                                clean_addr = addr & ~1
                                if clean_addr not in symbols_map:
                                    symbols_map[clean_addr] = Symbol(name=fname, address=clean_addr, source="wrapper.c")
                        current_ats = []

    def _parse_flashpatches(self, path: str, symbols_map: Dict[int, Symbol]):
        with open(path, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                # BPatchAddr(flash_patch_0, 0x001b9340);
                m = re.search(r"BPatchAddr\s*\(\s*([A-Za-z0-9_]+)\s*,\s*(0x[0-9a-fA-F]+)\s*\)", line)
                if m:
                    name = m.group(1)
                    addr = int(m.group(2), 0) & ~1
                    symbols_map[addr] = Symbol(name=f"hook_{name}", address=addr, source="flashpatches.c")

    def _parse_src_annotations(self, src_dir: str, symbols_map: Dict[int, Symbol]):
        at_pattern = re.compile(r"__attribute__\s*\(\s*\(\s*at\s*\(\s*(0x[0-9a-fA-F]+|[0-9]+)\s*,[^)]*\)\s*\)\s*\)\s*(?:(?:void|int|char|unsigned|uint32_t|uint16_t|uint8_t|struct\s+\w+)\s+[*]*)*([A-Za-z0-9_]+)")
        bpatch_pattern = re.compile(r"BPatch\s*\(\s*([A-Za-z0-9_]+)\s*,\s*(0x[0-9a-fA-F]+|[0-9]+)\s*\)")

        for c_file in glob.glob(os.path.join(src_dir, "*.c")):
            with open(c_file, "r", encoding="utf-8", errors="replace") as f:
                text = f.read()
            for m in at_pattern.finditer(text):
                addr = int(m.group(1), 0) & ~1
                name = m.group(2)
                if addr not in symbols_map:
                    symbols_map[addr] = Symbol(name=f"{name}_hook", address=addr, source="src_annotation")
            for m in bpatch_pattern.finditer(text):
                name = m.group(1)
                addr = int(m.group(2), 0) & ~1
                if addr not in symbols_map:
                    symbols_map[addr] = Symbol(name=f"{name}_bpatch", address=addr, source="src_annotation")

    def resolve_address(self, addr: int) -> ResolvedAddress:
        """Resolve a raw address to memory region and nearest preceding symbol."""
        is_thumb = bool(addr & 1)
        clean_addr = addr & ~1

        # 1. Determine region
        matched_region: Optional[MemoryRegion] = None
        for r in self.regions:
            if r.contains(clean_addr):
                matched_region = r
                break

        # 2. Binary search for exact match or nearest preceding symbol
        best_sym: Optional[Symbol] = None
        best_offset = 0

        if self._symbols_sorted:
            lo, hi = 0, len(self._symbols_sorted) - 1
            idx = -1
            while lo <= hi:
                mid = (lo + hi) // 2
                if self._symbols_sorted[mid].address <= clean_addr:
                    idx = mid
                    lo = mid + 1
                else:
                    hi = mid - 1

            if idx >= 0:
                cand = self._symbols_sorted[idx]
                offset = clean_addr - cand.address
                # Only associate if offset is reasonably small (< 64 KB or within size)
                max_offset = cand.size if cand.size > 0 else 0x10000
                if offset < max_offset:
                    best_sym = cand
                    best_offset = offset

        return ResolvedAddress(
            raw_addr=addr,
            clean_addr=clean_addr,
            is_thumb=is_thumb,
            region=matched_region,
            symbol=best_sym,
            offset=best_offset
        )


class TargetDatabase:
    """Manages chip targets and target auto-detection."""

    def __init__(self, repo_root: Optional[str] = None):
        self.repo_root = repo_root or self._find_repo_root()
        self._target_cache: Dict[str, ChipTarget] = {}

    @staticmethod
    def _find_repo_root() -> str:
        """Find the root directory of the nexmon repository."""
        cur = os.path.abspath(os.path.dirname(__file__))
        while cur and cur != "/":
            if os.path.isdir(os.path.join(cur, "firmwares")) and os.path.isdir(os.path.join(cur, "patches")):
                return cur
            cur = os.path.dirname(cur)
        return "/root/nexmon"

    def list_targets(self) -> List[Tuple[str, str]]:
        """List all available (chip, fw_version) pairs in the repository."""
        targets = set()
        for mk in glob.glob(os.path.join(self.repo_root, "firmwares", "**", "definitions.mk"), recursive=True):
            parts = os.path.relpath(mk, self.repo_root).split(os.sep)
            if len(parts) >= 3:
                targets.add((parts[1], parts[2]))
        for mk in glob.glob(os.path.join(self.repo_root, "patches", "bcm*", "**", "Makefile"), recursive=True):
            parts = os.path.relpath(mk, self.repo_root).split(os.sep)
            if len(parts) >= 3:
                targets.add((parts[1], parts[2]))
        return sorted(list(targets))

    def get_target(self, chip: str, fw: str) -> ChipTarget:
        """Get or load a ChipTarget instance."""
        key = f"{chip}:{fw}"
        if key not in self._target_cache:
            self._target_cache[key] = ChipTarget(chip, fw, self.repo_root)
        return self._target_cache[key]

    def auto_detect(self, text: str) -> Optional[ChipTarget]:
        """Auto-detect target chip and firmware version from log text."""
        # 1. Firmware version banner: Firmware: BCM4345/6 wl0: ... version 7.45.189
        m_fw = re.search(r"Firmware:\s*BCM([0-9a-zA-Z/]+).*?version\s*([0-9._]+)", text, re.IGNORECASE)
        if m_fw:
            chip_str = m_fw.group(1).lower().replace("/", "")
            fw_str = m_fw.group(2).replace(".", "_")
            # Search best match in available targets
            for c, f in self.list_targets():
                if chip_str in c.lower() and (fw_str in f or f in fw_str):
                    return self.get_target(c, f)

        # 2. Driver / module probe names: brcmfmac43455-sdio or cyfmac43455
        m_mod = re.search(r"(?:brcmfmac|cyfmac)([0-9a-zA-Z]+)", text, re.IGNORECASE)
        if m_mod:
            chip_num = m_mod.group(1).lower()
            for c, f in self.list_targets():
                if chip_num in c.lower():
                    return self.get_target(c, f)

        # 3. Direct chip naming in text
        for c, f in self.list_targets():
            if c.lower() in text.lower():
                return self.get_target(c, f)

        # Fallback to default Raspberry Pi 3B+/4B target (bcm43455c0 / 7_45_189) if available
        if ("bcm43455c0", "7_45_189") in self.list_targets():
            return self.get_target("bcm43455c0", "7_45_189")

        targets = self.list_targets()
        if targets:
            return self.get_target(targets[0][0], targets[0][1])

        return None


# -----------------------------------------------------------------------------
# Trap Parser & Exception Decoder
# -----------------------------------------------------------------------------

@dataclass
class TrapFrame:
    """Represents an extracted firmware trap / crash frame."""
    trap_type: int
    epc: int
    pc: int
    lr: int
    sp: int
    cpsr: int
    spsr: int = 0
    offset: int = 0
    regs: Dict[str, int] = field(default_factory=dict)
    assertion: Optional[str] = None
    assert_file: Optional[str] = None
    assert_line: Optional[int] = None
    stack_words: List[Tuple[int, int]] = field(default_factory=list)
    raw_lines: List[str] = field(default_factory=list)


class TrapParser:
    """Extracts trap frames from log strings."""

    TRAP_TYPE_NAMES = {
        0x0: "Reset",
        0x1: "Undefined Instruction",
        0x2: "Software Interrupt (SWI / SVC)",
        0x3: "Prefetch Abort (Instruction Fetch Fault)",
        0x4: "Data Abort (Memory Access Fault / NULL Dereference)",
        0x5: "Reserved / Hypervisor Trap",
        0x6: "IRQ (Interrupt Request)",
        0x7: "FIQ (Fast Interrupt Request)",
    }

    CPSR_MODES = {
        0x10: "User (usr)",
        0x11: "FIQ (fiq)",
        0x12: "IRQ (irq)",
        0x13: "Supervisor (svc)",
        0x17: "Abort (abt)",
        0x1b: "Undefined (und)",
        0x1f: "System (sys)",
    }

    @classmethod
    def parse_text(cls, text: str) -> List[TrapFrame]:
        traps: List[TrapFrame] = []
        lines = text.splitlines()
        i = 0
        n = len(lines)

        while i < n:
            line = lines[i]

            # Match: "dongle trap info: type 0x4 @ epc 0x00045678" or "TRAP type 0x4 @ epc 0x00045678"
            m_sdio = re.search(r"(?:dongle\s+trap\s+info|TRAP(?:_info)?):\s*type\s*(0x[0-9a-fA-F]+|[0-9]+)\s*@?\s*epc\s*(0x[0-9a-fA-F]+)", line)
            m_alt = re.search(r"TRAP:\s*type\s*(0x[0-9a-fA-F]+|[0-9]+).*?epc\s*=\s*(0x[0-9a-fA-F]+)", line)

            if m_sdio or m_alt:
                m = m_sdio or m_alt
                trap_type = int(m.group(1), 0)
                epc = int(m.group(2), 0)
                trap = TrapFrame(trap_type=trap_type, epc=epc, pc=epc, lr=0, sp=0, cpsr=0)
                trap.raw_lines.append(line)

                # Collect subsequent register lines
                j = i + 1
                while j < n and j < i + 12:
                    subline = lines[j]
                    trap.raw_lines.append(subline)

                    # Extract all key-value register patterns: name 0xvalue
                    reg_pairs = re.findall(r"\b([a-zA-Z][a-zA-Z0-9]*)\s+(?:0x)?([0-9a-fA-F]{1,8})\b", subline)
                    for k, v in reg_pairs:
                        kl = k.lower()
                        val = int(v, 16)
                        if kl == "cpsr":
                            trap.cpsr = val
                        elif kl == "spsr":
                            trap.spsr = val
                        elif kl == "sp" or kl == "r13":
                            trap.sp = val
                            trap.regs["sp"] = val
                        elif kl == "lr" or kl == "r14":
                            trap.lr = val
                            trap.regs["lr"] = val
                        elif kl == "pc" or kl == "r15":
                            trap.pc = val
                            trap.regs["pc"] = val
                        elif kl == "offset":
                            trap.offset = val
                        elif kl.startswith("r") and kl[1:].isdigit():
                            trap.regs[kl] = val

                    # Check for stack words on this line
                    # Format: Stack: 0x0023f890: 001a1820 00045678 ...
                    m_stack = re.search(r"(?:Stack|stack|MEM):\s*(?:0x)?([0-9a-fA-F]{4,8}):\s*((?:[0-9a-fA-F]{8}\s*)+)", subline)
                    if m_stack:
                        base_addr = int(m_stack.group(1), 16)
                        words = m_stack.group(2).split()
                        for w_idx, w in enumerate(words):
                            trap.stack_words.append((base_addr + w_idx * 4, int(w, 16)))

                    if "dongle trap info" in subline or "Firmware has halted" in subline:
                        break
                    j += 1

                traps.append(trap)
                i = j
                continue

            # Match assertion failures
            # assertion "len <= 1500" failed: file "wlc_bmac.c", line 452
            m_assert = re.search(r'assertion\s+"([^"]+)"\s+failed:\s+file\s+"([^"]+)",\s+line\s+([0-9]+)', line)
            if m_assert:
                expr = m_assert.group(1)
                afile = m_assert.group(2)
                aline = int(m_assert.group(3))
                # Create a synthetic assert trap or attach to preceding trap
                if traps:
                    traps[-1].assertion = expr
                    traps[-1].assert_file = afile
                    traps[-1].assert_line = aline
                else:
                    trap = TrapFrame(trap_type=0x4, epc=0, pc=0, lr=0, sp=0, cpsr=0)
                    trap.assertion = expr
                    trap.assert_file = afile
                    trap.assert_line = aline
                    trap.raw_lines.append(line)
                    traps.append(trap)

            i += 1

        return traps


# -----------------------------------------------------------------------------
# Diagnostics & Root Cause Analysis
# -----------------------------------------------------------------------------

class CrashAnalyzer:
    """Analyzes a decoded trap frame to provide root-cause diagnostics and backtraces."""

    @classmethod
    def decode_cpsr(cls, cpsr: int) -> Dict[str, Any]:
        mode_val = cpsr & 0x1F
        mode_str = TrapParser.CPSR_MODES.get(mode_val, f"Unknown (0x{mode_val:02x})")
        thumb = bool(cpsr & (1 << 5))
        fiq_masked = bool(cpsr & (1 << 6))
        irq_masked = bool(cpsr & (1 << 7))
        abort_masked = bool(cpsr & (1 << 8))
        endianness = "Big-Endian" if (cpsr & (1 << 9)) else "Little-Endian"
        negative = bool(cpsr & (1 << 31))
        zero = bool(cpsr & (1 << 30))
        carry = bool(cpsr & (1 << 29))
        overflow = bool(cpsr & (1 << 28))

        return {
            "mode": mode_str,
            "thumb": thumb,
            "fiq_masked": fiq_masked,
            "irq_masked": irq_masked,
            "abort_masked": abort_masked,
            "endianness": endianness,
            "flags": {
                "N": negative,
                "Z": zero,
                "C": carry,
                "V": overflow,
            }
        }

    @classmethod
    def diagnose(cls, trap: TrapFrame, target: ChipTarget) -> Tuple[str, List[str]]:
        """Formulate a specific root-cause diagnosis and actionable recommendations."""
        res_epc = target.resolve_address(trap.epc)
        res_lr = target.resolve_address(trap.lr)

        diagnosis = ""
        recs = []

        trap_name = TrapParser.TRAP_TYPE_NAMES.get(trap.trap_type, f"Trap 0x{trap.trap_type:x}")

        if trap.assertion:
            diagnosis = f"Assertion failure: \"{trap.assertion}\" at {trap.assert_file}:{trap.assert_line}"
            recs.append("Verify precondition arguments passed into the failing function.")
            recs.append("Check buffer boundary constraints and length headers in sk_buff / frame headers.")
            return diagnosis, recs

        if trap.trap_type == 0x4:  # Data Abort
            # Check register values for NULL pointers
            null_regs = [reg for reg in ("r0", "r1", "r2", "r3", "r4") if trap.regs.get(reg) == 0]
            if null_regs:
                diagnosis = f"Data Abort: NULL pointer dereference (register(s) {', '.join(null_regs)} = 0x00000000) during memory access in {res_epc.format_inline(False)}."
                recs.append(f"Inspect callers of {res_epc.symbol.name if res_epc.symbol else 'the crashing function'} to ensure the object pointer is checked before dereferencing.")
            else:
                diagnosis = f"Data Abort: Memory access violation / unaligned access at EPC {res_epc.format_inline(False)}."
                recs.append("Check if accessing an unmapped MMIO register, uninitialized pointer, or misaligned 32-bit word.")

            if res_epc.region and res_epc.region.name == "PATCH":
                recs.append("Crash occurred directly inside custom Nexmon patch code. Check buffer bounds and pointer arithmetic.")
            elif res_epc.region and res_epc.region.name == "ROM":
                recs.append(f"Crash occurred in stock ROM code. Called from LR: {res_lr.format_inline(False)}. Inspect parameters passed from the caller.")

        elif trap.trap_type == 0x3:  # Prefetch Abort
            if trap.epc == 0:
                diagnosis = f"Prefetch Abort: NULL function pointer invocation (jumped to 0x00000000). Caller was {res_lr.format_inline(False)}."
                recs.append("Check function pointer / vtable dispatch in the caller for uninitialized or NULL callbacks.")
            else:
                diagnosis = f"Prefetch Abort: CPU attempted to execute instruction from non-executable or unmapped address {res_epc.format_inline(False)}."
                recs.append("Suspected stack smashing or corrupted return address on the stack.")

        elif trap.trap_type == 0x1:  # Undefined Instruction
            cpsr_info = cls.decode_cpsr(trap.cpsr)
            if not cpsr_info["thumb"] and res_epc.region and res_epc.region.name in ("ROM", "RAM"):
                diagnosis = f"Undefined Instruction: CPU executed Thumb instructions in 32-bit ARM mode (Thumb bit clear in CPSR) at {res_epc.format_inline(False)}."
                recs.append("Ensure indirect function calls / branch exchange instructions (BX / BLX) have bit 0 set to 1 for Thumb targets.")
            else:
                diagnosis = f"Undefined Instruction: Illegal opcode encountered at {res_epc.format_inline(False)}."
                recs.append("Check for code corruption, misaligned branch targets, or invalid patch insertion.")

        else:
            diagnosis = f"{trap_name} at EPC {res_epc.format_inline(False)}, LR {res_lr.format_inline(False)}."
            recs.append("Check peripheral interrupt configurations and firmware exception handlers.")

        return diagnosis, recs

    @classmethod
    def reconstruct_backtrace(cls, trap: TrapFrame, target: ChipTarget) -> List[ResolvedAddress]:
        """Reconstruct a potential call stack from EPC, LR, and stack memory words."""
        backtrace: List[ResolvedAddress] = []
        seen_addrs = set()

        # Frame 0: EPC
        if trap.epc != 0:
            res_epc = target.resolve_address(trap.epc)
            backtrace.append(res_epc)
            seen_addrs.add(res_epc.clean_addr)

        # Frame 1: LR
        if trap.lr != 0 and (trap.lr & ~1) not in seen_addrs:
            res_lr = target.resolve_address(trap.lr)
            if res_lr.region and res_lr.region.name in ("ROM", "RAM", "PATCH"):
                backtrace.append(res_lr)
                seen_addrs.add(res_lr.clean_addr)

        # Frame 2+: Scan stack words for valid code pointers
        for _, val in trap.stack_words:
            clean = val & ~1
            if clean not in seen_addrs:
                res = target.resolve_address(val)
                if res.region and res.region.name in ("ROM", "RAM", "PATCH") and res.symbol:
                    backtrace.append(res)
                    seen_addrs.add(clean)

        return backtrace


# -----------------------------------------------------------------------------
# Reporting: Text Cards & JSON Output
# -----------------------------------------------------------------------------

class CrashReporter:
    """Formats and prints crash analysis results."""

    @classmethod
    def print_text_report(cls, traps: List[TrapFrame], target: ChipTarget, verbose: bool = False):
        b = Colors.BOLD
        green = Colors.GREEN
        yellow = Colors.YELLOW
        red = Colors.RED
        cyan = Colors.CYAN
        dim = Colors.DIM
        r = Colors.RESET

        print(f"\n{b}{green}=============================================================================={r}")
        print(f"{b}{green}           NEXMON FIRMWARE CRASH TELEMETRY & TRAP DECODER                   {r}")
        print(f"{b}{green}=============================================================================={r}")
        print(f"{b}Target Chip:{r}      {cyan}{target.chip}{r}")
        print(f"{b}Firmware Ver:{r}     {cyan}{target.fw}{r}")
        print(f"{b}Known Symbols:{r}    {len(target.symbols)} loaded")
        print(f"{b}Memory Map:{r}")
        for reg in target.regions:
            print(f"  - {reg.name:<12} 0x{reg.start:08x} - 0x{reg.end:08x} ({reg.desc})")
        print(f"{b}Traps Detected:{r}   {len(traps)}\n")

        for idx, trap in enumerate(traps, 1):
            trap_name = TrapParser.TRAP_TYPE_NAMES.get(trap.trap_type, f"0x{trap.trap_type:x}")
            res_epc = target.resolve_address(trap.epc)
            res_lr = target.resolve_address(trap.lr)
            res_sp = target.resolve_address(trap.sp)
            cpsr_info = CrashAnalyzer.decode_cpsr(trap.cpsr)
            diagnosis, recs = CrashAnalyzer.diagnose(trap, target)
            backtrace = CrashAnalyzer.reconstruct_backtrace(trap, target)

            print(f"{b}{red}------------------------------------------------------------------------------{r}")
            print(f"{b}{red} TRAP #{idx}: {trap_name} (Type 0x{trap.trap_type:x}){r}")
            print(f"{b}{red}------------------------------------------------------------------------------{r}")

            print(f"{b}Execution State:{r}")
            print(f"  {b}EPC (Crash Point):{r} {res_epc.format_inline()}")
            print(f"  {b}LR  (Caller / Ret):{r}{res_lr.format_inline()}")
            print(f"  {b}SP  (Stack Pointer):{r}0x{trap.sp:08x} [{res_sp.region.name if res_sp.region else 'UNKNOWN'}]")
            print(f"  {b}CPSR:{r}               0x{trap.cpsr:08x} (Mode: {cpsr_info['mode']}, Thumb: {cpsr_info['thumb']})")

            flags_str = "".join([k for k, v in cpsr_info["flags"].items() if v]) or "None"
            print(f"  {b}Condition Flags:{r}    {flags_str} | IRQ-Masked: {cpsr_info['irq_masked']} | FIQ-Masked: {cpsr_info['fiq_masked']}")

            if trap.regs:
                print(f"\n{b}General Purpose Registers:{r}")
                reg_names = [f"r{i}" for i in range(13)]
                for k in range(0, len(reg_names), 4):
                    chunk = reg_names[k:k+4]
                    items = []
                    for rname in chunk:
                        if rname in trap.regs:
                            val = trap.regs[rname]
                            r_res = target.resolve_address(val)
                            reg_tag = f"[{r_res.region.name}]" if (r_res.region and val != 0) else ""
                            items.append(f"{b}{rname:<3}{r}= 0x{val:08x} {dim}{reg_tag:<6}{r}")
                        else:
                            items.append(f"{dim}{rname:<3}= <unset>      {r}")
                    print("  " + "  ".join(items))

            print(f"\n{b}Diagnostic Assessment:{r}")
            print(f"  {yellow}• {diagnosis}{r}")
            for rec in recs:
                print(f"  {cyan}→ Recommendation:{r} {rec}")

            if backtrace:
                print(f"\n{b}Reconstructed Call Stack / Backtrace:{r}")
                for b_idx, frame in enumerate(backtrace):
                    marker = ">> " if b_idx == 0 else "   "
                    print(f"  {marker}#{b_idx:<2} {frame.format_inline()}")

            if verbose and trap.raw_lines:
                print(f"\n{b}Raw Log Excerpt:{r}")
                for l in trap.raw_lines:
                    print(f"  {dim}{l}{r}")

            print("")

    @classmethod
    def to_json_dict(cls, traps: List[TrapFrame], target: ChipTarget) -> Dict[str, Any]:
        result = {
            "target": {
                "chip": target.chip,
                "fw_version": target.fw,
                "memory_regions": [
                    {"name": r.name, "start": hex(r.start), "end": hex(r.end), "desc": r.desc}
                    for r in target.regions
                ],
                "symbols_loaded": len(target.symbols),
            },
            "crashes": []
        }

        for idx, trap in enumerate(traps, 1):
            res_epc = target.resolve_address(trap.epc)
            res_lr = target.resolve_address(trap.lr)
            res_sp = target.resolve_address(trap.sp)
            cpsr_info = CrashAnalyzer.decode_cpsr(trap.cpsr)
            diagnosis, recs = CrashAnalyzer.diagnose(trap, target)
            backtrace = CrashAnalyzer.reconstruct_backtrace(trap, target)

            crash_data = {
                "id": idx,
                "trap_type": trap.trap_type,
                "trap_name": TrapParser.TRAP_TYPE_NAMES.get(trap.trap_type, "Unknown"),
                "epc": {
                    "address": hex(trap.epc),
                    "symbol": res_epc.symbol.name if res_epc.symbol else None,
                    "offset": hex(res_epc.offset) if res_epc.symbol else None,
                    "region": res_epc.region.name if res_epc.region else None,
                    "is_thumb": res_epc.is_thumb,
                },
                "lr": {
                    "address": hex(trap.lr),
                    "symbol": res_lr.symbol.name if res_lr.symbol else None,
                    "offset": hex(res_lr.offset) if res_lr.symbol else None,
                    "region": res_lr.region.name if res_lr.region else None,
                    "is_thumb": res_lr.is_thumb,
                },
                "sp": {
                    "address": hex(trap.sp),
                    "region": res_sp.region.name if res_sp.region else None,
                },
                "cpsr": cpsr_info,
                "registers": {k: hex(v) for k, v in trap.regs.items()},
                "assertion": {
                    "expression": trap.assertion,
                    "file": trap.assert_file,
                    "line": trap.assert_line,
                } if trap.assertion else None,
                "diagnosis": diagnosis,
                "recommendations": recs,
                "backtrace": [
                    {
                        "frame": b_idx,
                        "address": hex(f.raw_addr),
                        "symbol": f.symbol.name if f.symbol else None,
                        "offset": hex(f.offset) if f.symbol else None,
                        "region": f.region.name if f.region else None,
                    }
                    for b_idx, f in enumerate(backtrace)
                ]
            }
            result["crashes"].append(crash_data)

        return result


# -----------------------------------------------------------------------------
# Automated Self-Test Verification Suite
# -----------------------------------------------------------------------------

def run_self_test() -> int:
    """Run built-in automated test suite verifying parser, resolver, and diagnostics."""
    print(f"{Colors.BOLD}Running Nexmon Crash Decoder Self-Test Suite...{Colors.RESET}")

    sample_log = """
[  123.456789] brcmfmac: brcmf_c_preinit_cmds: Firmware: BCM4345/6 wl0: Mar 23 2020 02:40:48 version 7.45.189 (r724032 CY) FWID 01-140b2f
[  124.000100] brcmfmac: brcmf_sdio_trap_info: dongle trap info: type 0x4 @ epc 0x00003834
[  124.000105]   cpsr 0x6000001f spsr 0x00000000 sp 0x0023f890
[  124.000108]   lr   0x001a1820 pc   0x00003834 offset 0x22e000
[  124.000112]   r0   0x00000000 r1   0x0023fa00 r2 0x00000020 r3 0x00000000
[  124.000115]   r4   0x0021a000 r5   0x00000004 r6 0x0023f8a0 r7 0x00000000
[  124.000120]   Stack: 0x0023f890: 001a1820 00003834 0021a000 000037e8
"""

    db = TargetDatabase()
    target = db.auto_detect(sample_log)
    assert target is not None, "Auto-detection failed for sample log"
    assert "43455" in target.chip, f"Expected 43455 in chip, got {target.chip}"

    traps = TrapParser.parse_text(sample_log)
    assert len(traps) == 1, f"Expected 1 trap, got {len(traps)}"

    t = traps[0]
    assert t.trap_type == 4, f"Expected trap type 4, got {t.trap_type}"
    assert t.epc == 0x00003834, f"Expected epc 0x00003834, got 0x{t.epc:x}"
    assert t.regs.get("r0") == 0, "Expected r0 == 0"

    res_epc = target.resolve_address(t.epc)
    assert res_epc.symbol is not None, "Expected symbol resolution for 0x3834"
    assert res_epc.symbol.name == "printf", f"Expected symbol 'printf', got '{res_epc.symbol.name}'"
    assert res_epc.region is not None and res_epc.region.name == "ROM", f"Expected ROM region, got {res_epc.region}"

    diagnosis, recs = CrashAnalyzer.diagnose(t, target)
    assert "NULL pointer dereference" in diagnosis, f"Expected NULL pointer diagnosis, got '{diagnosis}'"

    backtrace = CrashAnalyzer.reconstruct_backtrace(t, target)
    assert len(backtrace) >= 2, f"Expected at least 2 frames in backtrace, got {len(backtrace)}"

    json_dict = CrashReporter.to_json_dict(traps, target)
    assert len(json_dict["crashes"]) == 1
    assert json_dict["crashes"][0]["epc"]["symbol"] == "printf"

    print(f"{Colors.GREEN}{Colors.BOLD}✓ All Self-Test Assertions Passed Successfully!{Colors.RESET}\n")
    return 0


# -----------------------------------------------------------------------------
# Main CLI Entry Point
# -----------------------------------------------------------------------------

def main():
    parser = argparse.ArgumentParser(
        description="Nexmon Firmware Crash Telemetry Extractor & Decoder",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  # Decode from kernel ring buffer (live dmesg):
  ./nexmon_crash_decoder.py --live

  # Decode from a log file:
  ./nexmon_crash_decoder.py -f /var/log/dmesg

  # Pipe dmesg into decoder:
  dmesg | ./nexmon_crash_decoder.py -

  # Decode with explicit chip target and output JSON:
  ./nexmon_crash_decoder.py -f crash.log -c bcm43455c0 -v 7_45_189 --json

  # Run built-in self-test verification:
  ./nexmon_crash_decoder.py --test
"""
    )

    parser.add_argument("input_file", nargs="?", default=None, help="Path to dmesg or crash log file (or '-' for stdin)")
    parser.add_argument("-f", "--file", dest="file_opt", help="Alternative flag for log file path")
    parser.add_argument("-l", "--live", action="store_true", help="Read crash frames live from kernel dmesg ringbuffer")
    parser.add_argument("-c", "--chip", help="Explicit target WiFi chip (e.g. bcm43455c0, bcm43430a1)")
    parser.add_argument("-v", "--fw", help="Explicit target firmware version (e.g. 7_45_189, 7_45_41_46)")
    parser.add_argument("-s", "--raw-trap", help="Pass a raw trap string directly on the command line")
    parser.add_argument("-j", "--json", action="store_true", help="Output telemetry analysis as structured JSON")
    parser.add_argument("--list-targets", action="store_true", help="List all available chip and firmware targets in the repo")
    parser.add_argument("--no-color", action="store_true", help="Disable ANSI terminal colors")
    parser.add_argument("--verbose", action="store_true", help="Display verbose parsing and raw log details")
    parser.add_argument("--test", action="store_true", help="Run automated self-test verification suite")

    args = parser.parse_args()

    if args.no_color or not sys.stdout.isatty():
        Colors.disable()

    if args.test:
        return run_self_test()

    db = TargetDatabase()

    if args.list_targets:
        targets = db.list_targets()
        print(f"{Colors.BOLD}Available Firmware Targets in Nexmon ({len(targets)}):{Colors.RESET}")
        for c, f in targets:
            print(f"  • {c:<16} / {f}")
        return 0

    # 1. Acquire Input Text
    input_text = ""
    target_file = args.file_opt or args.input_file

    if args.raw_trap:
        input_text = args.raw_trap
    elif args.live:
        try:
            res = subprocess.run(["dmesg"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
            input_text = res.stdout
        except Exception as e:
            print(f"{Colors.RED}Error reading dmesg: {e}{Colors.RESET}", file=sys.stderr)
            return 1
    elif target_file == "-":
        input_text = sys.stdin.read()
    elif target_file:
        if not os.path.isfile(target_file):
            print(f"{Colors.RED}Error: File not found: {target_file}{Colors.RESET}", file=sys.stderr)
            return 1
        with open(target_file, "r", encoding="utf-8", errors="replace") as f:
            input_text = f.read()
    else:
        # Check if stdin has data piped
        if not sys.stdin.isatty():
            input_text = sys.stdin.read()
        else:
            # Fall back to live dmesg if available
            try:
                res = subprocess.run(["dmesg"], stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True, check=True)
                input_text = res.stdout
                print(f"{Colors.DIM}Reading live kernel dmesg ringbuffer...{Colors.RESET}")
            except Exception:
                parser.print_help()
                return 1

    if not input_text.strip():
        print(f"{Colors.YELLOW}No input provided or input is empty.{Colors.RESET}", file=sys.stderr)
        return 1

    # 2. Parse Traps
    traps = TrapParser.parse_text(input_text)
    if not traps:
        if args.json:
            print(json.dumps({"error": "No firmware traps or crash frames detected in input.", "crashes": []}, indent=2))
        else:
            print(f"{Colors.YELLOW}No firmware traps, crashes, or assertions detected in input.{Colors.RESET}")
        return 0

    # 3. Resolve Target
    target = None
    if args.chip and args.fw:
        target = db.get_target(args.chip, args.fw)
    elif args.chip:
        # Match chip with first available firmware
        for c, f in db.list_targets():
            if args.chip.lower() in c.lower():
                target = db.get_target(c, f)
                break
    else:
        target = db.auto_detect(input_text)

    if not target:
        # Default fallback
        target = db.get_target("bcm43455c0", "7_45_189")

    # 4. Generate & Output Report
    if args.json:
        result_dict = CrashReporter.to_json_dict(traps, target)
        print(json.dumps(result_dict, indent=2))
    else:
        CrashReporter.print_text_report(traps, target, verbose=args.verbose)

    return 0


if __name__ == "__main__":
    sys.exit(main())
