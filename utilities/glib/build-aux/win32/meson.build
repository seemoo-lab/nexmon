
if host_system == 'windows'
  static_lib = static_library('os-profile-app-static-lib',
                              sources: ['app.c'],
                              include_directories: [configinc])

  app_resources = windows.compile_resources('app.rc',
                                            depend_files: ['app.manifest.xml'])
  test_resources = windows.compile_resources('test.rc',
                                             depend_files: ['test.manifest.xml'])

  app_profile_dep = declare_dependency(link_whole: [static_lib],
                                       sources: [app_resources])
  test_profile_dep = declare_dependency(link_whole: [static_lib],
                                        sources: [test_resources])
else
  app_profile_dep = declare_dependency()
  test_profile_dep = declare_dependency()
endif

rc_conf_base = configuration_data({
  'MAJOR': major_version,
  'MINOR': minor_version,
  'MICRO': micro_version,
  'COPYRIGHTYEAR': '2026',
})

lib_rc_in = files('lib.rc.in')
