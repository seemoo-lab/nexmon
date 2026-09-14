gio_manpages = [
  'gapplication',
  'gdbus-codegen',
  'gdbus',
  'gio-querymodules',
  'gio',
  'glib-compile-resources',
  'glib-compile-schemas',
  'gresource',
  'gsettings',
]

if get_option('man-pages').enabled()
  foreach page: gio_manpages
    custom_target(page + '-man',
      input: page + '.rst',
      output: page + '.1',
      command: [
        rst2man,
        rst2man_flags,
        '@INPUT@',
      ],
      capture: true,
      install: true,
      install_dir: man1_dir,
      install_tag: 'doc',
    )
  endforeach
endif

if get_option('documentation')
  man_html_path = docs_dir / 'gio-2.0'

  foreach page: gio_manpages
    custom_target(
      input: page + '.rst',
      output: page + '.html',
      command: [
        rst2html5,
        '@INPUT@',
      ],
      capture: true,
      install: true,
      install_dir: man_html_path,
      install_tag: 'doc',
    )
  endforeach
endif

if get_option('documentation') and enable_gir
  expand_content_files = [
    'dbus-error.md',
    'dbus-introspection.md',
    'dbus-name-owning.md',
    'dbus-name-watching.md',
    'dbus-utils.md',
    'error.md',
    'file-attributes.md',
    'io-scheduler.md',
    'menu-exporter.md',
    'migrating-gconf.md',
    'migrating-gdbus.md',
    'migrating-gnome-vfs.md',
    'migrating-posix.md',
    'networking.md',
    'overview.md',
    'pollable-utils.md',
    'tls-overview.md',
    'content-types.md',
  ]
  expand_content_unix_files = [
    'unix-mounts.md',
  ]
  expand_content_win32_files = []

  gio_toml = configure_file(input: 'gio.toml.in', output: 'gio.toml', configuration: toml_conf)

  custom_target('gio-docs',
    input: [ gio_toml, gio_gir[0] ],
    output: 'gio-2.0',
    command: [
      gidocgen,
      'generate',
      gidocgen_common_args,
      '--config=@INPUT0@',
      '--output-dir=@OUTPUT@',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '@INPUT1@',
    ],
    build_by_default: true,
    depend_files: expand_content_files,
    install: true,
    install_dir: docs_dir,
    install_tag: 'doc',
  )

  if host_system == 'windows'
    gio_win32_toml = configure_file(input: 'gio-win32.toml.in', output: 'gio-win32.toml', configuration: toml_conf)

    custom_target('gio-win32-docs',
      input: [ gio_win32_toml, gio_win32_gir[0] ],
      output: 'gio-win32-2.0',
      command: [
        gidocgen,
        'generate',
        gidocgen_common_args,
        '--config=@INPUT0@',
        '--output-dir=@OUTPUT@',
        '--content-dir=@0@'.format(meson.current_source_dir()),
        '@INPUT1@',
      ],
      build_by_default: true,
      depend_files: expand_content_win32_files,
      install: true,
      install_dir: docs_dir,
      install_tag: 'doc',
    )
  else
    gio_unix_toml = configure_file(input: 'gio-unix.toml.in', output: 'gio-unix.toml', configuration: toml_conf)

    custom_target('gio-unix-docs',
      input: [ gio_unix_toml, gio_unix_gir[0] ],
      output: 'gio-unix-2.0',
      command: [
        gidocgen,
        'generate',
        gidocgen_common_args,
        '--config=@INPUT0@',
        '--output-dir=@OUTPUT@',
        '--content-dir=@0@'.format(meson.current_source_dir()),
        '@INPUT1@',
      ],
      build_by_default: true,
      depend_files: expand_content_unix_files,
      install: true,
      install_dir: docs_dir,
      install_tag: 'doc',
    )
  endif
endif
