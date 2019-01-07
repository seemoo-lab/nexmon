all: FORCE
	make -C jni
	cp libs/armeabi/libnexmonpentestsuite.so ../libs/armeabi/

clean: FORCE
	rm -rf libs
	rm -rf local
	rm -rf obj

FORCE:


.PHONY: all clean