# libwiretap for wireshark 4.x is cross-compiled with CMake by
# ../wireshark/build-android.sh; this target makes sure that shared build
# has published local/$(APP_ABI)/libwiretap.a.
APP_ABI ?= arm64-v8a

all:
	../wireshark/build-android.sh $(APP_ABI)

install:
	@echo "nothing to install ..."

clean:
	rm -Rf libs local
