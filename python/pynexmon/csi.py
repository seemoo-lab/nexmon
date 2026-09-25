"""Channel State Information (CSI) extraction, parsing, and streaming module for Nexmon.

Supports decoding raw CSI packets encapsulated in UDP frames (port 5500)
or radiotap vendor headers emitted by Nexmon CSI extraction firmware.
Provides native complex representations, NumPy matrix conversions, JSON streaming,
and real-time socket receivers.
"""

import cmath
import json
import math
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
    ):
        self.magic = magic
        self.src_mac = src_mac
        self.seq = seq
        self.core = core
        self.spatial_stream = spatial_stream
        self.chanspec = chanspec
        self.chip_id = chip_id
        self.raw_iq = raw_iq
        self.timestamp = timestamp or time.time()
        self.num_subcarriers = len(raw_iq)

        # Precompute complex samples
        self.complex_csi = [complex(i, q) for i, q in raw_iq]

    @property
    def bandwidth_mhz(self) -> int:
        """Estimate bandwidth based on subcarrier count."""
        if self.num_subcarriers >= 480:
            return 160
        elif self.num_subcarriers >= 240:
            return 80
        elif self.num_subcarriers >= 120:
            return 40
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


def parse_csi_payload(payload: bytes, offset: int = 0) -> CSIPacket:
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
    if len(payload) < offset + 18:
        raise ValueError(f"CSI payload too short: {len(payload) - offset} bytes (minimum 18 required)")

    magic = struct.unpack_from("<I", payload, offset)[0]
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
    )


class CSIReceiver:
    """Real-time UDP receiver and listener for Nexmon CSI frames."""

    def __init__(self, host: str = "0.0.0.0", port: int = DEFAULT_CSI_PORT, buffer_size: int = 4096):
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
        """Receive a single CSI packet. Returns None on timeout."""
        if not self._sock:
            self.start()
        assert self._sock is not None

        self._sock.settimeout(timeout)
        try:
            data, _addr = self._sock.recvfrom(self.buffer_size)
            return parse_csi_payload(data)
        except (socket.timeout, TimeoutError):
            return None

    def stream(self, max_packets: Optional[int] = None, timeout: Optional[float] = 2.0) -> Iterator[CSIPacket]:
        """Stream received CSI packets continuously as an iterator."""
        count = 0
        while max_packets is None or count < max_packets:
            pkt = self.recv(timeout=timeout)
            if pkt is not None:
                count += 1
                yield pkt


def main():
    """Command-line interface for capturing and displaying live CSI streams."""
    import argparse

    parser = argparse.ArgumentParser(description="Nexmon CSI Packet Receiver and Inspector")
    parser.add_argument("--port", "-p", type=int, default=DEFAULT_CSI_PORT, help="UDP listening port (default: 5500)")
    parser.add_argument("--count", "-c", type=int, default=0, help="Number of packets to capture (0 = infinite)")
    parser.add_argument("--json", action="store_true", help="Output stream as newline-delimited JSON")
    parser.add_argument("--quiet", "-q", action="store_true", help="Quiet output (show only packet count and rate)")
    args = parser.parse_args()

    print(f"Listening for Nexmon CSI frames on UDP port {args.port}...")
    with CSIReceiver(port=args.port) as rx:
        count = 0
        t0 = time.time()
        try:
            for pkt in rx.stream(max_packets=args.count if args.count > 0 else None, timeout=5.0):
                count += 1
                if args.json:
                    print(pkt.to_json())
                elif not args.quiet:
                    avg_amp = (sum(pkt.amplitudes) / len(pkt.amplitudes)) if pkt.amplitudes else 0
                    print(
                        f"[{pkt.timestamp:.4f}] #{count:04d} MAC: {pkt.src_mac} Seq: {pkt.seq:5d} "
                        f"BW: {pkt.bandwidth_mhz}MHz ({pkt.num_subcarriers} subcarriers) "
                        f"Avg Amp: {avg_amp:6.2f}"
                    )
        except KeyboardInterrupt:
            pass

        dur = max(time.time() - t0, 0.001)
        print(f"\nCaptured {count} CSI frames in {dur:.2f}s ({count / dur:.2f} frames/sec)")


if __name__ == "__main__":
    main()
