![NexMon logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/nexmon.png)

# What is nexmon?
Nexmon is our C-based firmware patching framework for Broadcom/Cypress WiFi chips 
that enables you to write your own firmware patches, for example, to enable monitor
mode with radiotap headers and frame injection.

Below, you find an overview what is possible with nexmon. This repository mainly
focuses on enabling monitor mode and frame injection on many chips. If you want
additional features, the following projects might be interesting for you:

* http://nexmon.org/jammer: A real Wi-Fi jammer that allows to overlay ongoing frame transmissions with an arbitrary jamming signal.
  * It uses the Wi-Fi chip as a Software-defined Radio to generate jamming signals
  * It allows using non-standard channels such as 80 MHz bandwidth in the 2.4 GHz bands
  * It allows to set arbitrary transmission powers
  * It allows patching the D11 core's real-time MAC implementation
* http://nexmon.org/csi: Channel State Information extractor for various Wi-Fi chips
  * It allows to extract CSI of up to 4x4 MIMO transmissions at 80 MHz bandwidth
* http://nexmon.org/debugger: Debugging ARM microcontrollers without JTAG access
  * It allows low-level access to debugging registers to set breakpoints and watchpoints and allows single stepping
* http://nexmon.org/covert_channel: Covert Channel that hides information in Wi-Fi signals
  * More advanced Software-defined Radio capabilities than the jammer
  * Example application for channel state information extraction
* http://nexmon.org/sdr: Use your Wi-Fi chip as Software-defined Radio
  * Currently only transmissions are working in both 2.4 and 5 GHz Wi-Fi bands

![NexMon logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/nexmon_overview.svg)

# WARNING
Our software may damage your hardware and may void your hardware’s warranty! You use our tools at your own risk and responsibility! If you don't like these terms, don't use nexmon!

# Supported Devices
The following devices are currently supported by our nexmon firmware patch.

WiFi Chip                 | Firmware Version     | Used in                   | Operating System             |  M  | RT  |  I  | FP  | UC  | CT 
------------------------- | -------------------- | ------------------------- | ---------------------------- | --- | --- | --- | --- | --- | ---
bcm4330                   | 5_90_100_41_sta      | Samsung Galaxy S2         | Cyanogenmod 13.0             |  X  |  X  |     |  X  |  X  |  O 
bcm4335b0                 | 6.30.171.1_sta       | Samsung Galaxy S4         | LineageOS 14.1               |  X  |  X  |  X  |     |  X  |  O 
bcm4339                   | 6_37_34_43           | Nexus 5                   | Android 6 Stock              |  X  |  X  |  X  |  X  |  X  |  O 
bcm43430a1<sup>1</sup>    | 7_45_41_26           | Raspberry Pi 3 and Zero W | Raspbian 8                   |  X  |  X  |  X  |  X  |  X  |  O 
bcm43430a1<sup>1</sup>    | 7_45_41_46           | Raspberry Pi 3 and Zero W | Raspbian Stretch             |  X  |  X  |  X  |  X  |  X  |  O 
bcm43451b1                | 7_63_43_0            | iPhone 6                  | iOS 10.1.1 (14B100)          |     |     |     |  X  |  X  |    
bcm43455                  | 7_45_77_0_hw         | Huawei P9                 | Android 7 Stock              |  X  |  X  |  X  |  X  |  X  |    
bcm43455                  | 7_120_5_1_sta_C0     | Galaxy J7 2017            | ?                            |     |     |     |  X  |  X  |    
bcm43455                  | 7_45_77_0_hw(8-2017) | Huawei P9                 | Android 7 Stock              |  X  |  X  |  X  |  X  |  X  |    
bcm43455<sup>5</sup>      | 7_46_77_11_hw        | Huawei P9                 | Android 8 China Stock        |  X  |  X  |  X  |  X  |  X  |    
bcm43455                  | 7_45_59_16           | Sony Xperia Z5 Compact    | LineageOS 14.1               |  X  |  X  |  X  |  X  |  X  |    
bcm43455c0                | 7_45_154             | Raspberry Pi B3+/B4       | Raspbian Kernel 4.9/14/19    |  X  |  X  |     |  X  |  X  |    
bcm43455c0                | 7_45_189             | Raspberry Pi B3+/B4       | Raspbian Kernel 4.14/19, 5.4 |  X  |  X  |     |  X  |  X  |    
bcm43455c0                | 7_45_206             | Raspberry Pi B3+/B4       | Raspberry Pi OS Kernel 5.4   |  X  |  X  |  X  |  X  |  X  |    
bcm4356                   | 7_35_101_5_sta       | Nexus 6                   | Android 7.1.2                |  X  |  X  |     |  X  |  X  |  O 
bcm4358                   | 7_112_200_17_sta     | Nexus 6P                  | Android 7 Stock              |  X  |  X  |     |  X  |  X  |  O 
bcm4358                   | 7_112_201_3_sta      | Nexus 6P                  | Android 7.1.2 Stock          |  X  |  X  |     |  X  |  X  |  O 
bcm4358<sup>2</sup>       | 7_112_300_14_sta     | Nexus 6P                  | Android 8.0.0 Stock          |  X  |  X  |  X  |  X  |  X  |  O 
bcm43596a0<sup>3</sup>    | 9_75_155_45_sta_c0   | Samsung Galaxy S7         | Android 7 Stock              |  X  |     |     |  O  |  X  |    
bcm43596a0<sup>3,2</sup>  | 9_96_4_sta_c0        | Samsung Galaxy S7         | LineageOS 14.1               |  X  |  X  |  X  |  O  |  X  |    
bcm4375b1<sup>3,5,6</sup> | 18_38_18_sta         | Samsung Galaxy S10        | Rooted + disabled SELinux    |  X  |  X  |  X  |  O  |  X  |    
bcm4375b1<sup>3,5,6</sup> | 18_41_8_9_sta        | Samsung Galaxy S20        | Rooted + disabled SELinux    |  X  |  X  |  X  |  O  |  X  |    
qca9500<sup>4</sup>       | 4-1-0_55             | TP-Link Talon AD7200      | Custom LEDE Image            |     |     |     |     |     |    

<sup>1</sup> bcm43430a1 was wrongly labeled bcm43438 in the past.

<sup>2</sup> use LD_PRELOAD=libnexmon.so instead of LD_PRELOAD=libfakeioctl.so to inject frames through ioctls

<sup>3</sup> flash patches need to be 8 bytes long and aligned on an 8 byte boundary

<sup>4</sup> 802.11ad Wi-Fi chip from first 60 GHz Wi-Fi router Talon AD7200. Patch your firmware using [nexmon-arc](https://github.com/seemoo-lab/nexmon-arc) and run it with our custom LEDE image [lede-ad7200](https://github.com/seemoo-lab/lede-ad7200)

<sup>5</sup> Disabled the execution protection (called Execute Never) on region 1, because it interferes with the nexmon code (Permission fault on Section)

<sup>6</sup> To use nexutil, you need to deactivate SELinux or set it to permissive

## Legend
- M = Monitor Mode
- RT = Monitor Mode with RadioTap headers
- I = Frame Injection
- FP = Flash Patching
- UC = Ucode Compression
- CT = c't Article Support (for consistent support, use our ct-artikel branch)

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
* Install at least *nexutil* and *libfakeioctl* from our utilities. The easiest way to do this is by using this app: https://nexmon.org/app. But you can also build it from the source by executing `make` in the *utilties* folder (Note: you will need the Android NDK properly installed for this).
* Connect to your Android phone using the ADB tools: `adb shell`
* Make sure you are **not** connected to an access point
* Use *nexutil* to enable monitor mode: `nexutil -m2`
* At this point the monitor mode is active. There is no need to call *airmon-ng*. 
* **Important:** Most tools need a Radiotap interface to work properly. *libfakeioctl* emulates this type of interface for you, therefore, use LD_PRELOAD to load this library when you call the favourite tool (e.g. tcpdump or airodump-ng): `LD_PRELOAD=libfakeioctl.so tcpdump -i wlan0`
* *untested hint:* Thanks to XDA member ruleh, there is a bcmdhd driver patch to activate native monitor mode, see: https://github.com/ruleh/misc/tree/master/monitor

### Using nexutil over UDP on Nexus 5
To be able to communicate with the firmware without root priviledges, we created a UDP interface accessible through the `libnexio`, which is also used by `nexutil`. You first have to prove to the firmware that you generally have root priviledges by setting a security cookie. Then you can use it for UDP based connections. Your wlan0 interface also needs an IP address in the 192.168.222.0/24 range or you have to change the default nexutil `broadcast-ip`:
* Set the IP address of the wlan0 interface: `ifconfig wlan0 192.168.222.1 netmask 255.255.255.0`
* Set the security cookie as root: `nexutil -x<cookie (uint)>`
* Start a UDP connection for example to activate monitor mode: `nexutil -X<cookie> -m1`

## Build patches for bcm43430a1 on the RPI3/Zero W or bcm434355c0 on the RPI3+/RPI4 using Raspbian/Raspberry Pi OS (recommended)
**Note:** We currently support Kernel Version 4.4 (deprecated), 4.9, 4.14, 4.19 and 5.4. Raspbian contains firmware version 7.45.154 for the bcm43455c0. We also support the newer firmware release 7.45.189 from Cypress. Raspberry Pi OS contains firmware version 7.45.206. Please, try which works best for you.
* Make sure the following commands are executed as root: `sudo su`
* Upgrade your Raspbian installation: `apt-get update && apt-get upgrade`
* Install the kernel headers to build the driver and some dependencies: `sudo apt install raspberrypi-kernel-headers git libgmp3-dev gawk qpdf bison flex make`
* Clone our repository: `git clone https://github.com/seemoo-lab/nexmon.git`
* Go into the root directory of our repository: `cd nexmon`
* Check if `/usr/lib/arm-linux-gnueabihf/libisl.so.10` exists, if not, compile it from source:
  * `cd buildtools/isl-0.10`, `./configure`, `make`, `make install`, `ln -s /usr/local/lib/libisl.so /usr/lib/arm-linux-gnueabihf/libisl.so.10`
* Check if `/usr/lib/arm-linux-gnueabihf/libmpfr.so.4` exists, if not, compile it from source:
  * `cd buildtools/mpfr-3.1.4`, `autoreconf -f -i`, `./configure`, `make`, `make install`, `ln -s /usr/local/lib/libmpfr.so /usr/lib/arm-linux-gnueabihf/libmpfr.so.4`
* Then you can setup the build environment for compiling firmware patches
  * Setup the build environment: `source setup_env.sh`
  * Compile some build tools and extract the ucode and flashpatches from the original firmware files: `make`
* Go to the *patches* folder for the bcm43430a1/bcm43455c0 chipset: `cd patches/bcm43430a1/7_45_41_46/nexmon/` / `patches/bcm43455c0/<7_45_154 or 7_45_189>/nexmon/`
  * Compile a patched firmware: `make`
  * Generate a backup of your original firmware file: `make backup-firmware`
  * Install the patched firmware on your RPI3: `make install-firmware`
* Install nexutil: from the root directory of our repository switch to the nexutil folder: `cd utilities/nexutil/`. Compile and install nexutil: `make && make install`.
* *Optional*: remove wpa_supplicant for better control over the WiFi interface: `apt-get remove wpasupplicant`
* **Note:** To connect to regular access points you have to execute `nexutil -m0` first

### Using the Monitor Mode patch
* Thanks to the prior work of Mame82, you can setup a new monitor mode interface by executing:
```iw phy `iw dev wlan0 info | gawk '/wiphy/ {printf "phy" $2}'` interface add mon0 type monitor```
* To activate monitor mode in the firmware, simply set the interface up: `ifconfig mon0 up`.
* At this point, monitor mode is active. There is no need to call *airmon-ng*. 
* The interface already set the Radiotap header, therefore, tools like *tcpdump* or *airodump-ng* can be used out of the box: `tcpdump -i mon0`
* *Optional*: To make the RPI3 load the modified driver after reboot:
  * Find the path of the default driver at reboot: `modinfo brcmfmac` #the first line should be the full path
  * Backup the original driver: `mv "<PATH TO THE DRIVER>/brcmfmac.ko" "<PATH TO THE DRIVER>/brcmfmac.ko.orig"`
  * Copy the modified driver (Kernel 4.9): `cp /home/pi/nexmon/patches/bcm43430a1/7_45_41_46/nexmon/brcmfmac_kernel49/brcmfmac.ko "<PATH TO THE DRIVER>/"`
  * Copy the modified driver (Kernel 4.14): `cp /home/pi/nexmon/patches/bcm43430a1/7_45_41_46/nexmon/brcmfmac_4.14.y-nexmon/brcmfmac.ko "<PATH TO THE DRIVER>/"`
  * Probe all modules and generate new dependency: `depmod -a`
  * The new driver should be loaded by default after reboot: `reboot`
  * **Note:** It is possible to connect to an access point or run your own access point in parallel to the monitor mode interface on the `wlan0` interface.

# How to build the utilities
To build the utilities such as nexmon or dhdutil for Android, you need to download the **old** NDK version 11c,
extract it and export the environment variable `NDK_ROOT` pointing to the directory where you extracted the NDK 
files.

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
For the BCM4339 and BCM4358, we created `rom_extraction` projects that load a firmware patch that copies ROM to 
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
* [P4wnP1](https://github.com/mame82/P4wnP1): Highly customizable attack platform, based on Raspberry Pi Zero W and Nexmon
* [kali Nethunter OS](https://github.com/nethunteros): ROM that brings Kali Linux to smartphones with Nexmon support
* [dustcloud-nexmon](https://github.com/dgiese/dustcloud-nexmon): Nexmon for Xiaomi IoT devices (ARM based)
* [InternalBlue](https://github.com/seemoo-lab/internalblue): Bluetooth experimentation framework based on Reverse Engineering of Broadcom Bluetooth Controllers

# Interesting articles on firmware hacks
If you know more projects that use nexmon or perform similar firmware hacks, let us know and we will add a link.

* [Project Zero](https://googleprojectzero.blogspot.de/2017/09/over-air-vol-2-pt-1-exploiting-wi-fi.html): Over The Air - Vol. 2, Pt. 1: Exploiting The Wi-Fi Stack on Apple Devices
* [broadpwn](https://blog.exodusintel.com/2017/07/26/broadpwn/): Remotely Compromising Android and IOS via a Bug in Broadcom's Wi-Fi Chipsets
* [Project Zero](https://googleprojectzero.blogspot.de/2017/04/over-air-exploiting-broadcoms-wi-fi_4.html): Over The Air: Exploiting Broadcom's Wi-Fi Stack (Part 1)
* [Project Zero](https://googleprojectzero.blogspot.de/2017/04/over-air-exploiting-broadcoms-wi-fi_11.html): Over The Air: Exploiting Broadcom's Wi-Fi Stack (Part 2) 

# Read my PhD thesis
* Matthias Schulz. [**Teaching Your Wireless Card New Tricks: Smartphone Performance and Security Enhancements through Wi-Fi Firmware Modifications**](http://tuprints.ulb.tu-darmstadt.de/7243/). Dr.-Ing. thesis, Technische Universität Darmstadt, Germany, February 2018. [pdf](http://tuprints.ulb.tu-darmstadt.de/7243/7/dissertation_2018_matthias_thomas_schulz.pdf)

# Read our papers
* F. Gringoli, M. Schulz, J. Link, and M. Hollick. [**Free Your CSI: A Channel State Information Extraction Platform For Modern Wi-Fi Chipsets**](https://doi.org/10.1145/3349623.3355477). Accepted to appear in *Proceedings of the 13th Workshop on Wireless Network Testbeds, Experimental evaluation & CHaracterization (WiNTECH 2019)*, October 2019. [code](https://nexmon.org/csi)
* D. Mantz, J. Classen, M. Schulz, and M. Hollick. [**InternalBlue - Bluetooth Binary Patching and Experimentation Framework**](https://dl.acm.org/citation.cfm?id=3326089). *In Proceedings of the 17th Annual International Conference on Mobile Systems, Applications, and Services (MobiSys '19)*. June 2019.
* M. Schuß, C. A. Boano, M. Weber, M. Schulz, M. Hollick, K. Römer. [**JamLab-NG: Benchmarking Low-Power Wireless Protocols under Controlable and Repeatable Wi-Fi Interference**](https://dl.acm.org/citation.cfm?id=3324331). *Proceedings of the 2019 International Conference on Embedded Wireless Systems and Networks (EWSN 2019)*, February 2019.
* M. Schulz, D. Wegemer, and M. Hollick. [**The Nexmon Firmware Analysis and Modification Framework: Empowering Researchers to Enhance Wi-Fi Devices**](https://doi.org/10.1016/j.comcom.2018.05.015). *Elsevier Computer Communications (COMCOM) Journal*. 2018.
* M. Schulz, J. Link, F. Gringoli, and M. Hollick. [**Shadow Wi-Fi: Teaching Smart- phones to Transmit Raw Signals and to Extract Channel State Information to Implement Practical Covert Channels over Wi-Fi**](https://dl.acm.org/citation.cfm?id=3210333). Accepted to appear in *Proceedings of the 16th ACM International Conference on Mobile Systems, Applications, and Services*, MobiSys 2018, June 2018.
* D. Steinmetzer, D. Wegemer, M. Schulz, J. Widmer, M. Hollick. [**Compressive Millimeter-Wave Sector Selection in Off-the-Shelf IEEE 802.11ad Devices**](https://dl.acm.org/citation.cfm?id=3143384). *Proceedings of the 13th International Conference on emerging Networking EXperiments and Technologies*, CoNEXT 2017, December 2017.
* M. Schulz, D. Wegemer, M. Hollick. [**Nexmon: Build Your Own Wi-Fi Testbeds With Low-Level MAC and PHY-Access Using Firmware Patches on Off-the-Shelf Mobile Devices**](https://dl.acm.org/citation.cfm?id=3131476). *Proceedings of the 11th ACM International Workshop on Wireless Network Testbeds, Experimental Evaluation & Characterization (WiNTECH 2017)*, October 2017. [pdf](https://www.seemoo.tu-darmstadt.de/mschulz/wintech2017) [video](https://youtu.be/m5Zrk4n4hoE)
* M. Schulz, F. Knapp, E. Deligeorgopoulos, D. Wegemer, F. Gringoli, M. Hollick. [**DEMO: Nexmon in Action: Advanced Applications Powered by the Nexmon Firmware Patching Framework**](https://dl.acm.org/citation.cfm?id=3133333), Accepted for publication in *Proceedings of the 11th ACM International Workshop on Wireless Network Testbeds, Experimental Evaluation & Characterization (WiNTECH 2017)*, October 2017. [pdf](https://www.seemoo.tu-darmstadt.de/mschulz/wintech2017demo)
* M. Schulz, F. Gringoli, D. Steinmetzer, M. Koch and M. Hollick. [**Massive Reactive Smartphone-Based Jamming using Arbitrary Waveforms and Adaptive Power Control**](https://dl.acm.org/citation.cfm?id=3098253). Proceedings of the *10th ACM Conference on Security and Privacy in Wireless and Mobile Networks (WiSec 2017)*, July 2017. [pdf](https://www.seemoo.tu-darmstadt.de/mschulz/wisec2017) [video](https://youtu.be/S2XPBK0KdiQ)
* M. Schulz, E. Deligeorgopoulos, M. Hollick and F. Gringoli. [**DEMO: Demonstrating Reactive Smartphone-Based Jamming**](https://dl.acm.org/citation.cfm?id=3106022). Proceedings of the *10th ACM Conference on Security and Privacy in Wireless and Mobile Networks (WiSec 2017)*, July 2017. [pdf](https://www.seemoo.tu-darmstadt.de/mschulz/wisec2017demo)
* M. Schulz. [**Nexmon - Wie man die eigene WLAN-Firmware hackt**](http://heise.de/-3538660), 
c't 26/2016, S. 168, Heise Verlag, 2016.
* M. Schulz, D. Wegemer, M. Hollick. [**DEMO: Using NexMon, the C-based WiFi 
firmware modification framework**](https://dl.acm.org/citation.cfm?id=2942419), 
Proceedings of the *9th ACM Conference on Security and Privacy in Wireless and 
Mobile Networks (WiSec 2016)*, July 2016. [pdf](https://www.seemoo.tu-darmstadt.de/mschulz/wisec2016demo1)
* M. Schulz, D. Wegemer and M. Hollick. [**NexMon: A Cookbook for Firmware 
Modifications on Smartphones to Enable Monitor Mode**](http://arxiv.org/abs/1601.07077), 
CoRR, vol. abs/1601.07077, December 2015. 
[bibtex](http://dblp.uni-trier.de/rec/bibtex/journals/corr/SchulzWH16)

[Get references as bibtex file](https://nexmon.org/bib)

# Reference our project
Any use of this project which results in an academic publication or other publication which includes a bibliography should include a citation to the Nexmon project and probably one of our papers depending on the code you use. Find all references in our [bibtex file](nexmon.bib). Here is the reference for the project only:
```
@electronic{nexmon:project,
	author = {Schulz, Matthias and Wegemer, Daniel and Hollick, Matthias},
	title = {Nexmon: The C-based Firmware Patching Framework},
	url = {https://nexmon.org},
	year = {2017}
}
```

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
## Technische Universität Darmstadt
<a href="https://www.tu-darmstadt.de/index.en.jsp">![TU Darmstadt logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/tudarmstadt.png)</a>
