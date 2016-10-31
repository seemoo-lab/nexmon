# nexmon
Nexmon is our C-based firmware patching framework for Broadcom/Cypress WiFi chips 
that enables you to write your own firmware patches, for example, to enable monitor
mode with radiotap headers and frame injection.

Before we started to work on this repository, we developed patches for the Nexus 5 (with bcm4339 WiFi chip) in the [bcm-public](https://github.com/seemoo-lab/bcm-public)  repository and those for the Raspberry Pi 3 (with bcm43438 WiFi chip) in the [bcm-rpi3](https://github.com/seemoo-lab/bcm-rpi3) repository. To remove the development overhead of maintaining multiple separate repositories, we decided to merge them in this repository and add support for some additional devices. In contrast to the former repositories, here, you can only build the firmware patch without drivers and kernels. The Raspberry Pi 3 makes an exception, as here it is always required to also build the driver.

# Supported Devices
WiFi Chip | Firmware Version | Used in           | Operating System | M | RT | I | FP | UC
--------- | ---------------- | ----------------- | ---------------- | - | -- | - | -- | --
bcm4330   | 5_90_100_41_sta  | Samsung Galaxy S2 | Cyanogenmod 13.0 | X | X  |   | X  | X

## Legend
M = Monitor Mode
RT = Monitor Mode with RadioTap headers
I = Frame Injection
FP = Flash Patching
UC = Ucode Compression
