TARGETS = mkbootimg unmkbootimg mkbootfs

all: $(TARGETS)

mkbootimg: src/mkbootimg/mkbootimg.c
	cd src/ && \
	   gcc -std=c99 -o ../mkbootimg libmincrypt/*.c mkbootimg/mkbootimg.c -Iinclude

unmkbootimg: src/mkbootimg/unmkbootimg.c
	cd src/ && \
	   gcc -std=c99 -o ../unmkbootimg libmincrypt/*.c mkbootimg/unmkbootimg.c -Iinclude

mkbootfs: src/cpio/mkbootfs.c src/libcutils/fs_config.c
	cd src/ && \
	   gcc -o ../mkbootfs cpio/mkbootfs.c libcutils/fs_config.c -Iinclude

clean:
	rm -f $(TARGETS)