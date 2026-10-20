gdbus_example_objectmanager_visibility_h = custom_target('gdbus-example-objectmanager-visibility',
  output: 'gdbus-example-objectmanager-visibility.h',
  command: [gen_visibility_macros, '2.0', 'visibility-macros',
            'GDBUS_OBJECT_MANAGER_EXAMPLE', '@OUTPUT@'],
)

gdbus_example_objectmanager_xml = files('gdbus-example-objectmanager.xml')
gdbus_example_objectmanager_generated = custom_target('objectmanager-gen',
  input  : gdbus_example_objectmanager_xml,
  output : ['objectmanager-gen.h',
            'objectmanager-gen.c',
            'objectmanager-gen-org.gtk.GDBus.Example.ObjectManager.Animal.xml',
            'objectmanager-gen-org.gtk.GDBus.Example.ObjectManager.Cat.xml'],
  command : [python, gdbus_codegen,
             '--interface-prefix', 'org.gtk.GDBus.Example.ObjectManager.',
             '--c-namespace', 'Example',
             '--c-generate-object-manager',
             '--output-directory', '@OUTDIR@',
             '--generate-c-code', 'objectmanager-gen',
             '--generate-docbook', 'objectmanager-gen',
             '--symbol-decorator', 'GDBUS_OBJECT_MANAGER_EXAMPLE_AVAILABLE_IN_ALL',
             '--symbol-decorator-header', 'gdbus-example-objectmanager-visibility.h',
             '@INPUT@'],
  depend_files : gdbus_codegen_built_files,
  depends : gdbus_codegen_built_targets,
)

gdbus_example_objectmanager_rst_gen = custom_target('objectmanager-rst-gen',
  input: gdbus_example_objectmanager_xml,
  output: [
    'objectmanager-rst-gen-org.gtk.GDBus.Example.ObjectManager.Animal.rst',
    'objectmanager-rst-gen-org.gtk.GDBus.Example.ObjectManager.Cat.rst',
  ],
  command: [
    python,
    gdbus_codegen,
    '--interface-prefix', 'org.gtk.GDBus.Example.ObjectManager.',
    '--generate-rst', 'objectmanager-rst-gen',
    '--output-directory', '@OUTDIR@',
    '@INPUT@',
  ],
  depend_files: gdbus_codegen_built_files,
  depends: gdbus_codegen_built_targets,
)

extra_c_args = []
if glib_build_static_only
  extra_c_args = '-DGDBUS_OBJECT_MANAGER_EXAMPLE_STATIC_COMPILATION'
endif

libgdbus_example_objectmanager = library('gdbus-example-objectmanager',
    gdbus_example_objectmanager_generated, gdbus_example_objectmanager_visibility_h,
    c_args : [test_c_args, extra_c_args,
              '-DGDBUS_OBJECT_MANAGER_EXAMPLE_COMPILATION'],
    dependencies : [libglib_dep, libgmodule_dep, libgobject_dep, libgio_dep],
    install : installed_tests_enabled,
    install_dir : installed_tests_execdir)

libgdbus_example_objectmanager_dep = declare_dependency(
  sources : [
    gdbus_example_objectmanager_generated[0],
    gdbus_example_objectmanager_rst_gen[0],
  ],
  link_with : libgdbus_example_objectmanager,
  dependencies : [libgio_dep],
  compile_args : extra_c_args,
)
