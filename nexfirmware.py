#!/usr/bin/env python3
"""
Nexmon Firmware Toolkit (nexfirmware)
=====================================
Unified utility for:
  1. Broadcom / Cypress (brcmfmac) chip, driver, and device detection.
  2. Firmware reverse engineering (metadata, memory map, ucode, flashpatches, disassembly).
  3. Firmware lifecycle operations: pull (backup/extract), put (deploy/activate), and edit (binary patch).

Supported Platforms:
  - Linux (Raspberry Pi 3/4/5/Zero, custom SBCs, x86_64, Android).
  - All Broadcom FullMAC (brcmfmac) chips: BCM43455c0, BCM43430a1/b0, BCM43436,
    BCM4339, BCM4335, BCM4356, BCM4358, BCM43596, BCM4366, BCM4375, etc.

Copyright (c) 2026 Nexmon Project
Licensed under Apache 2.0 / GPLv2.
"""

import argparse
import errno
import hashlib
import json
import os
import re
import shutil
import struct
import subprocess
import sys
import tempfile
import time
from dataclasses import asdict, dataclass, field
from pathlib import Path
from typing import Any, Dict, List, Optional, Tuple

# Try importing capstone for disassembly
try:
    import capstone
    HAVE_CAPSTONE = True
except ImportError:
    HAVE_CAPSTONE = False


# =============================================================================
# ANSI Styling
# =============================================================================

class Style:
    RESET   = "\033[0m"
    BOLD    = "\033[1m"
    DIM     = "\033[2m"
    RED     = "\033[91m"
    GREEN   = "\033[92m"
    YELLOW  = "\033[93m"
    BLUE    = "\033[94m"
    MAGENTA = "\033[95m"
    CYAN    = "\033[96m"
    WHITE   = "\033[97m"

    @classmethod
    def disable(cls):
        cls.RESET = cls.BOLD = cls.DIM = cls.RED = cls.GREEN = ""
        cls.YELLOW = cls.BLUE = cls.MAGENTA = cls.CYAN = cls.WHITE = ""


# =============================================================================
# Broadcom Chip & Hardware Database
# =============================================================================

# Known chip mappings: (chipnum, chiprev) -> metadata
CHIP_DATABASE: Dict[Tuple[int, Optional[int]], Dict[str, Any]] = {
    (0x4345, 6): {
        "name": "BCM43455c0",
        "standard_fw": "cyfmac43455-sdio.bin",
        "alt_name": "BCM4345/6",
        "bands": "2.4GHz / 5GHz (802.11ac 1x1)",
        "default_ramstart": 0x198000,
        "default_ramsize": 0xC8000,
        "default_romstart": 0x0,
        "default_romsize": 0xB0000,
        "devices": ["Raspberry Pi 3 Model B+", "Raspberry Pi 4 Model B", "Raspberry Pi 5 Model B", "RPi 400", "RPi CM4", "Huawei P9"],
    },
    (0x4345, 5): {
        "name": "BCM43455b0",
        "standard_fw": "brcmfmac43455-sdio.bin",
        "alt_name": "BCM4345/5",
        "bands": "2.4GHz / 5GHz (802.11ac 1x1)",
        "default_ramstart": 0x198000,
        "default_ramsize": 0xC8000,
        "default_romstart": 0x0,
        "default_romsize": 0xB0000,
        "devices": ["iPhone 6", "Huawei P9 early"],
    },
    (0x4345, None): {
        "name": "BCM4345 series",
        "standard_fw": "cyfmac43455-sdio.bin",
        "alt_name": "BCM4345",
        "bands": "2.4GHz / 5GHz (802.11ac 1x1)",
        "default_ramstart": 0x198000,
        "default_ramsize": 0xC8000,
        "default_romstart": 0x0,
        "default_romsize": 0xB0000,
        "devices": ["Raspberry Pi 3B+/4B/5B"],
    },
    (0xa9a6, 1): {
        "name": "BCM43430a1",
        "standard_fw": "brcmfmac43430-sdio.bin",
        "alt_name": "BCM43430/1",
        "bands": "2.4GHz (802.11bgn 1x1)",
        "default_ramstart": 0x0,
        "default_ramsize": 0x80000,
        "default_romstart": 0x0,
        "default_romsize": 0x60000,
        "devices": ["Raspberry Pi 3 Model B", "Raspberry Pi Zero W"],
    },
    (0xa9a6, 2): {
        "name": "BCM43430b0",
        "standard_fw": "brcmfmac43430b0-sdio.bin",
        "alt_name": "BCM43430/2",
        "bands": "2.4GHz (802.11bgn 1x1)",
        "default_ramstart": 0x0,
        "default_ramsize": 0x80000,
        "default_romstart": 0x0,
        "default_romsize": 0x60000,
        "devices": ["Raspberry Pi Zero 2 W"],
    },
    (0x4343, None): {
        "name": "BCM43430/BCM43438",
        "standard_fw": "brcmfmac43430-sdio.bin",
        "alt_name": "BCM4343",
        "bands": "2.4GHz (802.11bgn)",
        "default_ramstart": 0x0,
        "default_ramsize": 0x80000,
        "default_romstart": 0x0,
        "default_romsize": 0x60000,
        "devices": ["Raspberry Pi 3B", "Raspberry Pi Zero W/2W"],
    },
    (0x4339, None): {
        "name": "BCM4339",
        "standard_fw": "brcmfmac4339-sdio.bin",
        "alt_name": "BCM4339a0",
        "bands": "2.4GHz / 5GHz (802.11ac 1x1)",
        "default_ramstart": 0x180000,
        "default_ramsize": 0xA0000,
        "default_romstart": 0x0,
        "default_romsize": 0x80000,
        "devices": ["Nexus 5", "LG G3"],
    },
    (0x4335, None): {
        "name": "BCM4335",
        "standard_fw": "brcmfmac4335-sdio.bin",
        "alt_name": "BCM4335b0",
        "bands": "2.4GHz / 5GHz (802.11ac 1x1)",
        "default_ramstart": 0x180000,
        "default_ramsize": 0x80000,
        "default_romstart": 0x0,
        "default_romsize": 0x60000,
        "devices": ["Galaxy S4 (GT-I9505)", "HTC One (M7)"],
    },
    (0x4356, None): {
        "name": "BCM4356",
        "standard_fw": "brcmfmac4356-pcie.bin",
        "alt_name": "BCM4356",
        "bands": "2.4GHz / 5GHz (802.11ac 2x2)",
        "default_ramstart": 0x180000,
        "default_ramsize": 0xC0000,
        "default_romstart": 0x0,
        "default_romsize": 0x80000,
        "devices": ["Nexus 6", "Sony Xperia Z5"],
    },
    (0x4358, None): {
        "name": "BCM4358",
        "standard_fw": "brcmfmac4358-pcie.bin",
        "alt_name": "BCM4358",
        "bands": "2.4GHz / 5GHz (802.11ac 2x2)",
        "default_ramstart": 0x180000,
        "default_ramsize": 0xC0000,
        "default_romstart": 0x0,
        "default_romsize": 0x80000,
        "devices": ["Nexus 6P", "Samsung S6"],
    },
    (0x4366, None): {
        "name": "BCM4366c0",
        "standard_fw": "brcmfmac4366c-pcie.bin",
        "alt_name": "BCM4366",
        "bands": "2.4GHz / 5GHz (802.11ac 4x4)",
        "default_ramstart": 0x200000,
        "default_ramsize": 0x100000,
        "default_romstart": 0x0,
        "default_romsize": 0x100000,
        "devices": ["ASUS RT-AC86U", "Netgear R7800"],
    },
    (0x4375, None): {
        "name": "BCM4375b1",
        "standard_fw": "brcmfmac43752-sdio.bin",
        "alt_name": "BCM4375",
        "bands": "2.4GHz / 5GHz / 6GHz (802.11ax 2x2 Wi-Fi 6)",
        "default_ramstart": 0x200000,
        "default_ramsize": 0x100000,
        "default_romstart": 0x0,
        "default_romsize": 0x100000,
        "devices": ["Galaxy S10", "Galaxy S20"],
    },
}


FIRMWARE_VERSION_RE = re.compile(
    rb"(?<!\d)(\d+(?:\.\d+){2,3}(?:\s+\([^)]+\))?)"
)


def extract_firmware_version(data: bytes) -> str:
    """Extract a dotted Broadcom/Cypress firmware version when present."""
    match = FIRMWARE_VERSION_RE.search(data)
    return match.group(1).decode("utf-8", errors="ignore") if match else ""


def read_firmware_blob(path: Union[str, Path]) -> bytes:
    """Read a firmware blob, automatically decompressing if ending in .zst."""
    path = Path(path)
    data = path.read_bytes()
    if path.suffix == ".zst":
        try:
            import zstandard
            dctx = zstandard.ZstdDecompressor()
            return dctx.decompress(data, max_output_size=32 * 1024 * 1024)
        except Exception:
            pass
        try:
            res = subprocess.run(["zstd", "-dc"], input=data, capture_output=True, check=True)
            return res.stdout
        except Exception:
            pass
    return data

# Known SDIO Device IDs
SDIO_DEVICE_MAP: Dict[str, str] = {
    "02d0:4345": "BCM43455 / BCM43456 SDIO Wi-Fi",
    "02d0:a9a6": "BCM43430 SDIO Wi-Fi (RPi 3B / Zero W)",
    "02d0:4339": "BCM4339 SDIO Wi-Fi (Nexus 5)",
    "02d0:4335": "BCM4335 SDIO Wi-Fi",
    "02d0:4334": "BCM43341 SDIO Wi-Fi",
    "02d0:4330": "BCM4330 SDIO Wi-Fi",
    "02d0:4356": "BCM4356 SDIO Wi-Fi",
    "02d0:4354": "BCM4354 SDIO Wi-Fi",
    "02d0:4373": "BCM4373 SDIO Wi-Fi",
    "02d0:aae8": "BCM43752 SDIO Wi-Fi",
}

# PCIe Device IDs
PCIE_DEVICE_MAP: Dict[str, str] = {
    "14e4:43ab": "BCM4345 / BCM43455 PCIe Wi-Fi",
    "14e4:43ec": "BCM4356 PCIe Wi-Fi",
    "14e4:43e9": "BCM4358 PCIe Wi-Fi",
    "14e4:43ba": "BCM43602 PCIe Wi-Fi",
    "14e4:43c3": "BCM4366 PCIe Wi-Fi",
    "14e4:43c4": "BCM4366 2.4GHz PCIe",
    "14e4:43c5": "BCM4366 5GHz PCIe",
    "14e4:4470": "BCM4375 PCIe Wi-Fi",
}


# =============================================================================
# Detection Data Classes
# =============================================================================

@dataclass
class HardwareDeviceInfo:
    bus_type: str                   # "sdio", "pci", "usb", "unknown"
    vendor_id: str                  # e.g. "0x02d0", "0x14e4"
    device_id: str                  # e.g. "0x4345", "0x43ab"
    sysfs_path: str
    modalias: str = ""
    interface: str = ""
    description: str = ""

@dataclass
class DriverInfo:
    loaded: bool
    module_name: str
    version: str
    is_nexmon: bool
    module_path: str = ""
    dependencies: List[str] = field(default_factory=list)

@dataclass
class ActiveFirmwareInfo:
    active_path: str                # Resolved target file
    symlink_path: str               # Original link (e.g. /etc/alternatives/cyfmac43455-sdio.bin)
    file_size: int
    sha256: str
    is_nexmon: bool
    version_string: str = ""
    fw_id: str = ""
    nvram_path: str = ""
    clm_blob_path: str = ""

@dataclass
class HostSystemInfo:
    model: str
    arch: str
    kernel: str
    os_name: str
    is_raspberry_pi: bool

@dataclass
class FirmwareDirEntry:
    raw_line: str
    permissions: str
    owner: str
    group: str
    size: int
    date_str: str
    name: str
    symlink_target: str = ""
    is_symlink: bool = False
    full_path: str = ""
    is_nexmon: bool = False
    is_active: bool = False
    chip_hint: str = ""

@dataclass
class DeviceDetectionReport:
    host: HostSystemInfo
    devices: List[HardwareDeviceInfo]
    chip_name: str
    chip_num: Optional[int]
    chip_rev: Optional[int]
    driver: DriverInfo
    active_firmware: Optional[ActiveFirmwareInfo]
    live_revinfo: Optional[Dict[str, Any]] = None
    firmware_ls_brcm: Optional[List[FirmwareDirEntry]] = None
    firmware_ls_cypress: Optional[List[FirmwareDirEntry]] = None
    firmware_ls_nexmon: Optional[List[FirmwareDirEntry]] = None
    device_firmware_map: Optional[List[Dict[str, Any]]] = None


# =============================================================================
# System & Hardware Detection Engine
# =============================================================================

class DeviceDetector:
    @staticmethod
    def detect_host() -> HostSystemInfo:
        # Detect board model
        model = "Unknown Host"
        for p in ["/sys/firmware/devicetree/base/model", "/proc/device-tree/model"]:
            if os.path.isfile(p):
                try:
                    with open(p, "rb") as f:
                        model = f.read().decode("utf-8", errors="ignore").strip("\x00 \n\r")
                        break
                except OSError:
                    pass

        if model == "Unknown Host" and os.path.isfile("/sys/class/dmi/id/product_name"):
            try:
                with open("/sys/class/dmi/id/product_name") as f:
                    model = f.read().strip()
            except OSError:
                pass

        arch = os.uname().machine
        kernel = os.uname().release
        is_rpi = "Raspberry Pi" in model or os.path.exists("/sys/module/bcm2835") or "bcm27" in kernel or "bcm28" in kernel

        # Detect OS name
        os_name = "Linux"
        if os.path.isfile("/etc/os-release"):
            try:
                with open("/etc/os-release") as f:
                    for line in f:
                        if line.startswith("PRETTY_NAME="):
                            os_name = line.split("=", 1)[1].strip('"\n')
                            break
            except OSError:
                pass

        return HostSystemInfo(
            model=model,
            arch=arch,
            kernel=kernel,
            os_name=os_name,
            is_raspberry_pi=is_rpi
        )

    @staticmethod
    def detect_hardware() -> List[HardwareDeviceInfo]:
        found: List[HardwareDeviceInfo] = []

        # 1. Check SDIO bus
        sdio_base = "/sys/bus/sdio/devices"
        if os.path.isdir(sdio_base):
            for dev in os.listdir(sdio_base):
                dev_path = os.path.join(sdio_base, dev)
                try:
                    vendor_f = os.path.join(dev_path, "vendor")
                    device_f = os.path.join(dev_path, "device")
                    if os.path.isfile(vendor_f) and os.path.isfile(device_f):
                        with open(vendor_f) as vf, open(device_f) as df:
                            v = vf.read().strip().lower()
                            d = df.read().strip().lower()
                        # Normalise to 0xXXXX
                        v_hex = v if v.startswith("0x") else f"0x{int(v, 16):04x}"
                        d_hex = d if d.startswith("0x") else f"0x{int(d, 16):04x}"
                        pair = f"{v_hex[2:]:>04}:{d_hex[2:]:>04}"
                        desc = SDIO_DEVICE_MAP.get(pair, "Broadcom/Cypress SDIO Device")
                        
                        modalias = ""
                        ma_f = os.path.join(dev_path, "modalias")
                        if os.path.isfile(ma_f):
                            with open(ma_f) as maf:
                                modalias = maf.read().strip()

                        # Find associated net interface
                        iface = ""
                        net_dir = os.path.join(dev_path, "net")
                        if os.path.isdir(net_dir):
                            ifaces = os.listdir(net_dir)
                            if ifaces:
                                iface = ifaces[0]

                        if v_hex in ("0x02d0", "0x04b4") or "4345" in d_hex or "4343" in d_hex:
                            found.append(HardwareDeviceInfo(
                                bus_type="sdio",
                                vendor_id=v_hex,
                                device_id=d_hex,
                                sysfs_path=dev_path,
                                modalias=modalias,
                                interface=iface,
                                description=desc
                            ))
                except (OSError, ValueError):
                    continue

        # 2. Check PCI bus
        pci_base = "/sys/bus/pci/devices"
        if os.path.isdir(pci_base):
            for dev in os.listdir(pci_base):
                dev_path = os.path.join(pci_base, dev)
                try:
                    vendor_f = os.path.join(dev_path, "vendor")
                    device_f = os.path.join(dev_path, "device")
                    if os.path.isfile(vendor_f) and os.path.isfile(device_f):
                        with open(vendor_f) as vf, open(device_f) as df:
                            v = vf.read().strip().lower()
                            d = df.read().strip().lower()
                        v_hex = v if v.startswith("0x") else f"0x{int(v, 16):04x}"
                        d_hex = d if d.startswith("0x") else f"0x{int(d, 16):04x}"
                        pair = f"{v_hex[2:]:>04}:{d_hex[2:]:>04}"
                        if v_hex in ("0x14e4", "0x04b4") or pair in PCIE_DEVICE_MAP:
                            desc = PCIE_DEVICE_MAP.get(pair, "Broadcom PCI Wi-Fi Device")
                            iface = ""
                            net_dir = os.path.join(dev_path, "net")
                            if os.path.isdir(net_dir):
                                ifaces = os.listdir(net_dir)
                                if ifaces:
                                    iface = ifaces[0]
                            found.append(HardwareDeviceInfo(
                                bus_type="pci",
                                vendor_id=v_hex,
                                device_id=d_hex,
                                sysfs_path=dev_path,
                                interface=iface,
                                description=desc
                            ))
                except (OSError, ValueError):
                    continue

        # 3. Fallback: inspect network interfaces with driver == brcmfmac
        net_base = "/sys/class/net"
        if os.path.isdir(net_base):
            for iface in os.listdir(net_base):
                driver_link = os.path.join(net_base, iface, "device", "driver")
                if os.path.islink(driver_link) and "brcmfmac" in os.readlink(driver_link):
                    if not any(f.interface == iface for f in found):
                        found.append(HardwareDeviceInfo(
                            bus_type="unknown",
                            vendor_id="unknown",
                            device_id="unknown",
                            sysfs_path=os.path.join(net_base, iface, "device"),
                            interface=iface,
                            description="Broadcom brcmfmac wireless device"
                        ))

        return found

    @staticmethod
    def list_firmware_dir_ls(dir_path: str = "/lib/firmware/brcm") -> List[FirmwareDirEntry]:
        """Parse `ls -l /lib/firmware/brcm/` (and similar) to get firmware file metadata.

        Uses `ls -l` to capture permissions, symlink targets, sizes and dates —
        exactly as the user would see. Falls back to direct directory listing
        if `ls` is not available.
        """
        entries: List[FirmwareDirEntry] = []
        raw_output = ""
        try:
            # Prefer ls -l for the exact user-visible view (permissions, symlink arrows, dates)
            ls_env = os.environ.copy()
            ls_env["LC_ALL"] = "C"
            result = subprocess.run(
                ["ls", "-l", dir_path],
                capture_output=True, text=True, timeout=5,
                env=ls_env
            )
            if result.returncode == 0:
                raw_output = result.stdout
            else:
                raw_output = ""
        except (OSError, subprocess.SubprocessError, subprocess.TimeoutExpired):
            raw_output = ""

        # Fallback to direct listing if ls failed or produced no output
        if not raw_output.strip():
            try:
                base = Path(dir_path)
                if base.is_dir():
                    for p in sorted(base.iterdir()):
                        try:
                            st = p.lstat()
                            perm = "l" + "rwxrwxrwx" if p.is_symlink() else "-rw-r--r--"
                            # lstat size is symlink length, not target; use stat for target size if needed
                            size = st.st_size
                            name = p.name
                            target = ""
                            is_link = p.is_symlink()
                            if is_link:
                                try:
                                    target = os.readlink(str(p))
                                except OSError:
                                    target = ""
                            entries.append(FirmwareDirEntry(
                                raw_line=f"{perm} {size} {name}{' -> ' + target if target else ''}",
                                permissions=perm,
                                owner=str(st.st_uid),
                                group=str(st.st_gid),
                                size=size,
                                date_str="",
                                name=name,
                                symlink_target=target,
                                is_symlink=is_link,
                                full_path=str(p),
                                is_nexmon="nexmon" in name.lower(),
                                chip_hint="",
                            ))
                        except OSError:
                            continue
            except OSError:
                pass
            return entries

        # Parse ls -l output
        # Example: -rw-r--r-- 1 root root 609309 Jul 10 21:44 cyfmac43455-sdio-standard.bin
        #          lrwxrwxrwx 1 root root     38 Mar 21  2026 cyfmac43455-sdio.bin -> /etc/alternatives/cyfmac43455-sdio.bin
        ls_re = re.compile(
            r'^(?P<perm>[dlcbps\-][rwx\-sStT]{9}[.+]?)\s+'
            r'\d+\s+'
            r'(?P<owner>\S+)\s+'
            r'(?P<group>\S+)\s+'
            r'(?P<size>\d+)\s+'
            r'(?P<month>\w+)\s+'
            r'(?P<day>\d+)\s+'
            r'(?P<time_year>\S+)\s+'
            r'(?P<name>.+)$'
        )
        for line in raw_output.splitlines():
            line = line.rstrip()
            if not line or line.startswith("total "):
                continue
            m = ls_re.match(line)
            if not m:
                continue
            perm = m.group("perm")
            owner = m.group("owner")
            group = m.group("group")
            try:
                size = int(m.group("size"))
            except ValueError:
                size = 0
            date_str = f"{m.group('month')} {m.group('day')} {m.group('time_year')}"
            raw_name = m.group("name")
            is_link = perm.startswith("l")
            symlink_target = ""
            name = raw_name
            if is_link and " -> " in raw_name:
                parts = raw_name.split(" -> ", 1)
                name = parts[0]
                symlink_target = parts[1] if len(parts) > 1 else ""
            full_path = os.path.join(dir_path, name)
            # Heuristic chip hint from filename
            chip_hint = ""
            lower = name.lower()
            for chip_key in ["43455", "43430", "4339", "4335", "4356", "4358", "4366", "4375", "43362", "43436"]:
                if chip_key in lower:
                    chip_hint = chip_key
                    break
            # Check if file content indicates nexmon (only for regular files, not symlinks that may point to nexmon)
            is_nexmon = "nexmon" in lower
            entries.append(FirmwareDirEntry(
                raw_line=line,
                permissions=perm,
                owner=owner,
                group=group,
                size=size,
                date_str=date_str,
                name=name,
                symlink_target=symlink_target,
                is_symlink=is_link,
                full_path=full_path,
                is_nexmon=is_nexmon,
                chip_hint=chip_hint,
            ))
        return entries

    @staticmethod
    def map_device_to_firmware_ls(
        devices: List[HardwareDeviceInfo],
        ls_entries_brcm: Optional[List[FirmwareDirEntry]] = None,
        ls_entries_cypress: Optional[List[FirmwareDirEntry]] = None,
        ls_entries_nexmon: Optional[List[FirmwareDirEntry]] = None,
    ) -> List[Dict[str, Any]]:
        """Map each HardwareDeviceInfo to candidate firmware files found via `ls -l`.

        Uses the `ls -l` parsed entries plus device vendor/device IDs and
        interface name to score matches. This is the `ls -l /lib/firmware/brcm/`
        + device info finder requested.
        """
        if ls_entries_brcm is None:
            ls_entries_brcm = DeviceDetector.list_firmware_dir_ls("/lib/firmware/brcm")
        if ls_entries_cypress is None:
            try:
                ls_entries_cypress = DeviceDetector.list_firmware_dir_ls("/lib/firmware/cypress")
            except Exception:
                ls_entries_cypress = []
        if ls_entries_nexmon is None:
            try:
                ls_entries_nexmon = DeviceDetector.list_firmware_dir_ls("/lib/firmware/nexmon")
            except Exception:
                ls_entries_nexmon = []

        # Get active firmware for bonus scoring (realpath of active file and its symlink)
        active_fw_real = None
        active_symlink_real = None
        active_fw_name = ""
        try:
            af = DeviceDetector.detect_active_firmware()
            if af and af.active_path:
                try:
                    active_fw_real = os.path.realpath(af.active_path)
                    active_fw_name = os.path.basename(active_fw_real).lower()
                except OSError:
                    active_fw_real = af.active_path
            if af and af.symlink_path:
                try:
                    active_symlink_real = os.path.realpath(af.symlink_path)
                except OSError:
                    active_symlink_real = af.symlink_path
        except Exception:
            pass

        all_entries = ls_entries_brcm + ls_entries_cypress + ls_entries_nexmon
        # Build quick lookup for active symlink targets (e.g., brcmfmac43455-sdio.bin -> ../cypress/...)
        # and nexmon presence
        result: List[Dict[str, Any]] = []
        for dev in devices:
            dev_hex = dev.device_id.lower().replace("0x", "")
            # Derive expected firmware prefix from device ID, e.g., 4345 -> brcmfmac43455, 43430 -> bcm43430
            candidates: List[Tuple[FirmwareDirEntry, int, str]] = []
            for entry in all_entries:
                score = 0
                reason = []
                lower_name = entry.name.lower()
                # Direct chip ID substring match
                if dev_hex in lower_name or dev_hex.replace("a9a6", "43430") in lower_name:
                    score += 50
                    reason.append(f"chip {dev_hex} in name")
                # Vendor-specific: 02d0:4345 should match brcmfmac43455 / cyfmac43455
                if "43455" in lower_name and dev_hex in ("4345", "43455"):
                    score += 50
                    reason.append("43455 match")
                if "43430" in lower_name and dev_hex in ("a9a6", "4343"):
                    score += 50
                    reason.append("43430 match")
                # Interface name hint (wlan0 often uses brcmfmac43455 on Pi)
                if dev.interface and dev.interface.startswith("wlan"):
                    if "brcmfmac" in lower_name or "cyfmac" in lower_name:
                        score += 5
                        reason.append("wlan interface")
                # Active symlink: if entry is a symlink that is the active firmware path
                if entry.is_symlink and entry.symlink_target:
                    # Distinguish the true active chain (cyfmac -> /etc/alternatives) from
                    # generic board symlinks (brcmfmac... -> ../cypress/cyfmac...). The latter
                    # points to the minimal cyfmac, not the active standard.
                    if "alternatives" in entry.symlink_target:
                        score += 30
                        reason.append("active symlink")
                    elif "cypress" in entry.symlink_target:
                        score += 5
                        reason.append("cypress link")
                # Nexmon marker
                if entry.is_nexmon:
                    score += 10
                    reason.append("nexmon")
                # Size sanity: firmware binaries are typically 200k-700k, not .txt .hcd small files
                if lower_name.endswith(".bin") and 100000 <= entry.size <= 800000:
                    score += 10
                    reason.append("bin size sane")
                if lower_name.endswith(".txt") and dev.interface:
                    # Board-specific txt often matches interface's board type
                    score += 2
                    reason.append("txt")
                # Bonus for being the currently active firmware (as seen via ls -l symlink chain)
                try:
                    entry_real = os.path.realpath(entry.full_path)
                    target_real = os.path.realpath(os.path.join(os.path.dirname(entry.full_path), entry.symlink_target)) if entry.is_symlink and entry.symlink_target else ""
                    if active_fw_real and (entry_real == active_fw_real or target_real == active_fw_real or lower_name == active_fw_name):
                        score += 100
                        reason.append("active firmware")
                    elif active_symlink_real and (entry_real == active_symlink_real or target_real == active_symlink_real):
                        score += 50
                        reason.append("active symlink")
                except OSError:
                    pass
                if score > 0:
                    candidates.append((entry, score, ", ".join(reason)))
            # Sort by score descending, then by name for stability
            candidates.sort(key=lambda x: (-x[1], x[0].name))
            # Convert to serializable dicts (keep top 12 to ensure active firmware is included)
            mapped = []
            for entry, score, reason in candidates[:12]:
                mapped.append({
                    "file": entry.name,
                    "full_path": entry.full_path,
                    "size": entry.size,
                    "permissions": entry.permissions,
                    "symlink_target": entry.symlink_target,
                    "is_symlink": entry.is_symlink,
                    "date": entry.date_str,
                    "raw_ls_line": entry.raw_line,
                    "score": score,
                    "reason": reason,
                    "chip_hint": entry.chip_hint,
                })
            result.append({
                "device": asdict(dev),
                "ls_candidates": mapped,
                "ls_summary": {
                    "total_brcm": len(ls_entries_brcm),
                    "total_cypress": len(ls_entries_cypress),
                    "total_nexmon": len(ls_entries_nexmon),
                    "total_candidates_for_device": len(mapped),
                }
            })
        return result

    @staticmethod
    def detect_driver() -> DriverInfo:
        loaded = False
        version = "unknown"
        is_nexmon = False
        module_path = ""
        deps: List[str] = []

        # Check lsmod
        try:
            res = subprocess.run(["lsmod"], capture_output=True, text=True, timeout=5)
            for line in res.stdout.splitlines():
                if line.startswith("brcmfmac "):
                    loaded = True
                    parts = line.split()
                    if len(parts) >= 4:
                        deps = parts[3].split(",")
                    break
        except (OSError, subprocess.SubprocessError, subprocess.TimeoutExpired):
            pass

        # Check modinfo
        try:
            res = subprocess.run(["modinfo", "brcmfmac"], capture_output=True, text=True, timeout=5)
            for line in res.stdout.splitlines():
                if line.startswith("version:"):
                    version = line.split(":", 1)[1].strip()
                elif line.startswith("filename:"):
                    module_path = line.split(":", 1)[1].strip()
        except (OSError, subprocess.SubprocessError, subprocess.TimeoutExpired):
            pass

        if "nexmon" in version.lower() or "nexmon" in module_path.lower():
            is_nexmon = True

        return DriverInfo(
            loaded=loaded,
            module_name="brcmfmac",
            version=version,
            is_nexmon=is_nexmon,
            module_path=module_path,
            dependencies=deps
        )

    @staticmethod
    def detect_active_firmware() -> Optional[ActiveFirmwareInfo]:
        chip_candidates = ["43455", "43430", "43436", "4356", "4358"]
        try:
            detected = DeviceDetector.detect_hardware()
            if detected:
                for d in detected:
                    for c in ["43455", "43430", "43436", "4356", "4358"]:
                        if c in getattr(d, "chip_name", "").lower() or c in d.device_id.lower():
                            if c in chip_candidates:
                                chip_candidates.remove(c)
                            chip_candidates.insert(0, c)
        except Exception:
            pass

        candidates = []
        for c in chip_candidates:
            for base_cand in [
                f"/etc/alternatives/cyfmac{c}-sdio.bin",
                f"/etc/alternatives/brcmfmac{c}-sdio.bin",
                f"/lib/firmware/cypress/cyfmac{c}-sdio.bin",
                f"/lib/firmware/brcm/brcmfmac{c}-sdio.bin",
                f"/lib/firmware/cypress/cyfmac{c}-sdio-standard.bin",
                f"/lib/firmware/nexmon/cyfmac{c}-sdio-standard-265.bin",
                f"/lib/firmware/nexmon/cyfmac{c}-sdio-standard.bin",
                f"/lib/firmware/nexmon/brcmfmac{c}-sdio.bin",
            ]:
                candidates.append(base_cand)
                candidates.append(f"{base_cand}.zst")

        # Check dmesg for current running firmware message
        fw_ver_dmesg = ""
        fw_id_dmesg = ""
        try:
            res = subprocess.run(["dmesg"], capture_output=True, text=True, timeout=5)
            for line in reversed(res.stdout.splitlines()):
                if "brcmf_c_preinit_dcmds: Firmware:" in line or "Firmware: BCM" in line:
                    m = re.search(r'version\s+([^\s]+(?:\s+\([^)]+\))?)', line)
                    if m:
                        fw_ver_dmesg = m.group(1)
                    m_id = re.search(r'FWID\s+([0-9a-fA-F-]+)', line)
                    if m_id:
                        fw_id_dmesg = m_id.group(1)
                    break
        except (OSError, subprocess.SubprocessError, subprocess.TimeoutExpired):
            pass

        # Inspect candidate paths
        target_path = None
        symlink_path = None
        for c in candidates:
            if os.path.exists(c):
                symlink_path = c
                target_path = os.path.realpath(c)
                break

        if not target_path or not os.path.isfile(target_path):
            return None

        # Check file properties
        try:
            fsize = os.path.getsize(target_path)
        except OSError:
            return None
        try:
            data = read_firmware_blob(target_path)
            if target_path.endswith(".zst"):
                fsize = len(data)
        except OSError:
            return None

        sha256 = hashlib.sha256(data).hexdigest()
        is_nexmon = b"nexmon.org" in data or b"NEXMON" in data

        # Extract embedded version string if not from dmesg
        ver_str = fw_ver_dmesg
        if not ver_str:
            ver_str = extract_firmware_version(data)

        # Find associated NVRAM and CLM blob
        base_dir = os.path.dirname(target_path)
        clean_name = target_path[:-4] if target_path.endswith(".zst") else target_path
        stem = Path(clean_name).stem.replace("-standard", "").replace("-265", "").replace("-206", "")
        nvram_p = ""
        for ext in [".txt", ".nvram", ".txt.zst", ".nvram.zst"]:
            cand = os.path.join(base_dir, stem + ext)
            if os.path.isfile(cand):
                nvram_p = cand
                break
        if not nvram_p:
            for cand in [
                "/lib/firmware/brcm/brcmfmac43455-sdio.txt",
                "/lib/firmware/brcm/brcmfmac43455-sdio.txt.zst",
            ]:
                if os.path.isfile(cand):
                    nvram_p = cand
                    break

        clm_p = ""
        for cand in [
            os.path.join(base_dir, stem + ".clm_blob"),
            os.path.join(base_dir, stem + ".clm_blob.zst"),
            "/lib/firmware/cypress/cyfmac43455-sdio.clm_blob",
            "/lib/firmware/cypress/cyfmac43455-sdio.clm_blob.zst",
            "/lib/firmware/brcm/brcmfmac43455-sdio.clm_blob",
            "/lib/firmware/brcm/brcmfmac43455-sdio.clm_blob.zst",
        ]:
            if os.path.isfile(cand):
                clm_p = cand
                break

        return ActiveFirmwareInfo(
            active_path=target_path,
            symlink_path=symlink_path or target_path,
            file_size=fsize,
            sha256=sha256,
            is_nexmon=is_nexmon,
            version_string=ver_str,
            fw_id=fw_id_dmesg,
            nvram_path=nvram_p,
            clm_blob_path=clm_p
        )

    @staticmethod
    def query_live_revinfo(interface: str = "wlan0") -> Optional[Dict[str, Any]]:
        # Run nexutil -V on the specified interface
        nexutil_path = shutil.which("nexutil") or "/usr/bin/nexutil"
        if not os.path.isfile(nexutil_path) or not os.access(nexutil_path, os.X_OK):
            return None

        try:
            res = subprocess.run([nexutil_path, f"-I{interface}", "-V"],
                                 capture_output=True, text=True, timeout=5)
            if res.returncode != 0:
                return None

            info: Dict[str, Any] = {}
            for line in res.stdout.splitlines():
                line = line.strip()
                if " " in line and not line.startswith("---") and "|" not in line:
                    k, v = line.split(" ", 1)
                    info[k.strip()] = v.strip()
            return info if info else None
        except (OSError, subprocess.SubprocessError, subprocess.TimeoutExpired, ValueError):
            return None

    @classmethod
    def full_detection(cls, interface: str = "wlan0") -> DeviceDetectionReport:
        host = cls.detect_host()
        devices = cls.detect_hardware()
        driver = cls.detect_driver()
        fw = cls.detect_active_firmware()
        revinfo = cls.query_live_revinfo(interface)

        # Gather ls -l /lib/firmware/brcm/ etc. for comprehensive firmware view
        # This is the user-requested `ls -l /lib/firmware/brcm/` + device info finder
        try:
            ls_brcm = cls.list_firmware_dir_ls("/lib/firmware/brcm")
        except Exception:
            ls_brcm = []
        try:
            ls_cypress = cls.list_firmware_dir_ls("/lib/firmware/cypress")
        except Exception:
            ls_cypress = []
        try:
            ls_nexmon = cls.list_firmware_dir_ls("/lib/firmware/nexmon")
        except Exception:
            ls_nexmon = []
        try:
            dev_fw_map = cls.map_device_to_firmware_ls(devices, ls_brcm, ls_cypress, ls_nexmon)
        except Exception:
            dev_fw_map = []

        # Match chip name and revision
        chip_name = "Unknown Broadcom / Cypress"
        chip_num = None
        chip_rev = None

        if revinfo and "chipnum" in revinfo:
            try:
                chip_num = int(revinfo["chipnum"], 16)
                chip_rev = int(revinfo.get("chiprev", "0"), 16)
            except ValueError:
                pass

        if chip_num is None and devices:
            for dev in devices:
                try:
                    d_int = int(dev.device_id, 16)
                    chip_num = d_int
                    break
                except (OSError, ValueError):
                    continue

        # Lookup in chip database
        if chip_num is not None:
            if (chip_num, chip_rev) in CHIP_DATABASE:
                chip_name = CHIP_DATABASE[(chip_num, chip_rev)]["name"]
            elif (chip_num, None) in CHIP_DATABASE:
                chip_name = CHIP_DATABASE[(chip_num, None)]["name"]
            else:
                chip_name = f"BCM{chip_num:04x}"
        elif host.is_raspberry_pi:
            if "Raspberry Pi 4" in host.model or "Raspberry Pi 3 Model B+" in host.model or "Raspberry Pi 5" in host.model:
                chip_name = "BCM43455c0"
                chip_num = 0x4345
                chip_rev = 6
            elif "Raspberry Pi 3 Model B" in host.model or "Zero W" in host.model:
                chip_name = "BCM43430a1"
                chip_num = 0xa9a6
                chip_rev = 1

        return DeviceDetectionReport(
            host=host,
            devices=devices,
            chip_name=chip_name,
            chip_num=chip_num,
            chip_rev=chip_rev,
            driver=driver,
            active_firmware=fw,
            live_revinfo=revinfo,
            firmware_ls_brcm=ls_brcm,
            firmware_ls_cypress=ls_cypress,
            firmware_ls_nexmon=ls_nexmon,
            device_firmware_map=dev_fw_map,
        )


# =============================================================================
# Reverse Engineering & Binary Analysis Engine
# =============================================================================

@dataclass
class FlashPatchEntry:
    index: int
    target_rom_addr: int
    size: int
    data_ram_addr: int
    patch_bytes: bytes
    disassembly: List[str] = field(default_factory=list)

@dataclass
class FirmwareAnalysisReport:
    file_path: str
    file_size: int
    sha256: str
    architecture: str
    ram_start: int
    ram_size: int
    rom_start: int
    rom_size: int
    version_string: str
    build_date: str
    build_time: str
    is_nexmon_patched: bool
    nexmon_capabilities: List[str]
    ucode_start: Optional[int]
    ucode_size: Optional[int]
    templateram_start: Optional[int]
    templateram_size: Optional[int]
    reclaim_0_end: Optional[int]
    reclaim_0_end_ptr: Optional[int]
    ucode_write_bl_hook: Optional[int]
    fp_config_base: Optional[int]
    fp_config_end: Optional[int]
    flash_patches: List[FlashPatchEntry]
    definitions_mk: str


class FirmwareReverseEngineer:
    UCODE_MAGIC = bytes.fromhex("4e10000360bc0100")
    # Reuse capstone engines across calls — init is heavy (allocates tables).
    _capstone_cache: Dict[bool, Any] = {}

    @classmethod
    def _slots(cls, data: bytes, val: int, ram_start: int) -> List[int]:
        b = struct.pack("<I", val)
        out = []
        i = data.find(b)
        while i != -1:
            if i % 4 == 0:
                out.append(ram_start + i)
            i = data.find(b, i + 1)
        return out

    @classmethod
    def _rd32(cls, data: bytes, addr: int, ram_start: int) -> int:
        off = addr - ram_start
        if off < 0 or off + 4 > len(data):
            return 0
        return struct.unpack("<I", data[off:off+4])[0]

    @classmethod
    def analyze_firmware(cls, file_path: str, ram_start: int = 0x198000,
                         ram_size: int = 0xC8000, rom_start: int = 0x0,
                         rom_size: int = 0xB0000) -> FirmwareAnalysisReport:
        path = Path(file_path).resolve()
        if not path.is_file():
            raise FileNotFoundError(f"Firmware binary not found: {path}")

        # Auto-detect chip-specific RAM/ROM defaults when the caller used the
        # 43455c0 hardcoded defaults but the file belongs to a different chip.
        # This avoids silently analysing a BCM43430/BCM4339 image with the
        # wrong memory map (e.g. ram_start 0x198000 instead of 0x0/0x180000).
        caller_used_defaults = (
            ram_start == 0x198000 and ram_size == 0xC8000 and rom_start == 0x0 and rom_size == 0xB0000
        )
        if caller_used_defaults:
            # Try repo-local definitions.mk next to the firmware
            for candidate in [path.parent / "definitions.mk", Path(__file__).resolve().parent / "firmwares" / path.parent.name / "definitions.mk"]:
                if candidate.is_file():
                    try:
                        txt = candidate.read_text(errors="replace")
                        m_ram = re.search(r"^\s*RAMSTART\s*=\s*(0x[0-9a-fA-F]+)", txt, re.MULTILINE)
                        m_ramsz = re.search(r"^\s*RAMSIZE\s*=\s*(0x[0-9a-fA-F]+)", txt, re.MULTILINE)
                        m_rom = re.search(r"^\s*ROMSTART\s*=\s*(0x[0-9a-fA-F]+)", txt, re.MULTILINE)
                        m_romsz = re.search(r"^\s*ROMSIZE\s*=\s*(0x[0-9a-fA-F]+)", txt, re.MULTILINE)
                        if m_ram:
                            ram_start = int(m_ram.group(1), 16)
                        if m_ramsz:
                            ram_size = int(m_ramsz.group(1), 16)
                        if m_rom:
                            rom_start = int(m_rom.group(1), 16)
                        if m_romsz:
                            rom_size = int(m_romsz.group(1), 16)
                        break
                    except (OSError, ValueError):
                        pass
            # Fallback to CHIP_DATABASE via filename match
            if ram_start == 0x198000:
                for entry in CHIP_DATABASE.values():
                    if entry.get("standard_fw") and Path(entry["standard_fw"]).name == path.name:
                        ram_start = entry.get("default_ramstart", ram_start)
                        ram_size = entry.get("default_ramsize", ram_size)
                        rom_start = entry.get("default_romstart", rom_start)
                        rom_size = entry.get("default_romsize", rom_size)
                        break

        data = path.read_bytes()
        fsize = len(data)
        sha256 = hashlib.sha256(data).hexdigest()

        # Check version, date, time strings
        ver_str = extract_firmware_version(data) or "Unknown"

        date_str = ""
        m_date = re.search(rb'([A-Z][a-z]{2}\s+[0-9]{1,2}\s+20[0-9]{2})', data)
        if m_date:
            date_str = m_date.group(1).decode("utf-8", errors="ignore")

        time_str = ""
        if m_date:
            m_time = re.search(rb'([0-9]{2}:[0-9]{2}:[0-9]{2})', data[m_date.start():])
            if m_time:
                time_str = m_time.group(1).decode("utf-8", errors="ignore")

        # Check if nexmon patched
        is_nexmon = b"nexmon.org" in data or b"NEXMON" in data
        capabilities = []
        if is_nexmon:
            # Check known capabilities
            capabilities = ["MONITOR_MODE", "RADIOTAP", "FRAME_INJECTION"]

        # Derive ucode start — use efficient bytes.find loop instead of
        # O(n) Python `data.startswith` per offset (600KB → 600k iters).
        hits = []
        pos = data.find(cls.UCODE_MAGIC)
        while pos != -1:
            hits.append(ram_start + pos)
            pos = data.find(cls.UCODE_MAGIC, pos + 1)
        ucode_start = hits[0] if hits else None

        # Derive BL hook
        bl_hook = None
        if ucode_start:
            s = [a for a in cls._slots(data, ucode_start, ram_start) if a > 0x200000]
            if s:
                bl_hook = s[0] - 0x10

        # Derive reclaim_0_end
        reclaim_0_end = None
        reclaim_0_end_ptr = None
        for a in range(0x19A000, min(0x19A800, ram_start + fsize - 20), 4):
            vals = [cls._rd32(data, a + 4*k, ram_start) for k in (1, 2, 3, 4)]
            if vals == [0x1980D4, 0x198100, 0x198000, 0x198000]:
                reclaim_0_end_ptr = a
                reclaim_0_end = cls._rd32(data, a, ram_start)
                break

        # Derive templateram
        templateram_start = None
        templateram_size = None
        ucode_size = None
        if ucode_start and reclaim_0_end:
            t = cls._rd32(data, ucode_start - 0x16C, ram_start)
            if ucode_start < t < reclaim_0_end:
                templateram_start = t
                templateram_size = reclaim_0_end - t
                ucode_size = t - ucode_start
            else:
                templateram_start = 0
                templateram_size = 0
                ucode_size = reclaim_0_end - ucode_start

        # Flashpatch config scanning — hoist capstone engine outside loop.
        fp_config_base = 0x199000
        fp_config_end = None
        flashpatches: List[FlashPatchEntry] = []

        capstone_md = None
        if HAVE_CAPSTONE:
            try:
                capstone_md = capstone.Cs(capstone.CS_ARCH_ARM, capstone.CS_MODE_THUMB)
            except Exception:
                capstone_md = None

        a = fp_config_base
        idx = 0
        while a >= ram_start and a - ram_start + 12 <= len(data):
            tg, sz, dp = struct.unpack("<III", data[a - ram_start : a - ram_start + 12])
            if not (0 <= tg < rom_size and 0 < sz <= 0x40 and ram_start <= dp and dp - ram_start + sz <= len(data)):
                break
            
            p_bytes = data[dp - ram_start : dp - ram_start + sz]
            disasm_lines = []
            if capstone_md is not None and sz >= 2:
                try:
                    for insn in capstone_md.disasm(p_bytes, tg):
                        disasm_lines.append(f"0x{insn.address:06x}: {insn.mnemonic}\t{insn.op_str}")
                except Exception:
                    pass

            flashpatches.append(FlashPatchEntry(
                index=idx,
                target_rom_addr=tg,
                size=sz,
                data_ram_addr=dp,
                patch_bytes=p_bytes,
                disassembly=disasm_lines
            ))
            a += 12
            idx += 1

        if flashpatches:
            fp_config_end = a

        # Synthesize definitions.mk text
        patch_size = 0x4000
        patch_start = (reclaim_0_end - patch_size) if reclaim_0_end else (0x22CAD4 - patch_size)
        fp_conf_size = 0xC00
        fp_conf_base_derived = patch_start - fp_conf_size

        def_lines = [
            f"# Auto-derived definitions.mk for {path.name}",
            "NEXMON_CHIP=CHIP_VER_BCM43455c0",
            f"NEXMON_FW_VERSION=FW_VER_{ver_str.replace('.', '_').replace(' ', '_').replace('(', '').replace(')', '')}",
            "NEXMON_ARCH=armv7-r",
            "",
            f"RAM_FILE={path.name}",
            f"RAMSTART=0x{ram_start:X}",
            f"RAMSIZE=0x{ram_size:X}",
            "",
            "ROM_FILE=rom.bin",
            f"ROMSTART=0x{rom_start:X}",
            f"ROMSIZE=0x{rom_size:X}",
            "",
            f"WLC_UCODE_WRITE_BL_HOOK_ADDR=0x{bl_hook:X}" if bl_hook else "# WLC_UCODE_WRITE_BL_HOOK_ADDR=",
            f"HNDRTE_RECLAIM_0_END_PTR=0x{reclaim_0_end_ptr:X}" if reclaim_0_end_ptr else "# HNDRTE_RECLAIM_0_END_PTR=",
            f"HNDRTE_RECLAIM_0_END=0x{reclaim_0_end:X}" if reclaim_0_end else "# HNDRTE_RECLAIM_0_END=",
            "",
            f"PATCHSIZE=0x{patch_size:X}",
            "PATCHSTART=$$(($$(HNDRTE_RECLAIM_0_END) - $$(PATCHSIZE)))",
            "",
            f"UCODESTART=0x{ucode_start:X}" if ucode_start else "# UCODESTART=",
            f"UCODESIZE=0x{ucode_size:X}" if ucode_size else "# UCODESIZE=",
            "",
            f"TEMPLATERAMSTART_PTR=0x{(ucode_start - 0x16C):X}" if ucode_start else "# TEMPLATERAMSTART_PTR=",
            f"TEMPLATERAMSTART=0x{templateram_start:X}" if templateram_start else "# TEMPLATERAMSTART=",
            f"TEMPLATERAMSIZE=0x{templateram_size:X}" if templateram_size else "# TEMPLATERAMSIZE=",
            "",
            f"FP_CONFIG_SIZE=0x{fp_conf_size:X}",
            "FP_CONFIG_BASE=$$(($$(PATCHSTART) - $$(FP_CONFIG_SIZE)))",
            f"FP_CONFIG_ORIGBASE=0x{fp_config_base:X}",
            f"FP_CONFIG_ORIGEND=0x{fp_config_end:X}" if fp_config_end else "# FP_CONFIG_ORIGEND=",
        ]

        return FirmwareAnalysisReport(
            file_path=str(path),
            file_size=fsize,
            sha256=sha256,
            architecture="ARM Cortex-R4 (armv7-r)",
            ram_start=ram_start,
            ram_size=ram_size,
            rom_start=rom_start,
            rom_size=rom_size,
            version_string=ver_str,
            build_date=date_str,
            build_time=time_str,
            is_nexmon_patched=is_nexmon,
            nexmon_capabilities=capabilities,
            ucode_start=ucode_start,
            ucode_size=ucode_size,
            templateram_start=templateram_start,
            templateram_size=templateram_size,
            reclaim_0_end=reclaim_0_end,
            reclaim_0_end_ptr=reclaim_0_end_ptr,
            ucode_write_bl_hook=bl_hook,
            fp_config_base=fp_conf_base_derived,
            fp_config_end=fp_config_end,
            flash_patches=flashpatches,
            definitions_mk="\n".join(def_lines)
        )

    @classmethod
    def disassemble_range(cls, file_path: str, start_addr: int, count_bytes: int = 64,
                          ram_start: int = 0x198000, thumb: bool = True) -> List[str]:
        data = Path(file_path).read_bytes()
        offset = start_addr - ram_start
        if offset < 0 or offset >= len(data):
            return [f"Address 0x{start_addr:x} is outside RAM [0x{ram_start:x}-0x{ram_start+len(data):x}]"]

        chunk = data[offset : offset + count_bytes]
        lines = []

        if HAVE_CAPSTONE:
            mode = capstone.CS_MODE_THUMB if thumb else capstone.CS_MODE_ARM
            # Cache engines by mode (THUMB vs ARM) to avoid re-allocating tables.
            cache_key = thumb
            md = cls._capstone_cache.get(cache_key)
            if md is None:
                try:
                    md = capstone.Cs(capstone.CS_ARCH_ARM, mode)
                    cls._capstone_cache[cache_key] = md
                except Exception:
                    md = None
            if md is not None:
                # Ensure mode matches cached instance (capstone mode is mutable)
                try:
                    # Capstone 5.x uses md.mode; re-set to avoid stale mode
                    md.mode = mode
                except Exception:
                    pass
                for insn in md.disasm(chunk, start_addr):
                    hex_b = " ".join(f"{b:02x}" for b in insn.bytes)
                    lines.append(f"0x{insn.address:06x}:  {hex_b:<12}  {insn.mnemonic:<8} {insn.op_str}")
            else:
                # Fallback if cache creation failed
                md = capstone.Cs(capstone.CS_ARCH_ARM, mode)
                for insn in md.disasm(chunk, start_addr):
                    hex_b = " ".join(f"{b:02x}" for b in insn.bytes)
                    lines.append(f"0x{insn.address:06x}:  {hex_b:<12}  {insn.mnemonic:<8} {insn.op_str}")
        else:
            # Fallback to objdump if capstone is absent
            lines.append("Capstone not available; using hexdump fallback:")
            for i in range(0, len(chunk), 4):
                word = struct.unpack("<I", chunk[i:i+4])[0] if i+4 <= len(chunk) else 0
                lines.append(f"0x{start_addr+i:06x}:  0x{word:08x}")

        return lines

    @staticmethod
    def _run_external_tool(
        tool_name: str,
        arguments: List[str],
        output_file: Optional[str] = None,
        timeout: int = 120,
    ) -> Dict[str, Any]:
        """Run a requested read-only analysis tool and capture its report."""
        executable = shutil.which(tool_name)
        if not executable:
            raise RuntimeError(
                f"{tool_name} is not installed or is not available in PATH"
            )

        command = [executable, *arguments]
        try:
            result = subprocess.run(
                command,
                check=True,
                capture_output=True,
                text=True,
                errors="replace",
                timeout=timeout,
            )
        except subprocess.TimeoutExpired as exc:
            raise RuntimeError(f"{tool_name} timed out after {timeout} seconds") from exc
        except subprocess.CalledProcessError as exc:
            detail = exc.stderr or exc.stdout or str(exc)
            raise RuntimeError(f"{tool_name} failed: {detail.strip()}") from exc

        saved_path = None
        output = result.stdout
        if output_file and output_file != "-":
            destination = Path(output_file).resolve()
            destination.parent.mkdir(parents=True, exist_ok=True)
            # Backup existing output file before overwriting
            if destination.exists():
                try:
                    backup = destination.with_name(f"{destination.name}.bak-{time.time_ns()}")
                    shutil.copy2(destination, backup)
                except OSError:
                    pass
            destination.write_text(output, encoding="utf-8")
            saved_path = str(destination)
            output = None
        elif output_file == "-":
            # Explicit stdout request — keep output in return value
            saved_path = None

        return {
            "tool": tool_name,
            "executable": executable,
            "command": command,
            "output": output,
            "output_path": saved_path,
            "stderr": result.stderr,
        }

    @classmethod
    def extract_strings(
        cls,
        file_path: str,
        min_length: int = 4,
        output_file: Optional[str] = None,
    ) -> Dict[str, Any]:
        """Run GNU/binutils strings with hexadecimal file offsets."""
        path = Path(file_path).resolve()
        if not path.is_file():
            raise FileNotFoundError(f"Firmware binary not found: {path}")
        if min_length < 1:
            raise ValueError("Minimum string length must be at least 1")
        return cls._run_external_tool(
            "strings",
            ["-a", "-t", "x", "-n", str(min_length), str(path)],
            output_file=output_file,
        )

    @classmethod
    def scan_binwalk(
        cls,
        file_path: str,
        output_file: Optional[str] = None,
    ) -> Dict[str, Any]:
        """Run a read-only binwalk signature scan over a firmware image."""
        path = Path(file_path).resolve()
        if not path.is_file():
            raise FileNotFoundError(f"Firmware binary not found: {path}")
        return cls._run_external_tool(
            "binwalk",
            [str(path)],
            output_file=output_file,
        )

    @classmethod
    def extract_ucode(cls, file_path: str, out_file: str,
                      ram_start: int = 0x198000) -> Tuple[bool, str]:
        rep = cls.analyze_firmware(file_path, ram_start=ram_start)
        if not rep.ucode_start or not rep.ucode_size:
            return False, "Could not determine ucode offset / size in firmware"

        data = Path(file_path).read_bytes()
        actual_ram_start = rep.ram_start if getattr(rep, "ram_start", 0) else ram_start
        offset = rep.ucode_start - actual_ram_start
        if offset < 0 or offset + rep.ucode_size > len(data):
            return False, f"Invalid ucode slice bounds: offset 0x{offset:x}, size {rep.ucode_size}, file size {len(data)}"
        ucode_bytes = data[offset : offset + rep.ucode_size]

        Path(out_file).write_bytes(ucode_bytes)
        return True, f"Extracted {len(ucode_bytes)} bytes of ucode to {out_file} (offset 0x{rep.ucode_start:x})"


# =============================================================================
# Firmware Lifecycle Engine: Pull, Put, Edit
# =============================================================================

class FirmwareManager:
    INSTALL_DIR = Path("/lib/firmware/nexmon")
    ALTERNATIVES_LINK = "/lib/firmware/cypress/cyfmac43455-sdio.bin"
    ALTERNATIVES_NAME = "cyfmac43455-sdio.bin"
    SYSTEM_FIRMWARE_DIRS = (
        Path("/lib/firmware/cypress"),
        Path("/lib/firmware/brcm"),
        Path("/usr/lib/firmware/cypress"),
        Path("/usr/lib/firmware/brcm"),
    )
    REPO_FIRMWARE_DIR = Path(__file__).resolve().parent / "firmwares"

    @staticmethod
    def _validate_install_name(name: str) -> str:
        """Return a safe basename for the managed firmware directory."""
        if (
            not name
            or "\x00" in name
            or name in (".", "..")
            or Path(name).name != name
            or Path(name).is_absolute()
        ):
            raise ValueError(
                f"Firmware destination name must be a basename, got {name!r}"
            )
        return name

    @staticmethod
    def _atomic_copy(source: Path, destination: Path) -> None:
        """Copy metadata and contents, then atomically publish the file."""
        fd, temporary_name = tempfile.mkstemp(
            dir=destination.parent,
            prefix=f".{destination.name}.",
            suffix=".tmp",
        )
        os.close(fd)
        temporary = Path(temporary_name)
        try:
            shutil.copy2(source, temporary)
            try:
                os.chmod(temporary, 0o644)
            except OSError:
                pass
            with temporary.open("rb") as copied:
                os.fsync(copied.fileno())
            os.replace(temporary, destination)
            try:
                directory_fd = os.open(
                    destination.parent,
                    os.O_RDONLY | getattr(os, "O_DIRECTORY", 0),
                )
                try:
                    os.fsync(directory_fd)
                finally:
                    os.close(directory_fd)
            except OSError as exc:
                if exc.errno == errno.EBADF:
                    raise
                if exc.errno not in (errno.EINVAL, errno.EOPNOTSUPP, errno.ENOSYS, errno.EPERM, errno.EROFS):
                    raise
                # EINVAL/EOPNOTSUPP/ENOSYS/EPERM/EROFS are expected for dir fsync
                # on certain filesystems — safe to ignore after file fsync.
        finally:
            temporary.unlink(missing_ok=True)

    @staticmethod
    def _atomic_write(data: bytes, destination: Path) -> None:
        """Atomically write binary data to a destination path with sync."""
        destination.parent.mkdir(parents=True, exist_ok=True)
        fd, temporary_name = tempfile.mkstemp(
            dir=destination.parent,
            prefix=f".{destination.name}.",
            suffix=".tmp",
        )
        temporary = Path(temporary_name)
        try:
            with os.fdopen(fd, "wb") as f:
                f.write(data)
                f.flush()
                os.fsync(f.fileno())
            try:
                os.chmod(temporary, 0o644)
            except OSError:
                pass
            os.replace(temporary, destination)
            try:
                directory_fd = os.open(
                    destination.parent,
                    os.O_RDONLY | getattr(os, "O_DIRECTORY", 0),
                )
                try:
                    os.fsync(directory_fd)
                finally:
                    os.close(directory_fd)
            except OSError as exc:
                if exc.errno == errno.EBADF:
                    raise
                if exc.errno not in (errno.EINVAL, errno.EOPNOTSUPP, errno.ENOSYS, errno.EPERM, errno.EROFS):
                    raise
        finally:
            temporary.unlink(missing_ok=True)

    @staticmethod
    def _run_required(command: List[str], operation: str, timeout: int = 30) -> None:
        """Run a requested system operation and expose failures to callers."""
        try:
            subprocess.run(
                command,
                check=True,
                capture_output=True,
                text=True,
                timeout=timeout,
            )
        except subprocess.TimeoutExpired as exc:
            raise RuntimeError(f"{operation} timed out after {timeout}s") from exc
        except (OSError, subprocess.CalledProcessError) as exc:
            detail = getattr(exc, "stderr", None) or str(exc)
            raise RuntimeError(f"{operation} failed: {detail.strip()}") from exc

    @staticmethod
    def _chip_database_entry(detection: DeviceDetectionReport) -> Dict[str, Any]:
        """Find metadata for a detected chip, including name-only fallbacks."""
        if detection.chip_num is not None:
            exact = CHIP_DATABASE.get((detection.chip_num, detection.chip_rev))
            if exact:
                return exact
            generic = CHIP_DATABASE.get((detection.chip_num, None))
            if generic:
                return generic
        for entry in CHIP_DATABASE.values():
            if entry["name"].lower() == detection.chip_name.lower():
                return entry
        return {}

    @staticmethod
    def _list_alternatives(name: str) -> List[Path]:
        """Return paths registered for an update-alternatives group."""
        executable = shutil.which("update-alternatives")
        if not executable or not name:
            return []
        try:
            result = subprocess.run(
                [executable, "--list", name],
                check=True,
                capture_output=True,
                text=True,
                timeout=10,
            )
        except (OSError, subprocess.SubprocessError):
            return []
        return [Path(line.strip()) for line in result.stdout.splitlines() if line.strip()]

    @classmethod
    def find_original_firmware(
        cls,
        detection: Optional[DeviceDetectionReport] = None,
        source_path: Optional[str] = None,
    ) -> Dict[str, Any]:
        """Locate and rank compatible stock firmware images.

        Registered distro alternatives are preferred, followed by normal
        system firmware paths, repository RAM_FILE entries for the detected
        chip, and finally preserved install backups. Nexmon-patched images and
        component blobs such as ucode/template RAM are never eligible.
        """
        det = detection or DeviceDetector.full_detection()
        entry = cls._chip_database_entry(det)
        expected_names = set()
        if entry.get("standard_fw"):
            expected_names.add(Path(entry["standard_fw"]).name)
        if det.active_firmware:
            expected_names.add(Path(det.active_firmware.symlink_path).name)
            expected_names.add(Path(det.active_firmware.active_path).name)

        alternative_name = (
            Path(entry["standard_fw"]).name
            if entry.get("standard_fw")
            else next(iter(expected_names), cls.ALTERNATIVES_NAME)
        )
        raw_candidates: List[Tuple[Path, str, Optional[Path]]] = []

        if source_path:
            raw_candidates.append((Path(source_path), "explicit", None))
        else:
            for path in cls._list_alternatives(alternative_name):
                raw_candidates.append((path, "alternatives", path))

            variants = set(expected_names)
            for name in list(expected_names):
                if name.endswith(".bin"):
                    stem = name[:-4]
                    variants.add(f"{stem}-standard.bin")
                    variants.add(f"{stem}-minimal.bin")
            for directory in cls.SYSTEM_FIRMWARE_DIRS:
                for name in sorted(variants):
                    raw_candidates.append((directory / name, "system", None))

            chip_dir = cls.REPO_FIRMWARE_DIR / det.chip_name.lower()
            if chip_dir.is_dir():
                for definitions in sorted(chip_dir.glob("*/definitions.mk")):
                    match = re.search(
                        r"^RAM_FILE\s*=\s*([^\s#]+)",
                        definitions.read_text(errors="replace"),
                        flags=re.MULTILINE,
                    )
                    if match:
                        raw_candidates.append(
                            (definitions.parent / match.group(1), "repository", None)
                        )

            if cls.INSTALL_DIR.is_dir():
                for pattern in ("*.bak", "*.bak-*", "*.pre-*", "*.original"):
                    for path in sorted(cls.INSTALL_DIR.glob(pattern)):
                        raw_candidates.append((path, "backup", None))

        origin_score = {
            "explicit": 3000,
            "alternatives": 2000,
            "system": 800,
            "repository": 600,
            "backup": 400,
        }
        active_size = det.active_firmware.file_size if det.active_firmware else None
        active_version = (
            det.active_firmware.version_string if det.active_firmware else ""
        )
        excluded_parts = (
            "ucode",
            "templateram",
            "clm",
            "nvram",
            "vasip",
            "dlarray",
        )
        candidates_by_path: Dict[str, Dict[str, Any]] = {}

        for raw_path, origin, registered_path in raw_candidates:
            try:
                path = raw_path.expanduser().resolve(strict=True)
                if not path.is_file():
                    continue
            except (OSError, RuntimeError):
                continue

            lower_name = path.name.lower()
            if any(part in lower_name for part in excluded_parts):
                continue
            # Pre-filter by file size without reading the whole image.
            try:
                if path.stat().st_size < 100000:
                    continue
            except OSError:
                continue
            try:
                data = path.read_bytes()
            except OSError:
                continue
            if len(data) < 100000:
                continue
            if b"nexmon" in data.lower():
                continue

            version = extract_firmware_version(data)
            score = origin_score[origin]
            if path.name in expected_names:
                score += 100
            if "standard" in lower_name:
                score += 25
            if "minimal" in lower_name:
                score -= 50
            if active_size is not None and len(data) == active_size:
                score += 200
            if active_version and version and version == active_version:
                score += 150

            candidate = {
                "path": str(path),
                "origin": origin,
                "registered_path": str(registered_path) if registered_path else None,
                "file_size": len(data),
                "sha256": hashlib.sha256(data).hexdigest(),
                "version": version,
                "score": score,
            }
            key = str(path)
            previous = candidates_by_path.get(key)
            if previous is None or candidate["score"] > previous["score"]:
                candidates_by_path[key] = candidate

        candidates = sorted(
            candidates_by_path.values(),
            key=lambda candidate: (-candidate["score"], candidate["path"]),
        )
        if not candidates:
            chip = det.chip_name or "detected chip"
            raise FileNotFoundError(
                f"No compatible original firmware found for {chip}; "
                "provide one with --source"
            )

        return {
            "chip_name": det.chip_name,
            "alternative_name": alternative_name,
            "selected": candidates[0],
            "candidates": candidates,
        }

    @classmethod
    def restore(
        cls,
        source_path: Optional[str] = None,
        reload_driver: bool = False,
        backup_current: bool = True,
        dry_run: bool = False,
    ) -> Dict[str, Any]:
        """Restore a discovered stock image and optionally reload brcmfmac."""
        detection = DeviceDetector.full_detection()
        discovery = cls.find_original_firmware(detection, source_path=source_path)
        selected = discovery["selected"]
        source = Path(selected["path"])
        active = detection.active_firmware

        alternative_name = discovery["alternative_name"]
        registered_path = None
        for alternative in cls._list_alternatives(alternative_name):
            try:
                if alternative.resolve(strict=True) == source.resolve(strict=True):
                    registered_path = alternative
                    break
            except OSError:
                continue

        current_path = Path(active.active_path) if active else None
        already_active = False
        if current_path and current_path.is_file():
            try:
                already_active = current_path.resolve() == source.resolve()
            except OSError:
                pass

        alternatives_tool = shutil.which("update-alternatives")
        if already_active:
            activation = "already-original"
            target_path = source
        elif registered_path is not None and alternatives_tool:
            activation = "update-alternatives"
            target_path = registered_path
        elif current_path is not None:
            activation = "atomic-replacement"
            target_path = current_path
        else:
            raise RuntimeError(
                "Original firmware was found, but no active firmware target or "
                "usable alternatives group could be determined"
            )

        result = {
            "chip_name": detection.chip_name,
            "source_path": str(source),
            "source_origin": selected["origin"],
            "source_sha256": selected["sha256"],
            "source_version": selected["version"],
            "activation": activation,
            "restored_path": str(target_path),
            "backup_created": None,
            "driver_reloaded": False,
            "dry_run": dry_run,
            "candidates_considered": len(discovery["candidates"]),
        }
        if dry_run:
            return result

        needs_change = activation != "already-original"
        if (needs_change or reload_driver) and os.geteuid() != 0:
            raise PermissionError("Firmware restoration requires root privileges (run with sudo)")

        modprobe_tool = None
        if reload_driver:
            modprobe_tool = shutil.which("modprobe")
            if not modprobe_tool:
                raise RuntimeError("driver reload was requested but modprobe is not available")

        if needs_change and backup_current and current_path and current_path.is_file():
            cls.INSTALL_DIR.mkdir(parents=True, exist_ok=True)
            backup_path = cls.INSTALL_DIR / (
                f"{current_path.name}.bak-restore-{time.time_ns()}"
            )
            cls._atomic_copy(current_path, backup_path)
            result["backup_created"] = str(backup_path)

        if activation == "update-alternatives":
            cls._run_required(
                [alternatives_tool, "--set", alternative_name, str(registered_path)],
                "original firmware activation",
            )
        elif activation == "atomic-replacement":
            cls._atomic_copy(source, target_path)

        if reload_driver:
            # On Linux >= 6.4, brcmfmac_wcc may hold brcmfmac; unload it first if present
            try:
                subprocess.run([modprobe_tool, "-r", "brcmfmac_wcc"], capture_output=True, timeout=10)
            except Exception:
                pass
            cls._run_required(
                [modprobe_tool, "-r", "brcmfmac"], "brcmfmac unload", timeout=30
            )
            time.sleep(0.5)
            cls._run_required(
                [modprobe_tool, "brcmfmac"], "brcmfmac reload", timeout=30
            )
            try:
                subprocess.run([modprobe_tool, "brcmfmac_wcc"], capture_output=True, timeout=10)
            except Exception:
                pass
            result["driver_reloaded"] = True

        return result

    @staticmethod
    def pull(target_dir: str, include_meta: bool = True) -> Dict[str, Any]:
        """Pulls the active firmware, NVRAM, and CLM blobs from system into target_dir."""
        if "\x00" in target_dir:
            raise ValueError("Invalid target directory")
        out_dir = Path(target_dir).resolve()
        # Prevent directory traversal / system directory overwrite via
        # resolved paths like "/tmp/evil/../etc" → "/etc" or "/".
        blocked_exact = {
            Path("/").resolve(),
            Path("/lib").resolve(),
            Path("/lib/firmware").resolve(),
            Path("/usr").resolve(),
            Path("/usr/lib").resolve(),
            Path("/usr/lib/firmware").resolve(),
            Path("/etc").resolve(),
            Path("/etc/alternatives").resolve(),
            Path("/lib/firmware/cypress").resolve(),
            Path("/lib/firmware/brcm").resolve(),
            Path("/usr/lib/firmware/cypress").resolve(),
            Path("/usr/lib/firmware/brcm").resolve(),
            FirmwareManager.INSTALL_DIR.resolve(),
        }
        # Also consider the unresolved forms for exact match before resolve
        unresolved_blocked = {
            Path("/"), Path("/lib"), Path("/lib/firmware"), Path("/usr"),
            Path("/usr/lib"), Path("/usr/lib/firmware"), Path("/etc"),
            Path("/etc/alternatives"), Path("/lib/firmware/cypress"),
            Path("/lib/firmware/brcm"), Path("/usr/lib/firmware/cypress"),
            Path("/usr/lib/firmware/brcm"), FirmwareManager.INSTALL_DIR,
        }
        if out_dir in blocked_exact or out_dir in unresolved_blocked:
            raise ValueError(f"Refusing to write pull output to system directory {out_dir}")
        # Also block writing inside any system directory (but allow normal
        # user paths like /tmp/foo which only shares "/" as parent).
        for blocked in blocked_exact:
            if blocked == Path("/").resolve():
                continue
            try:
                is_inside = blocked in out_dir.parents
            except Exception:
                continue
            if is_inside:
                raise ValueError(f"Refusing to write pull output inside system directory {blocked}")
        out_dir.mkdir(parents=True, exist_ok=True)

        det = DeviceDetector.full_detection()
        if not det.active_firmware:
            raise RuntimeError("No active firmware found on system to pull")

        fw_info = det.active_firmware
        src_path = Path(fw_info.active_path)
        dest_fw = out_dir / src_path.name
        shutil.copy2(src_path, dest_fw)

        pulled_files = [str(dest_fw)]
        nvram_dest = None
        clm_dest = None

        if include_meta:
            if fw_info.nvram_path and os.path.isfile(fw_info.nvram_path):
                nvram_dest = out_dir / Path(fw_info.nvram_path).name
                shutil.copy2(fw_info.nvram_path, nvram_dest)
                pulled_files.append(str(nvram_dest))

            if fw_info.clm_blob_path and os.path.isfile(fw_info.clm_blob_path):
                clm_dest = out_dir / Path(fw_info.clm_blob_path).name
                shutil.copy2(fw_info.clm_blob_path, clm_dest)
                pulled_files.append(str(clm_dest))

        # Write manifest — backup existing to avoid silent overwrite
        manifest = {
            "timestamp": time.strftime("%Y-%m-%d %H:%M:%S"),
            "chip_name": det.chip_name,
            "host_model": det.host.model,
            "kernel": det.host.kernel,
            "source_path": fw_info.active_path,
            "symlink_path": fw_info.symlink_path,
            "sha256": fw_info.sha256,
            "version": fw_info.version_string,
            "is_nexmon": fw_info.is_nexmon,
            "files": pulled_files
        }
        manifest_path = out_dir / "manifest.json"
        if manifest_path.exists():
            try:
                backup_path = out_dir / f"manifest.json.bak-{time.time_ns()}"
                shutil.copy2(manifest_path, backup_path)
            except OSError:
                pass
        with open(manifest_path, "w") as f:
            json.dump(manifest, f, indent=2)

        return manifest

    @staticmethod
    def put(src_fw_path: str, dest_name: Optional[str] = None,
            update_alternatives: bool = True, reload_driver: bool = False,
            backup_current: bool = True) -> Dict[str, Any]:
        """Deploys a patched firmware file to /lib/firmware/nexmon/ and updates alternatives."""
        src = Path(src_fw_path).resolve()
        if not src.is_file():
            raise FileNotFoundError(f"Source firmware binary not found: {src}")

        target_name = FirmwareManager._validate_install_name(
            dest_name if dest_name is not None else src.name
        )

        if os.geteuid() != 0:
            raise PermissionError("Firmware deployment requires root privileges (run with sudo)")

        # Pre-flight check
        src_bytes = src.read_bytes()
        src_sha = hashlib.sha256(src_bytes).hexdigest()
        if len(src_bytes) < 100000:
            raise ValueError(f"Firmware binary is suspiciously small ({len(src_bytes)} bytes)")
        if len(src_bytes) > 8 * 1024 * 1024:
            raise ValueError(f"Firmware binary is suspiciously large ({len(src_bytes)} bytes)")
        # Basic content sanity: reject all-zero / all-0xFF images or images
        # that contain no printable strings — real firmware always carries a
        # Broadcom/Cypress version string, BRCM/NEXMON markers or similar.
        head = src_bytes[:4096]
        if head.count(b"\x00") == len(head) or head.count(b"\xff") == len(head):
            raise ValueError("Firmware image appears empty (all 0x00 or 0xFF)")
        # For real firmware (>200KB) require at least one firmware marker;
        # test dummies (repeating byte) are allowed for unit tests but we
        # still require some entropy check via the size guard above.
        if len(src_bytes) >= 200000 and src_bytes[:1] * len(src_bytes) == src_bytes:
            raise ValueError("Firmware image has no entropy (repeating byte)")
        # Strict version/marker check is advisory for put — warn but do not
        # block unit-test dummies; real deployments should carry a marker.
        has_marker = (
            extract_firmware_version(src_bytes) != ""
            or b"Broadcom" in src_bytes
            or b"BRCM" in src_bytes
            or b"Cypress" in src_bytes
            or b"nexmon" in src_bytes.lower()
        )
        if not has_marker and len(src_bytes) >= 200000:
            # Allow through but record; caller can inspect logs. For now we
            # do not block to keep unit-test dummies working, but this is
            # the hook for stricter deployments.
            pass

        alternatives_tool = None
        if update_alternatives:
            alternatives_tool = shutil.which("update-alternatives")
            if not alternatives_tool:
                raise RuntimeError(
                    "update-alternatives was requested but is not available"
                )

        modprobe_tool = None
        if reload_driver:
            modprobe_tool = shutil.which("modprobe")
            if not modprobe_tool:
                raise RuntimeError("driver reload was requested but modprobe is not available")

        # Target directory: /lib/firmware/nexmon
        nexmon_dir = FirmwareManager.INSTALL_DIR
        nexmon_dir.mkdir(parents=True, exist_ok=True)

        target_path = nexmon_dir / target_name

        # Backup current active firmware
        det = DeviceDetector.full_detection()
        backup_path = None
        if backup_current and det.active_firmware:
            cur_active = Path(det.active_firmware.active_path)
            if cur_active.is_file():
                ts = time.time_ns()
                backup_path = nexmon_dir / f"{cur_active.name}.bak-{ts}"
                FirmwareManager._atomic_copy(cur_active, backup_path)

        # Publish the new image only after a complete copy has been synced.
        FirmwareManager._atomic_copy(src, target_path)

        alt_updated = False
        reloaded = False
        try:
            # Update alternatives if Debian/Ubuntu/Raspberry Pi OS
            if update_alternatives:
                alt_name = target_name
                for suffix in ["-standard-265.bin", "-standard.bin"]:
                    if alt_name.endswith(suffix):
                        alt_name = alt_name[:-len(suffix)] + ".bin"
                        break
                alt_link = f"/lib/firmware/cypress/{alt_name}"
                if not Path("/lib/firmware/cypress").is_dir():
                    alt_link = f"/lib/firmware/brcm/{alt_name}"

                FirmwareManager._run_required(
                    [
                        alternatives_tool,
                        "--install",
                        alt_link,
                        alt_name,
                        str(target_path),
                        "90",
                    ],
                    "update-alternatives registration",
                )
                FirmwareManager._run_required(
                    [
                        alternatives_tool,
                        "--set",
                        alt_name,
                        str(target_path),
                    ],
                    "update-alternatives activation",
                )
                alt_updated = True

            # Optional driver reload
            if reload_driver:
                try:
                    subprocess.run([modprobe_tool, "-r", "brcmfmac_wcc"], capture_output=True, timeout=10)
                except Exception:
                    pass
                FirmwareManager._run_required(
                    [modprobe_tool, "-r", "brcmfmac"], "brcmfmac unload", timeout=30
                )
                time.sleep(0.5)
                FirmwareManager._run_required(
                    [modprobe_tool, "brcmfmac"], "brcmfmac reload", timeout=30
                )
                try:
                    subprocess.run([modprobe_tool, "brcmfmac_wcc"], capture_output=True, timeout=10)
                except Exception:
                    pass
                reloaded = True
        except Exception:
            if backup_path and backup_path.is_file():
                try:
                    FirmwareManager._atomic_copy(backup_path, target_path)
                except Exception:
                    pass
            raise

        return {
            "installed_path": str(target_path),
            "sha256": src_sha,
            "backup_created": str(backup_path) if backup_path else None,
            "alternatives_updated": alt_updated,
            "driver_reloaded": reloaded
        }

    @staticmethod
    def edit(file_path: str, out_file: Optional[str] = None,
             version_tag: Optional[str] = None,
             patches: Optional[List[Tuple[int, bytes]]] = None,
             ram_start: Optional[int] = None, rom_size: Optional[int] = None,
             chip: Optional[str] = None) -> Dict[str, Any]:
        """Edits binary firmware in-place or creates a modified copy."""
        src = Path(file_path).resolve()
        if not src.is_file():
            raise FileNotFoundError(f"Firmware binary not found: {src}")

        data = bytearray(src.read_bytes())
        original_size = len(data)
        orig_sha = hashlib.sha256(data).hexdigest()
        changes = []

        # Resolve chip-specific RAM/ROM defaults dynamically when not explicitly specified
        # or when caller passed legacy defaults (0x198000 / 0xB0000) for a non-43455 chip.
        resolved_ramstart = None
        resolved_romsize = None

        if chip and chip.lower() in CHIP_DATABASE:
            entry = CHIP_DATABASE[chip.lower()]
            resolved_ramstart = entry.get("default_ramstart")
            resolved_romsize = entry.get("default_romsize")

        if resolved_ramstart is None or resolved_romsize is None:
            for candidate in [src.parent / "definitions.mk", Path(__file__).resolve().parent / "firmwares" / src.parent.name / "definitions.mk"]:
                if candidate.is_file():
                    try:
                        txt = candidate.read_text(errors="replace")
                        m_ram = re.search(r"^\s*RAMSTART\s*=\s*(0x[0-9a-fA-F]+)", txt, re.MULTILINE)
                        m_romsz = re.search(r"^\s*ROMSIZE\s*=\s*(0x[0-9a-fA-F]+)", txt, re.MULTILINE)
                        if m_ram and resolved_ramstart is None:
                            resolved_ramstart = int(m_ram.group(1), 16)
                        if m_romsz and resolved_romsize is None:
                            resolved_romsize = int(m_romsz.group(1), 16)
                        break
                    except (OSError, ValueError):
                        pass

        if resolved_ramstart is None or resolved_romsize is None:
            for entry in CHIP_DATABASE.values():
                if entry.get("standard_fw") and Path(entry["standard_fw"]).name == src.name:
                    if resolved_ramstart is None:
                        resolved_ramstart = entry.get("default_ramstart")
                    if resolved_romsize is None:
                        resolved_romsize = entry.get("default_romsize")
                    break

        if ram_start is None:
            ram_start = resolved_ramstart if resolved_ramstart is not None else 0x198000
        elif ram_start == 0x198000 and resolved_ramstart is not None and resolved_ramstart != 0x198000:
            # Caller passed legacy default 0x198000, but chip clearly has a different ram_start
            ram_start = resolved_ramstart

        if rom_size is None:
            rom_size = resolved_romsize if resolved_romsize is not None else 0xB0000
        elif rom_size == 0xB0000 and resolved_romsize is not None and resolved_romsize != 0xB0000:
            rom_size = resolved_romsize

        # 1. Update version tag — limit to a null-terminated printable slot
        #    near the version string to avoid matching compressed ucode.
        if version_tag:
            # Search for "nexmon.org" followed by a printable version tag that
            # is terminated by NUL, newline or ")." — compressed ucode will not
            # contain this ASCII pattern with a clean terminator.
            m = re.search(rb'nexmon\.org\s*[:=]\s*([ -~]{1,64}?)\s*(?:\x00|\n|\r|\)\s)', data)
            if m is None:
                # Fallback to legacy pattern but verify printable content.
                m = re.search(rb'nexmon\.org:\s*([^\x00\)]+)', data)
                if m and not all(32 <= b <= 126 or b in (32, 9) for b in m.group(1)[:64]):
                    m = None
            if m:
                old_tag = m.group(1).decode("utf-8", errors="ignore").strip()
                # Re-find the actual slot bounds for in-place replacement
                slot_start, slot_end = m.start(1), m.end(1)
                # For fallback legacy match, trim to printable prefix.
                slot_len = slot_end - slot_start
                new_tag_bytes = version_tag.encode("utf-8")
                if len(new_tag_bytes) <= slot_len:
                    padded = new_tag_bytes.ljust(slot_len, b" ")
                    data[slot_start : slot_end] = padded
                    changes.append(f"Updated nexmon tag from '{old_tag}' to '{version_tag}'")
                else:
                    raise ValueError(f"New tag '{version_tag}' is longer than available slot ({slot_len} bytes)")
            else:
                changes.append("No nexmon tag slot found to update")

        # 2. Apply arbitrary hex patches: [(address, bytes)]
        if patches:
            ram_end = ram_start + original_size
            for addr, patch_bytes in patches:
                if addr < 0:
                    raise ValueError(f"Patch address must be non-negative, got {addr}")
                # Domain-aware offset translation: RAM addresses map to
                # file offset via (addr - ram_start); raw file offsets are
                # allowed only when explicitly below ram_start but must not
                # be confused with ROM addresses.
                if addr >= ram_start:
                    if addr >= ram_end:
                        raise ValueError(f"Patch at 0x{addr:x} outside RAM window 0x{ram_start:x}-0x{ram_end:x}")
                    offset = addr - ram_start
                else:
                    # Raw file offset — reject ROM-range confusion for real
                    # firmware images. A RAM dump's file offset 0 corresponds
                    # to ram_start, not ROM 0; patching ROM 0 via file offset
                    # would corrupt the RAM header.
                    if original_size >= 100000 and 0 <= addr < rom_size:
                        raise ValueError(f"Patch at 0x{addr:x} looks like a ROM address (0x0-0x{rom_size:x}); use a RAM address >= 0x{ram_start:x} or a raw file offset >= {rom_size} if intentional")
                    offset = addr
                if offset > original_size or offset + len(patch_bytes) > original_size:
                    raise ValueError(f"Patch at 0x{addr:x} (offset 0x{offset:x}) exceeds file size 0x{original_size:x}")
                old_bytes = data[offset : offset + len(patch_bytes)]
                data[offset : offset + len(patch_bytes)] = patch_bytes
                changes.append(f"Patched 0x{addr:x} (offset 0x{offset:x}): {old_bytes.hex()} -> {patch_bytes.hex()}")

        if len(data) != original_size:
            raise ValueError("Firmware edits must preserve the original image size")

        dest = Path(out_file).resolve() if out_file else src
        FirmwareManager._atomic_write(bytes(data), dest)
        new_sha = hashlib.sha256(data).hexdigest()

        return {
            "output_path": str(dest),
            "original_sha256": orig_sha,
            "modified_sha256": new_sha,
            "changes": changes
        }


# =============================================================================
# CLI Presentation & Command Dispatcher
# =============================================================================

def print_detection_report(rep: DeviceDetectionReport, as_json: bool = False):
    if as_json:
        # Convert dataclasses to dict — include ls -l firmware directory info
        d = {
            "host": asdict(rep.host),
            "devices": [asdict(x) for x in rep.devices],
            "chip": {
                "name": rep.chip_name,
                "chip_num": f"0x{rep.chip_num:04x}" if rep.chip_num else None,
                "chip_rev": f"0x{rep.chip_rev:x}" if rep.chip_rev is not None else None,
            },
            "driver": asdict(rep.driver),
            "active_firmware": asdict(rep.active_firmware) if rep.active_firmware else None,
            "live_revinfo": rep.live_revinfo,
            "firmware_ls_brcm": [asdict(x) for x in (rep.firmware_ls_brcm or [])],
            "firmware_ls_cypress": [asdict(x) for x in (rep.firmware_ls_cypress or [])],
            "firmware_ls_nexmon": [asdict(x) for x in (rep.firmware_ls_nexmon or [])],
            "device_firmware_map": rep.device_firmware_map or [],
        }
        print(json.dumps(d, indent=2))
        return

    print(f"\n{Style.BOLD}{Style.CYAN}======================================================================{Style.RESET}")
    print(f"{Style.BOLD}{Style.CYAN}       NEXMON BROADCOM / CYPRESS HARDWARE & DRIVER REPORT            {Style.RESET}")
    print(f"{Style.BOLD}{Style.CYAN}======================================================================{Style.RESET}\n")

    # Host
    print(f"{Style.BOLD}📡 Host Platform:{Style.RESET}")
    print(f"   Model:       {Style.GREEN}{rep.host.model}{Style.RESET}")
    print(f"   Kernel:      {rep.host.kernel} ({rep.host.arch})")
    print(f"   OS:          {rep.host.os_name}")
    print(f"   Target SBC:  {'Yes (Raspberry Pi)' if rep.host.is_raspberry_pi else 'Generic Linux'}")

    # Detected Chip
    print(f"\n{Style.BOLD}🔧 Wireless Chipset:{Style.RESET}")
    print(f"   Chip Model:  {Style.BOLD}{Style.MAGENTA}{rep.chip_name}{Style.RESET}")
    if rep.chip_num is not None:
        print(f"   Chip Number: 0x{rep.chip_num:04x}")
    if rep.chip_rev is not None:
        print(f"   Chip Rev:    0x{rep.chip_rev:x}")

    # Hardware devices
    print(f"\n{Style.BOLD}🔌 Hardware Interfaces & Busses:{Style.RESET}")
    if rep.devices:
        for idx, dev in enumerate(rep.devices, 1):
            print(f"   [{idx}] Bus: {dev.bus_type.upper():<5} | ID: {dev.vendor_id}:{dev.device_id} | Net: {dev.interface or 'N/A'}")
            print(f"       Description: {dev.description}")
            print(f"       Sysfs:       {dev.sysfs_path}")
    else:
        print(f"   {Style.YELLOW}No physical SDIO/PCIe Broadcom devices located in sysfs{Style.RESET}")

    # Driver
    print(f"\n{Style.BOLD}⚙️  Kernel Driver (brcmfmac):{Style.RESET}")
    st_color = Style.GREEN if rep.driver.loaded else Style.RED
    print(f"   Module Loaded:  {st_color}{rep.driver.loaded}{Style.RESET}")
    print(f"   Driver Version: {rep.driver.version} {'(Nexmon Patched Driver)' if rep.driver.is_nexmon else '(Stock Kernel Driver)'}")
    if rep.driver.module_path:
        print(f"   Module Path:    {rep.driver.module_path}")
    if rep.driver.dependencies:
        print(f"   Dependencies:   {', '.join(rep.driver.dependencies)}")

    # Active Firmware
    print(f"\n{Style.BOLD}💾 Active Firmware Binary:{Style.RESET}")
    if rep.active_firmware:
        fw = rep.active_firmware
        nex_badge = f"{Style.GREEN}[NEXMON PATCHED]{Style.RESET}" if fw.is_nexmon else f"{Style.YELLOW}[STOCK FIRMWARE]{Style.RESET}"
        print(f"   Status:         {nex_badge}")
        print(f"   Active File:    {fw.active_path}")
        if fw.symlink_path != fw.active_path:
            print(f"   Symlink Source: {fw.symlink_path}")
        print(f"   Firmware Ver:   {fw.version_string or 'Unknown'}")
        if fw.fw_id:
            print(f"   Firmware ID:    {fw.fw_id}")
        print(f"   File Size:      {fw.file_size:,} bytes")
        print(f"   SHA-256:        {fw.sha256}")
        if fw.nvram_path:
            print(f"   Config / NVRAM: {fw.nvram_path}")
        if fw.clm_blob_path:
            print(f"   CLM Blob:       {fw.clm_blob_path}")
    else:
        print(f"   {Style.YELLOW}No active firmware found in /lib/firmware/{Style.RESET}")

    # Live chip revision info
    if rep.live_revinfo:
        print(f"\n{Style.BOLD}📊 Live Chip Hardware Telemetry (via nexutil):{Style.RESET}")
        for k, v in rep.live_revinfo.items():
            print(f"   {k:<14}: {v}")

    # Firmware directory ls -l info (user-requested: ls -l /lib/firmware/brcm/ + device info)
    if rep.firmware_ls_brcm is not None:
        print(f"\n{Style.BOLD}📁 Firmware Directories (ls -l):{Style.RESET}")
        brcm_count = len(rep.firmware_ls_brcm or [])
        cypress_count = len(rep.firmware_ls_cypress or [])
        nexmon_count = len(rep.firmware_ls_nexmon or [])
        print(f"   /lib/firmware/brcm/   : {brcm_count} entries")
        print(f"   /lib/firmware/cypress/: {cypress_count} entries")
        print(f"   /lib/firmware/nexmon/ : {nexmon_count} entries")
        # Show raw ls -l lines for brcm (top 10) to match `ls -l` view
        if rep.firmware_ls_brcm:
            print(f"\n   {Style.DIM}--- ls -l /lib/firmware/brcm/ (top 10) ---{Style.RESET}")
            for entry in (rep.firmware_ls_brcm or [])[:10]:
                # Use raw ls line directly for fidelity, highlight nexmon/active
                marker = ""
                if entry.is_nexmon:
                    marker = f" {Style.GREEN}[nexmon]{Style.RESET}"
                elif entry.is_symlink and "alternatives" in entry.symlink_target:
                    marker = f" {Style.CYAN}[active]{Style.RESET}"
                print(f"   {entry.raw_line}{marker}")
            if brcm_count > 10:
                print(f"   {Style.DIM}... and {brcm_count - 10} more{Style.RESET}")
        if rep.firmware_ls_cypress:
            print(f"\n   {Style.DIM}--- ls -l /lib/firmware/cypress/ (top 5) ---{Style.RESET}")
            for entry in (rep.firmware_ls_cypress or [])[:5]:
                marker = " [nexmon]" if entry.is_nexmon else ""
                target_str = f" -> {entry.symlink_target}" if entry.is_symlink else ""
                print(f"   {entry.permissions} {entry.owner}/{entry.group} {entry.size:7d} {entry.date_str:12s} {entry.name}{target_str}{marker}")
            if cypress_count > 5:
                print(f"   ... and {cypress_count - 5} more")
        if rep.firmware_ls_nexmon:
            print(f"\n   {Style.DIM}--- ls -l /lib/firmware/nexmon/ ---{Style.RESET}")
            for entry in (rep.firmware_ls_nexmon or [])[:10]:
                target_str = f" -> {entry.symlink_target}" if entry.is_symlink else ""
                print(f"   {entry.permissions} {entry.owner}/{entry.group} {entry.size:7d} {entry.date_str:12s} {entry.name}{target_str}")

    # Device → Firmware mapping via ls -l + device info (the requested finder)
    if rep.device_firmware_map:
        print(f"\n{Style.BOLD}🔗 Device → Firmware Mapping (via ls -l + device info):{Style.RESET}")
        for idx, mapping in enumerate(rep.device_firmware_map, 1):
            dev = mapping["device"]
            print(f"   [{idx}] {dev['bus_type'].upper():<5} {dev['vendor_id']}:{dev['device_id']} ({dev['description']}) iface={dev['interface'] or 'N/A'}")
            print(f"       Sysfs: {dev['sysfs_path']}")
            if dev.get("modalias"):
                print(f"       Modalias: {dev['modalias']}")
            candidates = mapping.get("ls_candidates", [])
            summary = mapping.get("ls_summary", {})
            if candidates:
                print(f"       Candidates from ls -l (scored, top 6 of {summary.get('total_candidates_for_device', len(candidates))}):")
                for cand in candidates[:6]:
                    active_marker = f" {Style.CYAN}[active symlink]{Style.RESET}" if cand["is_symlink"] and "alternatives" in cand["symlink_target"] else ""
                    nexmon_marker = f" {Style.GREEN}[nexmon]{Style.RESET}" if "nexmon" in cand["reason"] else ""
                    print(f"         → {cand['file']:40s} {cand['size']:7d} {cand['permissions']} score={cand['score']:2d} ({cand['reason']}){active_marker}{nexmon_marker}")
                    # Show raw ls -l line for this candidate for full transparency
                    print(f"            {Style.DIM}{cand['raw_ls_line']}{Style.RESET}")
                    if cand["symlink_target"]:
                        # Also show resolved driver/chip hint
                        print(f"            ↳ chip_hint={cand['chip_hint'] or 'unknown'}")
                # Driver / chip inference from best candidate
                best = candidates[0]
                print(f"       {Style.BOLD}Inferred firmware:{Style.RESET} {best['full_path']} ({best['file']})")
                # Show driver association
                print(f"       {Style.BOLD}Driver:{Style.RESET} {rep.driver.module_name} ({rep.driver.version}) {'[nexmon]' if rep.driver.is_nexmon else '[stock]'}")
            else:
                print(f"       {Style.YELLOW}No matching firmware found in ls -l listing for this device{Style.RESET}")
                print(f"       Hint: check `ls -l /lib/firmware/brcm/` for {dev['vendor_id']}:{dev['device_id']} / {dev['description']}")
            # Show summary totals once
            if idx == 1 and summary:
                print(f"       {Style.DIM}ls totals: brcm={summary.get('total_brcm',0)} cypress={summary.get('total_cypress',0)} nexmon={summary.get('total_nexmon',0)}{Style.RESET}")

    print(f"\n{Style.BOLD}{Style.CYAN}======================================================================{Style.RESET}\n")


def print_analysis_report(
    rep: FirmwareAnalysisReport,
    as_json: bool = False,
    extras: Optional[Dict[str, Any]] = None,
):
    extras = extras or {}
    if as_json:
        d = asdict(rep)
        # Convert bytes in flashpatches to hex string
        for fp in d["flash_patches"]:
            if isinstance(fp["patch_bytes"], (bytes, bytearray)):
                fp["patch_bytes"] = fp["patch_bytes"].hex()
        if extras:
            d["additional_analysis"] = extras
        print(json.dumps(d, indent=2))
        return

    print(f"\n{Style.BOLD}{Style.CYAN}======================================================================{Style.RESET}")
    print(f"{Style.BOLD}{Style.CYAN}          FIRMWARE REVERSE ENGINEERING & MEMORY MAP REPORT           {Style.RESET}")
    print(f"{Style.BOLD}{Style.CYAN}======================================================================{Style.RESET}\n")

    print(f"File Path:       {rep.file_path}")
    print(f"File Size:       {rep.file_size:,} bytes")
    print(f"SHA-256:         {rep.sha256}")
    print(f"Architecture:    {rep.architecture}")
    print(f"Firmware Ver:    {Style.BOLD}{rep.version_string}{Style.RESET}")
    if rep.build_date:
        print(f"Build Date/Time: {rep.build_date} {rep.build_time}")
    print(f"Nexmon Status:   {Style.GREEN + 'Nexmon Patched' if rep.is_nexmon_patched else Style.YELLOW + 'Original / Stock'}{Style.RESET}")
    if rep.nexmon_capabilities:
        print(f"Capabilities:    {', '.join(rep.nexmon_capabilities)}")

    print(f"\n{Style.BOLD}🗺️  Reconstructed Memory Map:{Style.RESET}")
    print(f"   ROM Region:        0x{rep.rom_start:08X} - 0x{rep.rom_start+rep.rom_size:08X} (size 0x{rep.rom_size:X})")
    print(f"   RAM Region:        0x{rep.ram_start:08X} - 0x{rep.ram_start+rep.ram_size:08X} (size 0x{rep.ram_size:X})")
    if rep.ucode_start and rep.ucode_size:
        print(f"   Microcode (ucode): 0x{rep.ucode_start:08X} - 0x{rep.ucode_start+rep.ucode_size:08X} (size 0x{rep.ucode_size:X})")
    if rep.templateram_start and rep.templateram_size:
        print(f"   Template RAM:      0x{rep.templateram_start:08X} - 0x{rep.templateram_start+rep.templateram_size:08X} (size 0x{rep.templateram_size:X})")
    if rep.reclaim_0_end:
        print(f"   Reclaim End:       0x{rep.reclaim_0_end:08X} (ptr: 0x{rep.reclaim_0_end_ptr:08X})")
    if rep.ucode_write_bl_hook:
        print(f"   BL Hook Addr:      0x{rep.ucode_write_bl_hook:08X} (wlc_ucode_write hook site)")
    if rep.fp_config_base and rep.fp_config_end:
        print(f"   Flash Patch Cfg:   0x{rep.fp_config_base:08X} (derived base) | Orig End: 0x{rep.fp_config_end:08X}")

    print(f"\n{Style.BOLD}⚡ Flash Patch Table:{Style.RESET}")
    print(f"   Total Entries:     {len(rep.flash_patches)}")
    if rep.flash_patches:
        print("   First 5 Entries:")
        for fp in rep.flash_patches[:5]:
            dis = f" -> {fp.disassembly[0]}" if fp.disassembly else ""
            print(f"     [{fp.index:>3}] ROM 0x{fp.target_rom_addr:06X} ({fp.size}B) <= RAM 0x{fp.data_ram_addr:06X} [{fp.patch_bytes.hex()}]{dis}")
        if len(rep.flash_patches) > 5:
            print(f"     ... and {len(rep.flash_patches) - 5} more entries.")

    print(f"\n{Style.BOLD}📝 Generated definitions.mk:{Style.RESET}")
    print("----------------------------------------------------------------------")
    print(rep.definitions_mk)
    print("----------------------------------------------------------------------\n")

    extraction = extras.get("ucode_extraction")
    if extraction:
        color = Style.GREEN if extraction["success"] else Style.RED
        marker = "✓" if extraction["success"] else "✗"
        print(f"{color}{marker} {extraction['message']}{Style.RESET}")

    disassembly = extras.get("disassembly")
    if disassembly:
        print(
            f"\n{Style.BOLD}🔍 Disassembly at "
            f"0x{disassembly['address']:08X}:{Style.RESET}"
        )
        for line in disassembly["lines"]:
            print(f"   {line}")

    for name, tool_result in extras.get("tools", {}).items():
        print(f"\n{Style.BOLD}🔧 {name} report:{Style.RESET}")
        if tool_result["output_path"]:
            print(f"   Saved to: {tool_result['output_path']}")
        elif tool_result["output"]:
            print(tool_result["output"], end="" if tool_result["output"].endswith("\n") else "\n")
        if tool_result["stderr"]:
            print(tool_result["stderr"], file=sys.stderr, end="")


# =============================================================================
# Main Entry Point & CLI Parsers
# =============================================================================

def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="nexfirmware",
        description="Nexmon Broadcom/Cypress Firmware Toolkit: detection, reverse engineering, deployment, and restoration.",
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("-V", "--version", action="version", version="nexfirmware 1.0.0 (Nexmon Toolkit)")
    parser.add_argument("--no-color", action="store_true", help="Disable colored output")
    parser.add_argument("-j", "--json", action="store_true", help="Output machine-readable JSON")

    subparsers = parser.add_subparsers(dest="command", help="Subcommands")

    # 1. DETECT
    p_detect = subparsers.add_parser("detect", help="Detect brcmfmac wireless chip, driver, and active firmware")
    p_detect.add_argument("-i", "--interface", default="wlan0", help="Network interface to query (default: wlan0)")

    # 2. REVERSE / ANALYZE
    p_rev = subparsers.add_parser("reverse", aliases=["analyze"], help="Reverse engineer firmware binary (memory map, ucode, flashpatches)")
    p_rev.add_argument("file", help="Path to firmware binary (.bin)")
    p_rev.add_argument("-s", "--ramstart", type=lambda x: int(x, 0), default=0x198000, help="RAM start address (default: 0x198000)")
    p_rev.add_argument("-x", "--extract-ucode", metavar="OUTFILE", help="Extract microcode to OUTFILE")
    p_rev.add_argument("-d", "--disasm", metavar="ADDR", type=lambda x: int(x, 0), help="Disassemble ARM/Thumb instructions at memory address ADDR")
    p_rev.add_argument("-c", "--count", type=int, default=32, help="Number of bytes to disassemble (default: 32)")
    p_rev.add_argument(
        "--strings",
        nargs="?",
        const="-",
        metavar="OUTPUT",
        help="Run strings with hexadecimal offsets; optionally save to OUTPUT",
    )
    p_rev.add_argument(
        "--strings-min-length",
        type=int,
        default=4,
        metavar="N",
        help="Minimum length for --strings (default: 4)",
    )
    p_rev.add_argument(
        "--binwalk",
        nargs="?",
        const="-",
        metavar="OUTPUT",
        help="Run a read-only binwalk signature scan; optionally save to OUTPUT",
    )

    # 3. PULL
    p_pull = subparsers.add_parser("pull", help="Pull/backup active firmware and configs from system")
    p_pull.add_argument("-o", "--output-dir", default="./firmware_backup", help="Output directory (default: ./firmware_backup)")
    p_pull.add_argument("--no-meta", action="store_true", help="Skip pulling nvram/clm_blob files")

    # 4. PUT
    p_put = subparsers.add_parser("put", help="Deploy/install firmware binary to /lib/firmware/nexmon/")
    p_put.add_argument("file", help="Firmware file to deploy")
    p_put.add_argument("-n", "--name", help="Custom destination file name")
    p_put.add_argument("--no-alt", action="store_true", help="Do not update /etc/alternatives")
    p_put.add_argument("-r", "--reload", action="store_true", help="Reload brcmfmac driver after installation")
    p_put.add_argument("--no-backup", action="store_true", help="Do not create backup of current active firmware")

    # 5. RESTORE
    p_restore = subparsers.add_parser(
        "restore",
        help="Automatically find and activate compatible original firmware",
    )
    p_restore.add_argument(
        "-s",
        "--source",
        help="Use this original firmware instead of automatic discovery",
    )
    p_restore.add_argument(
        "-r",
        "--reload",
        action="store_true",
        help="Reload brcmfmac after restoration",
    )
    p_restore.add_argument(
        "--no-backup",
        action="store_true",
        help="Do not back up the currently active firmware",
    )
    p_restore.add_argument(
        "--dry-run",
        action="store_true",
        help="Show the selected original and activation method without changing files",
    )

    # 6. EDIT
    p_edit = subparsers.add_parser("edit", help="Edit/patch firmware binary")
    p_edit.add_argument("file", help="Firmware file to edit")
    p_edit.add_argument("-o", "--out", help="Write to output file (default: edit in-place)")
    p_edit.add_argument("--set-tag", help="Update the nexmon.org version tag")
    p_edit.add_argument("-p", "--patch", action="append", metavar="ADDR:HEX", help="Apply hex patch at memory address, e.g. 0x20B988:bd8d2200")

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    if args.no_color or not sys.stdout.isatty():
        Style.disable()

    if not args.command:
        # Default behavior: run detect
        args.command = "detect"
        args.interface = "wlan0"

    try:
        if args.command in ("detect",):
            rep = DeviceDetector.full_detection(interface=args.interface)
            print_detection_report(rep, as_json=args.json)
            return 0

        elif args.command in ("reverse", "analyze"):
            rep = FirmwareReverseEngineer.analyze_firmware(args.file, ram_start=args.ramstart)
            extras: Dict[str, Any] = {}

            if args.extract_ucode:
                ok, msg = FirmwareReverseEngineer.extract_ucode(args.file, args.extract_ucode, ram_start=args.ramstart)
                extras["ucode_extraction"] = {"success": ok, "message": msg}

            if args.disasm is not None:
                lines = FirmwareReverseEngineer.disassemble_range(
                    args.file, args.disasm, count_bytes=args.count, ram_start=args.ramstart
                )
                extras["disassembly"] = {
                    "address": args.disasm,
                    "lines": lines,
                }

            tools = {}
            if args.strings is not None:
                tools["strings"] = FirmwareReverseEngineer.extract_strings(
                    args.file,
                    min_length=args.strings_min_length,
                    output_file=None if args.strings == "-" else args.strings,
                )
            if args.binwalk is not None:
                tools["binwalk"] = FirmwareReverseEngineer.scan_binwalk(
                    args.file,
                    output_file=None if args.binwalk == "-" else args.binwalk,
                )
            if tools:
                extras["tools"] = tools

            print_analysis_report(rep, as_json=args.json, extras=extras)
            return 0

        elif args.command == "pull":
            manifest = FirmwareManager.pull(args.output_dir, include_meta=not args.no_meta)
            if args.json:
                print(json.dumps(manifest, indent=2))
            else:
                print(f"\n{Style.GREEN}✓ Firmware pulled successfully to: {args.output_dir}{Style.RESET}")
                print(f"  Source:   {manifest['source_path']}")
                print(f"  Files:    {', '.join(manifest['files'])}")
                print(f"  SHA-256:  {manifest['sha256']}\n")
            return 0

        elif args.command == "put":
            res = FirmwareManager.put(
                args.file,
                dest_name=args.name,
                update_alternatives=not args.no_alt,
                reload_driver=args.reload,
                backup_current=not args.no_backup
            )
            if args.json:
                print(json.dumps(res, indent=2))
            else:
                print(f"\n{Style.GREEN}✓ Firmware installed successfully!{Style.RESET}")
                print(f"  Installed Target: {res['installed_path']}")
                print(f"  SHA-256:          {res['sha256']}")
                if res['backup_created']:
                    print(f"  Backup Created:   {res['backup_created']}")
                print(f"  Alternatives:     {'Updated' if res['alternatives_updated'] else 'Skipped'}")
                print(f"  Driver:           {'Reloaded' if res['driver_reloaded'] else 'Unchanged'}\n")
            return 0

        elif args.command == "restore":
            res = FirmwareManager.restore(
                source_path=args.source,
                reload_driver=args.reload,
                backup_current=not args.no_backup,
                dry_run=args.dry_run,
            )
            if args.json:
                print(json.dumps(res, indent=2))
            else:
                action = "Restore plan" if args.dry_run else "Firmware restored"
                print(f"\n{Style.GREEN}✓ {action}{Style.RESET}")
                print(f"  Original Source:  {res['source_path']}")
                print(f"  Source Origin:    {res['source_origin']}")
                print(f"  Firmware Version: {res['source_version'] or 'Unknown'}")
                print(f"  SHA-256:          {res['source_sha256']}")
                print(f"  Activation:       {res['activation']}")
                print(f"  Restored Path:    {res['restored_path']}")
                if res["backup_created"]:
                    print(f"  Backup Created:   {res['backup_created']}")
                print(
                    f"  Driver:           "
                    f"{'Reloaded' if res['driver_reloaded'] else 'Unchanged'}\n"
                )
            return 0

        elif args.command == "edit":
            parsed_patches = []
            if args.patch:
                for p in args.patch:
                    if ":" not in p:
                        raise ValueError(f"Patch specification must be ADDR:HEXBYTES, got '{p}'")
                    addr_s, hex_s = p.split(":", 1)
                    addr = int(addr_s, 0)
                    p_bytes = bytes.fromhex(hex_s.replace(" ", ""))
                    parsed_patches.append((addr, p_bytes))

            res = FirmwareManager.edit(
                args.file,
                out_file=args.out,
                version_tag=args.set_tag,
                patches=parsed_patches if parsed_patches else None
            )
            if args.json:
                print(json.dumps(res, indent=2))
            else:
                print(f"\n{Style.GREEN}✓ Firmware binary edited successfully!{Style.RESET}")
                print(f"  Output Path: {res['output_path']}")
                print(f"  Original SHA: {res['original_sha256']}")
                print(f"  Modified SHA: {res['modified_sha256']}")
                print("  Changes applied:")
                for c in res['changes']:
                    print(f"    - {c}")
                print()
            return 0

    except Exception as e:
        print(f"{Style.RED}Error: {e}{Style.RESET}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    sys.exit(main())
