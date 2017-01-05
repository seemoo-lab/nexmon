UTILITIES_PATH = ../../../../utilities

all: assets/nexmon libs/armeabi
	
assets: assets/nexmon assets/nexmon/nc assets/nexmon/iw assets/nexmon/iwconfig assets/nexmon/tcpdump assets/nexmon/aircrack-ng assets/nexmon/dhdutil assets/nexmon/nexutil assets/nexmon/mdk3 assets/nexmon/libfakeioctl.so FORCE 

assets/nexmon: FORCE
	mkdir -p assets/nexmon/

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