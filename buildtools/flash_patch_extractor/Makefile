fpext : fpext.o darm/armv7.o darm/armv7-tbl.o darm/darm.o darm/darm-tbl.o darm/thumb2.o darm/thumb2-decoder.o darm/thumb2-tbl.o darm/thumb.o darm/thumb-tbl.o
ifeq ($(HOSTUNAME),Darwin)
	gcc-mp-5 -L/opt/local/lib -largp -o $@ $^
else
	gcc -rdynamic -o $@ $^
endif

fpext.o : fpext.c
ifeq ($(HOSTUNAME),Darwin)
	gcc-mp-5 -std=c99 -Wall -Wno-unused-result -O0 -D_BSD_SOURCE -o $@ -c $<
else
	gcc -rdynamic -std=c99 -Wall -Wno-unused-result -O0 -D_BSD_SOURCE -o $@ -c $<
endif

darm/%.o: darm/%.c
ifeq ($(HOSTUNAME),Darwin)
	gcc-mp-5 -std=c99 -Wall -O2 -Wextra -Wno-missing-field-initializers -o $@ -c $^
else
	gcc -rdynamic -std=c99 -Wall -O2 -Wextra -Wno-missing-field-initializers -o $@ -c $^
endif

clean: FORCE
	rm -f *.o darm/*.o fpext

FORCE:
