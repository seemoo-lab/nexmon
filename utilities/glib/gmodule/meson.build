gmoduleconf_conf = configuration_data()

g_module_need_uscore = 0
g_module_broken_rtld_global = 0
g_module_have_dlerror = 0

g_module_impl = ''

g_module_includedir = glib_includedir / 'gmodule'

# On Windows force native WIN32 shared lib loader
if host_system == 'windows'
  g_module_impl = 'G_MODULE_IMPL_WIN32'
# Force native AIX library loader
# dlopen() filepath must be of the form /path/libname.a(libname.so)
elif host_system == 'aix'
  g_module_impl = 'G_MODULE_IMPL_AR'
elif have_dlopen_dlsym
  g_module_impl = 'G_MODULE_IMPL_DL'
endif

# additional checks for G_MODULE_IMPL_DL
if g_module_impl == 'G_MODULE_IMPL_DL'
  # FIXME: check for OSF1/5.0 RTLD_GLOBAL brokenness (is this still relevant?)

  # Check whether we need preceding underscores
  if cc.get_id() == 'msvc' or cc.get_id() == 'clang-cl'
    message('Building for MSVC: assuming that symbols are prefixed with underscore')
    g_module_need_uscore = 1
  elif meson.can_run_host_binaries()
    rres = cc.run(dlopen_dlsym_test_code,
                  dependencies : libdl_dep,
                  name : 'dlsym() preceding underscores')
    if host_system == 'windows' or (rres.compiled() and
      rres.returncode() == 0 and rres.stdout().to_int() == 0)
      g_module_need_uscore = 1
    endif
  else
    message('Cross-compiling: assuming that symbols aren\'t prefixed with underscore')
    g_module_need_uscore = 0
  endif

  if cc.has_function('dlerror', dependencies : libdl_dep)
    g_module_have_dlerror = 1
  endif
endif

# Done, have we got an implementation?
if g_module_impl == ''
  g_module_impl = '0'
  message('WARNING: No suitable GModule implementation found!')
endif

gmoduleconf_conf.set('G_MODULE_IMPL', g_module_impl)
gmoduleconf_conf.set('G_MODULE_SUPPORTED', g_module_impl != '0')
gmoduleconf_conf.set('G_MODULE_HAVE_DLERROR', g_module_have_dlerror)
gmoduleconf_conf.set('G_MODULE_NEED_USCORE', g_module_need_uscore)
gmoduleconf_conf.set('G_MODULE_BROKEN_RTLD_GLOBAL', g_module_broken_rtld_global)

gmoduleconf_h = configure_file(input : 'gmoduleconf.h.in',
                               output : 'gmoduleconf.h',
                               configuration : gmoduleconf_conf)

# Expose as variable to be used by gobject-introspection
# when it includes GLib as a subproject
gmodule_h = files('gmodule.h')
gmodule_c = files('gmodule.c')
gmodule_deprecated_c = files('gmodule-deprecated.c')

install_headers([gmodule_h], install_dir : glib_includedir)

gmodule_visibility_h = custom_target(
  output: 'gmodule-visibility.h',
  command: [gen_visibility_macros, meson.project_version(), 'visibility-macros', 'GMODULE', '@OUTPUT@'],
  install: true,
  install_dir: g_module_includedir,
)

gmodule_sources = [gmodule_c, gmodule_visibility_h, gmodule_deprecated_c]

if host_system == 'windows' and glib_build_shared
  gmodule_rc_conf = configuration_data({
    'NAME': 'GModule',
    'FILENAME': (cc.get_argument_syntax() == 'msvc' ? '' : 'lib') + 'gmodule-' + glib_api_version + '-' + soversion.to_string() + '.dll',
    'DESCRIPTION': 'Portable API for dynamically loading modules',
  })
  gmodule_rc_conf.merge_from(rc_conf_base)
  lib_rc = configure_file(
    input: lib_rc_in,
    output: 'gmodule.rc',
    configuration: gmodule_rc_conf,
  )
  gmodule_sources += [windows.compile_resources(lib_rc,
    # Workaround for https://github.com/llvm/llvm-project/issues/63426
    args: ['-c', '65001'],
  )]
endif

libgmodule = library('gmodule-2.0',
  sources : gmodule_sources,
  version : library_version,
  soversion : soversion,
  darwin_versions : darwin_versions,
  install : true,
  include_directories : [configinc, gmoduleinc],
  dependencies : [libdl_dep, libglib_dep],
  c_args : ['-DG_LOG_DOMAIN="GModule"', '-DGMODULE_COMPILATION', warning_sign_conversion_args],
  gnu_symbol_visibility : 'hidden',
  link_args : [glib_link_flags],
)

supported_var = 'gmodule_supported=@0@'.format(g_module_impl != '0')

pkg.generate(libgmodule,
  libraries : [thread_dep],
  requires : ['glib-2.0'],
  version : glib_version,
  variables : [supported_var],
  install_dir : glib_pkgconfigreldir,
  filebase : 'gmodule-no-export-2.0',
  name : 'GModule',
  description : 'Dynamic module loader for GLib',
)

pkg.generate(libraries : [libgmodule, export_dynamic_ldflags],
  requires : ['glib-2.0'],
  version : glib_version,
  variables : [supported_var],
  install_dir : glib_pkgconfigreldir,
  filebase : 'gmodule-export-2.0',
  name : 'GModule',
  description : 'Dynamic module loader for GLib',
)

pkg.generate(libraries : [libgmodule, export_dynamic_ldflags],
  requires : ['glib-2.0'],
  version : glib_version,
  variables : [supported_var],
  install_dir : glib_pkgconfigreldir,
  filebase : 'gmodule-2.0',
  name : 'GModule',
  description : 'Dynamic module loader for GLib',
)

gmodule_inc_dep = declare_dependency(
  include_directories: [gmoduleinc],
  sources: [gmodule_visibility_h],
)

libgmodule_dep = declare_dependency(link_with : libgmodule,
  dependencies : [libglib_dep, gmodule_inc_dep])

meson.override_dependency('gmodule-no-export-2.0', libgmodule_dep)
meson.override_dependency('gmodule-export-2.0', libgmodule_dep)
meson.override_dependency('gmodule-2.0', libgmodule_dep)

if build_tests
  subdir('tests')
endif
