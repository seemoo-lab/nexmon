test_extra_programs_targets += {
  'spawn-test-helper-subdir' : executable('spawn-test-helper',
    sources: 'spawn-test-helper.c',
    c_args : test_cargs,
    dependencies : test_deps,
    install_dir: installed_tests_execdir / 'path-test-subdir',
    install_tag: 'tests',
    install: installed_tests_enabled,
  ),
}
