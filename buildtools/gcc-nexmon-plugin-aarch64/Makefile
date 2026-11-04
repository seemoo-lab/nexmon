CXX ?= g++
GCC15_DIR ?= ../arm-gnu-toolchain-15.3.rel1-aarch64-arm-none-eabi
GCC15_PLUGIN_INC ?= $(wildcard $(GCC15_DIR)/lib/gcc/arm-none-eabi/*/plugin/include)

all: nexmon.so

nexmon.so: nexmon.o
	$(CXX) -shared -o $@ $<

nexmon.o: ../gcc-nexmon-plugin/nexmon.c
	$(CXX) -std=c++17 -Wall -fno-rtti -Wno-literal-suffix -fPIC -I$(GCC15_PLUGIN_INC) -c -o $@ $<

clean:
	rm -f *.o *.so
