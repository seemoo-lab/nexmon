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

AUTOMAKE_OPTIONS = 1.6 gnu

# For Git:
#   * To avoid escaping the MPFR source tree, we check whether
#     $(top_srcdir)/.git is readable (note that this is not
#     necessarily a directory: in case of a "linked worktree",
#     this is a file; see the git-worktree(1) man page).
#     In particular, if one extracts an MPFR tarball within a
#     Git working tree (e.g. after "make dist"), then running
#     "make check" in this extracted source tree will work as
#     expected, giving correct version information.
#   * Additional test with "git rev-parse ...", which should
#     silently fail if git is not installed.
#   * See version-ext.sh for information on its output.
output_info = { \
  if [ -d $(top_srcdir)/.svn ]; then \
    output=`svnversion $(top_srcdir) 2> /dev/null`; \
    if [ -n "$$output" ]; then \
      echo "[svnversion] $$output"; \
    fi; \
  fi; \
  if [ -r $(top_srcdir)/.git ] && \
     [ "x`git rev-parse --is-inside-work-tree 2> /dev/null`" = xtrue ]; then \
    gitv=`GREP=$(GREP) SED=$(SED) $(top_srcdir)/version-ext.sh`; \
    echo "[git] $$gitv"; \
  fi; \
  $(GREP) -v '^\(PASS\|FAIL\) ' tversion.log 2> /dev/null || true; \
}

TESTS_NO_TVERSION = tabort_prec_max tassert tabort_defalloc1            \
     tabort_defalloc2 talloc tinternals tinits tisqrt tsgn tcheck       \
     tisnan texceptions tset_exp tset mpf_compat mpfr_compat reuse      \
     tabs tacos tacosh tacosu tadd tadd1sp tadd_d tadd_ui tagm tai      \
     talloc-cache tasin tasinh tasinu tatan tatanh tatanu tatan2u taway \
     tbeta tbuildopt tcan_round tcbrt tcmp tcmp2 tcmp_d tcmp_ld tcmp_ui \
     tcmpabs tcomparisons tcompound tconst_catalan tconst_euler         \
     tconst_log2 tconst_pi                                              \
     tcopysign tcos tcosh tcosu tcot tcoth tcsc tcsch td_div td_sub     \
     tdigamma tdim tdiv tdiv_d tdiv_ui tdot teint teq terandom          \
     terandom_chisq terf texp texp10 texp2 texpm1 texp10m1 texp2m1      \
     tfactorial tfits tfma tfmma tfmod tfms tfpif tfprintf tfrac tfrexp \
     tgamma tgamma_inc tget_d tget_d_2exp tget_f tget_flt tget_ld_2exp  \
     tget_q tget_set_d64 tget_set_d128 tget_sj tget_str tget_z tgmpop   \
     tgrandom thyperbolic thypot tinp_str                               \
     tj0 tj1 tjn tl2b tlgamma tli2 tlngamma tlog tlog10 tlog10p1 tlog1p \
     tlog2 tlog2p1                                                      \
     tlog_ui tmin_prec tminmax tmodf tmul tmul_2exp tmul_d tmul_ui      \
     tnext tnrandom tnrandom_chisq tout_str toutimpl tpow tpow3 tpowr   \
     tpow_all tpow_z tprec_round tprintf trandom trandom_deviate        \
     trec_sqrt treldiff tremquo trint trndna troot trootn_si trootn_ui  \
     tsec tsech tset_d tset_f tset_float128 tset_ld tset_q              \
     tset_si tset_sj tset_str tset_z tset_z_2exp tsi_op tsin tsin_cos   \
     tsinh tsinh_cosh tsinu tsprintf tsqr tsqrt tsqrt_ui tstckintc      \
     tstdint tstrtofr tsub tsub1sp tsub_d tsub_ui tsubnormal tsum tswap \
     ttan ttanh ttanu ttotal_order ttrunc tui_div tui_pow tui_sub       \
     turandom tvalist ty0 ty1 tyn tzeta tzeta_ui

check_PROGRAMS = tversion $(TESTS_NO_TVERSION)

LOGS_NO_TVERSION = $(TESTS_NO_TVERSION:%=%.log)

# Before Automake 1.13, we ran tversion at the beginning and at the end
# of the tests, and output from tversion appeared at the same place as
# the tests results (make output). With Automake 1.13+, the tests are
# parallelized by default and their output is sent to log files instead
# of the make output, so that the user could no longer see information
# from tversion. To mimic the old behavior, we now run tversion at the
# beginning and output the contents. Also output the contents after the
# tests, but when a test fails, they will not be shown.
tversion_special: tversion.log
	@$(output_info)
$(LOGS_NO_TVERSION): tversion_special
check:
	@$(output_info)

AM_CPPFLAGS = -DSRCDIR='"$(srcdir)"'

EXTRA_DIST = tgeneric.c tgeneric_ui.c mpf_compat.h inp_str.dat tmul.dat \
	tfpif_r1.dat tfpif_r2.dat

CLEANFILES = tfpif_rw.dat tfprintf_out.txt tout_str_out.txt toutimpl_out.txt tprintf_out.txt

LDADD = libfrtests.la $(MPFR_LIBM) $(MPFR_LIBQUADMATH) $(top_builddir)/src/libmpfr.la
AM_CPPFLAGS += -I$(top_srcdir)/src -I$(top_builddir)/src

# LOADLIBES (documented in the "GNU make" manual and equivalent to LDLIBS)
# enables to compile a program foo.c in the test directory by simply doing
# "make foo".
# Warning! This is not guaranteed to work, as libtool is not used. In
# particular, this may not work as expected under GNU/Linux if --with-gmp
# has been used, unless the directory is in your $LD_LIBRARY_PATH.
# Moreover, dependencies are not tracked. Thus you may want to run
# "make tversion" (for instance) just before, to make sure that every
# dependency has been rebuilt.
LOADLIBES=$(DEFS) $(AM_CPPFLAGS) $(CPPFLAGS) $(CFLAGS) $(top_builddir)/tests/.libs/libfrtests.a $(top_builddir)/src/.libs/libmpfr.a $(LIBS) $(MPFR_LIBM)

check_LTLIBRARIES = libfrtests.la
libfrtests_la_SOURCES = mpfr-test.h memory.c rnd_mode.c tests.c cmp_str.c random2.c

$(top_builddir)/src/libmpfr.la:
	cd $(top_builddir)/src && $(MAKE) $(AM_MAKEFLAGS) libmpfr.la

TESTS = $(check_PROGRAMS)

# Set DISPLAY='' as it is not needed and it can yield error messages when
# running the tests with LOG_COMPILER=wine (for builds with MinGW). See:
#   https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=914822
#   https://bugs.debian.org/cgi-bin/bugreport.cgi?bug=914949
# $(VALGRIND) is still there to be able to use a wrapper such as valgrind
# in case old Automake. With Automake 1.13+, where a parallel test harness
# is generated, use LOG_COMPILER instead. For additional information, see:
# https://www.gnu.org/software/automake/manual/automake.html#Parallel-Test-Harness
#
# Note: We do not use "env -u DISPLAY" as the -u option is a GNU extension
# and not supported by NetBSD. Setting DISPLAY='' is not equivalent, but
# it also avoids the bogus "Invalid MIT-MAGIC-COOKIE-1 key" error message
# that can make the parsing of the output fail.
TESTS_ENVIRONMENT = env DISPLAY='' MPFR_QUIET=1 $(VALGRIND)

# The -no-install option prevents libtool from generating wrapper scripts
# for the tests.
# This is useful to easily run individual test scripts under valgrind or gdb.
# See discussion
#   https://lists.gnu.org/archive/html/bug-gnulib/2011-08/msg00247.html
# and
#   https://lists.gnu.org/archive/html/bug-gnulib/2011-09/msg00081.html
# in particular.
#
# The -L$(top_builddir)/src/.libs option is necessary for some platforms,
# such as HP-UX, when --with-gmp or --with-gmp-lib is used and an old MPFR
# library is already installed in the corresponding lib directory: its
# purpose is to make sure that the local .libs comes first in the library
# search path (otherwise the tests are linked against the old MPFR library
# by the LINK command -- see the generated Makefile). See:
#   https://sympa.inria.fr/sympa/arc/mpfr/2011-10/msg00042.html
#   https://sympa.inria.fr/sympa/arc/mpfr/2011-10/msg00043.html
#   https://sympa.inria.fr/sympa/arc/mpfr/2011-10/msg00044.html
#   https://sympa.inria.fr/sympa/arc/mpfr/2011-10/msg00066.html
#   https://sympa.inria.fr/sympa/arc/mpfr/2011-10/msg00065.html
# and
#   https://debbugs.gnu.org/cgi/bugreport.cgi?bug=9728
#   https://debbugs.gnu.org/cgi/bugreport.cgi?bug=18662
#
AM_LDFLAGS = -no-install -L$(top_builddir)/src/.libs

.PHONY: tversion_special
