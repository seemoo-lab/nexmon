"""Channel State Information (CSI) extraction, parsing, and streaming module for Nexmon.

Supports decoding raw CSI packets encapsulated in UDP frames (port 5500),
zero-copy Generic Netlink streams (family "nexmon_csi", group "csi"),
or radiotap vendor headers emitted by Nexmon CSI extraction firmware.
Provides native complex representations, NumPy matrix conversions, JSON streaming,
and real-time socket/Netlink receivers.
"""

import cmath
import json
import os
import socket
import struct
import time
from typing import Any, Dict, Iterator, List, Optional, Tuple, Union

try:
    import numpy as np
    HAVE_NUMPY = True
except ImportError:
    np = None  # type: ignore
    HAVE_NUMPY = False

NEXMON_CSI_MAGIC = 0x11111111
DEFAULT_CSI_PORT = 5500

# Generic Netlink Constants
NETLINK_GENERIC = 16
SOL_NETLINK = 270
NETLINK_ADD_MEMBERSHIP = 1
NETLINK_DROP_MEMBERSHIP = 2

GENL_ID_CTRL = 16
CTRL_CMD_GETFAMILY = 3
CTRL_ATTR_FAMILY_ID = 1
CTRL_ATTR_FAMILY_NAME = 2
CTRL_ATTR_MCAST_GROUPS = 7
CTRL_ATTR_MCAST_GRP_NAME = 1
CTRL_ATTR_MCAST_GRP_ID = 2

NEXMON_CSI_GENL_NAME = "nexmon_csi"
NEXMON_CSI_GENL_MCGRP_NAME = "csi"
NEXMON_CSI_CMD_SAMPLE = 1
NEXMON_CSI_ATTR_RAW = 1
NEXMON_CSI_ATTR_TSFT = 2
NEXMON_CSI_ATTR_IFINDEX = 3


def _format_mac(raw_bytes: bytes) -> str:
    """Format 6 raw bytes into colon-separated hex MAC address."""
    return ":".join(f"{b:02x}" for b in raw_bytes)


class CSIPacket:
    """Represents a single decoded Channel State Information (CSI) frame."""

    def __init__(
        self,
        magic: int,
        src_mac: str,
        seq: int,
        core: int,
        spatial_stream: int,
        chanspec: int,
        chip_id: int,
        raw_iq: List[Tuple[int, int]],
        timestamp: Optional[float] = None,
        tsft_us: Optional[int] = None,
        ifindex: Optional[int] = None,
    ):
        self.magic = magic
        self.src_mac = src_mac
        self.seq = seq
        self.core = core
        self.spatial_stream = spatial_stream
        self.chanspec = chanspec
        self.chip_id = chip_id
        self.raw_iq = raw_iq
        self.timestamp = timestamp if timestamp is not None else time.time()
        self.tsft_us = tsft_us
        self.ifindex = ifindex
        self.num_subcarriers = len(raw_iq)

        # Precompute complex samples
        self.complex_csi = [complex(i, q) for i, q in raw_iq]

    @property
    def bandwidth_mhz(self) -> int:
        """Estimate bandwidth based on subcarrier count and chanspec."""
        if self.num_subcarriers >= 480:
            return 160
        elif self.num_subcarriers >= 240:
            return 80
        elif self.num_subcarriers >= 120:
            return 40
        elif self.num_subcarriers > 0:
            return 20

        # If subcarrier count is 0, derive from chanspec bits
        if self.chanspec:
            bw_field = self.chanspec & 0x3800
            if bw_field == 0x2800:
                return 160
            elif bw_field in (0x2000, 0x3000):
                return 80
            elif bw_field == 0x1800:
                return 40
            elif bw_field == 0x1000:
                return 20
        return 20

    @property
    def amplitudes(self) -> List[float]:
        """Compute signal amplitudes (magnitudes) for each subcarrier."""
        return [abs(c) for c in self.complex_csi]

    @property
    def phases(self) -> List[float]:
        """Compute signal phases in radians for each subcarrier."""
        return [cmath.phase(c) for c in self.complex_csi]

    def to_numpy(self) -> Any:
        """Convert CSI data to a 1D NumPy complex128 array.

        Requires numpy. Raises ImportError if numpy is not installed.
        """
        if not HAVE_NUMPY:
            raise ImportError("NumPy is required for to_numpy(). Install via: pip install numpy")
        return np.array(self.complex_csi, dtype=np.complex128)

    def amplitudes_numpy(self) -> Any:
        """Convert amplitudes to a NumPy float64 array."""
        if not HAVE_NUMPY:
            raise ImportError("NumPy is required for amplitudes_numpy(). Install via: pip install numpy")
        return np.abs(self.to_numpy())

    def phases_numpy(self) -> Any:
        """Convert phases to a NumPy float64 array (in radians)."""
        if not HAVE_NUMPY:
            raise ImportError("NumPy is required for phases_numpy(). Install via: pip install numpy")
        return np.angle(self.to_numpy())

    def to_dict(self, include_iq: bool = True) -> Dict[str, Any]:
        """Convert packet metadata and subcarriers to a Python dictionary."""
        d: Dict[str, Any] = {
            "timestamp": self.timestamp,
            "src_mac": self.src_mac,
            "seq": self.seq,
            "core": self.core,
            "spatial_stream": self.spatial_stream,
            "chanspec": f"0x{self.chanspec:04x}",
            "chip_id": f"0x{self.chip_id:04x}",
            "num_subcarriers": self.num_subcarriers,
            "bandwidth_mhz": self.bandwidth_mhz,
            "avg_amplitude": (sum(self.amplitudes) / len(self.amplitudes)) if self.amplitudes else 0.0,
        }
        if self.tsft_us is not None:
            d["tsft_us"] = self.tsft_us
        if self.ifindex is not None:
            d["ifindex"] = self.ifindex
        if include_iq:
            d["raw_iq"] = self.raw_iq
            d["amplitudes"] = [round(a, 3) for a in self.amplitudes]
            d["phases"] = [round(p, 4) for p in self.phases]
        return d

    def to_json(self, indent: Optional[int] = None) -> str:
        """Serialize packet to JSON string."""
        return json.dumps(self.to_dict(include_iq=True), indent=indent)

    def __repr__(self) -> str:
        return (
            f"<CSIPacket src={self.src_mac} seq={self.seq} core={self.core} "
            f"subcarriers={self.num_subcarriers} ({self.bandwidth_mhz}MHz) "
            f"chanspec=0x{self.chanspec:04x}>"
        )


def parse_csi_payload(payload: bytes, offset: int = 0, timestamp: Optional[float] = None) -> CSIPacket:
    """Parse a raw Nexmon CSI frame into a CSIPacket instance.

    Expected header layout (18 bytes minimum):
      - uint32 magic (0x11111111)
      - 6 bytes src_mac
      - uint16 seq
      - uint16 core_and_spatial (low byte = core, high byte = stream)
      - uint16 chanspec
      - uint16 chip_id
      - Followed by N * 4 bytes of raw I/Q pairs: int16 I, int16 Q.
    """
    if offset < 0:
        raise ValueError(f"Invalid offset: {offset} (must be non-negative)")
    if len(payload) < offset + 18:
        raise ValueError(f"CSI payload too short: {len(payload) - offset} bytes (minimum 18 required)")

    magic = struct.unpack_from("<I", payload, offset)[0]
    if magic != NEXMON_CSI_MAGIC:
        raise ValueError(f"Invalid Nexmon CSI magic: 0x{magic:08x} (expected 0x{NEXMON_CSI_MAGIC:08x})")
    src_mac = _format_mac(payload[offset + 4 : offset + 10])
    seq = struct.unpack_from("<H", payload, offset + 10)[0]
    core_spatial = struct.unpack_from("<H", payload, offset + 12)[0]
    core = core_spatial & 0xFF
    stream = (core_spatial >> 8) & 0xFF
    chanspec = struct.unpack_from("<H", payload, offset + 14)[0]
    chip_id = struct.unpack_from("<H", payload, offset + 16)[0]

    iq_bytes = payload[offset + 18 :]
    num_samples = len(iq_bytes) // 4
    if num_samples == 0:
        raw_iq: List[Tuple[int, int]] = []
    else:
        # Unpack num_samples signed 16-bit integer pairs
        unpacked = struct.unpack(f"<{2 * num_samples}h", iq_bytes[: 4 * num_samples])
        raw_iq = [(unpacked[i], unpacked[i + 1]) for i in range(0, len(unpacked), 2)]

    return CSIPacket(
        magic=magic,
        src_mac=src_mac,
        seq=seq,
        core=core,
        spatial_stream=stream,
        chanspec=chanspec,
        chip_id=chip_id,
        raw_iq=raw_iq,
        timestamp=timestamp,
    )


def resolve_genl_family(family_name: str, timeout: float = 1.0) -> Tuple[int, Dict[str, int]]:
    """Resolve Generic Netlink family name to (family_id, {group_name: group_id}).

    Raises:
        OSError: If AF_NETLINK is unsupported.
        RuntimeError: If family is not registered in the kernel.
    """
    if not hasattr(socket, "AF_NETLINK"):
        raise OSError("AF_NETLINK is not supported on this operating system")

    s = socket.socket(socket.AF_NETLINK, socket.SOCK_RAW, NETLINK_GENERIC)
    s.settimeout(timeout)
    try:
        s.bind((0, 0))
        name_bytes = family_name.encode("utf-8") + b"\0"
        attr_len = 4 + len(name_bytes)
        pad = (4 - (len(name_bytes) % 4)) % 4
        attr = struct.pack("HH", attr_len, CTRL_ATTR_FAMILY_NAME) + name_bytes + (b"\0" * pad)

        genl_hdr = struct.pack("BBH", CTRL_CMD_GETFAMILY, 1, 0)
        total_len = 16 + len(genl_hdr) + len(attr)
        nlmsghdr = struct.pack("IHHII", total_len, GENL_ID_CTRL, 1, 1, os.getpid() & 0xFFFFFFFF)
        msg = nlmsghdr + genl_hdr + attr

        s.send(msg)
        reply = s.recv(8192)

        nl_len, nl_type, nl_flags, nl_seq, nl_pid = struct.unpack_from("IHHII", reply, 0)
        if nl_type == 2:  # NLMSG_ERROR
            errno = -struct.unpack_from("i", reply, 16)[0]
            raise RuntimeError(f"Netlink error resolving family '{family_name}': errno={errno}")

        offset = 20
        fam_id = None
        mcgrps: Dict[str, int] = {}

        while offset + 4 <= min(nl_len, len(reply)):
            nla_len, nla_type = struct.unpack_from("HH", reply, offset)
            if nla_len < 4:
                break
            attr_data = reply[offset + 4 : offset + nla_len]
            attr_type = nla_type & 0x3FFF

            if attr_type == CTRL_ATTR_FAMILY_ID and len(attr_data) >= 2:
                fam_id = struct.unpack("<H", attr_data[:2])[0]
            elif attr_type == CTRL_ATTR_MCAST_GROUPS:
                grp_off = 0
                while grp_off + 4 <= len(attr_data):
                    gnla_len, _ = struct.unpack_from("HH", attr_data, grp_off)
                    if gnla_len < 4 or grp_off + gnla_len > len(attr_data):
                        break
                    nested = attr_data[grp_off + 4 : grp_off + gnla_len]
                    noff = 0
                    g_name = None
                    g_id = None
                    while noff + 4 <= len(nested):
                        sub_len, sub_type = struct.unpack_from("HH", nested, noff)
                        if sub_len < 4 or noff + sub_len > len(nested):
                            break
                        sub_data = nested[noff + 4 : noff + sub_len]
                        stype = sub_type & 0x3FFF
                        if stype == CTRL_ATTR_MCAST_GRP_NAME:
                            g_name = sub_data.rstrip(b"\0").decode("utf-8", errors="replace")
                        elif stype == CTRL_ATTR_MCAST_GRP_ID and len(sub_data) >= 4:
                            g_id = struct.unpack("<I", sub_data[:4])[0]
                        noff += (sub_len + 3) & ~3
                    if g_name and g_id is not None:
                        mcgrps[g_name] = g_id
                    grp_off += (gnla_len + 3) & ~3
            offset += (nla_len + 3) & ~3

        if fam_id is None:
            raise RuntimeError(f"Generic Netlink family '{family_name}' not found")
        return fam_id, mcgrps
    finally:
        s.close()


class NetlinkCSIReceiver:
    """Zero-copy Generic Netlink receiver for kernel-streamed Nexmon CSI frames."""

    def __init__(
        self,
        family_name: str = NEXMON_CSI_GENL_NAME,
        group_name: str = NEXMON_CSI_GENL_MCGRP_NAME,
        buffer_size: int = 65535,
    ):
        self.family_name = family_name
        self.group_name = group_name
        self.buffer_size = buffer_size
        self._sock: Optional[socket.socket] = None
        self._family_id: Optional[int] = None
        self._mcast_group_id: Optional[int] = None

    def start(self):
        """Resolve family, bind Generic Netlink socket, and subscribe to multicast group."""
        if self._sock is not None:
            return

        self._family_id, mcgrps = resolve_genl_family(self.family_name)
        if self.group_name not in mcgrps:
            raise RuntimeError(
                f"Multicast group '{self.group_name}' not found in family '{self.family_name}'. Available: {list(mcgrps.keys())}"
            )
        self._mcast_group_id = mcgrps[self.group_name]

        s = socket.socket(socket.AF_NETLINK, socket.SOCK_RAW, NETLINK_GENERIC)
        try:
            # Request larger socket buffer for high-bandwidth capture (80/160MHz)
            s.setsockopt(socket.SOL_SOCKET, socket.SO_RCVBUF, 4 * 1024 * 1024)
        except OSError:
            pass

        s.bind((0, 0))
        s.setsockopt(SOL_NETLINK, NETLINK_ADD_MEMBERSHIP, self._mcast_group_id)
        self._sock = s

    def close(self):
        """Drop multicast subscription and close socket."""
        if self._sock:
            if self._mcast_group_id is not None:
                try:
                    self._sock.setsockopt(SOL_NETLINK, NETLINK_DROP_MEMBERSHIP, self._mcast_group_id)
                except OSError:
                    pass
            try:
                self._sock.close()
            except OSError:
                pass
            self._sock = None

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def recv(self, timeout: Optional[float] = 1.0) -> Optional[CSIPacket]:
        """Receive a single CSI packet from the Netlink multicast ring buffer."""
        if self._sock is None:
            self.start()
        assert self._sock is not None

        self._sock.settimeout(timeout)
        try:
            data = self._sock.recv(self.buffer_size)
        except (socket.timeout, TimeoutError):
            return None

        offset = 0
        while offset + 16 <= len(data):
            nl_len, nl_type, nl_flags, nl_seq, nl_pid = struct.unpack_from("IHHII", data, offset)
            if nl_len < 16 or offset + nl_len > len(data):
                break
            msg_bytes = data[offset : offset + nl_len]
            offset += (nl_len + 3) & ~3

            if nl_type in (2, 3):  # NLMSG_ERROR, NLMSG_DONE
                continue
            if self._family_id is not None and nl_type != self._family_id:
                continue

            # Generic Netlink header: cmd (1B), version (1B), reserved (2B)
            if len(msg_bytes) < 20:
                continue
            cmd, ver, res = struct.unpack_from("BBH", msg_bytes, 16)
            if cmd != NEXMON_CSI_CMD_SAMPLE:
                continue

            # Parse attributes starting at byte 20
            attr_offset = 20
            raw_payload: Optional[bytes] = None
            tsft_us: Optional[int] = None
            ifindex: Optional[int] = None

            while attr_offset + 4 <= len(msg_bytes):
                nla_len, nla_type = struct.unpack_from("HH", msg_bytes, attr_offset)
                if nla_len < 4 or attr_offset + nla_len > len(msg_bytes):
                    break
                attr_data = msg_bytes[attr_offset + 4 : attr_offset + nla_len]
                atype = nla_type & 0x3FFF

                if atype == NEXMON_CSI_ATTR_RAW:
                    raw_payload = attr_data
                elif atype == NEXMON_CSI_ATTR_TSFT and len(attr_data) >= 8:
                    tsft_us = struct.unpack("<Q", attr_data[:8])[0]
                elif atype == NEXMON_CSI_ATTR_IFINDEX and len(attr_data) >= 4:
                    ifindex = struct.unpack("<I", attr_data[:4])[0]

                attr_offset += (nla_len + 3) & ~3

            if raw_payload:
                try:
                    ts = (tsft_us / 1e6) if tsft_us is not None else None
                    pkt = parse_csi_payload(raw_payload, timestamp=ts)
                    pkt.tsft_us = tsft_us
                    pkt.ifindex = ifindex
                    return pkt
                except ValueError:
                    return None

        return None

    def stream(self, max_packets: Optional[int] = None, timeout: Optional[float] = 2.0) -> Iterator[CSIPacket]:
        """Stream received CSI packets continuously as an iterator."""
        count = 0
        while max_packets is None or count < max_packets:
            pkt = self.recv(timeout=timeout)
            if pkt is None:
                if timeout is not None:
                    break
                continue
            count += 1
            yield pkt


class UDPCSIReceiver:
    """Real-time UDP receiver and listener for legacy Nexmon CSI frames (port 5500)."""

    def __init__(self, host: str = "0.0.0.0", port: int = DEFAULT_CSI_PORT, buffer_size: int = 65535):
        self.host = host
        self.port = port
        self.buffer_size = buffer_size
        self._sock: Optional[socket.socket] = None

    def start(self):
        """Bind and open UDP listening socket."""
        if self._sock is None:
            self._sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
            self._sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
            self._sock.bind((self.host, self.port))

    def close(self):
        """Close listening socket."""
        if self._sock:
            try:
                self._sock.close()
            except Exception:
                pass
            self._sock = None

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def recv(self, timeout: Optional[float] = 1.0) -> Optional[CSIPacket]:
        """Receive a single CSI packet. Returns None on timeout or malformed UDP datagram."""
        if not self._sock:
            self.start()
        assert self._sock is not None

        self._sock.settimeout(timeout)
        try:
            data, _addr = self._sock.recvfrom(self.buffer_size)
            try:
                return parse_csi_payload(data)
            except ValueError:
                return None
        except (socket.timeout, TimeoutError):
            return None

    def stream(self, max_packets: Optional[int] = None, timeout: Optional[float] = 2.0) -> Iterator[CSIPacket]:
        """Stream received CSI packets continuously as an iterator. Stops if socket times out."""
        count = 0
        while max_packets is None or count < max_packets:
            pkt = self.recv(timeout=timeout)
            if pkt is None:
                if timeout is not None:
                    break
                continue
            count += 1
            yield pkt


class CSIReceiver:
    """Unified CSI Receiver supporting automatic Generic Netlink detection and UDP fallback.

    Modes:
        "auto": Discovers and uses Netlink ring buffer if registered in the kernel,
                falling back seamlessly to UDP broadcast socket.
        "netlink": Strictly forces Generic Netlink zero-copy receiver.
        "udp": Strictly forces legacy UDP receiver (port 5500).
    """

    def __init__(
        self,
        mode: str = "auto",
        host: str = "0.0.0.0",
        port: int = DEFAULT_CSI_PORT,
        buffer_size: int = 65535,
        family_name: str = NEXMON_CSI_GENL_NAME,
        group_name: str = NEXMON_CSI_GENL_MCGRP_NAME,
    ):
        self.mode = mode.lower()
        self.host = host
        self.port = port
        self.buffer_size = buffer_size
        self.family_name = family_name
        self.group_name = group_name

        self._active_backend: Optional[str] = None
        self._delegate: Optional[Union[NetlinkCSIReceiver, UDPCSIReceiver]] = None

    @property
    def backend(self) -> Optional[str]:
        """Return the active receiver backend ('netlink' or 'udp')."""
        return self._active_backend

    def start(self):
        """Initialize the underlying receiver backend."""
        if self._delegate is not None:
            return

        # If user explicitly specified non-default UDP host/port, prefer UDP
        if self.mode == "auto" and (self.port != DEFAULT_CSI_PORT or (self.host not in ("0.0.0.0", ""))):
            self._delegate = UDPCSIReceiver(host=self.host, port=self.port, buffer_size=self.buffer_size)
            self._delegate.start()
            self._active_backend = "udp"
            return

        if self.mode in ("auto", "netlink"):
            try:
                nl_rx = NetlinkCSIReceiver(
                    family_name=self.family_name,
                    group_name=self.group_name,
                    buffer_size=self.buffer_size,
                )
                nl_rx.start()
                self._delegate = nl_rx
                self._active_backend = "netlink"
                return
            except Exception as ex:
                if self.mode == "netlink":
                    raise
                # Fall back to UDP for 'auto'
                self._delegate = None

        self._delegate = UDPCSIReceiver(host=self.host, port=self.port, buffer_size=self.buffer_size)
        self._delegate.start()
        self._active_backend = "udp"

    def close(self):
        """Close active receiver backend."""
        if self._delegate:
            self._delegate.close()
            self._delegate = None
            self._active_backend = None

    def __enter__(self):
        self.start()
        return self

    def __exit__(self, exc_type, exc_val, exc_tb):
        self.close()

    def recv(self, timeout: Optional[float] = 1.0) -> Optional[CSIPacket]:
        """Receive a single CSI packet from the active backend."""
        if self._delegate is None:
            self.start()
        assert self._delegate is not None
        return self._delegate.recv(timeout=timeout)

    def stream(self, max_packets: Optional[int] = None, timeout: Optional[float] = 2.0) -> Iterator[CSIPacket]:
        """Stream received CSI packets continuously from the active backend."""
        if self._delegate is None:
            self.start()
        assert self._delegate is not None
        return self._delegate.stream(max_packets=max_packets, timeout=timeout)


def main():
    """Command-line interface for capturing and displaying live CSI streams."""
    import argparse

    parser = argparse.ArgumentParser(description="Nexmon CSI Packet Receiver and Inspector")
    parser.add_argument("--mode", "-m", choices=["auto", "netlink", "udp"], default="auto",
                        help="Reception mode: auto (default), netlink, or udp")
    parser.add_argument("--netlink", "-n", action="store_true", help="Force Generic Netlink mode (shortcut for --mode netlink)")
    parser.add_argument("--port", "-p", type=int, default=DEFAULT_CSI_PORT, help="UDP listening port (default: 5500)")
    parser.add_argument("--count", "-c", type=int, default=0, help="Number of packets to capture (0 = infinite)")
    parser.add_argument("--json", action="store_true", help="Output stream as newline-delimited JSON")
    parser.add_argument("--quiet", "-q", action="store_true", help="Quiet output (show only packet count and rate)")
    args = parser.parse_args()

    mode = "netlink" if args.netlink else args.mode
    receiver = CSIReceiver(mode=mode, port=args.port)
    with receiver:
        print(f"Listening for Nexmon CSI frames via [{receiver.backend.upper()}] backend...")
        count = 0
        t0 = time.time()
        try:
            for pkt in receiver.stream(max_packets=args.count if args.count > 0 else None, timeout=5.0):
                count += 1
                if args.json:
                    print(pkt.to_json())
                elif not args.quiet:
                    avg_amp = (sum(pkt.amplitudes) / len(pkt.amplitudes)) if pkt.amplitudes else 0
                    tsft_str = f" TSF:{pkt.tsft_us}us" if pkt.tsft_us else ""
                    print(
                        f"[{pkt.timestamp:.4f}] #{count:04d} MAC: {pkt.src_mac} Seq: {pkt.seq:5d} "
                        f"BW: {pkt.bandwidth_mhz}MHz ({pkt.num_subcarriers} subcarriers){tsft_str} "
                        f"Avg Amp: {avg_amp:6.2f}"
                    )
        except KeyboardInterrupt:
            pass

        dur = max(time.time() - t0, 0.001)
        print(f"\nCaptured {count} CSI frames in {dur:.2f}s ({count / dur:.2f} frames/sec)")


if __name__ == "__main__":
    main()
