/* Control of exported symbols from libintl.
   Copyright (C) 2005-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as published by
   the Free Software Foundation; either version 2.1 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible.  */

#if @HAVE_VISIBILITY@ && BUILDING_LIBINTL
# define LIBINTL_SHLIB_EXPORTED __attribute__((__visibility__("default")))
#elif (defined _WIN32 && !defined __CYGWIN__) && @WOE32DLL@ && BUILDING_LIBINTL
/* The symbols from this file should be exported if and only if the object
   file gets included in a DLL.  Libtool, on Windows platforms, defines
   the C macro DLL_EXPORT (together with PIC) when compiling for a shared
   library (called DLL under Windows) and does not define it when compiling
   an object file meant to be linked statically into some executable.  */
# if defined DLL_EXPORT
#  define LIBINTL_SHLIB_EXPORTED __declspec(dllexport)
# else
#  define LIBINTL_SHLIB_EXPORTED
# endif
#elif (defined _WIN32 && !defined __CYGWIN__) && @WOE32DLL@
# define LIBINTL_SHLIB_EXPORTED __declspec(dllimport)
#else
# define LIBINTL_SHLIB_EXPORTED
#endif
