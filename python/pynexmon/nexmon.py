"""Native Python SDK for Broadcom Nexmon Wi-Fi firmware and drivers."""
import ctypes
import os
from pathlib import Path
import struct
from typing import Dict, Optional, Tuple, Union

# Broadcom WLC and Nexmon IOCTL commands
WLC_GET_PROMISC = 9
WLC_SET_PROMISC = 10
WLC_DISASSOC = 52
WLC_GET_REVINFO = 98
WLC_GET_MONITOR = 107
WLC_SET_MONITOR = 108
WLC_GET_SCANSUPPRESS = 115
WLC_SET_SCANSUPPRESS = 116
WLC_GET_VAR = 262
WLC_SET_VAR = 263
NEX_GET_CAPABILITIES = 400
NEX_INJECT_FRAME = 401
NEX_GET_SECURITYCOOKIE = 404
NEX_SET_SECURITYCOOKIE = 405
NEX_GET_WL_CNT = 407

INVCHANSPEC = 0


class NexmonError(Exception):
    """Exception raised for Nexmon IOCTL or hardware errors."""
    pass


def _find_libnexio() -> str:
    """Find libnexio.so in the package, repo utilities, or system paths."""
    candidates = [
        Path(__file__).resolve().parent / "libnexio.so",
        Path(__file__).resolve().parents[2] / "utilities/libnexio/libnexio.so",
        Path("/usr/local/lib/libnexio.so"),
        Path("/usr/lib/libnexio.so"),
    ]
    for p in candidates:
        if p.is_file():
            return str(p)
    return "libnexio.so"


class _LibNexioBindings:
    """Singleton holder for ctypes bindings to libnexio.so."""
    _instance: Optional["_LibNexioBindings"] = None

    def __init__(self, lib_path: Optional[str] = None):
        path = lib_path or _find_libnexio()
        self.lib = ctypes.CDLL(path)

        # struct nexio *nex_init_ioctl(const char *ifname)
        self.lib.nex_init_ioctl.argtypes = [ctypes.c_char_p]
        self.lib.nex_init_ioctl.restype = ctypes.c_void_p

        # struct nexio *nex_init_netlink_ifname(const char *ifname)
        self.lib.nex_init_netlink_ifname.argtypes = [ctypes.c_char_p]
        self.lib.nex_init_netlink_ifname.restype = ctypes.c_void_p

        # void nex_free(struct nexio *nexio)
        self.lib.nex_free.argtypes = [ctypes.c_void_p]
        self.lib.nex_free.restype = None

        # int nex_ioctl(struct nexio *nexio, int cmd, void *buf, int len, bool set)
        self.lib.nex_ioctl.argtypes = [ctypes.c_void_p, ctypes.c_int, ctypes.c_void_p, ctypes.c_int, ctypes.c_bool]
        self.lib.nex_ioctl.restype = ctypes.c_int

        # chanspec helpers
        self.lib.wf_chspec_aton.argtypes = [ctypes.c_char_p]
        self.lib.wf_chspec_aton.restype = ctypes.c_uint16

        self.lib.wf_chspec_ntoa_ex.argtypes = [ctypes.c_uint16, ctypes.c_char_p]
        self.lib.wf_chspec_ntoa_ex.restype = ctypes.c_char_p

        self.lib.wf_chspec_is_6g_psc.argtypes = [ctypes.c_uint16]
        self.lib.wf_chspec_is_6g_psc.restype = ctypes.c_bool

        self.lib.wf_chanspec2mhz.argtypes = [ctypes.c_uint16]
        self.lib.wf_chanspec2mhz.restype = ctypes.c_int

    @classmethod
    def get(cls) -> "_LibNexioBindings":
        if cls._instance is None:
            cls._instance = cls()
        return cls._instance


class Chanspec:
    """Broadcom channel and chanspec utilities."""

    @staticmethod
    def aton(spec: str) -> int:
        """Parse human-readable chanspec string (e.g. '36/80', '6g37') into uint16."""
        bindings = _LibNexioBindings.get()
        cs = bindings.lib.wf_chspec_aton(spec.encode("ascii"))
        if cs == INVCHANSPEC and spec not in ("0", "0x0000"):
            raise ValueError(f"Invalid chanspec specification: {spec!r}")
        return cs

    @staticmethod
    def ntoa(chspec: int) -> str:
        """Format uint16 chanspec into human-readable string."""
        bindings = _LibNexioBindings.get()
        buf = ctypes.create_string_buffer(64)
        res = bindings.lib.wf_chspec_ntoa_ex(chspec, buf)
        return res.decode("ascii") if res else ""

    @staticmethod
    def is_6g_psc(chspec: int) -> bool:
        """Check if chanspec is a 6 GHz Preferred Scanning Channel (PSC)."""
        bindings = _LibNexioBindings.get()
        return bool(bindings.lib.wf_chspec_is_6g_psc(chspec))

    @staticmethod
    def to_mhz(chspec: int) -> int:
        """Convert chanspec to center frequency in MHz."""
        bindings = _LibNexioBindings.get()
        return int(bindings.lib.wf_chanspec2mhz(chspec))


class NexmonDevice:
    """Interface to a Broadcom wireless device running Nexmon firmware."""

    def __init__(self, ifname: str = "wlan0", mode: str = "ioctl", check_interface: bool = True):
        self.ifname = ifname
        self.mode = mode.lower()

        if check_interface and not os.path.exists(f"/sys/class/net/{ifname}"):
            raise NexmonError(f"Network interface {ifname!r} does not exist in /sys/class/net")

        self._bindings = _LibNexioBindings.get()
        if self.mode == "netlink":
            self._handle = self._bindings.lib.nex_init_netlink_ifname(ifname.encode())
        else:
            self._handle = self._bindings.lib.nex_init_ioctl(ifname.encode())

        if not self._handle:
            raise NexmonError(f"Failed to initialize nexio handle for interface {ifname} (mode={mode})")

    def close(self):
        """Release underlying driver sockets and handle."""
        if self._handle:
            self._bindings.lib.nex_free(self._handle)
            self._handle = None

    def __enter__(self):
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def ioctl(self, cmd: int, data: Union[bytes, bytearray, int] = 0, length: int = 4, set_cmd: bool = False) -> bytearray:
        """Execute a raw IOCTL command to firmware."""
        if not self._handle:
            raise NexmonError("Device handle is closed")

        if isinstance(data, int):
            buf = bytearray(struct.pack("<I", data) if length == 4 else (data).to_bytes(length, "little"))
        else:
            buf = bytearray(data)
            if len(buf) < length:
                buf.extend(b"\x00" * (length - len(buf)))

        c_buf = (ctypes.c_char * len(buf)).from_buffer(buf)
        ret = self._bindings.lib.nex_ioctl(self._handle, cmd, c_buf, len(buf), set_cmd)
        if ret < 0:
            raise NexmonError(f"IOCTL {cmd} failed with error code {ret}")

        return buf

    def get_capabilities(self) -> int:
        """Query firmware capabilities bitmask."""
        res = self.ioctl(NEX_GET_CAPABILITIES, length=4, set_cmd=False)
        return struct.unpack("<I", res[:4])[0]

    def get_monitor(self) -> int:
        """Get monitor mode state (0=off, 1=on, 2=with radiotap)."""
        res = self.ioctl(WLC_GET_MONITOR, length=4, set_cmd=False)
        return struct.unpack("<I", res[:4])[0]

    def set_monitor(self, mode: Union[int, bool] = 1):
        """Set monitor mode (0=off, 1=standard monitor, 2=radiotap monitor)."""
        val = int(mode)
        self.ioctl(WLC_SET_MONITOR, data=val, length=4, set_cmd=True)

    def get_promisc(self) -> int:
        """Get promiscuous mode state."""
        res = self.ioctl(WLC_GET_PROMISC, length=4, set_cmd=False)
        return struct.unpack("<I", res[:4])[0]

    def set_promisc(self, mode: Union[int, bool] = 1):
        """Set promiscuous mode state."""
        val = int(mode)
        self.ioctl(WLC_SET_PROMISC, data=val, length=4, set_cmd=True)

    def get_chanspec(self) -> Tuple[int, str]:
        """Get current chanspec as (uint16_chanspec, formatted_string)."""
        var_req = bytearray(b"chanspec\x00\x00\x00\x00\x00")
        res = self.ioctl(WLC_GET_VAR, data=var_req, length=13, set_cmd=False)
        chanspec = struct.unpack("<H", res[:2])[0]
        return chanspec, Chanspec.ntoa(chanspec)

    def set_chanspec(self, chspec: Union[int, str]):
        """Set channel chanspec using integer or string representation (e.g. '36/80', '6')."""
        if isinstance(chspec, str):
            cs_val = Chanspec.aton(chspec)
        else:
            cs_val = int(chspec)

        var_req = bytearray(b"chanspec\x00" + struct.pack("<I", cs_val))
        self.ioctl(WLC_SET_VAR, data=var_req, length=13, set_cmd=True)

    def disassociate(self):
        """Send disassociate command to clear connection state."""
        self.ioctl(WLC_DISASSOC, data=1, length=4, set_cmd=True)

    def get_revinfo(self) -> Dict[str, int]:
        """Retrieve firmware and hardware revision info."""
        res = self.ioctl(WLC_GET_REVINFO, length=68, set_cmd=False)
        fields = [
            "vendorid", "deviceid", "radiorev", "chiprev", "corerev",
            "boardid", "boardvendor", "boardrev", "driverrev", "ucoderev",
            "bus", "chipnum", "phytype", "phyrev", "anarev", "chippkg", "nvramrev"
        ]
        values = struct.unpack("<17I", res[:68])
        return dict(zip(fields, values))
