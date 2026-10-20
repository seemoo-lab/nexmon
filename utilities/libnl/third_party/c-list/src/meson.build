#
# target: libclist.so
# (No .so is built so far, since we are header-only. This might change in the
#  future, if we add more complex list helpers.)
#

libclist_dep = declare_dependency(
        include_directories: include_directories('.'),
        version: meson.project_version(),
)

if not meson.is_subproject()
        install_headers('c-list.h')

        mod_pkgconfig.generate(
                description: project_description,
                filebase: 'libclist-'+major,
                name: 'libclist',
                version: meson.project_version(),
        )
endif

#
# target: test-*
#

test_api = executable('test-api', ['test-api.c'], dependencies: libclist_dep)
test('API Symbol Visibility', test_api)

test_basic = executable('test-basic', ['test-basic.c'], dependencies: libclist_dep)
test('Basic API Behavior', test_basic)

test_embed = executable('test-embed', ['test-embed.c'], dependencies: libclist_dep)
test('Embedded List Nodes', test_embed)
