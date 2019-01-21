## Bluetooth Firmware Patching

### What?
Within this directory you can find projects to generate HCD patch files for broadcom bluetooth chips.

### Why?
This way you can extend and manipulate the firmware on a pretty low layer.

#### HCD patching vs live patching
One possibility to patch the bluetooth chip' firmware is the *live patching mechanism*. A project to use this mechanism can be found in the [seemoo-lab/internalblue repository](https://github.com/seemoo-lab/internalblue). Patches applied using internalblue do only last until the next restart of the bluetooth-chip and they must be written in arm-assembly. Since this is problematic if you have to restart your Bluetooth chip and writing assembly code is hard, there's also this project providing HCD file patching.

When the bluetooth chip is booting it searches for `.hcd` files. Those files contain patching code which is then executed and loaded into the firmware. This project contains buildtools and code to write patches, which then will be integrated into hcd files to be loaded at boot time of the bluetooth chip. Unlike the *live patching mechanism* are these patches permanent and the patches can be written using C / C++.

### Where to find what?

#### Buildtools
base path: `nexmon/buildtools`

**hcd-extractor:** `nexmon/buildtools/hcd-extractor`

This tool parses and extracts patches from an existing `.hcd`-file. The extracted patches are later used to generate another - valid - patch file. 

**hcd-generator:** `nexmon/buildtools/hcd-generator`

The extracted patches, as well as the user written ones are merged together into a valid hcd file using this tool.

**nexmon.mk.2_bt.awk:** `nexmon/buildtools/scripts/nexmon.mk.2_bt.awk`

This awk-script is used to generate a proper `MAKEFILE` under consideration that we don't build a WiFi firmware patch, but a bluetooth one.

#### Firmware

**BCM 4330c0:**
- firmware: `nexmon/firmware/bcm433c0_BT/` 

#### Common Code
**common between all nexmon projects:**

- common  - source/c-files: `nexmon/patches/common`
- include - source/h-files: `nexmo/patches/include`

**common between all nexmon-bluetooth projects:**

- common  - source/c-files: `nexmon/patches/bluetooth/common`
- include - source/h-files: `nexmon/patches/bluetooth/include`

#### Yet Provided Patches

patch sources: `nexmon/patches/blueooth/bcm4335c0`

