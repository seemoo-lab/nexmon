# ndk-build writes local/<abi>/libcrypto.a and removes each .o before compiling
# it. utilities/aircrack-ng and utilities/tcpdump also invoke this makefile,
# so two ndk-builds can run at once and llvm-ar fails with
# "x509name.o: No such file or directory". flock keeps a single archiver.
# local/.built.stamp is the file those callers wait on; `all` stays phony so
# a direct `make` still lets ndk-build see newer sources.
.PHONY: all libs/armeabi/libcrypto.a

all libs/armeabi/libcrypto.a &:
	mkdir -p local
	flock local $(NDK_ROOT)/ndk-build NDK_APPLICATION_MK=`pwd`/Application.mk NDK_APP_OUT=. TARGET_PLATFORM=android-21 $(if $(APP_ABI),APP_ABI=$(APP_ABI))
	touch local/.built.stamp

clean:
	rm -Rf libs
	rm -Rf local

install:
	@echo "nothing to install ..."

FORCE:
	