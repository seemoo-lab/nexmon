CXX ?= g++

all: nexmon.so

nexmon.so: nexmon.o
	$(CXX) -m32 -shared -o $@ $<

nexmon.o: nexmon.c
	$(CXX) -std=c++11 -Wall -fno-rtti -Wno-literal-suffix -m32 -fPIC -I../gcc-arm-none-eabi-5_4-2016q2-linux-x86/lib/gcc/arm-none-eabi/5.4.1/plugin/include -I/usr/include/i386-linux-gnu -I/usr/include/x86_64-linux-gnu -c -o $@ $<

clean:
	rm -f *.o *.so