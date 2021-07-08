all: nexmon.so

TARGET = ../gcc-arm-none-eabi-5_4-2016q2-linux-x86
TARGET_CXX = $(TARGET)/bin/arm-none-eabi-g++
TARGET_CXX_VERSION = $(shell $(TARGET_CXX) -dumpversion | awk -F "." '{print $$1}')
TARGET_PLUGIN_DIR  = $(shell $(TARGET_CXX) -print-file-name=plugin)/include

CXXFLAGS = -std=c++11
CXXFLAGS += -Wall -Wno-literal-suffix
CXXFLAGS += -fno-rtti
CXXFLAGS += -m32
CXXFLAGS += -fPIC
CXXFLAGS += -I$(TARGET_PLUGIN_DIR)
CXXFLAGS += -DTARGET_CXX_VERSION=$(TARGET_CXX_VERSION)

nexmon.so: nexmon.o
	g++ -$(CXXFLAGS) -shared -o $@ $<

nexmon.o: nexmon.c
	g++ $(CXXFLAGS) -c -o $@ $<

clean:
	rm -f *.o *.so
