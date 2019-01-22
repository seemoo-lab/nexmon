## Bluetooth Firmware Patching

#### What?
Within this directory you can find projects to generate HCD patch files for broadcom bluetooth chips.

#### Why?
This way you can extend and manipulate the firmware on a pretty low layer.

### HCD patching vs live patching
One possibility to patch the bluetooth chip' firmware is the *live patching mechanism*. A project to use this mechanism can be found in the [seemoo-lab/internalblue repository](https://github.com/seemoo-lab/internalblue). Patches applied using internalblue do only last until the next restart of the bluetooth-chip and they must be written in arm-assembly. Since this is problematic if you have to restart your Bluetooth chip and writing assembly code is hard, there's also this project providing HCD file patching.

When the bluetooth chip is booting it searches for `.hcd` files. Those files contain patching code which is then executed and loaded into the firmware. This project contains buildtools and code to write patches, which then will be integrated into hcd files to be loaded at boot time of the bluetooth chip. Unlike the *live patching mechanism* are these patches permanent and the patches can be written using C / C++.

### Build patches for bcm4330, bcm4339 and bcm4358 using a x86 computer running Linux (e.g. Ubuntu 16.04)
* Install some dependencies:
  On Ubuntu / Debian based systems:
  ``` 
  sudo apt-get install git gawk qpdf adb flex bison
  ```
  
  On Arch based systems (to be checked):
  ```
  yay -S git gawk qpdf adb flex bison
  ```
* **Only necessary for x86_64 systems**, install i386 libs: 
  On Ubuntu / Debian based systems:
  ```
  sudo dpkg --add-architecture i386
  sudo apt-get update
  sudo apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386
  ```
  On Arch based systems:
  ```
  # Enable multilib in the 
  # /etc/pacman.conf
  # by uncommenting the following two lines:
  # [multilib]
  # Include = /etc/pacman.d/mirrorlist
  sudo pacman -Syu
  # lib32-gcc-libs contains lib32-libstdc++6
  # vim contains xxd
  sudo pacman -S lib32-glibc lib32-ncurses lib32-gcc-libs vim
  ```
* Set up the [seemoo-labl/internalblue](https://github.com/seemoo-lab/internalblue) project and install its cli. This is needed while compiling patches to non 4-byte aligned addresses.
* Clone our repository: `git clone https://github.com/seemoo-lab/nexmon.git`
* Go to the root directory of the repository: `cd nexmon`
  * Switch to the bluetooth branch: `git checkout bluetooth-wip`
  * Setup the build environment: `source setup_env.sh`
  * Compile some build tools and extract the ucode and flashpatches from the original firmware files: `make`
* Go to the *patches* folder of your target device (e.g. bcm4335c0 for the Nexus 5): `cd patches/bluetooth/bcm4335c0/nexmon`
  * Setup the specific build environment: `source setup_env.sh`
  * If you want to patch non 4-byte aligned addresses, make sure your smartphone is connected to your machine and [internalblue](https://github.com/seemoo-lab/internalblue) is accessible.
  * Compile a patched firmware: `make`
  * Generate a backup of your original firmware file: `make backup-hcd`
  * Install the patched firmware on your smartphone: `make install-patch` (make sure your smartphone is connected to your machine beforehand)
  * To reinstall the backed up hcd: `make install-backup`

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

