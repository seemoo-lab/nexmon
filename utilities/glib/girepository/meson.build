# SPDX-License-Identifier: LGPL-2.1-or-later
# SPDX-FileCopyrightText: 2017 Patrick Griffis
# SPDX-FileCopyrightText: 2017 Danny Forghieri
# SPDX-FileCopyrightText: 2017 Nirbheek Chauhan
# SPDX-FileCopyrightText: 2017, 2021, 2022 Emmanuele Bassi
# SPDX-FileCopyrightText: 2018 Mathieu Duponchelle
# SPDX-FileCopyrightText: 2018, 2019, 2020 Christoph Reiter
# SPDX-FileCopyrightText: 2018 Kai Kang
# SPDX-FileCopyrightText: 2018 Carlos Garnacho
# SPDX-FileCopyrightText: 2018, 2019, 2020 Alexander Kanavin
# SPDX-FileCopyrightText: 2019, 2020 Chun-wei Fan
# SPDX-FileCopyrightText: 2019 Aaron Boxer
# SPDX-FileCopyrightText: 2019 Thibault Saunier
# SPDX-FileCopyrightText: 2019 Joshua Watt
# SPDX-FileCopyrightText: 2020 Xavier Claessens
# SPDX-FileCopyrightText: 2020 Philip Chimento
# SPDX-FileCopyrightText: 2021 John Ericson
# SPDX-FileCopyrightText: 2021 Cimbali
# SPDX-FileCopyrightText: 2021, 2023 Simon McVittie
# SPDX-FileCopyrightText: 2022 Andoni Morales Alastruey

subdir('cmph')

gir_dir_prefix = get_option('gir_dir_prefix')
if gir_dir_prefix == '' or gir_dir_prefix == get_option('datadir')
  gir_dir_prefix = get_option('datadir')
  gir_dir_pc_prefix = '${datadir}'
else
  gir_dir_pc_prefix = join_paths('${prefix}', gir_dir_prefix)
endif

# This is effectively the GIR XML format major version
gir_suffix = 'gir-1.0'

glib_girdir = get_option('prefix') / gir_dir_prefix / gir_suffix

gir_includedir = glib_includedir / 'girepository'

gi_visibility_h = custom_target(
  output: 'gi-visibility.h',
  command: [gen_visibility_macros, meson.project_version(), 'visibility-macros', 'GI', '@OUTPUT@'],
  install: true,
  install_dir: gir_includedir,
  install_tag: 'devel',
)

girepo_headers = files(
  'giarginfo.h',
  'gibaseinfo.h',
  'gicallableinfo.h',
  'gicallbackinfo.h',
  'giconstantinfo.h',
  'gienuminfo.h',
  'gifieldinfo.h',
  'giflagsinfo.h',
  'gifunctioninfo.h',
  'giinterfaceinfo.h',
  'giobjectinfo.h',
  'gipropertyinfo.h',
  'giregisteredtypeinfo.h',
  'girepository.h',
  'girepository-autocleanups.h',
  'gisignalinfo.h',
  'gistructinfo.h',
  'gitypeinfo.h',
  'gitypelib.h',
  'gitypes.h',
  'giunioninfo.h',
  'giunresolvedinfo.h',
  'givalueinfo.h',
  'givfuncinfo.h',
)

girepo_ffi_headers = [
  'girffi.h',
]

girepo_ffi_sources = files(
  'girffi.c',
)

install_headers(girepo_headers + girepo_ffi_headers, install_dir: gir_includedir)

gir_c_args = [
  '-DGI_COMPILATION',
  '-DG_LOG_DOMAIN="GLib-GIRepository"',
  '-DGIR_SUFFIX="@0@"'.format(gir_suffix),
  '-DGIR_DIR="@0@"'.format(glib_girdir),
  '-DGOBJECT_INTROSPECTION_LIBDIR="@0@"'.format(glib_libdir),
  '-DGOBJECT_INTROSPECTION_DATADIR="@0@"'.format(glib_datadir),
  '-DGOBJECT_INTROSPECTION_RELATIVE_LIBDIR="@0@"'.format(get_option('libdir')),
  warning_sign_conversion_args,
]

custom_c_args = []

if cc.get_id() != 'msvc'
  custom_c_args = cc.get_supported_arguments([
    '-Wno-old-style-definition',
    '-Wno-cast-align',
  ])
endif

girepo_gthash_lib = static_library('girepository-gthash',
  sources: ['gthash.c', gi_visibility_h],
  include_directories : [configinc, girepoinc],
  c_args: gir_c_args + custom_c_args,
  dependencies: [
    cmph_dep,
    libglib_dep,
    libgmodule_dep,
    libgobject_dep,
  ],
  gnu_symbol_visibility : 'hidden',
)

girepo_gthash_dep = declare_dependency(
  link_with: girepo_gthash_lib,
  dependencies: [libglib_dep, libgmodule_dep, libgobject_dep],
  include_directories: [girepoinc],
)

if cc.get_id() != 'msvc'
  custom_c_args = cc.get_supported_arguments([
    '-Wno-unused-parameter',
    '-Wno-duplicated-branches',
    '-Wno-cast-align',
  ])
endif

libgirepository_internals = static_library('girepository-internals',
  sources: [
    'girmodule.c',
    'girnode.c',
    'giroffsets.c',
    'girparser.c',
    'girwriter.c',
    gi_visibility_h,
  ],
  c_args: gir_c_args + custom_c_args,
  gnu_symbol_visibility : 'hidden',
  include_directories : [configinc, girepoinc],
  dependencies: [girepo_gthash_dep, libffi_dep],
)

libgirepository_internals_dep = declare_dependency(
  link_with: libgirepository_internals,
  dependencies: libffi_dep,
  include_directories: [girepoinc],
)

girepo_sources = files(
  'gdump.c',
  'giarginfo.c',
  'gibaseinfo.c',
  'gicallableinfo.c',
  'gicallbackinfo.c',
  'giconstantinfo.c',
  'gienuminfo.c',
  'gifieldinfo.c',
  'giflagsinfo.c',
  'gifunctioninfo.c',
  'ginvoke.c',
  'giinterfaceinfo.c',
  'giobjectinfo.c',
  'gipropertyinfo.c',
  'giregisteredtypeinfo.c',
  'girepository.c',
  'girffi.c',
  'gisignalinfo.c',
  'gistructinfo.c',
  'gitypeinfo.c',
  'gitypelib.c',
  'giunioninfo.c',
  'giunresolvedinfo.c',
  'givalueinfo.c',
  'givfuncinfo.c',
)

if cc.get_id() != 'msvc'
  custom_c_args = cc.get_supported_arguments([
    '-Wno-unused-parameter',
    '-Wno-duplicated-branches',
    '-Wno-type-limits',
    '-Wno-cast-align',
    '-Wno-missing-field-initializers',
  ])
endif

libgirepository = library('girepository-2.0',
  sources: girepo_sources + girepo_ffi_sources + [gi_visibility_h],
  include_directories: [configinc, girepoinc],
  c_args: gir_c_args,
  version: library_version,
  soversion: soversion,
  darwin_versions: darwin_versions,
  gnu_symbol_visibility: 'hidden',
  link_args: glib_link_flags,
  dependencies: [
    libglib_dep,
    libgobject_dep,
    libgmodule_dep,
    libgio_dep,
    libgirepository_internals_dep,
  ],
  install: true,
)

libgirepository_dep = declare_dependency(
  link_with: libgirepository,
  dependencies: [libglib_dep, libgobject_dep, libgio_dep, libgmodule_dep],
  sources: [gi_visibility_h],
  include_directories: [girepoinc],
  variables: {
    # Export the path for the built GLib-2.0.typelib (etc.) for when GLib is
    # used as a subproject. The variable names match those in
    # pkgconfig_variables below.
    'girdir': meson.current_build_dir() / 'introspection',
    'typelibdir': meson.current_build_dir() / 'introspection',
  },
)
meson.override_dependency('girepository-2.0', libgirepository_dep)

executable('gi-dump-types',
  sources: 'gi-dump-types.c',
  dependencies: [
    libgirepository_dep,
    libgiounix_dep,
    libgiowin32_dep,
    app_profile_dep,
  ],
)

pkgconfig_variables = [
  'gidatadir=${datadir}/gobject-introspection-1.0',
  'girdir=' + gir_dir_pc_prefix / gir_suffix,
  'typelibdir=${libdir}/girepository-1.0',
  'gi_compile_repository=' + pkgconfig_multiarch_bindir / 'gi-compile-repository'
]

pkg.generate(libgirepository,
  name: 'girepository',
  version: glib_version,
  filebase: 'girepository-2.0',
  install_dir: glib_pkgconfigreldir,
  description: 'GObject Introspection repository parser',
  variables: pkgconfig_variables,
  libraries: [libglib_dep, libgobject_dep],
)

subdir('compiler')

if enable_gir
  subdir('introspection')
endif

subdir('decompiler')
subdir('inspector')

if build_tests
  subdir('tests')
endif
