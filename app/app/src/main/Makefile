UTILITIES_PATH = ../../../../utilities
PATCHES_PATH = ../../../../patches

all: assets/nexmon assets libs/armeabi

assets: \
	assets/nexmon/fw_bcm4330_5_90_100_41_sta.bin \
	assets/nexmon/fw_bcm4339_6_37_34_43.bin \
	assets/nexmon/fw_bcm4358_7_112_200_17_sta.bin \
	assets/nexmon \
	assets/nexmon/rawproxy \
	assets/nexmon/rawproxyreverse \
	assets/nexmon/nc \
	assets/nexmon/iw \
	assets/nexmon/iwconfig \
	assets/nexmon/tcpdump \
	assets/nexmon/aircrack-ng \
	assets/nexmon/dhdutil \
	assets/nexmon/nexutil \
	assets/nexmon/mdk3 \
	assets/nexmon/socat \
	assets/nexmon/libfakeioctl.so \
	FORCE

assets/nexmon: FORCE
	mkdir -p assets/nexmon/

assets/nexmon/fw_bcm4330_5_90_100_41_sta.bin: FORCE
	make -C $(PATCHES_PATH)/bcm4330/5_90_100_41_sta/nexmon
	cp $(PATCHES_PATH)/bcm4330/5_90_100_41_sta/nexmon/bcmdhd_sta.bin $@

assets/nexmon/fw_bcm4339_6_37_34_43.bin: FORCE
	make -C $(PATCHES_PATH)/bcm4339/6_37_34_43/nexmon
	cp $(PATCHES_PATH)/bcm4339/6_37_34_43/nexmon/fw_bcmdhd.bin $@

assets/nexmon/fw_bcm4358_7_112_200_17_sta.bin: FORCE
	make -C $(PATCHES_PATH)/bcm4358/7_112_200_17_sta/nexmon
	cp $(PATCHES_PATH)/bcm4358/7_112_200_17_sta/nexmon/fw_bcmdhd.bin $@

assets/nexmon/rawproxy: FORCE
	make -C $(UTILITIES_PATH)/rawproxy
	cp $(UTILITIES_PATH)/rawproxy/libs/armeabi/rawproxy $@

assets/nexmon/rawproxyreverse: FORCE
	make -C $(UTILITIES_PATH)/rawproxyreverse
	cp $(UTILITIES_PATH)/rawproxyreverse/libs/armeabi/rawproxyreverse $@

assets/nexmon/tcpdump: FORCE
	make -C $(UTILITIES_PATH)/tcpdump
	cp $(UTILITIES_PATH)/tcpdump/libs/armeabi/tcpdump $@

assets/nexmon/dhdutil: FORCE
	make -C $(UTILITIES_PATH)/dhdutil
	cp $(UTILITIES_PATH)/dhdutil/libs/armeabi/dhdutil $@

assets/nexmon/nexutil: FORCE
	make -C $(UTILITIES_PATH)/nexutil
	cp $(UTILITIES_PATH)/nexutil/libs/armeabi/nexutil $@

assets/nexmon/mdk3: FORCE
	make -C $(UTILITIES_PATH)/mdk3
	cp $(UTILITIES_PATH)/mdk3/libs/armeabi/mdk3 $@

assets/nexmon/libfakeioctl.so: FORCE
	make -C $(UTILITIES_PATH)/libfakeioctl
	cp $(UTILITIES_PATH)/libfakeioctl/libs/armeabi/libfakeioctl.so $@

assets/nexmon/aircrack-ng: FORCE
	make -C $(UTILITIES_PATH)/aircrack-ng
	cp $(UTILITIES_PATH)/aircrack-ng/libs/armeabi/* $(dir $@)

assets/nexmon/nc: FORCE
	make -C $(UTILITIES_PATH)/netcat
	cp $(UTILITIES_PATH)/netcat/libs/armeabi/nc $@

assets/nexmon/iw: FORCE
	make -C $(UTILITIES_PATH)/iw
	cp $(UTILITIES_PATH)/iw/libs/armeabi/iw $@

assets/nexmon/socat: FORCE
	make -C $(UTILITIES_PATH)/socat
	cp $(UTILITIES_PATH)/socat/libs/armeabi/socat $@

assets/nexmon/iwconfig: FORCE
	make -C $(UTILITIES_PATH)/wireless_tools
	cp $(UTILITIES_PATH)/wireless_tools/libs/armeabi/* $(dir $@)

libs/armeabi: FORCE
	mkdir -p libs/armeabi/
	make -C external
	make -C internal

clean: FORCE
	make -C external clean
	make -C internal clean
	rm -rf libs
	rm -rf assets/nexmon/

FORCE:


.PHONY: all clean
