/*
 * Copyright © 2025 Luca Bacci
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Luca Bacci <luca.bacci@outlook.com>
 */

#include "config.h"

#include <windows.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <crtdbg.h>

#ifdef HAVE_VCRUNTIME_H
#include <vcruntime.h>
#endif

#ifdef __MINGW64_VERSION_MAJOR
# if __MINGW64_VERSION_MAJOR > 14 || \
     (__MINGW64_VERSION_MAJOR == 14 && __MINGW64_VERSION_MINOR >= 1)
#   define HAVE_MINGW_W64_14_1
# endif
#endif

static void
set_process_wide_settings (void)
{
#if defined (_M_IX86) || defined (__i386__)
  /* https://learn.microsoft.com/en-us/archive/blogs/michael_howard/faq-about-heapsetinformation-in-windows-vista-and-heap-based-buffer-overruns */
  /* https://web.archive.org/web/20080825034220/https://blogs.msdn.com/sdl/archive/2008/06/06/corrupted-heap-termination-redux.aspx */
  HeapSetInformation (NULL, HeapEnableTerminationOnCorruption, NULL, 0);

  /* https://learn.microsoft.com/en-us/archive/blogs/michael_howard/new-nx-apis-added-to-windows-vista-sp1-windows-xp-sp3-and-windows-server-2008 */
  SetProcessDEPPolicy (PROCESS_DEP_ENABLE | PROCESS_DEP_DISABLE_ATL_THUNK_EMULATION);
#endif

  SetErrorMode (GetErrorMode () | SEM_FAILCRITICALERRORS);
}

static void
set_crt_non_interactive (void)
{
  /* The Debug CRT may show UI dialogs even in console applications.
   * Direct to stderr instead.
   */
  _CrtSetReportFile (_CRT_ASSERT, _CRTDBG_FILE_STDERR);
  _CrtSetReportMode (_CRT_ASSERT, _CRTDBG_MODE_FILE);

  _CrtSetReportFile (_CRT_ERROR, _CRTDBG_FILE_STDERR);
  _CrtSetReportMode (_CRT_ERROR, _CRTDBG_MODE_FILE);

  _CrtSetReportFile (_CRT_WARN, _CRTDBG_FILE_STDERR);
  _CrtSetReportMode (_CRT_WARN, _CRTDBG_MODE_FILE);
}

static void
set_stderr_unbuffered_mode (void)
{
  /* MSVCRT.DLL can open stderr in full-buffering mode. That depends on
   * the type of output device; for example, it's fully buffered for
   * named pipes but not for console devices.
   *
   * Having a fully buffered stderr is not a good default since we can
   * loose important messages before a crash. Moreover, POSIX forbids
   * full buffering on stderr. So here we set stderr to unbuffered mode.
   *
   * Note: line buffering mode would be good enough, but the Windows C
   * RunTime library implements it the same as full buffering:
   *
   * "for some systems, _IOLBF provides line buffering. However, for
   *  Win32, the behavior is the same as _IOFBF: Full Buffering"
   *
   * https://learn.microsoft.com/en-us/cpp/c-runtime-library/reference/setvbuf#remarks
   *
   * References:
   *
   *  - https://sourceforge.net/p/mingw-w64/mailman/message/59272395/
   *  - https://github.com/mingw-w64/mingw-w64/commit/b30a91ac
   */
#if !defined (HAVE_MINGW_W64_14_1)
#if !defined (_UCRT)
  int ret = setvbuf (stderr, NULL, _IONBF, 0);
  assert (ret == 0);
#endif
#endif
}

#if !defined (HAVE_MINGW_W64_14_1)

static void
early_flush_exit_handler (void)
{
  /* There are two ways to flush open streams: calling fflush with NULL
   * argument and calling _flushall. The former flushes output streams
   * only, the latter flushes both input and output streams.
   * We should not do anything with input streams here since flushing
   * means * discarding * data.
   */
  fflush (NULL);
}

#endif

static void
register_early_flush_at_exit (void)
{
  /* Implement the two-phase flushing at process exit.
   *
   * The C RunTime library flushes open streams within its DllMain handler.
   * This goes against the rules for DllMain, as each stream is protected
   * by a lock and locks must not be acquired in DllMain.
   *
   * So we flush from app code using an atexit handler. The handler runs when
   * the application is in a fully working state and thus is completely safe.
   *
   * This ensures that all important data is flushed. Anything that is written
   * after exit will be flushed lately by the C RunTime library (and therefore
   * may be skipped).
   *
   * See comments in "%ProgramFiles(x86)%\Windows Kits\10\Source\<version>\
   * \ucrt\stdio\fflush.cpp" for more informations.
   *
   * References:
   *
   *  - https://sourceforge.net/p/mingw-w64/mailman/message/59272390/
   *  - https://github.com/mingw-w64/mingw-w64/commit/96d60fc3
   */
#if !defined (HAVE_MINGW_W64_14_1)
  int ret = atexit (early_flush_exit_handler);
  assert (ret == 0);
#endif
}

/* Boilerplate for CRT constructor */

#ifdef _MSC_VER
static void startup (void);

__pragma (section (".CRT$XCT", long, read))

__declspec (allocate (".CRT$XCT"))
void (* const ptr_startup) (void) = startup;

#ifdef _M_IX86
__pragma (comment (linker, "/INCLUDE:" "_ptr_startup"))
#else
__pragma (comment (linker, "/INCLUDE:" "ptr_startup"))
#endif
#else
static void __attribute__((constructor)) startup (void);
#endif

static void
startup (void)
{
  set_crt_non_interactive ();
  set_process_wide_settings ();
  set_stderr_unbuffered_mode ();
  register_early_flush_at_exit ();
}
