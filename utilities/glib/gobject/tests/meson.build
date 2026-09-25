subdir('performance')

marshalers_h = custom_target('marshalers_h',
  output : 'marshalers.h',
  input : 'marshalers.list',
  command : [
    python, glib_genmarshal,
    '--prefix=test',
    '--valist-marshallers',
    '--output=@OUTPUT@',
    '--quiet',
    '--header',
    '@INPUT@',
  ],
)
marshalers_c = custom_target('marshalers_c',
  output : 'marshalers.c',
  input : 'marshalers.list',
  command : [
    python, glib_genmarshal,
    '--prefix=test',
    '--valist-marshallers',
    '--include-header=marshalers.h',
    '--output=@OUTPUT@',
    '--quiet',
    '--body',
    '@INPUT@',
  ],
)

gobject_tests = {
  'notify-init' : {},
  'notify-init2' : {},
  'custom-dispatch' : {},
  'qdata' : {},
  'accumulator' : {
    'source' : ['accumulator.c', marshalers_h, marshalers_c],
  },
  'basics-gobject' : {},
  'boxed' : {},
  'defaultiface' : {
    'source' : ['defaultiface.c', 'testmodule.c'],
  },
  'deftype' : { 'protocol': 'exitcode' },
  'deprecated-properties' : {},
  'dynamictype' : {
    'source' : ['dynamictype.c', 'testmodule.c'],
  },
  'enums' : {},
  'max-version' : {'install': false},
  'override' : {},
  'param' : {},
  'references' : {},
  'basic-signals' : {},
  'singleton' : {},
  'threadtests' : {},
  'dynamictests' : {},
  'binding' : {},
  'bindinggroup' : {},
  'properties' : {},
  'properties-introspection' : {},
  'reference' : {
    # FIXME: https://gitlab.gnome.org/GNOME/glib/-/issues/3148
    'can_fail' : host_system == 'gnu',
  },
  'flags' : {},
  'value' : {},
  'type' : {},
  'gobject-private' : {
    'source' : 'private.c',
  },
  'closure' : {},
  'closure-refcount' : { 'suite': ['slow'] },
  'object' : {},
  'signal-handler' : {},
  'ifaceproperties' : {},
  'signals' : {
    'source' : ['signals.c', marshalers_h, marshalers_c],
  },
  'signalgroup' : {},
  'testing' : {},
  'type-flags' : {},
  'objects-refcount1' : {
    'c_args': cc.get_supported_arguments([
      '-DG_DISABLE_CAST_CHECKS',
      '-Werror',
      '-Wcast-align=strict',
    ]),
  },
  'objects-refcount2' : {'suite' : ['slow']},
  'properties-refcount1' : {},
  'properties-refcount2' : {'suite' : ['slow']},
  'properties-refcount3' : {'suite' : ['slow']},
  'properties-refcount4' : {},
  'signals-refcount1' : {
    'source' : 'signals-refcount.c',
    'c_args' : ['-DTESTNUM=1'],
  },
  'signals-refcount2' : {
    'source' : 'signals-refcount.c',
    'c_args' : ['-DTESTNUM=2'],
  },
  'signals-refcount3' : {
    'source' : 'signals-refcount.c',
    'c_args' : ['-DTESTNUM=3'],
  },
  'signals-refcount4' : {
    'source' : 'signals-refcount.c',
    'c_args' : ['-DTESTNUM=4'],
  },
}

if have_cxx
  gobject_tests += {
    'cxx' : {
      'protocol': 'exitcode',
      'source' : ['cxx.cpp'],
      'suite' : ['cpp'],
    },
  }

  foreach std, arg: cxx_standards
    gobject_tests += {
      'cxx-@0@'.format(std) : {
        'protocol': 'exitcode',
        'source' : ['cxx.cpp'],
        'suite' : ['cpp'],
        'cpp_args' : [arg],
      },
    }
  endforeach
endif

if cc.get_id() != 'msvc'
  gobject_tests += {'autoptr' : {}}
endif

python_tests = {
  'genmarshal.py' : {
    'env': {'_G_TEST_PROGRAM_RUNNER_PATH': fs.parent(glib_genmarshal.full_path())},
  },
  'gobject-query.py' : {
    'depends' : gobject_query,
    'env': {'_G_TEST_PROGRAM_RUNNER_PATH': fs.parent(gobject_query.full_path())},
    'can_fail' : host_system == 'windows',
  },
  'mkenums.py' : {
    'env': {'_G_TEST_PROGRAM_RUNNER_PATH': fs.parent(glib_mkenums.full_path())},
  },
}

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

test_deps = [libm, thread_dep, libglib_dep, libgobject_dep, test_profile_dep]
test_cargs = ['-DG_LOG_DOMAIN="GLib-GObject"', '-UG_DISABLE_ASSERT', warning_sign_conversion_args]
test_cpp_args = test_cargs

foreach test_name, extra_args : gobject_tests
  source = extra_args.get('source', test_name + '.c')
  install = installed_tests_enabled and extra_args.get('install', true)

  if install
    test_conf = configuration_data()
    test_conf.set('installed_tests_dir', installed_tests_execdir)
    test_conf.set('program', test_name)
    test_conf.set('env', '')
    configure_file(
      input: installed_tests_template_tap,
      output: test_name + '.test',
      install_dir: installed_tests_metadir,
      install_tag: 'tests',
      configuration: test_conf
    )
  endif

  c_args = test_cargs + extra_args.get('c_args', [])
  if 'cpp' not in extra_args.get('suite', [])
    c_args += [warning_shorten_64_to_32_args]
  endif

  exe = executable(test_name, source,
    c_args : c_args,
    cpp_args : test_cpp_args + extra_args.get('cpp_args', []),
    dependencies : test_deps + extra_args.get('dependencies', []),
    install_dir: installed_tests_execdir,
    install_tag: 'tests',
    install: install,
  )

  suite = ['gobject'] + extra_args.get('suite', [])
  timeout = suite.contains('slow') ? test_timeout_slow : test_timeout

  if extra_args.get('can_fail', false)
    suite += 'failing'
  endif

  # FIXME: https://gitlab.gnome.org/GNOME/glib/issues/1316
  # aka https://bugs.debian.org/880883
  if test_name == 'closure-refcount' and ['arm', 'aarch64'].contains(host_machine.cpu_family())
    timeout = timeout * 10
  endif

  test(test_name,
    exe,
    protocol : extra_args.get('protocol', test_protocol),
    env : test_env,
    timeout : timeout,
    suite : suite,
  )
endforeach

python_test_env = test_env
python_test_env.prepend('PYTHONPATH', python_test_libraries_path)
python_test_env.prepend('PYTHONPATH', python_test_libraries_built)
if python_test_env_common_path.length() > 0
  python_test_env.prepend('PATH', python_test_env_common_path)
endif

foreach test_name, extra_args : python_tests
  depends = [extra_args.get('depends', [])]
  suite = ['gobject', 'no-valgrind']

  if extra_args.get('can_fail', false)
    suite += 'failing'
  endif

  local_test_env = python_test_env
  foreach var, value : extra_args.get('env', {})
    local_test_env.set(var, value)
  endforeach

  test(
    test_name,
    python,
    protocol : extra_args.get('protocol', test_protocol),
    depends: depends,
    args: ['-B', files(test_name)],
    env: local_test_env,
    suite: suite,
  )

  if installed_tests_enabled
    install_data(
      files(test_name),
      install_dir: installed_tests_execdir,
      install_tag: 'tests',
      install_mode: 'rwxr-xr-x',
    )

    test_conf = configuration_data()
    test_conf.set('installed_tests_dir', installed_tests_execdir)
    test_conf.set('program', test_name)
    test_conf.set('env', '')
    configure_file(
      input: installed_tests_template_tap,
      output: test_name + '.test',
      install_dir: installed_tests_metadir,
      install_tag: 'tests',
      configuration: test_conf,
    )
  endif
endforeach
