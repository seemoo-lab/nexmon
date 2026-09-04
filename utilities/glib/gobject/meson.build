gobject_includedir = glib_includedir / 'gobject'

gobject_install_headers = files(
  'gobject-autocleanups.h',
  'glib-types.h',
  'gbinding.h',
  'gbindinggroup.h',
  'gboxed.h',
  'gclosure.h',
  'genums.h',
  'gmarshal.h',
  'gobject.h',
  'gparam.h',
  'gparamspecs.h',
  'gsignal.h',
  'gsignalgroup.h',
  'gsourceclosure.h',
  'gtype.h',
  'gtypemodule.h',
  'gtypeplugin.h',
  'gvalue.h',
  'gvaluearray.h',
  'gvaluecollector.h',
  'gvaluetypes.h',
  'gobjectnotifyqueue.c', # sic
)

gobject_sources = []

gobject_visibility_h = custom_target(
  output: 'gobject-visibility.h',
  command: [gen_visibility_macros, meson.project_version(), 'visibility-macros', 'GOBJECT', '@OUTPUT@'],
  install: true,
  install_dir: gobject_includedir,
)
gobject_sources += gobject_visibility_h

install_headers(gobject_install_headers, install_dir : gobject_includedir)

gobject_sources += files(
  'gatomicarray.c',
  'gbinding.c',
  'gbindinggroup.c',
  'gboxed.c',
  'gclosure.c',
  'genums.c',
  'gmarshal.c',
  'gobject.c',
  'gparam.c',
  'gparamspecs.c',
  'gsignal.c',
  'gsignalgroup.c',
  'gsourceclosure.c',
  'gtype.c',
  'gtypemodule.c',
  'gtypeplugin.c',
  'gvalue.c',
  'gvaluearray.c',
  'gvaluetransform.c',
  'gvaluetypes.c',
)

if host_system == 'windows' and glib_build_shared
  gobject_rc_conf = configuration_data({
    'NAME': 'GObject',
    'FILENAME': (cc.get_argument_syntax() == 'msvc' ? '' : 'lib') + 'gobject-' + glib_api_version + '-' + soversion.to_string() + '.dll',
    'DESCRIPTION': 'The base type system and object class',
  })
  gobject_rc_conf.merge_from(rc_conf_base)
  lib_rc = configure_file(
    input: lib_rc_in,
    output: 'gobject.rc',
    configuration: gobject_rc_conf,
  )
  gobject_sources += [windows.compile_resources(lib_rc,
    # Workaround for https://github.com/llvm/llvm-project/issues/63426
    args: ['-c', '65001'],
  )]
endif

if enable_dtrace
  gobject_dtrace_obj = dtrace_obj_gen.process('gobject_probes.d')
  gobject_dtrace_hdr = dtrace_hdr_gen.process('gobject_probes.d')
else
  gobject_dtrace_obj = []
  gobject_dtrace_hdr = []
endif

python_tools = [
  'glib-genmarshal',
  'glib-mkenums',
]

python_tools_conf = configuration_data()
python_tools_conf.set('VERSION', glib_version)
python_tools_conf.set('PYTHON', python_shebang)

foreach tool: python_tools
  tool_bin = configure_file(
    input : tool + '.in',
    output : tool,
    configuration : python_tools_conf,
    install_dir : glib_bindir,
    install_tag : 'bin-devel',
  )

  # Set variables for later use
  set_variable(tool.underscorify(), tool_bin)
  # Provide tools for others when we're a subproject and they use the Meson GNOME module
  meson.override_find_program(tool, tool_bin)
endforeach

# Generate a header file containing the GObject enum types for the enums defined
# in libglib.
#
# For now, we only include gunicode.h here, since GScriptType is needed for
# Pango. More headers can be added as needed in future.
#
# We can't use gnome.mkenums() because the GNOME module looks for glib-mkenums
# in PATH, which means you can't bootstrap glib with its own glib-mkenums.
glib_enumtypes_input_headers = files(
  '../glib/gunicode.h',
)

glib_enumtypes_h = custom_target('glib_enumtypes_h',
  output : 'glib-enumtypes.h',
  capture : true,
  input : glib_enumtypes_input_headers,
  install : true,
  install_dir : join_paths(get_option('includedir'), 'glib-2.0/gobject'),
  install_tag: 'devel',
  command : [python, glib_mkenums,
             '--template', files('glib-enumtypes.h.template'),
             '@INPUT@'])

glib_enumtypes_c = custom_target('glib_enumtypes_c',
  output : 'glib-enumtypes.c',
  capture : true,
  input : glib_enumtypes_input_headers,
  depends : [glib_enumtypes_h],
  command : [python, glib_mkenums,
             '--template', files('glib-enumtypes.c.template'),
             '@INPUT@'])

# Expose as variable to be used by gobject-introspection
# when it includes GLib as a subproject
glib_types_h = files('glib-types.h')

libgobject = library('gobject-2.0',
  gobject_dtrace_obj, gobject_dtrace_hdr, glib_enumtypes_h, glib_enumtypes_c,
  sources : gobject_sources,
  version : library_version,
  soversion : soversion,
  darwin_versions : darwin_versions,
  install : true,
  include_directories : [configinc],
  dependencies : [libffi_dep, libglib_dep],
  c_args : ['-DG_LOG_DOMAIN="GLib-GObject"', '-DGOBJECT_COMPILATION', warning_sign_conversion_args, warning_shorten_64_to_32_args],
  gnu_symbol_visibility : 'hidden',
  link_args : glib_link_flags,
)

pkg.generate(libgobject,
  requires : ['glib-2.0'],
  version : glib_version,
  install_dir : glib_pkgconfigreldir,
  filebase : 'gobject-2.0',
  name : 'GObject',
  description : 'GLib Type, Object, Parameter and Signal Library',
)

libgobject_dep = declare_dependency(link_with : libgobject,
  include_directories : [gobjectinc],
  sources : [gobject_visibility_h, glib_enumtypes_h],
  dependencies : [libglib_dep],
)
meson.override_dependency('gobject-2.0', libgobject_dep)

gobject_query = executable('gobject-query', 'gobject-query.c',
  install : true,
  install_tag : 'bin-devel',
  dependencies : [libglib_dep, libgobject_dep, app_profile_dep])

install_data('gobject_gdb.py',
  install_dir : glib_pkgdatadir / 'gdb',
  install_tag : 'devel',
)
gdb_conf = configuration_data()
gdb_conf.set('datadir', glib_datadir)
configure_file(
  input: 'libgobject-gdb.py.in',
  output: 'libgobject-2.0.so.@0@-gdb.py'.format(library_version),
  configuration: gdb_conf,
  install_dir: gdb_install_dir,
  install_tag: 'devel',
  install: gdb_install,
)

# This is needed to make gdb find gobject_gdb.py
env = environment()
env.prepend('PYTHONPATH', meson.current_source_dir())
meson.add_devenv(env)

if enable_systemtap
  gobject_stp = configure_file(input : 'gobject.stp.in',
    output : '@0@.stp'.format(libgobject.full_path().split('/').get(-1)),
    configuration : stp_cdata,
    install_dir : tapset_install_dir,
    install_tag : 'systemtap',
  )
endif

if build_tests
  subdir('tests')
endif
