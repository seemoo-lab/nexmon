![NexMon logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/nexmon.png)

# What is nexmon?
Nexmon is our C-based firmware patching framework for Broadcom/Cypress WiFi chips 
that enables you to write your own firmware patches, for example, to enable monitor
mode with radiotap headers and frame injection.

Before we started to work on this repository, we developed patches for the Nexus 5 (with bcm4339 WiFi chip) in the [bcm-public](https://github.com/seemoo-lab/bcm-public)  repository and those for the Raspberry Pi 3 (with bcm43438 WiFi chip) in the [bcm-rpi3](https://github.com/seemoo-lab/bcm-rpi3) repository. To remove the development overhead of maintaining multiple separate repositories, we decided to merge them in this repository and add support for some additional devices. In contrast to the former repositories, here, you can only build the firmware patch without drivers and kernels. The Raspberry Pi 3 makes an exception, as here it is always required to also build the driver.

# WARNING
Our software may damage your hardware and may void your hardware’s warranty! You use our tools at your own risk and responsibility! If you don't like these terms, don't use nexmon!

# Important changes
* Starting with commit 4f8697743dc46ffc37d87d960825367531baeef9 the brcmfmac driver for the RPi3 can now be used as a regular interface. You need to use nexutil to activate monitor mode (`nexutil -m2` for monitor mode with radiotap headers), which will automtically adjust the interface type.
* Starting with commit 184480edd6696392aae5f818f305f244606f2d17 you can choose different monitor mode options using nexutil. Use `nexutil -m1` to activate monitor mode without radiotap headers, `nexutil -m2` to activate it with radiotap headers. The numbers were chosen as non-Nexmon firmwares also support native monitor mode without radiotap headers by activating monitor mode with `nexutil -m1`.
* Starting with commit 1bcfdc95b4395c2e8bdd962791ae20c4ba602f5b we changed the nexutil interface. Instead of calling `nexutil -m true` to activate monitor mode, you should now write `nexutil -m1`. To get the current monitor mode state execute `nexutil -m` instead of `nexutil -n`.

# Supported Devices
The following devices are currently supported by our nexmon firmware patch.

WiFi Chip | Firmware Version | Used in           | Operating System     |  M  | RT  |  I  | FP  | UC  | CT 
--------- | ---------------- | ----------------- | -------------------- | --- | --- | --- | --- | --- | ---
bcm4330   | 5_90_100_41_sta  | Samsung Galaxy S2 | Cyanogenmod 13.0     |  X  |  X  |     |  X  |  X  |  O 
bcm4339   | 6_37_34_43       | Nexus 5           | Android 6 Stock      |  X  |  X  |  X  |  X  |  X  |  O 
bcm43438  | 7_45_41_26       | Raspberry Pi 3    | Raspbian 8           |  X  |  X  |  X  |  X  |  X  |  O 
bcm4358   | 7_112_200_17_sta | Nexus 6P          | Android 7 Stock      |  X  |  X  |     |  X  |  X  |  O 
bcm4358   | 7_112_201_3_sta  | Nexus 6P          | Android 7.1.2 Stock  |  X  |  X  |     |  X  |  X  |  O 

## Legend
- M = Monitor Mode
- RT = Monitor Mode with RadioTap headers
- I = Frame Injection
- FP = Flash Patching
- UC = Ucode Compression
- CT = c't Article Support (for consistend support, use our ct-artikel branch)

# Steps to create your own firmware patches

## Build patches for bcm4330, bcm4339 and bcm4358 using a x86 computer running Linux (e.g. Ubuntu 16.04)
* Install some dependencies: `sudo apt-get install git gawk qpdf adb flex bison`
* **Only necessary for x86_64 systems**, install i386 libs: 

  ```
  sudo dpkg --add-architecture i386
  sudo apt-get update
  sudo apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386
  ```
* Clone our repository: `git clone https://github.com/seemoo-lab/nexmon.git`
* In the root directory of the repository: `cd nexmon`
  * Setup the build environment: `source setup_env.sh`
  * Compile some build tools and extract the ucode and flashpatches from the original firmware files: `make`
* Go to the *patches* folder of your target device (e.g. bcm4339 for the Nexus 5): `cd patches/bcm4339/6_37_34_43/nexmon/`
  * Compile a patched firmware: `make`
  * Generate a backup of your original firmware file: `make backup-firmware`
  * Install the patched firmware on your smartphone: `make install-firmware` (make sure your smartphone is connected to your machine beforehand)

### Using the Monitor Mode patch
* Install at least *nexutil* and *libfakeioctl* from our utilities. The easiest way to do this is by using this app: https://play.google.com/store/apps/details?id=de.tu_darmstadt.seemoo.nexmon. But you can also build it from the source by executing `make` in the *utilties* folder (Note: you will need the Android NDK properly installed for this).
* Connect to your Android phone using the ADB tools: `adb shell`
* Make sure you are **not** connected to an access point
* Use *nexutil* to enable monitor mode: `nexutil -m2`
* At this point the monitor mode is active. There is no need to call *airmon-ng*. 
* **Important:** Most tools need a Radiotap interface to work properly. *libfakeioctl* emulates this type of interface for you, therefore, use LD_PRELOAD to load this library when you call the favourite tool (e.g. tcpdump or airodump-ng): `LD_PRELOAD=libfakeioctl.so tcpdump -i wlan0`

### Using nexutil over UDP on Nexus 5
To be able to communicate with the firmware without root priviledges, we created a UDP interface accessible through the `libnexio`, which is also used by `nexutil`. You first have to prove to the firmware that you generally have root priviledges by setting a securtiy cookie. Then you can use it for UDP based connections. Your wlan0 interface also needs an IP address in the 192.168.222.0/24 range or you have to change the default nexutil `broadcast-ip`:
* Set the IP address of the wlan0 interface: `ifconfig wlan0 192.168.222.1 netmask 255.255.255.0`
* Set the security cookie as root: `nexutil -x<cookie (uint)>`
* Start a UDP connection for example to activate monitor mode: `nexutil -X<cookie> -m1`

## Build patches for bcm43438 on the RPI3 using Raspbian 8 (recommended)
* Make sure the following commands are executed as root: `sudo su`
* Upgrade your Raspbian installation: `apt-get update && apt-get upgrade`
* Install the kernel headers to build the driver and some dependencies: `sudo apt install raspberrypi-kernel-headers git libgmp3-dev gawk qpdf`
* Clone our repository: `git clone https://github.com/seemoo-lab/nexmon.git`
* Go into the root directory of our repository: `cd nexmon`
  * Setup the build environment: `source setup_env.sh`
  * Compile some build tools and extract the ucode and flashpatches from the original firmware files: `make`
* Go to the *patches* folder for the bcm43438 chipset: `cd patches/bcm43438/7_45_41_26/nexmon/`
  * Compile a patched firmware: `make`
  * Generate a backup of your original firmware file: `make backup-firmware`
  * Install the patched firmware on your RPI3: `make install-firmware`
* Install nexutil: from the root directory of our repository switch to the nexutil folder: `cd utilities/nexutil/`. Compile and install nexutil: `make && make install`.
* *Optional*: remove wpa_supplicant for better control over the WiFi interface: `apt-get remove wpasupplicant`

### Using the Monitor Mode patch
* ~~Our modified driver sets the interface in monitor mode as soon as the interface goes up: `ifconfig wlan0 up`~~
* In the default setting the brcmfmac driver can be used regularly as a WiFi station with out firmware. To activate monitor mode, execute `nexutil -m2`.
* At this point the monitor mode is active. There is no need to call *airmon-ng*. 
* The interface already set the Radiotap header, therefore, tools like *tcpdump* or *airodump-ng* can be used out of the box: `tcpdump -i wlan0`
* *Optional*: To make the RPI3 load the modified driver after reboot:
  * Find the path of the default driver at reboot: `modinfo brcmfmac` #the first line should be the full path
  * Backup the original driver: `mv "<PATH TO THE DRIVER>/brcmfmac.ko" "<PATH TO THE DRIVER>/brcmfmac.ko.orig"`
  * Copy the modified driver: `cp /home/pi/nexmon/patches/bcm43438/7_45_41_26/nexmon/brcmfmac/brcmfmac.ko "<PATH TO THE DRIVER>/"`
  * Probe all modules and generate new dependency: `depmod -a`
  * The new driver should be loaded by default after reboot: `reboot`
  * **Note:** With this setting, you can toggle between Monitor mode and Managed mode with: `nexutil -m2` and `nexutil -m0`
  * **Note:** It is possible to connect to an access point using our modified driver and firmware, just set the wireless interface in Managed mode.

# How to extract the ROM

The Wi-Fi firmware consists of a read-only part stored in the ROM of every Wi-Fi chip and another part that is 
loaded by the driver into the RAM. To analyze the whole firmware, one needs to extract the ROM. There are two 
options to do this. Either you write a firmware patch that simply copies the contents of the ROM to RAM and then 
you dump the RAM, or you directly dump the ROM after loading the regular firmware into the RAM. Even though, 
the second option is easier, it only works, if the ROM can be directly accessed by the driver, which is not always 
the case. Additionally, the firmware loaded into RAM can contain ROM patches that overlay the data stored in ROM. 
By dumping the ROM after loading the original RAM firmware, it contains flash patches. Hence, the ROM needs to be 
dumped again for every RAM firmware update to be consistent. As a conclusion, we prefer to dump the clean ROM after 
copying it to RAM.

## Dumping the ROM directly

To dump the ROM directly, you need to know, where to find it and how large it is. On chips with Cortex-M3 it is 
usually at upper addresses such as 0x800000, while on chips with Cortex-R4 it is likely at 0x0. Run dhdutil to 
perform the dump:
```
dhdutil membytes -r 0x0 0xA0000 > rom.bin
```

## Dumping a clean ROM after copying to RAM

For the BCM4339 and BCM4358, we created `rom_extraction` projects` that load a firmware patch that copies ROM to 
RAM and them dumps it using dhdutil. To dump the ROM simply execute the following in the project directory:
```
make dump-rom
```

After ROM extraction, the `rom.bin` file will be copies to the corresponding firmwares subdirectory. To apply the 
flash patches of a specific RAM firmware version, enter its directory and execute:
```
make rom.bin
```



# Structure of this repository

* `buildtools`: Contains compilers and other tools to build the firmware
* `firmwares`
  * `<chip version>`
    * `<firmware version>`
      * `<firmware file>`: The original firmware that will be loaded into the RAM of the WiFi Chip
      * `definitions.mk`: Contains mainly firmware specific addresses
      * `structs.h`: Structures only valid for this firmware version
      * `Makefile`: Used to extract flashpatches and ucode
      * `flashpatches.c` (generated by Makefile): Contains flashpatches
      * `ucode.bin` (extracted by Makefile): Contains uncompressed Ucode
    * `structs.common.h`: Structures that are common between firmware versions
* `patches`
  * `<chip version>`
    * `<firmware version>`
      * `nexmon`
        * `Makefile`: Used to build the firmware
        * `patch.ld`: Linker file
        * `src`
          * `patch.c`: General patches to the firmware
          * `injection.c`: Code related to frame injection
          * `monitormode.c`: Code related to monitor mode with radiotap headers
          * `ioctl.c`: Handling of custom IOCTLs
          * ...
        * `obj` (generated by Makefile): Object files created from C files
        * `log` (generated by Makefile): Logs written during compilation
        * `gen` (generated by Makefile): Files generated during the build process
          * `nexmon.pre` (generated by gcc plugin): Extracted at-attributes and targetregion-pragmas
          * `nexmon.ld` (generated from nexmon.pre): Linker file use to place patch code at defined addresses in the firmware
          * `nexmon.mk` (generated from nexmon.pre): Make file used take code from patch.elf and place it into firmware
          * `flashpatches.ld` (generated from nexmon.pre): Linker file that places flashpatches at target locations in firmware ROM
          * `flashpatches.mk` (generated from nexmon.pre): Make file used to insert flashpatch config and data structures into firmware
          * `patch.elf` (generated from object files and linker scripts): contains the newly compiled code placed at predefined addresses
    * `common`
      * `wrapper.c`: Wrappers for functions that already exist in the firmware
      * `ucode_compression.c`: [tinflate](http://achurch.org/tinflate.c) based ucode decompression
      * `radiotap.c`: RadioTap header parser
      * `helper.c`: Helpful utility functions
    * `include`: Common include files
      * `firmware_version.h`: Definitions of chip and firmware versions
      * `patcher.h`: Macros use to perform patching for existing firmware code (e.g., BPatch patches a branch instruction)
      * `capabilities.h`: Allows to indicate capabilities (such as, monitor mode and frame injection)
      * `nexioctl.h`: Defines custom IOCTL numbers

# Related projects
* [bcmon](https://bcmon.blogspot.de/): Monitor Mode and Frame Injection for the bcm4329 and bcm4330
* [monmob](https://github.com/tuter/monmob): Monitor Mode and Frame Injection for the bcm4325, bcm4329 and bcm4330
* [imon](https://imon.site/): Penetration Testing for Apple devices with Broadcom WiFi Chip

# Projects using nexmon
If you know more projects that use nexmon, let us know and we will add a link.

* [Project Zero](https://googleprojectzero.blogspot.de/2017/04/over-air-exploiting-broadcoms-wi-fi_4.html): Over The Air: Exploiting Broadcom's Wi-Fi Stack (Part 1)
* [Project Zero](https://googleprojectzero.blogspot.de/2017/04/over-air-exploiting-broadcoms-wi-fi_11.html): Over The Air: Exploiting Broadcom's Wi-Fi Stack (Part 2) 

# Read our papers
* M. Schulz. [Nexmon - Wie man die eigene WLAN-Firmware hackt](http://heise.de/-3538660), 
c't 26/2016, S. 168, Heise Verlag, 2016.
* M. Schulz, D. Wegemer, M. Hollick. [DEMO: Using NexMon, the C-based WiFi 
firmware modification framework](https://dl.acm.org/citation.cfm?id=2942419), 
Proceedings of the 9th ACM Conference on Security and Privacy in Wireless and 
Mobile Networks, WiSec 2016, July 2016.
* M. Schulz, D. Wegemer and M. Hollick. [NexMon: A Cookbook for Firmware 
Modifications on Smartphones to Enable Monitor Mode]
(http://arxiv.org/abs/1601.07077), CoRR, vol. abs/1601.07077, December 2015. 
[bibtex](http://dblp.uni-trier.de/rec/bibtex/journals/corr/SchulzWH16)

# Contact
* [Matthias Schulz](https://seemoo.tu-darmstadt.de/mschulz) <mschulz@seemoo.tu-darmstadt.de>
* Daniel Wegemer <dwegemer@seemoo.tu-darmstadt.de>

# Powered By
## Secure Mobile Networking Lab (SEEMOO)
<a href="https://www.seemoo.tu-darmstadt.de">![SEEMOO logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/seemoo.png)</a>
## Networked Infrastructureless Cooperation for Emergency Response (NICER)
<a href="https://www.nicer.tu-darmstadt.de">![NICER logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/nicer.png)</a>
## Multi-Mechanisms Adaptation for the Future Internet (MAKI)
<a href="http://www.maki.tu-darmstadt.de/">![MAKI logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/maki.png)</a>
