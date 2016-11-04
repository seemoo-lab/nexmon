# -*- Makefile -*-

AM_CPPFLAGS  = -Wall -I${top_srcdir}/include -I${top_builddir}/include -D_GNU_SOURCE -DPKGLIBDIR=\"$(pkglibdir)\" -DSYSCONFDIR=\"$(sysconfdir)\" -rdynamic
AM_LDFLAGS = -L${top_builddir}/lib -ldl

#nobase_pkglib_LTLIBRARIES = cls/basic.la cls/ematch/cmp.la
#cls_basic_la_LDFLAGS = -module -version-info 2:0:0
#cls_ematch_cmp_la_LDFLAGS = -module -version-info 2:0:0

#cls/ematch_grammar.c: cls/ematch_grammar.l
#	$(LEX) --header-file=cls/ematch_grammar.h $(LFLAGS) -o $@ $^

#cls/ematch_syntax.c: cls/ematch_syntax.y
#	$(YACC) -d $(YFLAGS) -o $@ $^

#cls/pktloc_grammar.c: cls/pktloc_grammar.l
#	$(LEX) --header-file=cls/pktloc_grammar.h $(LFLAGS) -o $@ $^

#cls/pktloc_syntax.c: cls/pktloc_syntax.y
#	$(YACC) -d $(YFLAGS) -o $@ $^

#CLEANFILES = \
#	cls/ematch_grammar.c cls/ematch_grammar.h \
#	cls/ematch_syntax.c cls/ematch_syntax.h \
#	cls/pktloc_grammar.c cls/pktloc_grammar.h \
#	cls/pktloc_syntax.c cls/pktloc_syntax.h

lib_LTLIBRARIES = \
	libnl-cli.la

libnl_cli_la_LDFLAGS = -version-info 2:0:0

libnl_cli_la_LIBADD  = ${top_builddir}/lib/libnl.la \
		       ${top_builddir}/lib/libnl-route.la \
		       ${top_builddir}/lib/libnl-nf.la \
		       ${top_builddir}/lib/libnl-genl.la

libnl_cli_la_SOURCES = \
	utils.c addr.c ct.c link.c neigh.c qdisc.c rule.c route.c
#	cls/ematch_syntax.c cls/ematch_grammar.c cls/ematch.c
#	cls/pktloc_syntax.c cls/pktloc_grammar.c cls/utils.c
