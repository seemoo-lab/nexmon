# Doxygen

doxygen = find_program('doxygen')

# TODO: To make the xml directory work as dependency of the
# Python target, we must make sure that its timestamp changes
# whenever the docs are rebuilt. Either delete the directory
# before generating, or touch it afterward?
doxygen_docs = custom_target(
    'Doxygen documentation',
    input: [
        xml_src_files, libxml_headers,
        files('libxml2.css', 'mainpage.md'),
    ],
    output: [ 'html', 'xml' ],
    command: [ doxygen, '-q', files('Doxyfile') ],
    env: {
        'SOURCE_ROOT': meson.project_source_root() + '/',
        'BUILD_ROOT':  meson.project_build_root()  + '/',
    },
    install: true,
    install_dir: [ want_docs ? dir_doc : false, false ],
)

if want_docs
    # xml2-config

    install_man('xml2-config.1')

    # Docbook

    xsltproc = find_program('xsltproc')
    types = [
        [ 'manpages', '.1',    dir_man / 'man1' ],
        [ 'html',     '.html', dir_doc ],
    ]
    programs = [ 'xmllint' ]
    if want_catalog and want_output
        programs += 'xmlcatalog'
    endif

    foreach prog : programs
        foreach type : types
            format = type[0]
            ext = type[1]
            install_dir = type[2]

            xsl = 'http://docbook.sourceforge.net' + \
                  f'/release/xsl/current/@format@/docbook.xsl'
            output = prog + ext

            custom_target(
                output,
                input: prog + '.xml',
                output: output,
                command: [xsltproc,
                    '--nonet', '--novalid',
                    '--param', 'man.output.quietly', '1',
                    '-o', '@OUTPUT@',
                    xsl, '@INPUT@'
                ],
                install: true,
                install_dir: install_dir
            )
        endforeach
    endforeach
endif
