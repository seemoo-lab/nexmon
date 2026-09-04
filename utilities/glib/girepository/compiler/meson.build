# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2024 GNOME Foundation

custom_c_args = [
  '-DG_LOG_DOMAIN="GLib-GirCompiler"',  
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

gicompilerepository = executable('gi-compile-repository', 'compiler.c',
  dependencies: [
    libgirepository_dep,
    libgirepository_internals_dep,
    libgio_dep,
    app_profile_dep,
  ],
  install: true,
  c_args: custom_c_args,
)

# Only override the user-installed compiler if we need to generate the GIRs
# GLib, Gio, and GObject...
if enable_gir
  # Replace the default g-ir-compiler target with the version we
  # just built.
  meson.override_find_program('g-ir-compiler', gicompilerepository)
endif
