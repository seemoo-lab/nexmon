all: libssl.so libcrypto.so libsqlite.so

libssl.so: 
	adb pull /system/lib/libssl.so .

libcrypto.so: 
	adb pull /system/lib/libcrypto.so .

libsqlite.so: 
	adb pull /system/lib/libsqlite.so .

clean:
	rm -Rf *.so
