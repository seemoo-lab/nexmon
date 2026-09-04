# glib 2.89.4 is cross-compiled by build-android.sh (meson + NDK) rather
# than ndk-build; the script refreshes the checked-in archives and the
# glib-2.0/ headers consumed by libwsutil/libwiretap/libwireshark.
APP_ABI ?= arm64-v8a

all:
	./build-android.sh $(APP_ABI)

install:
	@echo "nothing to install ..."

clean:
	rm -Rf libs local
