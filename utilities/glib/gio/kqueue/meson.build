kqueue_sources = [
  'gkqueuefilemonitor.c',
  'kqueue-helper.c',
  'kqueue-missing.c',
  'dep-list.c',
]

kqueue_lib = static_library('kqueue',
  sources : [kqueue_sources],
  include_directories : [configinc, glibinc],
  dependencies : [
    gioenumtypes_dep,
    libglib_dep,
    libgobject_dep,
    gmodule_inc_dep,
  ],
  gnu_symbol_visibility : 'hidden',
  pic : true,
  c_args : [gio_c_args, gio_c_args_internal])
