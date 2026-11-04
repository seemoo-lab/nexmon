# Stamps, not local/<abi>/*.a: APP_ABI=all does not produce local/all/.
BORINGSSL_CRYPTO_INPUTS := ../libcrypto/Android.mk ../libcrypto/Application.mk ../boringssl/crypto-sources.mk ../boringssl/sources.mk \
	$(wildcard ../boringssl/crypto/*.cc ../boringssl/crypto/*/*.cc ../boringssl/gen/crypto/*.cc ../boringssl/include/openssl/*.h)
BORINGSSL_SSL_INPUTS := ../libssl/Android.mk ../libssl/Application.mk ../boringssl/ssl-sources.mk ../boringssl/sources.mk \
	$(wildcard ../boringssl/ssl/*.cc ../boringssl/include/openssl/*.h)

all: libs/armeabi-v7a/tcpdump

libs/armeabi-v7a/tcpdump: Android.mk ../libssl/local/.built.stamp ../libcrypto/local/.built.stamp ../libpcap/local/armeabi-v7a/libpcap.a
	$(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21

../libpcap/local/armeabi-v7a/libpcap.a: FORCE
	cd ../libpcap && $(MAKE) $(if $(APP_ABI),APP_ABI=$(APP_ABI))

../libssl/local/.built.stamp: $(BORINGSSL_SSL_INPUTS)
	$(MAKE) -C ../libssl $(if $(APP_ABI),APP_ABI=$(APP_ABI))

../libcrypto/local/.built.stamp: $(BORINGSSL_CRYPTO_INPUTS)
	$(MAKE) -C ../libcrypto $(if $(APP_ABI),APP_ABI=$(APP_ABI))

install: libs/armeabi-v7a/tcpdump
	adb push $< /sdcard/
	adb shell 'su -c "mount -o rw,remount /system"'
	adb shell 'su -c "cp /sdcard/tcpdump /system/bin/tcpdump"'
	adb shell 'su -c "chmod +x /system/bin/tcpdump"'

clean:
	rm -Rf libs
	rm -Rf local

FORCE:
