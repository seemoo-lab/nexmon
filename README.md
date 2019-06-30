![NexMon logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/nexmon.png)

# Nexmon Software Defined Radio

This projects demonstrates our discovery that turns Broadcom's 802.11ac Wi-Fi chips into
software-defined radios that transmit arbitrary signals in the Wi-Fi bands. In this example,
we patch the Wi-Fi firmware of BCM4339 devices installed in Nexus 5 smartphones and BCM43455c0
devices installed in Raspberry Pi B3+ computers. The Raspberry Pi B3 will never be supported as
it only contains an 802.11n PHY. The firmware patch activates three ioctls:

1. `NEX_WRITE_TEMPLATE_RAM` (426) writes arbitrary data into Template RAM that stores the raw
   IQ samples that we may transmit. The ioctl's payload contains (1) an int32 value indicating
   the offset where data should be written in Template RAM in bytes, (2) an int32 value 
   indicating the length of the data that should be written and (3) the IQ samples as array of
   IQ values, where I (inphase components) and Q (quadrature components) are stored as int16 
   numbers.

2. `NEX_SDR_START_TRANSMISSION` (427) that triggers the transmission of IQ samples. The ioctl's
    payload contains (1) an int32 value indicating the number of samples to transmit, (2) an 
    int32 value indicating the offset where the signal starts in Template RAM, (3) an int32 
    value indicating a chanspec (channel number, bandwidth, band, ...), (4) an int32 value
    indicating the power index (lower value means higher output power), and (5) an int32
    value indicating whether to loop over the IQ samples or transmit them only once.

3. `NEX_SDR_STOP_TRANSMISSION` (428) stops a transmission started using 
   `NEX_SDR_START_TRANSMISSION`.

# Transmitting an Example Signal

The directory `payload_generation` contains the MATLAB script `generate_frame.m` that generates
a Wi-Fi beacon frame with SSID `MyCovertChannel`. The generated IQ samples are written to a bash
script that calls `nexutil` from the nexmon.org project to load the samples into the Wi-Fi 
chip's Template RAM by using ioctls. You can either generate your own signals or use the
example `myframe.sh` file for transmitting the generated Wi-Fi frame. To this end, follow the
Getting Started instructions below to install our patched Wi-Fi firmware on a Nexus 5 smartphone.
Then, you need to copy `myframe.sh` to a directory that allows execution (such as `/su/xbin/`).
To load the samples and start a single transmission, simply executute the bash script and 
observe the results by listening with a Wi-Fi sniffer on channel 1. A suitable Wireshark filter
is `wlan.addr == 82:7b:be:f0:96:e0`. Of course, you are not limited to transmitting handcrafted
Wi-Fi signals, you can transmit whatever you like in the 2.4 and 5 GHz bands. Nevertheless, you
have to obey your local laws for transmitting signals, that might prohibit you to transmit any
signal at all.

# Extract from our License

Any use of the Software which results in an academic publication or
other publication which includes a bibliography must include
citations to the nexmon project a) and the paper cited under b) or 
the thesis cited under c):

   a) "Matthias Schulz, Daniel Wegemer and Matthias Hollick. Nexmon:
       The C-based Firmware Patching Framework. https://nexmon.org"

   b) "Matthias Schulz, Jakob Link, Francesco Gringoli, and Matthias 
       Hollick. Shadow Wi-Fi: Teaching Smartphones to Transmit Raw 
       Signals and to Extract Channel State Information to Implement 
       Practical Covert Channels over Wi-Fi. Accepted to appear in 
       Proceedings of the 16th ACM International Conference on Mobile 
       Systems, Applications, and Services (MobiSys 2018), June 2018."

   c) "Matthias Schulz. Teaching Your Wireless Card New Tricks: 
       Smartphone Performance and Security Enhancements through Wi-Fi
       Firmware Modifications. Dr.-Ing. thesis, Technische Universität
       Darmstadt, Germany, February 2018."

# Getting Started

To compile the source code, you are required to first clone the original nexmon repository 
that contains our C-based patching framework for Wi-Fi firmwares. Than you clone this 
repository as one of the sub-projects in the corresponding patches sub-directory. This 
allows you to build and compile all the firmware patches required to repeat our experiments.
The following steps will get you started on Xubuntu 16.04 LTS:

1. Install some dependencies: `sudo apt-get install git gawk qpdf adb`
2. **Only necessary for x86_64 systems**, install i386 libs: 

  ```
  sudo dpkg --add-architecture i386
  sudo apt-get update
  sudo apt-get install libc6:i386 libncurses5:i386 libstdc++6:i386
  ```
3. Clone the nexmon base repository: `git clone https://github.com/seemoo-lab/nexmon.git`.
4. Download and extract Android NDK r11c (use exactly this version!).
5. Export the NDK_ROOT environment variable pointing to the location where you extracted the 
   ndk so that it can be found by our build environment.
6. Navigate to the previously cloned nexmon directory and execute `source setup_env.sh` to set 
   a couple of environment variables.
7. Run `make` to extract ucode, templateram and flashpatches from the original firmwares.
8. Navigate to utilities and run `make` to build all utilities such as nexmon.
9. Attach your rooted Nexus 5 smartphone running stock firmware version 6.0.1 (M4B30Z, Dec 2016).
10. Run `make install` to install all the built utilities on your phone.
11. Navigate to patches/bcm4339/6_37_34_43/ and clone this repository: 
    `git clone https://github.com/seemoo-lab/mobisys2018_nexmon_software_defined_radio.git`
12. Enter the created subdirectory mobisys2018_nexmon_software_defined_radio and run 
    `make install-firmware` to compile our firmware patch and install it on the attached Nexus 5 
    smartphone or run `make install-rpi3plus` to compile our firmware patch and install it on
    a Raspberry Pi B3+.

# References

* Matthias Schulz, Daniel Wegemer and Matthias Hollick. **Nexmon: The C-based Firmware Patching 
  Framework**. https://nexmon.org
* Matthias Schulz, Jakob Link, Francesco Gringoli, and Matthias Hollick. **Shadow Wi-Fi: Teaching 
  Smartphones to Transmit Raw Signals and to Extract Channel State Information to Implement 
  Practical Covert Channels over Wi-Fi**. Accepted to appear in *Proceedings of the 16th ACM 
  International Conference on Mobile Systems, Applications, and Services*, MobiSys 2018, June 2018.
* Matthias Schulz. **Teaching Your Wireless Card New Tricks: Smartphone Performance and Security 
  Enhancements through Wi-Fi Firmware Modifications**. Dr.-Ing. thesis, Technische Universität
  Darmstadt, Germany, February 2018.

[Get references as bibtex file](https://nexmon.org/bib)

# Contact

* [Matthias Schulz](https://seemoo.tu-darmstadt.de/mschulz) <mschulz@seemoo.tu-darmstadt.de>

# Powered By

## Secure Mobile Networking Lab (SEEMOO)
<a href="https://www.seemoo.tu-darmstadt.de">![SEEMOO logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/seemoo.png)</a>
## Networked Infrastructureless Cooperation for Emergency Response (NICER)
<a href="https://www.nicer.tu-darmstadt.de">![NICER logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/nicer.png)</a>
## Multi-Mechanisms Adaptation for the Future Internet (MAKI)
<a href="http://www.maki.tu-darmstadt.de/">![MAKI logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/maki.png)</a>
## Technische Universität Darmstadt
<a href="https://www.tu-darmstadt.de/index.en.jsp">![TU Darmstadt logo](https://github.com/seemoo-lab/nexmon/raw/master/gfx/tudarmstadt.png)</a>
