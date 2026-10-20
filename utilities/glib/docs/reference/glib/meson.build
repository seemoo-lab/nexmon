glib_manpages = ['glib-gettextize', 'gtester', 'gtester-report']

if get_option('man-pages').enabled()
  foreach page: glib_manpages
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
  # GVariant specification is currently standalone
  spec_path = docs_dir / 'glib-2.0'
  man_html_path = docs_dir / 'glib-2.0'

  figures = files(
    'gvariant-byte-boundaries.svg',
    'gvariant-integer-and-string-structure.svg',
    'gvariant-integer-array.svg',
    'gvariant-string-array.svg',
  )

  custom_target('gvariant-specification-1.0',
    input: 'gvariant-specification-1.0.rst',
    output: 'gvariant-specification-1.0.html',
    command: [
      rst2html5,
      '@INPUT@',
    ],
    capture: true,
    install: true,
    install_dir: spec_path,
    install_tag: 'doc',
    depend_files: figures,
  )

  install_data(figures,
    install_dir: spec_path,
    install_tag: 'doc',
  )

  foreach page: glib_manpages
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
    'atomic.md',
    'base64.md',
    'building.md',
    'character-set.md',
    'checked-math.md',
    'compiling.md',
    'cross-compiling.md',
    'datalist-and-dataset.md',
    'error-reporting.md',
    'file-utils.md',
    'gvariant-format-strings.md',
    'gvariant-text-format.md',
    'i18n.md',
    'logging.md',
    'main-loop.md',
    'memory.md',
    'memory-slices.md',
    'numerical.md',
    'random.md',
    'reference-counting.md',
    'running.md',
    'testing.md',
    'threads.md',
    'threads-deprecated.md',
    'markup.md',
    'misc-utils.md',
    'goption.md',
    'host-utils.md',
    'data-structures.md',
    'programming.md',
    'resources.md',
    'shell.md',
    'spawn.md',
    'string-utils.md',
    'types.md',
    'unicode.md',
    'uuid.md',
    'version.md',
    'warnings.md',
  ]
  expand_content_unix_files = [
    'unix.md',
  ]
  expand_content_win32_files = [
    'windows.md',
  ]

  glib_toml = configure_file(input: 'glib.toml.in', output: 'glib.toml', configuration: toml_conf)

  custom_target('glib-docs',
    input: [ glib_toml, glib_gir[0] ],
    output: 'glib-2.0',
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
    glib_win32_toml = configure_file(input: 'glib-win32.toml.in', output: 'glib-win32.toml', configuration: toml_conf)

    custom_target('glib-win32-docs',
      input: [ glib_win32_toml, glib_win32_gir[0] ],
      output: 'glib-win32-2.0',
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
    glib_unix_toml = configure_file(input: 'glib-unix.toml.in', output: 'glib-unix.toml', configuration: toml_conf)

    custom_target('glib-unix-docs',
      input: [ glib_unix_toml, glib_unix_gir[0] ],
      output: 'glib-unix-2.0',
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
