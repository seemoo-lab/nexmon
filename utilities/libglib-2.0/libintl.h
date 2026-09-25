/* Minimal <libintl.h> for the Android cross-build.
 *
 * bionic ships no libintl and NLS is disabled in config.h, so glib's glibintl.h
 * already macro-defines the gettext family as untranslated passthroughs. A few
 * glib sources still #include <libintl.h> unconditionally, so this header only
 * has to exist. The declarations below are guarded with #ifndef so they never
 * clash with glib's macros, and nothing here is ever actually called or linked. */
#ifndef _LIBINTL_H_SHIM
#define _LIBINTL_H_SHIM 1

#ifndef gettext
extern char *gettext (const char *__msgid);
#endif
#ifndef dgettext
extern char *dgettext (const char *__domainname, const char *__msgid);
#endif
#ifndef dcgettext
extern char *dcgettext (const char *__domainname, const char *__msgid, int __category);
#endif
#ifndef ngettext
extern char *ngettext (const char *__msgid1, const char *__msgid2, unsigned long int __n);
#endif
#ifndef dngettext
extern char *dngettext (const char *__domainname, const char *__msgid1,
                        const char *__msgid2, unsigned long int __n);
#endif
#ifndef textdomain
extern char *textdomain (const char *__domainname);
#endif
#ifndef bindtextdomain
extern char *bindtextdomain (const char *__domainname, const char *__dirname);
#endif
#ifndef bind_textdomain_codeset
extern char *bind_textdomain_codeset (const char *__domainname, const char *__codeset);
#endif

#endif /* _LIBINTL_H_SHIM */
