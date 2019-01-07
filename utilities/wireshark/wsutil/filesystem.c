/* filesystem.c
 * Filesystem utility routines
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include <config.h>

/*
 * Required with GNU libc to get dladdr().
 * We define it here because <dlfcn.h> apparently gets included by
 * one of the headers we include below.
 */
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <glib.h>

#ifdef _WIN32
#include <windows.h>
#include <tchar.h>
#include <shlobj.h>
#include <wsutil/unicode-utils.h>
#else /* _WIN32 */
#ifdef __APPLE__
#include <mach-o/dyld.h>
#endif
#ifdef __linux__
#include <sys/utsname.h>
#endif
#ifdef __FreeBSD__
#include <sys/types.h>
#include <sys/sysctl.h>
#endif
#ifdef HAVE_DLADDR
#include <dlfcn.h>
#endif
#include <pwd.h>
#endif /* _WIN32 */

#include "filesystem.h"
#include <wsutil/report_err.h>
#include <wsutil/privileges.h>
#include <wsutil/file_util.h>
#include <wsutil/utf8_entities.h>

#include <wiretap/wtap.h>   /* for WTAP_ERR_SHORT_WRITE */

#define PROFILES_DIR    "profiles"
#define PLUGINS_DIR_NAME    "plugins"

char *persconffile_dir = NULL;
char *persdatafile_dir = NULL;
char *persconfprofile = NULL;

static gboolean do_store_persconffiles = FALSE;
static GHashTable *profile_files = NULL;

/*
 * Given a pathname, return a pointer to the last pathname separator
 * character in the pathname, or NULL if the pathname contains no
 * separators.
 */
char *
find_last_pathname_separator(const char *path)
{
    char *separator;

#ifdef _WIN32
    char c;

    /*
     * We have to scan for '\' or '/'.
     * Get to the end of the string.
     */
    separator = strchr(path, '\0');     /* points to ending '\0' */
    while (separator > path) {
        c = *--separator;
        if (c == '\\' || c == '/')
            return separator;   /* found it */
    }

    /*
     * OK, we didn't find any, so no directories - but there might
     * be a drive letter....
     */
    return strchr(path, ':');
#else
    separator = strrchr(path, '/');
    return separator;
#endif
}

/*
 * Given a pathname, return the last component.
 */
const char *
get_basename(const char *path)
{
    const char *filename;

    g_assert(path != NULL);
    filename = find_last_pathname_separator(path);
    if (filename == NULL) {
        /*
         * There're no directories, drive letters, etc. in the
         * name; the pathname *is* the file name.
         */
        filename = path;
    } else {
        /*
         * Skip past the pathname or drive letter separator.
         */
        filename++;
    }
    return filename;
}

/*
 * Given a pathname, return a string containing everything but the
 * last component.  NOTE: this overwrites the pathname handed into
 * it....
 */
char *
get_dirname(char *path)
{
    char *separator;

    g_assert(path != NULL);
    separator = find_last_pathname_separator(path);
    if (separator == NULL) {
        /*
         * There're no directories, drive letters, etc. in the
         * name; there is no directory path to return.
         */
        return NULL;
    }

    /*
     * Get rid of the last pathname separator and the final file
     * name following it.
     */
    *separator = '\0';

    /*
     * "path" now contains the pathname of the directory containing
     * the file/directory to which it referred.
     */
    return path;
}

/*
 * Given a pathname, return:
 *
 *  the errno, if an attempt to "stat()" the file fails;
 *
 *  EISDIR, if the attempt succeeded and the file turned out
 *  to be a directory;
 *
 *  0, if the attempt succeeded and the file turned out not
 *  to be a directory.
 */

int
test_for_directory(const char *path)
{
    ws_statb64 statb;

    if (ws_stat64(path, &statb) < 0)
        return errno;

    if (S_ISDIR(statb.st_mode))
        return EISDIR;
    else
        return 0;
}

int
test_for_fifo(const char *path)
{
    ws_statb64 statb;

    if (ws_stat64(path, &statb) < 0)
        return errno;

    if (S_ISFIFO(statb.st_mode))
        return ESPIPE;
    else
        return 0;
}

/*
 * Directory from which the executable came.
 */
static char *progfile_dir;

#ifdef __APPLE__
/*
 * Directory of the application bundle in which we're contained,
 * if we're contained in an application bundle.  Otherwise, NULL.
 *
 * Note: Table 2-5 "Subdirectories of the Contents directory" of
 *
 *    https://developer.apple.com/library/mac/documentation/CoreFoundation/Conceptual/CFBundles/BundleTypes/BundleTypes.html#//apple_ref/doc/uid/10000123i-CH101-SW1
 *
 * says that the "Frameworks" directory
 *
 *    Contains any private shared libraries and frameworks used by the
 *    executable.  The frameworks in this directory are revision-locked
 *    to the application and cannot be superseded by any other, even
 *    newer, versions that may be available to the operating system.  In
 *    other words, the frameworks included in this directory take precedence
 *    over any other similarly named frameworks found in other parts of
 *    the operating system.  For information on how to add private
 *    frameworks to your application bundle, see Framework Programming Guide.
 *
 * so if we were to ship with any frameworks (e.g. Qt) we should
 * perhaps put them in a Frameworks directory rather than under
 * Resources.
 *
 * It also says that the "PlugIns" directory
 *
 *    Contains loadable bundles that extend the basic features of your
 *    application. You use this directory to include code modules that
 *    must be loaded into your applicationbs process space in order to
 *    be used. You would not use this directory to store standalone
 *    executables.
 *
 * Our plugins are just raw .so/.dylib files; I don't know whether by
 * "bundles" they mean application bundles (i.e., directory hierarchies)
 * or just "bundles" in the Mach-O sense (which are an image type that
 * can be loaded with dlopen() but not linked as libraries; our plugins
 * are, I think, built as dylibs and can be loaded either way).
 *
 * And it says that the "SharedSupport" directory
 *
 *    Contains additional non-critical resources that do not impact the
 *    ability of the application to run. You might use this directory to
 *    include things like document templates, clip art, and tutorials
 *    that your application expects to be present but that do not affect
 *    the ability of your application to run.
 *
 * I don't think I'd put the files that currently go under Resources/share
 * into that category; they're not, for example, sample Lua scripts that
 * don't actually get run by Wireshark, they're configuration/data files
 * for Wireshark whose absence might not prevent Wireshark from running
 * but that would affect how it behaves when run.
 */
static char *appbundle_dir;
#endif

/*
 * TRUE if we're running from the build directory and we aren't running
 * with special privileges.
 */
static gboolean running_in_build_directory_flag = FALSE;

#ifndef _WIN32
/*
 * Get the pathname of the executable using various platform-
 * dependent mechanisms for various UN*Xes.
 *
 * These calls all should return something independent of the argv[0]
 * passed to the program, so it shouldn't be fooled by an argv[0]
 * that doesn't match the executable path.
 *
 * Sadly, not all UN*Xes necessarily have dladdr(), and those that
 * do don't necessarily have dladdr(main) return information about
 * the executable image, and those that do aren't necessarily running
 * on a platform wherein the executable image can get its own path
 * from the kernel (either by a call or by it being handed to it along
 * with argv[] and the environment), and those that can don't
 * necessarily use that to supply the path you get from dladdr(main),
 * so we try this first and, if that fails, use dladdr(main) if
 * available.
 *
 * In particular, some dynamic linkers supply a dladdr() such that
 * dladdr(main) just returns something derived from argv[0], so
 * just using dladdr(main) is the wrong thing to do if there's
 * another mechanism that can get you a more reliable version of
 * the executable path.
 *
 * However, at least in newer versions of DragonFly BSD, the dynamic
 * linker *does* get it from the aux vector passed to the program
 * by the kernel,  readlink /proc/curproc/file - which came first?
 *
 * On OpenBSD, dladdr(main) returns a value derived from argv[0],
 * and there doesn't appear to be any way to get the executable path
 * from the kernel, so we're out of luck there.
 *
 * So, on platforms where some versions have a version of dladdr()
 * that gives an argv[0]-based path and that also have a mechanism
 * to get a more reliable version of the path, we try that.  On
 * other platforms, we return NULL.  If our caller gets back a NULL
 * from us, it falls back on dladdr(main) if dladdr() is available,
 * and if that fails or is unavailable, it falls back on processing
 * argv[0] itself.
 *
 * This is not guaranteed to return an absolute path; if it doesn't,
 * our caller must prepend the current directory if it's a path.
 *
 * This is not guaranteed to return the "real path"; it might return
 * something with symbolic links in the path.  Our caller must
 * use realpath() if they want the real thing, but that's also true of
 * something obtained by looking at argv[0].
 */
static const char *
get_executable_path(void)
{
#if defined(__APPLE__)
    char *executable_path;
    uint32_t path_buf_size;

    path_buf_size = PATH_MAX;
    executable_path = (char *)g_malloc(path_buf_size);
    if (_NSGetExecutablePath(executable_path, &path_buf_size) == -1) {
        executable_path = (char *)g_realloc(executable_path, path_buf_size);
        if (_NSGetExecutablePath(executable_path, &path_buf_size) == -1)
            return NULL;
    }
    return executable_path;
#elif defined(__linux__)
    /*
     * In older versions of GNU libc's dynamic linker, as used on Linux,
     * dladdr(main) supplies a path based on argv[0], so we use
     * /proc/self/exe instead; there are Linux distributions with
     * kernels that support /proc/self/exe and those older versions
     * of the dynamic linker, and this will get a better answer on
     * those versions.
     *
     * It only works on Linux 2.2 or later, so we just give up on
     * earlier versions.
     *
     * XXX - are there OS versions that support "exe" but not "self"?
     */
    struct utsname name;
    static char executable_path[PATH_MAX + 1];
    ssize_t r;

    if (uname(&name) == -1)
        return NULL;
    if (strncmp(name.release, "1.", 2) == 0)
        return NULL; /* Linux 1.x */
    if (strcmp(name.release, "2.0") == 0 ||
        strncmp(name.release, "2.0.", 4) == 0 ||
        strcmp(name.release, "2.1") == 0 ||
        strncmp(name.release, "2.1.", 4) == 0)
        return NULL; /* Linux 2.0.x or 2.1.x */
    if ((r = readlink("/proc/self/exe", executable_path, PATH_MAX)) == -1)
        return NULL;
    executable_path[r] = '\0';
    return executable_path;
#elif defined(__FreeBSD__) && defined(KERN_PROC_PATHNAME)
    /*
     * In older versions of FreeBSD's dynamic linker, dladdr(main)
     * supplies a path based on argv[0], so we use the KERN_PROC_PATHNAME
     * sysctl instead; there are, I think, versions of FreeBSD
     * that support the sysctl that have and those older versions
     * of the dynamic linker, and this will get a better answer on
     * those versions.
     */
    int mib[4];
    char *executable_path;
    size_t path_buf_size;

    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_PATHNAME;
    mib[3] = -1;
    path_buf_size = PATH_MAX;
    executable_path = (char *)g_malloc(path_buf_size);
    if (sysctl(mib, 4, executable_path, &path_buf_size, NULL, 0) == -1) {
        if (errno != ENOMEM)
            return NULL;
        executable_path = (char *)g_realloc(executable_path, path_buf_size);
        if (sysctl(mib, 4, executable_path, &path_buf_size, NULL, 0) == -1)
            return NULL;
    }
    return executable_path;
#elif defined(__NetBSD__)
    /*
     * In all versions of NetBSD's dynamic linker as of 2013-08-12,
     * dladdr(main) supplies a path based on argv[0], so we use
     * /proc/curproc/exe instead.
     *
     * XXX - are there OS versions that support "exe" but not "curproc"
     * or "self"?  Are there any that support "self" but not "curproc"?
     */
    static char executable_path[PATH_MAX + 1];
    ssize_t r;

    if ((r = readlink("/proc/curproc/exe", executable_path, PATH_MAX)) == -1)
        return NULL;
    executable_path[r] = '\0';
    return executable_path;
#elif defined(__DragonFly__)
    /*
     * In older versions of DragonFly BSD's dynamic linker, dladdr(main)
     * supplies a path based on argv[0], so we use /proc/curproc/file
     * instead; it appears to be supported by all versions of DragonFly
     * BSD.
     */
    static char executable_path[PATH_MAX + 1];
    ssize_t r;

    if ((r = readlink("/proc/curproc/file", executable_path, PATH_MAX)) == -1)
        return NULL;
    executable_path[r] = '\0';
    return executable_path;
#elif (defined(sun) || defined(__sun)) && defined(HAVE_GETEXECNAME)
    /*
     * It appears that getexecname() dates back to at least Solaris 8,
     * but /proc/{pid}/path is first documented in the Solaris 10 documentation,
     * so we use getexecname() if available, rather than /proc/self/path/a.out
     * (which isn't documented, but appears to be a symlink to the
     * executable image file).
     */
    return getexecname();
#else
    /* Fill in your favorite UN*X's code here, if there is something */
    return NULL;
#endif
}
#endif /* _WIN32 */

/*
 * Get the pathname of the directory from which the executable came,
 * and save it for future use.  Returns NULL on success, and a
 * g_mallocated string containing an error on failure.
 */
char *
init_progfile_dir(const char *arg0
#ifdef _WIN32
    _U_
#endif
, int (*function_addr)(int, char **)
#if defined(_WIN32) || !defined(HAVE_DLADDR)
    _U_
#endif
)
{
#ifdef _WIN32
    TCHAR prog_pathname_w[_MAX_PATH+2];
    char *prog_pathname;
    DWORD error;
    TCHAR *msg_w;
    guchar *msg;
    size_t msglen;

    /*
     * Attempt to get the full pathname of the currently running
     * program.
     */
    if (GetModuleFileName(NULL, prog_pathname_w, G_N_ELEMENTS(prog_pathname_w)) != 0 && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        /*
         * XXX - Should we use g_utf16_to_utf8()?
         */
        prog_pathname = utf_16to8(prog_pathname_w);
        /*
         * We got it; strip off the last component, which would be
         * the file name of the executable, giving us the pathname
         * of the directory where the executable resides.
         */
        progfile_dir = g_path_get_dirname(prog_pathname);
        if (progfile_dir != NULL) {
            return NULL;    /* we succeeded */
        } else {
            /*
             * OK, no. What do we do now?
             */
            return g_strdup_printf("No \\ in executable pathname \"%s\"",
                prog_pathname);
        }
    } else {
        /*
         * Oh, well.  Return an indication of the error.
         */
        error = GetLastError();
        if (FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS,
            NULL, error, 0, (LPTSTR) &msg_w, 0, NULL) == 0) {
            /*
             * Gak.  We can't format the message.
             */
            return g_strdup_printf("GetModuleFileName failed: %u (FormatMessage failed: %u)",
                error, GetLastError());
        }
        msg = utf_16to8(msg_w);
        LocalFree(msg_w);
        /*
         * "FormatMessage()" "helpfully" sticks CR/LF at the
         * end of the message.  Get rid of it.
         */
        msglen = strlen(msg);
        if (msglen >= 2) {
            msg[msglen - 1] = '\0';
            msg[msglen - 2] = '\0';
        }
        return g_strdup_printf("GetModuleFileName failed: %s (%u)",
            msg, error);
    }
#else
#ifdef HAVE_DLADDR
    Dl_info info;
#endif
    const char *execname;
    char *prog_pathname;
    char *curdir;
    long path_max;
    const char *pathstr;
    const char *path_start, *path_end;
    size_t path_component_len, path_len;
    char *retstr;
    char *path;
    char *dir_end;

    /*
     * Check whether WIRESHARK_RUN_FROM_BUILD_DIRECTORY is set in the
     * environment; if so, set running_in_build_directory_flag if we
     * weren't started with special privileges.  (If we were started
     * with special privileges, it's not safe to allow the user to point
     * us to some other directory; running_in_build_directory_flag, when
     * set, causes us to look for plugins and the like in the build
     * directory.)
     */
    if (g_getenv("WIRESHARK_RUN_FROM_BUILD_DIRECTORY") != NULL
        && !started_with_special_privs())
        running_in_build_directory_flag = TRUE;

    execname = get_executable_path();
#ifdef HAVE_DLADDR
    if (function_addr != NULL && execname == NULL) {
        /*
         * Try to use dladdr() to find the pathname of the executable.
         * dladdr() is not guaranteed to give you anything better than
         * argv[0] (i.e., it might not contain a / at all, much less
         * being an absolute path), and doesn't appear to do so on
         * Linux, but on other platforms it could give you an absolute
         * path and obviate the need for us to determine the absolute
         * path.
         */
DIAG_OFF(pedantic)
        if (dladdr((void *)function_addr, &info)) {
DIAG_ON(pedantic)
            execname = info.dli_fname;
        }
    }
#endif
    if (execname == NULL) {
        /*
         * OK, guess based on argv[0].
         */
        execname = arg0;
    }

    /*
     * Try to figure out the directory in which the currently running
     * program resides, given something purporting to be the executable
     * name (from dladdr() or from the argv[0] it was started with.
     * That might be the absolute path of the program, or a path relative
     * to the current directory of the process that started it, or
     * just a name for the program if it was started from the command
     * line and was searched for in $PATH.  It's not guaranteed to be
     * any of those, however, so there are no guarantees....
     */
    if (execname[0] == '/') {
        /*
         * It's an absolute path.
         */
        prog_pathname = g_strdup(execname);
    } else if (strchr(execname, '/') != NULL) {
        /*
         * It's a relative path, with a directory in it.
         * Get the current directory, and combine it
         * with that directory.
         */
        path_max = pathconf(".", _PC_PATH_MAX);
        if (path_max == -1) {
            /*
             * We have no idea how big a buffer to
             * allocate for the current directory.
             */
            return g_strdup_printf("pathconf failed: %s\n",
                g_strerror(errno));
        }
        curdir = (char *)g_malloc(path_max);
        if (getcwd(curdir, path_max) == NULL) {
            /*
             * It failed - give up, and just stick
             * with DATAFILE_DIR.
             */
            g_free(curdir);
            return g_strdup_printf("getcwd failed: %s\n",
                g_strerror(errno));
        }
        path = g_strdup_printf("%s/%s", curdir, execname);
        g_free(curdir);
        prog_pathname = path;
    } else {
        /*
         * It's just a file name.
         * Search the path for a file with that name
         * that's executable.
         */
        prog_pathname = NULL;   /* haven't found it yet */
        pathstr = g_getenv("PATH");
        path_start = pathstr;
        if (path_start != NULL) {
            while (*path_start != '\0') {
                path_end = strchr(path_start, ':');
                if (path_end == NULL)
                    path_end = path_start + strlen(path_start);
                path_component_len = path_end - path_start;
                path_len = path_component_len + 1
                    + strlen(execname) + 1;
                path = (char *)g_malloc(path_len);
                memcpy(path, path_start, path_component_len);
                path[path_component_len] = '\0';
                g_strlcat(path, "/", path_len);
                g_strlcat(path, execname, path_len);
                if (access(path, X_OK) == 0) {
                    /*
                     * Found it!
                     */
                    prog_pathname = path;
                    break;
                }

                /*
                 * That's not it.  If there are more
                 * path components to test, try them.
                 */
                if (*path_end == '\0') {
                    /*
                     * There's nothing more to try.
                     */
                    break;
                }
                if (*path_end == ':')
                    path_end++;
                path_start = path_end;
                g_free(path);
            }
            if (prog_pathname == NULL) {
                /*
                 * Program not found in path.
                 */
                return g_strdup_printf("\"%s\" not found in \"%s\"",
                    execname, pathstr);
            }
        } else {
            /*
             * PATH isn't set.
             * XXX - should we pick a default?
             */
            return g_strdup("PATH isn't set");
        }
    }

    /*
     * OK, we have what we think is the pathname
     * of the program.
     *
     * First, find the last "/" in the directory,
     * as that marks the end of the directory pathname.
     */
    dir_end = strrchr(prog_pathname, '/');
    if (dir_end != NULL) {
        /*
         * Found it.  Strip off the last component,
         * as that's the path of the program.
         */
        *dir_end = '\0';

        /*
         * Is there a "/.libs" at the end?
         */
        dir_end = strrchr(prog_pathname, '/');
        if (dir_end != NULL) {
            if (strcmp(dir_end, "/.libs") == 0) {
                /*
                 * Yup, it's ".libs".
                 * Strip that off; it's an
                 * artifact of libtool.
                 */
                *dir_end = '\0';

                /*
                 * This presumably means we're run from
                 * the libtool wrapper, which probably
                 * means we're being run from the build
                 * directory.  If we weren't started
                 * with special privileges, set
                 * running_in_build_directory_flag.
                 *
                 * XXX - should we check whether what
                 * follows ".libs/" begins with "lt-"?
                 */
                if (!started_with_special_privs())
                    running_in_build_directory_flag = TRUE;
            }
            else if (!started_with_special_privs()) {
                /*
                 * Check for the CMake output directory. As people may name
                 * their directories "run" (really?), also check for the
                 * CMakeCache.txt file before assuming a CMake output dir.
                 */
                if (strcmp(dir_end, "/run") == 0) {
                    gchar *cmake_file;
                    cmake_file = g_strdup_printf("%.*s/CMakeCache.txt",
                                                 (int)(dir_end - prog_pathname),
                                                 prog_pathname);
                    if (file_exists(cmake_file))
                        running_in_build_directory_flag = TRUE;
                    g_free(cmake_file);
                }
#ifdef __APPLE__
                if (!running_in_build_directory_flag) {
                    /*
                     * Scan up the path looking for a component
                     * named "Contents".  If we find it, we assume
                     * we're in a bundle, and that the top-level
                     * directory of the bundle is the one containing
                     * "Contents".
                     *
                     * Not all executables are in the Contents/MacOS
                     * directory, so we can't just check for those
                     * in the path and strip them off.
                     *
                     * XXX - should we assume that it's either
                     * Contents/MacOS or Resources/bin?
                     */
                    char *component_end, *p;

                    component_end = strchr(prog_pathname, '\0');
                    p = component_end;
                    for (;;) {
                        while (p >= prog_pathname && *p != '/')
                            p--;
                        if (p == prog_pathname) {
                            /*
                             * We're looking at the first component of
                             * the pathname now, so we're definitely
                             * not in a bundle, even if we're in
                             * "/Contents".
                             */
                            break;
                        }
                        if (strncmp(p, "/Contents", component_end - p) == 0) {
                            /* Found it. */
                            appbundle_dir = (char *)g_malloc(p - prog_pathname + 1);
                            memcpy(appbundle_dir, prog_pathname, p - prog_pathname);
                            appbundle_dir[p - prog_pathname] = '\0';
                            break;
                        }
                        component_end = p;
                        p--;
                    }
                }
#endif
            }
        }

        /*
         * OK, we have the path we want.
         */
        progfile_dir = prog_pathname;
        return NULL;
    } else {
        /*
         * This "shouldn't happen"; we apparently
         * have no "/" in the pathname.
         * Just free up prog_pathname.
         */
        retstr = g_strdup_printf("No / found in \"%s\"", prog_pathname);
        g_free(prog_pathname);
        return retstr;
    }
#endif
}

/*
 * Get the directory in which the program resides.
 */
const char *
get_progfile_dir(void)
{
    return progfile_dir;
}

/*
 * Get the directory in which the global configuration and data files are
 * stored.
 *
 * On Windows, we use the directory in which the executable for this
 * process resides.
 *
 * On UN*X, we use the DATAFILE_DIR value supplied by the configure
 * script, unless we think we're being run from the build directory,
 * in which case we use the directory in which the executable for this
 * process resides.
 *
 * XXX - if we ever make libwireshark a real library, used by multiple
 * applications (more than just TShark and versions of Wireshark with
 * various UIs), should the configuration files belong to the library
 * (and be shared by all those applications) or to the applications?
 *
 * If they belong to the library, that could be done on UNIX by the
 * configure script, but it's trickier on Windows, as you can't just
 * use the pathname of the executable.
 *
 * If they belong to the application, that could be done on Windows
 * by using the pathname of the executable, but we'd have to have it
 * passed in as an argument, in some call, on UNIX.
 *
 * Note that some of those configuration files might be used by code in
 * libwireshark, some of them might be used by dissectors (would they
 * belong to libwireshark, the application, or a separate library?),
 * and some of them might be used by other code (the Wireshark preferences
 * file includes resolver preferences that control the behavior of code
 * in libwireshark, dissector preferences, and UI preferences, for
 * example).
 */
const char *
get_datafile_dir(void)
{
    static const char *datafile_dir = NULL;

    if (datafile_dir != NULL)
        return datafile_dir;

#ifdef _WIN32
    /*
     * Do we have the pathname of the program?  If so, assume we're
     * running an installed version of the program.  If we fail,
     * we don't change "datafile_dir", and thus end up using the
     * default.
     *
     * XXX - does NSIS put the installation directory into
     * "\HKEY_LOCAL_MACHINE\SOFTWARE\Wireshark\InstallDir"?
     * If so, perhaps we should read that from the registry,
     * instead.
     */
    if (progfile_dir != NULL) {
        /*
         * Yes, we do; use that.
         */
        datafile_dir = progfile_dir;
    } else {
        /*
         * No, we don't.
         * Fall back on the default installation directory.
         */
        datafile_dir = "C:\\Program Files\\Wireshark\\";
    }
#else

    if (running_in_build_directory_flag) {
        /*
         * We're (probably) being run from the build directory and
         * weren't started with special privileges.
         *
         * (running_in_build_directory_flag is never set to TRUE
         * if we're started with special privileges, so we need
         * only check it; we don't need to call started_with_special_privs().)
         *
         * Use the top-level source directory as the datafile directory
         * because most of our data files (radius/, COPYING) are there.
         */
        datafile_dir = g_strdup(TOP_SRCDIR);
        return datafile_dir;
    } else {
        if (g_getenv("WIRESHARK_DATA_DIR") && !started_with_special_privs()) {
            /*
             * The user specified a different directory for data files
             * and we aren't running with special privileges.
             * XXX - We might be able to dispense with the priv check
             */
            datafile_dir = g_strdup(g_getenv("WIRESHARK_DATA_DIR"));
        }
#ifdef __APPLE__
        /*
         * If we're running from an app bundle and weren't started
         * with special privileges, use the Contents/Resources/share/wireshark
         * subdirectory of the app bundle.
         *
         * (appbundle_dir is not set to a non-null value if we're
         * started with special privileges, so we need only check
         * it; we don't need to call started_with_special_privs().)
         */
        else if (appbundle_dir != NULL) {
            datafile_dir = g_strdup_printf("%s/Contents/Resources/share/wireshark",
                                           appbundle_dir);
        }
#endif
        else {
            datafile_dir = DATAFILE_DIR;
        }
    }

#endif
    return datafile_dir;
}

#if defined(HAVE_PLUGINS) || defined(HAVE_LUA)
/*
 * Find the directory where the plugins are stored.
 *
 * On Windows, we use the "plugin" subdirectory of the datafile directory.
 *
 * On UN*X, we use the PLUGIN_INSTALL_DIR value supplied by the configure
 * script, unless we think we're being run from the build directory,
 * in which case we use the "plugin" subdirectory of the datafile directory.
 *
 * In both cases, we then use the subdirectory of that directory whose
 * name is the version number.
 *
 * XXX - if we think we're being run from the build directory, perhaps we
 * should have the plugin code not look in the version subdirectory
 * of the plugin directory, but look in all of the subdirectories
 * of the plugin directory, so it can just fetch the plugins built
 * as part of the build process.
 */
static const char *plugin_dir = NULL;

static void
init_plugin_dir(void)
{
#ifdef _WIN32
    /*
     * On Windows, the data file directory is the installation
     * directory; the plugins are stored under it.
     *
     * Assume we're running the installed version of Wireshark;
     * on Windows, the data file directory is the directory
     * in which the Wireshark binary resides.
     */
    plugin_dir = g_strdup_printf("%s\\plugins\\%s", get_datafile_dir(),
                     VERSION);

    /*
     * Make sure that pathname refers to a directory.
     */
    if (test_for_directory(plugin_dir) != EISDIR) {
        /*
         * Either it doesn't refer to a directory or it
         * refers to something that doesn't exist.
         *
         * Assume that means we're running a version of
         * Wireshark we've built in a build directory,
         * in which case {datafile dir}\plugins is the
         * top-level plugins source directory, and use
         * that directory and set the "we're running in
         * a build directory" flag, so the plugin
         * scanner will check all subdirectories of that
         * directory for plugins.
         */
        g_free( (gpointer) plugin_dir);
        plugin_dir = g_strdup_printf("%s\\plugins", get_datafile_dir());
        running_in_build_directory_flag = TRUE;
    }
#else
    if (running_in_build_directory_flag) {
        /*
         * We're (probably) being run from the build directory and
         * weren't started with special privileges, so we'll use
         * the "plugins" subdirectory of the directory where the program
         * we're running is (that's the build directory).
         */
        plugin_dir = g_strdup_printf("%s/plugins", get_progfile_dir());
    } else {
        if (g_getenv("WIRESHARK_PLUGIN_DIR") && !started_with_special_privs()) {
            /*
             * The user specified a different directory for plugins
             * and we aren't running with special privileges.
             */
            plugin_dir = g_strdup(g_getenv("WIRESHARK_PLUGIN_DIR"));
        }
#ifdef __APPLE__
        /*
         * If we're running from an app bundle and weren't started
         * with special privileges, use the Contents/PlugIns/wireshark
         * subdirectory of the app bundle.
         *
         * (appbundle_dir is not set to a non-null value if we're
         * started with special privileges, so we need only check
         * it; we don't need to call started_with_special_privs().)
         */
        else if (appbundle_dir != NULL) {
            plugin_dir = g_strdup_printf("%s/Contents/PlugIns/wireshark",
                                         appbundle_dir);
        }
#endif
        else {
            plugin_dir = PLUGIN_INSTALL_DIR;
        }
    }
#endif
}
#endif /* HAVE_PLUGINS || HAVE_LUA */

/*
 * Get the directory in which the plugins are stored.
 */
const char *
get_plugin_dir(void)
{
#if defined(HAVE_PLUGINS) || defined(HAVE_LUA)
    if (!plugin_dir) init_plugin_dir();
    return plugin_dir;
#else
    return NULL;
#endif
}

#if defined(HAVE_EXTCAP)
/*
 * Find the directory where the extcap hooks are stored.
 *
 * On Windows, we use the "extcap" subdirectory of the datafile directory.
 *
 * On UN*X, we use the EXTCAP_DIR value supplied by the configure
 * script, unless we think we're being run from the build directory,
 * in which case we use the "extcap" subdirectory of the datafile directory.
 *
 * In both cases, we then use the subdirectory of that directory whose
 * name is the version number.
 *
 * XXX - if we think we're being run from the build directory, perhaps we
 * should have the extcap code not look in the version subdirectory
 * of the extcap directory, but look in all of the subdirectories
 * of the extcap directory, so it can just fetch the extcap hooks built
 * as part of the build process.
 */
static const char *extcap_dir = NULL;

static void init_extcap_dir(void) {
#ifdef _WIN32
    const char *alt_extcap_path;

    /*
     * On Windows, the data file directory is the installation
     * directory; the extcap hooks are stored under it.
     *
     * Assume we're running the installed version of Wireshark;
     * on Windows, the data file directory is the directory
     * in which the Wireshark binary resides.
     */
    alt_extcap_path = g_getenv("WIRESHARK_EXTCAP_DIR");
    if (alt_extcap_path) {
        /*
         * The user specified a different directory for extcap hooks.
         */
        extcap_dir = g_strdup(alt_extcap_path);
    } else {
        extcap_dir = g_strdup_printf("%s\\extcap", get_datafile_dir());
    }
#else
    if (running_in_build_directory_flag) {
        /*
         * We're (probably) being run from the build directory and
         * weren't started with special privileges, so we'll use
         * the "extcap hooks" subdirectory of the directory where the program
         * we're running is (that's the build directory).
         */
        extcap_dir = g_strdup_printf("%s/extcap", get_progfile_dir());
    } else {
        if (g_getenv("WIRESHARK_EXTCAP_DIR") && !started_with_special_privs()) {
            /*
             * The user specified a different directory for extcap hooks
             * and we aren't running with special privileges.
             */
            extcap_dir = g_strdup(g_getenv("WIRESHARK_EXTCAP_DIR"));
        }
#ifdef __APPLE__
        /*
         * If we're running from an app bundle and weren't started
         * with special privileges, use the Contents/MacOS/extcap
         * subdirectory of the app bundle.
         *
         * (appbundle_dir is not set to a non-null value if we're
         * started with special privileges, so we need only check
         * it; we don't need to call started_with_special_privs().)
         */
        else if (appbundle_dir != NULL) {
            extcap_dir = g_strdup_printf("%s/Contents/MacOS/extcap",
                                         appbundle_dir);
        }
#endif
        else {
            extcap_dir = EXTCAP_DIR;
        }
    }
#endif
}
#endif /* HAVE_EXTCAP */

/*
 * Get the directory in which the extcap hooks are stored.
 *
 * XXX - A fix instead of HAVE_EXTCAP must be found
 */
const char *
get_extcap_dir(void) {
#if defined(HAVE_EXTCAP)
    if (!extcap_dir)
        init_extcap_dir();
    return extcap_dir;
#else
    return NULL;
#endif
}

/*
 * Get the flag indicating whether we're running from a build
 * directory.
 */
gboolean
running_in_build_directory(void)
{
    return running_in_build_directory_flag;
}

/*
 * Get the directory in which files that, at least on UNIX, are
 * system files (such as "/etc/ethers") are stored; on Windows,
 * there's no "/etc" directory, so we get them from the global
 * configuration and data file directory.
 */
const char *
get_systemfile_dir(void)
{
#ifdef _WIN32
    return get_datafile_dir();
#else
    return "/etc";
#endif
}

void
set_profile_name(const gchar *profilename)
{
    g_free (persconfprofile);

    if (profilename && strlen(profilename) > 0 &&
        strcmp(profilename, DEFAULT_PROFILE) != 0) {
        persconfprofile = g_strdup (profilename);
    } else {
        /* Default Profile */
        persconfprofile = NULL;
    }
}

const char *
get_profile_name(void)
{
    if (persconfprofile) {
        return persconfprofile;
    } else {
        return DEFAULT_PROFILE;
    }
}

gboolean
is_default_profile(void)
{
    return (!persconfprofile || strcmp(persconfprofile, DEFAULT_PROFILE) == 0) ? TRUE : FALSE;
}

gboolean
has_global_profiles(void)
{
    WS_DIR *dir;
    WS_DIRENT *file;
    const gchar *global_dir = get_global_profiles_dir();
    gchar *filename;
    gboolean has_global = FALSE;

    if ((test_for_directory(global_dir) == EISDIR) &&
        ((dir = ws_dir_open(global_dir, 0, NULL)) != NULL))
    {
        while ((file = ws_dir_read_name(dir)) != NULL) {
            filename = g_strdup_printf ("%s%s%s", global_dir, G_DIR_SEPARATOR_S,
                            ws_dir_get_name(file));
            if (test_for_directory(filename) == EISDIR) {
                has_global = TRUE;
                g_free (filename);
                break;
            }
            g_free (filename);
        }
        ws_dir_close(dir);
    }

    return has_global;
}

void
profile_store_persconffiles(gboolean store)
{
    if (store) {
        profile_files = g_hash_table_new (g_str_hash, g_str_equal);
    }
    do_store_persconffiles = store;
}

/*
 * Get the directory in which personal configuration files reside.
 *
 * On Windows, it's "Wireshark", under %APPDATA% or, if %APPDATA% isn't set,
 * it's "%USERPROFILE%\Application Data" (which is what %APPDATA% normally
 * is on Windows 2000).
 *
 * On UNIX-compatible systems, we first look in XDG_CONFIG_HOME/wireshark
 * and, if that doesn't exist, ~/.wireshark, for backwards compatibility.
 * If neither exists, we use XDG_CONFIG_HOME/wireshark, so that the directory
 * is initially created as XDG_CONFIG_HOME/wireshark.  We use that regardless
 * of whether the user is running under an XDG desktop or not, so that
 * if the user's home directory is on a server and shared between
 * different desktop environments on different machines, they can all
 * share the same configuration file directory.
 *
 * XXX - what about stuff that shouldn't be shared between machines,
 * such as plugins in the form of shared loadable images?
 */
static const char *
get_persconffile_dir_no_profile(void)
{
#ifdef _WIN32
    const char *env;
#else
    char *xdg_path, *path;
    struct passwd *pwd;
    const char *homedir;
#endif

    /* Return the cached value, if available */
    if (persconffile_dir != NULL)
        return persconffile_dir;

#ifdef _WIN32
    /*
     * See if the user has selected an alternate environment.
     */
    env = g_getenv("WIRESHARK_APPDATA");
    if (env != NULL) {
        persconffile_dir = g_strdup(env);
        return persconffile_dir;
    }

    /*
     * Use %APPDATA% or %USERPROFILE%, so that configuration
     * files are stored in the user profile, rather than in
     * the home directory.  The Windows convention is to store
     * configuration information in the user profile, and doing
     * so means you can use Wireshark even if the home directory
     * is an inaccessible network drive.
     */
    env = g_getenv("APPDATA");
    if (env != NULL) {
        /*
         * Concatenate %APPDATA% with "\Wireshark".
         */
        persconffile_dir = g_build_filename(env, "Wireshark", NULL);
        return persconffile_dir;
    }

    /*
     * OK, %APPDATA% wasn't set, so use %USERPROFILE%\Application Data.
     */
    env = g_getenv("USERPROFILE");
    if (env != NULL) {
        persconffile_dir = g_build_filename(env, "Application Data", "Wireshark", NULL);
        return persconffile_dir;
    }

    /*
     * Give up and use "C:".
     */
    persconffile_dir = g_build_filename("C:", "Wireshark", NULL);
    return persconffile_dir;
#else
    /*
     * Check if XDG_CONFIG_HOME/wireshark exists and is a directory.
     */
    xdg_path = g_build_filename(g_get_user_config_dir(), "wireshark", NULL);
    if (g_file_test(xdg_path, G_FILE_TEST_IS_DIR)) {
        persconffile_dir = xdg_path;
        return persconffile_dir;
    }

    /*
     * It doesn't exist, or it does but isn't a directory, so try
     * ~/.wireshark.
     *
     * If $HOME is set, use that for ~.
     *
     * (Note: before GLib 2.36, g_get_home_dir() didn't look at $HOME,
     * but we always want to do so, so we don't use g_get_home_dir().)
     */
    homedir = g_getenv("HOME");
    if (homedir == NULL) {
        /*
         * It's not set.
         *
         * Get their home directory from the password file.
         * If we can't even find a password file entry for them,
         * use "/tmp".
         */
        pwd = getpwuid(getuid());
        if (pwd != NULL) {
            homedir = pwd->pw_dir;
        } else {
            homedir = "/tmp";
        }
    }
    path = g_build_filename(homedir, ".wireshark", NULL);
    if (g_file_test(path, G_FILE_TEST_IS_DIR)) {
        g_free(xdg_path);
        persconffile_dir = path;
        return persconffile_dir;
    }

    /*
     * Neither are directories that exist; use the XDG path, so we'll
     * create that as necessary.
     */
    g_free(path);
    persconffile_dir = xdg_path;
    return persconffile_dir;
#endif
}

void
set_persconffile_dir(const char *p)
{
    g_free(persconffile_dir);
    persconffile_dir = g_strdup(p);
}

const char *
get_profiles_dir(void)
{
    static char *profiles_dir = NULL;

    g_free (profiles_dir);
    profiles_dir = g_strdup_printf ("%s%s%s", get_persconffile_dir_no_profile (),
                    G_DIR_SEPARATOR_S, PROFILES_DIR);

    return profiles_dir;
}

const char *
get_global_profiles_dir(void)
{
    static char *global_profiles_dir = NULL;

    if (!global_profiles_dir) {
        global_profiles_dir = g_strdup_printf ("%s%s%s", get_datafile_dir(),
                               G_DIR_SEPARATOR_S, PROFILES_DIR);
    }

    return global_profiles_dir;
}

static const char *
get_persconffile_dir(const gchar *profilename)
{
    static char *persconffile_profile_dir = NULL;

    g_free (persconffile_profile_dir);

    if (profilename && strlen(profilename) > 0 &&
        strcmp(profilename, DEFAULT_PROFILE) != 0) {
      persconffile_profile_dir = g_strdup_printf ("%s%s%s", get_profiles_dir (),
                              G_DIR_SEPARATOR_S, profilename);
    } else {
      persconffile_profile_dir = g_strdup (get_persconffile_dir_no_profile ());
    }

    return persconffile_profile_dir;
}

gboolean
profile_exists(const gchar *profilename, gboolean global)
{
    if (global) {
        gchar *path = g_strdup_printf ("%s%s%s", get_global_profiles_dir(),
                           G_DIR_SEPARATOR_S, profilename);
        if (test_for_directory (path) == EISDIR) {
            g_free (path);
            return TRUE;
        }
        g_free (path);
    } else {
        if (test_for_directory (get_persconffile_dir (profilename)) == EISDIR) {
            return TRUE;
        }
    }

    return FALSE;
}

static int
delete_directory (const char *directory, char **pf_dir_path_return)
{
    WS_DIR *dir;
    WS_DIRENT *file;
    gchar *filename;
    int ret = 0;

    if ((dir = ws_dir_open(directory, 0, NULL)) != NULL) {
        while ((file = ws_dir_read_name(dir)) != NULL) {
            filename = g_strdup_printf ("%s%s%s", directory, G_DIR_SEPARATOR_S,
                            ws_dir_get_name(file));
            if (test_for_directory(filename) != EISDIR) {
                ret = ws_remove(filename);
#if 0
            } else {
                /* The user has manually created a directory in the profile directory */
                /* I do not want to delete the directory recursively yet */
                ret = delete_directory (filename, pf_dir_path_return);
#endif
            }
            if (ret != 0) {
                *pf_dir_path_return = filename;
                break;
            }
            g_free (filename);
        }
        ws_dir_close(dir);
    }

    if (ret == 0 && (ret = ws_remove(directory)) != 0) {
        *pf_dir_path_return = g_strdup (directory);
    }

    return ret;
}

int
delete_persconffile_profile(const char *profilename, char **pf_dir_path_return)
{
    const char *profile_dir = get_persconffile_dir(profilename);
    int ret = 0;

    if (test_for_directory (profile_dir) == EISDIR) {
        ret = delete_directory (profile_dir, pf_dir_path_return);
    }

    return ret;
}

int
rename_persconffile_profile(const char *fromname, const char *toname,
                char **pf_from_dir_path_return, char **pf_to_dir_path_return)
{
    char *from_dir = g_strdup (get_persconffile_dir(fromname));
    char *to_dir = g_strdup (get_persconffile_dir(toname));
    int ret = 0;

    ret = ws_rename (from_dir, to_dir);
    if (ret != 0) {
        *pf_from_dir_path_return = g_strdup (from_dir);
        *pf_to_dir_path_return = g_strdup (to_dir);
    }

    g_free (from_dir);
    g_free (to_dir);

    return ret;
}

/*
 * Create the directory that holds personal configuration files, if
 * necessary.  If we attempted to create it, and failed, return -1 and
 * set "*pf_dir_path_return" to the pathname of the directory we failed
 * to create (it's g_mallocated, so our caller should free it); otherwise,
 * return 0.
 */
int
create_persconffile_profile(const char *profilename, char **pf_dir_path_return)
{
    const char *pf_dir_path;
#ifdef _WIN32
    char *pf_dir_path_copy, *pf_dir_parent_path;
    size_t pf_dir_parent_path_len;
#endif
    ws_statb64 s_buf;
    int ret;
    int save_errno;

    if (profilename) {
        /*
         * Create the "Default" personal configuration files directory, if necessary.
         */
        if (create_persconffile_profile (NULL, pf_dir_path_return) == -1) {
            return -1;
        }

        /*
         * Check if profiles directory exists.
         * If not then create it.
         */
        pf_dir_path = get_profiles_dir ();
        if (ws_stat64(pf_dir_path, &s_buf) != 0) {
            if (errno != ENOENT) {
                /* Some other problem; give up now. */
                save_errno = errno;
                *pf_dir_path_return = g_strdup(pf_dir_path);
                errno = save_errno;
                return -1;
            }

            /*
             * It doesn't exist; try to create it.
             */
            ret = ws_mkdir(pf_dir_path, 0755);
            if (ret == -1) {
                *pf_dir_path_return = g_strdup(pf_dir_path);
                return ret;
            }
        }
    }

    pf_dir_path = get_persconffile_dir(profilename);
    if (ws_stat64(pf_dir_path, &s_buf) != 0) {
        if (errno != ENOENT) {
            /* Some other problem; give up now. */
            save_errno = errno;
            *pf_dir_path_return = g_strdup(pf_dir_path);
            errno = save_errno;
            return -1;
        }
#ifdef _WIN32
        /*
         * Does the parent directory of that directory
         * exist?  %APPDATA% may not exist even though
         * %USERPROFILE% does.
         *
         * We check for the existence of the directory
         * by first checking whether the parent directory
         * is just a drive letter and, if it's not, by
         * doing a "stat()" on it.  If it's a drive letter,
         * or if the "stat()" succeeds, we assume it exists.
         */
        pf_dir_path_copy = g_strdup(pf_dir_path);
        pf_dir_parent_path = get_dirname(pf_dir_path_copy);
        pf_dir_parent_path_len = strlen(pf_dir_parent_path);
        if (pf_dir_parent_path_len > 0
            && pf_dir_parent_path[pf_dir_parent_path_len - 1] != ':'
            && ws_stat64(pf_dir_parent_path, &s_buf) != 0) {
            /*
             * Not a drive letter and the stat() failed.
             */
            if (errno != ENOENT) {
                /* Some other problem; give up now. */
                save_errno = errno;
                *pf_dir_path_return = g_strdup(pf_dir_path);
                errno = save_errno;
                return -1;
            }
            /*
             * No, it doesn't exist - make it first.
             */
            ret = ws_mkdir(pf_dir_parent_path, 0755);
            if (ret == -1) {
                *pf_dir_path_return = pf_dir_parent_path;
                return -1;
            }
        }
        g_free(pf_dir_path_copy);
        ret = ws_mkdir(pf_dir_path, 0755);
#else
        ret = g_mkdir_with_parents(pf_dir_path, 0755);
#endif
    } else {
        /*
         * Something with that pathname exists; if it's not
         * a directory, we'll get an error if we try to put
         * something in it, so we don't fail here, we wait
         * for that attempt fo fail.
         */
        ret = 0;
    }
    if (ret == -1)
        *pf_dir_path_return = g_strdup(pf_dir_path);
    return ret;
}

int
create_persconffile_dir(char **pf_dir_path_return)
{
    return create_persconffile_profile(persconfprofile, pf_dir_path_return);
}

int
copy_persconffile_profile(const char *toname, const char *fromname, gboolean from_global,
              char **pf_filename_return, char **pf_to_dir_path_return, char **pf_from_dir_path_return)
{
    gchar *from_dir;
    gchar *to_dir = g_strdup (get_persconffile_dir(toname));
    gchar *filename, *from_file, *to_file;
    GList *files, *file;

    if (from_global) {
        if (strcmp(fromname, DEFAULT_PROFILE) == 0) {
            from_dir = g_strdup (get_global_profiles_dir());
        } else {
            from_dir = g_strdup_printf ("%s%s%s", get_global_profiles_dir(), G_DIR_SEPARATOR_S, fromname);
        }
    } else {
        from_dir = g_strdup (get_persconffile_dir(fromname));
    }

    files = g_hash_table_get_keys(profile_files);
    file = g_list_first(files);
    while (file) {
        filename = (gchar *)file->data;
        from_file = g_strdup_printf ("%s%s%s", from_dir, G_DIR_SEPARATOR_S, filename);
        to_file =  g_strdup_printf ("%s%s%s", to_dir, G_DIR_SEPARATOR_S, filename);

        if (file_exists(from_file) && !copy_file_binary_mode(from_file, to_file)) {
            *pf_filename_return = g_strdup(filename);
            *pf_to_dir_path_return = to_dir;
            *pf_from_dir_path_return = from_dir;
            g_free (from_file);
            g_free (to_file);
            return -1;
        }

        g_free (from_file);
        g_free (to_file);

        file = g_list_next(file);
    }

    g_list_free (files);
    g_free (from_dir);
    g_free (to_dir);

    return 0;
}

/*
 * Get the (default) directory in which personal data is stored.
 *
 * On Win32, this is the "My Documents" folder in the personal profile.
 * On UNIX this is simply the current directory.
 */
/* XXX - should this and the get_home_dir() be merged? */
extern const char *
get_persdatafile_dir(void)
{
#ifdef _WIN32
    TCHAR tszPath[MAX_PATH];

    /* Return the cached value, if available */
    if (persdatafile_dir != NULL)
        return persdatafile_dir;

    /*
     * Hint: SHGetFolderPath is not available on MSVC 6 - without
     * Platform SDK
     */
    if (SHGetSpecialFolderPath(NULL, tszPath, CSIDL_PERSONAL, FALSE)) {
        persdatafile_dir = g_utf16_to_utf8(tszPath, -1, NULL, NULL, NULL);
        return persdatafile_dir;
    } else {
        return "";
    }
#else
    return "";
#endif
}

void
set_persdatafile_dir(const char *p)
{
    g_free(persdatafile_dir);
    persdatafile_dir = g_strdup(p);
}

#ifdef _WIN32
/*
 * Returns the user's home directory on Win32.
 */
static const char *
get_home_dir(void)
{
    static const char *home = NULL;
    const char *homedrive, *homepath;
    char *homestring;
    char *lastsep;

    /* Return the cached value, if available */
    if (home)
        return home;

    /*
     * XXX - should we use USERPROFILE anywhere in this process?
     * Is there a chance that it might be set but one or more of
     * HOMEDRIVE or HOMEPATH isn't set?
     */
    homedrive = g_getenv("HOMEDRIVE");
    if (homedrive != NULL) {
        homepath = g_getenv("HOMEPATH");
        if (homepath != NULL) {
            /*
             * This is cached, so we don't need to worry about
             * allocating multiple ones of them.
             */
            homestring = g_strdup_printf("%s%s", homedrive, homepath);

            /*
             * Trim off any trailing slash or backslash.
             */
            lastsep = find_last_pathname_separator(homestring);
            if (lastsep != NULL && *(lastsep + 1) == '\0') {
                /*
                 * Last separator is the last character
                 * in the string.  Nuke it.
                 */
                *lastsep = '\0';
            }
            home = homestring;
        } else
            home = homedrive;
    } else {
        /*
         * Give up and use C:.
         */
        home = "C:";
    }

    return home;
}
#endif

/*
 * Construct the path name of a personal configuration file, given the
 * file name.
 *
 * On Win32, if "for_writing" is FALSE, we check whether the file exists
 * and, if not, construct a path name relative to the ".wireshark"
 * subdirectory of the user's home directory, and check whether that
 * exists; if it does, we return that, so that configuration files
 * from earlier versions can be read.
 *
 * The returned file name was g_malloc()'d so it must be g_free()d when the
 * caller is done with it.
 */
char *
get_persconffile_path(const char *filename, gboolean from_profile)
{
    char *path;
    if (do_store_persconffiles && from_profile && !g_hash_table_lookup (profile_files, filename)) {
        /* Store filenames so we know which filenames belongs to a configuration profile */
        g_hash_table_insert (profile_files, g_strdup(filename), g_strdup(filename));
    }

    if (from_profile) {
      path = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s",
                 get_persconffile_dir(persconfprofile), filename);
    } else {
      path = g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s",
                 get_persconffile_dir(NULL), filename);
    }

    return path;
}

/*
 * Construct the path name of a global configuration file, given the
 * file name.
 *
 * The returned file name was g_malloc()'d so it must be g_free()d when the
 * caller is done with it.
 */
char *
get_datafile_path(const char *filename)
{
    if (running_in_build_directory_flag &&
        (!strcmp(filename, "AUTHORS-SHORT") ||
         !strcmp(filename, "hosts"))) {
        /* We're running in the build directory and the requested file is a
         * generated (or a test) file.  Return the file name in the build
         * directory (not in the source/data directory).
         * (Oh the things we do to keep the source directory pristine...)
         */
        return g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", get_progfile_dir(), filename);
    } else {
        return g_strdup_printf("%s" G_DIR_SEPARATOR_S "%s", get_datafile_dir(), filename);
    }
}

/* Get the personal plugin dir */
/* Return value is malloced so the caller should g_free() it. */
char *
get_plugins_pers_dir(void)
{
    return get_persconffile_path(PLUGINS_DIR_NAME, FALSE);
}

/*
 * Return an error message for UNIX-style errno indications on open or
 * create operations.
 */
const char *
file_open_error_message(int err, gboolean for_writing)
{
    const char *errmsg;
    static char errmsg_errno[1024+1];

    switch (err) {

    case ENOENT:
        if (for_writing)
            errmsg = "The path to the file \"%s\" doesn't exist.";
        else
            errmsg = "The file \"%s\" doesn't exist.";
        break;

    case EACCES:
        if (for_writing)
            errmsg = "You don't have permission to create or write to the file \"%s\".";
        else
            errmsg = "You don't have permission to read the file \"%s\".";
        break;

    case EISDIR:
        errmsg = "\"%s\" is a directory (folder), not a file.";
        break;

    case ENOSPC:
        errmsg = "The file \"%s\" could not be created because there is no space left on the file system.";
        break;

#ifdef EDQUOT
    case EDQUOT:
        errmsg = "The file \"%s\" could not be created because you are too close to, or over, your disk quota.";
        break;
#endif

    case EINVAL:
        errmsg = "The file \"%s\" could not be created because an invalid filename was specified.";
        break;

#ifdef ENAMETOOLONG
    case ENAMETOOLONG:
        /* XXX Make sure we truncate on a character boundary. */
        errmsg = "The file name \"%.80s" UTF8_HORIZONTAL_ELLIPSIS "\" is too long.";
        break;
#endif

    case ENOMEM:
        /*
         * The problem probably has nothing to do with how much RAM the
         * user has on their machine, so don't confuse them by saying
         * "memory".  The problem is probably either virtual address
         * space or swap space.
         */
#if GLIB_SIZEOF_VOID_P == 4
        /*
         * ILP32; we probably ran out of virtual address space.
         */
#define ENOMEM_REASON "it can't be handled by a 32-bit application"
#else
        /*
         * LP64 or LLP64; we probably ran out of swap space.
         */
#if defined(_WIN32)
        /*
         * You need to make the pagefile bigger.
         */
#define ENOMEM_REASON "the pagefile is too small"
#elif defined(__APPLE__)
        /*
         * dynamic_pager couldn't, or wouldn't, create more swap files.
         */
#define ENOMEM_REASON "your system ran out of swap file space"
#else
        /*
         * Either you have a fixed swap partition or a fixed swap file,
         * and it needs to be made bigger.
         *
         * This is UN*X, but it's not OS X, so we assume the user is
         * *somewhat* nerdy.
         */
#define ENOMEM_REASON "your system is out of swap space"
#endif
#endif /* GLIB_SIZEOF_VOID_P == 4 */
        if (for_writing)
            errmsg = "The file \"%s\" could not be created because " ENOMEM_REASON ".";
        else
            errmsg = "The file \"%s\" could not be opened because " ENOMEM_REASON ".";
        break;

    default:
        g_snprintf(errmsg_errno, sizeof(errmsg_errno),
               "The file \"%%s\" could not be %s: %s.",
               for_writing ? "created" : "opened",
               g_strerror(err));
        errmsg = errmsg_errno;
        break;
    }
    return errmsg;
}

/*
 * Return an error message for UNIX-style errno indications on write
 * operations.
 */
const char *
file_write_error_message(int err)
{
    const char *errmsg;
    static char errmsg_errno[1024+1];

    switch (err) {

    case ENOSPC:
        errmsg = "The file \"%s\" could not be saved because there is no space left on the file system.";
        break;

#ifdef EDQUOT
    case EDQUOT:
        errmsg = "The file \"%s\" could not be saved because you are too close to, or over, your disk quota.";
        break;
#endif

    default:
        g_snprintf(errmsg_errno, sizeof(errmsg_errno),
               "An error occurred while writing to the file \"%%s\": %s.",
               g_strerror(err));
        errmsg = errmsg_errno;
        break;
    }
    return errmsg;
}


gboolean
file_exists(const char *fname)
{
    ws_statb64 file_stat;

    if (!fname) {
        return FALSE;
    }

#if defined(_MSC_VER) && _MSC_VER < 1900

    /*
     * This is a bit tricky on win32. The st_ino field is documented as:
     * "The inode, and therefore st_ino, has no meaning in the FAT, ..."
     * but it *is* set to zero if stat() returns without an error,
     * so this is working, but maybe not quite the way expected. ULFL
     */
    file_stat.st_ino = 1;   /* this will make things work if an error occurred */
    ws_stat64(fname, &file_stat);
    if (file_stat.st_ino == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
#else
    if (ws_stat64(fname, &file_stat) != 0 && errno == ENOENT) {
        return FALSE;
    } else {
        return TRUE;
    }
#endif
}

/*
 * Check that the from file is not the same as to file
 * We do it here so we catch all cases ...
 * Unfortunately, the file requester gives us an absolute file
 * name and the read file name may be relative (if supplied on
 * the command line), so we can't just compare paths. From Joerg Mayer.
 */
gboolean
files_identical(const char *fname1, const char *fname2)
{
    /* Two different implementations, because:
     *
     * - _fullpath is not available on UN*X, so we can't get full
     *   paths and compare them (which wouldn't work with hard links
     *   in any case);
     *
     * - st_ino isn't filled in with a meaningful value on Windows.
     */
#ifdef _WIN32
    char full1[MAX_PATH], full2[MAX_PATH];

    /*
     * Get the absolute full paths of the file and compare them.
     * That won't work if you have hard links, but those aren't
     * much used on Windows, even though NTFS supports them.
     *
     * XXX - will _fullpath work with UNC?
     */
    if( _fullpath( full1, fname1, MAX_PATH ) == NULL ) {
        return FALSE;
    }

    if( _fullpath( full2, fname2, MAX_PATH ) == NULL ) {
        return FALSE;
    }

    if(strcmp(full1, full2) == 0) {
        return TRUE;
    } else {
        return FALSE;
    }
#else
    ws_statb64 filestat1, filestat2;

    /*
     * Compare st_dev and st_ino.
     */
    if (ws_stat64(fname1, &filestat1) == -1)
        return FALSE;   /* can't get info about the first file */
    if (ws_stat64(fname2, &filestat2) == -1)
        return FALSE;   /* can't get info about the second file */
    return (filestat1.st_dev == filestat2.st_dev &&
        filestat1.st_ino == filestat2.st_ino);
#endif
}

/*
 * Copy a file in binary mode, for those operating systems that care about
 * such things.  This should be OK for all files, even text files, as
 * we'll copy the raw bytes, and we don't look at the bytes as we copy
 * them.
 *
 * Returns TRUE on success, FALSE on failure. If a failure, it also
 * displays a simple dialog window with the error message.
 */
gboolean
copy_file_binary_mode(const char *from_filename, const char *to_filename)
{
    int           from_fd, to_fd, err;
    ssize_t       nread, nwritten;
    guint8        *pd = NULL;

    /* Copy the raw bytes of the file. */
    from_fd = ws_open(from_filename, O_RDONLY | O_BINARY, 0000 /* no creation so don't matter */);
    if (from_fd < 0) {
        report_open_failure(from_filename, errno, FALSE);
        goto done;
    }

    /* Use open() instead of creat() so that we can pass the O_BINARY
       flag, which is relevant on Win32; it appears that "creat()"
       may open the file in text mode, not binary mode, but we want
       to copy the raw bytes of the file, so we need the output file
       to be open in binary mode. */
    to_fd = ws_open(to_filename, O_WRONLY | O_CREAT | O_TRUNC | O_BINARY, 0644);
    if (to_fd < 0) {
        report_open_failure(to_filename, errno, TRUE);
        ws_close(from_fd);
        goto done;
    }

#define FS_READ_SIZE 65536
    pd = (guint8 *)g_malloc(FS_READ_SIZE);
    while ((nread = ws_read(from_fd, pd, FS_READ_SIZE)) > 0) {
        nwritten = ws_write(to_fd, pd, nread);
        if (nwritten < nread) {
            if (nwritten < 0)
                err = errno;
            else
                err = WTAP_ERR_SHORT_WRITE;
            report_write_failure(to_filename, err);
            ws_close(from_fd);
            ws_close(to_fd);
            goto done;
        }
    }
    if (nread < 0) {
        err = errno;
        report_read_failure(from_filename, err);
        ws_close(from_fd);
        ws_close(to_fd);
        goto done;
    }
    ws_close(from_fd);
    if (ws_close(to_fd) < 0) {
        report_write_failure(to_filename, errno);
        goto done;
    }

    g_free(pd);
    pd = NULL;
    return TRUE;

done:
    g_free(pd);
    return FALSE;
}

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
