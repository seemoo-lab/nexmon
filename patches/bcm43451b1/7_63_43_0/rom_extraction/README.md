# ROM Extraction Patch for iPhone 6

The ROM extraction patch allows to dump the ROM of the BCM43451b1 Wi-Fi chip installed in the iPhone 6. To dump the ROM, you need a jailbroken iPhone 6 running iOS 10.0.2, 10.1.1 or some similar version that comes with a preinstalled Wi-Fi firmware (found at /usr/share/firmware/wifi/C-4345__s-B1/tempranillo.trx on the iPhone) with version 7.63.43.0. Others might also work, but is is not tested. Additionally, `nexutil` needs to be installed on the iPhone to send ioctls to the Wi-Fi firmware. A 64-bit compatible version can be build from the `$NEXMON_ROOT/ios_utilities/nexutil` directory using [theos](https://github.com/theos/theos) and iPhoneOS 9.3 SDK.

The ROM extraction patch adds additional ioctls to the Wi-Fi firmware to dump arbitrary memory locations. To dump the first 1024 bytes of the ROM, one may execute: `nexutil -g0x602 -l1024 -i -v0x0 -r > /var/root/romdump.bin`. As the buffer length for each ioctl is limited, we need to call the ioctl multiple times with different start addresses passed through `-v<address>`. The resulting ROM dump will extract the clean ROM of the Wi-Fi chip by automatically reverting flash patches after reading from patched memory locations. This allows us to have one clean ROM version to which we can manually apply flash patches using our `fpext` utility. This avoids having to extract the ROM again, whenever the RAM firmware version changes.

To actually extract the ROM, you first need to install the patched firmware by executing `make install-firmware` and then you can dump the clean ROM to `rom.bin` by executing `make dump-rom`.

# Accessing the console

On iPhones, the Wi-Fi chips internal console writes to a UART interface. In the past the console output was supposed to show up in the /dev/uart.wlan-debug device, however, it does not exist on iOS 10 on the iPhone 6 and we were not able to read anything from /dev/uart.wlan. Hence, we decided to remove the `printf` flash patch from our patched firmware and simple write to the chips internal console buffer. To dump this buffer, we added the ioctl `0x605` that can be used in the following way: `nexutil -g0x605 -l0x400 -r`, where `0x400` is the default console length. If you want to write to the console, you can either call the `printf` function in the firmware or use the ioctl `0x604`. For example: `nexutil -s0x604 -l100 -v"Hello World"` leads to the output:

```
RTE (SDIO-MSG_BUF) 7.63.43 (r653088) on BCM4345 r5 @ 37.4/160.5/160.5MHz
000000.001 EL: 30 94 14
000000.005 allocating a max of 255 rxcplid buffers
000000.005 pciemsgbuf0: Broadcom PCIE MSGBUF driver
000000.140 reclaim section 0: Returned 39508 bytes to the heap
000000.195 wl0: wlc_channels_commit: no valid channel for "#n" nbands 2 bandlocked 0
000000.204 wl0: Broadcom BCM4345 802.11 Wireless Controller 7.63.43 (r653088)
000000.205 TCAM: 256 used: 183 exceed:0
000000.206 reclaim section 1: Returned 83352 bytes to the heap
000000.207 reclaim section boot-patch: Returned 2048 bytes to the heap
000000.475 EL: 30 200
000000.543 wl0: wlc_enable_probe_req: state down, deferring setting of host flags
000000.640 wl0: link up (wl0.3)
000004.389 Hello World
000004.776 wl0: link up (wl0)
```

# Using precompiled tools

If you only want to extract the ROM without having to compile the firmware patch and nexutil on your own, you can simply use our precompiled binaries from https://github.com/seemoo-lab/nexmon/releases/download/2.2.2/rom_extraction.zip. Keep in mind to execute `ldid -Sent.xml /usr/bin/nexutil` to give nexutil the entitlement to control the Wi-Fi interface.
