cmph_sources = [
  'bdz.c',
  'bdz_ph.c',
  'bmz8.c',
  'bmz.c',
  'brz.c',
  'buffer_entry.c',
  'buffer_manager.c',
  'chd.c',
  'chd_ph.c',
  'chm.c',
  'cmph.c',
  'cmph_structs.c',
  'compressed_rank.c',
  'compressed_seq.c',
  'fch_buckets.c',
  'fch.c',
  'graph.c',
  'hash.c',
  'jenkins_hash.c',
  'miller_rabin.c',
  'select.c',
  'vqueue.c',
  'vstack.c',
]

cmph_deps = [
  libglib_dep,
  libgobject_dep,
  libm,
]

custom_c_args = []

if cc.get_id() != 'msvc'
  custom_c_args = cc.get_supported_arguments([
    '-Wno-implicit-fallthrough',
    '-Wno-old-style-definition',
    '-Wno-suggest-attribute=noreturn',
    '-Wno-type-limits',
    '-Wno-undef',
    '-Wno-unused-parameter',
    '-Wno-cast-align',
    '-Wno-unused-function',
    '-Wno-return-type',
    '-Wno-sometimes-uninitialized',
  ])
endif

cmph = static_library('cmph',
  sources: cmph_sources,
  c_args: custom_c_args,
  dependencies: cmph_deps,
  gnu_symbol_visibility : 'hidden',
)

cmph_dep = declare_dependency(
  link_with: cmph,
  include_directories: include_directories('.'),
)

if cc.get_id() != 'msvc'
  custom_c_args = cc.get_supported_arguments([
    '-Wno-old-style-definition',
    '-Wno-type-limits',
  ])
endif
