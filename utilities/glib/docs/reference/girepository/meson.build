if get_option('man-pages').enabled()
  manpages = [
    'gi-compile-repository',
    'gi-decompile-typelib',
    'gi-inspect-typelib',
  ]
  foreach page : manpages
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

if get_option('documentation') and enable_gir
  expand_content_files = [
    'migrating-gi.md',
  ]

  girepository_toml = configure_file(
    input: 'girepository.toml.in',
    output: 'girepository.toml',
    configuration: toml_conf,
  )

  custom_target('girepository-docs',
    input: [ girepository_toml, girepository_gir[0] ],
    output: 'girepository-2.0',
    command: [
      gidocgen,
      'generate',
      gidocgen_common_args,
      '--config=@INPUT0@',
      '--output-dir=@OUTPUT@',
      '--content-dir=@0@'.format(meson.current_source_dir()),
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../../../gobject'),
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../../../gmodule'),
      '--add-include-path=@0@'.format(meson.current_build_dir() / '../../../gio'),
      '@INPUT1@',
    ],
    build_by_default: true,
    depend_files: expand_content_files,
    install: true,
    install_dir: docs_dir,
    install_tag: 'doc',
  )
endif
