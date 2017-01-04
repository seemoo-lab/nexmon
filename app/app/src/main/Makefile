all: FORCE
	mkdir -p libs/armeabi/
	mkdir -p assets/nexmon/
	make -C external
	make -C internal

clean: FORCE
	make -C external clean
	make -C internal clean
	rm -rf libs
	rm -rf assets/nexmon/

FORCE:


.PHONY: all clean