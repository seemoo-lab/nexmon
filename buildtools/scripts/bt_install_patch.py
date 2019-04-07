#!/usr/bin/python2

from pwn import *
from internalblue.adbcore import ADBCore

"""
This script needs to be appended by binary patches for Broadcom Bluetooth chips generated width Nexmon.
It is not meant to be used on its own. Make scripts will include patches below.
"""

internalblue = ADBCore()
internalblue.interface = internalblue.device_list()[0][1]  # just use the first device

# setup sockets
if not internalblue.connect():
    log.critical("No connection to target device.")
    exit(-1)

progress_log = log.info("Connected to first ADB target, installing patches...")

# GENERATED PATCHES


# shutdown connection
internalblue.shutdown()
log.info("Goodbye")
