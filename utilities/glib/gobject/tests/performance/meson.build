gobject_tests = {
  'performance' : {
    'args' : [ '--seconds', '0' ],
    # FIXME: https://gitlab.gnome.org/GNOME/glib/-/issues/3148
    'can_fail' : host_system == 'gnu',
  },
  'performance-threaded' : { 'args' : [ '--seconds', '0' ] },
}

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

test_deps = [libm, thread_dep, libglib_dep, libgobject_dep, test_profile_dep]
test_cargs = ['-DG_LOG_DOMAIN="GLib-GObject"', '-UG_DISABLE_ASSERT']

foreach test_name, extra_args : gobject_tests
  source = extra_args.get('source', test_name + '.c')
  install = installed_tests_enabled and extra_args.get('install', true)

  if install
    test_conf = configuration_data()
    test_conf.set('installed_tests_dir', installed_tests_execdir)
    test_conf.set('program', '@0@ --seconds 1'.format(test_name))
    test_conf.set('env', '')
    configure_file(
      input: installed_tests_template,
      output: test_name + '.test',
      install_dir: installed_tests_metadir,
      install_tag: 'tests',
      configuration: test_conf,
    )
  endif

  exe = executable(test_name, source,
    c_args : test_cargs + extra_args.get('c_args', []),
    dependencies : test_deps + extra_args.get('dependencies', []),
    install_dir: installed_tests_execdir,
    install_tag: 'tests',
    install: install,
  )

  suite = ['gobject', 'performance', 'no-valgrind'] + extra_args.get('suite', [])
  timeout = suite.contains('slow') ? test_timeout_slow : test_timeout
  args = extra_args.get('args', [])

  if extra_args.get('can_fail', false)
    suite += 'failing'
  endif

  test(test_name, exe,
    env : test_env,
    timeout : timeout,
    suite : suite,
    args : args,
  )
endforeach
