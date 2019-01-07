# Additional editing of Makefiles
/@GMSGFMT@/ s,\$GMSGFMT,msgfmt,
/@MSGFMT@/ s,\$MSGFMT,msgfmt,
/@XGETTEXT@/ s,\$XGETTEXT,xgettext,
/ac_given_INSTALL=/,/^CEOF/ {
  /^CEOF$/ i\
# DJGPP specific Makefile changes.\
  /^aliaspath[ 	]*=/s,:,";",g\
  /^lispdir[ 	]*=/ c\\\\\
lispdir = \\$(prefix)/gnu/emacs/site-lisp\
  /TEXINPUTS[ 	]*=/s,:,";",g\
  /PATH[ 	]*=/s,:,";",g\
  s,\\.new\\.,_new.,g\
  s,\\.old\\.,_old.,g\
  s,\\.tab\\.c,_tab.c,g\
  s,\\.tab\\.h,_tab.h,g\
  s,\\([1-9]\\)\\.html\\.in,\\1hi,g\
  s,\\([1-9]\\)\\.html,\\1-html,g\
  s,\\([1-9]\\)\\.in,\\1-in,g\
  s,\\.sh\\.in,.sh-in,g\
  s,config\\.h\\.in,config.h-in,g\
  s,COPYING.LIB-2.0,COPYING_LIB.20,g\
  s,COPYING.LIB-2.1,COPYING_LIB.21,g\
  s,gettext_1.html,gettext.1-html,g\
  s,gettext_10.html,gettext.10-html,g\
  s,gettext_11.html,gettext.11-html,g\
  s,gettext_12.html,gettext.12-html,g\
  s,gettext_13.html,gettext.13-html,g\
  s,gettext_14.html,gettext.14-html,g\
  s,gettext_15.html,gettext.15-html,g\
  s,gettext_16.html,gettext.16-html,g\
  s,gettext_2.html,gettext.2-html,g\
  s,gettext_3.html,gettext.3-html,g\
  s,gettext_4.html,gettext.4-html,g\
  s,gettext_5.html,gettext.5-html,g\
  s,gettext_6.html,gettext.6-html,g\
  s,gettext_7.html,gettext.7-html,g\
  s,gettext_8.html,gettext.8-html,g\
  s,gettext_9.html,gettext.9-html,g\
  s,gettext_foot.html,gettext.foot-html,g\
  s,gettext_toc.html,gettext.toc-html,g\
  s,javacomp\\.sh\\.in,javacomp.sh-in,g\
  s,javaexec\\.sh\\.in,javaexec.sh-in,g\
  s,stdbool\\.h\\.in,stdbool.h-in,g\
  s,gettext.1.in,gettext.1-in,g\
  s,ngettext.1.in,ngettext.1-in,g\
  s,gettext.3.in,gettext.3-in,g\
  s,ngettext.3.in,ngettext.3-in,g\
  s,textdomain.3.in,textdomain.3-in,g\
  s,bindtextdomain.3.in,bindtextdomain.3-in,g\
  s,bind_textdomain_codeset.3.in,bind_textdomain_codeset.3-in,g\
  s,gettext.1.html.in,gettext.1hin,g\
  s,ngettext.1.html.in,ngettext.1hin,g\
  s,msgcmp.1.html,msgcmp.1-html,g\
  s,msgfmt.1.html,msgfmt.1-html,g\
  s,msgmerge.1.html,msgmerge.1-html,g\
  s,msgunfmt.1.html,msgunfmt.1-html,g\
  s,xgettext.1.html,xgettext.1-html,g\
  s,msgattrib.1.html,msgattrib.1-html,g\
  s,msgcat.1.html,msgcat.1-html,g\
  s,msgcomm.1.html,msgcomm.1-html,g\
  s,msgconv.1.html,msgconv.1-html,g\
  s,msgen.1.html,msgen.1-html,g\
  s,msgexec.1.html,msgexec.1-html,g\
  s,msgfilter.1.html,msgfilter.1-html,g\
  s,msggrep.1.html,msggrep.1-html,g\
  s,msginit.1.html,msginit.1-html,g\
  s,msguniq.1.html,msguniq.1-html,g\
  s,gettext.3.html,gettext.3-html,g\
  s,ngettext.3.html,ngettext.3-html,g\
  s,textdomain.3.html,textdomain.3-html,g\
  s,bindtextdomain.3.html,bindtextdomain.3-html,g\
  s,bind_textdomain_codeset.3.html,bind_textdomain_codeset.3-html,g\
  s,Makefile\\.in\\.in,Makefile.in-in,g\
  s,format-librep.c,format_librep.c,g\
  s,format-pascal.c,format_pascal.c,g\
  s,blue-ball.gif,b-ball.gif,g\
  s,cyan-ball.gif,c-ball.gif,g\
  s,green-ball.gif,g-ball.gif,g\
  s,magenta-ball.gif,m-ball.gif,g\
  s,red-ball.gif,r-ball.gif,g\
  s,yellow-ball.gif,y-ball.gif,g\
  s,constructors.gif,ctors.gif,g\
  s,variables.gif,vars.gif,g\
  s,package-frame.html,package_frame.html,g\
  s,package-tree.html,package_tree.html,g\
  s,gettext-1,gettext.1,g\
  s,gettext-2,gettext.2,g\
  s,msgattrib-1,msgattrib.1,g\
  s,msgattrib-2,msgattrib.2,g\
  s,msgattrib-3,msgattrib.3,g\
  s,msgattrib-4,msgattrib.4,g\
  s,msgattrib-5,msgattrib.5,g\
  s,msgattrib-6,msgattrib.6,g\
  s,msgattrib-7,msgattrib.7,g\
  s,msgattrib-8,msgattrib.8,g\
  s,msgattrib-9,msgattrib.9,g\
  s,msgattrib-10,msgattrib.10,g\
  s,msgattrib-11,msgattrib.11,g\
  s,msgattrib-12,msgattrib.12,g\
  s,msgattrib-13,msgattrib.13,g\
  s,msgattrib-14,msgattrib.14,g\
  s,msgcat-1,msgcat.1,g\
  s,msgcat-2,msgcat.2,g\
  s,msgcat-3,msgcat.3,g\
  s,msgcat-4,msgcat.4,g\
  s,msgcat-5,msgcat.5,g\
  s,msgcat-6,msgcat.6,g\
  s,msgcat-7,msgcat.7,g\
  s,msgcmp-1,msgcmp.1,g\
  s,msgcmp-2,msgcmp.2,g\
  s,msgcomm-1,msgcomm.1,g\
  s,msgcomm-2,msgcomm.2,g\
  s,msgcomm-3,msgcomm.3,g\
  s,msgcomm-4,msgcomm.4,g\
  s,msgcomm-5,msgcomm.5,g\
  s,msgcomm-6,msgcomm.6,g\
  s,msgcomm-7,msgcomm.7,g\
  s,msgcomm-8,msgcomm.8,g\
  s,msgcomm-9,msgcomm.9,g\
  s,msgcomm-10,msgcomm.10,g\
  s,msgcomm-11,msgcomm.11,g\
  s,msgcomm-12,msgcomm.12,g\
  s,msgcomm-13,msgcomm.13,g\
  s,msgcomm-14,msgcomm.14,g\
  s,msgcomm-15,msgcomm.15,g\
  s,msgcomm-16,msgcomm.16,g\
  s,msgcomm-17,msgcomm.17,g\
  s,msgcomm-18,msgcomm.18,g\
  s,msgcomm-19,msgcomm.19,g\
  s,msgcomm-20,msgcomm.20,g\
  s,msgcomm-21,msgcomm.21,g\
  s,msgcomm-22,msgcomm.22,g\
  s,msgcomm-23,msgcomm.23,g\
  s,msgconv-1,msgconv.1,g\
  s,msgconv-2,msgconv.2,g\
  s,msgconv-3,msgconv.3,g\
  s,msgen-1,msgen.1,g\
  s,msgexec-1,msgexec.1,g\
  s,msgexec-2,msgexec.2,g\
  s,msgfilter-1,msgfilter.1,g\
  s,msgfilter-2,msgfilter.2,g\
  s,msgfmt-1,msgfmt.1,g\
  s,msgfmt-2,msgfmt.2,g\
  s,msgfmt-3,msgfmt.3,g\
  s,msgfmt-4,msgfmt.4,g\
  s,msgfmt-5,msgfmt.5,g\
  s,msgfmt-6,msgfmt.6,g\
  s,msgfmt-7,msgfmt.7,g\
  s,msgfmt-8,msgfmt.8,g\
  s,msgfmt-9,msgfmt.9,g\
  s,msgfmt-10,msgfmt.10,g\
  s,msggrep-1,msggrep.1,g\
  s,msggrep-2,msggrep.2,g\
  s,msggrep-3,msggrep.3,g\
  s,msggrep-4,msggrep.4,g\
  s,msgmerge-1,msgmerge.1,g\
  s,msgmerge-2,msgmerge.2,g\
  s,msgmerge-3,msgmerge.3,g\
  s,msgmerge-4,msgmerge.4,g\
  s,msgmerge-5,msgmerge.5,g\
  s,msgmerge-6,msgmerge.6,g\
  s,msgmerge-7,msgmerge.7,g\
  s,msgmerge-8,msgmerge.8,g\
  s,msgmerge-9,msgmerge.9,g\
  s,msgmerge-10,msgmerge.10,g\
  s,msgmerge-11,msgmerge.11,g\
  s,msgmerge-12,msgmerge.12,g\
  s,msgmerge-13,msgmerge.13,g\
  s,msgmerge-14,msgmerge.14,g\
  s,msgmerge-15,msgmerge.15,g\
  s,msgmerge-16,msgmerge.16,g\
  s,msgmerge-17,msgmerge.17,g\
  s,msgmerge-18,msgmerge.18,g\
  s,msgmerge-19,msgmerge.19,g\
  s,msgmerge-20,msgmerge.20,g\
  s,msgunfmt-1,msgunfmt.1,g\
  s,msguniq-1,msguniq.1,g\
  s,msguniq-2,msguniq.2,g\
  s,msguniq-3,msguniq.3,g\
  s,xgettext-1,xgettext.1,g\
  s,xgettext-2,xgettext.2,g\
  s,xgettext-3,xgettext.3,g\
  s,xgettext-4,xgettext.4,g\
  s,xgettext-5,xgettext.5,g\
  s,xgettext-6,xgettext.6,g\
  s,xgettext-7,xgettext.7,g\
  s,xgettext-8,xgettext.8,g\
  s,xgettext-9,xgettext.9,g\
  s,xgettext-10,xgettext.10,g\
  s,xgettext-11,xgettext.11,g\
  s,xgettext-12,xgettext.12,g\
  s,xgettext-13,xgettext.13,g\
  s,xgettext-14,xgettext.14,g\
  s,xgettext-15,xgettext.15,g\
  s,xgettext-16,xgettext.16,g\
  s,xgettext-17,xgettext.17,g\
  s,format-c-1,format/c.1,g\
  s,format-c-2,format/c.2,g\
  s,format-elisp-1,format/elisp.1,g\
  s,format-elisp-2,format/elisp.2,g\
  s,format-java-1,format/java.1,g\
  s,format-java-2,format/java.2,g\
  s,format-librep-1,format/librep.1,g\
  s,format-librep-2,format/librep.2,g\
  s,format-lisp-1,format/lisp.1,g\
  s,format-lisp-2,format/lisp.2,g\
  s,format-python-1,format/python.1,g\
  s,format-python-2,format/python.2,g\
  s,format-pascal-1,format/pascal.1,g\
  s,format-pascal-2,format/pascal.2,g\
  s,format-ycp-1,format/ycp.1,g\
  s,format-ycp-2,format/ycp.2,g\
  s,lang-c++,lang-cxx,g\
  s,rpath-1a,rpath/1a,g\
  s,rpath-1b,rpath/1b,g\
  s,rpath-2aaa,rpath/2aaa,g\
  s,rpath-2aab,rpath/2aab,g\
  s,rpath-2aac,rpath/2aac,g\
  s,rpath-2aad,rpath/2aad,g\
  s,rpath-2aba,rpath/2aba,g\
  s,rpath-2abb,rpath/2abb,g\
  s,rpath-2abc,rpath/2abc,g\
  s,rpath-2abd,rpath/2abd,g\
  s,rpath-2baa,rpath/2baa,g\
  s,rpath-2bab,rpath/2bab,g\
  s,rpath-2bac,rpath/2bac,g\
  s,rpath-2bad,rpath/2bad,g\
  s,rpath-2bba,rpath/2bba,g\
  s,rpath-2bbb,rpath/2bbb,g\
  s,rpath-2bbc,rpath/2bbc,g\
  s,rpath-2bbd,rpath/2bbd,g\
  s,xg-test1.ok.po,xg-test1.ok-po,g\
  s,rpath-1,rpath/1,g\
  s,rpath-2_a,rpath/2_a,g\
  s,rpath-2_b,rpath/2_b,g\
  s,rpath-2.README,rpath/2.README,g\
  s,rpathcfg.sh,rpathcfg.sh,g\
  s,gettext_\\*\\.,gettext.*-,g\
  s,format-librep,format_librep,g\
  s,format-pascal,format_pascal,g\
  /^TESTS[ 	]*=/,/^$/ s,plural-\\([1-9]\\+\\),plural.\\1,g\
  /^install-info-am:/,/^$/ {\
    /@list=/ s,\\\$(INFO_DEPS),& gettext.i,\
    s,file-\\[0-9\\]\\[0-9\\],& \\$\\$file[0-9] \\$\\$file[0-9][0-9],\
  }\
  /^iso-639\\.texi[ 	]*:.*$/ {\
    s,iso-639,\\$(srcdir)/&,g\
    s,ISO_639,\\$(srcdir)/&,\
  }\
  /^iso-3166\\.texi[ 	]*:.*$/ {\
    s,iso-3166,\\$(srcdir)/&,g\
    s,ISO_3166,\\$(srcdir)/&,\
  }\
  /^# Some rules for yacc handling\\./,$ {\
    /\\\$(YACC)/ a\\\\\
	-@test -f y.tab.c && mv -f y.tab.c y_tab.c\\\\\
	-@test -f y.tab.h && mv -f y.tab.h y_tab.h\
  }\
  /^POTFILES:/,/^$/ s,\\\$@-t,t-\\$@,g\
  s,basename\\.o,,g\
  s,po-gram-gen2\\.h,po-gram_gen2.h,g\
  /^Makefile[ 	]*:/,/^$/ {\
    /CONFIG_FILES=/ s,\\\$(subdir)/\\\$@\\.in,&:\\$(subdir)/\\$@.in-in,\
  }\
  /html:/ s,split$,monolithic,g\
  /^TEXI2HTML[ 	]*=/ s,=[ 	]*,&-,
}

# javacomp.sh is renamed to javacomp.sh-in,
# javaexec.sh is renamed to javaexec.sh-in,
# Makefile.in.in is renamed to Makefile.in-in...
/^CONFIG_FILES=/,/^EOF/ {
  s|lib/javacomp\.sh|&:lib/javacomp.sh-in|
  s|lib/javaexec\.sh|&:lib/javaexec.sh-in|
  s|po/Makefile\.in|&:po/Makefile.in-in|
}

# ...and config.h.in into config.h-in
/^ *CONFIG_HEADERS=/,/^EOF/ {
  s|config\.h|&:config.h-in|
}

# The same as above but this time
# for configure scripts created with Autoconf 2.14a.
/^config_files="\\\\/,/^$/ {
  s|po/Makefile\.in|&:po/Makefile.in-in|
}
/^config_headers="\\\\/,/^$/ {
  s|config\.h|&:config.h-in|
}
/# Handling of arguments./,/^$/ {
  s|po/Makefile\.in|&:po/Makefile.in-in|2
  s|config\.h|&:config.h-in|2
}

# Replace `(command) > /dev/null` with `command > /dev/null`, since
# parenthesized commands always return zero status in the ported Bash,
# even if the named command doesn't exist
/if [^{].*null/,/ then/ {
  /test .*null/ {
    s,(,,
    s,),,
  }
}

# DOS-style absolute file names should be supported as well
/\*) srcdir=/s,/\*,[\\\\/]* | [A-z]:[\\\\/]*,
/\$]\*) INSTALL=/s,\[/\$\]\*,[\\\\/$]* | [A-z]:[\\\\/]*,
/\$]\*) ac_rel_source=/s,\[/\$\]\*,[\\\\/$]* | [A-z]:[\\\\/]*,

# Switch the order of the two Sed commands, since DOS path names
# could include a colon
/ac_file_inputs=/s,\( -e "s%\^%\$ac_given_srcdir/%"\)\( -e "s%:% $ac_given_srcdir/%g"\),\2\1,

# Prevent the spliting of conftest.subs.
# The sed script: conftest.subs is split into 48 or 90 lines long files.
# This will produce sed scripts called conftest.s1, conftest.s2, etc.
# that will not work if conftest.subs contains a multi line sed command
# at line #90. In this case the first part of the sed command will be the
# last line of conftest.s1 and the rest of the command will be the first lines
# of conftest.s2. So both script will not work properly.
# This matches the configure script produced by Autoconf 2.12
/ac_max_sed_cmds=[0-9]/ s,=.*$,=`sed -n "$=" conftest.subs`,
# This matches the configure script produced by Autoconf 2.14a
/ac_max_sed_lines=[0-9]/ s,=.*$,=`sed -n "$=" $ac_cs_root.subs `,

# The following two items are changes needed for configuring
# and compiling across partitions.
# 1) The given srcdir value is always translated from the
#    "x:" syntax into "/dev/x" syntax while we run configure.
/^[ 	]*-srcdir=\*.*$/ a\
    ac_optarg=`echo "$ac_optarg" | sed "s,^\\([A-Za-z]\\):,/dev/\\1,"`
/set X `ls -Lt \$srcdir/ i\
   if `echo $srcdir | grep "^/dev/" - > /dev/null`; then\
     srcdir=`echo "$srcdir" | sed -e "s%^/dev/%%" -e "s%/%:/%"`\
   fi

#  2) We need links across partitions, so we will use "cp -pf" instead of "ln".
/# Make a symlink if possible; otherwise try a hard link./,/EOF/ {
  s,;.*then, 2>/dev/null || cp -pf \$srcdir/\$ac_source \$ac_dest&,
}

# Let libtool use _libs all the time.
/objdir=/s,\.libs,_libs,

# Stock djdev203 does not provide an unsetenv() function,
# so we will use djdev204 CVS tree's one.
/^LTLIBOBJS=/ s,|, unsetenv.c |,

# Stock djdev203 does not provide pw_gecos,
# so we will use djdev204 CVS tree's one.
/^LTLIBOBJS=/ s,|, getpwnam.c |,
