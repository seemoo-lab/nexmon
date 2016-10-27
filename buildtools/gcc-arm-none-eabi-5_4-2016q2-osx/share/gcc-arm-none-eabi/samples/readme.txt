Samples of GNU Tools for ARM Embedded Processors v4.7+

* Directory structure *
samples/startup   - Startup code needed by samples
       /ldscripts - Shared linker script pieces needed by samples
       /src       - Sample source code and Makefile, each sub-dir is a case

* Build *
For Windows, linux-like tools is required to run command like make and rm.
These samples are tested with cygwin.

To build all samples, set arm-none-eabi-gcc in path, cd src and make.

The makefile is configured to use newlib-nano by default. To disable it,
pass USE_NANO= to make command line like:
$ make USE_NANO=

The makefile is configured to build for Cortex-M0 by default. To build for
M3, M4 or M7, pass CORTEX_M=3/4/7 respectively:
$ make CORTEX_M=3

* Porting *
These samples are written in a way that easily porting to variant Cortex-M
boards. Usually there are only two files you need modify for your boards.

ldscripts/mem.ld defines address ranges for flash and RAM. Modify them to
reflect start address and length of flash/RAM banks in your board, by
following the embedded comments.

src/retarget/retarget.c implements how to initialize UART, send/receive
strings via UART. Since UART is configured and operated differently in
different boards, you need put your board specific code in retarget.c
to make UART work.

Recommend to make clean after modifying mem.ld.

* Sample cases introduction *
** minimum - A minimum skeleton to start a C program with limited features.
This case has a empty main. Code size built from it is only about 150 bytes,
since it has almost no "fat" for a Cortex-M C program. Be noticed that this
case doesn't support semihosting or C++ global constructor/destructor.

** semihost - Show how to use semihosting for file IO.
This case uses printf to output a string. Here printf is backup by
semihosting, which does the real print job to a host machine.

** retarget - Show how to use retarget (UART) for standard IO.
This case uses printf to output a string. Here printf is backup by retarget.
There is a retarget.c showing which libc interfaces need overloading to
re-direct file IO operations to on-board devices (usually UART). This case
isn't complete, real board specific code need adding to make UART work.

This case also doesn't support semihosting or C++ global
constructor/destructor.

** fpout - Show how to print floating point.
This case uses printf from newlib-nano to print floating point values. Since
newlib-nano by default doesn't link code needed by floating point IO, this
case demonstrates how to use this function in command line options. It also
uses semihosting.

** fpin - Show how to input/output floating point.
This case uses sscanf from newlib-nano to input floating point values. Since
newlib-nano by default doesn't link code needed by floating point IO, this
case demonstrates how to use this function in command line options. It also
uses semihosting.

** qemu - Show how to build a Cortex-M bare-metal program running on qemu.
This case builds a simple application that can run on qemu for ARM. It uses
semihosting for file IO, which qemu supports.

** cpp - How to setup a C++ project
This case builds a simple C++ application using memory allocator, integer IO
and global constructor/destructor.

** multiram - How to use multiple RAM banks
This case works with system with two RAM banks. It include startup and linker
scripts that initializing multiple data sections and bss sections.
