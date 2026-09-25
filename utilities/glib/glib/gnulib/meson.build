# glib enables -Werror=format-nonliteral by default, but the embedded gnulib
# needs to handle user provided format strings.
extra_gnulib_args = cc.get_supported_arguments([
  '-Wno-format-nonliteral',
  '-Wno-float-conversion',
  '-Wno-duplicated-branches',
])

if host_system == 'windows' and cc.get_id() == 'clang'
  extra_gnulib_args += cc.get_supported_arguments([
    '-Wno-unused-but-set-variable',
    '-Wno-implicit-fallthrough',
  ])
endif

math_h_config = configuration_data ()

unneeded_funcs = [
  'ACOSF',
  'ACOSL',
  'ASINF',
  'ASINL',
  'ATAN2F',
  'ATANF',
  'ATANL',
  'CBRT',
  'CBRTF',
  'CBRTL',
  'CEIL',
  'CEILF',
  'CEILL',
  'COPYSIGN',
  'COPYSIGNF',
  'COPYSIGNL',
  'COSF',
  'COSHF',
  'COSL',
  'EXP2',
  'EXP2F',
  'EXP2L',
  'EXPF',
  'EXPL',
  'EXPM1',
  'EXPM1F',
  'EXPM1L',
  'FABSF',
  'FABSL',
  'FLOOR',
  'FLOORF',
  'FLOORL',
  'FMA',
  'FMAF',
  'FMAL',
  'FMOD',
  'FMODF',
  'FMODL',
  'FREXPF',
  'HYPOT',
  'HYPOTF',
  'HYPOTL',
  'ILOGB',
  'ILOGBF',
  'ILOGBL',
  'LDEXPF',
  'LOG',
  'LOG10',
  'LOG10F',
  'LOG10L',
  'LOG1P',
  'LOG1PF',
  'LOG1PL',
  'LOG2',
  'LOG2F',
  'LOG2L',
  'LOGB',
  'LOGBF',
  'LOGBL',
  'LOGF',
  'LOGL',
  'MODF',
  'MODFF',
  'MODFL',
  'POWF',
  'REMAINDER',
  'REMAINDERF',
  'REMAINDERL',
  'RINT',
  'RINTF',
  'RINTL',
  'ROUND',
  'ROUNDF',
  'ROUNDL',
  'SINF',
  'SINHF',
  'SINL',
  'SQRTF',
  'SQRTL',
  'TANF',
  'TANHF',
  'TANL',
  'TRUNC',
  'TRUNCF',
  'TRUNCL',
]

foreach f : unneeded_funcs
  math_h_config.set ('GNULIB_' + f, 0)
  # These are not used in practice, guarded by
  # the appropriate GNULIB_*, but meson config
  # processor doesn't know that
  math_h_config.set ('HAVE_' + f, 'variable not used')
  math_h_config.set ('REPLACE_' + f, 'variable not used')
endforeach

needed_funcs = [
  'FREXP',
  'FREXPL',
  'ISFINITE',
  'ISINF',
  'ISNAN',
  'ISNAND',
  'ISNANF',
  'ISNANL',
  'LDEXPL',
  'SIGNBIT',
]

foreach f : needed_funcs
  math_h_config.set ('GNULIB_' + f, 1)
endforeach

math_h_config.set ('GUARD_PREFIX', 'GL')

decls_for_unused_funcs = [
  'ACOSL',
  'ASINL',
  'ATANL',
  'CBRTF',
  'CBRTL',
  'CEILF',
  'CEILL',
  'COPYSIGNF',
  'COSL',
  'EXP2',
  'EXP2F',
  'EXP2L',
  'EXPL',
  'EXPM1L',
  'FLOORF',
  'FLOORL',
  'LOG10L',
  'LOG2',
  'LOG2F',
  'LOG2L',
  'LOGB',
  'LOGL',
  'REMAINDER',
  'REMAINDERL',
  'RINTF',
  'ROUND',
  'ROUNDF',
  'ROUNDL',
  'SINL',
  'SQRTL',
  'TANL',
  'TRUNC',
  'TRUNCF',
  'TRUNCL',
]

foreach f : decls_for_unused_funcs
  math_h_config.set ('HAVE_DECL_' + f, 0)
endforeach

decls_for_used_funcs = [
  'frexpl',
  'ldexpl',
]

foreach f : decls_for_used_funcs
  compiles = cc.compiles('''#include <math.h>
                            int main ()
                            {
                              (void) @0@;
                              return 0;
                            }
                         '''.format (f))
  math_h_config.set ('HAVE_DECL_' + f.to_upper (), compiles ? 1 : 0)
  set_variable ('have_decl_' + f, compiles ? true : false)
endforeach

nan_tmpl = '''#include <math.h>
              #if __GNUC__ >= 4
              # undef @0@
              # define @0@(x) @2@ ((@1@)(x))
              #else
              # undef @0@
              # define @0@(x) isnan ((@1@)(x))
              #endif
              double x;
              int main () {return @0@ (x);}
           '''

links = cc.links (nan_tmpl.format ('isnand', 'double', '__builtin_isnan'),
                  dependencies : [libm])

math_h_config.set ('HAVE_ISNAN', links ? 1 : 0)
set_variable ('have_isnan', links)
math_h_config.set ('HAVE_ISNAND', links ? 1 : 0)
set_variable ('have_isnand', links)

if links
  extra_gnulib_args += '-DHAVE_ISNAN_IN_LIBC'
  extra_gnulib_args += '-DHAVE_ISNAND_IN_LIBC'
endif

links = cc.links (nan_tmpl.format ('isnanf', 'float', '__builtin_isnanf'),
                  dependencies : [libm])

math_h_config.set ('HAVE_ISNANF', links ? 1 : 0)
set_variable ('have_isnanf', links)

if links
  extra_gnulib_args += '-DHAVE_ISNANF_IN_LIBC'
endif

links = cc.links (nan_tmpl.format ('isnanl', 'long double', '__builtin_isnanl'),
                  dependencies : [libm])

math_h_config.set ('HAVE_ISNANL', links ? 1 : 0)
set_variable ('have_isnanl', links)

if links
  extra_gnulib_args += '-DHAVE_ISNANL_IN_LIBC'
endif

math_h_config.set ('REPLACE_ISNAN', (have_isnand and have_isnanf and have_isnanl) ? 0 : 1)

other_needed_math_sources = []

if not (have_isnand and have_isnanf and have_isnanl)
  other_needed_math_sources += [ 'isnand.c', 'isnanf.c', 'isnanl.c' ]
endif

links = cc.links ('''#include <math.h>
                     double x;
                     int y;
                     int main () {return ldexp (x, y) < 1;}
                  ''',
                  dependencies : [libm])
math_h_config.set ('HAVE_LDEXP', links ? 1 : 0)
math_h_config.set ('HAVE_LDEXP_IN_LIBC', links ? 1 : 0)
set_variable ('have_ldexp', links)

links = cc.links ('''#include <math.h>
                     long double x;
                     int main () {return ldexpl (x, -1) > 0;}
                  ''',
                  dependencies : [libm])
math_h_config.set ('HAVE_LDEXPL', links ? 1 : 0)
math_h_config.set ('HAVE_LDEXPL_IN_LIBC', links ? 1 : 0)
set_variable ('have_ldexpl', links)

links = cc.links ('''#include <math.h>
                     double x;
                     int main () {int e; return frexp (x, &e) > 0;}
                  ''',
                  dependencies : [libm])
math_h_config.set ('HAVE_FREXP', links ? 1 : 0)
math_h_config.set ('HAVE_FREXP_IN_LIBC', links ? 1 : 0)
set_variable ('have_frexp', links)

links = cc.links ('''#include <math.h>
                     long double x;
                     int main () {int e; return frexpl (x, &e) > 0;}
                  ''',
                  dependencies : [libm])
math_h_config.set ('HAVE_FREXPL', links ? 1 : 0)
math_h_config.set ('HAVE_FREXPL_IN_LIBC', links ? 1 : 0)
set_variable ('have_frexpl', links)

math_h_config.set ('INCLUDE_NEXT_AS_FIRST_DIRECTIVE', 'include')
math_h_config.set ('NEXT_AS_FIRST_DIRECTIVE_MATH_H', '<math.h>')
math_h_config.set ('PRAGMA_COLUMNS', '')
math_h_config.set ('PRAGMA_SYSTEM_HEADER', '')

compiles = cc.compiles ('''
#include <math.h>
/* Solaris 10 has a broken definition of NAN.  Other platforms
        fail to provide NAN, or provide it only in C99 mode; this
        test only needs to fail when NAN is provided but wrong.  */
int main () {
         float f = 1.0f;
#ifdef NAN
         f = NAN;
#endif
         return f == 0;
}
''')
math_h_config.set ('REPLACE_NAN', compiles ? 0 : 1)

if have_frexp
  subdir ('gl_cv_func_frexp_works')
else
  gl_cv_func_frexp_works = false
  gl_cv_func_frexp_broken_beyond_repair = true
endif
if have_frexpl
  subdir ('gl_cv_func_frexpl_works')
else
  gl_cv_func_frexpl_works = false
  gl_cv_func_frexpl_broken_beyond_repair = true
endif

if not gl_cv_func_frexp_works and gl_cv_func_frexp_broken_beyond_repair
  error ('frexp() is missing or broken beyond repair, and we have nothing to replace it with')
endif
if not gl_cv_func_frexpl_works and gl_cv_func_frexpl_broken_beyond_repair
  error ('frexpl() is missing or broken beyond repair, and we have nothing to replace it with')
endif

math_h_config.set ('REPLACE_FREXP', gl_cv_func_frexp_works ? 0 : 1)
math_h_config.set ('REPLACE_FREXPL', gl_cv_func_frexpl_works ? 0 : 1)
math_h_config.set ('HAVE_DECL_FREXPL', gl_cv_func_frexpl_decl ? 0 : 1)

math_h_config.set ('REPLACE_ITOLD', 0)
math_h_config.set ('REPLACE_HUGE_VAL', 0)
math_h_config.set ('REPLACE_SIGNBIT_USING_GCC', 0)

if have_ldexpl
  subdir ('gl_cv_func_ldexpl_works')
else
  gl_cv_func_ldexpl_works = false
endif
math_h_config.set ('REPLACE_LDEXPL', gl_cv_func_ldexpl_works ? 0 : 1)
math_h_config.set ('HAVE_DECL_LDEXPL', gl_cv_func_ldexpl_decl ? 0 : 1)

inf_tmpl = '''#include <math.h>
              double x;
              int main () {return @0@ (x);}
           '''

# Some compilers may not have isfinite, isinf available
foreach f: ['isfinite', 'isinf', 'signbit']
  links = cc.links (inf_tmpl.format('@0@'.format(f)),
                    dependencies : [libm])
  math_h_config.set ('HAVE_@0@'.format(f.to_upper()), links ? 1 : 0)
  math_h_config.set ('HAVE_@0@_IN_LIBC'.format(f.to_upper()), links ? 1 : 0)
  math_h_config.set ('REPLACE_@0@'.format(f.to_upper()), links ? 0 : 1)
  set_variable ('have_@0@'.format(f), links)
  if not links
    if f == 'signbit'
      other_needed_math_sources += [ 'signbitd.c', 'signbitf.c', 'signbitl.c' ]
    elif f != 'isfinite' and f != 'isnan'
      other_needed_math_sources += [ '@0@.c'.format(f) ]
    endif
  endif
endforeach

math_h = configure_file (input: 'gnulib_math.h.in',
                         output: 'gnulib_math.h',
                         configuration: math_h_config)

gnulib_sources = ['asnprintf.c', 'printf.c', 'printf-args.c', 'printf-parse.c', 'printf-frexp.c', 'printf-frexpl.c', 'vasnprintf.c', 'xsize.c']

if not gl_cv_func_frexp_works
  gnulib_sources += ['frexp.c']
endif
if not gl_cv_func_frexpl_works
  gnulib_sources += ['frexpl.c']
endif

gnulib_sources += other_needed_math_sources

# g-gnulib.h includes glib.h that requires some headers to be generated
gnulib_sources += [
  'g-gnulib.h',
  glib_built_headers,
]

gnulib_lib = static_library('gnulib', gnulib_sources,
  dependencies : [libm],
  include_directories : [configinc, glibinc, include_directories ('.')],
  pic : true,
  c_args : ['-DGCC_LINT=1', '-DLIBDIR="@0@"'.format(glib_libdir), '-DG_LOG_DOMAIN="GLib"',
            glib_c_args_internal, extra_gnulib_args],
  gnu_symbol_visibility : 'hidden',
)

gnulib_libm_dependency = [libm]
