# Just a skeleton lib for backwards compatibility since all the functionality
# has been moved into glib now

gthread_sources = ['gthread-impl.c']
if host_system == 'windows' and glib_build_shared
  gthread_rc_conf = configuration_data({
    'NAME': 'GThread',
    'FILENAME': (cc.get_argument_syntax() == 'msvc' ? '' : 'lib') + 'gthread-' + glib_api_version + '-' + soversion.to_string() + '.dll',
    'DESCRIPTION': 'Portable threading API',
  })
  gthread_rc_conf.merge_from(rc_conf_base)
  lib_rc = configure_file(
    input: lib_rc_in,
    output: 'gthread.rc',
    configuration: gthread_rc_conf,
  )
  gthread_sources += [windows.compile_resources(lib_rc,
    # Workaround for https://github.com/llvm/llvm-project/issues/63426
    args: ['-c', '65001'],
  )]
endif

libgthread = library('gthread-2.0',
  sources : gthread_sources,
  version : library_version,
  soversion : soversion,
  darwin_versions : darwin_versions,
  install : true,
  dependencies : [libglib_dep],
  c_args : ['-DG_LOG_DOMAIN="GThread"', glib_c_args_internal, warning_sign_conversion_args, warning_shorten_64_to_32_args],
  gnu_symbol_visibility : 'hidden',
  link_args : glib_link_flags,
)

pkg.generate(libgthread,
  libraries : [thread_dep],
  requires : ['glib-2.0'],
  version : glib_version,
  install_dir : glib_pkgconfigreldir,
  filebase : 'gthread-2.0',
  name : 'GThread',
  description : 'Thread support for GLib',
)

libgthread_dep = declare_dependency(link_with : libgthread)
meson.override_dependency('gthread-2.0', libgthread_dep)

if build_tests
  subdir('tests')
endif
