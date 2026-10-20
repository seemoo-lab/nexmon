glib_sources = []
glib_sub_includedir = glib_includedir / 'glib'

glibconfig_h = configure_file(input : 'glibconfig.h.in', output : 'glibconfig.h',
  install_dir : join_paths(get_option('libdir'), 'glib-2.0/include'),
  install_tag : 'devel',
  configuration : glibconfig_conf)
glib_sources += glibconfig_h

gversionmacros_h = custom_target(
  input: 'gversionmacros.h.in',
  output: 'gversionmacros.h',
  command: [gen_visibility_macros, meson.project_version(), 'versions-macros', '@INPUT@', '@OUTPUT@'],
  install: true,
  install_dir: glib_sub_includedir,
)

glib_visibility_h = custom_target(
  output: 'glib-visibility.h',
  command: [gen_visibility_macros, meson.project_version(), 'visibility-macros', 'GLIB', '@OUTPUT@'],
  install: true,
  install_dir: glib_sub_includedir,
)

glib_built_headers = [gversionmacros_h, glib_visibility_h]
glib_sources += glib_built_headers
glib_c_args_internal = [
  '-DGLIB_COMPILATION',
]

subdir('libcharset')

# libsysprof-capture support
libsysprof_capture_dep = dependency('sysprof-capture-4', version: '>= 3.38.0',
  required: get_option('sysprof'),
  default_options: [
    'examples=false',
    'gtk=false',
    'tests=false',
    'tools=false',
    'libsysprof=false',
    'sysprofd=none',
    'help=false',
  ],
  fallback: ['sysprof', 'libsysprof_capture_dep'],
)
glib_conf.set('HAVE_SYSPROF', libsysprof_capture_dep.found())

if use_system_printf
  gnulib_lib = []
  gnulib_libm_dependency = []
  glib_conf.set ('gl_unused', '')
  glib_conf.set ('gl_extern_inline', '')
else
  subdir ('gnulib/gl_extern_inline')
  subdir ('gnulib/gl_cv_long_double_equals_double')
  subdir ('gnulib/gl_cv_cc_double_expbit0')
  subdir ('gnulib/gl_cv_func_printf_precision')
  subdir ('gnulib/gl_cv_func_printf_enomem')
  subdir ('gnulib/gl_cv_func_printf_flag_zero')
  subdir ('gnulib/gl_cv_func_printf_flag_leftadjust')
  subdir ('gnulib/gl_cv_func_printf_flag_grouping')
  subdir ('gnulib/gl_cv_func_printf_directive_a')
  subdir ('gnulib/gl_cv_func_printf_directive_f')
  subdir ('gnulib/gl_cv_func_printf_directive_ls')
  subdir ('gnulib/gl_cv_func_printf_long_double')
  subdir ('gnulib/gl_cv_func_printf_infinite')
  subdir ('gnulib/gl_cv_func_printf_infinite_long_double')

  gl_unused = '''
/* Define as a marker that can be attached to declarations that might not
    be used.  This helps to reduce warnings, such as from
    GCC -Wunused-parameter.  */
#if __GNUC__ >= 3 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 7)
# define _GL_UNUSED __attribute__ ((__unused__))
#else
# define _GL_UNUSED
#endif
'''
  glib_conf.set ('gl_unused', gl_unused)
  glib_conf.set ('gl_extern_inline', gl_extern_inline)

  if (gl_cv_long_double_equals_double)
    glib_conf.set ('HAVE_SAME_LONG_DOUBLE_AS_DOUBLE', 1)
  endif

  if (gl_cv_cc_double_expbit0_word >= 0 and
      gl_cv_cc_double_expbit0_bit >= 0)
    glib_conf.set('DBL_EXPBIT0_WORD', gl_cv_cc_double_expbit0_word)
    glib_conf.set('DBL_EXPBIT0_BIT', gl_cv_cc_double_expbit0_bit)
  endif

  if not gl_cv_func_printf_precision
    glib_conf.set('NEED_PRINTF_UNBOUNDED_PRECISION', 1)
  endif

  if not gl_cv_func_printf_enomem
    glib_conf.set('NEED_PRINTF_ENOMEM', 1)
  endif

  if not gl_cv_func_printf_flag_leftadjust
    glib_conf.set('NEED_PRINTF_FLAG_LEFTADJUST', 1)
  endif

  if not gl_cv_func_printf_flag_zero
    glib_conf.set('NEED_PRINTF_FLAG_ZERO', 1)
  endif

  if not gl_cv_func_printf_flag_grouping
    glib_conf.set('NEED_PRINTF_FLAG_GROUPING', 1)
  endif

  if not gl_cv_func_printf_directive_a
    glib_conf.set('NEED_PRINTF_DIRECTIVE_A', 1)
  endif

  if not gl_cv_func_printf_directive_f
    glib_conf.set('NEED_PRINTF_DIRECTIVE_F', 1)
  endif

  if not gl_cv_func_printf_directive_ls
    glib_conf.set('NEED_PRINTF_DIRECTIVE_LS', 1)
  endif

  if (not gl_cv_func_printf_precision or
      not gl_cv_func_printf_enomem)
    glib_conf.set('NEED_PRINTF_DOUBLE', 1)
    glib_conf.set('NEED_PRINTF_LONG_DOUBLE', 1)
  endif

  if not gl_cv_func_printf_infinite
    glib_conf.set('NEED_PRINTF_INFINITE_DOUBLE', 1)
  endif

  if gl_cv_func_printf_long_double and gl_cv_func_printf_infinite_long_double
    glib_conf.set('NEED_PRINTF_INFINITE_LONG_DOUBLE', 1)
  endif

  subdir('gnulib')
endif

glib_headers = files(
  'glib.h',
  'glib-object.h',
)
install_headers(glib_headers, install_dir : glib_includedir)

if host_system != 'windows'
  glib_unix_headers = files('glib-unix.h')
  glib_unix_sources = files('glib-unix.c')
  install_headers(glib_unix_headers, install_dir : glib_includedir)
endif

if host_system == 'windows'
  install_headers([ 'msvc_recommended_pragmas.h' ], install_dir : glib_includedir)
endif

glib_deprecated_headers = files(
  'deprecated/gallocator.h',
  'deprecated/gcache.h',
  'deprecated/gcompletion.h',
  'deprecated/gmain.h',
  'deprecated/grel.h',
  'deprecated/gthread.h',
)
install_headers(glib_deprecated_headers,
  install_dir : glib_sub_includedir / 'deprecated')

glib_sub_headers = files(
  'glib-autocleanups.h',
  'glib-typeof.h',
  'galloca.h',
  'garray.h',
  'gasyncqueue.h',
  'gatomic.h',
  'gbacktrace.h',
  'gbase64.h',
  'gbitlock.h',
  'gbookmarkfile.h',
  'gbytes.h',
  'gcharset.h',
  'gchecksum.h',
  'gconvert.h',
  'gdataset.h',
  'gdate.h',
  'gdatetime.h',
  'gdir.h',
  'genviron.h',
  'gerror.h',
  'gfileutils.h',
  'ggettext.h',
  'ghash.h',
  'ghmac.h',
  'ghook.h',
  'ghostutils.h',
  'gi18n.h',
  'gi18n-lib.h',
  'giochannel.h',
  'gkeyfile.h',
  'glist.h',
  'gmacros.h',
  'gmain.h',
  'gmappedfile.h',
  'gmarkup.h',
  'gmem.h',
  'gmessages.h',
  'gnode.h',
  'goption.h',
  'gpathbuf.h',
  'gpattern.h',
  'gpoll.h',
  'gprimes.h',
  'gqsort.h',
  'gquark.h',
  'gqueue.h',
  'grand.h',
  'grcbox.h',
  'grefcount.h',
  'grefstring.h',
  'gregex.h',
  'gscanner.h',
  'gsequence.h',
  'gshell.h',
  'gslice.h',
  'gslist.h',
  'gspawn.h',
  'gstdio.h',
  'gstrfuncs.h',
  'gstrvbuilder.h',
  'gtestutils.h',
  'gstring.h',
  'gstringchunk.h',
  'gthread.h',
  'gthreadpool.h',
  'gtimer.h',
  'gtimezone.h',
  'gtrashstack.h',
  'gtree.h',
  'gtypes.h',
  'guuid.h',
  'gunicode.h',
  'guri.h',
  'gutils.h',
  'gvarianttype.h',
  'gvariant.h',
  'gversion.h',
  'gprintf.h',
)

install_headers(glib_sub_headers, install_dir : glib_sub_includedir)

if host_system == 'windows'
  glib_win32_headers = files('gwin32.h')
  glib_win32_sources = files('gwin32.c')
  install_headers(glib_win32_headers, install_dir : glib_sub_includedir)
endif

glib_deprecated_sources = files(
  'deprecated/gallocator.c',
  'deprecated/gcache.c',
  'deprecated/gcompletion.c',
  'deprecated/grel.c',
  'deprecated/gthread-deprecated.c'
)

glib_sources += files(
  'garcbox.c',
  'garray.c',
  'gasyncqueue.c',
  'gatomic.c',
  'gbacktrace.c',
  'gbase64.c',
  'gbitlock.c',
  'gbookmarkfile.c',
  'gbytes.c',
  'gcharset.c',
  'gchecksum.c',
  'gconvert.c',
  'gdataset.c',
  'gdate.c',
  'gdatetime.c',
  'gdatetime-private.c',
  'gdir.c',
  'genviron.c',
  'gerror.c',
  'gfileutils.c',
  'ggettext.c',
  'ghash.c',
  'ghmac.c',
  'ghook.c',
  'ghostutils.c',
  'giochannel.c',
  'gkeyfile.c',
  'glib-init.c',
  'glib-private.c',
  'glist.c',
  'gmain.c',
  'gmappedfile.c',
  'gmarkup.c',
  'gmem.c',
  'gmessages.c',
  'gnode.c',
  'goption.c',
  'gpathbuf.c',
  'gpattern.c',
  'gpoll.c',
  'gprimes.c',
  'gqsort.c',
  'gquark.c',
  'gqueue.c',
  'grand.c',
  'grcbox.c',
  'grefcount.c',
  'grefstring.c',
  'gregex.c',
  'gscanner.c',
  'gsequence.c',
  'gshell.c',
  'gslice.c',
  'gslist.c',
  'gspawn.c',
  'gstdio.c',
  'gstrfuncs.c',
  'gstring.c',
  'gstringchunk.c',
  'gstrvbuilder.c',
  'gtestutils.c',
  'gthread.c',
  'gthreadpool.c',
  'gtimer.c',
  'gtimezone.c',
  'gtrace.c',
  'gtranslit.c',
  'gtrashstack.c',
  'gtree.c',
  'guniprop.c',
  'gutf8.c',
  'gunibreak.c',
  'gunicollate.c',
  'gunidecomp.c',
  'guri.c',
  'gutils.c',
  'guuid.c',
  'gvariant.c',
  'gvariant-core.c',
  'gvariant-parser.c',
  'gvariant-serialiser.c',
  'gvarianttypeinfo.c',
  'gvarianttype.c',
  'gversion.c',
  'gwakeup.c',
  'gprint.c',
  'gprintf.c',
)

platform_deps = []

if host_system == 'windows'
  if glib_build_shared
    glib_rc_conf = configuration_data({
      'NAME': 'GLib',
      'FILENAME': (cc.get_argument_syntax() == 'msvc' ? '' : 'lib') + 'glib-' + glib_api_version + '-' + soversion.to_string() + '.dll',
      'DESCRIPTION': 'GLib is a general-purpose, portable utility library',
    })
    glib_rc_conf.merge_from(rc_conf_base)
    lib_rc = configure_file(
      input: lib_rc_in,
      output: 'glib.rc',
      configuration: glib_rc_conf,
    )
    glib_sources += [windows.compile_resources(lib_rc,
      # Workaround for https://github.com/llvm/llvm-project/issues/63426
      args: ['-c', '65001'],
    )]
  endif

  glib_sources += files('gwin32.c', 'gspawn-win32.c', 'giowin32.c')
  platform_deps = [winsock2, cc.find_library('winmm')]
  if cc.get_id() == 'msvc' or cc.get_id() == 'clang-cl'
    glib_sources += files('dirent/wdirent.c')
  endif
else
  glib_sources += files('glib-unix.c', 'giounix.c')
  if process_spawn_allowed
    glib_sources += files('gspawn-posix.c')
  else
    glib_sources += files('gspawn-unsupported.c')
  endif
  platform_deps = []
endif

if host_system == 'linux'
  glib_sources += files('gjournal-private.c')
endif

if glib_have_cocoa
  glib_sources += files('gosxutils.m')
  framework_dep = dependency('appleframeworks', modules : ['Foundation', 'CoreFoundation', 'AppKit'])
  platform_deps += [framework_dep]
endif

if glib_have_carbon
  framework_dep = dependency('appleframeworks', modules : 'Carbon')
  platform_deps += [framework_dep]
endif

if enable_dtrace
  glib_dtrace_obj = dtrace_obj_gen.process('glib_probes.d')
  glib_dtrace_hdr = dtrace_hdr_gen.process('glib_probes.d')
else
  glib_dtrace_obj = []
  glib_dtrace_hdr = []
endif

pcre2_static_args = []

if use_pcre2_static_flag
  pcre2_static_args = ['-DPCRE2_STATIC']
endif

glib_c_args = ['-DG_LOG_DOMAIN="GLib"'] + glib_c_args_internal + pcre2_static_args
libglib = library('glib-2.0',
  glib_dtrace_obj, glib_dtrace_hdr,
  sources : [glib_deprecated_sources, glib_sources],
  version : library_version,
  soversion : soversion,
  darwin_versions : darwin_versions,
  install : true,
  # intl.lib is not compatible with SAFESEH
  link_args : [noseh_link_args, glib_link_flags, win32_ldflags],
  include_directories : configinc,
  link_with: [charset_lib, gnulib_lib],
  dependencies : [
    atomic_dep,
    gnulib_libm_dependency,
    libiconv,
    libintl_deps,
    libm,
    librt,
    libsysprof_capture_dep,
    pcre2,
    platform_deps,
    thread_dep,
  ],
  c_args : glib_c_args,
  objc_args : glib_c_args,
  gnu_symbol_visibility : 'hidden',
)

libglib_dep = declare_dependency(
  link_with : libglib,
  dependencies : libintl_deps,
  sources : glib_built_headers,
  # We sadly need to export configinc here because everyone includes <glib/*.h>
  include_directories : [configinc, glibinc])

libglib_static_dep = declare_dependency(
  compile_args: ['-DGLIB_STATIC_COMPILATION'],
  objects: libglib.extract_all_objects(recursive: true),
  dependencies: [
    atomic_dep,
    gnulib_libm_dependency,
    libiconv,
    libintl_deps,
    libm,
    librt,
    libsysprof_capture_dep,
    pcre2,
    platform_deps,
    thread_dep,
  ],
  link_with: [charset_lib, gnulib_lib],
  sources: glib_built_headers,
  include_directories: [configinc, glibinc],
)

pkg.generate(libglib,
  libraries : [libintl_deps],
  libraries_private : [win32_ldflags],
  subdirs : ['glib-2.0'],
  extra_cflags : ['-I${libdir}/glib-2.0/include'] + win32_cflags,
  variables : [
    'glib_genmarshal=' + '${bindir}' / 'glib-genmarshal',
    'gobject_query=' + '${bindir}' / 'gobject-query',
    'glib_mkenums=' + '${bindir}' / 'glib-mkenums',
    'glib_valgrind_suppressions=' + '${datadir}' /
      valgrind_suppression_file_install_subdir /
      fs.name(valgrind_suppression_file),
  ],
  uninstalled_variables : [
    'glib_genmarshal=${prefix}/gobject/glib-genmarshal',
    'gobject_query=${prefix}/gobject/gobject-query',
    'glib_mkenums=${prefix}/gobject/glib-mkenums',
    'glib_valgrind_suppressions=' + valgrind_suppression_file.full_path(),
  ],
  version : glib_version,
  install_dir : glib_pkgconfigreldir,
  filebase : 'glib-2.0',
  name : 'GLib',
  description : 'C Utility Library',
)
meson.override_dependency('glib-2.0', libglib_dep)

# FIXME: This needs to be added to the `depends` of every invocation of
# glib_compile_resources in the build process. We can drop this when
# https://github.com/mesonbuild/meson/issues/13385 is fixed.
gspawn_helpers = []
# On Windows, glib needs a spawn helper for g_spawn* API
if host_system == 'windows'
  if host_machine.cpu_family() == 'x86'
    gspawn_helpers += executable('gspawn-win32-helper', 'gspawn-win32-helper.c',
      install : true,
      win_subsystem : 'windows',
      include_directories : configinc,
      dependencies : [libglib_dep, app_profile_dep])
    gspawn_helpers += executable('gspawn-win32-helper-console', 'gspawn-win32-helper.c',
      install : true,
      c_args : ['-DHELPER_CONSOLE'],
      include_directories : configinc,
      dependencies : [libglib_dep, app_profile_dep])
  else
    gspawn_helpers += executable('gspawn-win64-helper', 'gspawn-win32-helper.c',
      install : true,
      win_subsystem : 'windows',
      include_directories : configinc,
      dependencies : [libglib_dep, app_profile_dep])
    gspawn_helpers += executable('gspawn-win64-helper-console', 'gspawn-win32-helper.c',
      install : true,
      c_args : ['-DHELPER_CONSOLE'],
      include_directories : configinc,
      dependencies : [libglib_dep, app_profile_dep])
  endif
else
  gtester = executable('gtester', 'gtester.c',
    install : true,
    install_tag : 'bin-devel',
    c_args : ['-UG_DISABLE_ASSERT'],
    include_directories : configinc,
    dependencies : [libglib_dep, app_profile_dep])

  # Provide tools for others when we're a subproject and they use the Meson GNOME module
  meson.override_find_program('gtester', gtester)
endif

report_conf = configuration_data()
report_conf.set('GLIB_VERSION', glib_version)
report_conf.set('PYTHON', python_shebang)
configure_file(
  input: 'gtester-report.in',
  output: 'gtester-report',
  install_dir: get_option('bindir'),
  install_tag : 'bin-devel',
  configuration: report_conf,
  install_mode: 'rwxr-xr-x'
)

install_data('glib_gdb.py',
  install_dir : glib_pkgdatadir / 'gdb',
  install_tag : 'devel',
)

# This is needed to make gdb find glib_gdb.py
env = environment()
env.prepend('PYTHONPATH', meson.current_source_dir())
meson.add_devenv(env)

gdb_conf = configuration_data()
gdb_conf.set('datadir', glib_datadir)
# This is also used in gobject/meson.build
if host_system != 'windows'
  gdb_install = true
else
  # FIXME: Cannot install on Windows because the path will contain a drive
  # letter and colons are not allowed in paths.
  gdb_install = false
endif

# XXX: We add a leading './' because glib_libdir is an absolute path and we
# need it to be a relative path so that join_paths appends it to the end.
gdb_install_dir = join_paths(glib_datadir, 'gdb', 'auto-load', './' + glib_libdir)

configure_file(
  input: 'libglib-gdb.py.in',
  output: 'libglib-2.0.so.@0@-gdb.py'.format(library_version),
  configuration: gdb_conf,
  install_dir: gdb_install_dir,
  install_tag: 'devel',
  install: gdb_install,
)

if enable_systemtap
  glib_stp = configure_file(input : 'glib.stp.in',
    output : '@0@.stp'.format(libglib.full_path().split('/').get(-1)),
    configuration : stp_cdata,
    install_dir : tapset_install_dir,
    install_tag : 'systemtap',
  )
endif

if build_tests
  subdir('tests')
endif
