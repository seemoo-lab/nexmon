if get_option('documentation') and enable_gir
  expand_content_files = [
    'modules.md',
  ]

  gmodule_toml = configure_file(input: 'gmodule.toml.in', output: 'gmodule.toml', configuration: toml_conf)

  custom_target('gmodule-docs',
    input: [ gmodule_toml, gmodule_gir[0] ],
    output: 'gmodule-2.0',
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
