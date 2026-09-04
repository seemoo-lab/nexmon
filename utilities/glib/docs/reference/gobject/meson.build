gobject_manpages = ['glib-mkenums', 'glib-genmarshal', 'gobject-query']

if get_option('man-pages').enabled()
  foreach page: gobject_manpages
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
  man_html_path = docs_dir / 'gobject-2.0'

  foreach page: gobject_manpages
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
    'boxed.md',
    'concepts.md',
    'enum-types.md',
    'floating-refs.md',
    'gvalue.md',
    'tutorial.md',
    'types.md',
    'signals.md',
    'value-collection.md',
  ]

  gobject_toml = configure_file(input: 'gobject.toml.in', output: 'gobject.toml', configuration: toml_conf)

  custom_target('gobject-docs',
    input: [ gobject_toml, gobject_gir[0] ],
    output: 'gobject-2.0',
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
endif
