fpext : fpext.o darm/armv7.o darm/armv7-tbl.o darm/darm.o darm/darm-tbl.o darm/thumb2.o darm/thumb2-decoder.o darm/thumb2-tbl.o darm/thumb.o darm/thumb-tbl.o
	gcc -o $@ $^

fpext.o : fpext.c
	gcc -std=c99 -Wall -Wno-unused-result -O0 -D_BSD_SOURCE -o $@ -c $<

darm/%.o: darm/%.c
	gcc -std=c99 -Wall -O2 -Wextra -Wno-missing-field-initializers -o $@ -c $^

fp.6.37.32.RC23.34.43_r639704_hammerhead_mob30y.c: ../../bootimg_src/firmware/fw_bcmdhd.6.37.32.RC23.34.43_r639704_hammerhead_mob30y.bin
	./fpext -r $< -b 0x1d2000 -e 0x1d2948 > $@
