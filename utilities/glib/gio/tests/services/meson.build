dbus_service_files = [
  'org.gtk.GDBus.FakeService.service'
]

srcdir_cdata = configuration_data()
srcdir_cdata.set('installed_tests_dir', meson.current_build_dir() / '..')

installed_cdata = configuration_data()
installed_cdata.set('installed_tests_dir', installed_tests_execdir)

foreach service_file_name : dbus_service_files
  service_file = configure_file(
    input: service_file_name + '.in',
    output: service_file_name,
    configuration: srcdir_cdata,
  )
  if installed_tests_enabled
    # Build a second copy of the service file for the installed
    # version of the tests.
    installed_service_file = configure_file(
      input: service_file_name + '.in',
      output: service_file_name + '.to-install',
      configuration: installed_cdata,
    )
    install_data(installed_service_file,
      install_dir: installed_tests_execdir / 'services',
      install_tag: 'tests',
      rename: [service_file_name],
    )
  endif
endforeach
