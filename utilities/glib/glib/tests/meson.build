glib_tests = {
  'array-test' : {},
  'asyncqueue' : {},
  'atomic' : {
    'c_args' : cc.get_id() == 'gcc' ? ['-Wstrict-aliasing=2'] : [],
    'c_standards': c_standards.keys(),
  },
  'base64' : {},
  'bitlock' : {},
  'bookmarkfile' : {},
  'bytes' : {},
  'cache' : {},
  'charset' : {},
  'checksum' : {},
  'collate' : {
    # musl: collate fail due to missing collation support in musl libc
    # From https://wiki.musl-libc.org/roadmap#Open_future_goals
    # "LC_COLLATE support for collation orders other than simple codepoint order"
    'can_fail' : linux_libc == 'musl',
  },
  'completion' : {},
  'cond' : {},
  'convert' : {
    # FIXME: musl: /conversion/illegal-sequence: https://gitlab.gnome.org/GNOME/glib/-/issues/3182
    'can_fail' : linux_libc == 'musl',
  },
  'dataset' : {},
  'date' : {
    # FIXME: darwin: https://gitlab.gnome.org/GNOME/glib/-/issues/1392
    # musl: /date/month_substring should be skipped but it's not. The fix should
    # be on musl side: https://www.openwall.com/lists/musl/2023/08/10/3
    # FIXME: musl: /date/strftime: https://gitlab.gnome.org/GNOME/glib/-/issues/3171
    'can_fail' : host_system == 'darwin' or linux_libc == 'musl',
  },
  'dir' : {},
  'environment' : {},
  'error' : {},
  'fileutils' : {},
  'gdatetime' : {
    'source' : ['gdatetime.c', '../gdatetime-private.c'],
    'suite' : ['slow'],
    # musl: GDateTime/format_mixed/non_utf8_time_non_utf8_messages should be
    # skipped but it's not. The fix should be on musl side:
    # https://www.openwall.com/lists/musl/2023/08/10/3
    'can_fail' : host_system == 'windows' or linux_libc == 'musl',
  },
  'guuid' : {},
  'gvariant' : {
    'suite' : ['slow'],
  },
  'gwakeup' : {
    'source' : ['gwakeuptest.c', '../gwakeup.c'],
    'install' : false,
  },
  'hash' : {},
  'hmac' : {},
  'hook' : {},
  'hostutils' : {},
  'io-channel-basic' : {},
  'io-channel' : {},
  'keyfile' : {},
  'list' : {},
  'logging' : {},
  'macros' : {
    'c_standards': c_standards.keys(),
  },
  'mainloop' : {},
  'mappedfile' : {},
  'mapping' : {},
  'markup' : {},
  'markup-parse' : {},
  'markup-collect' : {},
  'markup-escape' : {},
  'markup-subparser' : {},
  'max-version' : {'install' : false},
  'memchunk' : {},
  'mem-overflow' : {
    'link_args' : cc.get_id() == 'gcc' and cc.version().version_compare('> 6')
      ? ['-Wno-alloc-size-larger-than'] : [],
  },
  'monotonic-time' : {},
  'mutex' : {},
  'node' : {},
  'once' : {},
  'onceinit' : {},
  'option-context' : {
    # musl: /option/arg/repetition/locale should be skipped but it's not. The
    # fix should be on musl side: https://www.openwall.com/lists/musl/2023/08/10/3
    'can_fail' : linux_libc == 'musl',
  },
  'option-argv0' : {},
  'overflow' : {},
  'overflow-fallback' : {
    'source' : 'overflow.c',
    'c_args' : ['-D_GLIB_TEST_OVERFLOW_FALLBACK'],
  },
  'pathbuf' : {},
  'pattern' : {},
  'print' : {
    'can_fail' : host_system == 'windows',
  },
  'private' : {},
  'protocol' : {},
  'queue' : {},
  'rand' : {},
  'rcbox' : {},
  'rec-mutex' : {},
  'refcount' : {},
  'refcount-macro' : {
    'source' : 'refcount.c',
    'c_args' : ['-DG_DISABLE_CHECKS'],
  },
  'refstring' : {},
  'regex' : {
    'dependencies' : [pcre2],
    'c_args' : use_pcre2_static_flag ? ['-DPCRE2_STATIC'] : [],
  },
  'relation' : {},
  'rwlock' : {},
  'scannerapi' : {},
  'search-utils' : {},
  'sequence' : {
    'suite' : ['slow'],
  },
  'shell' : {},
  'slice' : {},
  'slist' : {},
  'sort' : {},
  'spawn-multithreaded' : {
    'can_fail': glib_build_static and host_system == 'windows',
    'suite': host_system == 'windows' ? ['flaky'] : [],
    'extra_programs' : ['test-spawn-echo'] + (
      host_machine.system() == 'windows' ? ['test-spawn-sleep'] : []),
  },
  'spawn-path-search' : {
    'extra_programs' : [
      'spawn-path-search-helper',
      'spawn-test-helper',
      'spawn-test-helper-subdir',
    ],
  },
  'spawn-singlethread' : {
    'dependencies' : [winsock2],
    'extra_programs' : ['test-spawn-echo'],
    # FIXME: https://gitlab.gnome.org/GNOME/glib/-/issues/3148
    'can_fail' : host_system == 'gnu',
  },
  'spawn-test' : {
    'extra_programs' : host_machine.system() == 'windows' ? ['spawn-test-win32-gui'] : [],
    # FIXME: https://gitlab.gnome.org/GNOME/glib/-/issues/3148
    'can_fail' : host_system == 'gnu',
  },
  'strfuncs' : {},
  'string' : {
    'c_args' : cc.get_id() == 'gcc' ? ['-Werror=sign-conversion'] : [],
  },
  'strvbuilder' : {},
  'testing' : {
    'args': [ '--verbose' ],
    'extra_programs' : ['testing-helper'],
    'c_args' : cc.get_id() == 'gcc' ? ['-Werror=sign-conversion'] : [],
  },
  'testing-nonfatal' : {
    'protocol' : 'exitcode'
  },
  'test-printf' : {},
  'thread' : {},
  'thread-deprecated' : {},
  'thread-pool' : {},
  'thread-pool-slow' : {'suite' : ['slow']},
  'timeout' : {},
  'timer' : {},
  'tree' : {},
  'types' : {},
  'utf8-performance' : {},
  'utf8-pointer' : {
    'c_args' : cc.get_id() == 'gcc' ? ['-Werror=cast-qual'] : [],
  },
  'utf8-private' : {},
  'utf8-validate' : {},
  'utf8-misc' : {},
  'utils' : {
    'c_standards': c_standards.keys(),
  },
  'utils-isolated' : {},
  'utils-unisolated' : {},
  'unicode' : {},
  'unicode-encoding' : {},
  'unicode-normalize': {},
  'uri' : {},
  '1bit-mutex' : {},
  '1bit-emufutex' : {
    'source' : '1bit-mutex.c',
    'c_args' : ['-DTEST_EMULATED_FUTEX'],
    'install' : false,
    'suite' : ['slow'],
  },
  '642026' : {
    'suite' : ['slow'],
  },
  '642026-ec' : {
    'source' : '642026.c',
    'c_args' : ['-DG_ERRORCHECK_MUTEXES'],
    'suite' : ['slow'],
  },
}

if have_cxx
  glib_tests += {
    'cxx' : {
      'source' : ['cxx.cpp'],
      'suite': ['cpp'],
      'dependencies' : [wincodecs],
    }
  }

  foreach std, arg: cxx_standards
    glib_tests += {
      'cxx-@0@'.format(std) : {
        'source' : ['cxx.cpp'],
        'suite' : ['cpp'],
        'cpp_args' : [arg, '-D_G_EXPECTED_CXX_STANDARD="@0@"'.format(std)],
        'dependencies' : [wincodecs],
      },
    }
  endforeach
endif

if cc.get_id() != 'msvc'
  glib_tests += {'autoptr' : {}}
endif

if glib_conf.has('HAVE_EVENTFD')
  glib_tests += {
    'gwakeup-fallback' : {
      'source' : ['gwakeuptest.c', '../gwakeup.c'],
      'c_args' : ['-DTEST_EVENTFD_FALLBACK'],
      'install' : false,
    },
  }
endif

if host_machine.system() == 'windows'
  if winsock2.found()
    glib_tests += {
      'gpoll' : {
        'dependencies' : [winsock2],
      },
    }
  endif
  glib_tests += {
    'win32' : {
      'dependencies' : [wincodecs],
    },
    'win32-private' : {
      'dependencies' : [winsock2],
    },
    'win32-helper-path-private' : {
      'source' : 'win32-helper-path.c',
    },
  }
else
  glib_tests += {
    'include' : {},
    'poll-fdset' : {},
    'unix' : {},
  }
  if have_rtld_next and glib_build_shared
    getpwuid_preload = shared_library('getpwuid-preload',
                                      'getpwuid-preload.c',
                                      name_prefix : '',
                                      dependencies: [libdl_dep, libglib_dep],
                                      install_dir : installed_tests_execdir,
                                      install_tag : 'tests',
                                      install: installed_tests_enabled)

    if host_system != 'darwin'
      var_preload = 'LD_PRELOAD'
    else
      var_preload = 'DYLD_INSERT_LIBRARIES'
    endif

    asan_env = {}
    if 'address' in glib_sanitizers
      asan_env = {'ASAN_OPTIONS': 'verify_asan_link_order=0'}
    endif

    glib_tests += {
      'gutils-user-database' : {
        'depends' : getpwuid_preload,
        'env' : {
          var_preload: getpwuid_preload.full_path()
        } + asan_env,
        'installed_tests_env' : {
          var_preload: installed_tests_execdir / fs.name(getpwuid_preload.full_path())
        } + asan_env,
      },
    }
  endif
endif

if installed_tests_enabled
  install_data(
    '4096-random-bytes',
    'casefold.txt',
    'casemap.txt',
    'echo-script',
    'echo-script.bat',
    'empty',
    'iochannel-test-infile',
    'keyfile.c',
    'keyfiletest.ini',
    'NormalizationTest.txt',
    'pages.ini',
    install_dir : installed_tests_execdir,
    install_tag : 'tests',
  )
  install_subdir('bookmarks',
    install_dir : installed_tests_execdir,
    install_tag : 'tests',
  )
  install_subdir('markups',
    install_dir : installed_tests_execdir,
    install_tag : 'tests',
  )
  install_subdir('time-zones',
    install_dir : installed_tests_execdir,
    install_tag : 'tests',
  )
endif

test_extra_programs = {
  'assert-msg-test' : {},
  'markup-example' : {'install' : false},
  'spawn-path-search-helper' : {},
  'spawn-test-helper' : {},
  'testing-helper' : {},
  # test-spawn-echo helper binary required by the spawn tests above
  'test-spawn-echo' : {},
}

if have_dlopen_dlsym and have_rtld_next
  test_extra_programs += {
    'messages-low-memory' : {
      'dependencies' : libdl_dep,
      'override_options' : ['b_asneeded=false'],
    },
  }
endif

if host_machine.system() == 'windows'
  # test-spawn-sleep helper binary required by the spawn tests above
  test_extra_programs += {
    'test-spawn-sleep' : {},
    'spawn-test-win32-gui' : { 'win_subsystem': 'windows' },
  }
endif

if host_machine.system() == 'windows' or have_dlopen_dlsym
  constructor_helper = shared_library('constructor-helper',
                                      'constructor-helper.c',
                                      dependencies: [libglib_dep],
                                      install : false)
  constructor_lib = shared_library('constructor-lib',
                                   'constructor.c',
                                   c_args: ['-DPREFIX=lib'],
                                   dependencies: [libglib_dep],
                                   link_with: [constructor_helper],
                                   install : false)
  glib_tests += {
    'constructor' : {
      'install': false,
      'c_args': ['-DLIB_NAME=' + fs.name(constructor_lib.full_path()),
                 '-DBUILD_TEST_EXECUTABLE',
                 '-DPREFIX=app'],
      'dependencies' : libdl_dep,
      'depends': constructor_lib,
      'link_with': [constructor_helper]
    }
  }
endif

foreach test_name, extra_args : glib_tests
  foreach std: extra_args.get('c_standards', [])
    if c_standards.has_key(std)
      glib_tests += {
        '@0@-c-@1@'.format(test_name, std) : extra_args + {
          'source' : extra_args.get('source', test_name + '.c'),
          'suite' : ['cc'] + extra_args.get('suite', []),
          'override_options' : extra_args.get('override_options', []) + [
            'c_std=c@0@'.format(std),
          ],
          'c_args' : extra_args.get('c_args', []) + [
            '-D_G_EXPECTED_C_STANDARD="@0@"'.format(std),
          ],
        }
      }
    endif
  endforeach
endforeach

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

test_deps = [libm, thread_dep, libglib_dep, test_profile_dep]
test_deps_static = [libm, thread_dep, libglib_static_dep, test_profile_dep]
test_cargs = ['-DG_LOG_DOMAIN="GLib"', '-UG_DISABLE_ASSERT']
test_cpp_args = test_cargs

test_extra_programs_targets = {}
foreach program_name, extra_args : test_extra_programs
  source = extra_args.get('source', program_name + '.c')
  extra_sources = extra_args.get('extra_sources', [])
  install = installed_tests_enabled and extra_args.get('install', true)
  test_extra_programs_targets += {
    program_name : executable(program_name,
      sources: [source, extra_sources],
      c_args : test_cargs,
      cpp_args: test_cpp_args,
      override_options : extra_args.get('override_options', []),
      dependencies : test_deps + extra_args.get('dependencies', []),
      install_dir : installed_tests_execdir,
      install_tag : 'tests',
      install : install,
      win_subsystem : extra_args.get('win_subsystem', 'console'),
    )
  }
endforeach

subdir('path-test-subdir')

foreach test_name, extra_args : glib_tests
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

  base_deps = test_name.contains('-private') ? test_deps_static : test_deps

  exe = executable(test_name, source,
    c_args : test_cargs + extra_args.get('c_args', []),
    cpp_args : test_cpp_args + extra_args.get('cpp_args', []),
    link_args : extra_args.get('link_args', []),
    override_options : extra_args.get('override_options', []),
    dependencies : base_deps + extra_args.get('dependencies', []),
    link_with : extra_args.get('link_with', []),
    install_dir: installed_tests_execdir,
    install_tag: 'tests',
    install: install,
  )

  depends = [extra_args.get('depends', []), gspawn_helpers]
  suite = ['glib', 'core'] + extra_args.get('suite', [])
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
    protocol : extra_args.get('protocol', test_protocol),
    depends : depends,
    env : local_test_env,
    timeout : timeout,
    suite : suite,
    should_fail : extra_args.get('should_fail', false),
  )
endforeach

if installed_tests_enabled
  install_data(
    'utf8.txt',
    install_dir : installed_tests_execdir,
    install_tag : 'tests',
  )
endif

python_tests = {
  'assert-msg-test.py' : {
    'can_fail' : host_system == 'windows',
    'extra_programs': ['assert-msg-test'],
    'env': {'_GLIB_TEST_SANITIZERS': ','.join(glib_sanitizers)},
  },
}

if 'messages-low-memory' in test_extra_programs
  python_tests += {
    'messages-low-memory.py' : {
      'extra_programs': ['messages-low-memory'],
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
  suite = ['glib', 'core', 'no-valgrind']

  if extra_args.get('can_fail', false)
    suite += 'failing'
  endif

  local_test_env = python_test_env
  foreach var, value : extra_args.get('env', {})
    local_test_env.append(var, value)
  endforeach

  foreach program : extra_args.get('extra_programs', [])
    depends += test_extra_programs_targets[program]
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

# some testing of gtester functionality
# FIXME: Work around Meson not automatically using exe_wrapper around gtester
# https://github.com/mesonbuild/meson/issues/11029
if not meson.is_cross_build() and host_system != 'windows'
  xmllint = find_program('xmllint', required: false)
  if xmllint.found()
    tmpsample_xml = custom_target('tmpsample.xml',
      output : 'tmpsample.xml',
      command : [ gtester, '-k', '--quiet', '--i-know-this-is-deprecated', '-o', '@OUTPUT@',
                  '--test-arg=--gtester-selftest', gtester])

    test('gtester-xmllint-check', xmllint,
      args : ['--noout', tmpsample_xml],
      env : test_env,
      suite : ['glib', 'core'],
    )
  endif
endif

if have_bash and have_pkg_config
  prefix = get_option('prefix')
  if prefix.endswith(':/')
    prefix += '/'
  endif
  test('glib-2.0-pkg-config',
    bash,
    args: [
      '-xe', '-c',
      '\n'.join([
        'pkg-config --validate glib-2.0',
        'test "$(pkg-config --modversion glib-2.0)" = "@0@"'.format(glib_version),
        'test "$(pkg-config --variable=prefix glib-2.0)" = "@0@"'.format(
          get_option('prefix')),
        'test "$(pkg-config --variable=datadir glib-2.0)" = "@0@"'.format(
          prefix / get_option('datadir')),
        'test "$(pkg-config --variable=gobject_query glib-2.0)" = "@0@"'.format(
          prefix / get_option('bindir') / 'gobject-query'),
        'test "$(pkg-config --variable=glib_mkenums glib-2.0)" = "@0@"'.format(
          prefix / get_option('bindir') / 'glib-mkenums'),
        'test "$(pkg-config --variable=glib_valgrind_suppressions glib-2.0)" = "@0@"'.format(
          prefix / get_option('datadir') /
          valgrind_suppression_file_install_subdir / fs.name(valgrind_suppression_file)),
      ]),
    ],
    suite: ['glib', 'core', 'no-valgrind', 'pkg-config'],
    env: {
      'PKG_CONFIG_PATH': meson.project_build_root() / 'meson-private',
    },
  )
endif
