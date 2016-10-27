ucodeext: ucodeext.o
	gcc -o $@ $^

ucodeext.o: ucodeext.c
	gcc -std=c99 -Wall -Wno-unused-result -O0 -D_BSD_SOURCE -o $@ -c $<

clean: FORCE
	rm -f *.o

FORCE:
