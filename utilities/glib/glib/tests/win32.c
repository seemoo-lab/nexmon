/* Unit test for VEH on Windows
 * Copyright (C) 2019 Руслан Ижбулатов
 *
 * SPDX-License-Identifier: LicenseRef-old-glib-tests
 *
 * This work is provided "as is"; redistribution and modification
 * in whole or in part, in any medium, physical or electronic is
 * permitted without restriction.
 *
 * This work is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * In no event shall the authors or contributors be liable for any
 * direct, indirect, incidental, special, exemplary, or consequential
 * damages (including, but not limited to, procurement of substitute
 * goods or services; loss of use, data, or profits; or business
 * interruption) however caused and on any theory of liability, whether
 * in contract, strict liability, or tort (including negligence or
 * otherwise) arising in any way out of the use of this software, even
 * if advised of the possibility of such damage.
 */

#include "config.h"

#include <glib.h>
#include <gprintf.h>
#include <glib/gstdio.h>

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <fcntl.h>
#include <io.h>
#include <windows.h>
#include <winnt.h> /* for RTL_OSVERSIONINFO */

#define COBJMACROS
#include <wincodec.h>

static char *argv0 = NULL;

/* Crash with access violation */
static void
test_access_violation (void)
{
  int *integer = NULL;
  /* Use SEM_NOGPFAULTERRORBOX to prevent an error dialog
   * from being shown.
   */
  DWORD dwMode = SetErrorMode (SEM_NOGPFAULTERRORBOX);
  SetErrorMode (dwMode | SEM_NOGPFAULTERRORBOX);
  *integer = 1;
  SetErrorMode (dwMode);
}

/* Crash with illegal instruction */
static void
test_illegal_instruction (void)
{
  DWORD dwMode = SetErrorMode (SEM_NOGPFAULTERRORBOX);
  SetErrorMode (dwMode | SEM_NOGPFAULTERRORBOX);
  RaiseException (EXCEPTION_ILLEGAL_INSTRUCTION, 0, 0, NULL);
  SetErrorMode (dwMode);
}

static void
test_veh_crash_access_violation (void)
{
  g_unsetenv ("G_DEBUGGER");
  /* Run a test that crashes */
  g_test_trap_subprocess ("/win32/subprocess/access_violation", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
}

static void
test_veh_crash_illegal_instruction (void)
{
  g_unsetenv ("G_DEBUGGER");
  /* Run a test that crashes */
  g_test_trap_subprocess ("/win32/subprocess/illegal_instruction", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
}

static void
test_veh_debug (void)
{
  /* Set up a debugger to be run on crash */
  gchar *command = g_strdup_printf ("%s %s", argv0, "%p %e");
  g_setenv ("G_DEBUGGER", command, TRUE);
  /* Because the "debugger" here is not really a debugger,
   * it can't write into stderr of this process, unless
   * we allow it to inherit our stderr.
   */
  g_setenv ("G_DEBUGGER_OLD_CONSOLE", "1", TRUE);
  g_free (command);
  /* Run a test that crashes and runs a debugger */
  g_test_trap_subprocess ("/win32/subprocess/debuggee", 0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_failed ();
  g_test_trap_assert_stderr ("Debugger invoked, attaching to*");
}

static void
test_veh_debuggee (void)
{
  /* Crash */
  test_access_violation ();
}

static void
veh_debugger (int argc, char *argv[])
{
  char *end;
  DWORD pid = strtoul (argv[1], &end, 10);
  guintptr event = (guintptr) _strtoui64 (argv[2], &end, 10);
  /* Unfreeze the debuggee and announce ourselves */
  SetEvent ((HANDLE) event);
  CloseHandle ((HANDLE) event);
  g_fprintf (stderr, "Debugger invoked, attaching to %lu and signalling %" G_GUINTPTR_FORMAT, pid, event);
}

static void
test_clear_com (void)
{
  IWICImagingFactory *o = NULL;
  IWICImagingFactory *tmp;

  CoInitialize (NULL);
  g_win32_clear_com (&o);
  g_assert_null (o);
  g_assert_true (SUCCEEDED (CoCreateInstance (&CLSID_WICImagingFactory, NULL, CLSCTX_INPROC_SERVER, &IID_IWICImagingFactory, (void **)&tmp)));
  g_assert_nonnull (tmp);
  IWICImagingFactory_QueryInterface (tmp, &IID_IWICImagingFactory, (void **)&o); /* IWICImagingFactory_QueryInterface increments tmp's refcount */
  g_assert_nonnull (o);
  g_assert_cmpint (IWICImagingFactory_AddRef (tmp), ==, 3); /* tmp's refcount incremented, again */
  g_win32_clear_com (&o);  /* tmp's refcount decrements */
  g_assert_null (o);
  g_assert_cmpint (IWICImagingFactory_Release (tmp), ==, 1);   /* tmp's refcount decrements, again */
  g_win32_clear_com (&tmp);
  g_assert_null (tmp);

  CoUninitialize ();
}

static void
test_subprocess_stderr_buffering_mode (void)
{
  int ret = fprintf (stderr, "hello world\n");
  g_assert_cmpint (ret, >, 0);

  /* We want to exit without flushing stdio streams. We could
   * use _Exit here, but the C standard doesn't specify whether
   * _Exit flushes stdio streams or not.
   * The Windows C RunTime library doesn't flush streams, but
   * we should not rely on implementation details which may
   * change in the future. Use TerminateProcess.
   */
  TerminateProcess (GetCurrentProcess (), 0);
}

static void
test_stderr_buffering_mode (void)
{
  /* MSVCRT.DLL can open stderr in full-buffering mode.
   * This can cause loss of important messages before
   * a crash. Additionally, POSIX disallows full buffering
   * of stderr, so this is not good for portability.
   * We have a workaround in the app-profile dependency
   * that we add to each executable.
   */
  g_test_trap_subprocess ("/win32/subprocess/stderr-buffering-mode",
                          0,
                          G_TEST_SUBPROCESS_DEFAULT);
  g_test_trap_assert_passed ();
  g_test_trap_assert_stderr ("hello world?\n");
}

#ifndef _WIN32_WINNT_WIN10
#define _WIN32_WINNT_WIN10 0x0A00
#endif
#ifndef STATUS_SUCCESS
#define STATUS_SUCCESS 0
#endif

static void
test_manifest_os_compatibility (void)
{
  const WORD highest_known_major_minor_word = _WIN32_WINNT_WIN10;

  HMODULE module_handle = LoadLibraryEx (L"NTDLL.DLL", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
  if (module_handle == NULL)
    {
      g_error ("%s (%s) failed: %s",
               "LoadLibraryEx", "NTDLL.DLL",
               g_win32_error_message (GetLastError ()));
    }

  typedef NTSTATUS (WINAPI *ptr_RtlGetVersion_t) (RTL_OSVERSIONINFOW *);

  ptr_RtlGetVersion_t ptr_RtlGetVersion =
    (ptr_RtlGetVersion_t) GetProcAddress (module_handle, "RtlGetVersion");
  if (ptr_RtlGetVersion == NULL)
    {
      g_error ("%s (%s, %s) failed: %s",
               "GetProcAddress", "NTDLL.DLL", "RtlGetVersion",
               g_win32_error_message (GetLastError ()));
    }

  /* RtlGetVersion is not subject to compatibility settings
   * present in the activation context, it always returns
   * the real OS version.
   */
  RTL_OSVERSIONINFOW rtl_os_version_info;
  rtl_os_version_info.dwOSVersionInfoSize = sizeof (rtl_os_version_info);

  NTSTATUS status = ptr_RtlGetVersion (&rtl_os_version_info);
  if (status != STATUS_SUCCESS)
    {
      g_error ("%s failed",
               "RtlGetVersion");
    }

  /* Now verify if the activation context contains up-to-date
   * compatibility info.
   */
  OSVERSIONINFO os_version_info;
  os_version_info.dwOSVersionInfoSize = sizeof (os_version_info);

  BOOL success = GetVersionEx (&os_version_info);
  if (!success)
    {
      g_error ("%s failed: %s",
               "GetVersionEx",
               g_win32_error_message (GetLastError ()));
    }

  if (rtl_os_version_info.dwMajorVersion != os_version_info.dwMajorVersion ||
      rtl_os_version_info.dwMinorVersion != os_version_info.dwMinorVersion ||
      rtl_os_version_info.dwBuildNumber != os_version_info.dwBuildNumber)
    {
      WORD rtl_major_minor_word = MAKEWORD ((BYTE)rtl_os_version_info.dwMinorVersion,
                                            (BYTE)rtl_os_version_info.dwMajorVersion);

      if (rtl_major_minor_word > highest_known_major_minor_word)
        g_error ("Please, update the manifest XML and the test's constant");

      g_assert_cmpuint (rtl_os_version_info.dwMajorVersion, ==, os_version_info.dwMajorVersion);
      g_assert_cmpuint (rtl_os_version_info.dwMinorVersion, ==, os_version_info.dwMinorVersion);
      g_assert_cmpuint (rtl_os_version_info.dwBuildNumber, ==, os_version_info.dwBuildNumber);
    }

  FreeLibrary (module_handle);
}

typedef struct {
  FILE *stream;
  bool done;
  GMutex mutex;
  GCond cond;
} StreamLockData_t;

static gpointer
thread_acquire_stdio_stream_lock (gpointer user_data)
{
  StreamLockData_t *data = (StreamLockData_t *) user_data;
  FILE *stream = data->stream;

  /* For the test to be effective, this thread must be holding
   * the stream lock when the subprocess "main function" (i.e.
   * test_subprocess_early_flush) returns.
   *
   * This simulates a thread doing I/O on a stream at the time
   * exit invokes ExitProcess. Note that _lock_file is exactly
   * what stdio functions call internally.
   */

  _lock_file (stream);

  g_mutex_lock (&data->mutex);
  data->done = true;
  g_cond_signal (&data->cond);
  g_mutex_unlock (&data->mutex);
  data = NULL;

  /* Sleep a bit to let the main thread exit. Note: if somehow
   * the main thread doesn't exit in time, this test will
   * succeed but won't actually verify anything.
   */
  g_usleep (1 * G_USEC_PER_SEC);

  _unlock_file (stream);

  return NULL;
}

static void
test_subprocess_early_stdio_flush (void)
{
  int64_t fd;
  FILE *stream;
  int ret;

  fd = g_ascii_strtoull (g_getenv ("G_TEST_PIPE_FD"), NULL, 10);
  g_assert_cmpuint (fd, >=, 0);
  g_assert_cmpuint (fd, <=, INT_MAX);

  stream = _fdopen ((int) fd, "w");
  g_assert_nonnull (stream);

  ret = setvbuf (stream, NULL, _IOFBF, 1024);
  g_assert_cmpint (ret, ==, 0);

  ret = fprintf (stream, "hello world");
  g_assert_cmpint (ret, ==, (int) strlen ("hello world"));
  g_assert_false (ferror (stream));

  StreamLockData_t data;
  memset (&data, 0, sizeof (data));
  data.stream = stream;
  data.done = false;

  GThread *thread = g_thread_new ("lock stdio stream",
                                  thread_acquire_stdio_stream_lock,
                                  (gpointer) &data);
  g_thread_unref (thread);

  /* Wait until the worker thread has acquired the stream lock.
   */
  g_mutex_lock (&data.mutex);
  while (!data.done)
    g_cond_wait (&data.cond, &data.mutex);
  g_mutex_unlock (&data.mutex);

  /* If the early flush is not implemented, the C RunTime will attempt
   * to acquire the stream's internal CRITICAL_SECTION from DllMain.
   * Then the process will be terminated abruptly (with the same exit
   * code passed to exit) leaving stream unflushed.
   */
}

static gpointer
thread_read_pipe (gpointer user_data)
{
  int fd = GPOINTER_TO_INT (user_data);
  const char *iter = "hello world";
  int size = (int) strlen (iter);

  while (size > 0)
    {
      char buffer[20];
      int ret = _read (fd, buffer, sizeof (buffer));

      g_assert_cmpint (ret, >, 0);
      g_assert_cmpint (ret, <=, size);

      g_assert_cmpmem (buffer, ret, iter, ret);

      iter += ret;
      size -= ret;
    }

  return NULL;
}

static void
test_early_stdio_flush (void)
{
  int pipe_fds[2];
  int pipe_read;
  int pipe_write;
  char buffer[10];
  GStrv envp = NULL;
  GError *error = NULL;
  int ret;

  ret = _pipe (pipe_fds, 1024, _O_BINARY);
  if (ret < 0)
    g_error ("%s failed: %s", "_pipe", g_strerror (errno));

  pipe_read = pipe_fds[0];
  pipe_write = pipe_fds[1];

  g_snprintf (buffer, sizeof (buffer), "%i", pipe_write);
  envp = g_get_environ ();
  envp = g_environ_setenv (g_steal_pointer (&envp), "G_TEST_PIPE_FD", buffer, TRUE);

  GThread *thread = g_thread_new ("read pipe",
                                  thread_read_pipe,
                                  GINT_TO_POINTER (pipe_read));

  typedef const char * const * GStrvConst_t;
  g_test_trap_subprocess_with_envp ("/win32/subprocess/early-stdio-flush",
                                    (GStrvConst_t) envp, 0,
                                    G_TEST_SUBPROCESS_INHERIT_DESCRIPTORS);
  g_test_trap_assert_passed ();

  g_close (pipe_write, &error);
  g_assert_no_error (error);

  g_thread_join (thread);

  g_close (pipe_read, NULL);
  g_strfreev (envp);
}

int
main (int   argc,
      char *argv[])
{
  argv0 = argv[0];

  g_test_init (&argc, &argv, NULL);

  if (argc > 2)
    {
      veh_debugger (argc, argv);
      return 0;
    }

  g_test_add_func ("/win32/veh/access_violation", test_veh_crash_access_violation);
  g_test_add_func ("/win32/veh/illegal_instruction", test_veh_crash_illegal_instruction);
  g_test_add_func ("/win32/veh/debug", test_veh_debug);

  g_test_add_func ("/win32/subprocess/debuggee", test_veh_debuggee);
  g_test_add_func ("/win32/subprocess/access_violation", test_access_violation);
  g_test_add_func ("/win32/subprocess/illegal_instruction", test_illegal_instruction);
  g_test_add_func ("/win32/com/clear", test_clear_com);

  g_test_add_func ("/win32/subprocess/stderr-buffering-mode", test_subprocess_stderr_buffering_mode);
  g_test_add_func ("/win32/stderr-buffering-mode", test_stderr_buffering_mode);
  g_test_add_func ("/win32/manifest-os-compatibility", test_manifest_os_compatibility);
  g_test_add_func ("/win32/subprocess/early-stdio-flush", test_subprocess_early_stdio_flush);
  g_test_add_func ("/win32/early-stdio-flush", test_early_stdio_flush);

  return g_test_run();
}
