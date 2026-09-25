
pymod = import('python')
py = pymod.find_installation('python3', required: false)

if py.found() == true
    message('Installing python modules to', py.get_install_dir())

    pygenerated = custom_target(
        'Python generated files',
        input: doxygen_docs[1],
        output: [
            'libxml2-py.h',
            'libxml2-export.c',
            'libxml2-py.c',
            'libxml2.py',
        ],
        command: [ py, files('generator.py'), meson.current_build_dir() ],
        install: true,
        install_dir: [ false, false, false, py.get_install_dir() ],
    )
    pygenerated_dep = declare_dependency(
        sources : [pygenerated[0]],
    )

    libxml2mod_src = [
        files('libxml.c', 'types.c'),
        pygenerated[2],
    ]

    py.extension_module(
        'libxml2mod',
        libxml2mod_src,
        dependencies: [
            py.dependency(),
            xml_dep,
            pygenerated_dep,
        ],
        include_directories: [config_dir, '.'],
        install: true,
    )

    py.install_sources(files('drv_libxml2.py'))

    setup_py = configuration_data()
    setup_py.set('prefix', get_option('prefix'))
    setup_py.set('LIBXML_VERSION', meson.project_version())
    setup_py.set('WITH_ICONV', want_iconv.to_int())
    setup_py.set('WITH_ICU', want_icu.to_int())
    setup_py.set('WITH_ZLIB', want_zlib.to_int())
    setup_py.set('WITH_THREADS', want_threads.to_int())
    configure_file(
        input: 'setup.py.in',
        output: 'setup.py',
        configuration: setup_py,
    )

    python_tests = [
        'attribs.py',
        'build.py',
        'compareNodes.py',
        'ctxterror.py',
        'cutnpaste.py',
        'dtdvalid.py',
        'error.py',
        'inbuf.py',
        'indexes.py',
        'input_callback.py',
        'nsdel.py',
        'outbuf.py',
        'push.py',
        'pushSAX.py',
        'pushSAXhtml.py',
        'reader.py',
        'reader2.py',
        'reader3.py',
        'reader4.py',
        'reader5.py',
        'reader6.py',
        'reader7.py',
        'reader8.py',
        'readererr.py',
        'readernext.py',
        'regexp.py',
        'relaxng.py',
        'resolver.py',
        'schema.py',
        'serialize.py',
        'sync.py',
        'thread2.py',
        'tst.py',
        'tstLastError.py',
        'tstURI.py',
        'tstmem.py',
        'tstxpath.py',
        'unicode.py',
        'validDTD.py',
        'validRNG.py',
        'validSchemas.py',
        'validate.py',
        'walker.py',
        'xpath.py',
        'xpathext.py',
        'xpathleak.py',
        'xpathns.py',
        'xpathret.py',
    ]

    foreach file : python_tests
        test(file, py, args: [ file ],
             workdir: meson.current_source_dir() / 'tests',
             env: { 'PYTHONPATH': meson.current_build_dir() })
    endforeach
endif
