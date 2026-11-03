# Copyright 2000-2025 Free Software Foundation, Inc.
# Contributed by the Pascaline and Caramba projects, INRIA.

# This file is part of the GNU MPFR Library.

# This Makefile.am is free software; the Free Software Foundation
# gives unlimited permission to copy and/or distribute it,
# with or without modifications, as long as this notice is preserved.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY, to the extent permitted by law; without
# even the implied warranty of MERCHANTABILITY or FITNESS FOR A
# PARTICULAR PURPOSE.


# The ".POSIX" line is needed in particular for GNU "make", so that
# recipes are invoked as if the shell had been passed the -e flag.
# But note that since GNU Automake adds non-comment lines before it
# in its generated Makefile, this does not ensure POSIX behavior with
# other "make" implementations.
.POSIX:

EXTRA_DIST = round_raw_generic.c jyn_asympt.c                           \
        add1sp1_extracted.c mul_1_extracted.c sub1sp1_extracted.c       \
        amd/mparam.h arm/mparam.h                                       \
        generic/coverage/mparam.h generic/mparam.h                      \
        mips/mparam.h powerpc64/mparam.h sparc64/mparam.h               \
        x86/mparam.h x86_64/core2/mparam.h x86_64/mparam.h

include_HEADERS = mpfr.h mpf2mpfr.h
nodist_include_HEADERS =

BUILT_SOURCES = get_patches.c mparam.h

lib_LTLIBRARIES = libmpfr.la

libmpfr_la_SOURCES = mpfr.h mpf2mpfr.h mpfr-cvers.h mpfr-gmp.h          \
mpfr-impl.h mpfr-intmax.h mpfr-longlong.h mpfr-sassert.h mpfr-thread.h  \
exceptions.c extract.c uceil_exp2.c                                     \
uceil_log2.c ufloor_log2.c add.c add1.c add_ui.c agm.c clear.c cmp.c    \
cmpabs.c cmp_si.c cmp_ui.c comparisons.c div_2exp.c div_2si.c           \
div_2ui.c div.c div_ui.c dot.c dump.c eq.c exp10.c exp2.c exp3.c exp.c  \
frac.c frexp.c get_d.c get_exp.c get_str.c init.c inp_str.c isinteger.c \
isinf.c isnan.c isnum.c const_log2.c log.c modf.c mul_2exp.c mul_2si.c  \
mul_2ui.c mul.c mul_ui.c neg.c next.c out_str.c printf.c vasprintf.c    \
const_pi.c pow.c pow_si.c pow_ui.c pow_sj.c pow_uj.c print_raw.c        \
print_rnd_mode.c                                                        \
reldiff.c round_prec.c set.c setmax.c setmin.c set_d.c set_dfl_prec.c   \
set_exp.c set_rnd.c set_f.c set_prc_raw.c set_prec.c set_q.c set_si.c   \
set_str.c set_str_raw.c set_ui.c set_z.c sqrt.c sqrt_ui.c sub.c sub1.c  \
sub_ui.c rint.c ui_div.c ui_sub.c urandom.c urandomb.c get_z_2exp.c     \
swap.c factorial.c cosh.c sinh.c tanh.c sinh_cosh.c acosh.c asinh.c     \
atanh.c atan.c cmp2.c exp_2.c asin.c const_euler.c cos.c sin.c tan.c    \
fma.c fms.c hypot.c log1p.c expm1.c log2.c log2p1.c log10.c log10p1.c   \
ui_pow.c ui_pow_ui.c minmax.c dim.c signbit.c copysign.c setsign.c      \
gmp_op.c init2.c acos.c sin_cos.c set_nan.c set_inf.c set_zero.c        \
powerof2.c gamma.c set_ld.c get_ld.c cbrt.c volatile.c fits_s.h         \
fits_sshort.c fits_sint.c fits_slong.c fits_u.h fits_ushort.c           \
fits_uint.c fits_ulong.c fits_uintmax.c fits_intmax.c get_si.c get_ui.c \
zeta.c cmp_d.c erf.c inits.c inits2.c clears.c sgn.c check.c sub1sp.c   \
version.c mpn_exp.c mpfr-gmp.c mp_clz_tab.c sum.c add1sp.c              \
free_cache.c si_op.c cmp_ld.c set_ui_2exp.c set_si_2exp.c set_uj.c      \
set_sj.c get_sj.c get_uj.c get_z.c iszero.c cache.c sqr.c               \
int_ceil_log2.c isqrt.c strtofr.c pow_z.c logging.c mulders.c get_f.c   \
round_p.c erfc.c atan2.c atan2u.c subnormal.c const_catalan.c root.c    \
gen_inverse.h sec.c csc.c cot.c eint.c sech.c csch.c coth.c             \
round_near_x.c constant.c abort_prec_max.c stack_interface.c lngamma.c  \
zeta_ui.c set_d64.c get_d64.c jn.c yn.c rem1.c add_d.c                  \
sub_d.c d_sub.c mul_d.c div_d.c d_div.c li2.c rec_sqrt.c min_prec.c     \
buildopt.c digamma.c bernoulli.c isregular.c set_flt.c get_flt.c        \
scale2.c set_z_2exp.c ai.c gammaonethird.c ieee_floats.h                \
grandom.c fpif.c set_float128.c get_float128.c rndna.c nrandom.c        \
random_deviate.h random_deviate.c erandom.c mpfr-mini-gmp.c             \
mpfr-mini-gmp.h fmma.c log_ui.c gamma_inc.c ubf.c invert_limb.h         \
invsqrt_limb.h beta.c odd_p.c get_q.c pool.c total_order.c set_d128.c   \
get_d128.c nbits_ulong.c cmpabs_ui.c sinu.c cosu.c tanu.c fmod_ui.c     \
acosu.c asinu.c atanu.c compound.c exp2m1.c exp10m1.c powr.c

nodist_libmpfr_la_SOURCES = $(BUILT_SOURCES)

libmpfr_la_LIBADD = @LIBOBJS@

# Libtool -version-info CURRENT[:REVISION[:AGE]] for libmpfr.la
#
# 1. No interfaces changed, only implementations (good):
#    ==> Increment REVISION.
# 2. Interfaces added, none removed (good):
#    ==> Increment CURRENT, increment AGE, set REVISION to 0.
# 3. Interfaces removed or changed (BAD, breaks upward compatibility):
#    ==> Increment CURRENT, set AGE and REVISION to 0.
#
# For each new MPFR major-minor version, add a new line below with the
# corresponding -version-info CURRENT and AGE. This will be checked by
# the tools/ck-version-info script.
#
#         MPFR     -version-info
#        2.1.x      -
#        2.2.x    1:x:0
#        2.3.x    2:x:1
#        2.4.x    3:x:2
#        3.0.x    4:x:0
#        3.1.x    5:x:1
#        4.0.x    6:x:0
#        4.1.x    7:x:1
#        4.2.x    8:x:2
libmpfr_la_LDFLAGS = $(MPFR_LDFLAGS) $(LIBMPFR_LDFLAGS) -version-info 8:2:2

GET_PATCHES_SH = $(top_srcdir)/tools/get_patches.sh
PATCHES = $(top_srcdir)/PATCHES
get_patches.c: $(GET_PATCHES_SH) $(PATCHES)
	$(GET_PATCHES_SH) $(PATCHES) > $@ || { rm -f $@; exit 1; }

if MINI_GMP

# The mini-gmp.{c,h} files are external files, not part of MPFR, thus they
# must not be put in the tarballs by "make dist". Hence the use of nodist_
# on the corresponding sources (this is needed even when --with-mini-gmp
# has not been used).
nodist_include_HEADERS += mini-gmp.h

# The noinst_ below seems OK as libminigmp should be included in libmpfr,
# and this is confirmed by ldd (replacing noinst_ by lib_ does not seem to
# do this and makes linking of the test programs fail).
noinst_LTLIBRARIES = libminigmp.la
nodist_libminigmp_la_SOURCES = mini-gmp.h mini-gmp.c
libmpfr_la_LIBADD += libminigmp.la

DISTCLEANFILES = $(nodist_libminigmp_la_SOURCES)

endif

# For check-gmp-symbols
GMPC = $(top_builddir)/src/gmp.c
GMPI = $(top_builddir)/src/gmp.i

# For check-gmp-symbols and check-exported-symbols (if the library does
# not have this name, e.g. on some platforms or when the shared library
# is disabled, these rules do nothing).
LIBMPFRSO = $(top_builddir)/src/.libs/libmpfr.so

# Check that MPFR does not use GMP internal symbols. Of course, do not run
# this rule if you use --with-gmp-build or --enable-gmp-internals. This
# test does nothing if --disable-shared has been used.
# Note: we use a temporary file $(GMPC) instead of "| $(COMPILE) -E -"
# because the latter is not supported by all compilers (at least under
# MS Windows).
check-gmp-symbols: $(LTLIBRARIES)
	if [ -f "$(LIBMPFRSO)" ]; then \
	  printf "#include <%s.h>\n" stdarg stdio gmp > $(GMPC) && \
	  $(COMPILE) -E $(GMPC) > $(GMPI) || exit 1; \
	  internals=`$(NM) -u "$(LIBMPFRSO)" | \
	    $(SED) -n 's/^ *U \(__gmp.*\)/\1/p' | \
	    while read s; \
	    do \
	      $(GREP) -q "$$s[ (;]" $(GMPI) || echo "$$s"; \
	    done`; \
	  if [ -n "$$internals" ]; then \
	    echo "Internal GMP symbols:" $$internals; \
	    exit 1; \
	  fi; \
	  rm $(GMPC) $(GMPI); \
	fi

# Check that MPFR does not define symbols with a GMP reserved prefix.
# For instance, with r11968, and
#   ./configure --with-gmp-build=... CC=tcc
# the symbol __gmpn_clz_tab is defined, which is wrong.
# Note: the "$(GREP) -v '@plt$$'" below is for tcc.
check-exported-symbols: $(LTLIBRARIES)
	if [ -f "$(LIBMPFRSO)" ]; then \
	  gsymbols=`$(NM) -gP "$(LIBMPFRSO)" | perl -ne \
	    '/^(__gmp[a-z]?_[_0-9A-Za-z]*) +[A-TV-Z]/ and print " $$1"' | \
	    { $(GREP) -v '@plt$$' || true ; }`; \
	  if [ -n "$$gsymbols" ]; then \
	    echo "MPFR defines symbols with a GMP reserved prefix:$$gsymbols"; \
	    exit 1; \
	  fi; \
	fi

CLEANFILES = get_patches.c $(GMPC) $(GMPI)

.PHONY: check-gmp-symbols check-exported-symbols
