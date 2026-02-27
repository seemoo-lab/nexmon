LOCAL_PATH:= $(call my-dir)
GLIB_SRC_PATH:= $(LOCAL_PATH)/../glib
GETTEXT_SRC_PATH:= $(LOCAL_PATH)/../gettext
ICONV_SRC_PATH:= $(LOCAL_PATH)/../libiconv

include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
	$(GLIB_SRC_PATH)/glib/deprecated/gallocator.c \
	$(GLIB_SRC_PATH)/glib/deprecated/gcache.c \
	$(GLIB_SRC_PATH)/glib/deprecated/gcompletion.c \
	$(GLIB_SRC_PATH)/glib/deprecated/grel.c \
	$(GLIB_SRC_PATH)/glib/deprecated/gthread-deprecated.c \
	$(GLIB_SRC_PATH)/glib/garray.c \
	$(GLIB_SRC_PATH)/glib/gasyncqueue.c \
	$(GLIB_SRC_PATH)/glib/gatomic.c \
	$(GLIB_SRC_PATH)/glib/gbacktrace.c \
	$(GLIB_SRC_PATH)/glib/gbase64.c \
	$(GLIB_SRC_PATH)/glib/gbitlock.c \
	$(GLIB_SRC_PATH)/glib/gbookmarkfile.c \
	$(GLIB_SRC_PATH)/glib/gbytes.c \
	$(GLIB_SRC_PATH)/glib/gcharset.c \
	$(GLIB_SRC_PATH)/glib/gchecksum.c \
	$(GLIB_SRC_PATH)/glib/gconvert.c \
	$(GLIB_SRC_PATH)/glib/gdataset.c \
	$(GLIB_SRC_PATH)/glib/gdate.c \
	$(GLIB_SRC_PATH)/glib/gdatetime.c \
	$(GLIB_SRC_PATH)/glib/gdir.c \
	$(GLIB_SRC_PATH)/glib/genviron.c \
	$(GLIB_SRC_PATH)/glib/gerror.c \
	$(GLIB_SRC_PATH)/glib/gfileutils.c \
	$(GLIB_SRC_PATH)/glib/ggettext.c \
	$(GLIB_SRC_PATH)/glib/ghash.c \
	$(GLIB_SRC_PATH)/glib/ghmac.c \
	$(GLIB_SRC_PATH)/glib/ghook.c \
	$(GLIB_SRC_PATH)/glib/ghostutils.c \
	$(GLIB_SRC_PATH)/glib/giochannel.c \
	$(GLIB_SRC_PATH)/glib/gkeyfile.c \
	$(GLIB_SRC_PATH)/glib/glib-init.c \
	$(GLIB_SRC_PATH)/glib/glib-private.c \
	$(GLIB_SRC_PATH)/glib/glist.c \
	$(GLIB_SRC_PATH)/glib/gmain.c \
	$(GLIB_SRC_PATH)/glib/gmappedfile.c \
	$(GLIB_SRC_PATH)/glib/gmarkup.c \
	$(GLIB_SRC_PATH)/glib/gmem.c \
	$(GLIB_SRC_PATH)/glib/gmessages.c \
	$(GLIB_SRC_PATH)/glib/gnode.c \
	$(GLIB_SRC_PATH)/glib/goption.c \
	$(GLIB_SRC_PATH)/glib/gpattern.c \
	$(GLIB_SRC_PATH)/glib/gpoll.c \
	$(GLIB_SRC_PATH)/glib/gprimes.c \
	$(GLIB_SRC_PATH)/glib/gqsort.c \
	$(GLIB_SRC_PATH)/glib/gquark.c \
	$(GLIB_SRC_PATH)/glib/gqueue.c \
	$(GLIB_SRC_PATH)/glib/grand.c \
	$(GLIB_SRC_PATH)/glib/gregex.c \
	$(GLIB_SRC_PATH)/glib/gscanner.c \
	$(GLIB_SRC_PATH)/glib/gsequence.c \
	$(GLIB_SRC_PATH)/glib/gshell.c \
	$(GLIB_SRC_PATH)/glib/gslice.c \
	$(GLIB_SRC_PATH)/glib/gslist.c \
	$(GLIB_SRC_PATH)/glib/gstdio.c \
	$(GLIB_SRC_PATH)/glib/gstrfuncs.c \
	$(GLIB_SRC_PATH)/glib/gstring.c \
	$(GLIB_SRC_PATH)/glib/gstringchunk.c \
	$(GLIB_SRC_PATH)/glib/gtestutils.c \
	$(GLIB_SRC_PATH)/glib/gthread.c \
	$(GLIB_SRC_PATH)/glib/gthreadpool.c \
	$(GLIB_SRC_PATH)/glib/gtimer.c \
	$(GLIB_SRC_PATH)/glib/gtimezone.c \
	$(GLIB_SRC_PATH)/glib/gtranslit.c \
	$(GLIB_SRC_PATH)/glib/gtrashstack.c \
	$(GLIB_SRC_PATH)/glib/gtree.c \
	$(GLIB_SRC_PATH)/glib/guniprop.c \
	$(GLIB_SRC_PATH)/glib/gutf8.c \
	$(GLIB_SRC_PATH)/glib/gunibreak.c \
	$(GLIB_SRC_PATH)/glib/gunicollate.c \
	$(GLIB_SRC_PATH)/glib/gunidecomp.c \
	$(GLIB_SRC_PATH)/glib/gurifuncs.c \
	$(GLIB_SRC_PATH)/glib/gutils.c \
	$(GLIB_SRC_PATH)/glib/gvariant.c \
	$(GLIB_SRC_PATH)/glib/gvariant-core.c \
	$(GLIB_SRC_PATH)/glib/gvariant-parser.c \
	$(GLIB_SRC_PATH)/glib/gvariant-serialiser.c \
	$(GLIB_SRC_PATH)/glib/gvarianttypeinfo.c \
	$(GLIB_SRC_PATH)/glib/gvarianttype.c \
	$(GLIB_SRC_PATH)/glib/gversion.c \
	$(GLIB_SRC_PATH)/glib/gwakeup.c \
	$(GLIB_SRC_PATH)/glib/gprintf.c \
	$(GLIB_SRC_PATH)/glib/glib-unix.c \
	$(GLIB_SRC_PATH)/glib/gthread-posix.c \
	$(GLIB_SRC_PATH)/glib/giounix.c \
	$(GLIB_SRC_PATH)/glib/gspawn.c \
	$(GLIB_SRC_PATH)/glib/libcharset/localcharset.c \
	$(GLIB_SRC_PATH)/glib/gnulib/asnprintf.c \
	$(GLIB_SRC_PATH)/glib/gnulib/printf-args.c \
	$(GLIB_SRC_PATH)/glib/gnulib/printf-parse.c \
	$(GLIB_SRC_PATH)/glib/gnulib/printf.c \
	$(GLIB_SRC_PATH)/glib/gnulib/vasnprintf.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_byte_order.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_chartables.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_compile.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_config.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_dfa_exec.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_exec.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_fullinfo.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_get.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_globals.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_jit_compile.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_newline.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_ord2utf8.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_string_utils.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_study.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_tables.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_valid_utf8.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_version.c \
	$(GLIB_SRC_PATH)/glib/pcre/pcre_xclass.c

LOCAL_MODULE := libglib-2.0

LOCAL_C_INCLUDES := \
	$(GLIB_SRC_PATH) \
	$(GLIB_SRC_PATH)/glib \
	$(GETTEXT_SRC_PATH)/gettext-tools/intl \
	$(ICONV_SRC_PATH)/include


LOCAL_CFLAGS := -DHAVE_CONFIG_H -I. -I.. -I.. -I../glib -I../glib -I.. \
	-DG_LOG_DOMAIN=\"GLib\" -DG_DISABLE_CAST_CHECKS -DGLIB_COMPILATION -DPCRE_STATIC \
	-DANDROID -DNO_XMALLOC -mandroid -pthread -Wall -Wstrict-prototypes -Werror=declaration-after-statement \
	-Werror=missing-prototypes -Werror=implicit-function-declaration -Werror=pointer-arith -Werror=init-self \
	-Werror=format=2 -Werror=missing-include-dirs -fvisibility=hidden -Wno-multichar -marm

include $(BUILD_STATIC_LIBRARY)
