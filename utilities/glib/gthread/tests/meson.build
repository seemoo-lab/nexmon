gthread_tests = {
  'init' : {},
}

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

test_deps = [thread_dep, libglib_dep, libgthread_dep, test_profile_dep]
test_cargs = ['-DG_LOG_DOMAIN="GLib-GThread"', '-UG_DISABLE_ASSERT', warning_sign_conversion_args, warning_shorten_64_to_32_args]
test_cpp_args = test_cargs

foreach test_name, extra_args : gthread_tests
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

  exe = executable(test_name, source,
    c_args : test_cargs + extra_args.get('c_args', []),
    cpp_args : test_cpp_args + extra_args.get('cpp_args', []),
    dependencies : test_deps + extra_args.get('dependencies', []),
    install_dir: installed_tests_execdir,
    install_tag: 'tests',
    install: install,
  )

  suite = ['gthread'] + extra_args.get('suite', [])
  timeout = suite.contains('slow') ? test_timeout_slow : test_timeout

  if extra_args.get('can_fail', false)
    suite += 'failing'
  endif

  test(test_name,
    exe,
    protocol : extra_args.get('protocol', test_protocol),
    env : test_env,
    timeout : timeout,
    suite : suite,
  )
endforeach

if have_bash and have_pkg_config
  test('gthread-2.0-pkg-config',
    bash,
    args: [
      '-xe', '-c',
      '\n'.join([
        'pkg-config --validate gthread-2.0',
        'test "$(pkg-config --modversion gthread-2.0)" = "@0@"'.format(glib_version),
        'test "$(pkg-config --variable=prefix gthread-2.0)" = "@0@"'.format(
          get_option('prefix')),
      ]),
    ],
    suite: ['gthread', 'no-valgrind', 'pkg-config'],
    env: {
      'PKG_CONFIG_PATH': meson.project_build_root() / 'meson-private',
    },
  )
endif
