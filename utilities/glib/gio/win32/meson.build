giowin32_sources = [
  'gwin32fsmonitorutils.c',
  'gwin32filemonitor.c',
  'gwinhttpvfs.c',
  'gwinhttpfile.c',
  'gwinhttpfileinputstream.c',
  'gwinhttpfileoutputstream.c',
]

giowin32_lib = static_library('giowin32',
  sources : [giowin32_sources],
  include_directories : [configinc, glibinc, gioinc],
  dependencies : [
    libintl,
    gioenumtypes_dep,
    libglib_dep,
    gmodule_inc_dep,
  ],
  gnu_symbol_visibility : 'hidden',
  pic : true,
  c_args : [gio_c_args, gio_c_args_internal])
