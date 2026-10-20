# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2024 GNOME Foundation

custom_c_args = [
  '-DG_LOG_DOMAIN="GLib-GirDecompiler"',
  warning_sign_conversion_args,
]

if cc.get_id() != 'msvc'
  custom_c_args = cc.get_supported_arguments([
    '-Wno-old-style-definition',
    '-Wno-cast-align',
    '-Wno-unused-parameter',
    '-Wno-duplicated-branches',
  ])
endif

gidecompiletypelib = executable('gi-decompile-typelib', 'decompiler.c',
  dependencies: [
    libgirepository_dep,
    libgirepository_internals_dep,
    libgio_dep,
    app_profile_dep,
  ],
  install: true,
  c_args: custom_c_args,
)
