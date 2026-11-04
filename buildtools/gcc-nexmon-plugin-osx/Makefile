CXX ?= $(shell command -v g++-mp-5 2>/dev/null || echo c++)

all: nexmon.so

nexmon.so: nexmon.o
	$(CXX) -dynamiclib -undefined dynamic_lookup -o $@ $<

nexmon.o: ../gcc-nexmon-plugin/nexmon.c
	$(CXX) -std=c++11 -Wall -fno-rtti -Wno-literal-suffix -fPIC -I../gcc-arm-none-eabi-5_4-2016q2-osx/lib/gcc/arm-none-eabi/5.4.1/plugin/include -I/opt/local/include -c -o $@ $<

clean:
	rm -f *.o *.so