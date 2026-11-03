CC ?= gcc
CFLAGS ?= -std=c99
MINCRYPT_SRCS = $(wildcard src/libmincrypt/*.c)
HEADERS = $(wildcard src/include/*.h src/include/*/*.h)

TARGETS = mkbootimg unmkbootimg mkbootfs

all: $(TARGETS)

mkbootimg: src/mkbootimg/mkbootimg.c $(MINCRYPT_SRCS) $(HEADERS)
	cd src/ && $(CC) $(CFLAGS) -o ../mkbootimg libmincrypt/*.c mkbootimg/mkbootimg.c -Iinclude

unmkbootimg: src/mkbootimg/unmkbootimg.c $(MINCRYPT_SRCS) $(HEADERS)
	cd src/ && $(CC) $(CFLAGS) -o ../unmkbootimg libmincrypt/*.c mkbootimg/unmkbootimg.c -Iinclude

mkbootfs: src/cpio/mkbootfs.c src/libcutils/fs_config.c $(HEADERS)
	cd src/ && $(CC) -o ../mkbootfs cpio/mkbootfs.c libcutils/fs_config.c -Iinclude

clean:
	rm -f $(TARGETS)

.PHONY: all clean