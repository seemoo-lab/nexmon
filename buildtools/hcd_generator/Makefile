all: bthcd_generate

bthcd_generate : bthcd_generate.o 
ifeq ($(HOSTUNAME),Darwin)
	gcc-mp-5 -L/opt/local/lib -largp -o $@ $^
else
	gcc -o $@ $^
endif

bthcd_generate.o : bthcd_generate.c
ifeq ($(HOSTUNAME),Darwin)
	gcc-mp-5 -I${NEXMON_ROOT}/patches/include -std=c99 -Wall -Wno-unused-result -O0 -D_BSD_SOURCE -o $@ -c $<
else
	gcc -std=c99 -I${NEXMON_ROOT}/patches/include -Wall -Wno-unused-result -O0 -D_BSD_SOURCE -o $@ -c $<
endif


clean: FORCE
	rm -f *.o bthcd_generate

FORCE:
