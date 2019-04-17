#!/usr/bin/python2

from pwn import *
from internalblue.adbcore import ADBCore
from internalblue.bluezcore import BluezCore

"""
This script needs to be appended by binary patches for Broadcom Bluetooth chips generated width Nexmon.
It is not meant to be used on its own. Make scripts will include patches below.
"""

internalblue = ADBCore()
try:
    internalblue.interface = internalblue.device_list()[0][1]  # just use the first Android device
except IndexError:
    internalblue = BluezCore()
    try:
        internalblue.interface = internalblue.device_list()[0][1]  # ...or the first local HCI interface
    except IndexError:
        log.critical("Adapt the Python script to use an available Broadcom Bluetooth interface.")
        exit(-1)

# setup sockets
if not internalblue.connect():
    log.critical("No connection to target device.")
    exit(-1)

progress_log = log.info("Connected to first ADB target, installing patches...")

# GENERATED PATCHES


# shutdown connection
internalblue.shutdown()
log.info("Goodbye")
