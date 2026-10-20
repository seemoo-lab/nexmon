# Copyright 2024 Collabora Ltd.
# Copyright 2025 Simon McVittie
# SPDX-License-Identifier: LGPL-2.1-or-later

if installed_tests_enabled
  install_data(
    files(
      'taptestrunner.py',
      'testprogramrunner.py',
    ),
    install_dir: installed_tests_execdir,
    install_tag: 'tests',
  )
endif

tests_conf = configuration_data()
tests_conf.set('gir_suffix', gir_suffix)
tests_conf.set('glib_datadir', glib_datadir)
tests_conf.set('glib_girdir', glib_girdir)

configure_file(
  configuration: tests_conf,
  input: 'glibconfig.py.in',
  output: 'glibconfig.py',
  install: installed_tests_enabled,
  install_dir: installed_tests_execdir,
  install_tag: 'tests',
)
