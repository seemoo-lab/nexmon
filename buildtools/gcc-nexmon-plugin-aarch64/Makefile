all: nexmon.so

nexmon.so: nexmon.o
	g++ -shared -o $@ $<

nexmon.o: ../gcc-nexmon-plugin/nexmon.c
	g++ -std=c++11 -Wall -fno-rtti -Wno-literal-suffix -fPIC -I../gcc-arm-none-eabi-5_4-2016q3-linux-aarch64/lib/gcc/arm-none-eabi/5.4.1/plugin/include -c -o $@ $<

clean:
	rm -f *.o *.so
