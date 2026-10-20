project(
    'libxml2',
    'c',
    version: files('VERSION'),
    license: 'MIT',
    default_options: [
        meson.version().version_compare('>=1.3.0') ? 'c_std=c11,c99,c89' : 'c_std=c11',
        'warning_level=3',
    ],
    meson_version: '>= 0.61',
)

v_array = meson.project_version().split('.')
v_maj = v_array[0].to_int()
v_min = v_array[1].to_int()
v_mic = v_array[2].to_int()
v_nbr = v_maj * 10000 + v_min * 100 + v_mic
v_extra = ''
r = run_command('git', 'describe', check: false)
if (r.returncode() == 0)
    v_extra = '-GIT' + r.stdout().strip()
endif

# install paths
dir_prefix = get_option('prefix')
dir_bin = dir_prefix / get_option('bindir')
dir_include = dir_prefix / get_option('includedir')
dir_pkginclude = dir_include / meson.project_name()
dir_lib = dir_prefix / get_option('libdir')
dir_man = dir_prefix / get_option('mandir')
dir_data = dir_prefix / get_option('datadir')
dir_doc = dir_data / 'doc' / meson.project_name()
dir_locale = dir_prefix / get_option('localedir')

# host

host_os = host_machine.system()

sys_cygwin = host_os == 'cygwin'
sys_windows = host_os == 'windows'

libxml2_cflags = []
xml_cflags = ''
dep_args = []

if sys_cygwin or sys_windows
    if get_option('default_library') == 'static'
        xml_cflags = '-DLIBXML_STATIC'
        libxml2_cflags += '-DLIBXML_STATIC'
        dep_args += '-DLIBXML_STATIC'
    endif
endif

# binaries
cc = meson.get_compiler('c')

# global compiler flags

global_args = [
    '-D_XOPEN_SOURCE=600',

    # Enabled by warning_level=3
    # '-pedantic',
    # '-Wall',
    # '-Wextra',

    '-Wshadow',
    '-Wpointer-arith',
    '-Wcast-align',
    '-Wwrite-strings',
    '-Wstrict-prototypes',
    '-Wmissing-prototypes',
    '-Wmissing-variable-declarations',
    '-Wno-long-long',
    '-Wno-format-extra-args',
    '-Wno-array-bounds',
]
global_args = cc.get_supported_arguments(global_args)
add_project_arguments(global_args, language: 'c')

# options

# disabled by default
want_docs = get_option('docs').enabled()
want_http = get_option('http').enabled()
want_icu = get_option('icu').enabled()
want_legacy = get_option('legacy').enabled()
want_python = get_option('python').enabled()
want_thread_alloc = get_option('thread-alloc').enabled()
want_tls = get_option('tls').enabled()

# default depends on minimum option

want_minimum = get_option('minimum')

feature = get_option('catalog')
want_catalog = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('debugging')
want_debug = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('html')
want_html = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('iconv')
want_iconv = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('iso8859x')
want_iso8859x = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('modules')
want_modules = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('sax1')
want_sax1 = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('threads')
want_threads = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('valid')
want_valid = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('xinclude')
want_xinclude = want_minimum ? feature.enabled() : feature.allowed()

# default depends on legacy option

feature = get_option('zlib')
want_zlib = want_legacy ? feature.allowed() : feature.enabled()

# dependencies

feature = get_option('output')
want_output = not want_minimum \
    or get_option('c14n').enabled() \
    or get_option('writer').enabled() ? \
    feature.allowed() : feature.enabled()

feature = get_option('pattern')
want_pattern = not want_minimum \
    or get_option('schemas').enabled() \
    or get_option('schematron').enabled() ? \
    feature.allowed() : feature.enabled()

feature = get_option('regexps')
want_regexps = not want_minimum \
    or get_option('relaxng').enabled() \
    or get_option('schemas').enabled() ? \
    feature.allowed() : feature.enabled()

feature = get_option('push')
want_push = not want_minimum \
    or get_option('reader').enabled() \
    or get_option('writer').enabled() ? \
    feature.allowed() : feature.enabled()

feature = get_option('readline')
want_readline = get_option('history').enabled() ? \
    feature.allowed() : feature.enabled()

feature = get_option('xpath')
want_xpath = not want_minimum \
    or get_option('c14n').enabled() \
    or get_option('schematron').enabled() \
    or get_option('xptr').enabled() ? \
    feature.allowed() : feature.enabled()

feature = get_option('c14n') \
    .require(want_output, error_message: 'c14n requires output') \
    .require(want_xpath, error_message: 'c14n requires xpath')
want_c14n = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('history') \
    .require(want_readline, error_message: 'history requires readline')
want_history = feature.enabled()

feature = get_option('reader') \
    .require(want_push, error_message: 'reader requires push')
want_reader = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('schemas') \
    .require(want_pattern, error_message: 'schemas requires pattern') \
    .require(want_regexps, error_message: 'schemas requires regexps')
want_schemas = not want_minimum \
    or get_option('relaxng').enabled() ? \
    feature.allowed() : feature.enabled()

feature = get_option('relaxng') \
    .require(want_regexps, error_message: 'relaxng requires regexps') \
    .require(want_schemas, error_message: 'relaxng requires schemas')
want_relaxng = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('schematron') \
    .require(want_pattern, error_message: 'schematron requires pattern') \
    .require(want_xpath, error_message: 'schematron requires xpath')
want_schematron = want_legacy ? feature.allowed() : feature.enabled()

feature = get_option('writer') \
    .require(want_output, error_message: 'writer requires output') \
    .require(want_push, error_message: 'writer requires push')
want_writer = want_minimum ? feature.enabled() : feature.allowed()

feature = get_option('xptr') \
    .require(want_xpath, error_message: 'xptr requires xpath')
want_xptr = want_minimum ? feature.enabled() : feature.allowed()

cflags_try = []

### workaround for native compilers, see configure.ac
if cc.get_argument_syntax() == 'gcc'
    cflags_try += [
        '-Wshadow',
        '-Wpointer-arith',
        '-Wcast-align',
        '-Wwrite-strings',
        '-Wstrict-prototypes',
        '-Wmissing-prototypes',
        '-Wno-long-long',
        '-Wno-format-extra-args',
        '-Wno-array-bounds',
    ]

    if host_machine.cpu_family() == 'alpha'
        cflags_try += '-mieee'
    endif
else
    if host_machine.cpu_family() == 'alpha'
        cflags_try += '-ieee'
    elif host_machine.cpu_family() == 'parisc'
        cflags_try += '-Wp,-H30000'
    endif
endif

foreach cf : cflags_try
    if cc.has_argument(cf)
        libxml2_cflags += cf
    endif
endforeach

# configuration
#
# X : done
# N : not done
#
# [X] config.h.in
# [X] include/libxml/xmlversion.h.in
# [X] libxml-2.0.pc.in
# [X] libxml2-config.cmake.in
# [X] python/setup.py.in
# [X] xml2-config.in

## config.h
config_h = configuration_data()
config_h.set_quoted('XML_SYSCONFDIR',
                    get_option('prefix') / get_option('sysconfdir'))

# header files
xml_check_headers = [
    [ 'stdint.h', true ],
]

foreach header : xml_check_headers
    if header[1] and cc.has_header(header[0])
        config_h.set10('HAVE_' + header[0].underscorify().to_upper(), true)
    endif
endforeach

# library functions
xml_check_functions = [
    # fct             | header
    ['getentropy', 'sys/random.h'],
    ['glob', 'glob.h'],
    ['mmap', 'sys/mman.h'],
]

foreach function : xml_check_functions
    config_h.set10('HAVE_DECL_' + function[0].underscorify().to_upper(),
                   cc.has_header_symbol(function[1], function[0]))
endforeach

# library

config_dir = [include_directories('.'), include_directories('include')]

## dependencies

xml_deps = []

### math library
if sys_windows == false
    m_dep = cc.find_library('m', required: false)
    if m_dep.found()
        xml_deps += m_dep
    endif
endif

### thread local storage
if want_tls
    foreach t : ['_Thread_local', '__thread', '__declspec(thread)']
        if cc.compiles('@0@ int v;'.format(t))
            config_h.set('XML_THREAD_LOCAL', t)
            break
        endif
    endforeach
endif

### __attribute__((destructor))
if cc.has_function_attribute('destructor')
    config_h.set10('HAVE_FUNC_ATTRIBUTE_DESTRUCTOR', true)
endif

### DSO support
if sys_cygwin or sys_windows
    module_extension = '.dll'
else
    module_extension = '.so'
endif

if want_modules and not sys_windows
    if meson.version().version_compare('>=0.62')
        dl_dep = dependency('dl', required: false)
    else
        dl_dep = dependency('', required: false)
    endif
    if not dl_dep.found()
        cc.has_header_symbol('dlfcn.h', 'dlopen', required: true)
        dl_dep = cc.find_library('dl', required: false)
    endif
    if dl_dep.found()
        xml_deps += dl_dep
    endif
    config_h.set10('HAVE_DLOPEN', true)
endif

### threads
if want_threads and not sys_windows
    threads_dep = dependency('threads')
    xml_deps += threads_dep
else
    threads_dep = dependency('', required: false)
endif

### xmllint shell history
xmllint_deps = []

if want_readline
    readline_dep = dependency('readline')
    config_h.set('HAVE_LIBREADLINE', true)
    xmllint_deps += readline_dep
endif

if want_history
    history_dep = dependency('history')
    config_h.set('HAVE_LIBHISTORY', true)
    xmllint_deps += history_dep
endif

if want_zlib
    zlib_dep = dependency('zlib')
    xmllint_deps += zlib_dep
else
    zlib_dep = dependency('', required: false)
endif

### crypto
if sys_windows == true
    bcrypt_dep = cc.find_library('bcrypt', required: true)
    xml_deps += bcrypt_dep
endif

### zlib
if want_zlib
    xml_deps += dependency('zlib')
endif

# icu
if want_icu
    icu_dep = dependency('icu-uc')
    defs = icu_dep.get_variable(pkgconfig: 'DEFS', default_value: '')
    if cc.has_argument(defs)
        libxml2_cflags += defs
    endif
    xml_deps += icu_dep
endif

### iconv
if want_iconv
    xml_deps += dependency('iconv')
endif

subdir('include/libxml')

# Set config_h after all subdirs and dependencies have set values

configure_file(output: 'config.h', configuration: config_h)

## libxml2 library

xml_src = [
    'buf.c',
    'chvalid.c',
    'dict.c',
    'entities.c',
    'encoding.c',
    'error.c',
    'globals.c',
    'hash.c',
    'list.c',
    'parser.c',
    'parserInternals.c',
    'SAX2.c',
    'threads.c',
    'tree.c',
    'uri.c',
    'valid.c',
    'xmlIO.c',
    'xmlmemory.c',
    'xmlstring.c',
]

xml_opt_src = [
    [want_c14n, ['c14n.c']],
    [want_catalog, ['catalog.c']],
    [want_debug, ['debugXML.c']],
    [want_html, ['HTMLparser.c', 'HTMLtree.c']],
    [want_http, ['nanohttp.c']],
    [want_modules, ['xmlmodule.c']],
    [want_output, ['xmlsave.c']],
    [want_pattern, ['pattern.c']],
    [want_reader, ['xmlreader.c']],
    [want_regexps, ['xmlregexp.c']],
    [want_relaxng, ['relaxng.c']],
    [want_schemas, ['xmlschemas.c', 'xmlschemastypes.c']],
    [want_schematron, ['schematron.c']],
    [want_writer, ['xmlwriter.c']],
    [want_xinclude, ['xinclude.c']],
    [want_xpath, ['xpath.c']],
    [want_xptr, ['xlink.c', 'xpointer.c']],
]

foreach file : xml_opt_src
    want = file[0]
    src = file[1]
    if want == true
        if src.length() > 1
            foreach s : src
                xml_src += s
            endforeach
        else
            xml_src += src
        endif
    endif
endforeach

xml_src_files = files(xml_src)
v_min_compat = 14
so_version = v_maj + v_min_compat
age = v_min - v_min_compat
darwin_compat = v_maj + v_min + 1
xml_lib = library(
    'xml2',
    xml_src_files,
    c_args: libxml2_cflags,
    dependencies: xml_deps,
    include_directories: config_dir,
    install: true,
    version: f'@so_version@.@age@.@v_mic@',
    soversion: so_version,
    darwin_versions: [ darwin_compat, f'@darwin_compat@.@v_mic@' ],
)

dep_inc = include_directories('include')
xml_dep = declare_dependency(include_directories: dep_inc, link_with: xml_lib, compile_args: dep_args)

meson.override_dependency('libxml-2.0', xml_dep)

## xmllint tool

xmllint = executable(
    'xmllint',
    files('xmllint.c', 'shell.c', 'lintmain.c'),
    dependencies: [xml_dep, xmllint_deps],
    include_directories: config_dir,
    install: true,
)

## xmlcatalog tool

if want_catalog and want_output
    xmlcatalog = executable(
        'xmlcatalog',
        files('xmlcatalog.c'),
        dependencies: [xml_dep, xmllint_deps],
        include_directories: config_dir,
        install: true,
    )
endif

## testdso module

testdso_mod = shared_module(
    'testdso',
    files('testdso.c'),
    build_rpath: get_option('libdir'),
    include_directories: config_dir,
    name_prefix: '',
)

## tests

checks = {
    'runsuite': [],
    'runtest': [threads_dep, zlib_dep],
    'runxmlconf': [],
# Disabled for now, see #694
#    'testModule': [],
    'testapi': [],
    'testcatalog': [],
    'testchar': [],
    'testdict': [],
    'testlimits': [],
    'testparser': [],
    'testrecurse': [],
}

foreach check, deps : checks
    exe = executable(
        check,
        files(check + '.c'),
        dependencies: [deps, xml_dep],
        include_directories: config_dir,
        build_by_default: false,
    )
    if check != 'testlimits'
        test(check, exe, timeout: 0, workdir: meson.current_source_dir())
    endif
endforeach

add_test_setup(
  'valgrind',
  exe_wrapper: [
    'valgrind', '--quiet', '--leak-check=full', '--error-exitcode=1',
  ],
  # default timeout in meson is 30s
  timeout_multiplier: 4,
)

if want_output
    sh = find_program('sh', required: false)

    if sh.found()
        if want_debug
            test('xmllint shell', sh,
                 args: [ 'test/scripts/test.sh', xmllint.full_path() ],
                 workdir: meson.current_source_dir())
        endif
        if want_catalog
            test('xmlcatalog shell', sh,
                 args: [ 'test/catalogs/test.sh', xmlcatalog.full_path() ],
                 workdir: meson.current_source_dir())
            test('xmlcatalog shell SGML', sh,
                 args: [ 'test/catalogs/test_sgml.sh', xmlcatalog.full_path() ],
                 workdir: meson.current_source_dir())
        endif
    endif
endif

subdir('example')

if want_docs or want_python
    subdir('doc')
endif

if want_python == true
    subdir('python')
endif

## pc files

pkgmod = import('pkgconfig')

pkgmod.generate(
    xml_lib,
    description: 'libXML library version2.',
    filebase: 'libxml-2.0',
    name: 'libXML',
    subdirs:  [meson.project_name()],
    variables: 'modules=' + want_modules.to_string('1', '0'),
)

## libxml2-config.cmake file

config_cmake = configuration_data()
config_cmake.set('LIBXML_MAJOR_VERSION', v_maj)
config_cmake.set('LIBXML_MINOR_VERSION', v_min)
config_cmake.set('LIBXML_MICRO_VERSION', v_mic)
config_cmake.set('INSTALL_BINDIR', dir_bin)
config_cmake.set('INSTALL_INCLUDEDIR', dir_include)
config_cmake.set('INSTALL_LIBDIR', dir_lib)
config_cmake.set('VERSION', meson.project_version())
config_cmake.set10('BUILD_SHARED_LIBS',
                   get_option('default_library') != 'static')
config_cmake.set10('WITH_ICONV', want_iconv)
config_cmake.set10('WITH_ICU', want_icu)
config_cmake.set10('WITH_MODULES', want_modules)
config_cmake.set10('WITH_THREADS', want_threads)
config_cmake.set10('WITH_ZLIB', want_zlib)
config_cmake.set('XML_CFLAGS', xml_cflags)
configure_file(
    input: 'libxml2-config.cmake.in',
    output: 'libxml2-config.cmake',
    configuration: config_cmake,
    install_dir: dir_lib / 'cmake' / meson.project_name(),
)

## xml2-config script

install_data(
    'xml2-config-meson',
    install_dir: dir_bin,
    install_mode: 'rwxr-xr-x',
    rename: 'xml2-config',
)

# summary

summary(
    {
        'OS': host_os,
        'c14n': want_c14n,
        'catalog': want_catalog,
        'debug': want_debug,
        'docs': want_docs,
        'history': want_history,
        'html': want_html,
        'http': want_http,
        'iconv': want_iconv,
        'icu': want_icu,
        'iso8859x': want_iso8859x,
        'legacy': want_legacy,
        'modules': want_modules,
        'output': want_output,
        'pattern': want_pattern,
        'push': want_push,
        'python': want_python,
        'reader': want_reader,
        'readline': want_readline,
        'regexps': want_regexps,
        'relaxng': want_relaxng,
        'sax1': want_sax1,
        'schemas': want_schemas,
        'schematron': want_schematron,
        'threads': want_threads,
        'thread-alloc': want_thread_alloc,
        'tls': want_tls,
        'valid': want_valid,
        'writer': want_writer,
        'xinclude': want_xinclude,
        'xpath': want_xpath,
        'xptr': want_xptr,
        'zlib': want_zlib,
    },
    section: 'Configuration Options Summary:',
)
