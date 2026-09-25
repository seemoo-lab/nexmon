project(
        'c-list',
        'c',
        default_options: [
                'c_std=c99',
        ],
        license: 'Apache',
        meson_version: '>=0.60.0',
        version: '3.1.0',
)
major = meson.project_version().split('.')[0]
project_description = 'Circular Intrusive Double Linked List Collection'

mod_pkgconfig = import('pkgconfig')

# See c-stdaux for details on these. We do not have c-stdaux as dependency, so
# we keep a duplicated set here, reduced to the minimum.
cflags = meson.get_compiler('c').get_supported_arguments(
        '-D_GNU_SOURCE',

        '-Wno-gnu-alignof-expression',
        '-Wno-maybe-uninitialized',
        '-Wno-unknown-warning-option',
        '-Wno-unused-parameter',

        '-Wno-error=type-limits',
        '-Wno-error=missing-field-initializers',

        '-Wdate-time',
        '-Wdeclaration-after-statement',
        '-Wlogical-op',
        '-Wmissing-include-dirs',
        '-Wmissing-noreturn',
        '-Wnested-externs',
        '-Wredundant-decls',
        '-Wshadow',
        '-Wstrict-aliasing=3',
        '-Wsuggest-attribute=noreturn',
        '-Wundef',
        '-Wwrite-strings',
)
add_project_arguments(cflags, language: 'c')

subdir('src')

meson.override_dependency('libclist-'+major, libclist_dep, static: true)
