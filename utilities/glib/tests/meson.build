# Copyright 2024 Collabora Ltd.
# SPDX-License-Identifier: LGPL-2.1-or-later

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

lint_scripts = [
  'black.sh',
  'flake8.sh',
  'reuse.sh',
  'shellcheck.sh',
]

if have_bash
  foreach test_name : lint_scripts
    test(
      test_name,
      bash,
      args : [files(test_name)],
      env : test_env,
      suite : ['lint', 'no-valgrind'],
      protocol : 'tap',
    )
  endforeach
endif

if not meson.is_subproject()
  test(
    'check-missing-install-tag.py',
    python,
    args : ['-B', files('check-missing-install-tag.py')],
    env : test_env,
    suite : ['lint', 'no-valgrind'],
    protocol : 'tap',
  )
endif

subdir('lib')
