all: nexmon.so

nexmon.so: nexmon.o
	g++ -shared -o $@ $<

nexmon.o: ../gcc-nexmon-plugin/nexmon.c
	g++ -std=c++11 -Wall -fno-rtti -Wno-literal-suffix -fPIC -I../gcc-arm-none-eabi-9_2-2019q4-linux-aarch64/lib/gcc/arm-none-eabi/9.2.1/plugin/include -c -o $@ $<
	

clean:
	rm -f *.o *.so

check:
	../gcc-arm-none-eabi-9_2-2019q4-linux-aarch64/bin/arm-none-eabi-g++ -fplugin=./nexmon.so -c -x c++ /dev/null -o /dev/null
