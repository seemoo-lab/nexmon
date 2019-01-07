Please do not compile this package (GLib) in paths that contain
spaces in them-as strange problems may occur during compilation or during
the use of the library.

Please refer to the following GNOME Live! page for more detailed
instructions on building GLib and its dependencies with Visual C++:

https://wiki.gnome.org/Projects/GTK%2B/Win32/MSVCCompilationOfGTKStack

This VS12 solution and the projects it includes are intented to be used
in a GLib source tree unpacked from a tarball. In a git checkout you
first need to use some Unix-like environment or run build/win32/setup.py, 
which will do the work for you:

$python build/win32/setup.py --perl path_to_your_perl.exe

for more usage on this script, run
$python build/win32/setup.py -h/--help

The required dependencies are zlib, proxy-libintl and LibFFI. Fetch the latest
proxy-libintl-dev and zlib-dev zipfiles from
http://ftp.gnome.org/pub/GNOME/binaries/win32/dependencies/ for 32-bit
builds, and correspondingly
http://ftp.gnome.org/pub/GNOME/binaries/win64/dependencies/ for 64-bit
builds.

One may wish to build his/her own ZLib-It is recommended that ZLib is
built using the win32/Makefile.msc makefile with VS12 with the ASM routines
to avoid linking problems-see win32/Makefile.msc in ZLib for more details.

For LibFFI, please get version 3.0.10 or later, as Visual C++ build support
was added in the 3.0.10 release series.  Please see the README file that
comes with the LibFFI source package for more details on how to build LibFFI
on Visual C++-please note that the mozilla-build package from Mozilla is needed
in order to build LibFFI on Windows.

Please note, although using one's own existing PCRE installation to build GLib
is possible, it is still recommended to build PCRE during the process of building
GLib (i.e. using the Debug or Release configurations), as GLib's bundled PCRE
has been patched to work optimally with GLib.  If building against an existing
PCRE is desired, use the(BuildType)_ExtPCRE configurations, but one needs to ensure
that the existing PCRE is:
-Built with VS12
-Unicode support is built in (please see the CMake options for this)
-It is built with the Multithreaded DLL (/MD, for release builds) or the
 Multithreaded DLL Debug (/MDd, for debug builds)

If using static builds of PCRE, please add PCRE_STATIC to the "Preprocessor
Definitions" of the glib project settings.

Please be aware that the GLib's regex test program will only pass with PCRE directly
built into GLib.

Set up the source tree as follows under some arbitrary top
folder <root>:

<root>\<this-glib-source-tree>
<root>\vs12\<PlatformName>

*this* file you are now reading is thus located at
<root>\<this-glib-source-tree>\build\win32\vs12\README.

<PlatformName> is either Win32 or x64, as in VS12 project files.

You should unpack the proxy-libintl-dev zip file into
<root>\vs12\<PlatformName>, so that for instance libintl.h end up at
<root>\vs12\<PlatformName>\include\libintl.h.

For LibFFI, one should also put the generated ffi.h and ffitarget.h
into <root>\vs12\<PlatformName>\include\ and the compiled static libffi.lib
(or copy libffi-convenience.lib into libffi.lib) into 
<root>\vs12\<PlatformName>\lib\.

The "install" project will copy build results and headers into their
appropriate location under <root>\vs12\<PlatformName>. For instance,
built DLLs go into <root>\vs12\<PlatformName>\bin, built LIBs into
<root>\vs12\<PlatformName>\lib and GLib headers into
<root>\vs12\<PlatformName>\include\glib-2.0. This is then from where
project files higher in the stack are supposed to look for them, not
from a specific GLib source tree.

Note: If you see C4819 errors and you are compiling GLib on a DBCS
(Chinese/Korean/Japanese) version of Windows, you may need to switch
to an English locale in Control Panel->Region and Languages->System->
Change System Locale, reboot and rebuild to ensure GLib, Pango, GDK-Pixbuf,
ATK and GTK+ is built correctly.  This is due to a bug in Visual C++ running
on DBCS locales, and also affects many other opensource projects which are
built with Visual C++, including but not limited to QT and the Mozilla apps.

--Tor Lillqvist <tml@iki.fi>
--Updated by Chun-wei Fan <fanc999@gmail.com>
