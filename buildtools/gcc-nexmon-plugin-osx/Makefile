all: nexmon.so

nexmon.so: nexmon.o
	g++-mp-5 -dynamiclib -undefined dynamic_lookup -o $@ $<

nexmon.o: ../gcc-nexmon-plugin/nexmon.c
	g++-mp-5 -std=c++11 -Wall -fno-rtti -Wno-literal-suffix -fPIC -I../gcc-arm-none-eabi-5_4-2016q2-osx/lib/gcc/arm-none-eabi/5.4.1/plugin/include -I/opt/local/include -c -o $@ $<

clean:
	rm -f *.o *.so