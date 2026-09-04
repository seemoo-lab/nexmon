# Copyright 2016 Centricular
# Copyright 2018 Endless Mobile, Inc.
# Copyright 2022 Collabora, Ltd.
# Copyright 2023 Eli Schwartz
# Copyright 2023 Canonical Ltd.
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

inotify_sources = [
  'inotify-kernel.c',
  'inotify-sub.c',
  'inotify-path.c',
  'inotify-missing.c',
  'inotify-helper.c',
  'ginotifyfilemonitor.c',
]

# necessary for the libinotify-kqueue backend
libinotify_kqueue_dep = dependency('libinotify', required: file_monitor_backend == 'libinotify-kqueue')

inotify_lib = static_library('inotify',
  sources : [inotify_sources],
  include_directories : [configinc, glibinc],
  dependencies : [
    gioenumtypes_dep,
    libglib_dep,
    libgobject_dep,
    gmodule_inc_dep,
    libinotify_kqueue_dep,
  ],
  gnu_symbol_visibility : 'hidden',
  pic : true,
  c_args : [gio_c_args, gio_c_args_internal])
