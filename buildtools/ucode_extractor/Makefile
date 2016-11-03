ucodeext: ucodeext.o
ifeq ($(HOSTUNAME),Darwin)
	gcc-mp-5 -L/opt/local/lib -largp -o $@ $^
else
	gcc -o $@ $^
endif

ucodeext.o: ucodeext.c
ifeq ($(HOSTUNAME),Darwin)
	gcc-mp-5 -std=c99 -Wall -Wno-unused-result -O0 -D_BSD_SOURCE -o $@ -c $<
else
	gcc -std=c99 -Wall -Wno-unused-result -O0 -D_BSD_SOURCE -o $@ -c $<
endif

clean: FORCE
	rm -f *.o

FORCE:
