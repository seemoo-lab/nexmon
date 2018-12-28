all: bthcd bthcd_rompatches

bthcd : bthcd.o 
ifeq ($(HOSTUNAME),Darwin)
	gcc-mp-5 -I${NEXMON_ROOT}/patches/bluetooth/include -L/opt/local/lib -largp -o $@ $^
else
	gcc -I${NEXMON_ROOT}/patches/bluetooth/include -o $@ $^
endif

bthcd_rompatches : bthcd_rompatches.o 
ifeq ($(HOSTUNAME),Darwin)
	gcc-mp-5 -I${NEXMON_ROOT}/patches/bluetooth/include -L/opt/local/lib -largp -o $@ $^
else
	gcc -I${NEXMON_ROOT}/patches/bluetooth/include -o $@ $^
endif

bthcd.o : bthcd.c
ifeq ($(HOSTUNAME),Darwin)
	gcc-mp-5 -std=c99 -I${NEXMON_ROOT}/patches/bluetooth/include -Wall -Wno-unused-result -O0 -D_BSD_SOURCE -o $@ -c $<
else
	gcc -std=c99 -I${NEXMON_ROOT}/patches/bluetooth/include -Wall -Wno-unused-result -O0 -D_BSD_SOURCE -o $@ -c $<
endif

bthcd_rompatches.o : bthcd_rompatches.c
ifeq ($(HOSTUNAME),Darwin)
	gcc-mp-5 -I${NEXMON_ROOT}/patches/bluetooth/include -std=c99 -Wall -Wno-unused-result -O0 -D_BSD_SOURCE -o $@ -c $<
else
	gcc -std=c99 -I${NEXMON_ROOT}/patches/bluetooth/include -Wall -Wno-unused-result -O0 -D_BSD_SOURCE -o $@ -c $<
endif

darm/%.o: darm/%.c
ifeq ($(HOSTUNAME),Darwin)
	gcc-mp-5 -std=c99 -Wall -O2 -Wextra -Wno-missing-field-initializers -o $@ -c $^
else
	gcc -std=c99 -Wall -O2 -Wextra -Wno-missing-field-initializers -o $@ -c $^
endif

clean: FORCE
	rm -f *.o darm/*.o bthcd bthcd_rompatches

FORCE:
