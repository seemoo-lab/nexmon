project('glib', 'c',
  version : '2.89.4',
  # NOTE: See the policy in docs/meson-version.md before changing the Meson dependency
  meson_version : '>= 1.4.0',
  default_options : [
    'buildtype=debugoptimized',
    'warning_level=3',
    # FIXME: After https://github.com/mesonbuild/meson/issues/13639 is
    # fixed, change this to
    # meson.version().version_compare('>=FIXED_MESON_VERSION') ? 'c_std=gnu11,c11' : 'c_std=gnu11'
    # to avoid a warning on MSVC.
    'c_std=gnu11',
  ]
)

fs = import('fs')

cc = meson.get_compiler('c')
c_standards = {}

foreach std : ['89', '99', '11', '17', '23']
  arg = (cc.get_argument_syntax() == 'msvc' ? '/std:' : '-std=') + 'c' + std
  if cc.has_argument(arg)
    c_standards += { std: arg }
  endif
endforeach

# In the 2.85 cycle, this soft-dependency on C11 will become a hard dependency.
# If this is a problem on your toolchain or platform, please file an issue
# (https://gitlab.gnome.org/GNOME/glib/-/issues) and let us know. We have very
# few metrics on the toolchains which are used to build GLib.
if '11' not in c_standards
  warning('Toolchain support for C11 will become a requirement for building ' +
          'GLib in the next unstable development series (2.85). If this is ' +
          'problematic for you, please file an issue ' +
          '(https://gitlab.gnome.org/GNOME/glib/-/issues) now')
endif

have_cxx = add_languages('cpp', native: false, required: get_option('oss_fuzz').enabled())
if have_cxx
  cxx = meson.get_compiler('cpp')
  cxx_standards = {}

  foreach std : ['98', '03', '11', '14', '17', '20', '23', '2c', 'latest']
    arg = (cxx.get_argument_syntax() == 'msvc' ? '/std:' : '-std=') + 'c++' + std
    if cxx.has_argument(arg)
      cxx_standards += { std: arg }
    endif
  endforeach
endif

cc_can_run = meson.can_run_host_binaries()

if cc.get_argument_syntax() == 'msvc'
  # Ignore several spurious warnings for things glib does very commonly
  # (also for clang-cl)
  add_project_arguments('/FIglib/msvc_recommended_pragmas.h', language : 'c')
endif

if cc.get_id() == 'msvc'
  # Set the input and exec encoding to utf-8, like is the default with GCC
  add_project_arguments(cc.get_supported_arguments(['/utf-8']), language: ['c', 'cpp'])
  # Disable SAFESEH with MSVC for plugins and libs that use external deps that
  # are built with MinGW
  noseh_link_args = ['/SAFESEH:NO']
else
  noseh_link_args = []
  # -mms-bitfields vs -fnative-struct ?
endif

host_system = host_machine.system()
subsystem = ''

linux_libc = ''
if host_system == 'linux'
  musl_test_code = '''#include <stdlib.h>
  #if defined __GLIBC__ || defined __UCLIBC__
  #error "Not in glibc or uclibc"
  #endif'''
  if cc.compiles(musl_test_code, name : 'building for musl libc')
    linux_libc = 'musl'
  endif
elif host_system == 'darwin'
  subsystem = host_machine.subsystem()
elif host_system == 'ios'
  warning('Setting system = \'ios\' in cross files is deprecated, please use subsystem names')
  host_system = 'darwin'
  subsystem = 'ios'
endif

glib_version = meson.project_version()
glib_api_version = '2.0'
version_arr = glib_version.split('.')
major_version = version_arr[0].to_int()
minor_version = version_arr[1].to_int()
micro_version = version_arr[2].to_int()

interface_age = minor_version.is_odd() ? 0 : micro_version
binary_age = 100 * minor_version + micro_version

soversion = 0
# Maintain compatibility with previous libtool versioning
# current = minor * 100 + micro
current = binary_age - interface_age
library_version = '@0@.@1@.@2@'.format(soversion, current, interface_age)
darwin_versions = [current + 1, '@0@.@1@'.format(current + 1, interface_age)]

configinc = include_directories('.')
glibinc = include_directories('glib')
gobjectinc = include_directories('gobject')
gmoduleinc = include_directories('gmodule')
gioinc = include_directories('gio')
girepoinc = include_directories('girepository')

glib_prefix = get_option('prefix')
glib_bindir = join_paths(glib_prefix, get_option('bindir'))
glib_libdir = join_paths(glib_prefix, get_option('libdir'))
glib_libexecdir = join_paths(glib_prefix, get_option('libexecdir'))
glib_datadir = join_paths(glib_prefix, get_option('datadir'))
glib_localedir = join_paths(glib_prefix, get_option('localedir'))
glib_pkgdatadir = join_paths(glib_datadir, 'glib-2.0')
glib_includedir = join_paths(glib_prefix, get_option('includedir'), 'glib-2.0')
if get_option('gio_module_dir') != ''
  glib_giomodulesdir = join_paths(glib_prefix, get_option('gio_module_dir'))
else
  glib_giomodulesdir = join_paths(glib_libdir, 'gio', 'modules')
endif

if get_option('multiarch')
  # For multiarch/multilib distributions, install each architecture's
  # build of executables used in packaging triggers (like gio-querymodules)
  # to an architecture-dependent location, with a compatibility symlink
  # in the PATH.
  multiarch_bindir = get_option('libdir') / 'glib-2.0'
  multiarch_libexecdir = multiarch_bindir
  pkgconfig_multiarch_bindir = '${libdir}/glib-2.0'
else
  # For single-architecture distributions, just install them into the PATH
  # as was traditionally done.
  multiarch_bindir = get_option('bindir')
  multiarch_libexecdir = get_option('libexecdir')
  pkgconfig_multiarch_bindir = '${bindir}'
endif

glib_pkgconfigreldir = join_paths(glib_libdir, 'pkgconfig')

if get_option('charsetalias_dir') != ''
  glib_charsetaliasdir = join_paths(glib_prefix, get_option('charsetalias_dir'))
else
  glib_charsetaliasdir = glib_libdir
endif

glib_localstatedir = glib_prefix / get_option('localstatedir')

if get_option('runtime_dir') != ''
  glib_runstatedir = glib_prefix / get_option('runtime_dir')
else
  # While we’d normally prefix directories like this with, for example,
  # glib_localstatedir, `/run` is a bit different in that it’s for runtime state
  # rather than data files, so it’s typically functionally useless to use a
  # prefixed version. No other processes will be using it. So we default to the
  # unprefixed system `/run` directory.
  glib_runstatedir = '/run'
endif

# When building glib and gobject-introspection with subprojects, gobject-introspection
# requires to know the path of the sources and the build directory for the subproject.
# We provide it here with a variable.
glib_source_dir = meson.current_source_dir()
glib_build_dir = meson.current_build_dir()

installed_tests_metadir = join_paths(glib_datadir, 'installed-tests', meson.project_name())
installed_tests_execdir = join_paths(glib_libexecdir, 'installed-tests', meson.project_name())
installed_tests_enabled = get_option('installed_tests')
installed_tests_template = files('tests/template.test.in')
installed_tests_template_tap = files('tests/template-tap.test.in')

# Don’t build the tests unless we can run them (either natively, in an exe wrapper, or by installing them for later use)
build_tests = get_option('tests') and (meson.can_run_host_binaries() or installed_tests_enabled)

common_test_env = [
  'G_DEBUG=gc-friendly',
  'G_ENABLE_DIAGNOSTIC=1',
  'MALLOC_CHECK_=2',
]

if get_option('werror')
  common_test_env += 'LINT_WARNINGS_ARE_ERRORS=1'
endif

# Note: this may cause the tests output not to be printed when running in
# verbose mode, see https://github.com/mesonbuild/meson/issues/11185
# Can be changed it to 'exitcode' if required during development.
test_protocol = 'tap'
test_timeout = 30
test_timeout_slow = 90

add_test_setup('default',
  is_default: not meson.is_subproject(),
  exclude_suites: ['flaky', 'failing'],
  env: common_test_env,
  timeout_multiplier: 2,
)

add_test_setup('unstable_tests',
  env: common_test_env,
  timeout_multiplier: 2,
  # Empty test setup, used for having different results set for flaky tests
  # Sadly we can't use (https://github.com/mesonbuild/meson/issues/10934):
  #suites: ['flaky', 'unstable']
)

add_test_setup('thorough',
  exclude_suites: ['flaky', 'failing', 'performance'],
  env: common_test_env,
  timeout_multiplier: 20,
  exe_wrapper: [find_program('./.gitlab-ci/thorough-test-wrapper.sh', required: true)],
)

# Allow the tests to be easily run under valgrind using --setup=valgrind
valgrind = find_program('valgrind', required: false)
valgrind_suppression_file = files('tools' / 'glib.supp')[0]
valgrind_suppression_file_install_subdir = 'glib-2.0' / 'valgrind'

if valgrind.found()
  add_test_setup('valgrind',
    exclude_suites: [ 'no-valgrind', 'flaky' ],
    exe_wrapper: [
      valgrind,
      '--tool=memcheck',
      '--error-exitcode=1',
      '--track-origins=yes',
      '--leak-check=full',
      '--leak-resolution=high',
      '--num-callers=50',
      '--show-leak-kinds=definite,possible',
      '--show-error-list=yes',
      '--suppressions=' + valgrind_suppression_file.full_path(),
    ],
    env: common_test_env,
    timeout_multiplier: 20,
  )
  meson.add_devenv({'VALGRIND_OPTS': '--suppressions=' + valgrind_suppression_file.full_path()}, method : 'append')
endif

add_project_arguments('-D_GNU_SOURCE', language: ['c', 'cpp', 'objc', 'objcpp'])

if host_system == 'qnx'
  add_project_arguments('-D_QNX_SOURCE', language: ['c', 'cpp'])
elif host_system == 'sunos'
  add_project_arguments('-D__EXTENSIONS__', language: ['c', 'cpp'])
endif

# dummy/empty dependency() object to declare fallbacks and simpler dependencies
not_found = dependency('', required: false)

get_dll_paths_script = not_found

if host_system == 'windows'
  add_project_arguments(['-DUNICODE', '-D_UNICODE'], language: ['c', 'cpp'])

  # Script to obtain paths where the DLLs that we will reside for g-ir-scanner
  get_dll_paths_script = find_program('tools/grab-gio-dll-paths.py', native: true)
endif

# Disable strict aliasing;
# see https://bugzilla.gnome.org/show_bug.cgi?id=791622
if cc.has_argument('-fno-strict-aliasing')
  add_project_arguments('-fno-strict-aliasing', language: ['c', 'cpp', 'objc', 'objcpp'])
endif

########################
# Configuration begins #
########################
glib_conf = configuration_data()
glibconfig_conf = configuration_data()

# accumulated list of defines as we check for them, so we can easily
# use them later in test programs (autoconf does this automatically)
glib_conf_prefix = ''

glib_conf.set('GLIB_MAJOR_VERSION', major_version)
glib_conf.set('GLIB_MINOR_VERSION', minor_version)
glib_conf.set('GLIB_MICRO_VERSION', micro_version)
glib_conf.set('GLIB_INTERFACE_AGE', interface_age)
glib_conf.set('GLIB_BINARY_AGE', binary_age)
glib_conf.set_quoted('GETTEXT_PACKAGE', 'glib20')
glib_conf.set_quoted('PACKAGE_BUGREPORT', 'https://gitlab.gnome.org/GNOME/glib/issues/new')
glib_conf.set_quoted('PACKAGE_NAME', 'glib')
glib_conf.set_quoted('PACKAGE_STRING', 'glib @0@'.format(meson.project_version()))
glib_conf.set_quoted('PACKAGE_TARNAME', 'glib')
glib_conf.set_quoted('PACKAGE_URL', '')
glib_conf.set_quoted('PACKAGE_VERSION', meson.project_version())
glib_conf.set('ENABLE_NLS', 1)

# used by the .rc.in files
glibconfig_conf.set('LT_CURRENT_MINUS_AGE', soversion)

process_spawn_allowed = true
if subsystem.startswith('tvos') or subsystem.startswith('watchos')
  process_spawn_allowed = false
endif

glib_conf.set('_GNU_SOURCE', 1)

if host_system in ['windows', 'darwin']
  # Poll doesn't work on devices on Windows, and macOS's poll() implementation is known to be broken
  glib_conf.set('BROKEN_POLL', true)
endif

if host_system == 'windows' and cc.get_id() != 'msvc' and cc.get_id() != 'clang-cl'
  # FIXME: Ideally we shouldn't depend on this on Windows and should use
  # 64 bit capable Windows API that also works with MSVC.
  # The autotools build did set this for mingw and while meson sets it
  # for gcc/clang by default, it doesn't do so on Windows.
  glib_conf.set('_FILE_OFFSET_BITS', 64)
endif

glib_build_shared = false
glib_build_static = false
if get_option('default_library') == 'both'
  glib_build_static = true
  glib_build_shared = true
elif get_option('default_library') == 'static'
  glib_build_static = true
elif get_option('default_library') == 'shared'
  glib_build_shared = true
endif

glib_build_both = glib_build_static and glib_build_shared
glib_build_static_only = glib_build_static and not glib_build_shared
glib_build_shared_only = glib_build_shared and not glib_build_static

if glib_build_shared and glib_build_static and (
  host_system == 'windows' or host_system == 'cygwin')
  error('On Windows default_library must be "shared" or "static" but not "both"')
endif

if glib_build_static_only
  glibconfig_conf.set('GLIB_STATIC_COMPILATION', '1')
  glibconfig_conf.set('GOBJECT_STATIC_COMPILATION', '1')
  glibconfig_conf.set('GIO_STATIC_COMPILATION', '1')
  glibconfig_conf.set('GMODULE_STATIC_COMPILATION', '1')
  glibconfig_conf.set('GI_STATIC_COMPILATION', '1')
endif

# Cygwin glib port maintainers made it clear
# (via the patches they apply) that they want no
# part of glib W32 code, therefore we do not define
# G_PLATFORM_WIN32 for host_system == 'cygwin'.
# This makes G_PLATFORM_WIN32 a synonym for
# G_OS_WIN32.
if host_system == 'windows'
  glib_os = '''#define G_OS_WIN32
#define G_PLATFORM_WIN32'''
elif host_system == 'cygwin'
  glib_os = '''#define G_OS_UNIX
#define G_WITH_CYGWIN'''
else
  glib_os = '#define G_OS_UNIX'
endif
glibconfig_conf.set('glib_os', glib_os)

# We need to know the CRT being used to determine what .lib files we need on
# Visual Studio for dependencies that don't normally come with pkg-config files
vs_crt = 'release'
vs_crt_opt = get_option('b_vscrt')
if vs_crt_opt in ['mdd', 'mtd']
  vs_crt = 'debug'
elif vs_crt_opt == 'from_buildtype'
  if get_option('buildtype') == 'debug'
    vs_crt = 'debug'
  endif
endif

# Use debug/optimization flags to determine whether to enable debug or disable
# cast checks. We have a non-production (debug) build if debug is true and if
# optimization is 0 or g; otherwise, we have a production build.
glib_debug_cflags = []
glib_debug = get_option('glib_debug')
if (glib_debug.enabled() or (
    glib_debug.auto() and get_option('debug') and get_option('optimization') in [ '0', 'g' ]))
  glib_debug_cflags += ['-DG_ENABLE_DEBUG']
  message('Enabling various debug infrastructure')
else
  glib_debug_cflags += ['-DG_DISABLE_CAST_CHECKS']
  message('Disabling cast checks')
endif

if not get_option('glib_assert')
  glib_debug_cflags += ['-DG_DISABLE_ASSERT']
  message('Disabling GLib asserts')
endif

if not get_option('glib_checks')
  glib_debug_cflags += ['-DG_DISABLE_CHECKS']
  message('Disabling GLib checks')
endif

add_project_arguments(glib_debug_cflags, language: ['c', 'cpp', 'objc', 'objcpp'])

# check for header files

headers = [
  'alloca.h',
  'afunix.h',
  'crt_externs.h',
  'dirent.h', # MSC does not come with this by default
  'float.h',
  'fstab.h',
  'ftw.h',
  'grp.h',
  'intsafe.h',
  'inttypes.h',
  'libproc.h',
  'limits.h',
  'linux/netlink.h',
  'locale.h',
  'mach/mach_time.h',
  'memory.h',
  'mntent.h',
  'netlink/netlink.h',
  'netlink/netlink_route.h',
  'poll.h',
  'pwd.h',
  'sched.h',
  'spawn.h',
  'stdatomic.h',
  'stdckdint.h',
  'stdint.h',
  'stdlib.h',
  'string.h',
  'strings.h',
  'sys/auxv.h',
  'sys/event.h',
  'sys/uio.h',
  'sys/filio.h',
  'sys/inotify.h',
  'sys/mkdev.h',
  'sys/mntctl.h',
  'sys/mnttab.h',
  'sys/mount.h',
  'sys/param.h',
  'sys/prctl.h',
  'sys/resource.h',
  'sys/select.h',
  'sys/statfs.h',
  'sys/stat.h',
  'sys/statvfs.h',
  'sys/sysctl.h',
  'sys/time.h', # MSC does not come with this by default
  'sys/times.h',
  'sys/types.h',
  'sys/ucred.h',
  'sys/uio.h',
  'sys/vfs.h',
  'sys/vfstab.h',
  'sys/vmount.h',
  'sys/wait.h',
  'syslog.h',
  'termios.h',
  'unistd.h',
  'values.h',
  'vcruntime.h',
  'wchar.h',
  'xlocale.h',
]

foreach h : headers
  if cc.has_header(h)
    define = 'HAVE_' + h.underscorify().to_upper()
    glib_conf.set(define, 1)
    glib_conf_prefix = glib_conf_prefix + '#define @0@ 1\n'.format(define)
  endif
endforeach

# FreeBSD includes a malloc.h which always throws compilation error, so we have
# to use check_header() rather than has_header().
if cc.check_header('malloc.h')
  glib_conf.set('HAVE_MALLOC_H', 1)
  glib_conf_prefix = glib_conf_prefix + '#define HAVE_MALLOC_H 1\n'
endif

if (glib_conf.has('HAVE_LINUX_NETLINK_H') or
    (glib_conf.has('HAVE_NETLINK_NETLINK_H') and glib_conf.has('HAVE_NETLINK_NETLINK_ROUTE_H')))
  glib_conf.set('HAVE_NETLINK', 1)
endif

# Is statx() supported? Android systems don’t reliably support it as of August 2020.
statx_code = '''
  #ifndef _GNU_SOURCE
  #define _GNU_SOURCE
  #endif
  #include <sys/stat.h>
  #include <fcntl.h>
  int main (void)
  {
    struct statx stat_buf;
    return statx (AT_FDCWD, "/", AT_SYMLINK_NOFOLLOW, STATX_BASIC_STATS | STATX_BTIME, &stat_buf);
  }
  '''
if host_system != 'android' and cc.compiles(statx_code, name : 'statx() test')
  glib_conf.set('HAVE_STATX', 1)
endif

if glib_conf.has('HAVE_LOCALE_H')
  if cc.has_header_symbol('locale.h', 'LC_MESSAGES')
    glib_conf.set('HAVE_LC_MESSAGES', 1)
  endif
endif

struct_stat_blkprefix = '''
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_STATFS_H
#include <sys/statfs.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif
'''

struct_members = [
  [ 'stat', 'st_mtimensec' ],
  [ 'stat', 'st_mtim.tv_nsec' ],
  [ 'stat', 'st_atimensec' ],
  [ 'stat', 'st_atim.tv_nsec' ],
  [ 'stat', 'st_ctimensec' ],
  [ 'stat', 'st_ctim.tv_nsec' ],
  [ 'stat', 'st_birthtime' ],
  [ 'stat', 'st_birthtimensec' ],
  [ 'stat', 'st_birthtim' ],
  [ 'stat', 'st_birthtim.tv_nsec' ],
  [ 'stat', 'st_blksize', struct_stat_blkprefix ],
  [ 'stat', 'st_blocks', struct_stat_blkprefix ],
  [ 'statfs', 'f_fstypename', struct_stat_blkprefix ],
  [ 'statfs', 'f_bavail', struct_stat_blkprefix ],
  [ 'dirent', 'd_type', '''#include <sys/types.h>
                           #include <dirent.h>''' ],
  [ 'statvfs', 'f_basetype', '#include <sys/statvfs.h>' ],
  [ 'statvfs', 'f_fstypename', '#include <sys/statvfs.h>' ],
  [ 'statvfs', 'f_type', '#include <sys/statvfs.h>' ],
  [ 'tm', 'tm_gmtoff', '#include <time.h>' ],
  [ 'tm', '__tm_gmtoff', '#include <time.h>' ],
]

foreach m : struct_members
  header_check_prefix = glib_conf_prefix
  if m.length() == 3
    header_check_prefix = header_check_prefix + m[2]
  else
    header_check_prefix = header_check_prefix + '#include <sys/stat.h>'
  endif
  # Reimplement cc.has_member() to workaround compiler warning
  # FIXME: https://github.com/mesonbuild/meson/pull/12818
  code = header_check_prefix + '''
    void bar(void) {
        struct ''' + m[0] + ''' foo;
        (void) ( foo.''' + m[1] + ''' );
        (void) foo;
    }
    '''
  if cc.compiles(code, name : 'type "struct ' + m[0] + '" has member "' + m[1] + '"')
    define = 'HAVE_STRUCT_@0@_@1@'.format(m[0].to_upper(), m[1].underscorify().to_upper())
    glib_conf.set(define, 1)
    glib_conf_prefix = glib_conf_prefix + '#define @0@ 1\n'.format(define)
  else
  endif
endforeach

# Compiler flags
if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  warning_common_args = [
    '-Wduplicated-branches',
    '-Wfloat-conversion',
    '-Wimplicit-fallthrough',
    '-Wmisleading-indentation',
    '-Wmissing-field-initializers',
    '-Wnonnull',
    '-Wnull-dereference',
    '-Wunused',
    # Due to maintained deprecated code, we do not want to see unused parameters
    '-Wno-unused-parameter',
    # Due to pervasive use of things like GPOINTER_TO_UINT(), we do not support
    # building with -Wbad-function-cast.
    '-Wno-cast-function-type',
    # Due to function casts through (void*) we cannot support -Wpedantic:
    # ./docs/toolchain-requirements.md#Function_pointer_conversions.
    '-Wno-pedantic',
    # A zero-length format string shouldn't be considered an issue.
    '-Wno-format-zero-length',
    # We explicitly require variadic macros
    '-Wno-variadic-macros',
    '-Werror=format=2',
    '-Werror=init-self',
    '-Werror=missing-include-dirs',
    '-Werror=pointer-arith',
    '-Werror=unused-result',
  ]

  warning_c_args = warning_common_args + [
    '-Wstrict-prototypes',
    # Due to pervasive use of things like GPOINTER_TO_UINT(), we do not support
    # building with -Wbad-function-cast.
    '-Wno-bad-function-cast',
    '-Werror=implicit-function-declaration',
    '-Werror=missing-prototypes',
    '-Werror=pointer-sign',
    '-Wno-string-plus-int',
  ]
  warning_cxx_args = warning_common_args
  warning_c_link_args = [
    '-Wl,-z,nodelete',
  ]
  if get_option('bsymbolic_functions')
    warning_c_link_args += ['-Wl,-Bsymbolic-functions']
  endif
elif cc.get_id() == 'msvc'
  warning_c_args = [
    # If a warning is completely useless and spammy, use '/wdXXXX' to suppress it
    # If a warning is harmless but hard to fix, use '/woXXXX' so it's shown once
    # NOTE: Only add warnings here if you are sure they're spurious
    '/wo4057', # 'operator': 'identifier1' differs in indirection to slightly different base types from 'identifier2'
    '/wd4068', # unknown pragma
    '/wo4090', # 'operation': different 'modifier' qualifiers
    '/wd4100', # 'identifier': unreferenced formal parameter
    '/wd4116', # unnamed type definition in parentheses
    '/wo4125', # decimal digit terminates octal escape sequence
    '/wd4127', # conditional expression is constant
    '/wd4146', # unary minus operator applied to unsigned type, result still unsigned
    '/wd4152', # nonstandard extension, function/data pointer conversion in expression
    '/wd4201', # nonstandard extension used: nameless struct/union
    '/wd4232', # nonstandard extension used: 'identifier': address of dllimport 'dllimport' is not static, identity not guaranteed
    '/wo4245', # 'conversion_type': conversion from 'type1' to 'type2', signed/unsigned mismatch
    '/wo4267', # 'variable': conversion from 'size_t' to 'type', possible loss of data
    '/wd4334', # 'shift_operator': result of 32-bit shift implicitly converted to 64 bits (was 64-bit shift intended?)
    '/wo4389', # 'operator': signed/unsigned mismatch
    '/wo4702', # unreachable code
    '/wd4706', # assignment within conditional expression
  ]

  warning_cxx_args = []
  warning_c_link_args = []
else
  warning_c_args = []
  warning_cxx_args = []
  warning_c_link_args = []
endif

add_project_arguments(cc.get_supported_arguments(warning_c_args), language: ['c', 'objc'])
if have_cxx
  add_project_arguments(cxx.get_supported_arguments(warning_cxx_args), language: ['cpp', 'objcpp'])
endif

# FIXME: This is checked and implemented separately from warning_common_args for
# now because it causes so many warnings. Adding it as a variable in various
# directories allows us to gradually tighten up support for it. Eventually, the
# warning can be enabled unconditionally in warning_c_args, and this variable
# removed. See https://gitlab.gnome.org/GNOME/glib/-/issues/3405
warning_sign_conversion_args = cc.get_supported_arguments(['-Wsign-conversion'])

# FIXME: As above
# See https://gitlab.gnome.org/GNOME/glib/-/issues/3527
warning_shorten_64_to_32_args = cc.get_supported_arguments(['-Wshorten-64-to-32'])

# FIXME: We cannot build some of the GResource tests with -z nodelete, which
# means we cannot use that flag in add_project_link_arguments(), and must add
# it to the relevant targets manually. We do the same with -Bsymbolic-functions
# because that is what the autotools build did.
# See https://github.com/mesonbuild/meson/pull/3520 for a way to eventually
# improve this.
glib_link_flags = cc.get_supported_link_arguments(warning_c_link_args)

glib_sanitizers = get_option('b_sanitize').split(',')
if glib_sanitizers == ['none']
  glib_sanitizers = []
endif

# Windows SDK requirements and checks
if host_system == 'windows'
  # Check whether we're building for UWP apps
  code = '''
  #include <windows.h>
  #if !(WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_APP) && !WINAPI_FAMILY_PARTITION(WINAPI_PARTITION_DESKTOP))
  #error "Not building for UWP"
  #endif'''
  if cc.compiles(code, name : 'building for UWP')
    glib_conf.set('G_WINAPI_ONLY_APP', true)
    # We require Windows 10+ on WinRT
    glib_conf.set('_WIN32_WINNT', '0x0A00')
    uwp_gio_deps = [cc.find_library('shcore'),
                    cc.find_library('runtimeobject')]
  else
    # We require Windows 7+ on Win32
    glib_conf.set('_WIN32_WINNT', '0x0601')
    uwp_gio_deps = []
  endif
endif

functions = [
  'accept4',
  'close_range',
  'copy_file_range',
  'endmntent',
  'endservent',
  'epoll_create1',
  'faccessat',
  'fallocate',
  'fchmod',
  'fchown',
  'fdwalk',
  'free_aligned_sized',
  'free_sized',
  'fsync',
  'ftruncate64',
  'getauxval',
  'getc_unlocked',
  'getfsent',
  'getfsstat',
  'getgrgid_r',
  'getifaddrs',
  'getmntent_r',
  'getpwnam_r',
  'getpwuid_r',
  'getresuid',
  'getvfsstat',
  'gmtime_r',
  'hasmntopt',
  'inotify_init1',
  'issetugid',
  'kevent',
  'kqueue',
  'lchmod',
  'lchown',
  'link',
  'localtime_r',
  'lstat',
  'mbrtowc',
  'memalign',
  'memmem',
  'mmap',
  'newlocale',
  'pipe2',
  'poll',
  'prlimit',
  'readlink',
  'recvmmsg',
  'sendmmsg',
  'setenv',
  'setmntent',
  'strerror_r',
  'strnlen',
  'strsignal',
  'strtod_l',
  'strtoll_l',
  'strtoull_l',
  'symlink',
  'timegm',
  'unsetenv',
  'uselocale',
  'utimes',
  'utimensat',
  'valloc',
  'vasprintf',
  'vsnprintf',
  'wcrtomb',
  'wcslen',
  'wcsnlen',
  'sysctlbyname',
]

# _NSGetEnviron is available on iOS too, but its usage gets apps rejected from
# the app store since it's considered 'private API'
if host_system == 'darwin'
  functions += ['_NSGetEnviron']
endif

if glib_conf.has('HAVE_SYS_STATVFS_H')
  functions += ['statvfs']
else
  have_func_statvfs = false
endif
if glib_conf.has('HAVE_SYS_STATFS_H') or glib_conf.has('HAVE_SYS_MOUNT_H')
  functions += ['statfs']
else
  have_func_statfs = false
endif
if glib_conf.has('HAVE_SYS_PRCTL_H')
  functions += ['prctl']
else
  have_func_prctl = false
endif

if host_system == 'windows'
  iphlpapi_dep = cc.find_library('iphlpapi')
  iphlpapi_funcs = ['if_nametoindex', 'if_indextoname']
  foreach ifunc : iphlpapi_funcs
    iphl_prefix =  '''#define _WIN32_WINNT @0@
      #include <winsock2.h>
      #include <iphlpapi.h>'''.format(glib_conf.get('_WIN32_WINNT'))
    if cc.has_function(ifunc,
                       prefix : iphl_prefix,
                       dependencies : iphlpapi_dep)
      idefine = 'HAVE_' + ifunc.underscorify().to_upper()
      glib_conf.set(idefine, 1)
      glib_conf_prefix = glib_conf_prefix + '#define @0@ 1\n'.format(idefine)
      set_variable('have_func_' + ifunc, true)
    else
      set_variable('have_func_' + ifunc, false)
    endif
  endforeach
else
  functions += ['if_indextoname', 'if_nametoindex']
endif

# AIX splice is something else
if host_system != 'aix'
  functions += ['splice']
endif

# SVR4 sysinfo() does not behave the same as sysinfo() on other Unixes
if host_system != 'sunos'
  functions += ['sysinfo']
endif

foreach f : functions
  if cc.has_function(f)
    define = 'HAVE_' + f.underscorify().to_upper()
    glib_conf.set(define, 1)
    glib_conf_prefix = glib_conf_prefix + '#define @0@ 1\n'.format(define)
    set_variable('have_func_' + f, true)
  else
    set_variable('have_func_' + f, false)
  endif
endforeach

# Export the information about free_sized() so we can correctly define a macro
# wrapper around g_free()/g_free_sized() depending on whether it’s available
glibconfig_conf.set('G_HAVE_FREE_SIZED', have_func_free_sized)

# Check that stpcpy() is usable; must use header.
# See:
# https://github.com/mesonbuild/meson/issues/5628.
if cc.has_function('stpcpy', prefix : '#include <string.h>')
  glib_conf.set('HAVE_STPCPY', 1)
endif

if cc.has_function('memalign', prefix: '#include <stdlib.h>\n#include <malloc.h>')
  glib_conf.set('HAVE_MEMALIGN', 1)
endif

# For example on Openbsd, getservbyname_r() has a different signature.
# https://man.openbsd.org/getservbyname.3
if cc.compiles('''#include <netdb.h>
               int main (int argc, char ** argv) {
                 int (*fcn)(const char *,
                            const char *,
                            struct servent *,
                            char *,
                            size_t,
                            struct servent **) = getservbyname_r;
                 (void) fcn;
                 return 0;
               }''',
               name : 'getservbyname_r()',
               args: '-Werror=incompatible-pointer-types')
  glib_conf.set('HAVE_GETSERVBYNAME_R', 1)
endif

if cc.has_function('_aligned_malloc', prefix: '#include <malloc.h>')
  glib_conf.set('HAVE__ALIGNED_MALLOC', 1)
endif

if host_system != 'windows' and cc.has_function('aligned_alloc', prefix: '#include <stdlib.h>')
  glib_conf.set('HAVE_ALIGNED_ALLOC', 1)
endif

if host_system != 'windows' and cc.has_function('posix_memalign', prefix: '#include <stdlib.h>')
  glib_conf.set('HAVE_POSIX_MEMALIGN', 1)
endif

# Check that posix_spawn() is usable; must use header
if cc.has_function('posix_spawn', prefix : '#include <spawn.h>')
  glib_conf.set('HAVE_POSIX_SPAWN', 1)
endif

# Check whether strerror_r returns char *
if have_func_strerror_r
  if cc.compiles('''#define _GNU_SOURCE
                    #include <string.h>
                    int func (void) {
                      char error_string[256];
                      char *ptr = strerror_r (-2, error_string, 256);
                      char c = *strerror_r (-2, error_string, 256);
                      return c != 0 && ptr != (void*) 0L;
                    }
                 ''',
                 name : 'strerror_r() returns char *')
    glib_conf.set('STRERROR_R_CHAR_P', 1,
                  description: 'Defined if strerror_r returns char *')
  endif
endif

# Special-case these functions that have alternative names on Windows/MSVC
if cc.has_function('snprintf') or cc.has_header_symbol('stdio.h', 'snprintf')
  glib_conf.set('HAVE_SNPRINTF', 1)
  glib_conf_prefix = glib_conf_prefix + '#define HAVE_SNPRINTF 1\n'
elif cc.has_function('_snprintf') or cc.has_header_symbol('stdio.h', '_snprintf')
  hack_define = '1\n#define snprintf _snprintf'
  glib_conf.set('HAVE_SNPRINTF', hack_define)
  glib_conf_prefix = glib_conf_prefix + '#define HAVE_SNPRINTF ' + hack_define
endif

if cc.has_function('strcasecmp', prefix: '#include <strings.h>')
  glib_conf.set('HAVE_STRCASECMP', 1)
  glib_conf_prefix = glib_conf_prefix + '#define HAVE_STRCASECMP 1\n'
elif cc.has_function('_stricmp')
  hack_define = '1\n#define strcasecmp _stricmp'
  glib_conf.set('HAVE_STRCASECMP', hack_define)
  glib_conf_prefix = glib_conf_prefix + '#define HAVE_STRCASECMP ' + hack_define
endif

if cc.has_function('strncasecmp', prefix: '#include <strings.h>')
  glib_conf.set('HAVE_STRNCASECMP', 1)
  glib_conf_prefix = glib_conf_prefix + '#define HAVE_STRNCASECMP 1\n'
elif cc.has_function('_strnicmp')
  hack_define = '1\n#define strncasecmp _strnicmp'
  glib_conf.set('HAVE_STRNCASECMP', hack_define)
  glib_conf_prefix = glib_conf_prefix + '#define HAVE_STRNCASECMP ' + hack_define
endif

if cc.has_header_symbol('sys/sysmacros.h', 'major')
  glib_conf.set('MAJOR_IN_SYSMACROS', 1)
elif cc.has_header_symbol('sys/mkdev.h', 'major')
  glib_conf.set('MAJOR_IN_MKDEV', 1)
elif cc.has_header_symbol('sys/types.h', 'major')
  glib_conf.set('MAJOR_IN_TYPES', 1)
endif

if cc.has_header_symbol('dlfcn.h', 'RTLD_LAZY')
  glib_conf.set('HAVE_RTLD_LAZY', 1)
endif

if cc.has_header_symbol('dlfcn.h', 'RTLD_NOW')
  glib_conf.set('HAVE_RTLD_NOW', 1)
endif

if cc.has_header_symbol('dlfcn.h', 'RTLD_GLOBAL')
  glib_conf.set('HAVE_RTLD_GLOBAL', 1)
endif

have_rtld_next = false
if cc.has_header_symbol('dlfcn.h', 'RTLD_NEXT', args: '-D_GNU_SOURCE')
  have_rtld_next = true
  glib_conf.set('HAVE_RTLD_NEXT', 1)
endif

if cc.has_type('loff_t', prefix: '#include <sys/types.h>')
  glib_conf.set('HAVE_LOFF_T', 1)
endif

if cc.has_function('unshare', prefix: '#include <sched.h>', args: '-D_GNU_SOURCE')
  glib_conf.set('HAVE_UNSHARE', 1)
endif

# Check whether to use statfs or statvfs
# Some systems have both statfs and statvfs, pick the most "native" for these
if have_func_statfs and have_func_statvfs
  # on solaris and irix, statfs doesn't even have the f_bavail field
  if not glib_conf.has('HAVE_STRUCT_STATFS_F_BAVAIL')
    have_func_statfs = false
  else
    # at least on linux, statfs is the actual syscall
    have_func_statvfs = false
  endif
endif
if have_func_statfs
  glib_conf.set('USE_STATFS', 1)
  stat_func_to_use = 'statfs'
elif have_func_statvfs
  glib_conf.set('USE_STATVFS', 1)
  stat_func_to_use = 'statvfs'
else
  stat_func_to_use = 'neither'
endif
message('Checking whether to use statfs or statvfs .. ' + stat_func_to_use)

if host_system == 'linux'
  if cc.has_function('mkostemp',
                     prefix: '''#define _GNU_SOURCE
                                #include <stdlib.h>''')
    glib_conf.set('HAVE_MKOSTEMP', 1)
  endif
endif

osx_ldflags = []
glib_have_carbon = false
glib_have_cocoa = false
if host_system == 'darwin' and subsystem == 'macos'
  add_languages('objc', native: false, required: true)
  objcc = meson.get_compiler('objc')
  if not objcc.compiles('''#include <AvailabilityMacros.h>
                           #if MAC_OS_X_VERSION_MIN_REQUIRED < 101300
                           #error message "GLib requires macOS 10.13 or later."
                           #endif''',
                         name : 'macOS 10.13 or later')
    error('GLib requires macOS 10.13 or later.')
  endif

  # Mac OS X Carbon support
  glib_have_carbon = objcc.compiles('''#include <Carbon/Carbon.h>
                                       #include <CoreServices/CoreServices.h>''',
                                    name : 'Mac OS X Carbon support')

  if glib_have_carbon
    glib_conf.set('HAVE_CARBON', true)
  endif

  # Mac OS X Cocoa support
  glib_have_cocoa = objcc.compiles('''#include <Cocoa/Cocoa.h>
                                      #ifdef GNUSTEP_BASE_VERSION
                                      #error "Detected GNUstep, not Cocoa"
                                      #endif''',
                                   name : 'Mac OS X Cocoa support')

  if glib_have_cocoa
    glib_conf.set('HAVE_COCOA', true)
    osx_ldflags += ['-Wl,-framework,Foundation', '-Wl,-framework,AppKit']
  endif
endif

if host_system == 'qnx'
  glib_conf.set('HAVE_QNX', 1)
endif

# Check for futex(2)
if cc.compiles('''#include <linux/futex.h>
               #include <sys/syscall.h>
               #include <unistd.h>
               int main (int argc, char ** argv) {
                 syscall (__NR_futex, NULL, FUTEX_WAKE, FUTEX_WAIT);
                 return 0;
               }''', name : 'futex(2) system call')
  glib_conf.set('HAVE_FUTEX', 1)
endif
if cc.compiles('''#include <linux/futex.h>
               #include <sys/syscall.h>
               #include <unistd.h>
               int main (int argc, char ** argv) {
                 syscall (__NR_futex_time64, NULL, FUTEX_WAKE, FUTEX_WAIT);
                 return 0;
               }''', name : 'futex_time64(2) system call')
  glib_conf.set('HAVE_FUTEX_TIME64', 1)
endif

# Check for eventfd(2)
if cc.links('''#include <sys/eventfd.h>
               #include <unistd.h>
               int main (int argc, char ** argv) {
                 eventfd (0, EFD_CLOEXEC);
                 return 0;
               }''', name : 'eventfd(2) system call')
  glib_conf.set('HAVE_EVENTFD', 1)
endif

# Check for ppoll(2)
if cc.links('''#define _GNU_SOURCE
               #include <poll.h>
               #include <stddef.h>
               int main (int argc, char ** argv) {
                 struct pollfd fds[1] = {{0}};
                 struct timespec ts = {0};
                 ppoll (fds, 1, NULL, NULL);
                 return 0;
               }''', name : 'ppoll(2) system call')
  glib_conf.set('HAVE_PPOLL', 1)
endif

# Check for pidfd_open(2)
if cc.links('''#include <sys/syscall.h>
               #include <sys/wait.h>
               #include <linux/wait.h>
               #include <unistd.h>
               int main (int argc, char ** argv) {
                 siginfo_t child_info = { 0, };
                 syscall (SYS_pidfd_open, 0, 0);
                 waitid (P_PIDFD, 0, &child_info, WEXITED | WNOHANG);
                 return 0;
               }''', name : 'pidfd_open(2) system call')
  glib_conf.set('HAVE_PIDFD', 1)
endif

# Check for __uint128_t (gcc) by checking for 128-bit division
uint128_t_src = '''int main() {
static __uint128_t v1 = 100;
static __uint128_t v2 = 10;
static __uint128_t u;
u = v1 / v2;
(void) u;
}'''
if cc.compiles(uint128_t_src, name : '__uint128_t available')
  glib_conf.set('HAVE_UINT128_T', 1)
endif

clock_gettime_test_code = '''
  #include <time.h>
  struct timespec t;
  int main (int argc, char ** argv) {
    return clock_gettime(CLOCK_REALTIME, &t);
  }'''
librt = []
if cc.links(clock_gettime_test_code, name : 'clock_gettime')
  glib_conf.set('HAVE_CLOCK_GETTIME', 1)
elif cc.links(clock_gettime_test_code, args : '-lrt', name : 'clock_gettime in librt')
  glib_conf.set('HAVE_CLOCK_GETTIME', 1)
  librt = cc.find_library('rt')
endif

if cc.has_function('clock_nanosleep', prefix : '#include <time.h>')
  glib_conf.set('HAVE_CLOCK_NANOSLEEP', 1)
endif

dlopen_dlsym_test_code = '''
#include <dlfcn.h>
#include <stdio.h>
int r;
int glib_underscore_test (void) { return 42; }
int main (int argc, char ** argv) {
  void *f1 = (void*)0, *f2 = (void*)0, *handle;
  handle = dlopen ((void*)0, 0);
  if (handle) {
    f1 = dlsym (handle, "glib_underscore_test");
    f2 = dlsym (handle, "_glib_underscore_test");
  }
  r = (!f2 || f1) ? puts ("1") : puts ("0");
  return r > 0 ? 0 : r;
}'''
libdl_dep = []
if cc.links(dlopen_dlsym_test_code, name : 'dlopen() and dlsym() in system libraries')
  have_dlopen_dlsym = true
elif cc.links(dlopen_dlsym_test_code, args : '-ldl', name : 'dlopen() and dlsym() in libdl')
  have_dlopen_dlsym = true
  libdl_dep = cc.find_library('dl')
else
  have_dlopen_dlsym = false
endif

# if statfs() takes 2 arguments (Posix) or 4 (Solaris)
if have_func_statfs
  if cc.compiles(glib_conf_prefix + '''
                 #include <unistd.h>
                        #ifdef HAVE_SYS_PARAM_H
                        #include <sys/param.h>
                        #endif
                        #ifdef HAVE_SYS_VFS_H
                        #include <sys/vfs.h>
                        #endif
                        #ifdef HAVE_SYS_MOUNT_H
                        #include <sys/mount.h>
                        #endif
                        #ifdef HAVE_SYS_STATFS_H
                        #include <sys/statfs.h>
                        #endif
                        void some_func (void) {
                          struct statfs st;
                          statfs("/", &st);
                        }''', name : 'number of arguments to statfs() (n=2)')
    glib_conf.set('STATFS_ARGS', 2)
  elif cc.compiles(glib_conf_prefix + '''
                   #include <unistd.h>
                          #ifdef HAVE_SYS_PARAM_H
                          #include <sys/param.h>
                          #endif
                          #ifdef HAVE_SYS_VFS_H
                          #include <sys/vfs.h>
                          #endif
                          #ifdef HAVE_SYS_MOUNT_H
                          #include <sys/mount.h>
                          #endif
                          #ifdef HAVE_SYS_STATFS_H
                          #include <sys/statfs.h>
                          #endif
                          void some_func (void) {
                            struct statfs st;
                            statfs("/", &st, sizeof (st), 0);
                          }''', name : 'number of arguments to statfs() (n=4)')
    glib_conf.set('STATFS_ARGS', 4)
  else
    error('Unable to determine number of arguments to statfs()')
  endif
endif

# open takes O_DIRECTORY as an option
#AC_MSG_CHECKING([])
if cc.compiles('''#include <fcntl.h>
                  #include <sys/types.h>
                  #include <sys/stat.h>
                  void some_func (void) {
                    open(".", O_DIRECTORY, 0);
                  }''', name : 'open() option O_DIRECTORY')
  glib_conf.set('HAVE_OPEN_O_DIRECTORY', 1)
endif

# fcntl takes F_FULLFSYNC as an option
# See https://developer.apple.com/library/archive/documentation/System/Conceptual/ManPages_iPhoneOS/man2/fsync.2.html
if cc.compiles('''#include <fcntl.h>
                  #include <sys/types.h>
                  #include <sys/stat.h>
                  void some_func (void) {
                    fcntl(0, F_FULLFSYNC, 0);
                  }''', name : 'fcntl() option F_FULLFSYNC')
  glib_conf.set('HAVE_FCNTL_F_FULLFSYNC', 1)
endif

# Check whether there is a vsnprintf() function with C99 semantics installed.
# (similar tests to AC_FUNC_VSNPRINTF_C99)
# Check whether there is a snprintf() function with C99 semantics installed.
# (similar tests to AC_FUNC_SNPRINTF_C99)
# Check whether there is a printf() function with Unix98 semantics installed.
# (similar tests to AC_FUNC_PRINTF_UNIX98)
have_good_vsnprintf = false
have_good_snprintf = false
have_good_printf = false

if host_system == 'windows' and (cc.get_id() == 'msvc' or cc.get_id() == 'clang-cl')
  # Unfortunately the Visual Studio 2015+ implementations of C99-style
  # snprintf and vsnprintf don't seem to be quite good enough.
  # (Sorry, I don't know exactly what is the problem,
  # but it is related to floating point formatting and decimal point vs. comma.)
  # The simple tests in AC_FUNC_VSNPRINTF_C99 and AC_FUNC_SNPRINTF_C99 aren't
  # rigorous enough to notice, though.
  glib_conf.set('HAVE_C99_SNPRINTF', false)
  glib_conf.set('HAVE_C99_VSNPRINTF', false)
  glib_conf.set('HAVE_UNIX98_PRINTF', false)
elif not cc_can_run and host_system == 'darwin'
  # All these are true when compiling natively on macOS, so we should use good
  # defaults when building for iOS and tvOS.
  glib_conf.set('HAVE_C99_SNPRINTF', true)
  glib_conf.set('HAVE_C99_VSNPRINTF', true)
  glib_conf.set('HAVE_UNIX98_PRINTF', true)
  have_good_vsnprintf = true
  have_good_snprintf = true
  have_good_printf = true
else
  vsnprintf_c99_test_code = '''
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

int
doit(char * s, ...)
{
  char buffer[32];
  va_list args;
  int r;

  va_start(args, s);
  r = vsnprintf(buffer, 5, s, args);
  va_end(args);

  if (r != 7)
    exit(1);

  /* AIX 5.1 and Solaris seems to have a half-baked vsnprintf()
     implementation. The above will return 7 but if you replace
     the size of the buffer with 0, it borks! */
  va_start(args, s);
  r = vsnprintf(buffer, 0, s, args);
  va_end(args);

  if (r != 7)
    exit(1);

  exit(0);
}

int
main(void)
{
  doit("1234567");
  exit(1);
}'''

  if cc_can_run
    rres = cc.run(vsnprintf_c99_test_code, name : 'C99 vsnprintf')
    if rres.compiled() and rres.returncode() == 0
      glib_conf.set('HAVE_C99_VSNPRINTF', 1)
      have_good_vsnprintf = true
    endif
  else
      have_good_vsnprintf = meson.get_external_property('have_c99_vsnprintf', false)
      glib_conf.set('HAVE_C99_VSNPRINTF', have_good_vsnprintf)
  endif

  snprintf_c99_test_code = '''
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

int
doit()
{
  char buffer[32];
  int r;

  r = snprintf(buffer, 5, "1234567");

  if (r != 7)
    exit(1);

  r = snprintf(buffer, 0, "1234567");

  if (r != 7)
    exit(1);

  r = snprintf(NULL, 0, "1234567");

  if (r != 7)
    exit(1);

  exit(0);
}

int
main(void)
{
  doit();
  exit(1);
}'''

  if cc_can_run
    rres = cc.run(snprintf_c99_test_code, name : 'C99 snprintf')
    if rres.compiled() and rres.returncode() == 0
      glib_conf.set('HAVE_C99_SNPRINTF', 1)
      have_good_snprintf = true
    endif
  else
      have_good_snprintf = meson.get_external_property('have_c99_snprintf', false)
      glib_conf.set('HAVE_C99_SNPRINTF', have_good_snprintf)
  endif

  printf_unix98_test_code = '''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main (void)
{
  char buffer[128];

  sprintf (buffer, "%2\$d %3\$d %1\$d", 1, 2, 3);
  if (strcmp ("2 3 1", buffer) == 0)
    exit (0);
  exit (1);
}'''

  if cc_can_run
    rres = cc.run(printf_unix98_test_code, name : 'Unix98 printf positional parameters')
    if rres.compiled() and rres.returncode() == 0
      glib_conf.set('HAVE_UNIX98_PRINTF', 1)
      have_good_printf = true
    endif
  else
      have_good_printf = meson.get_external_property('have_unix98_printf', false)
      glib_conf.set('HAVE_UNIX98_PRINTF', have_good_printf)
  endif
endif

if host_system == 'windows'
  glib_conf.set_quoted('EXEEXT', '.exe')
else
  glib_conf.set('EXEEXT', '')
endif

# Our printf is 'good' only if vsnpintf()/snprintf()/printf() supports C99 well enough
use_system_printf = have_good_vsnprintf and have_good_snprintf and have_good_printf
glib_conf.set('USE_SYSTEM_PRINTF', use_system_printf)
glibconfig_conf.set('GLIB_USING_SYSTEM_PRINTF', use_system_printf)

if not use_system_printf
  # gnulib has vasprintf so override the previous check
  glib_conf.set('HAVE_VASPRINTF', 1)
endif

# Check for nl_langinfo and CODESET
if cc.links('''#include <langinfo.h>
               int main (int argc, char ** argv) {
                 char *codeset = nl_langinfo (CODESET);
                 (void) codeset;
                 return 0;
               }''', name : 'nl_langinfo and CODESET')
  glib_conf.set('HAVE_LANGINFO_CODESET', 1)
  glib_conf.set('HAVE_CODESET', 1)
endif

# Check for nl_langinfo and LC_TIME parts that are needed in gdatetime.c
have_langinfo_time = false
if cc.links('''#include <langinfo.h>
               int main (int argc, char ** argv) {
                 char *str;
                 str = nl_langinfo (PM_STR);
                 str = nl_langinfo (D_T_FMT);
                 str = nl_langinfo (D_FMT);
                 str = nl_langinfo (T_FMT);
                 str = nl_langinfo (T_FMT_AMPM);
                 str = nl_langinfo (MON_1);
                 str = nl_langinfo (ABMON_12);
                 str = nl_langinfo (DAY_1);
                 str = nl_langinfo (ABDAY_7);
                 (void) str;
                 return 0;
               }''', name : 'nl_langinfo (PM_STR)')
  have_langinfo_time = true
  glib_conf.set('HAVE_LANGINFO_TIME', 1)
endif

# Linux glibc supports ERA, but FreeBSD and macOS don’t
if cc.links('''#include <langinfo.h>
               int main (int argc, char **argv) {
                 char *str;
                 str = nl_langinfo (ERA);
                 str = nl_langinfo (ERA_D_T_FMT);
                 str = nl_langinfo (ERA_D_FMT);
                 str = nl_langinfo (ERA_T_FMT);
                 str = nl_langinfo (_NL_TIME_ERA_NUM_ENTRIES);
                 (void) str;
                 return 0;
               }''', name : 'nl_langinfo (ERA)')
  glib_conf.set('HAVE_LANGINFO_ERA', 1)
  if not have_langinfo_time
    error('nl_langinfo(ERA) is supported but more basic nl_langinfo() functionality like PM_STR is not')
  endif
endif

if cc.links('''#include <langinfo.h>
               int main (int argc, char ** argv) {
                 char *str;
                 str = nl_langinfo (_NL_CTYPE_OUTDIGIT0_MB);
                 str = nl_langinfo (_NL_CTYPE_OUTDIGIT1_MB);
                 str = nl_langinfo (_NL_CTYPE_OUTDIGIT2_MB);
                 str = nl_langinfo (_NL_CTYPE_OUTDIGIT3_MB);
                 str = nl_langinfo (_NL_CTYPE_OUTDIGIT4_MB);
                 str = nl_langinfo (_NL_CTYPE_OUTDIGIT5_MB);
                 str = nl_langinfo (_NL_CTYPE_OUTDIGIT6_MB);
                 str = nl_langinfo (_NL_CTYPE_OUTDIGIT7_MB);
                 str = nl_langinfo (_NL_CTYPE_OUTDIGIT8_MB);
                 str = nl_langinfo (_NL_CTYPE_OUTDIGIT9_MB);
                 (void) str;
                 return 0;
               }''', name : 'nl_langinfo (_NL_CTYPE_OUTDIGITn_MB)')
  glib_conf.set('HAVE_LANGINFO_OUTDIGIT', 1)
  if not have_langinfo_time
    error('nl_langinfo(_NL_CTYPE_OUTDIGITn_MB) is supported but more basic nl_langinfo() functionality like PM_STR is not')
  endif
endif

# Check for nl_langinfo and alternative month names
if cc.links('''#ifndef _GNU_SOURCE
              # define _GNU_SOURCE
              #endif
              #include <langinfo.h>
               int main (int argc, char ** argv) {
                 char *str;
                 str = nl_langinfo (ALTMON_1);
                 str = nl_langinfo (ALTMON_2);
                 str = nl_langinfo (ALTMON_3);
                 str = nl_langinfo (ALTMON_4);
                 str = nl_langinfo (ALTMON_5);
                 str = nl_langinfo (ALTMON_6);
                 str = nl_langinfo (ALTMON_7);
                 str = nl_langinfo (ALTMON_8);
                 str = nl_langinfo (ALTMON_9);
                 str = nl_langinfo (ALTMON_10);
                 str = nl_langinfo (ALTMON_11);
                 str = nl_langinfo (ALTMON_12);
                 (void) str;
                 return 0;
               }''', name : 'nl_langinfo (ALTMON_n)')
  glib_conf.set('HAVE_LANGINFO_ALTMON', 1)
  if not have_langinfo_time
    error('nl_langinfo(ALTMON_n) is supported but more basic nl_langinfo() functionality like PM_STR is not')
  endif
endif

# Check for nl_langinfo and abbreviated alternative month names
if cc.links('''#ifndef _GNU_SOURCE
              # define _GNU_SOURCE
              #endif
              #include <langinfo.h>
               int main (int argc, char ** argv) {
                 char *str;
                 str = nl_langinfo (_NL_ABALTMON_1);
                 str = nl_langinfo (_NL_ABALTMON_2);
                 str = nl_langinfo (_NL_ABALTMON_3);
                 str = nl_langinfo (_NL_ABALTMON_4);
                 str = nl_langinfo (_NL_ABALTMON_5);
                 str = nl_langinfo (_NL_ABALTMON_6);
                 str = nl_langinfo (_NL_ABALTMON_7);
                 str = nl_langinfo (_NL_ABALTMON_8);
                 str = nl_langinfo (_NL_ABALTMON_9);
                 str = nl_langinfo (_NL_ABALTMON_10);
                 str = nl_langinfo (_NL_ABALTMON_11);
                 str = nl_langinfo (_NL_ABALTMON_12);
                 (void) str;
                 return 0;
               }''', name : 'nl_langinfo (_NL_ABALTMON_n)')
  glib_conf.set('HAVE_LANGINFO_ABALTMON', 1)
  if not have_langinfo_time
    error('nl_langinfo(_NL_ABALTMON_n) is supported but more basic nl_langinfo() functionality like PM_STR is not')
  endif
endif

# Check for nl_langinfo and _NL_TIME_CODESET
if cc.links('''#include <langinfo.h>
               int main (int argc, char ** argv) {
                 char *codeset = nl_langinfo (_NL_TIME_CODESET);
                 (void) codeset;
                 return 0;
               }''', name : 'nl_langinfo and _NL_TIME_CODESET')
  glib_conf.set('HAVE_LANGINFO_TIME_CODESET', 1)
  if not have_langinfo_time
    error('nl_langinfo(_NL_TIME_CODESET) is supported but more basic nl_langinfo() functionality like PM_STR is not')
  endif
endif

# Check if C compiler supports the 'signed' keyword
if not cc.compiles('''signed char x;''', name : 'signed')
  glib_conf.set('signed', '/* NOOP */')
endif

# Check if the ptrdiff_t type exists
if cc.has_header_symbol('stddef.h', 'ptrdiff_t')
  glib_conf.set('HAVE_PTRDIFF_T', 1)
endif

# Check for sig_atomic_t type
if cc.links('''#include <signal.h>
               #include <sys/types.h>
               sig_atomic_t val = 42;
               int main (int argc, char ** argv) {
                 return val == 42 ? 0 : 1;
               }''', name : 'sig_atomic_t')
  glib_conf.set('HAVE_SIG_ATOMIC_T', 1)
endif

# Check if 'long long' works
# jm_AC_TYPE_LONG_LONG
if cc.compiles('''long long ll = 1LL;
                  int i = 63;
                  int some_func (void) {
                    long long llmax = (long long) -1;
                    return ll << i | ll >> i | llmax / ll | llmax % ll;
                  }''', name : 'long long')
  glib_conf.set('HAVE_LONG_LONG', 1)
  have_long_long = true
else
  have_long_long = false
endif

# Test whether the compiler supports the 'long double' type.
if cc.compiles('''/* The Stardent Vistra knows sizeof(long double), but does not support it.  */
                  long double foo = 0.0;
                  /* On Ultrix 4.3 cc, long double is 4 and double is 8.  */
                  int array [2*(sizeof(long double) >= sizeof(double)) - 1];''',
               name : 'long double')
  glib_conf.set('HAVE_LONG_DOUBLE', 1)
endif

# Test whether <stddef.h> has the 'wchar_t' type.
if cc.has_header_symbol('stddef.h', 'wchar_t')
  glib_conf.set('HAVE_WCHAR_T', 1)
endif

# Test whether <wchar.h> has the 'wint_t' type.
if cc.has_header_symbol('wchar.h', 'wint_t')
  glib_conf.set('HAVE_WINT_T', 1)
endif

found_uintmax_t = false

# Define HAVE_INTTYPES_H_WITH_UINTMAX if <inttypes.h> exists,
# doesn't clash with <sys/types.h>, and declares uintmax_t.
# jm_AC_HEADER_INTTYPES_H
if cc.compiles('''#include <sys/types.h>
                  #include <inttypes.h>
                  void some_func (void) {
                    uintmax_t i = (uintmax_t) -1;
                    (void) i;
                  }''', name : 'uintmax_t in inttypes.h')
  glib_conf.set('HAVE_INTTYPES_H_WITH_UINTMAX', 1)
  found_uintmax_t = true
endif

# Define HAVE_STDINT_H_WITH_UINTMAX if <stdint.h> exists,
# doesn't clash with <sys/types.h>, and declares uintmax_t.
# jm_AC_HEADER_STDINT_H
if cc.compiles('''#include <sys/types.h>
                  #include <stdint.h>
                  void some_func (void) {
                    uintmax_t i = (uintmax_t) -1;
                    (void) i;
                  }''', name : 'uintmax_t in stdint.h')
  glib_conf.set('HAVE_STDINT_H_WITH_UINTMAX', 1)
  found_uintmax_t = true
endif

# Define intmax_t to 'long' or 'long long'
# if it is not already defined in <stdint.h> or <inttypes.h>.
# For simplicity, we assume that a header file defines 'intmax_t' if and
# only if it defines 'uintmax_t'.
if found_uintmax_t
  glib_conf.set('HAVE_INTMAX_T', 1)
elif have_long_long
  glib_conf.set('intmax_t', 'long long')
else
  glib_conf.set('intmax_t', 'long')
endif

char_size = cc.sizeof('char')
short_size = cc.sizeof('short')
int_size = cc.sizeof('int')
voidp_size = cc.sizeof('void*')
long_size = cc.sizeof('long')
if have_long_long
  long_long_size = cc.sizeof('long long')
else
  long_long_size = 0
endif
sizet_size = cc.sizeof('size_t')
if cc.get_id() == 'msvc' or cc.get_id() == 'clang-cl'
  ssizet_size = cc.sizeof('SSIZE_T', prefix : '#include <BaseTsd.h>')
else
  ssizet_size = cc.sizeof('ssize_t', prefix : '#include <unistd.h>')
endif

# Some platforms (Apple) hard-code int64_t to long long instead of
# using long on 64-bit architectures. This can cause type mismatch
# warnings when trying to interface with code using the standard
# library type. Test for the warnings and set gint64 to whichever
# works.
if long_long_size == long_size
  if cc.compiles('''#if defined(_AIX) && !defined(__GNUC__)
                    #pragma options langlvl=stdc99
                    #endif
                    #pragma GCC diagnostic error "-Wincompatible-pointer-types"
                    #include <stdint.h>
                    #include <stdio.h>
                    int main () {
                      int64_t i1 = 1;
                      long *i2 = &i1;
                      (void) i2;
                      return 1;
                    }''', name : 'int64_t is long')
    int64_t_typedef = 'long'
  elif cc.compiles('''#if defined(_AIX) && !defined(__GNUC__)
                      #pragma options langlvl=stdc99
                      #endif
                      #pragma GCC diagnostic error "-Wincompatible-pointer-types"
                      #include <stdint.h>
                      #include <stdio.h>
                      int main () {
                        int64_t i1 = 1;
                        long long *i2 = &i1;
                        (void) i2;
                        return 1;
                      }''', name : 'int64_t is long long')
    int64_t_typedef = 'long long'
  else
    error('Cannot detect int64_t typedef')
  endif
endif

int64_m = 'll'
char_align = cc.alignment('char')
short_align = cc.alignment('short')
int_align = cc.alignment('int')
voidp_align = cc.alignment('void*')
long_align = cc.alignment('long')
long_long_align = cc.alignment('long long')
# NOTE: We don't check for size of __int64 because long long is guaranteed to
# be 64-bit in C99, and it is available on all supported compilers
sizet_align = cc.alignment('size_t')

glib_conf.set('SIZEOF_CHAR', char_size)
glib_conf.set('SIZEOF_INT', int_size)
glib_conf.set('SIZEOF_SHORT', short_size)
glib_conf.set('SIZEOF_LONG', long_size)
glib_conf.set('SIZEOF_LONG_LONG', long_long_size)
glib_conf.set('SIZEOF_SIZE_T', sizet_size)
glib_conf.set('SIZEOF_SSIZE_T', ssizet_size)
glib_conf.set('SIZEOF_VOID_P', voidp_size)
glib_conf.set('SIZEOF_WCHAR_T', cc.sizeof('wchar_t', prefix: '#include <stddef.h>'))

if short_size == 2
  gint16 = 'short'
  gint16_modifier='h'
  gint16_format='hi'
  guint16_format='hu'
elif int_size == 2
  gint16 = 'int'
  gint16_modifier=''
  gint16_format='i'
  guint16_format='u'
else
  error('Compiler provides no native 16-bit integer type')
endif
glibconfig_conf.set('gint16', gint16)
glibconfig_conf.set_quoted('gint16_modifier', gint16_modifier)
glibconfig_conf.set_quoted('gint16_format', gint16_format)
glibconfig_conf.set_quoted('guint16_format', guint16_format)

if short_size == 4
  gint32 = 'short'
  gint32_modifier='h'
  gint32_format='hi'
  guint32_format='hu'
  guint32_align = short_align
elif int_size == 4
  gint32 = 'int'
  gint32_modifier=''
  gint32_format='i'
  guint32_format='u'
  guint32_align = int_align
elif long_size == 4
  gint32 = 'long'
  gint32_modifier='l'
  gint32_format='li'
  guint32_format='lu'
  guint32_align = long_align
else
  error('Compiler provides no native 32-bit integer type')
endif
glibconfig_conf.set('gint32', gint32)
glibconfig_conf.set_quoted('gint32_modifier', gint32_modifier)
glibconfig_conf.set_quoted('gint32_format', gint32_format)
glibconfig_conf.set_quoted('guint32_format', guint32_format)

if int_size == 8
  gint64 = 'int'
  gint64_modifier=''
  gint64_format='i'
  guint64_format='u'
  glib_extension=''
  gint64_constant='(val)'
  guint64_constant='(val)'
  guint64_align = int_align
elif long_size == 8 and (long_long_size != long_size or int64_t_typedef == 'long')
  gint64 = 'long'
  glib_extension=''
  gint64_modifier='l'
  gint64_format='li'
  guint64_format='lu'
  gint64_constant='(val##L)'
  guint64_constant='(val##UL)'
  guint64_align = long_align
elif long_long_size == 8 and (long_long_size != long_size or int64_t_typedef == 'long long')
  gint64 = 'long long'
  glib_extension='G_GNUC_EXTENSION '
  gint64_modifier=int64_m
  gint64_format=int64_m + 'i'
  guint64_format=int64_m + 'u'
  gint64_constant='(G_GNUC_EXTENSION (val##LL))'
  guint64_constant='(G_GNUC_EXTENSION (val##ULL))'
  guint64_align = long_long_align
else
  error('Compiler provides no native 64-bit integer type')
endif
glibconfig_conf.set('glib_extension', glib_extension)
glibconfig_conf.set('gint64', gint64)
glibconfig_conf.set_quoted('gint64_modifier', gint64_modifier)
glibconfig_conf.set_quoted('gint64_format', gint64_format)
glibconfig_conf.set_quoted('guint64_format', guint64_format)
glibconfig_conf.set('gint64_constant', gint64_constant)
glibconfig_conf.set('guint64_constant', guint64_constant)

if host_system == 'windows'
  glibconfig_conf.set('g_pid_type', 'void*')
  glibconfig_conf.set_quoted('g_pid_format', 'p')
  if host_machine.cpu_family() in ['x86_64', 'aarch64']
    glibconfig_conf.set_quoted('g_pollfd_format', '%#' + int64_m + 'x')
  else
    glibconfig_conf.set_quoted('g_pollfd_format', '%#x')
  endif
  glibconfig_conf.set('g_dir_separator', '\\\\')
  glibconfig_conf.set('g_searchpath_separator', ';')
else
  glibconfig_conf.set('g_pid_type', 'int')
  glibconfig_conf.set_quoted('g_pid_format', 'i')
  glibconfig_conf.set_quoted('g_pollfd_format', '%d')
  glibconfig_conf.set('g_dir_separator', '/')
  glibconfig_conf.set('g_searchpath_separator', ':')
endif

g_sizet_compatibility = {
  'short': sizet_size == short_size,
  'int': sizet_size == int_size,
  'long': sizet_size == long_size,
  'long long': sizet_size == long_long_size,
}

# Do separate checks for gcc/clang (and ignore other compilers for now), since
# we need to explicitly pass -Werror to the compilers.
# FIXME: https://github.com/mesonbuild/meson/issues/5399
if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
  foreach type_name, size_compatibility : g_sizet_compatibility
    g_sizet_compatibility += { type_name: size_compatibility and
      cc.compiles(
        '''#include <stddef.h>
        static size_t f (size_t *i) { return *i + 1; }
        int main (void) {
          unsigned ''' + type_name + ''' i = 0;
          f (&i);
          return 0;
        }''',
        args: ['-Werror'],
        name : 'GCC size_t typedef is ' + type_name), }
  endforeach
endif

if g_sizet_compatibility['short']
  glibconfig_conf.set('glib_size_type_define', 'short')
  glibconfig_conf.set_quoted('gsize_modifier', 'h')
  glibconfig_conf.set_quoted('gssize_modifier', 'h')
  glibconfig_conf.set_quoted('gsize_format', 'hu')
  glibconfig_conf.set_quoted('gssize_format', 'hi')
  glibconfig_conf.set('glib_msize_type', 'SHRT')
elif g_sizet_compatibility['int']
  glibconfig_conf.set('glib_size_type_define', 'int')
  glibconfig_conf.set_quoted('gsize_modifier', '')
  glibconfig_conf.set_quoted('gssize_modifier', '')
  glibconfig_conf.set_quoted('gsize_format', 'u')
  glibconfig_conf.set_quoted('gssize_format', 'i')
  glibconfig_conf.set('glib_msize_type', 'INT')
elif g_sizet_compatibility['long']
  glibconfig_conf.set('glib_size_type_define', 'long')
  glibconfig_conf.set_quoted('gsize_modifier', 'l')
  glibconfig_conf.set_quoted('gssize_modifier', 'l')
  glibconfig_conf.set_quoted('gsize_format', 'lu')
  glibconfig_conf.set_quoted('gssize_format', 'li')
  glibconfig_conf.set('glib_msize_type', 'LONG')
elif g_sizet_compatibility['long long']
  glibconfig_conf.set('glib_size_type_define', 'long long')
  glibconfig_conf.set_quoted('gsize_modifier', int64_m)
  glibconfig_conf.set_quoted('gssize_modifier', int64_m)
  glibconfig_conf.set_quoted('gsize_format', int64_m + 'u')
  glibconfig_conf.set_quoted('gssize_format', int64_m + 'i')
  glibconfig_conf.set('glib_msize_type', 'INT64')
else
  error('Could not determine size of size_t.')
endif

if voidp_size == int_size
  glibconfig_conf.set('glib_intptr_type_define', 'int')
  glibconfig_conf.set_quoted('gintptr_modifier', '')
  glibconfig_conf.set_quoted('gintptr_format', 'i')
  glibconfig_conf.set_quoted('guintptr_format', 'u')
  glibconfig_conf.set('glib_gpi_cast', '(gint)')
  glibconfig_conf.set('glib_gpui_cast', '(guint)')
elif voidp_size == long_size
  glibconfig_conf.set('glib_intptr_type_define', 'long')
  glibconfig_conf.set_quoted('gintptr_modifier', 'l')
  glibconfig_conf.set_quoted('gintptr_format', 'li')
  glibconfig_conf.set_quoted('guintptr_format', 'lu')
  glibconfig_conf.set('glib_gpi_cast', '(glong)')
  glibconfig_conf.set('glib_gpui_cast', '(gulong)')
elif voidp_size == long_long_size
  glibconfig_conf.set('glib_intptr_type_define', 'long long')
  glibconfig_conf.set_quoted('gintptr_modifier', int64_m)
  glibconfig_conf.set_quoted('gintptr_format', int64_m + 'i')
  glibconfig_conf.set_quoted('guintptr_format', int64_m + 'u')
  glibconfig_conf.set('glib_gpi_cast', '(gint64)')
  glibconfig_conf.set('glib_gpui_cast', '(guint64)')
else
  error('Could not determine size of void *')
endif

if long_size != 8 and long_long_size != 8 and int_size != 8
  error('GLib requires a 64-bit type. You might want to consider using the GNU C compiler.')
endif

glibconfig_conf.set('gintbits', int_size * 8)
glibconfig_conf.set('glongbits', long_size * 8)
glibconfig_conf.set('gsizebits', sizet_size * 8)
glibconfig_conf.set('gssizebits', ssizet_size * 8)

# XXX: https://gitlab.gnome.org/GNOME/glib/issues/1413
if host_system == 'windows' or host_system == 'cygwin'
  g_module_suffix = 'dll'
else
  g_module_suffix = 'so'
endif
glibconfig_conf.set('g_module_suffix', g_module_suffix)

glibconfig_conf.set('GLIB_MAJOR_VERSION', major_version)
glibconfig_conf.set('GLIB_MINOR_VERSION', minor_version)
glibconfig_conf.set('GLIB_MICRO_VERSION', micro_version)
glibconfig_conf.set('GLIB_VERSION', glib_version)

glibconfig_conf.set('glib_void_p', voidp_size)
glibconfig_conf.set('glib_long', long_size)
glibconfig_conf.set('glib_size_t', sizet_size)
glibconfig_conf.set('glib_ssize_t', ssizet_size)
if host_machine.endian() == 'big'
  glibconfig_conf.set('g_byte_order', 'G_BIG_ENDIAN')
  glibconfig_conf.set('g_bs_native', 'BE')
  glibconfig_conf.set('g_bs_alien', 'LE')
else
  glibconfig_conf.set('g_byte_order', 'G_LITTLE_ENDIAN')
  glibconfig_conf.set('g_bs_native', 'LE')
  glibconfig_conf.set('g_bs_alien', 'BE')
endif

# === va_copy checks ===

glib_vacopy = ''

# We check for G_VA_COPY_AS_ARRAY for historical reasons, but we no longer
# use it: use Standard C va_copy() instead.

va_list_val_copy_prog = '''
  #include <stdarg.h>
  #include <stdlib.h>
  void f (int i, ...) {
    va_list args1, args2;
    va_start (args1, i);
    args2 = args1;
    if (va_arg (args2, int) != 42 || va_arg (args1, int) != 42)
      exit (1);
    va_end (args1); va_end (args2);
  }
  int main() {
    f (0, 42);
    return 0;
  }'''

if cc_can_run
  rres = cc.run(va_list_val_copy_prog, name : 'va_lists can be copied as values')
  glib_va_val_copy = rres.compiled() and rres.returncode() == 0
else
  glib_va_val_copy = meson.get_external_property('va_val_copy', true)
endif
if not glib_va_val_copy
  glib_vacopy = glib_vacopy + '\n#define G_VA_COPY_AS_ARRAY 1'
  glib_conf.set('G_VA_COPY_AS_ARRAY', 1)
endif
glibconfig_conf.set('glib_vacopy', glib_vacopy)

# check for flavours of varargs macros
g_have_iso_c_varargs = cc.compiles('''
  void some_func (void) {
    int a(int p1, int p2, int p3);
    #define call_a(...) a(1,__VA_ARGS__)
    call_a(2,3);
  }''', name : 'ISO C99 varargs macros in C')

if not g_have_iso_c_varargs
  error('GLib requires a C compiler with support for C99 __VA_ARG__ in macros.')
endif

if have_cxx
  g_have_iso_cxx_varargs = cxx.compiles('''
    void some_func (void) {
      int a(int p1, int p2, int p3);
      #define call_a(...) a(1,__VA_ARGS__)
      call_a(2,3);
    }''', name : 'ISO C99 varargs macros in C++')

  if not g_have_iso_cxx_varargs
    error('GLib requires a C++ compiler with support for C99 __VA_ARG__ in macros.')
  endif
endif

g_have_gnuc_varargs = cc.compiles('''
  void some_func (void) {
    int a(int p1, int p2, int p3);
    #define call_a(params...) a(1,params)
    call_a(2,3);
  }''', name : 'GNUC varargs macros')

if cc.has_header('alloca.h')
  glibconfig_conf.set('GLIB_HAVE_ALLOCA_H', true)
endif
has_syspoll = cc.has_header('sys/poll.h')
has_systypes = cc.has_header('sys/types.h')
if has_syspoll
  glibconfig_conf.set('GLIB_HAVE_SYS_POLL_H', true)
endif
has_winsock2 = cc.has_header('winsock2.h')

if has_syspoll and has_systypes
  poll_includes = '''
      #include<sys/poll.h>
      #include<sys/types.h>'''
elif has_winsock2
  poll_includes = '''
      #define _WIN32_WINNT @0@
      #include <winsock2.h>'''.format(glib_conf.get('_WIN32_WINNT'))
else
  # FIXME?
  error('FIX POLL* defines')
endif

poll_defines = [
  [ 'POLLIN', 'g_pollin', 1 ],
  [ 'POLLOUT', 'g_pollout', 4 ],
  [ 'POLLPRI', 'g_pollpri', 2 ],
  [ 'POLLERR', 'g_pollerr', 8 ],
  [ 'POLLHUP', 'g_pollhup', 16 ],
  [ 'POLLNVAL', 'g_pollnval', 32 ],
]

if has_syspoll and has_systypes
  foreach d : poll_defines
    val = cc.compute_int(d[0], prefix: poll_includes, guess: d[2])
    glibconfig_conf.set(d[1], val)
  endforeach
elif has_winsock2
  # Due to a missed bug in configure.ac the poll test
  # never succeeded on Windows and used some pre-defined
  # values as a fallback. Keep using them to maintain
  # ABI compatibility with autotools builds of glibs
  # and with *any* glib-using code compiled against them,
  # since these values end up in a public header glibconfig.h.
  foreach d : poll_defines
    glibconfig_conf.set(d[1], d[2])
  endforeach
endif

# Internet address families
# FIXME: what about Cygwin (G_WITH_CYGWIN)
if host_system == 'windows'
  inet_includes = '''
      #include <winsock2.h>'''
else
  inet_includes = '''
      #include <sys/types.h>
      #include <sys/socket.h>'''
endif

inet_defines = [
  [ 'AF_UNIX', 'g_af_unix', 1 ],
  [ 'AF_INET', 'g_af_inet', 2 ],
  [ 'AF_INET6', 'g_af_inet6', 30 ],
  [ 'MSG_OOB', 'g_msg_oob', 1 ],
  [ 'MSG_PEEK', 'g_msg_peek', 2 ],
  [ 'MSG_DONTROUTE', 'g_msg_dontroute', 4 ],
]
foreach d : inet_defines
  val = cc.compute_int(d[0], prefix: inet_includes, guess: d[2])
  glibconfig_conf.set(d[1], val)
endforeach

if host_system == 'windows'
  have_ipv6 = true
else
  have_ipv6 = cc.has_type('struct in6_addr', prefix: '#include <netinet/in.h>')
endif
glib_conf.set('HAVE_IPV6', have_ipv6)

# We need to decide at configure time if GLib will use real atomic
# operations ("lock free") or emulated ones with a mutex.  This is
# because we must put this information in glibconfig.h so we know if
# it is safe or not to inline using compiler intrinsics directly from
# the header.
#
# We also publish the information via G_ATOMIC_LOCK_FREE in case the
# user is interested in knowing if they can use the atomic ops across
# processes.
#
# We can currently support the atomic ops natively when building GLib
# with recent versions of GCC or MSVC.
#
# Note that the atomic ops are only available with GCC on x86 when
# using -march=i486 or higher.  If we detect that the atomic ops are
# not available but would be available given the right flags, we want
# to abort and advise the user to fix their CFLAGS.  It's better to do
# that then to silently fall back on emulated atomic ops just because
# the user had the wrong build environment.
atomictest = '''int main() {
  int atomic = 2;
  __sync_bool_compare_and_swap (&atomic, 2, 3);
  return 0;
}
'''

atomicdefine = '''
#ifndef __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
#error "compiler has atomic ops, but doesn't define __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4"
#endif
'''

# We know that we can always use real ("lock free") atomic operations with MSVC
if cc.get_id() == 'msvc' or cc.get_id() == 'clang-cl' or cc.links(atomictest, name : 'atomic ops')
  have_atomic_lock_free = true
  if cc.get_id() == 'gcc' and not cc.compiles(atomicdefine, name : 'atomic ops define')
    # Old gcc release may provide
    # __sync_bool_compare_and_swap but doesn't define
    # __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4
    glib_conf.set('__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4', true)
  endif

  if cc.get_id() == 'gcc' or cc.get_id() == 'clang'
    sync_swap_test = '''
      int main() {
        int atomic = 2;
        __sync_swap (&atomic, 2);
        return 0;
      }
    '''

    glib_conf.set('_GLIB_GCC_HAVE_SYNC_SWAP', cc.links(sync_swap_test, name : 'sync swap'))
  endif
else
  have_atomic_lock_free = false
  if host_machine.cpu_family() == 'x86' and cc.links(atomictest, args : '-march=i486')
    error('GLib must be built with -march=i486 or later.')
  endif
endif
glibconfig_conf.set('G_ATOMIC_LOCK_FREE', have_atomic_lock_free)

# === Threads ===

if get_option('force_posix_threads')
  warning('DEPRECATION: Option \'force_posix_threads\' is deprecated and will be removed after GLib 2.72; please file an issue with your use case if you still require it')
endif

# Determination of thread implementation
if host_system == 'windows' and not get_option('force_posix_threads')
  thread_dep = []
  threads_implementation = 'win32'
  glibconfig_conf.set('g_threads_impl_def', 'WIN32')
  glib_conf.set('THREADS_WIN32', 1)
else
  thread_dep = dependency('threads')
  threads_implementation = 'posix'
  pthread_prefix = '''
      #ifndef _GNU_SOURCE
      # define _GNU_SOURCE
      #endif
      #include <pthread.h>'''
  glibconfig_conf.set('g_threads_impl_def', 'POSIX')
  glib_conf.set('THREADS_POSIX', 1)
  if cc.has_header_symbol('pthread.h', 'pthread_attr_setstacksize')
    glib_conf.set('HAVE_PTHREAD_ATTR_SETSTACKSIZE', 1)
  endif
  if cc.has_header_symbol('pthread.h', 'pthread_attr_setinheritsched')
    glib_conf.set('HAVE_PTHREAD_ATTR_SETINHERITSCHED', 1)
  endif
  if cc.has_header_symbol('pthread.h', 'pthread_condattr_setclock')
    glib_conf.set('HAVE_PTHREAD_CONDATTR_SETCLOCK', 1)
  endif
  if cc.has_header_symbol('pthread.h', 'pthread_cond_timedwait_relative_np')
    glib_conf.set('HAVE_PTHREAD_COND_TIMEDWAIT_RELATIVE_NP', 1)
  endif
  if cc.has_header_symbol('pthread.h', 'pthread_getname_np', prefix : pthread_prefix)
    glib_conf.set('HAVE_PTHREAD_GETNAME_NP', 1)
  endif
  if cc.has_header_symbol('pthread.h', 'pthread_getaffinity_np', prefix : pthread_prefix)
    glib_conf.set('HAVE_PTHREAD_GETAFFINITY_NP', 1)
  endif

  # Assume that pthread_setname_np is available in some form; same as configure
  if cc.links(pthread_prefix + '''
              int main() {
                pthread_setname_np("example");
                return 0;
              }''',
              name : 'pthread_setname_np(const char*)',
              dependencies : thread_dep)
    # macOS and iOS
    glib_conf.set('HAVE_PTHREAD_SETNAME_NP_WITHOUT_TID', 1)
  elif cc.links(pthread_prefix + '''
                int main() {
                  pthread_setname_np(pthread_self(), "example");
                  return 0;
                }''',
                name : 'pthread_setname_np(pthread_t, const char*)',
                dependencies : thread_dep)
    # Linux, Solaris, etc.
    glib_conf.set('HAVE_PTHREAD_SETNAME_NP_WITH_TID', 1)
  elif cc.links(pthread_prefix + '''
                int main() {
                  pthread_setname_np(pthread_self(), "%s", "example");
                  return 0;
                }''',
                name : 'pthread_setname_np(pthread_t, const char*, void*)',
                dependencies : thread_dep)
    # NetBSD
    glib_conf.set('HAVE_PTHREAD_SETNAME_NP_WITH_TID_AND_ARG', 1)
  elif cc.links(pthread_prefix + '''
                #include <pthread_np.h>
                int main() {
                  pthread_set_name_np(pthread_self(), "example");
                  return 0;
                }''',
                name : 'pthread_set_name_np(pthread_t, const char*)',
                dependencies : thread_dep)
    # FreeBSD, DragonFlyBSD, OpenBSD, etc.
    glib_conf.set('HAVE_PTHREAD_SET_NAME_NP', 1)
  endif
endif

# FIXME: we should make it print the result and always return 0, so that
# the output in meson shows up as green
# volatile is needed here to avoid optimisations in the test
stack_grows_check_prog = '''
  volatile int *a = 0, *b = 0;
  void f (int i) {
    volatile int x = 5;
    if (i == 0)
      b = &x;
    else
      f (i - 1);
  }
  int main () {
    volatile int y = 7;
    a = &y;
    f (100);
    return b > a ? 0 : 1;
  }'''

if cc_can_run
  rres = cc.run(stack_grows_check_prog, name : 'stack grows check')
  growing_stack = rres.compiled() and rres.returncode() == 0
else
  growing_stack = meson.get_external_property('growing_stack', false)
endif

glibconfig_conf.set10('G_HAVE_GROWING_STACK', growing_stack)

# Tests for iconv
#
# We should never use the MinGW C library's iconv because it may not be
# available in the actual runtime environment. On Windows, we always use
# the built-in implementation
if host_system == 'windows'
  # We have a #include "win_iconv.c" in gconvert.c on Windows, so we don't need
  # any external library for it
  libiconv = []
else
  libiconv = dependency('iconv')
endif

pcre2_req = '>=10.32'
pcre2_options = [
  'default_library=static',
  'test=false',
]

# Pick up pcre from the system, or if "--force-fallback-for libpcre2-8" was specified
pcre2 = dependency('libpcre2-8', version: pcre2_req, required: false, default_options: pcre2_options)
if not pcre2.found()
  if cc.get_id() == 'msvc' or cc.get_id() == 'clang-cl'
  # MSVC: Search for the PCRE2 library by the configuration, which corresponds
  # to the output of CMake builds of PCRE2.  Note that debugoptimized
  # is really a Release build with .PDB files.
    if vs_crt == 'debug'
      pcre2 = cc.find_library('pcre2d-8', required : false)
    else
      pcre2 = cc.find_library('pcre2-8', required : false)
    endif
  endif
endif

# Try again with the fallback
if not pcre2.found()
  pcre2 = dependency('libpcre2-8', version: pcre2_req, allow_fallback: true, default_options: pcre2_options)
  assert(pcre2.type_name() == 'internal')
  # static flags are automatically enabled by the subproject if it's built
  # with default_library=static
  use_pcre2_static_flag = false
elif host_system == 'windows' and pcre2.type_name() != 'internal'
  pcre2_static = cc.links('''#define PCRE2_STATIC
                             #define PCRE2_CODE_UNIT_WIDTH 8
                             #include <pcre2.h>
                             int main() {
                               void *p = NULL;
                               pcre2_code_free(p);
                               return 0;
                             }''',
                         dependencies: pcre2,
                         name : 'Windows system PCRE2 is a static build')
  use_pcre2_static_flag = pcre2_static
else
  use_pcre2_static_flag = false
endif

# Import the gvdb sources as a subproject to avoid having the copylib in-tree
subproject('gvdb', default_options: {'tests': false})
gvdb_dep = dependency('gvdb')

libm = cc.find_library('m', required : false)
libffi_dep = dependency('libffi', version : '>= 3.0.0', default_options: {'werror': false, 'tests': false, 'doc': false})

libz_dep = dependency('zlib')

libatomic_test_code = '''
  int main (int argc, char ** argv) {
    return 0;
  }'''
atomic_dep = []
if cc.links(libatomic_test_code, args : '-latomic', name : 'check for -latomic')
  atomic_dep = cc.find_library('atomic')
endif

# First check in libc, fallback to libintl, and as last chance build
# proxy-libintl subproject.
# FIXME: glib-gettext.m4 has much more checks to detect broken/incompatible
# implementations. This could be extended if issues are found in some platforms.
libintl_deps = []
libintl_prefix = '#include <libintl.h>'
libintl = dependency('intl', required: false)
if libintl.found() and libintl.type_name() != 'internal'
  # libintl supports different threading APIs, which may not
  # require additional flags, but it defaults to using pthreads if
  # found. Meson's "threads" dependency does not allow you to
  # prefer pthreads. We may not be using pthreads for glib itself
  # either so just link the library to satisfy libintl rather than
  # also defining the macros with the -pthread flag.
  #
  # Meson's builtin dependency lookup as of 0.60.0 doesn't check for
  # pthread, so we do this manually here.
  if cc.has_function('ngettext', dependencies : libintl, prefix: libintl_prefix)
    libintl_deps += [libintl]
  else
    libintl_iconv = cc.find_library('iconv', required : false)
    if libintl_iconv.found() and cc.has_function('ngettext', args : osx_ldflags, dependencies : [libintl, libintl_iconv])
      libintl_deps += [libintl, libintl_iconv]
    else
      libintl_pthread = cc.find_library('pthread', required : false)
      if libintl_pthread.found() and cc.has_function('ngettext', dependencies : [libintl, libintl_pthread], prefix: libintl_prefix)
        libintl_deps += [libintl, libintl_pthread]
      else
        libintl = disabler()
      endif
    endif
  endif
endif

if libintl.found() and libintl.type_name() != 'internal'
  have_bind_textdomain_codeset = cc.has_function('bind_textdomain_codeset', dependencies: libintl_deps, prefix: libintl_prefix)
else
  # using proxy-libintl fallback
  libintl = dependency('intl', allow_fallback: true)
  assert(libintl.type_name() == 'internal')
  libintl_deps = [libintl]
  have_bind_textdomain_codeset = true  # proxy-libintl supports it
endif

glib_conf.set('HAVE_BIND_TEXTDOMAIN_CODESET', have_bind_textdomain_codeset)

# We require gettext to always be present
glib_conf.set('HAVE_DCGETTEXT', 1)
glib_conf.set('HAVE_GETTEXT', 1)

glib_conf.set_quoted('GLIB_LOCALE_DIR', glib_localedir)

glib_conf.set_quoted('GLIB_LOCALSTATEDIR', glib_localstatedir)
glib_conf.set_quoted('GLIB_RUNSTATEDIR', glib_runstatedir)

# libmount is only used by gio, but we need to fetch the libs to generate the
# pkg-config file below
libmount_dep = []
if host_system == 'linux'
  libmount_dep = dependency('mount', version : '>=2.23', required : get_option('libmount'))
  glib_conf.set('HAVE_LIBMOUNT', libmount_dep.found())

  if libmount_dep.found() and cc.has_function('mnt_monitor_veil_kernel', dependencies: libmount_dep)
    glib_conf.set('HAVE_MNT_MONITOR_VEIL_KERNEL', 1)
  endif
endif

if host_system == 'windows'
  winsock2 = cc.find_library('ws2_32')
  # Used for g_win32_clear_com() tests...
  wincodecs = cc.find_library('windowscodecs')
else
  winsock2 = not_found
  wincodecs = not_found
endif

selinux_dep = []
if host_system == 'linux'
  selinux_dep = dependency('libselinux', version: '>=2.2', required: get_option('selinux'))

  glib_conf.set('HAVE_SELINUX', selinux_dep.found())
endif

xattr_dep = []
if host_system != 'windows' and get_option('xattr')
  # either glibc or libattr can provide xattr support
  # for both of them, we check for getxattr being in
  # the library and a valid xattr header.

  # try glibc
  if cc.has_function('getxattr') and cc.has_header('sys/xattr.h')
    glib_conf.set('HAVE_SYS_XATTR_H', 1)
    glib_conf_prefix = glib_conf_prefix + '#define @0@ 1\n'.format('HAVE_SYS_XATTR_H')
  #failure. try libattr
  elif cc.has_header_symbol('attr/xattr.h', 'getxattr')
    glib_conf.set('HAVE_ATTR_XATTR_H', 1)
    glib_conf_prefix = glib_conf_prefix + '#define @0@ 1\n'.format('HAVE_ATTR_XATTR_H')
    xattr_dep = [cc.find_library('xattr')]
  else
    error('No getxattr implementation found in C library or libxattr')
  endif

  glib_conf.set('HAVE_XATTR', 1)
  if cc.compiles(glib_conf_prefix + '''
                 #include <stdio.h>
                 #ifdef HAVE_SYS_TYPES_H
                 #include <sys/types.h>
                 #endif
                 #ifdef HAVE_SYS_XATTR_H
                 #include <sys/xattr.h>
                 #elif HAVE_ATTR_XATTR_H
                 #include <attr/xattr.h>
                 #endif

                 int main (void) {
                   ssize_t len = getxattr("", "", NULL, 0, 0, XATTR_NOFOLLOW);
                   return len;
                 }''',
                 name : 'XATTR_NOFOLLOW')
    glib_conf.set('HAVE_XATTR_NOFOLLOW', 1)
  endif
endif

# If strlcpy is present (BSD and similar), check that it conforms to the BSD
# specification. Specifically Solaris 8's strlcpy() does not, see
# https://bugzilla.gnome.org/show_bug.cgi?id=53933 for further context.
if cc.has_function('strlcpy')
  if cc_can_run
    rres = cc.run('''#include <stdlib.h>
                    #include <string.h>
                    int main() {
                      char p[10];
                      (void) strlcpy (p, "hi", 10);
                      if (strlcat (p, "bye", 0) != 3)
                        return 1;
                      return 0;
                    }''',
                  name : 'OpenBSD strlcpy/strlcat')
    if rres.compiled() and rres.returncode() == 0
      glib_conf.set('HAVE_STRLCPY', 1)
    endif
  elif meson.get_external_property('have_strlcpy', false)
    glib_conf.set('HAVE_STRLCPY', 1)
  endif
endif

cmdline_test_code = '''
#include <fcntl.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int
__getcmdline (void)
{
/* This code is a dumbed-down version of g_file_get_contents() */
#ifndef O_BINARY
#define O_BINARY 0
#endif
#define BUFSIZE 1024
  char result[BUFSIZE];
  struct stat stat_buf;

  int fd = open ("/proc/self/cmdline", O_RDONLY|O_BINARY);
  if (fd < 0)
    exit (1);
  if (fstat (fd, &stat_buf))
    exit (1);

  if (stat_buf.st_size > 0 && S_ISREG (stat_buf.st_mode))
    {
      if (read (fd, result, BUFSIZE) <= 0)
        exit (1);
    }
  else
    {
      FILE *f = fdopen (fd, "r");
      if (f == NULL)
        exit (1);

      if (fread (result, 1, BUFSIZE, f) <= 0)
        exit (1);
    }

  return 0;
}

int
main (void)
{
  exit (__getcmdline ());
}'''

if cc_can_run
  rres = cc.run(cmdline_test_code, name : '/proc/self/cmdline')
  have_proc_self_cmdline = rres.compiled() and rres.returncode() == 0
else
  have_proc_self_cmdline = meson.get_external_property('have_proc_self_cmdline', false)
endif

glib_conf.set('HAVE_PROC_SELF_CMDLINE', have_proc_self_cmdline)

python = import('python').find_installation()

# Sets the shebang on script files. This has been changed many times including
# using env, the absolute path to the Python executable, and using both python
# and python3 names. Please review the history here, prior to suggesting
# further changes: https://gitlab.gnome.org/GNOME/glib/-/merge_requests/4391
python_shebang = '/usr/bin/env python3'

python_version = python.language_version()
python_version_req = '>=3.7'
python_test_libraries_path = meson.project_source_root() / 'tests' / 'lib'
assert(fs.exists(python_test_libraries_path))
if not python_version.version_compare(python_version_req)
  error('Requires Python @0@, @1@ found.'.format(python_version_req, python_version))
endif
python_test_libraries_built = meson.project_build_root() / 'tests' / 'lib'

# FIXME: https://github.com/mesonbuild/meson/issues/4668
#
# If we have a Python test which spawns a built native binary, that binary is
# listed in the `depends` argument of the `test()`. On Linux, this results in
# the directories containing the built libraries which the binary depends on
# being added to the `LD_LIBRARY_PATH` of the test invocation. On Windows,
# however, Meson currently doesn’t add those directories to `PATH` (which is the
# equivalent of `LD_LIBRARY_PATH`), so we have to do it manually.
python_test_env_common_path = []
if host_system == 'windows'
  build_root = meson.project_build_root()
  python_test_env_common_path += [
    build_root / 'gmodule',
    build_root / 'girepository',
    build_root / 'gio',
    build_root / 'glib',
    build_root / 'gobject',
  ]
endif

# Determine which user environment-dependent files that we want to install
bash = find_program('bash', required : false)
have_bash = bash.found() # For completion scripts
bash_comp_dep = dependency('bash-completion', version: '>=2.0', required: false)
have_sh = find_program('sh', required : false).found() # For glib-gettextize
have_pkg_config = find_program('pkg-config', required: false).found()

# Some installed tests require a custom environment
env_program = find_program('env', required: installed_tests_enabled)

# illumos & Solaris may need extra definitions to expose some SUS/POSIX
# interfaces in headers that conflict with previous Solaris headers.
# But if we define them to request an older version of the standards,
# we may hide things introduced in newer versions.  We only check the
# versions that are supported on systems new enough that meson runs on them.
if host_system == 'sunos'
  xopen_test_code = '''#include <unistd.h>
  #if _XOPEN_VERSION != _XOPEN_SOURCE
  #error "XOPEN_SOURCE of _XOPEN_SOURCE not supported"
  #endif'''
  foreach std : ['800', '700', '600']
    if cc.compiles(xopen_test_code, args: '-D_XOPEN_SOURCE=' + std, name: 'building with _XOPEN_SOURCE=' + std)
      xopen_version = std
      break
    endif
  endforeach
  glib_conf.set('_XOPEN_SOURCE', xopen_version)
  glib_conf.set('__EXTENSIONS__',1)
endif

# Sadly Meson does not expose this value:
# https://github.com/mesonbuild/meson/pull/3460
if host_system == 'windows'
  # Autotools explicitly removed --Wl,--export-all-symbols from windows builds,
  # with no explanation. Do the same here for now but this could be revisited if
  # if causes issues.
  export_dynamic_ldflags = []
elif host_system == 'cygwin'
  export_dynamic_ldflags = ['-Wl,--export-all-symbols']
elif host_system == 'darwin'
  export_dynamic_ldflags = []
elif host_system == 'sunos'
  export_dynamic_ldflags = []
else
  export_dynamic_ldflags = ['-Wl,--export-dynamic']
endif

win32_cflags = []
win32_ldflags = []
if host_system == 'windows' and cc.get_id() != 'msvc' and cc.get_id() != 'clang-cl'
  # Ensure MSVC-compatible struct packing convention is used when
  # compiling for Win32 with gcc. It is used for the whole project and exposed
  # in glib-2.0.pc.
  if not cc.compiles('''
struct _GTestMSBitfields
{
  int a : 1;
  short b : 1;
};

typedef char _StaticCheck[sizeof(struct _GTestMSBitfields) != sizeof(int) ? 1 : -1];
''')
    warning('''
Your compiler does not have ms-bitfields packing by default.
Please use gcc >= 4.7 or clang >= 12: GLib will drop -mms-bitfields in the future.
''')
  endif
  win32_cflags = ['-mms-bitfields']
  add_project_arguments(win32_cflags, language : 'c')

  # Win32 API libs, used only by libglib and exposed in glib-2.0.pc
  win32_ldflags = ['-lws2_32', '-lole32', '-lwinmm', '-lshlwapi', '-luuid']
elif host_system == 'cygwin'
  win32_ldflags = ['-luser32', '-lkernel32']
endif

# Tracing: dtrace
dtrace_option = get_option('dtrace')
enable_dtrace = dtrace_option.allowed()

if glib_have_carbon
  if dtrace_option.enabled()
    error('GLib dtrace support not yet compatible with macOS dtrace')
  else
    enable_dtrace = false
  endif
endif

if enable_dtrace
  dtrace = find_program('dtrace', required : dtrace_option)
  if not dtrace.found()
    enable_dtrace = false
  endif
endif

if enable_dtrace
  if not cc.has_header('sys/sdt.h')
    if dtrace_option.enabled()
      error('dtrace support needs sys/sdt.h header')
    else
      enable_dtrace = false
    endif
  endif
endif

if enable_dtrace
  # FIXME: autotools build also passes -fPIC -DPIC but is it needed in this case?
  dtrace_obj_gen = generator(dtrace,
    output : '@BASENAME@.o',
    arguments : ['-G', '-s', '@INPUT@', '-o', '@OUTPUT@'])
  dtrace_hdr_gen = generator(python,
    output : '@BASENAME@.h',
    arguments : ['-c', '''
import subprocess, sys
subprocess.run(sys.argv[1:], check=True)
output = sys.argv[6]
with open(output) as f:
  contents = f.read()
contents = contents.replace("define STAP_HAS_SEMAPHORES 1",
  "undef STAP_HAS_SEMAPHORES")
contents = contents.replace("define _SDT_HAS_SEMAPHORES 1",
  "undef _SDT_HAS_SEMAPHORES")
with open(output, "w") as f:
  f.write(contents)
''', dtrace.full_path(), '-h', '-s', '@INPUT@', '-o', '@OUTPUT@'])
  glib_conf.set('HAVE_DTRACE', 1)
endif

if cc.has_header_symbol('sys/ptrace.h', 'PTRACE_O_EXITKILL')
  glib_conf.set('HAVE_PTRACE_O_EXITKILL', 1)
endif

# systemtap
systemtap = get_option('systemtap').require(enable_dtrace, error_message: 'Cannot enable systemtap because dtrace feature is disabled')
enable_systemtap = systemtap.allowed()

if enable_systemtap
  tapset_install_dir = get_option('tapset_install_dir')
  if tapset_install_dir == ''
    tapset_install_dir = join_paths(get_option('datadir'), 'systemtap/tapset', host_machine.cpu_family())
  endif
  stp_cdata = configuration_data()
  stp_cdata.set('ABS_GLIB_RUNTIME_LIBDIR', glib_libdir)
  stp_cdata.set('LT_CURRENT', current)
  stp_cdata.set('LT_REVISION', interface_age)
endif

# Introspection
#
# We depend on g-ir-scanner from gobject-introspection-1.0 to scan our source
# to build GIR files. We then use gi-compile-repository from the local build
# tree to build the typelibs (rather than using g-ir-compiler from
# gobject-introspection). However, we use Meson’s gnome.generate_gir() function,
# which adds a hard dependency on gobject-introspection-1.0, so we’d better
# check for that here.
gobject_introspection_required_version = '>= 1.80.0'
gir_scanner = find_program('g-ir-scanner', required: get_option('introspection'), version: gobject_introspection_required_version)
gobject_introspection = dependency('gobject-introspection-1.0', required: get_option('introspection'), version: gobject_introspection_required_version)
enable_gir = get_option('introspection').allowed() and gir_scanner.found() and gobject_introspection.found() and meson.can_run_host_binaries()

if get_option('introspection').enabled() and not meson.can_run_host_binaries()
  error('Running binaries on the build host needs to be supported to build with -Dintrospection=enabled')
endif

gir_args = [
  '--quiet',
]
if gobject_introspection.version().version_compare('>=1.83.2')
  gir_args += [
    '--doc-format=gi-docgen',
  ]
endif

pkg = import('pkgconfig')
windows = import('windows')
gnome = import('gnome')

subdir('build-aux')
subdir('tools')
subdir('glib')
subdir('gobject')
subdir('gthread')
subdir('gmodule')
subdir('gio')
subdir('girepository')
subdir('fuzzing')
if get_option('tests')
  subdir('tests')
endif

# xgettext is optional (on Windows for instance)
if find_program('xgettext', required : get_option('nls')).found()
  subdir('po')
endif

# Install m4 macros that other projects use
install_data('m4macros/glib-2.0.m4', 'm4macros/glib-gettext.m4', 'm4macros/gsettings.m4',
  install_dir : get_option('datadir') / 'aclocal',
  install_tag : 'devel',
)

# Check whether we support overriding the invalid parameter handler on Windows for _get_osfhandle(),
# g_fsync() (i.e. _commit()), etc
if host_system == 'windows'
  if cc.has_function('_set_thread_local_invalid_parameter_handler', prefix: '#include <stdlib.h>')
    glib_conf.set('HAVE__SET_THREAD_LOCAL_INVALID_PARAMETER_HANDLER', 1)
  endif
  if cc.has_function('_set_invalid_parameter_handler', prefix: '#include <stdlib.h>')
    glib_conf.set('HAVE__SET_INVALID_PARAMETER_HANDLER', 1)
  endif
  if cc.has_header_symbol('crtdbg.h', '_CrtSetReportMode')
    glib_conf.set('HAVE__CRT_SET_REPORT_MODE', 1)
  endif
endif

configure_file(output : 'config.h', configuration : glib_conf)

rst2man = find_program('rst2man', 'rst2man.py', required: get_option('man-pages'))
if rst2man.found()
  rst2man_flags = [
    '--syntax-highlight=none',
  ]
  man1_dir = glib_prefix / get_option('mandir') / 'man1'
endif

rst2html5 = find_program('rst2html5', 'rst2html5.py', required: get_option('documentation'))

gnome = import('gnome')
subdir('docs/reference')

summary({
  'host cpu' : host_machine.cpu_family(),
  'host endian' : host_machine.endian(),
  'host system' : host_system,
  'C Compiler' : cc.get_id(),
  'C++ Compiler' : have_cxx ? cxx.get_id() : 'none',
  'shared build' : glib_build_shared,
  'static build' : glib_build_static,
}, section: 'Build environment')

if build_machine.system() != host_system
  summary({
    'build cpu' : build_machine.cpu_family(),
    'build endian' : build_machine.endian(),
    'build system' : build_machine.system(),
  }, section: 'Build environment')
endif

if linux_libc != ''
  summary({
    'linux_libc' : linux_libc
  }, section: 'Build environment')
endif

summary({
  'prefix' : glib_prefix,
  'bindir' : glib_bindir,
  'libexecdir' : glib_libexecdir,
  'pkgdatadir' : glib_pkgdatadir,
  'datadir' : glib_datadir,
  'localedir' : glib_localedir,
  'includedir' : glib_includedir,
  'giomodulesdir' : glib_giomodulesdir,
  'localstatedir' : glib_localstatedir,
  'runstatedir' : glib_runstatedir,
}, section: 'Directories')

if get_option('multiarch')
  summary({
    'multiarch bindir' : glib_bindir,
    'multiarch libexecdir' : glib_libexecdir,
  }, section: 'Directories')
endif

if enable_systemtap
  summary('tapset dir', get_option('tapset_install_dir'), section: 'Directories')
endif

if host_system == 'linux'
  summary({
    'selinux' : selinux_dep.found(),
    'libmount' : libmount_dep.found(),
  }, section: 'Options')
endif

summary({
  'xattr' : xattr_dep.length() > 0,
  'man-pages' : get_option('man-pages'),
  'dtrace' : enable_dtrace,
  'systemtap' : enable_systemtap,
  'sysprof' : libsysprof_capture_dep.found(),
  'documentation' : get_option('documentation'),
  'bsymbolic_functions' : get_option('bsymbolic_functions'),
  'force_posix_threads' : get_option('force_posix_threads'),
  'tests' : get_option('tests'),
  'installed_tests' : get_option('installed_tests'),
  'nls' : get_option('nls'),
  'oss_fuzz' : get_option('oss_fuzz'),
  'glib_debug' : get_option('glib_debug'),
  'glib_assert' : get_option('glib_assert'),
  'glib_checks' : get_option('glib_checks'),
  'libelf' : get_option('libelf'),
  'multiarch' : get_option('multiarch'),
  'introspection' : enable_gir,
  'file_monitor_backend' : get_option('file_monitor_backend'),
}, section: 'Options')
