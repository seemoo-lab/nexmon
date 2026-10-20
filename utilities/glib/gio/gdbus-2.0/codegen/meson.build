# Copyright 2016, 2017 Centricular
#
# SPDX-License-Identifier: LGPL-2.1-or-later
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, see <http://www.gnu.org/licenses/>.

gdbus_codegen_files = [
  '__init__.py',
  'codegen.py',
  'codegen_main.py',
  'codegen_docbook.py',
  'codegen_md.py',
  'codegen_rst.py',
  'dbustypes.py',
  'parser.py',
  'utils.py',
]

gdbus_codegen_conf = configuration_data()
gdbus_codegen_conf.set('VERSION', glib_version)
gdbus_codegen_conf.set('MAJOR_VERSION', major_version)
gdbus_codegen_conf.set('MINOR_VERSION', minor_version)
gdbus_codegen_conf.set('PYTHON', python_shebang)
gdbus_codegen_conf.set('DATADIR', glib_datadir)

# Install gdbus-codegen executable
gdbus_codegen = configure_file(input : 'gdbus-codegen.in',
  output : 'gdbus-codegen',
  install_dir : get_option('bindir'),
  install_tag : 'bin-devel',
  configuration : gdbus_codegen_conf
)
  # Provide tools for others when we're a subproject and they use the Meson GNOME module
meson.override_find_program('gdbus-codegen', gdbus_codegen)

codegen_dir = join_paths(glib_datadir, 'glib-2.0', 'codegen')

gdbus_codegen_built_files = []
gdbus_codegen_built_targets = []
gdbus_codegen_built_files += configure_file(input : 'config.py.in',
                                            output : 'config.py',
                                            install_dir : codegen_dir,
                                            install_tag : 'bin-devel',
                                            configuration : gdbus_codegen_conf)

foreach f : gdbus_codegen_files
  # Copy these into the builddir so that gdbus-codegen can be used uninstalled
  # and then install it too so that it can be used after installation
  gdbus_codegen_built_targets += fs.copyfile(f, f,
                                             install : true,
                                             install_dir : codegen_dir,
                                             install_tag : 'bin-devel')
endforeach
