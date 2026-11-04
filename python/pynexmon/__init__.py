"""Nexmon Native Python SDK (pynexmon).

High-performance Python interface to Broadcom Nexmon firmware ioctls,
channel math (libnexio), and Channel State Information (CSI) processing.
"""

from .nexmon import (
    NexmonDevice,
    Chanspec,
    NexmonError,
    NexmonValueError,
    NEX_READ_D11_SHM,
    NEX_WRITE_D11_SHM,
    NEX_READ_TEMPLATE_RAM,
    NEX_WRITE_TEMPLATE_RAM,
)
from .csi import CSIPacket, CSIReceiver, parse_csi_payload

__all__ = [
    "NexmonDevice",
    "Chanspec",
    "NexmonError",
    "NexmonValueError",
    "NEX_READ_D11_SHM",
    "NEX_WRITE_D11_SHM",
    "NEX_READ_TEMPLATE_RAM",
    "NEX_WRITE_TEMPLATE_RAM",
    "CSIPacket",
    "CSIReceiver",
    "parse_csi_payload",
]
__version__ = "1.1.0"
