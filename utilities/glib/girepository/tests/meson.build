# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2024 GNOME Foundation

girepository_tests = {
  'cmph-bdz': {
    'dependencies': [cmph_dep],
  },
  'dump' : {
    'export_dynamic': true,
  },
  'gthash' : {
    'dependencies': [girepo_gthash_dep],
  },
}

# Some tests require GIR files to have been generated
if enable_gir
  glib_gir_testing_dep = glib_gir
  if host_system == 'windows'
    glib_platform_gir_testing_dep = [
      glib_gir_testing_dep,
      glib_win32_gir,
    ]
  else
    glib_platform_gir_testing_dep = [
      glib_gir_testing_dep,
      glib_unix_gir,
    ]
  endif

  gobject_gir_testing_dep = [
    glib_platform_gir_testing_dep,
    gobject_gir,
  ]

  gmodule_gir_testing_dep = [
    glib_platform_gir_testing_dep,
    gmodule_gir,
  ]

  gio_gir_testing_dep = [
    gobject_gir_testing_dep,
    gmodule_gir_testing_dep,
    gio_gir,
  ]

  gio_platform_gir_testing_dep = [
    gio_gir_testing_dep,
    gio_platform_gir,
  ]

  girepository_gir_testing_dep = [
    gio_gir_testing_dep,
    girepository_gir,
  ]

  girepository_tests += {
    'callable-info' : {
      'dependencies': [libffi_dep],
      'depends': gio_gir_testing_dep,
    },
    'field-info' : {
      'depends': glib_platform_gir_testing_dep,
    },
    'function-info' : {
      'dependencies': [libffi_dep],
      'depends': glib_platform_gir_testing_dep,
    },
    'ir-parser' : {
      'dependencies': [libgirepository_internals_dep],
    },
    'object-info' : {
      'depends': gio_gir_testing_dep,
    },
    'registered-type-info' : {
      'depends': gobject_gir_testing_dep,
    },
    'repository' : {
      'depends': [gio_gir_testing_dep, gio_platform_gir_testing_dep],
      'dependencies': [libgio_dep],
    },
    'repository-search-paths' : {
      'c_args': '-DGOBJECT_INTROSPECTION_LIBDIR="@0@"'.format(glib_libdir),
      'depends': glib_platform_gir_testing_dep,
    },
    'struct-info' : {
      'depends': gobject_gir_testing_dep,
    },
    'throws' : {
      'depends': gio_gir_testing_dep,
    },
    'union-info' : {
      'depends': glib_platform_gir_testing_dep,
    },
  }

  if cc.get_id() != 'msvc'
    girepository_tests += {
      'autoptr-girepository' : {
        'dependencies': [libffi_dep],
        'source' : 'autoptr.c',
        'depends': gio_gir_testing_dep,
      },
    }
  endif
endif

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

test_deps = [libm, thread_dep, libgirepository_dep, test_profile_dep]
test_cargs = ['-DG_LOG_DOMAIN="GIRepository"', '-UG_DISABLE_ASSERT', warning_sign_conversion_args]
test_cpp_args = test_cargs

foreach test_name, extra_args : girepository_tests
  source = extra_args.get('source', test_name + '.c')
  install = installed_tests_enabled and extra_args.get('install', true)
  installed_tests_env = extra_args.get('installed_tests_env', {})

  if install
    test_conf = configuration_data()
    test_conf.set('installed_tests_dir', installed_tests_execdir)
    test_conf.set('program', test_name)
    test_env_override = ''
    if installed_tests_env != {}
      envs = []
      foreach var, value : installed_tests_env
        envs += '@0@=\'@1@\''.format(var, value)
      endforeach
      test_env_override = '@0@ @1@ '.format(env_program.full_path(), ' '.join(envs))
    endif
    test_conf.set('env', test_env_override)
    configure_file(
      input: installed_tests_template_tap,
      output: test_name + '.test',
      install_dir: installed_tests_metadir,
      install_tag: 'tests',
      configuration: test_conf
    )
  endif

  exe = executable(test_name, source, 'test-common.c',
    c_args: test_cargs + extra_args.get('c_args', []),
    cpp_args: test_cpp_args + extra_args.get('cpp_args', []),
    link_args: extra_args.get('link_args', []),
    override_options: extra_args.get('override_options', []),
    dependencies: test_deps + extra_args.get('dependencies', []),
    link_with: extra_args.get('link_with', []),
    export_dynamic: extra_args.get('export_dynamic', false),
    install_dir: installed_tests_execdir,
    install_tag: 'tests',
    install: install,
  )

  depends = [extra_args.get('depends', [])]
  suite = ['girepository', 'core'] + extra_args.get('suite', [])
  timeout = suite.contains('slow') ? test_timeout_slow : test_timeout

  if extra_args.get('can_fail', false)
    suite += 'failing'
  endif

  foreach program : extra_args.get('extra_programs', [])
    depends += test_extra_programs_targets[program]
  endforeach

  local_test_env = test_env
  foreach var, value : extra_args.get('env', {})
    local_test_env.append(var, value)
  endforeach

  test(test_name, exe,
    args: extra_args.get('args', []),
    protocol: extra_args.get('protocol', test_protocol),
    depends: depends,
    env: local_test_env,
    timeout: timeout,
    suite: suite,
    should_fail: extra_args.get('should_fail', false),
  )
endforeach

python_tests = {}

if enable_gir
  python_tests += {
    'gi-compile-repository.py': {
      'depends': [gicompilerepository, glib_gir[0], gobject_gir[0], gio_gir[0]],
      'env': {
        '_G_TEST_PROGRAM_RUNNER_PATH': fs.parent(gicompilerepository.full_path()),
      },
      'suite': ['compiler'],
    },
    'gi-inspect-typelib.py': {
      'depends': [giinspecttypelib, glib_gir[1], gobject_gir[1], gio_gir[1]],
      'env': {'_G_TEST_PROGRAM_RUNNER_PATH': fs.parent(giinspecttypelib.full_path())},
      'suite': ['inspector'],
    },
  }
endif

python_test_env = test_env
python_test_env.prepend('PYTHONPATH', python_test_libraries_path)
python_test_env.prepend('PYTHONPATH', python_test_libraries_built)
if python_test_env_common_path.length() > 0
  python_test_env.prepend('PATH', python_test_env_common_path)
endif

foreach test_name, extra_args : python_tests
  depends = [extra_args.get('depends', [])]
  suite = ['girepository', 'no-valgrind'] + extra_args.get('suite', [])

  if extra_args.get('can_fail', false)
    suite += 'failing'
  endif

  local_test_env = python_test_env
  foreach var, value : extra_args.get('env', {})
    local_test_env.append(var, value)
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
    installed_tests_env = extra_args.get('installed_tests_env', {})

    install_data(
      files(test_name),
      install_dir: installed_tests_execdir,
      install_tag: 'tests',
      install_mode: 'rwxr-xr-x',
    )

    test_conf = configuration_data()
    test_conf.set('installed_tests_dir', installed_tests_execdir)
    test_conf.set('program', test_name)

    test_env_override = ''
    installed_tests_env = extra_args.get('installed_tests_env', {})
    if installed_tests_env != {}
      envs = []
      foreach var, value : installed_tests_env
         envs += '@0@="@1@"'.format(var, value)
      endforeach
      test_env_override = '@0@ @1@ '.format(env_program.full_path(), ' '.join(envs))
    endif
    test_conf.set('env', test_env_override)

    configure_file(
      input: installed_tests_template_tap,
      output: test_name + '.test',
      install_dir: installed_tests_metadir,
      install_tag: 'tests',
      configuration: test_conf,
    )
  endif
endforeach
