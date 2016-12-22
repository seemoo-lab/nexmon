/* Extracts strings from C source file to Uniforum style .po file.
   Copyright (C) 1995-1998, 2000-2016 Free Software Foundation, Inc.
   Written by Ulrich Drepper <drepper@gnu.ai.mit.edu>, April 1995.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif
#include <alloca.h>

#include <ctype.h>
#include <errno.h>
#include <getopt.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>
#include <locale.h>
#include <limits.h>

#include "xgettext.h"
#include "closeout.h"
#include "dir-list.h"
#include "file-list.h"
#include "str-list.h"
#include "error.h"
#include "error-progname.h"
#include "progname.h"
#include "relocatable.h"
#include "basename.h"
#include "xerror.h"
#include "xvasprintf.h"
#include "xsize.h"
#include "xalloc.h"
#include "xmalloca.h"
#include "c-strstr.h"
#include "xerror.h"
#include "filename.h"
#include "concat-filename.h"
#include "c-strcase.h"
#include "open-catalog.h"
#include "read-catalog-abstract.h"
#include "read-po.h"
#include "message.h"
#include "po-charset.h"
#include "msgl-iconv.h"
#include "msgl-ascii.h"
#include "msgl-check.h"
#include "po-xerror.h"
#include "po-time.h"
#include "write-catalog.h"
#include "write-po.h"
#include "write-properties.h"
#include "write-stringtable.h"
#include "color.h"
#include "format.h"
#include "propername.h"
#include "sentence.h"
#include "unistr.h"
#include "its.h"
#include "locating-rule.h"
#include "search-path.h"
#include "gettext.h"

/* A convenience macro.  I don't like writing gettext() every time.  */
#define _(str) gettext (str)


#include "x-c.h"
#include "x-po.h"
#include "x-sh.h"
#include "x-python.h"
#include "x-lisp.h"
#include "x-elisp.h"
#include "x-librep.h"
#include "x-scheme.h"
#include "x-smalltalk.h"
#include "x-java.h"
#include "x-properties.h"
#include "x-csharp.h"
#include "x-appdata.h"
#include "x-awk.h"
#include "x-ycp.h"
#include "x-tcl.h"
#include "x-perl.h"
#include "x-php.h"
#include "x-stringtable.h"
#include "x-rst.h"
#include "x-glade.h"
#include "x-lua.h"
#include "x-javascript.h"
#include "x-vala.h"
#include "x-gsettings.h"
#include "x-desktop.h"


#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))
#define ENDOF(a) ((a) + SIZEOF(a))


/* If nonzero add all comments immediately preceding one of the keywords. */
static bool add_all_comments = false;

/* Tag used in comment of prevailing domain.  */
static char *comment_tag;

/* Name of default domain file.  If not set defaults to messages.po.  */
static const char *default_domain;

/* If called with --debug option the output reflects whether format
   string recognition is done automatically or forced by the user.  */
static int do_debug;

/* Content of .po files with symbols to be excluded.  */
message_list_ty *exclude;

/* Force output of PO file even if empty.  */
static int force_po;

/* Copyright holder of the output file and the translations.  */
static const char *copyright_holder = "THE PACKAGE'S COPYRIGHT HOLDER";

/* Package name.  */
static const char *package_name = NULL;

/* Package version.  */
static const char *package_version = NULL;

/* Email address or URL for reports of bugs in msgids.  */
static const char *msgid_bugs_address = NULL;

/* String used as prefix for msgstr.  */
static const char *msgstr_prefix;

/* String used as suffix for msgstr.  */
static const char *msgstr_suffix;

/* Directory in which output files are created.  */
static char *output_dir;

/* The output syntax: .pot or .properties or .strings.  */
static catalog_output_format_ty output_syntax = &output_format_po;

/* If nonzero omit header with information about this run.  */
int xgettext_omit_header;

/* Table of flag_context_list_ty tables.  */
static flag_context_list_table_ty flag_table_c;
static flag_context_list_table_ty flag_table_cxx_qt;
static flag_context_list_table_ty flag_table_cxx_kde;
static flag_context_list_table_ty flag_table_cxx_boost;
static flag_context_list_table_ty flag_table_objc;
static flag_context_list_table_ty flag_table_gcc_internal;
static flag_context_list_table_ty flag_table_sh;
static flag_context_list_table_ty flag_table_python;
static flag_context_list_table_ty flag_table_lisp;
static flag_context_list_table_ty flag_table_elisp;
static flag_context_list_table_ty flag_table_librep;
static flag_context_list_table_ty flag_table_scheme;
static flag_context_list_table_ty flag_table_java;
static flag_context_list_table_ty flag_table_csharp;
static flag_context_list_table_ty flag_table_awk;
static flag_context_list_table_ty flag_table_ycp;
static flag_context_list_table_ty flag_table_tcl;
static flag_context_list_table_ty flag_table_perl;
static flag_context_list_table_ty flag_table_php;
static flag_context_list_table_ty flag_table_lua;
static flag_context_list_table_ty flag_table_javascript;
static flag_context_list_table_ty flag_table_vala;

/* If true, recognize Qt format strings.  */
static bool recognize_format_qt;

/* If true, recognize KDE format strings.  */
static bool recognize_format_kde;

/* If true, recognize Boost format strings.  */
static bool recognize_format_boost;

/* Syntax checks enabled by default.  */
static enum is_syntax_check default_syntax_check[NSYNTAXCHECKS];

/* Canonicalized encoding name for all input files.  */
const char *xgettext_global_source_encoding;

#if HAVE_ICONV
/* Converter from xgettext_global_source_encoding to UTF-8 (except from
   ASCII or UTF-8, when this conversion is a no-op).  */
iconv_t xgettext_global_source_iconv;
#endif

/* Canonicalized encoding name for the current input file.  */
const char *xgettext_current_source_encoding;

#if HAVE_ICONV
/* Converter from xgettext_current_source_encoding to UTF-8 (except from
   ASCII or UTF-8, when this conversion is a no-op).  */
iconv_t xgettext_current_source_iconv;
#endif

static locating_rule_list_ty *its_locating_rules;

#define ITS_ROOT_UNTRANSLATABLE \
  "<its:rules xmlns:its=\"http://www.w3.org/2005/11/its\"" \
  "           version=\"2.0\">" \
  "  <its:translateRule selector=\"/*\" translate=\"no\"/>" \
  "</its:rules>"

/* If nonzero add comments used by itstool. */
static bool add_itstool_comments = false;

/* Long options.  */
static const struct option long_options[] =
{
  { "add-comments", optional_argument, NULL, 'c' },
  { "add-location", optional_argument, NULL, 'n' },
  { "boost", no_argument, NULL, CHAR_MAX + 11 },
  { "c++", no_argument, NULL, 'C' },
  { "check", required_argument, NULL, CHAR_MAX + 17 },
  { "color", optional_argument, NULL, CHAR_MAX + 14 },
  { "copyright-holder", required_argument, NULL, CHAR_MAX + 1 },
  { "debug", no_argument, &do_debug, 1 },
  { "default-domain", required_argument, NULL, 'd' },
  { "directory", required_argument, NULL, 'D' },
  { "escape", no_argument, NULL, 'E' },
  { "exclude-file", required_argument, NULL, 'x' },
  { "extract-all", no_argument, NULL, 'a' },
  { "files-from", required_argument, NULL, 'f' },
  { "flag", required_argument, NULL, CHAR_MAX + 8 },
  { "force-po", no_argument, &force_po, 1 },
  { "foreign-user", no_argument, NULL, CHAR_MAX + 2 },
  { "from-code", required_argument, NULL, CHAR_MAX + 3 },
  { "help", no_argument, NULL, 'h' },
  { "indent", no_argument, NULL, 'i' },
  { "its", required_argument, NULL, CHAR_MAX + 20 },
  { "itstool", no_argument, NULL, CHAR_MAX + 19 },
  { "join-existing", no_argument, NULL, 'j' },
  { "kde", no_argument, NULL, CHAR_MAX + 10 },
  { "keyword", optional_argument, NULL, 'k' },
  { "language", required_argument, NULL, 'L' },
  { "msgid-bugs-address", required_argument, NULL, CHAR_MAX + 5 },
  { "msgstr-prefix", optional_argument, NULL, 'm' },
  { "msgstr-suffix", optional_argument, NULL, 'M' },
  { "no-escape", no_argument, NULL, 'e' },
  { "no-location", no_argument, NULL, CHAR_MAX + 16 },
  { "no-wrap", no_argument, NULL, CHAR_MAX + 4 },
  { "omit-header", no_argument, &xgettext_omit_header, 1 },
  { "output", required_argument, NULL, 'o' },
  { "output-dir", required_argument, NULL, 'p' },
  { "package-name", required_argument, NULL, CHAR_MAX + 12 },
  { "package-version", required_argument, NULL, CHAR_MAX + 13 },
  { "properties-output", no_argument, NULL, CHAR_MAX + 6 },
  { "qt", no_argument, NULL, CHAR_MAX + 9 },
  { "sentence-end", required_argument, NULL, CHAR_MAX + 18 },
  { "sort-by-file", no_argument, NULL, 'F' },
  { "sort-output", no_argument, NULL, 's' },
  { "strict", no_argument, NULL, 'S' },
  { "string-limit", required_argument, NULL, 'l' },
  { "stringtable-output", no_argument, NULL, CHAR_MAX + 7 },
  { "style", required_argument, NULL, CHAR_MAX + 15 },
  { "trigraphs", no_argument, NULL, 'T' },
  { "version", no_argument, NULL, 'V' },
  { "width", required_argument, NULL, 'w', },
  { NULL, 0, NULL, 0 }
};


/* The extractors must all be functions returning void and taking three
   arguments designating the input stream and one message domain list argument
   in which to add the messages.  */
typedef void (*extractor_func) (FILE *fp, const char *real_filename,
                                const char *logical_filename,
                                flag_context_list_table_ty *flag_table,
                                msgdomain_list_ty *mdlp);

typedef struct extractor_ty extractor_ty;
struct extractor_ty
{
  extractor_func func;
  flag_context_list_table_ty *flag_table;
  struct formatstring_parser *formatstring_parser1;
  struct formatstring_parser *formatstring_parser2;
  struct formatstring_parser *formatstring_parser3;
  struct literalstring_parser *literalstring_parser;
};


/* Forward declaration of local functions.  */
static void usage (int status)
#if defined __GNUC__ && ((__GNUC__ == 2 && __GNUC_MINOR__ > 4) || __GNUC__ > 2)
        __attribute__ ((noreturn))
#endif
;
static void read_exclusion_file (char *file_name);
static void extract_from_file (const char *file_name, extractor_ty extractor,
                               msgdomain_list_ty *mdlp);
static void extract_from_xml_file (const char *file_name,
                                   its_rule_list_ty *rules,
                                   msgdomain_list_ty *mdlp);
static message_ty *construct_header (void);
static void finalize_header (msgdomain_list_ty *mdlp);
static extractor_ty language_to_extractor (const char *name);
static const char *extension_to_language (const char *extension);


int
main (int argc, char *argv[])
{
  int optchar;
  bool do_help = false;
  bool do_version = false;
  msgdomain_list_ty *mdlp;
  bool join_existing = false;
  bool no_default_keywords = false;
  bool some_additional_keywords = false;
  bool sort_by_msgid = false;
  bool sort_by_filepos = false;
  char **dirs;
  char **its_dirs;
  char *explicit_its_filename = NULL;
  const char *file_name;
  const char *files_from = NULL;
  string_list_ty *file_list;
  char *output_file = NULL;
  const char *language = NULL;
  extractor_ty extractor = { NULL, NULL, NULL, NULL };
  int cnt;
  size_t i;

  /* Set program name for messages.  */
  set_program_name (argv[0]);
  error_print_progname = maybe_print_progname;

#ifdef HAVE_SETLOCALE
  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");
#endif

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  bindtextdomain ("bison-runtime", relocate (BISON_LOCALEDIR));
  textdomain (PACKAGE);

  /* Ensure that write errors on stdout are detected.  */
  atexit (close_stdout);

  /* Set initial value of variables.  */
  default_domain = MESSAGE_DOMAIN_DEFAULT;
  xgettext_global_source_encoding = po_charset_ascii;
  init_flag_table_c ();
  init_flag_table_objc ();
  init_flag_table_gcc_internal ();
  init_flag_table_kde ();
  init_flag_table_sh ();
  init_flag_table_python ();
  init_flag_table_lisp ();
  init_flag_table_elisp ();
  init_flag_table_librep ();
  init_flag_table_scheme ();
  init_flag_table_java ();
  init_flag_table_csharp ();
  init_flag_table_awk ();
  init_flag_table_ycp ();
  init_flag_table_tcl ();
  init_flag_table_perl ();
  init_flag_table_php ();
  init_flag_table_lua ();
  init_flag_table_javascript ();
  init_flag_table_vala ();

  while ((optchar = getopt_long (argc, argv,
                                 "ac::Cd:D:eEf:Fhijk::l:L:m::M::no:p:sTVw:W:x:",
                                 long_options, NULL)) != EOF)
    switch (optchar)
      {
      case '\0':                /* Long option.  */
        break;

      case 'a':
        x_c_extract_all ();
        x_sh_extract_all ();
        x_python_extract_all ();
        x_lisp_extract_all ();
        x_elisp_extract_all ();
        x_librep_extract_all ();
        x_scheme_extract_all ();
        x_java_extract_all ();
        x_csharp_extract_all ();
        x_awk_extract_all ();
        x_tcl_extract_all ();
        x_perl_extract_all ();
        x_php_extract_all ();
        x_lua_extract_all ();
        x_javascript_extract_all ();
        x_vala_extract_all ();
        break;

      case 'c':
        if (optarg == NULL)
          {
            add_all_comments = true;
            comment_tag = NULL;
          }
        else
          {
            add_all_comments = false;
            comment_tag = optarg;
            /* We ignore leading white space.  */
            while (isspace ((unsigned char) *comment_tag))
              ++comment_tag;
          }
        break;

      case 'C':
        language = "C++";
        break;

      case 'd':
        default_domain = optarg;
        break;

      case 'D':
        dir_list_append (optarg);
        break;

      case 'e':
        message_print_style_escape (false);
        break;

      case 'E':
        message_print_style_escape (true);
        break;

      case 'f':
        files_from = optarg;
        break;

      case 'F':
        sort_by_filepos = true;
        break;

      case 'h':
        do_help = true;
        break;

      case 'i':
        message_print_style_indent ();
        break;

      case 'j':
        join_existing = true;
        break;

      case 'k':
        if (optarg != NULL && *optarg == '\0')
          /* Make "--keyword=" work like "--keyword" and "-k".  */
          optarg = NULL;
        x_c_keyword (optarg);
        x_objc_keyword (optarg);
        x_sh_keyword (optarg);
        x_python_keyword (optarg);
        x_lisp_keyword (optarg);
        x_elisp_keyword (optarg);
        x_librep_keyword (optarg);
        x_scheme_keyword (optarg);
        x_java_keyword (optarg);
        x_csharp_keyword (optarg);
        x_awk_keyword (optarg);
        x_tcl_keyword (optarg);
        x_perl_keyword (optarg);
        x_php_keyword (optarg);
        x_lua_keyword (optarg);
        x_javascript_keyword (optarg);
        x_vala_keyword (optarg);
        x_desktop_keyword (optarg);
        if (optarg == NULL)
          no_default_keywords = true;
        else
          some_additional_keywords = true;
        break;

      case 'l':
        /* Accepted for backward compatibility with 0.10.35.  */
        break;

      case 'L':
        language = optarg;
        break;

      case 'm':
        /* -m takes an optional argument.  If none is given "" is assumed. */
        msgstr_prefix = optarg == NULL ? "" : optarg;
        break;

      case 'M':
        /* -M takes an optional argument.  If none is given "" is assumed. */
        msgstr_suffix = optarg == NULL ? "" : optarg;
        break;

      case 'n':
        if (handle_filepos_comment_option (optarg))
          usage (EXIT_FAILURE);
        break;

      case 'o':
        output_file = optarg;
        break;

      case 'p':
        {
          size_t len = strlen (optarg);

          if (output_dir != NULL)
            free (output_dir);

          if (optarg[len - 1] == '/')
            output_dir = xstrdup (optarg);
          else
            output_dir = xasprintf ("%s/", optarg);
        }
        break;

      case 's':
        sort_by_msgid = true;
        break;

      case 'S':
        message_print_style_uniforum ();
        break;

      case 'T':
        x_c_trigraphs ();
        break;

      case 'V':
        do_version = true;
        break;

      case 'w':
        {
          int value;
          char *endp;
          value = strtol (optarg, &endp, 10);
          if (endp != optarg)
            message_page_width_set (value);
        }
        break;

      case 'x':
        read_exclusion_file (optarg);
        break;

      case CHAR_MAX + 1:        /* --copyright-holder */
        copyright_holder = optarg;
        break;

      case CHAR_MAX + 2:        /* --foreign-user */
        copyright_holder = "";
        break;

      case CHAR_MAX + 3:        /* --from-code */
        xgettext_global_source_encoding = po_charset_canonicalize (optarg);
        if (xgettext_global_source_encoding == NULL)
          {
            multiline_warning (xasprintf (_("warning: ")),
                               xasprintf (_("\
'%s' is not a valid encoding name.  Using ASCII as fallback.\n"),
                                          optarg));
            xgettext_global_source_encoding = po_charset_ascii;
          }
        break;

      case CHAR_MAX + 4:        /* --no-wrap */
        message_page_width_ignore ();
        break;

      case CHAR_MAX + 5:        /* --msgid-bugs-address */
        msgid_bugs_address = optarg;
        break;

      case CHAR_MAX + 6:        /* --properties-output */
        output_syntax = &output_format_properties;
        break;

      case CHAR_MAX + 7:        /* --stringtable-output */
        output_syntax = &output_format_stringtable;
        break;

      case CHAR_MAX + 8:        /* --flag */
        xgettext_record_flag (optarg);
        break;

      case CHAR_MAX + 9:        /* --qt */
        recognize_format_qt = true;
        break;

      case CHAR_MAX + 10:       /* --kde */
        recognize_format_kde = true;
        activate_additional_keywords_kde ();
        break;

      case CHAR_MAX + 11:       /* --boost */
        recognize_format_boost = true;
        break;

      case CHAR_MAX + 12:       /* --package-name */
        package_name = optarg;
        break;

      case CHAR_MAX + 13:       /* --package-version */
        package_version = optarg;
        break;

      case CHAR_MAX + 14: /* --color */
        if (handle_color_option (optarg) || color_test_mode)
          usage (EXIT_FAILURE);
        break;

      case CHAR_MAX + 15: /* --style */
        handle_style_option (optarg);
        break;

      case CHAR_MAX + 16: /* --no-location */
        message_print_style_filepos (filepos_comment_none);
        break;

      case CHAR_MAX + 17: /* --check */
        for (i = 0; i < NSYNTAXCHECKS; i++)
          {
            if (strcmp (optarg, syntax_check_name[i]) == 0)
              {
                default_syntax_check[i] = yes;
                break;
              }
          }
        if (i == NSYNTAXCHECKS)
          error (EXIT_FAILURE, 0, _("syntax check '%s' unknown"), optarg);
        break;

      case CHAR_MAX + 18: /* --sentence-end */
        if (strcmp (optarg, "single-space") == 0)
          sentence_end_required_spaces = 1;
        else if (strcmp (optarg, "double-space") == 0)
          sentence_end_required_spaces = 2;
        else
          error (EXIT_FAILURE, 0, _("sentence end type '%s' unknown"), optarg);
        break;

      case CHAR_MAX + 20: /* --its */
        explicit_its_filename = optarg;
        break;

      case CHAR_MAX + 19: /* --itstool */
        add_itstool_comments = true;
        break;

      default:
        usage (EXIT_FAILURE);
        /* NOTREACHED */
      }

  /* Version information requested.  */
  if (do_version)
    {
      printf ("%s (GNU %s) %s\n", basename (program_name), PACKAGE, VERSION);
      /* xgettext: no-wrap */
      printf (_("Copyright (C) %s Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
"),
              "1995-1998, 2000-2016");
      printf (_("Written by %s.\n"), proper_name ("Ulrich Drepper"));
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* Verify selected options.  */
  if (sort_by_msgid && sort_by_filepos)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
           "--sort-output", "--sort-by-file");

  /* We cannot support both Qt and KDE, or Qt and Boost, or KDE and Boost
     format strings, because there are only two formatstring parsers per
     language, and formatstring_c is the first one for C++.  */
  if (recognize_format_qt && recognize_format_kde)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
           "--qt", "--kde");
  if (recognize_format_qt && recognize_format_boost)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
           "--qt", "--boost");
  if (recognize_format_kde && recognize_format_boost)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
           "--kde", "--boost");

  if (join_existing && strcmp (default_domain, "-") == 0)
    error (EXIT_FAILURE, 0, _("\
--join-existing cannot be used when output is written to stdout"));

  if (no_default_keywords && !some_additional_keywords)
    {
      error (0, 0, _("\
xgettext cannot work without keywords to look for"));
      usage (EXIT_FAILURE);
    }

  /* Test whether we have some input files given.  */
  if (files_from == NULL && optind >= argc)
    {
      error (EXIT_SUCCESS, 0, _("no input file given"));
      usage (EXIT_FAILURE);
    }

  /* Explicit ITS file selection and language specification are
     mutually exclusive.  */
  if (explicit_its_filename != NULL && language != NULL)
    error (EXIT_FAILURE, 0, _("%s and %s are mutually exclusive"),
           "--its", "--language");

  if (explicit_its_filename == NULL)
    {
      its_dirs = get_search_path ("its");
      its_locating_rules = locating_rule_list_alloc ();
      for (dirs = its_dirs; *dirs != NULL; dirs++)
        locating_rule_list_add_from_directory (its_locating_rules, *dirs);
    }

  /* Determine extractor from language.  */
  if (language != NULL)
    extractor = language_to_extractor (language);

  /* Canonize msgstr prefix/suffix.  */
  if (msgstr_prefix != NULL && msgstr_suffix == NULL)
    msgstr_suffix = "";
  else if (msgstr_prefix == NULL && msgstr_suffix != NULL)
    msgstr_prefix = "";

  /* Default output directory is the current directory.  */
  if (output_dir == NULL)
    output_dir = ".";

  /* Construct the name of the output file.  If the default domain has
     the special name "-" we write to stdout.  */
  if (output_file)
    {
      if (IS_ABSOLUTE_PATH (output_file) || strcmp (output_file, "-") == 0)
        file_name = xstrdup (output_file);
      else
        /* Please do NOT add a .po suffix! */
        file_name = xconcatenated_filename (output_dir, output_file, NULL);
    }
  else if (strcmp (default_domain, "-") == 0)
    file_name = "-";
  else
    file_name = xconcatenated_filename (output_dir, default_domain, ".po");

  /* Determine list of files we have to process.  */
  if (files_from != NULL)
    file_list = read_names_from_file (files_from);
  else
    file_list = string_list_alloc ();
  /* Append names from command line.  */
  for (cnt = optind; cnt < argc; ++cnt)
    string_list_append_unique (file_list, argv[cnt]);

  /* Allocate converter from xgettext_global_source_encoding to UTF-8 (except
     from ASCII or UTF-8, when this conversion is a no-op).  */
  if (xgettext_global_source_encoding != po_charset_ascii
      && xgettext_global_source_encoding != po_charset_utf8)
    {
#if HAVE_ICONV
      iconv_t cd;

      /* Avoid glibc-2.1 bug with EUC-KR.  */
# if ((__GLIBC__ == 2 && __GLIBC_MINOR__ <= 1) && !defined __UCLIBC__) \
     && !defined _LIBICONV_VERSION
      if (strcmp (xgettext_global_source_encoding, "EUC-KR") == 0)
        cd = (iconv_t)(-1);
      else
# endif
      cd = iconv_open (po_charset_utf8, xgettext_global_source_encoding);
      if (cd == (iconv_t)(-1))
        error (EXIT_FAILURE, 0, _("\
Cannot convert from \"%s\" to \"%s\". %s relies on iconv(), \
and iconv() does not support this conversion."),
               xgettext_global_source_encoding, po_charset_utf8,
               basename (program_name));
      xgettext_global_source_iconv = cd;
#else
      error (EXIT_FAILURE, 0, _("\
Cannot convert from \"%s\" to \"%s\". %s relies on iconv(). \
This version was built without iconv()."),
             xgettext_global_source_encoding, po_charset_utf8,
             basename (program_name));
#endif
    }

  /* Allocate a message list to remember all the messages.  */
  mdlp = msgdomain_list_alloc (true);

  /* Generate a header, so that we know how and when this PO file was
     created.  */
  if (!xgettext_omit_header)
    message_list_append (mdlp->item[0]->messages, construct_header ());

  /* Read in the old messages, so that we can add to them.  */
  if (join_existing)
    {
      /* Temporarily reset the directory list to empty, because file_name
         is an output file and therefore should not be searched for.  */
      void *saved_directory_list = dir_list_save_reset ();
      extractor_ty po_extractor = { extract_po, NULL, NULL, NULL };

      extract_from_file (file_name, po_extractor, mdlp);
      if (!is_ascii_msgdomain_list (mdlp))
        mdlp = iconv_msgdomain_list (mdlp, "UTF-8", true, file_name);

      dir_list_restore (saved_directory_list);
    }

  /* Process all input files.  */
  for (i = 0; i < file_list->nitems; i++)
    {
      const char *filename;
      extractor_ty this_file_extractor;
      its_rule_list_ty *its_rules = NULL;

      filename = file_list->item[i];

      if (extractor.func)
        this_file_extractor = extractor;
      else if (explicit_its_filename != NULL)
        {
          its_rules = its_rule_list_alloc ();
          if (!its_rule_list_add_from_file (its_rules,
                                            explicit_its_filename))
            {
              error (EXIT_FAILURE, 0, _("\
warning: ITS rule file '%s' does not exist"), explicit_its_filename);
            }
        }
      else
        {
          const char *language_from_extension = NULL;
          const char *base;
          char *reduced;

          base = strrchr (filename, '/');
          if (!base)
            base = filename;

          reduced = xstrdup (base);
          /* Remove a trailing ".in" - it's a generic suffix.  */
          while (strlen (reduced) >= 3
                 && memcmp (reduced + strlen (reduced) - 3, ".in", 3) == 0)
            reduced[strlen (reduced) - 3] = '\0';

          /* If no language is specified with -L, deduce it the extension.  */
          if (language == NULL)
            {
              const char *p;

              /* Work out what the file extension is.  */
              p = reduced + strlen (reduced);
              for (; p > reduced && language_from_extension == NULL; p--)
                {
                  if (*p == '.')
                    {
                      const char *extension = p + 1;

                      /* Derive the language from the extension, and
                         the extractor function from the language.  */
                      language_from_extension =
                        extension_to_language (extension);
                    }
                }
            }

          /* If language is not determined from the file name
             extension, check ITS locating rules.  */
          if (language_from_extension == NULL
              && strcmp (filename, "-") != 0)
            {
              const char *its_basename;

              its_basename = locating_rule_list_locate (its_locating_rules,
                                                        filename,
                                                        language);

              if (its_basename != NULL)
                {
                  size_t j;

                  its_rules = its_rule_list_alloc ();

                  /* If the ITS file is identified by the name,
                     set the root element untranslatable.  */
                  if (language != NULL)
                    its_rule_list_add_from_string (its_rules,
                                                   ITS_ROOT_UNTRANSLATABLE);

                  for (j = 0; its_dirs[j] != NULL; j++)
                    {
                      char *its_filename =
                        xconcatenated_filename (its_dirs[j], its_basename,
                                                NULL);
                      struct stat statbuf;
                      bool ok = false;

                      if (stat (its_filename, &statbuf) == 0)
                        ok = its_rule_list_add_from_file (its_rules,
                                                          its_filename);
                      free (its_filename);
                      if (ok)
                        break;
                    }
                  if (its_dirs[j] == NULL)
                    {
                      error (0, 0, _("\
warning: ITS rule file '%s' does not exist; check your gettext installation"),
                             its_basename);
                      its_rule_list_free (its_rules);
                      its_rules = NULL;
                    }
                }
            }

          if (its_rules == NULL)
            {
              if (language_from_extension == NULL)
                {
                  const char *extension = strrchr (reduced, '.');
                  if (extension == NULL)
                    extension = "";
                  else
                    extension++;
                  error (0, 0, _("\
warning: file '%s' extension '%s' is unknown; will try C"), filename, extension);
                  language_from_extension = "C";
                }

              this_file_extractor =
                language_to_extractor (language_from_extension);
            }

          free (reduced);
        }

      if (its_rules != NULL)
        {
          /* Extract the strings from the file, using ITS.  */
          extract_from_xml_file (filename, its_rules, mdlp);
          its_rule_list_free (its_rules);
        }
      else
        /* Extract the strings from the file.  */
        extract_from_file (filename, this_file_extractor, mdlp);
    }
  string_list_free (file_list);

  /* Finalize the constructed header.  */
  if (!xgettext_omit_header)
    finalize_header (mdlp);

  /* Free the allocated converter.  */
#if HAVE_ICONV
  if (xgettext_global_source_encoding != po_charset_ascii
      && xgettext_global_source_encoding != po_charset_utf8)
    iconv_close (xgettext_global_source_iconv);
#endif

  /* Sorting the list of messages.  */
  if (sort_by_filepos)
    msgdomain_list_sort_by_filepos (mdlp);
  else if (sort_by_msgid)
    msgdomain_list_sort_by_msgid (mdlp);

  /* Check syntax of messages.  */
  {
    int nerrors = 0;

    for (i = 0; i < mdlp->nitems; i++)
      {
        message_list_ty *mlp = mdlp->item[i]->messages;
        nerrors = syntax_check_message_list (mlp);
      }

    /* Exit with status 1 on any error.  */
    if (nerrors > 0)
      error (EXIT_FAILURE, 0,
             ngettext ("found %d fatal error", "found %d fatal errors",
                       nerrors),
             nerrors);
  }

  /* Write the PO file.  */
  msgdomain_list_print (mdlp, file_name, output_syntax, force_po, do_debug);

  if (its_locating_rules)
    locating_rule_list_free (its_locating_rules);

  for (i = 0; its_dirs[i] != NULL; i++)
    free (its_dirs[i]);
  free (its_dirs);

  exit (EXIT_SUCCESS);
}


/* Display usage information and exit.  */
static void
usage (int status)
{
  if (status != EXIT_SUCCESS)
    fprintf (stderr, _("Try '%s --help' for more information.\n"),
             program_name);
  else
    {
      printf (_("\
Usage: %s [OPTION] [INPUTFILE]...\n\
"), program_name);
      printf ("\n");
      printf (_("\
Extract translatable strings from given input files.\n\
"));
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n\
Similarly for optional arguments.\n\
"));
      printf ("\n");
      printf (_("\
Input file location:\n"));
      printf (_("\
  INPUTFILE ...               input files\n"));
      printf (_("\
  -f, --files-from=FILE       get list of input files from FILE\n"));
      printf (_("\
  -D, --directory=DIRECTORY   add DIRECTORY to list for input files search\n"));
      printf (_("\
If input file is -, standard input is read.\n"));
      printf ("\n");
      printf (_("\
Output file location:\n"));
      printf (_("\
  -d, --default-domain=NAME   use NAME.po for output (instead of messages.po)\n"));
      printf (_("\
  -o, --output=FILE           write output to specified file\n"));
      printf (_("\
  -p, --output-dir=DIR        output files will be placed in directory DIR\n"));
      printf (_("\
If output file is -, output is written to standard output.\n"));
      printf ("\n");
      printf (_("\
Choice of input file language:\n"));
      printf (_("\
  -L, --language=NAME         recognise the specified language\n\
                                (C, C++, ObjectiveC, PO, Shell, Python, Lisp,\n\
                                EmacsLisp, librep, Scheme, Smalltalk, Java,\n\
                                JavaProperties, C#, awk, YCP, Tcl, Perl, PHP,\n\
                                GCC-source, NXStringTable, RST, Glade, Lua,\n\
                                JavaScript, Vala, Desktop)\n"));
      printf (_("\
  -C, --c++                   shorthand for --language=C++\n"));
      printf (_("\
By default the language is guessed depending on the input file name extension.\n"));
      printf ("\n");
      printf (_("\
Input file interpretation:\n"));
      printf (_("\
      --from-code=NAME        encoding of input files\n\
                                (except for Python, Tcl, Glade)\n"));
      printf (_("\
By default the input files are assumed to be in ASCII.\n"));
      printf ("\n");
      printf (_("\
Operation mode:\n"));
      printf (_("\
  -j, --join-existing         join messages with existing file\n"));
      printf (_("\
  -x, --exclude-file=FILE.po  entries from FILE.po are not extracted\n"));
      printf (_("\
  -cTAG, --add-comments=TAG   place comment blocks starting with TAG and\n\
                                preceding keyword lines in output file\n\
  -c, --add-comments          place all comment blocks preceding keyword lines\n\
                                in output file\n"));
      printf (_("\
      --check=NAME            perform syntax check on messages\n\
                                (ellipsis-unicode, space-ellipsis,\n\
                                 quote-unicode, bullet-unicode)\n"));
      printf (_("\
      --sentence-end=TYPE     type describing the end of sentence\n\
                                (single-space, which is the default, \n\
                                 or double-space)\n"));
      printf ("\n");
      printf (_("\
Language specific options:\n"));
      printf (_("\
  -a, --extract-all           extract all strings\n"));
      printf (_("\
                                (only languages C, C++, ObjectiveC, Shell,\n\
                                Python, Lisp, EmacsLisp, librep, Scheme, Java,\n\
                                C#, awk, Tcl, Perl, PHP, GCC-source, Glade,\n\
                                Lua, JavaScript, Vala)\n"));
      printf (_("\
  -kWORD, --keyword=WORD      look for WORD as an additional keyword\n\
  -k, --keyword               do not to use default keywords\n"));
      printf (_("\
                                (only languages C, C++, ObjectiveC, Shell,\n\
                                Python, Lisp, EmacsLisp, librep, Scheme, Java,\n\
                                C#, awk, Tcl, Perl, PHP, GCC-source, Glade,\n\
                                Lua, JavaScript, Vala, Desktop)\n"));
      printf (_("\
      --flag=WORD:ARG:FLAG    additional flag for strings inside the argument\n\
                              number ARG of keyword WORD\n"));
      printf (_("\
                                (only languages C, C++, ObjectiveC, Shell,\n\
                                Python, Lisp, EmacsLisp, librep, Scheme, Java,\n\
                                C#, awk, YCP, Tcl, Perl, PHP, GCC-source,\n\
                                Lua, JavaScript, Vala)\n"));
      printf (_("\
  -T, --trigraphs             understand ANSI C trigraphs for input\n"));
      printf (_("\
                                (only languages C, C++, ObjectiveC)\n"));
      printf (_("\
      --its=FILE              apply ITS rules from FILE\n"));
      printf (_("\
                                (only XML based languages)\n"));
      printf (_("\
      --qt                    recognize Qt format strings\n"));
      printf (_("\
                                (only language C++)\n"));
      printf (_("\
      --kde                   recognize KDE 4 format strings\n"));
      printf (_("\
                                (only language C++)\n"));
      printf (_("\
      --boost                 recognize Boost format strings\n"));
      printf (_("\
                                (only language C++)\n"));
      printf (_("\
      --debug                 more detailed formatstring recognition result\n"));
      printf ("\n");
      printf (_("\
Output details:\n"));
      printf (_("\
      --color                 use colors and other text attributes always\n\
      --color=WHEN            use colors and other text attributes if WHEN.\n\
                              WHEN may be 'always', 'never', 'auto', or 'html'.\n"));
      printf (_("\
      --style=STYLEFILE       specify CSS style rule file for --color\n"));
      printf (_("\
  -e, --no-escape             do not use C escapes in output (default)\n"));
      printf (_("\
  -E, --escape                use C escapes in output, no extended chars\n"));
      printf (_("\
      --force-po              write PO file even if empty\n"));
      printf (_("\
  -i, --indent                write the .po file using indented style\n"));
      printf (_("\
      --no-location           do not write '#: filename:line' lines\n"));
      printf (_("\
  -n, --add-location          generate '#: filename:line' lines (default)\n"));
      printf (_("\
      --strict                write out strict Uniforum conforming .po file\n"));
      printf (_("\
      --properties-output     write out a Java .properties file\n"));
      printf (_("\
      --stringtable-output    write out a NeXTstep/GNUstep .strings file\n"));
      printf (_("\
      --itstool               write out itstool comments\n"));
      printf (_("\
  -w, --width=NUMBER          set output page width\n"));
      printf (_("\
      --no-wrap               do not break long message lines, longer than\n\
                              the output page width, into several lines\n"));
      printf (_("\
  -s, --sort-output           generate sorted output\n"));
      printf (_("\
  -F, --sort-by-file          sort output by file location\n"));
      printf (_("\
      --omit-header           don't write header with 'msgid \"\"' entry\n"));
      printf (_("\
      --copyright-holder=STRING  set copyright holder in output\n"));
      printf (_("\
      --foreign-user          omit FSF copyright in output for foreign user\n"));
      printf (_("\
      --package-name=PACKAGE  set package name in output\n"));
      printf (_("\
      --package-version=VERSION  set package version in output\n"));
      printf (_("\
      --msgid-bugs-address=EMAIL@ADDRESS  set report address for msgid bugs\n"));
      printf (_("\
  -m[STRING], --msgstr-prefix[=STRING]  use STRING or \"\" as prefix for msgstr\n\
                                values\n"));
      printf (_("\
  -M[STRING], --msgstr-suffix[=STRING]  use STRING or \"\" as suffix for msgstr\n\
                                values\n"));
      printf ("\n");
      printf (_("\
Informative output:\n"));
      printf (_("\
  -h, --help                  display this help and exit\n"));
      printf (_("\
  -V, --version               output version information and exit\n"));
      printf ("\n");
      /* TRANSLATORS: The placeholder indicates the bug-reporting address
         for this package.  Please add _another line_ saying
         "Report translation bugs to <...>\n" with the address for translation
         bugs (typically your translation team's web or email address).  */
      fputs (_("Report bugs to <bug-gnu-gettext@gnu.org>.\n"),
             stdout);
    }

  exit (status);
}


static void
exclude_directive_domain (abstract_catalog_reader_ty *pop, char *name)
{
  po_gram_error_at_line (&gram_pos,
                         _("this file may not contain domain directives"));
}


static void
exclude_directive_message (abstract_catalog_reader_ty *pop,
                           char *msgctxt,
                           char *msgid,
                           lex_pos_ty *msgid_pos,
                           char *msgid_plural,
                           char *msgstr, size_t msgstr_len,
                           lex_pos_ty *msgstr_pos,
                           char *prev_msgctxt,
                           char *prev_msgid,
                           char *prev_msgid_plural,
                           bool force_fuzzy, bool obsolete)
{
  message_ty *mp;

  /* See if this message ID has been seen before.  */
  if (exclude == NULL)
    exclude = message_list_alloc (true);
  mp = message_list_search (exclude, msgctxt, msgid);
  if (mp != NULL)
    free (msgid);
  else
    {
      mp = message_alloc (msgctxt, msgid, msgid_plural, "", 1, msgstr_pos);
      /* Do not free msgid.  */
      message_list_append (exclude, mp);
    }

  /* All we care about is the msgid.  Throw the msgstr away.
     Don't even check for duplicate msgids.  */
  free (msgstr);
}


/* So that the one parser can be used for multiple programs, and also
   use good data hiding and encapsulation practices, an object
   oriented approach has been taken.  An object instance is allocated,
   and all actions resulting from the parse will be through
   invocations of method functions of that object.  */

static abstract_catalog_reader_class_ty exclude_methods =
{
  sizeof (abstract_catalog_reader_ty),
  NULL, /* constructor */
  NULL, /* destructor */
  NULL, /* parse_brief */
  NULL, /* parse_debrief */
  exclude_directive_domain,
  exclude_directive_message,
  NULL, /* comment */
  NULL, /* comment_dot */
  NULL, /* comment_filepos */
  NULL, /* comment_special */
};


static void
read_exclusion_file (char *filename)
{
  char *real_filename;
  FILE *fp = open_catalog_file (filename, &real_filename, true);
  abstract_catalog_reader_ty *pop;

  pop = catalog_reader_alloc (&exclude_methods);
  catalog_reader_parse (pop, fp, real_filename, filename, &input_format_po);
  catalog_reader_free (pop);

  if (fp != stdin)
    fclose (fp);
}


void
split_keywordspec (const char *spec,
                   const char **endp, struct callshape *shapep)
{
  const char *p;
  int argnum1 = 0;
  int argnum2 = 0;
  int argnumc = 0;
  bool argnum1_glib_context = false;
  bool argnum2_glib_context = false;
  int argtotal = 0;
  string_list_ty xcomments;

  string_list_init (&xcomments);

  /* Start parsing from the end.  */
  p = spec + strlen (spec);
  while (p > spec)
    {
      if (isdigit ((unsigned char) p[-1])
          || ((p[-1] == 'c' || p[-1] == 'g' || p[-1] == 't')
              && p - 1 > spec && isdigit ((unsigned char) p[-2])))
        {
          bool contextp = (p[-1] == 'c');
          bool glibp = (p[-1] == 'g');
          bool totalp = (p[-1] == 't');

          do
            p--;
          while (p > spec && isdigit ((unsigned char) p[-1]));

          if (p > spec && (p[-1] == ',' || p[-1] == ':'))
            {
              char *dummy;
              int arg = strtol (p, &dummy, 10);

              if (contextp)
                {
                  if (argnumc != 0)
                    /* Only one context argument can be given.  */
                    break;
                  argnumc = arg;
                }
              else if (totalp)
                {
                  if (argtotal != 0)
                    /* Only one total number of arguments can be given.  */
                    break;
                  argtotal = arg;
                }
              else
                {
                  if (argnum2 != 0)
                    /* At most two normal arguments can be given.  */
                    break;
                  argnum2 = argnum1;
                  argnum2_glib_context = argnum1_glib_context;
                  argnum1 = arg;
                  argnum1_glib_context = glibp;
                }
            }
          else
            break;
        }
      else if (p[-1] == '"')
        {
          const char *xcomment_end;

          p--;
          xcomment_end = p;

          while (p > spec && p[-1] != '"')
            p--;

          if (p > spec /* && p[-1] == '"' */)
            {
              const char *xcomment_start;

              xcomment_start = p;
              p--;
              if (p > spec && (p[-1] == ',' || p[-1] == ':'))
                {
                  size_t xcomment_len = xcomment_end - xcomment_start;
                  char *xcomment = XNMALLOC (xcomment_len + 1, char);

                  memcpy (xcomment, xcomment_start, xcomment_len);
                  xcomment[xcomment_len] = '\0';
                  string_list_append (&xcomments, xcomment);
                }
              else
                break;
            }
          else
            break;
        }
      else
        break;

      /* Here an element of the comma-separated list has been parsed.  */
      if (!(p > spec && (p[-1] == ',' || p[-1] == ':')))
        abort ();
      p--;
      if (*p == ':')
        {
          size_t i;

          if (argnum1 == 0 && argnum2 == 0)
            /* At least one non-context argument must be given.  */
            break;
          if (argnumc != 0
              && (argnum1_glib_context || argnum2_glib_context))
            /* Incompatible ways to specify the context.  */
            break;
          *endp = p;
          shapep->argnum1 = (argnum1 > 0 ? argnum1 : 1);
          shapep->argnum2 = argnum2;
          shapep->argnumc = argnumc;
          shapep->argnum1_glib_context = argnum1_glib_context;
          shapep->argnum2_glib_context = argnum2_glib_context;
          shapep->argtotal = argtotal;
          /* Reverse the order of the xcomments.  */
          string_list_init (&shapep->xcomments);
          for (i = xcomments.nitems; i > 0; )
            string_list_append (&shapep->xcomments, xcomments.item[--i]);
          string_list_destroy (&xcomments);
          return;
        }
    }

  /* Couldn't parse the desired syntax.  */
  *endp = spec + strlen (spec);
  shapep->argnum1 = 1;
  shapep->argnum2 = 0;
  shapep->argnumc = 0;
  shapep->argnum1_glib_context = false;
  shapep->argnum2_glib_context = false;
  shapep->argtotal = 0;
  string_list_init (&shapep->xcomments);
  string_list_destroy (&xcomments);
}


void
insert_keyword_callshape (hash_table *table,
                          const char *keyword, size_t keyword_len,
                          const struct callshape *shape)
{
  void *old_value;

  if (hash_find_entry (table, keyword, keyword_len, &old_value))
    {
      /* Create a one-element 'struct callshapes'.  */
      struct callshapes *shapes = XMALLOC (struct callshapes);
      shapes->nshapes = 1;
      shapes->shapes[0] = *shape;
      keyword =
        (const char *) hash_insert_entry (table, keyword, keyword_len, shapes);
      if (keyword == NULL)
        abort ();
      shapes->keyword = keyword;
      shapes->keyword_len = keyword_len;
    }
  else
    {
      /* Found a 'struct callshapes'.  See whether it already contains the
         desired shape.  */
      struct callshapes *old_shapes = (struct callshapes *) old_value;
      bool found;
      size_t i;

      found = false;
      for (i = 0; i < old_shapes->nshapes; i++)
        if (old_shapes->shapes[i].argnum1 == shape->argnum1
            && old_shapes->shapes[i].argnum2 == shape->argnum2
            && old_shapes->shapes[i].argnumc == shape->argnumc
            && old_shapes->shapes[i].argnum1_glib_context
               == shape->argnum1_glib_context
            && old_shapes->shapes[i].argnum2_glib_context
               == shape->argnum2_glib_context
            && old_shapes->shapes[i].argtotal == shape->argtotal)
          {
            old_shapes->shapes[i].xcomments = shape->xcomments;
            found = true;
            break;
          }

      if (!found)
        {
          /* Replace the existing 'struct callshapes' with a new one.  */
          struct callshapes *shapes =
            (struct callshapes *)
            xmalloc (xsum (sizeof (struct callshapes),
                           xtimes (old_shapes->nshapes,
                                   sizeof (struct callshape))));

          shapes->keyword = old_shapes->keyword;
          shapes->keyword_len = old_shapes->keyword_len;
          shapes->nshapes = old_shapes->nshapes + 1;
          for (i = 0; i < old_shapes->nshapes; i++)
            shapes->shapes[i] = old_shapes->shapes[i];
          shapes->shapes[i] = *shape;
          if (hash_set_value (table, keyword, keyword_len, shapes))
            abort ();
          free (old_shapes);
        }
    }
}


/* Null context.  */
flag_context_ty null_context = { undecided, false, undecided, false };

/* Transparent context.  */
flag_context_ty passthrough_context = { undecided, true, undecided, true };


flag_context_ty
inherited_context (flag_context_ty outer_context,
                   flag_context_ty modifier_context)
{
  flag_context_ty result = modifier_context;

  if (result.pass_format1)
    {
      result.is_format1 = outer_context.is_format1;
      result.pass_format1 = false;
    }
  if (result.pass_format2)
    {
      result.is_format2 = outer_context.is_format2;
      result.pass_format2 = false;
    }
  if (result.pass_format3)
    {
      result.is_format3 = outer_context.is_format3;
      result.pass_format3 = false;
    }
  return result;
}


/* Null context list iterator.  */
flag_context_list_iterator_ty null_context_list_iterator = { 1, NULL };

/* Transparent context list iterator.  */
static flag_context_list_ty passthrough_context_circular_list =
  {
    1,
    { undecided, true, undecided, true },
    &passthrough_context_circular_list
  };
flag_context_list_iterator_ty passthrough_context_list_iterator =
  {
    1,
    &passthrough_context_circular_list
  };


flag_context_list_iterator_ty
flag_context_list_iterator (flag_context_list_ty *list)
{
  flag_context_list_iterator_ty result;

  result.argnum = 1;
  result.head = list;
  return result;
}


flag_context_ty
flag_context_list_iterator_advance (flag_context_list_iterator_ty *iter)
{
  if (iter->head == NULL)
    return null_context;
  if (iter->argnum == iter->head->argnum)
    {
      flag_context_ty result = iter->head->flags;

      /* Special casing of circular list.  */
      if (iter->head != iter->head->next)
        {
          iter->head = iter->head->next;
          iter->argnum++;
        }

      return result;
    }
  else
    {
      iter->argnum++;
      return null_context;
    }
}


flag_context_list_ty *
flag_context_list_table_lookup (flag_context_list_table_ty *flag_table,
                                const void *key, size_t keylen)
{
  void *entry;

  if (flag_table->table != NULL
      && hash_find_entry (flag_table, key, keylen, &entry) == 0)
    return (flag_context_list_ty *) entry;
  else
    return NULL;
}


static void
flag_context_list_table_insert (flag_context_list_table_ty *table,
                                unsigned int index,
                                const char *name_start, const char *name_end,
                                int argnum, enum is_format value, bool pass)
{
  char *allocated_name = NULL;

  if (table == &flag_table_lisp)
    {
      /* Convert NAME to upper case.  */
      size_t name_len = name_end - name_start;
      char *name = allocated_name = (char *) xmalloca (name_len);
      size_t i;

      for (i = 0; i < name_len; i++)
        name[i] = (name_start[i] >= 'a' && name_start[i] <= 'z'
                   ? name_start[i] - 'a' + 'A'
                   : name_start[i]);
      name_start = name;
      name_end = name + name_len;
    }
  else if (table == &flag_table_tcl)
    {
      /* Remove redundant "::" prefix.  */
      if (name_end - name_start > 2
          && name_start[0] == ':' && name_start[1] == ':')
        name_start += 2;
    }

  /* Insert the pair (VALUE, PASS) at INDEX in the element numbered ARGNUM
     of the list corresponding to NAME in the TABLE.  */
  if (table->table == NULL)
    hash_init (table, 100);
  {
    void *entry;

    if (hash_find_entry (table, name_start, name_end - name_start, &entry) != 0)
      {
        /* Create new hash table entry.  */
        flag_context_list_ty *list = XMALLOC (flag_context_list_ty);
        list->argnum = argnum;
        memset (&list->flags, '\0', sizeof (list->flags));
        switch (index)
          {
          case 0:
            list->flags.is_format1 = value;
            list->flags.pass_format1 = pass;
            break;
          case 1:
            list->flags.is_format2 = value;
            list->flags.pass_format2 = pass;
            break;
          case 2:
            list->flags.is_format3 = value;
            list->flags.pass_format3 = pass;
            break;
          default:
            abort ();
          }
        list->next = NULL;
        hash_insert_entry (table, name_start, name_end - name_start, list);
      }
    else
      {
        flag_context_list_ty *list = (flag_context_list_ty *)entry;
        flag_context_list_ty **lastp = NULL;
        /* Invariant: list == (lastp != NULL ? *lastp : entry).  */

        while (list != NULL && list->argnum < argnum)
          {
            lastp = &list->next;
            list = *lastp;
          }
        if (list != NULL && list->argnum == argnum)
          {
            /* Add this flag to the current argument number.  */
            switch (index)
              {
              case 0:
                list->flags.is_format1 = value;
                list->flags.pass_format1 = pass;
                break;
              case 1:
                list->flags.is_format2 = value;
                list->flags.pass_format2 = pass;
                break;
              case 2:
                list->flags.is_format3 = value;
                list->flags.pass_format3 = pass;
                break;
              default:
                abort ();
              }
          }
        else if (lastp != NULL)
          {
            /* Add a new list entry for this argument number.  */
            list = XMALLOC (flag_context_list_ty);
            list->argnum = argnum;
            memset (&list->flags, '\0', sizeof (list->flags));
            switch (index)
              {
              case 0:
                list->flags.is_format1 = value;
                list->flags.pass_format1 = pass;
                break;
              case 1:
                list->flags.is_format2 = value;
                list->flags.pass_format2 = pass;
                break;
              case 2:
                list->flags.is_format3 = value;
                list->flags.pass_format3 = pass;
                break;
              default:
                abort ();
              }
            list->next = *lastp;
            *lastp = list;
          }
        else
          {
            /* Add a new list entry for this argument number, at the beginning
               of the list.  Since we don't have an API for replacing the
               value of a key in the hash table, we have to copy the first
               list element.  */
            flag_context_list_ty *copy = XMALLOC (flag_context_list_ty);
            *copy = *list;

            list->argnum = argnum;
            memset (&list->flags, '\0', sizeof (list->flags));
            switch (index)
              {
              case 0:
                list->flags.is_format1 = value;
                list->flags.pass_format1 = pass;
                break;
              case 1:
                list->flags.is_format2 = value;
                list->flags.pass_format2 = pass;
                break;
              case 2:
                list->flags.is_format3 = value;
                list->flags.pass_format3 = pass;
                break;
              default:
                abort ();
              }
            list->next = copy;
          }
      }
  }

  if (allocated_name != NULL)
    freea (allocated_name);
}


void
xgettext_record_flag (const char *optionstring)
{
  /* Check the string has at least two colons.  (Colons in the name are
     allowed, needed for the Lisp and the Tcl backends.)  */
  const char *colon1;
  const char *colon2;

  for (colon2 = optionstring + strlen (optionstring); ; )
    {
      if (colon2 == optionstring)
        goto err;
      colon2--;
      if (*colon2 == ':')
        break;
    }
  for (colon1 = colon2; ; )
    {
      if (colon1 == optionstring)
        goto err;
      colon1--;
      if (*colon1 == ':')
        break;
    }
  {
    const char *name_start = optionstring;
    const char *name_end = colon1;
    const char *argnum_start = colon1 + 1;
    const char *argnum_end = colon2;
    const char *flag = colon2 + 1;
    int argnum;

    /* Check the parts' syntax.  */
    if (name_end == name_start)
      goto err;
    if (argnum_end == argnum_start)
      goto err;
    {
      char *endp;
      argnum = strtol (argnum_start, &endp, 10);
      if (endp != argnum_end)
        goto err;
    }
    if (argnum <= 0)
      goto err;

    /* Analyze the flag part.  */
    {
      bool pass;

      pass = false;
      if (strlen (flag) >= 5 && memcmp (flag, "pass-", 5) == 0)
        {
          pass = true;
          flag += 5;
        }

      /* Unlike po_parse_comment_special(), we don't accept "fuzzy",
         "wrap", or "check" here - it has no sense.  */
      if (strlen (flag) >= 7
          && memcmp (flag + strlen (flag) - 7, "-format", 7) == 0)
        {
          const char *p;
          size_t n;
          enum is_format value;
          size_t type;

          p = flag;
          n = strlen (flag) - 7;

          if (n >= 3 && memcmp (p, "no-", 3) == 0)
            {
              p += 3;
              n -= 3;
              value = no;
            }
          else if (n >= 9 && memcmp (p, "possible-", 9) == 0)
            {
              p += 9;
              n -= 9;
              value = possible;
            }
          else if (n >= 11 && memcmp (p, "impossible-", 11) == 0)
            {
              p += 11;
              n -= 11;
              value = impossible;
            }
          else
            value = yes_according_to_context;

          for (type = 0; type < NFORMATS; type++)
            if (strlen (format_language[type]) == n
                && memcmp (format_language[type], p, n) == 0)
              {
                switch (type)
                  {
                  case format_c:
                    flag_context_list_table_insert (&flag_table_c, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    flag_context_list_table_insert (&flag_table_cxx_qt, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    flag_context_list_table_insert (&flag_table_cxx_kde, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    flag_context_list_table_insert (&flag_table_cxx_boost, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    flag_context_list_table_insert (&flag_table_objc, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_objc:
                    flag_context_list_table_insert (&flag_table_objc, 1,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_sh:
                    flag_context_list_table_insert (&flag_table_sh, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_python:
                    flag_context_list_table_insert (&flag_table_python, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_python_brace:
                    flag_context_list_table_insert (&flag_table_python, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_lisp:
                    flag_context_list_table_insert (&flag_table_lisp, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_elisp:
                    flag_context_list_table_insert (&flag_table_elisp, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_librep:
                    flag_context_list_table_insert (&flag_table_librep, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_scheme:
                    flag_context_list_table_insert (&flag_table_scheme, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_smalltalk:
                    break;
                  case format_java:
                    flag_context_list_table_insert (&flag_table_java, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_csharp:
                    flag_context_list_table_insert (&flag_table_csharp, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_awk:
                    flag_context_list_table_insert (&flag_table_awk, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_pascal:
                    break;
                  case format_ycp:
                    flag_context_list_table_insert (&flag_table_ycp, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_tcl:
                    flag_context_list_table_insert (&flag_table_tcl, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_perl:
                    flag_context_list_table_insert (&flag_table_perl, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_perl_brace:
                    flag_context_list_table_insert (&flag_table_perl, 1,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_php:
                    flag_context_list_table_insert (&flag_table_php, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_gcc_internal:
                    flag_context_list_table_insert (&flag_table_gcc_internal, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_gfc_internal:
                    flag_context_list_table_insert (&flag_table_gcc_internal, 1,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_qt:
                    flag_context_list_table_insert (&flag_table_cxx_qt, 1,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_qt_plural:
                    flag_context_list_table_insert (&flag_table_cxx_qt, 2,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_kde:
                    flag_context_list_table_insert (&flag_table_cxx_kde, 1,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_kde_kuit:
                    flag_context_list_table_insert (&flag_table_cxx_kde, 2,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_boost:
                    flag_context_list_table_insert (&flag_table_cxx_boost, 1,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_lua:
                    flag_context_list_table_insert (&flag_table_lua, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  case format_javascript:
                    flag_context_list_table_insert (&flag_table_javascript, 0,
                                                    name_start, name_end,
                                                    argnum, value, pass);
                    break;
                  default:
                    abort ();
                  }
                return;
              }
          /* If the flag is not among the valid values, the optionstring is
             invalid.  */
        }
    }
  }

err:
  error (EXIT_FAILURE, 0, _("\
A --flag argument doesn't have the <keyword>:<argnum>:[pass-]<flag> syntax: %s"),
         optionstring);
}


/* Comment handling: There is a list of automatic comments that may be appended
   to the next message.  Used by remember_a_message().  */

static string_list_ty *comment;

static void
xgettext_comment_add (const char *str)
{
  if (comment == NULL)
    comment = string_list_alloc ();
  string_list_append (comment, str);
}

static const char *
xgettext_comment (size_t n)
{
  if (comment == NULL || n >= comment->nitems)
    return NULL;
  return comment->item[n];
}

static void
xgettext_comment_reset ()
{
  if (comment != NULL)
    {
      string_list_free (comment);
      comment = NULL;
    }
}


refcounted_string_list_ty *savable_comment;

void
savable_comment_add (const char *str)
{
  if (savable_comment == NULL)
    {
      savable_comment = XMALLOC (refcounted_string_list_ty);
      savable_comment->refcount = 1;
      string_list_init (&savable_comment->contents);
    }
  else if (savable_comment->refcount > 1)
    {
      /* Unshare the list by making copies.  */
      struct string_list_ty *oldcontents;
      size_t i;

      savable_comment->refcount--;
      oldcontents = &savable_comment->contents;

      savable_comment = XMALLOC (refcounted_string_list_ty);
      savable_comment->refcount = 1;
      string_list_init (&savable_comment->contents);
      for (i = 0; i < oldcontents->nitems; i++)
        string_list_append (&savable_comment->contents, oldcontents->item[i]);
    }
  string_list_append (&savable_comment->contents, str);
}

void
savable_comment_reset ()
{
  drop_reference (savable_comment);
  savable_comment = NULL;
}

static void
savable_comment_to_xgettext_comment (refcounted_string_list_ty *rslp)
{
  xgettext_comment_reset ();
  if (rslp != NULL)
    {
      size_t i;

      for (i = 0; i < rslp->contents.nitems; i++)
        xgettext_comment_add (rslp->contents.item[i]);
    }
}

refcounted_string_list_ty *
savable_comment_convert_encoding (refcounted_string_list_ty *comment,
                                  lex_pos_ty *pos)
{
  refcounted_string_list_ty *result;
  size_t i;

  result = XMALLOC (refcounted_string_list_ty);
  result->refcount = 1;
  string_list_init (&result->contents);

  for (i = 0; i < comment->contents.nitems; i++)
    {
      const char *old_string = comment->contents.item[i];
      char *string = from_current_source_encoding (old_string,
                                                   lc_comment,
                                                   pos->file_name,
                                                   pos->line_number);
      string_list_append (&result->contents, string);
      if (string != old_string)
        free (string);
    }

  return result;
}



static FILE *
xgettext_open (const char *fn,
               char **logical_file_name_p, char **real_file_name_p)
{
  FILE *fp;
  char *new_name;
  char *logical_file_name;

  if (strcmp (fn, "-") == 0)
    {
      new_name = xstrdup (_("standard input"));
      logical_file_name = xstrdup (new_name);
      fp = stdin;
    }
  else if (IS_ABSOLUTE_PATH (fn))
    {
      new_name = xstrdup (fn);
      fp = fopen (fn, "r");
      if (fp == NULL)
        error (EXIT_FAILURE, errno, _("\
error while opening \"%s\" for reading"), fn);
      logical_file_name = xstrdup (new_name);
    }
  else
    {
      int j;

      for (j = 0; ; ++j)
        {
          const char *dir = dir_list_nth (j);

          if (dir == NULL)
            error (EXIT_FAILURE, ENOENT, _("\
error while opening \"%s\" for reading"), fn);

          new_name = xconcatenated_filename (dir, fn, NULL);

          fp = fopen (new_name, "r");
          if (fp != NULL)
            break;

          if (errno != ENOENT)
            error (EXIT_FAILURE, errno, _("\
error while opening \"%s\" for reading"), new_name);
          free (new_name);
        }

      /* Note that the NEW_NAME variable contains the actual file name
         and the logical file name is what is reported by xgettext.  In
         this case NEW_NAME is set to the file which was found along the
         directory search path, and LOGICAL_FILE_NAME is is set to the
         file name which was searched for.  */
      logical_file_name = xstrdup (fn);
    }

  *logical_file_name_p = logical_file_name;
  *real_file_name_p = new_name;
  return fp;
}


/* Language dependent format string parser.
   NULL if the language has no notion of format strings.  */
static struct formatstring_parser *current_formatstring_parser1;
static struct formatstring_parser *current_formatstring_parser2;
static struct formatstring_parser *current_formatstring_parser3;

static struct literalstring_parser *current_literalstring_parser;

static void
extract_from_file (const char *file_name, extractor_ty extractor,
                   msgdomain_list_ty *mdlp)
{
  char *logical_file_name;
  char *real_file_name;
  FILE *fp = xgettext_open (file_name, &logical_file_name, &real_file_name);

  /* Set the default for the source file encoding.  May be overridden by
     the extractor function.  */
  xgettext_current_source_encoding = xgettext_global_source_encoding;
#if HAVE_ICONV
  xgettext_current_source_iconv = xgettext_global_source_iconv;
#endif

  current_formatstring_parser1 = extractor.formatstring_parser1;
  current_formatstring_parser2 = extractor.formatstring_parser2;
  current_formatstring_parser3 = extractor.formatstring_parser3;
  current_literalstring_parser = extractor.literalstring_parser;
  extractor.func (fp, real_file_name, logical_file_name, extractor.flag_table,
                  mdlp);

  if (fp != stdin)
    fclose (fp);
  free (logical_file_name);
  free (real_file_name);
}

static message_ty *
xgettext_its_extract_callback (message_list_ty *mlp,
                               const char *msgctxt,
                               const char *msgid,
                               lex_pos_ty *pos,
                               const char *extracted_comment,
                               const char *marker,
                               enum its_whitespace_type_ty whitespace)
{
  message_ty *message;

  message = remember_a_message (mlp,
                                msgctxt == NULL ? NULL : xstrdup (msgctxt),
                                xstrdup (msgid),
                                null_context, pos,
                                extracted_comment, NULL);

  if (add_itstool_comments)
    {
      char *dot = xasprintf ("(itstool) path: %s", marker);
      message_comment_dot_append (message, dot);
      free (dot);

      if (whitespace == ITS_WHITESPACE_PRESERVE)
        message->do_wrap = no;
    }

  return message;
}

static void
extract_from_xml_file (const char *file_name,
                       its_rule_list_ty *rules,
                       msgdomain_list_ty *mdlp)
{
  char *logical_file_name;
  char *real_file_name;
  FILE *fp = xgettext_open (file_name, &logical_file_name, &real_file_name);

  /* The default encoding for XML is UTF-8.  It can be overridden by
     an XML declaration in the XML file itself, not through the
     --from-code option.  */
  xgettext_current_source_encoding = po_charset_utf8;

#if HAVE_ICONV
  xgettext_current_source_iconv = xgettext_global_source_iconv;
#endif

  its_rule_list_extract (rules, fp, real_file_name, logical_file_name,
                         NULL,
                         mdlp,
                         xgettext_its_extract_callback);

  if (fp != stdin)
    fclose (fp);
  free (logical_file_name);
  free (real_file_name);
}



/* Error message about non-ASCII character in a specific lexical context.  */
char *
non_ascii_error_message (lexical_context_ty lcontext,
                         const char *file_name, size_t line_number)
{
  char buffer[21];
  char *errmsg;

  if (line_number == (size_t)(-1))
    buffer[0] = '\0';
  else
    sprintf (buffer, ":%ld", (long) line_number);

  switch (lcontext)
    {
    case lc_outside:
      errmsg =
        xasprintf (_("Non-ASCII character at %s%s."), file_name, buffer);
      break;
    case lc_comment:
      errmsg =
        xasprintf (_("Non-ASCII comment at or before %s%s."),
                   file_name, buffer);
      break;
    case lc_string:
      errmsg =
        xasprintf (_("Non-ASCII string at %s%s."), file_name, buffer);
      break;
    default:
      abort ();
    }
  return errmsg;
}

/* Convert the given string from xgettext_current_source_encoding to
   the output file encoding (i.e. ASCII or UTF-8).
   The resulting string is either the argument string, or freshly allocated.
   The file_name and line_number are only used for error message purposes.  */
char *
from_current_source_encoding (const char *string,
                              lexical_context_ty lcontext,
                              const char *file_name, size_t line_number)
{
  if (xgettext_current_source_encoding == po_charset_ascii)
    {
      if (!is_ascii_string (string))
        {
          multiline_error (xstrdup (""),
                           xasprintf ("%s\n%s\n",
                                      non_ascii_error_message (lcontext,
                                                               file_name,
                                                               line_number),
                                      _("\
Please specify the source encoding through --from-code.")));
          exit (EXIT_FAILURE);
        }
    }
  else if (xgettext_current_source_encoding != po_charset_utf8)
    {
#if HAVE_ICONV
      struct conversion_context context;

      context.from_code = xgettext_current_source_encoding;
      context.to_code = po_charset_utf8;
      context.from_filename = file_name;
      context.message = NULL;

      string = convert_string_directly (xgettext_current_source_iconv, string,
                                        &context);
#else
      /* If we don't have iconv(), the only supported values for
         xgettext_global_source_encoding and thus also for
         xgettext_current_source_encoding are ASCII and UTF-8.
         convert_string_directly() should not be called in this case.  */
      abort ();
#endif
    }

  return (char *) string;
}

#define CONVERT_STRING(string, lcontext) \
  string = from_current_source_encoding (string, lcontext, pos->file_name, \
                                         pos->line_number);


/* Update the is_format[] flags depending on the information given in the
   context.  */
static void
set_format_flags_from_context (enum is_format is_format[NFORMATS],
                               flag_context_ty context, const char *string,
                               lex_pos_ty *pos, const char *pretty_msgstr)
{
  size_t i;

  if (context.is_format1 != undecided
      || context.is_format2 != undecided
      || context.is_format3 != undecided)
    for (i = 0; i < NFORMATS; i++)
      {
        if (is_format[i] == undecided)
          {
            if (formatstring_parsers[i] == current_formatstring_parser1
                && context.is_format1 != undecided)
              is_format[i] = (enum is_format) context.is_format1;
            if (formatstring_parsers[i] == current_formatstring_parser2
                && context.is_format2 != undecided)
              is_format[i] = (enum is_format) context.is_format2;
            if (formatstring_parsers[i] == current_formatstring_parser3
                && context.is_format3 != undecided)
              is_format[i] = (enum is_format) context.is_format3;
          }
        if (possible_format_p (is_format[i]))
          {
            struct formatstring_parser *parser = formatstring_parsers[i];
            char *invalid_reason = NULL;
            void *descr = parser->parse (string, false, NULL, &invalid_reason);

            if (descr != NULL)
              parser->free (descr);
            else
              {
                /* The string is not a valid format string.  */
                if (is_format[i] != possible)
                  {
                    char buffer[21];

                    error_with_progname = false;
                    if (pos->line_number == (size_t)(-1))
                      buffer[0] = '\0';
                    else
                      sprintf (buffer, ":%ld", (long) pos->line_number);
                    multiline_warning (xasprintf (_("%s%s: warning: "),
                                                  pos->file_name, buffer),
                                       xasprintf (is_format[i] == yes_according_to_context
                                                  ? _("Although being used in a format string position, the %s is not a valid %s format string. Reason: %s\n")
                                                  : _("Although declared as such, the %s is not a valid %s format string. Reason: %s\n"),
                                                  pretty_msgstr,
                                                  format_language_pretty[i],
                                                  invalid_reason));
                    error_with_progname = true;
                  }

                is_format[i] = impossible;
                free (invalid_reason);
              }
          }
      }
}


static void
warn_format_string (enum is_format is_format[NFORMATS], const char *string,
                    lex_pos_ty *pos, const char *pretty_msgstr)
{
  if (possible_format_p (is_format[format_python])
      && get_python_format_unnamed_arg_count (string) > 1)
    {
      char buffer[21];

      error_with_progname = false;
      if (pos->line_number == (size_t)(-1))
        buffer[0] = '\0';
      else
        sprintf (buffer, ":%ld", (long) pos->line_number);
      multiline_warning (xasprintf (_("%s%s: warning: "),
                                    pos->file_name, buffer),
                         xasprintf (_("\
'%s' format string with unnamed arguments cannot be properly localized:\n\
The translator cannot reorder the arguments.\n\
Please consider using a format string with named arguments,\n\
and a mapping instead of a tuple for the arguments.\n"),
                                    pretty_msgstr));
      error_with_progname = true;
    }
}


message_ty *
remember_a_message (message_list_ty *mlp, char *msgctxt, char *msgid,
                    flag_context_ty context, lex_pos_ty *pos,
                    const char *extracted_comment,
                    refcounted_string_list_ty *comment)
{
  enum is_format is_format[NFORMATS];
  struct argument_range range;
  enum is_wrap do_wrap;
  enum is_syntax_check do_syntax_check[NSYNTAXCHECKS];
  message_ty *mp;
  char *msgstr;
  size_t i;

  /* See whether we shall exclude this message.  */
  if (exclude != NULL && message_list_search (exclude, msgctxt, msgid) != NULL)
    {
      /* Tell the lexer to reset its comment buffer, so that the next
         message gets the correct comments.  */
      xgettext_comment_reset ();
      savable_comment_reset ();

      if (msgctxt != NULL)
        free (msgctxt);
      free (msgid);

      return NULL;
    }

  savable_comment_to_xgettext_comment (comment);

  for (i = 0; i < NFORMATS; i++)
    is_format[i] = undecided;
  range.min = -1;
  range.max = -1;
  do_wrap = undecided;
  for (i = 0; i < NSYNTAXCHECKS; i++)
    do_syntax_check[i] = undecided;

  if (msgctxt != NULL)
    CONVERT_STRING (msgctxt, lc_string);
  CONVERT_STRING (msgid, lc_string);

  if (msgctxt == NULL && msgid[0] == '\0' && !xgettext_omit_header)
    {
      char buffer[21];

      error_with_progname = false;
      if (pos->line_number == (size_t)(-1))
        buffer[0] = '\0';
      else
        sprintf (buffer, ":%ld", (long) pos->line_number);
      multiline_warning (xasprintf (_("%s%s: warning: "), pos->file_name,
                                    buffer),
                         xstrdup (_("\
Empty msgid.  It is reserved by GNU gettext:\n\
gettext(\"\") returns the header entry with\n\
meta information, not the empty string.\n")));
      error_with_progname = true;
    }

  /* See if we have seen this message before.  */
  mp = message_list_search (mlp, msgctxt, msgid);
  if (mp != NULL)
    {
      if (msgctxt != NULL)
        free (msgctxt);
      free (msgid);
      for (i = 0; i < NFORMATS; i++)
        is_format[i] = mp->is_format[i];
      do_wrap = mp->do_wrap;
      for (i = 0; i < NSYNTAXCHECKS; i++)
        do_syntax_check[i] = mp->do_syntax_check[i];
    }
  else
    {
      /* Construct the msgstr from the prefix and suffix, otherwise use the
         empty string.  */
      if (msgstr_prefix)
        msgstr = xasprintf ("%s%s%s", msgstr_prefix, msgid, msgstr_suffix);
      else
        msgstr = "";

      /* Allocate a new message and append the message to the list.  */
      mp = message_alloc (msgctxt, msgid, NULL, msgstr, strlen (msgstr) + 1,
                          pos);
      /* Do not free msgctxt and msgid.  */
      message_list_append (mlp, mp);
    }

  /* Determine whether the context specifies that the msgid is a format
     string.  */
  set_format_flags_from_context (is_format, context, mp->msgid, pos, "msgid");

  /* Ask the lexer for the comments it has seen.  */
  {
    size_t nitems_before;
    size_t nitems_after;
    int j;
    bool add_all_remaining_comments;
    /* The string before the comment tag.  For example, If "** TRANSLATORS:"
       is seen and the comment tag is "TRANSLATORS:",
       then comment_tag_prefix is set to "** ".  */
    const char *comment_tag_prefix = "";
    size_t comment_tag_prefix_length = 0;

    nitems_before = (mp->comment_dot != NULL ? mp->comment_dot->nitems : 0);

    if (extracted_comment != NULL)
      {
        char *copy = xstrdup (extracted_comment);
        char *rest;

        rest = copy;
        while (*rest != '\0')
          {
            char *newline = strchr (rest, '\n');

            if (newline != NULL)
              {
                *newline = '\0';
                message_comment_dot_append (mp, rest);
                rest = newline + 1;
              }
            else
              {
                message_comment_dot_append (mp, rest);
                break;
              }
          }
        free (copy);
      }

    add_all_remaining_comments = add_all_comments;
    for (j = 0; ; ++j)
      {
        const char *s = xgettext_comment (j);
        const char *t;
        if (s == NULL)
          break;

        CONVERT_STRING (s, lc_comment);

        /* To reduce the possibility of unwanted matches we do a two
           step match: the line must contain 'xgettext:' and one of
           the possible format description strings.  */
        if ((t = c_strstr (s, "xgettext:")) != NULL)
          {
            bool tmp_fuzzy;
            enum is_format tmp_format[NFORMATS];
            struct argument_range tmp_range;
            enum is_wrap tmp_wrap;
            enum is_syntax_check tmp_syntax_check[NSYNTAXCHECKS];
            bool interesting;

            t += strlen ("xgettext:");

            po_parse_comment_special (t, &tmp_fuzzy, tmp_format, &tmp_range,
                                      &tmp_wrap, tmp_syntax_check);

            interesting = false;
            for (i = 0; i < NFORMATS; i++)
              if (tmp_format[i] != undecided)
                {
                  is_format[i] = tmp_format[i];
                  interesting = true;
                }
            if (has_range_p (tmp_range))
              {
                range = tmp_range;
                interesting = true;
              }
            if (tmp_wrap != undecided)
              {
                do_wrap = tmp_wrap;
                interesting = true;
              }
            for (i = 0; i < NSYNTAXCHECKS; i++)
              if (tmp_syntax_check[i] != undecided)
                {
                  do_syntax_check[i] = tmp_syntax_check[i];
                  interesting = true;
                }

            /* If the "xgettext:" marker was followed by an interesting
               keyword, and we updated our is_format/do_wrap variables,
               we don't print the comment as a #. comment.  */
            if (interesting)
              continue;
          }

        if (!add_all_remaining_comments && comment_tag != NULL)
          {
            /* When the comment tag is seen, it drags in not only the line
               which it starts, but all remaining comment lines.  */
            if ((t = c_strstr (s, comment_tag)) != NULL)
              {
                add_all_remaining_comments = true;
                comment_tag_prefix = s;
                comment_tag_prefix_length = t - s;
              }
          }

        if (add_all_remaining_comments)
          {
            if (strncmp (s, comment_tag_prefix, comment_tag_prefix_length) == 0)
              s += comment_tag_prefix_length;
            message_comment_dot_append (mp, s);
          }
      }

    nitems_after = (mp->comment_dot != NULL ? mp->comment_dot->nitems : 0);

    /* Don't add the comments if they are a repetition of the tail of the
       already present comments.  This avoids unneeded duplication if the
       same message appears several times, each time with the same comment.  */
    if (nitems_before < nitems_after)
      {
        size_t added = nitems_after - nitems_before;

        if (added <= nitems_before)
          {
            bool repeated = true;

            for (i = 0; i < added; i++)
              if (strcmp (mp->comment_dot->item[nitems_before - added + i],
                          mp->comment_dot->item[nitems_before + i]) != 0)
                {
                  repeated = false;
                  break;
                }

            if (repeated)
              {
                for (i = 0; i < added; i++)
                  free ((char *) mp->comment_dot->item[nitems_before + i]);
                mp->comment_dot->nitems = nitems_before;
              }
          }
      }
  }

  /* If it is not already decided, through programmer comments, whether the
     msgid is a format string, examine the msgid.  This is a heuristic.  */
  for (i = 0; i < NFORMATS; i++)
    {
      if (is_format[i] == undecided
          && (formatstring_parsers[i] == current_formatstring_parser1
              || formatstring_parsers[i] == current_formatstring_parser2
              || formatstring_parsers[i] == current_formatstring_parser3)
          /* But avoid redundancy: objc-format is stronger than c-format.  */
          && !(i == format_c && possible_format_p (is_format[format_objc]))
          && !(i == format_objc && possible_format_p (is_format[format_c]))
          /* Avoid flagging a string as c-format when it's known to be a
             qt-format or qt-plural-format or kde-format or boost-format
             string.  */
          && !(i == format_c
               && (possible_format_p (is_format[format_qt])
                   || possible_format_p (is_format[format_qt_plural])
                   || possible_format_p (is_format[format_kde])
                   || possible_format_p (is_format[format_kde_kuit])
                   || possible_format_p (is_format[format_boost])))
          /* Avoid flagging a string as kde-format when it's known to
             be a kde-kuit-format string.  */
          && !(i == format_kde
               && possible_format_p (is_format[format_kde_kuit]))
          /* Avoid flagging a string as kde-kuit-format when it's
             known to be a kde-format string.  Note that this relies
             on the fact that format_kde < format_kde_kuit, so a
             string will be marked as kde-format if both are
             undecided.  */
          && !(i == format_kde_kuit
               && possible_format_p (is_format[format_kde])))
        {
          struct formatstring_parser *parser = formatstring_parsers[i];
          char *invalid_reason = NULL;
          void *descr = parser->parse (mp->msgid, false, NULL, &invalid_reason);

          if (descr != NULL)
            {
              /* msgid is a valid format string.  We mark only those msgids
                 as format strings which contain at least one format directive
                 and thus are format strings with a high probability.  We
                 don't mark strings without directives as format strings,
                 because that would force the programmer to add
                 "xgettext: no-c-format" anywhere where a translator wishes
                 to use a percent sign.  So, the msgfmt checking will not be
                 perfect.  Oh well.  */
              if (parser->get_number_of_directives (descr) > 0
                  && !(parser->is_unlikely_intentional != NULL
                       && parser->is_unlikely_intentional (descr)))
                is_format[i] = possible;

              parser->free (descr);
            }
          else
            {
              /* msgid is not a valid format string.  */
              is_format[i] = impossible;
              free (invalid_reason);
            }
        }
      mp->is_format[i] = is_format[i];
    }

  if (has_range_p (range))
    {
      if (has_range_p (mp->range))
        {
          if (range.min < mp->range.min)
            mp->range.min = range.min;
          if (range.max > mp->range.max)
            mp->range.max = range.max;
        }
      else
        mp->range = range;
    }

  mp->do_wrap = do_wrap == no ? no : yes;       /* By default we wrap.  */

  for (i = 0; i < NSYNTAXCHECKS; i++)
    {
      if (do_syntax_check[i] == undecided)
        do_syntax_check[i] = default_syntax_check[i] == yes ? yes : no;

      mp->do_syntax_check[i] = do_syntax_check[i];
    }

  /* Warn about the use of non-reorderable format strings when the programming
     language also provides reorderable format strings.  */
  warn_format_string (is_format, mp->msgid, pos, "msgid");

  /* Remember where we saw this msgid.  */
  message_comment_filepos (mp, pos->file_name, pos->line_number);

  /* Tell the lexer to reset its comment buffer, so that the next
     message gets the correct comments.  */
  xgettext_comment_reset ();
  savable_comment_reset ();

  return mp;
}


void
remember_a_message_plural (message_ty *mp, char *string,
                           flag_context_ty context, lex_pos_ty *pos,
                           refcounted_string_list_ty *comment)
{
  char *msgid_plural;
  char *msgstr1;
  size_t msgstr1_len;
  char *msgstr;
  size_t i;

  msgid_plural = string;

  savable_comment_to_xgettext_comment (comment);

  CONVERT_STRING (msgid_plural, lc_string);

  /* See if the message is already a plural message.  */
  if (mp->msgid_plural == NULL)
    {
      mp->msgid_plural = msgid_plural;

      /* Construct the first plural form from the prefix and suffix,
         otherwise use the empty string.  The translator will have to
         provide additional plural forms.  */
      if (msgstr_prefix)
        msgstr1 =
          xasprintf ("%s%s%s", msgstr_prefix, msgid_plural, msgstr_suffix);
      else
        msgstr1 = "";
      msgstr1_len = strlen (msgstr1) + 1;
      msgstr = XNMALLOC (mp->msgstr_len + msgstr1_len, char);
      memcpy (msgstr, mp->msgstr, mp->msgstr_len);
      memcpy (msgstr + mp->msgstr_len, msgstr1, msgstr1_len);
      mp->msgstr = msgstr;
      mp->msgstr_len = mp->msgstr_len + msgstr1_len;
      if (msgstr_prefix)
        free (msgstr1);

      /* Determine whether the context specifies that the msgid_plural is a
         format string.  */
      set_format_flags_from_context (mp->is_format, context, mp->msgid_plural,
                                     pos, "msgid_plural");

      /* If it is not already decided, through programmer comments or
         the msgid, whether the msgid is a format string, examine the
         msgid_plural.  This is a heuristic.  */
      for (i = 0; i < NFORMATS; i++)
        if ((formatstring_parsers[i] == current_formatstring_parser1
             || formatstring_parsers[i] == current_formatstring_parser2
             || formatstring_parsers[i] == current_formatstring_parser3)
            && (mp->is_format[i] == undecided || mp->is_format[i] == possible)
            /* But avoid redundancy: objc-format is stronger than c-format.  */
            && !(i == format_c
                 && possible_format_p (mp->is_format[format_objc]))
            && !(i == format_objc
                 && possible_format_p (mp->is_format[format_c]))
            /* Avoid flagging a string as c-format when it's known to be a
               qt-format or qt-plural-format or boost-format string.  */
            && !(i == format_c
                 && (possible_format_p (mp->is_format[format_qt])
                     || possible_format_p (mp->is_format[format_qt_plural])
                     || possible_format_p (mp->is_format[format_kde])
                     || possible_format_p (mp->is_format[format_kde_kuit])
                     || possible_format_p (mp->is_format[format_boost])))
            /* Avoid flagging a string as kde-format when it's known
               to be a kde-kuit-format string.  */
            && !(i == format_kde
                 && possible_format_p (mp->is_format[format_kde_kuit]))
            /* Avoid flagging a string as kde-kuit-format when it's
               known to be a kde-format string.  Note that this relies
               on the fact that format_kde < format_kde_kuit, so a
               string will be marked as kde-format if both are
               undecided.  */
            && !(i == format_kde_kuit
                 && possible_format_p (mp->is_format[format_kde])))
          {
            struct formatstring_parser *parser = formatstring_parsers[i];
            char *invalid_reason = NULL;
            void *descr =
              parser->parse (mp->msgid_plural, false, NULL, &invalid_reason);

            if (descr != NULL)
              {
                /* Same heuristic as in remember_a_message.  */
                if (parser->get_number_of_directives (descr) > 0
                    && !(parser->is_unlikely_intentional != NULL
                         && parser->is_unlikely_intentional (descr)))
                  mp->is_format[i] = possible;

                parser->free (descr);
              }
            else
              {
                /* msgid_plural is not a valid format string.  */
                mp->is_format[i] = impossible;
                free (invalid_reason);
              }
          }

      /* Warn about the use of non-reorderable format strings when the programming
         language also provides reorderable format strings.  */
      warn_format_string (mp->is_format, mp->msgid_plural, pos, "msgid_plural");
    }
  else
    free (msgid_plural);

  /* Tell the lexer to reset its comment buffer, so that the next
     message gets the correct comments.  */
  xgettext_comment_reset ();
  savable_comment_reset ();
}


struct arglist_parser *
arglist_parser_alloc (message_list_ty *mlp, const struct callshapes *shapes)
{
  if (shapes == NULL || shapes->nshapes == 0)
    {
      struct arglist_parser *ap =
        (struct arglist_parser *)
        xmalloc (offsetof (struct arglist_parser, alternative[0]));

      ap->mlp = mlp;
      ap->keyword = NULL;
      ap->keyword_len = 0;
      ap->nalternatives = 0;

      return ap;
    }
  else
    {
      struct arglist_parser *ap =
        (struct arglist_parser *)
        xmalloc (xsum (sizeof (struct arglist_parser),
                       xtimes (shapes->nshapes - 1,
                               sizeof (struct partial_call))));
      size_t i;

      ap->mlp = mlp;
      ap->keyword = shapes->keyword;
      ap->keyword_len = shapes->keyword_len;
      ap->nalternatives = shapes->nshapes;
      for (i = 0; i < shapes->nshapes; i++)
        {
          ap->alternative[i].argnumc = shapes->shapes[i].argnumc;
          ap->alternative[i].argnum1 = shapes->shapes[i].argnum1;
          ap->alternative[i].argnum2 = shapes->shapes[i].argnum2;
          ap->alternative[i].argnum1_glib_context =
            shapes->shapes[i].argnum1_glib_context;
          ap->alternative[i].argnum2_glib_context =
            shapes->shapes[i].argnum2_glib_context;
          ap->alternative[i].argtotal = shapes->shapes[i].argtotal;
          ap->alternative[i].xcomments = shapes->shapes[i].xcomments;
          ap->alternative[i].msgctxt = NULL;
          ap->alternative[i].msgctxt_escape = LET_NONE;
          ap->alternative[i].msgctxt_pos.file_name = NULL;
          ap->alternative[i].msgctxt_pos.line_number = (size_t)(-1);
          ap->alternative[i].msgid = NULL;
          ap->alternative[i].msgid_escape = LET_NONE;
          ap->alternative[i].msgid_context = null_context;
          ap->alternative[i].msgid_pos.file_name = NULL;
          ap->alternative[i].msgid_pos.line_number = (size_t)(-1);
          ap->alternative[i].msgid_comment = NULL;
          ap->alternative[i].msgid_plural = NULL;
          ap->alternative[i].msgid_plural_escape = LET_NONE;
          ap->alternative[i].msgid_plural_context = null_context;
          ap->alternative[i].msgid_plural_pos.file_name = NULL;
          ap->alternative[i].msgid_plural_pos.line_number = (size_t)(-1);
        }

      return ap;
    }
}


struct arglist_parser *
arglist_parser_clone (struct arglist_parser *ap)
{
  struct arglist_parser *copy =
    (struct arglist_parser *)
    xmalloc (xsum (sizeof (struct arglist_parser) - sizeof (struct partial_call),
                   xtimes (ap->nalternatives, sizeof (struct partial_call))));
  size_t i;

  copy->mlp = ap->mlp;
  copy->keyword = ap->keyword;
  copy->keyword_len = ap->keyword_len;
  copy->nalternatives = ap->nalternatives;
  for (i = 0; i < ap->nalternatives; i++)
    {
      const struct partial_call *cp = &ap->alternative[i];
      struct partial_call *ccp = &copy->alternative[i];

      ccp->argnumc = cp->argnumc;
      ccp->argnum1 = cp->argnum1;
      ccp->argnum2 = cp->argnum2;
      ccp->argnum1_glib_context = cp->argnum1_glib_context;
      ccp->argnum2_glib_context = cp->argnum2_glib_context;
      ccp->argtotal = cp->argtotal;
      ccp->xcomments = cp->xcomments;
      ccp->msgctxt = (cp->msgctxt != NULL ? xstrdup (cp->msgctxt) : NULL);
      ccp->msgctxt_escape = cp->msgctxt_escape;
      ccp->msgctxt_pos = cp->msgctxt_pos;
      ccp->msgid = (cp->msgid != NULL ? xstrdup (cp->msgid) : NULL);
      ccp->msgid_escape = cp->msgid_escape;
      ccp->msgid_context = cp->msgid_context;
      ccp->msgid_pos = cp->msgctxt_pos;
      ccp->msgid_comment = add_reference (cp->msgid_comment);
      ccp->msgid_plural =
        (cp->msgid_plural != NULL ? xstrdup (cp->msgid_plural) : NULL);
      ccp->msgid_plural_escape = cp->msgid_plural_escape;
      ccp->msgid_plural_context = cp->msgid_plural_context;
      ccp->msgid_plural_pos = cp->msgid_plural_pos;
    }

  return copy;
}


void
arglist_parser_remember_literal (struct arglist_parser *ap,
                                 int argnum, char *string,
                                 flag_context_ty context,
                                 char *file_name, size_t line_number,
                                 refcounted_string_list_ty *comment,
                                 enum literalstring_escape_type type)
{
  bool stored_string = false;
  size_t nalternatives = ap->nalternatives;
  size_t i;

  if (!(argnum > 0))
    abort ();
  for (i = 0; i < nalternatives; i++)
    {
      struct partial_call *cp = &ap->alternative[i];

      if (argnum == cp->argnumc)
        {
          cp->msgctxt = string;
          cp->msgctxt_escape = type;
          cp->msgctxt_pos.file_name = file_name;
          cp->msgctxt_pos.line_number = line_number;
          stored_string = true;
          /* Mark msgctxt as done.  */
          cp->argnumc = 0;
        }
      else
        {
          if (argnum == cp->argnum1)
            {
              cp->msgid = string;
              cp->msgid_escape = type;
              cp->msgid_context = context;
              cp->msgid_pos.file_name = file_name;
              cp->msgid_pos.line_number = line_number;
              cp->msgid_comment = add_reference (comment);
              stored_string = true;
              /* Mark msgid as done.  */
              cp->argnum1 = 0;
            }
          if (argnum == cp->argnum2)
            {
              cp->msgid_plural = string;
              cp->msgid_plural_escape = type;
              cp->msgid_plural_context = context;
              cp->msgid_plural_pos.file_name = file_name;
              cp->msgid_plural_pos.line_number = line_number;
              stored_string = true;
              /* Mark msgid_plural as done.  */
              cp->argnum2 = 0;
            }
        }
    }
  /* Note: There is a memory leak here: When string was stored but is later
     not used by arglist_parser_done, we don't free it.  */
  if (!stored_string)
    free (string);
}

void
arglist_parser_remember (struct arglist_parser *ap,
                         int argnum, char *string,
                         flag_context_ty context,
                         char *file_name, size_t line_number,
                         refcounted_string_list_ty *comment)
{
  arglist_parser_remember_literal (ap, argnum, string, context,
                                   file_name, line_number,
                                   comment, LET_NONE);
}

bool
arglist_parser_decidedp (struct arglist_parser *ap, int argnum)
{
  size_t i;

  /* Test whether all alternatives are decided.
     Note: A decided alternative can be complete
       cp->argnumc == 0 && cp->argnum1 == 0 && cp->argnum2 == 0
       && cp->argtotal == 0
     or it can be failed if no literal strings were found at the specified
     argument positions:
       cp->argnumc <= argnum && cp->argnum1 <= argnum && cp->argnum2 <= argnum
     or it can be failed if the number of arguments is exceeded:
       cp->argtotal > 0 && cp->argtotal < argnum
   */
  for (i = 0; i < ap->nalternatives; i++)
    {
      struct partial_call *cp = &ap->alternative[i];

      if (!((cp->argnumc <= argnum
             && cp->argnum1 <= argnum
             && cp->argnum2 <= argnum)
            || (cp->argtotal > 0 && cp->argtotal < argnum)))
        /* cp is still undecided.  */
        return false;
    }
  return true;
}


void
arglist_parser_done (struct arglist_parser *ap, int argnum)
{
  size_t ncomplete;
  size_t i;

  /* Determine the number of complete calls.  */
  ncomplete = 0;
  for (i = 0; i < ap->nalternatives; i++)
    {
      struct partial_call *cp = &ap->alternative[i];

      if (cp->argnumc == 0 && cp->argnum1 == 0 && cp->argnum2 == 0
          && (cp->argtotal == 0 || cp->argtotal == argnum))
        ncomplete++;
    }

  if (ncomplete > 0)
    {
      struct partial_call *best_cp = NULL;
      bool ambiguous = false;

      /* Find complete calls where msgctxt, msgid, msgid_plural are all
         provided.  */
      for (i = 0; i < ap->nalternatives; i++)
        {
          struct partial_call *cp = &ap->alternative[i];

          if (cp->argnumc == 0 && cp->argnum1 == 0 && cp->argnum2 == 0
              && (cp->argtotal == 0 || cp->argtotal == argnum)
              && cp->msgctxt != NULL
              && cp->msgid != NULL
              && cp->msgid_plural != NULL)
            {
              if (best_cp != NULL)
                {
                  ambiguous = true;
                  break;
                }
              best_cp = cp;
            }
        }

      if (best_cp == NULL)
        {
          struct partial_call *best_cp1 = NULL;
          struct partial_call *best_cp2 = NULL;

          /* Find complete calls where msgctxt, msgid are provided.  */
          for (i = 0; i < ap->nalternatives; i++)
            {
              struct partial_call *cp = &ap->alternative[i];

              if (cp->argnumc == 0 && cp->argnum1 == 0 && cp->argnum2 == 0
                  && (cp->argtotal == 0 || cp->argtotal == argnum)
                  && cp->msgctxt != NULL
                  && cp->msgid != NULL)
                {
                  if (best_cp1 != NULL)
                    {
                      ambiguous = true;
                      break;
                    }
                  best_cp1 = cp;
                }
            }

          /* Find complete calls where msgid, msgid_plural are provided.  */
          for (i = 0; i < ap->nalternatives; i++)
            {
              struct partial_call *cp = &ap->alternative[i];

              if (cp->argnumc == 0 && cp->argnum1 == 0 && cp->argnum2 == 0
                  && (cp->argtotal == 0 || cp->argtotal == argnum)
                  && cp->msgid != NULL
                  && cp->msgid_plural != NULL)
                {
                  if (best_cp2 != NULL)
                    {
                      ambiguous = true;
                      break;
                    }
                  best_cp2 = cp;
                }
            }

          if (best_cp1 != NULL)
            best_cp = best_cp1;
          if (best_cp2 != NULL)
            {
              if (best_cp != NULL)
                ambiguous = true;
              else
                best_cp = best_cp2;
            }
        }

      if (best_cp == NULL)
        {
          /* Find complete calls where msgid is provided.  */
          for (i = 0; i < ap->nalternatives; i++)
            {
              struct partial_call *cp = &ap->alternative[i];

              if (cp->argnumc == 0 && cp->argnum1 == 0 && cp->argnum2 == 0
                  && (cp->argtotal == 0 || cp->argtotal == argnum)
                  && cp->msgid != NULL)
                {
                  if (best_cp != NULL)
                    {
                      ambiguous = true;
                      break;
                    }
                  best_cp = cp;
                }
            }
        }

      if (ambiguous)
        {
          error_with_progname = false;
          error_at_line (0, 0,
                         best_cp->msgid_pos.file_name,
                         best_cp->msgid_pos.line_number,
                         _("ambiguous argument specification for keyword '%.*s'"),
                         (int) ap->keyword_len, ap->keyword);
          error_with_progname = true;
        }

      if (best_cp != NULL)
        {
          /* best_cp indicates the best found complete call.
             Now call remember_a_message.  */
          message_ty *mp;

          /* Split strings in the GNOME glib syntax "msgctxt|msgid".  */
          if (best_cp->argnum1_glib_context || best_cp->argnum2_glib_context)
            /* split_keywordspec should not allow the context to be specified
               in two different ways.  */
            if (best_cp->msgctxt != NULL)
              abort ();
          if (best_cp->argnum1_glib_context)
            {
              const char *separator = strchr (best_cp->msgid, '|');

              if (separator == NULL)
                {
                  error_with_progname = false;
                  error_at_line (0, 0,
                                 best_cp->msgid_pos.file_name,
                                 best_cp->msgid_pos.line_number,
                                 _("warning: missing context for keyword '%.*s'"),
                                 (int) ap->keyword_len, ap->keyword);
                  error_with_progname = true;
                }
              else
                {
                  size_t ctxt_len = separator - best_cp->msgid;
                  char *ctxt = XNMALLOC (ctxt_len + 1, char);

                  memcpy (ctxt, best_cp->msgid, ctxt_len);
                  ctxt[ctxt_len] = '\0';
                  best_cp->msgctxt = ctxt;
                  best_cp->msgid = xstrdup (separator + 1);
                }
            }
          if (best_cp->msgid_plural != NULL && best_cp->argnum2_glib_context)
            {
              const char *separator = strchr (best_cp->msgid_plural, '|');

              if (separator == NULL)
                {
                  error_with_progname = false;
                  error_at_line (0, 0,
                                 best_cp->msgid_plural_pos.file_name,
                                 best_cp->msgid_plural_pos.line_number,
                                 _("warning: missing context for plural argument of keyword '%.*s'"),
                                 (int) ap->keyword_len, ap->keyword);
                  error_with_progname = true;
                }
              else
                {
                  size_t ctxt_len = separator - best_cp->msgid_plural;
                  char *ctxt = XNMALLOC (ctxt_len + 1, char);

                  memcpy (ctxt, best_cp->msgid_plural, ctxt_len);
                  ctxt[ctxt_len] = '\0';
                  if (best_cp->msgctxt == NULL)
                    best_cp->msgctxt = ctxt;
                  else
                    {
                      if (strcmp (ctxt, best_cp->msgctxt) != 0)
                        {
                          error_with_progname = false;
                          error_at_line (0, 0,
                                         best_cp->msgid_plural_pos.file_name,
                                         best_cp->msgid_plural_pos.line_number,
                                         _("context mismatch between singular and plural form"));
                          error_with_progname = true;
                        }
                      free (ctxt);
                    }
                  best_cp->msgid_plural = xstrdup (separator + 1);
                }
            }

          {
            flag_context_ty msgid_context = best_cp->msgid_context;
            flag_context_ty msgid_plural_context = best_cp->msgid_plural_context;
            struct literalstring_parser *parser = current_literalstring_parser;
            const char *encoding;

            /* Special support for the 3-argument tr operator in Qt:
               When --qt and --keyword=tr:1,1,2c,3t are specified, add to the
               context the information that the argument is expeected to be a
               qt-plural-format.  */
            if (recognize_format_qt
                && current_formatstring_parser3 == &formatstring_qt_plural
                && best_cp->msgid_plural == best_cp->msgid)
              {
                msgid_context.is_format3 = yes_according_to_context;
                msgid_plural_context.is_format3 = yes_according_to_context;
              }

            if (best_cp->msgctxt != NULL)
              {
                if (parser != NULL && best_cp->msgctxt_escape != 0)
                  {
                    char *msgctxt =
                      parser->parse (best_cp->msgctxt,
                                     &best_cp->msgctxt_pos,
                                     best_cp->msgctxt_escape);
                    free (best_cp->msgctxt);
                    best_cp->msgctxt = msgctxt;
                  }
                else
                  {
                    lex_pos_ty *pos = &best_cp->msgctxt_pos;
                    CONVERT_STRING (best_cp->msgctxt, lc_string);
                  }
              }

            if (parser != NULL && best_cp->msgid_escape != 0)
              {
                char *msgid = parser->parse (best_cp->msgid,
                                             &best_cp->msgid_pos,
                                             best_cp->msgid_escape);
                if (best_cp->msgid_plural == best_cp->msgid)
                  best_cp->msgid_plural = msgid;
                free (best_cp->msgid);
                best_cp->msgid = msgid;
              }
            else
              {
                lex_pos_ty *pos = &best_cp->msgid_pos;
                CONVERT_STRING (best_cp->msgid, lc_string);
              }

            if (best_cp->msgid_plural)
              {
                /* best_cp->msgid_plural may point to best_cp->msgid.
                   In that case, it is already interpreted and converted.  */
                if (best_cp->msgid_plural != best_cp->msgid)
                  {
                    if (parser != NULL
                        && best_cp->msgid_plural_escape != 0)
                      {
                        char *msgid_plural =
                          parser->parse (best_cp->msgid_plural,
                                         &best_cp->msgid_plural_pos,
                                         best_cp->msgid_plural_escape);
                        free (best_cp->msgid_plural);
                        best_cp->msgid_plural = msgid_plural;
                      }
                    else
                      {
                        lex_pos_ty *pos = &best_cp->msgid_plural_pos;
                        CONVERT_STRING (best_cp->msgid_plural, lc_string);
                      }
                  }

                /* If best_cp->msgid_plural equals to best_cp->msgid,
                   the ownership will be transferred to
                   remember_a_message before it is passed to
                   remember_a_message_plural.

                   Make a copy of the string in that case.  */
                if (best_cp->msgid_plural == best_cp->msgid)
                  best_cp->msgid_plural = xstrdup (best_cp->msgid);
              }

            if (best_cp->msgid_comment != NULL)
              {
                refcounted_string_list_ty *msgid_comment =
                  savable_comment_convert_encoding (best_cp->msgid_comment,
                                                    &best_cp->msgid_pos);
                drop_reference (best_cp->msgid_comment);
                best_cp->msgid_comment = msgid_comment;
              }

            /* best_cp->msgctxt, best_cp->msgid, and best_cp->msgid_plural
               are already in UTF-8.  Prevent further conversion in
               remember_a_message.  */
            encoding = xgettext_current_source_encoding;
            xgettext_current_source_encoding = po_charset_utf8;
            mp = remember_a_message (ap->mlp, best_cp->msgctxt, best_cp->msgid,
                                     msgid_context,
                                     &best_cp->msgid_pos,
                                     NULL, best_cp->msgid_comment);
            if (mp != NULL && best_cp->msgid_plural != NULL)
              remember_a_message_plural (mp,
                                         best_cp->msgid_plural,
                                         msgid_plural_context,
                                         &best_cp->msgid_plural_pos,
                                         NULL);
            xgettext_current_source_encoding = encoding;
          }

          if (best_cp->xcomments.nitems > 0)
            {
              /* Add best_cp->xcomments to mp->comment_dot, unless already
                 present.  */
              size_t i;

              for (i = 0; i < best_cp->xcomments.nitems; i++)
                {
                  const char *xcomment = best_cp->xcomments.item[i];
                  bool found = false;

                  if (mp != NULL && mp->comment_dot != NULL)
                    {
                      size_t j;

                      for (j = 0; j < mp->comment_dot->nitems; j++)
                        if (strcmp (xcomment, mp->comment_dot->item[j]) == 0)
                          {
                            found = true;
                            break;
                          }
                    }
                  if (!found)
                    message_comment_dot_append (mp, xcomment);
                }
            }
        }
    }
  else
    {
      /* No complete call was parsed.  */
      /* Note: There is a memory leak here: When there is more than one
         alternative, the same string can be stored in multiple alternatives,
         and it's not easy to free all strings reliably.  */
      if (ap->nalternatives == 1)
        {
          if (ap->alternative[0].msgctxt != NULL)
            free (ap->alternative[0].msgctxt);
          if (ap->alternative[0].msgid != NULL)
            free (ap->alternative[0].msgid);
          if (ap->alternative[0].msgid_plural != NULL)
            free (ap->alternative[0].msgid_plural);
        }
    }

  for (i = 0; i < ap->nalternatives; i++)
    drop_reference (ap->alternative[i].msgid_comment);
  free (ap);
}


struct mixed_string_buffer *
mixed_string_buffer_alloc (lexical_context_ty lcontext,
                           const char *logical_file_name,
                           int line_number)
{
  struct mixed_string_buffer *bp = XMALLOC (struct mixed_string_buffer);
  bp->utf8_buffer = NULL;
  bp->utf8_buflen = 0;
  bp->utf8_allocated = 0;
  bp->utf16_surr = 0;
  bp->curr_buffer = NULL;
  bp->curr_buflen = 0;
  bp->curr_allocated = 0;
  bp->lcontext = lcontext;
  bp->logical_file_name = logical_file_name;
  bp->line_number = line_number;
  return bp;
}

/* Auxiliary function: Append a byte to bp->curr.  */
static inline void
mixed_string_buffer_append_to_curr_buffer (struct mixed_string_buffer *bp,
                                           unsigned char c)
{
  if (bp->curr_buflen == bp->curr_allocated)
    {
      bp->curr_allocated = 2 * bp->curr_allocated + 10;
      bp->curr_buffer = xrealloc (bp->curr_buffer, bp->curr_allocated);
    }
  bp->curr_buffer[bp->curr_buflen++] = c;
}

/* Auxiliary function: Ensure count more bytes are available in bp->utf8.  */
static inline void
mixed_string_buffer_grow_utf8_buffer (struct mixed_string_buffer *bp,
                                         size_t count)
{
  if (bp->utf8_buflen + count > bp->utf8_allocated)
    {
      size_t new_allocated = 2 * bp->utf8_allocated + 10;
      if (new_allocated < bp->utf8_buflen + count)
        new_allocated = bp->utf8_buflen + count;
      bp->utf8_allocated = new_allocated;
      bp->utf8_buffer = xrealloc (bp->utf8_buffer, new_allocated);
    }
}

/* Auxiliary function: Append a Unicode character to bp->utf8.
   uc must be < 0x110000.  */
static inline void
mixed_string_buffer_append_to_utf8_buffer (struct mixed_string_buffer *bp,
                                           ucs4_t uc)
{
  unsigned char utf8buf[6];
  int count = u8_uctomb (utf8buf, uc, 6);

  if (count < 0)
    /* The caller should have ensured that uc is not out-of-range.  */
    abort ();

  mixed_string_buffer_grow_utf8_buffer (bp, count);
  memcpy (bp->utf8_buffer + bp->utf8_buflen, utf8buf, count);
  bp->utf8_buflen += count;
}

/* Auxiliary function: Flush bp->utf16_surr into bp->utf8_buffer.  */
static inline void
mixed_string_buffer_flush_utf16_surr (struct mixed_string_buffer *bp)
{
  if (bp->utf16_surr != 0)
    {
      /* A half surrogate is invalid, therefore use U+FFFD instead.  */
      mixed_string_buffer_append_to_utf8_buffer (bp, 0xfffd);
      bp->utf16_surr = 0;
    }
}

/* Auxiliary function: Flush bp->curr_buffer into bp->utf8_buffer.  */
static inline void
mixed_string_buffer_flush_curr_buffer (struct mixed_string_buffer *bp,
                                       int line_number)
{
  if (bp->curr_buflen > 0)
    {
      char *curr;
      size_t count;

      mixed_string_buffer_append_to_curr_buffer (bp, '\0');

      /* Convert from the source encoding to UTF-8.  */
      curr = from_current_source_encoding (bp->curr_buffer, bp->lcontext,
                                           bp->logical_file_name,
                                           line_number);

      /* Append it to bp->utf8_buffer.  */
      count = strlen (curr);
      mixed_string_buffer_grow_utf8_buffer (bp, count);
      memcpy (bp->utf8_buffer + bp->utf8_buflen, curr, count);
      bp->utf8_buflen += count;

      if (curr != bp->curr_buffer)
        free (curr);
      bp->curr_buflen = 0;
    }
}

void
mixed_string_buffer_append_char (struct mixed_string_buffer *bp, int c)
{
  /* Switch from Unicode character mode to multibyte character mode.  */
  mixed_string_buffer_flush_utf16_surr (bp);

  /* When a newline is seen, convert the accumulated multibyte sequence.
     This ensures a correct line number in the error message in case of
     a conversion error.  The "- 1" is to account for the newline.  */
  if (c == '\n')
    mixed_string_buffer_flush_curr_buffer (bp, bp->line_number - 1);

  mixed_string_buffer_append_to_curr_buffer (bp, (unsigned char) c);
}

void
mixed_string_buffer_append_unicode (struct mixed_string_buffer *bp, int c)
{
  /* Switch from multibyte character mode to Unicode character mode.  */
  mixed_string_buffer_flush_curr_buffer (bp, bp->line_number);

  /* Test whether this character and the previous one form a Unicode
     surrogate character pair.  */
  if (bp->utf16_surr != 0 && (c >= 0xdc00 && c < 0xe000))
    {
      unsigned short utf16buf[2];
      ucs4_t uc;

      utf16buf[0] = bp->utf16_surr;
      utf16buf[1] = c;
      if (u16_mbtouc (&uc, utf16buf, 2) != 2)
        abort ();

      mixed_string_buffer_append_to_utf8_buffer (bp, uc);
      bp->utf16_surr = 0;
    }
  else
    {
      mixed_string_buffer_flush_utf16_surr (bp);

      if (c >= 0xd800 && c < 0xdc00)
        bp->utf16_surr = c;
      else if (c >= 0xdc00 && c < 0xe000)
        {
          /* A half surrogate is invalid, therefore use U+FFFD instead.  */
          mixed_string_buffer_append_to_utf8_buffer (bp, 0xfffd);
        }
      else
        mixed_string_buffer_append_to_utf8_buffer (bp, c);
    }
}

char *
mixed_string_buffer_done (struct mixed_string_buffer *bp)
{
  char *utf8_buffer;

  /* Flush all into bp->utf8_buffer.  */
  mixed_string_buffer_flush_utf16_surr (bp);
  mixed_string_buffer_flush_curr_buffer (bp, bp->line_number);
  /* NUL-terminate it.  */
  mixed_string_buffer_grow_utf8_buffer (bp, 1);
  bp->utf8_buffer[bp->utf8_buflen] = '\0';

  /* Free curr_buffer and bp itself.  */
  utf8_buffer = bp->utf8_buffer;
  free (bp->curr_buffer);
  free (bp);

  /* Return it.  */
  return utf8_buffer;
}


static message_ty *
construct_header ()
{
  char *project_id_version;
  time_t now;
  char *timestring;
  message_ty *mp;
  char *msgstr;
  char *comment;
  static lex_pos_ty pos = { __FILE__, __LINE__ };

  if (package_name != NULL)
    {
      if (package_version != NULL)
        project_id_version = xasprintf ("%s %s", package_name, package_version);
      else
        project_id_version = xasprintf ("%s", package_name);
    }
  else
    project_id_version = xstrdup ("PACKAGE VERSION");

  if (msgid_bugs_address != NULL && msgid_bugs_address[0] == '\0')
    multiline_warning (xasprintf (_("warning: ")),
                       xstrdup (_("\
The option --msgid-bugs-address was not specified.\n\
If you are using a 'Makevars' file, please specify\n\
the MSGID_BUGS_ADDRESS variable there; otherwise please\n\
specify an --msgid-bugs-address command line option.\n\
")));

  time (&now);
  timestring = po_strftime (&now);

  msgstr = xasprintf ("\
Project-Id-Version: %s\n\
Report-Msgid-Bugs-To: %s\n\
POT-Creation-Date: %s\n\
PO-Revision-Date: YEAR-MO-DA HO:MI+ZONE\n\
Last-Translator: FULL NAME <EMAIL@ADDRESS>\n\
Language-Team: LANGUAGE <LL@li.org>\n\
Language: \n\
MIME-Version: 1.0\n\
Content-Type: text/plain; charset=CHARSET\n\
Content-Transfer-Encoding: 8bit\n",
                      project_id_version,
                      msgid_bugs_address != NULL ? msgid_bugs_address : "",
                      timestring);
  free (timestring);
  free (project_id_version);

  mp = message_alloc (NULL, "", NULL, msgstr, strlen (msgstr) + 1, &pos);

  if (copyright_holder[0] != '\0')
    comment = xasprintf ("\
SOME DESCRIPTIVE TITLE.\n\
Copyright (C) YEAR %s\n\
This file is distributed under the same license as the %s package.\n\
FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n",
                           copyright_holder,
                           package_name != NULL ? package_name : "PACKAGE");
  else
    comment = xstrdup ("\
SOME DESCRIPTIVE TITLE.\n\
This file is put in the public domain.\n\
FIRST AUTHOR <EMAIL@ADDRESS>, YEAR.\n");
  message_comment_append (mp, comment);
  free (comment);

  mp->is_fuzzy = true;

  return mp;
}

static void
finalize_header (msgdomain_list_ty *mdlp)
{
  /* If the generated PO file has plural forms, add a Plural-Forms template
     to the constructed header.  */
  {
    bool has_plural;
    size_t i, j;

    has_plural = false;
    for (i = 0; i < mdlp->nitems; i++)
      {
        message_list_ty *mlp = mdlp->item[i]->messages;

        for (j = 0; j < mlp->nitems; j++)
          {
            message_ty *mp = mlp->item[j];

            if (mp->msgid_plural != NULL)
              {
                has_plural = true;
                break;
              }
          }
        if (has_plural)
          break;
      }

    if (has_plural)
      {
        message_ty *header =
          message_list_search (mdlp->item[0]->messages, NULL, "");
        if (header != NULL
            && c_strstr (header->msgstr, "Plural-Forms:") == NULL)
          {
            size_t insertpos = strlen (header->msgstr);
            const char *suffix;
            size_t suffix_len;
            char *new_msgstr;

            suffix = "\nPlural-Forms: nplurals=INTEGER; plural=EXPRESSION;\n";
            if (insertpos == 0 || header->msgstr[insertpos-1] == '\n')
              suffix++;
            suffix_len = strlen (suffix);
            new_msgstr = XNMALLOC (header->msgstr_len + suffix_len, char);
            memcpy (new_msgstr, header->msgstr, insertpos);
            memcpy (new_msgstr + insertpos, suffix, suffix_len);
            memcpy (new_msgstr + insertpos + suffix_len,
                    header->msgstr + insertpos,
                    header->msgstr_len - insertpos);
            header->msgstr = new_msgstr;
            header->msgstr_len = header->msgstr_len + suffix_len;
          }
      }
  }

  /* If not all the strings were plain ASCII, or if the output syntax
     requires a charset conversion, set the charset in the header to UTF-8.
     All messages have already been converted to UTF-8 in remember_a_message
     and remember_a_message_plural.  */
  {
    bool has_nonascii = false;
    size_t i;

    for (i = 0; i < mdlp->nitems; i++)
      {
        message_list_ty *mlp = mdlp->item[i]->messages;

        if (!is_ascii_message_list (mlp))
          has_nonascii = true;
      }

    if (has_nonascii || output_syntax->requires_utf8)
      {
        message_list_ty *mlp = mdlp->item[0]->messages;

        iconv_message_list (mlp, po_charset_utf8, po_charset_utf8, NULL);
      }
  }
}


static extractor_ty
language_to_extractor (const char *name)
{
  struct table_ty
  {
    const char *name;
    extractor_func func;
    flag_context_list_table_ty *flag_table;
    struct formatstring_parser *formatstring_parser1;
    struct formatstring_parser *formatstring_parser2;
    struct literalstring_parser *literalstring_parser;
  };
  typedef struct table_ty table_ty;

  static table_ty table[] =
  {
    SCANNERS_C
    SCANNERS_PO
    SCANNERS_SH
    SCANNERS_PYTHON
    SCANNERS_LISP
    SCANNERS_ELISP
    SCANNERS_LIBREP
    SCANNERS_SCHEME
    SCANNERS_SMALLTALK
    SCANNERS_JAVA
    SCANNERS_PROPERTIES
    SCANNERS_CSHARP
    SCANNERS_AWK
    SCANNERS_YCP
    SCANNERS_TCL
    SCANNERS_PERL
    SCANNERS_PHP
    SCANNERS_STRINGTABLE
    SCANNERS_RST
    SCANNERS_GLADE
    SCANNERS_LUA
    SCANNERS_JAVASCRIPT
    SCANNERS_VALA
    SCANNERS_GSETTINGS
    SCANNERS_DESKTOP
    SCANNERS_APPDATA
    /* Here may follow more languages and their scanners: pike, etc...
       Make sure new scanners honor the --exclude-file option.  */
  };

  table_ty *tp;

  for (tp = table; tp < ENDOF(table); ++tp)
    if (c_strcasecmp (name, tp->name) == 0)
      {
        extractor_ty result;

        result.func = tp->func;
        result.flag_table = tp->flag_table;
        result.formatstring_parser1 = tp->formatstring_parser1;
        result.formatstring_parser2 = tp->formatstring_parser2;
        result.formatstring_parser3 = NULL;
        result.literalstring_parser = tp->literalstring_parser;

        /* Handle --qt.  It's preferrable to handle this facility here rather
           than through an option --language=C++/Qt because the latter would
           conflict with the language "C++" regarding the file extensions.  */
        if (recognize_format_qt && strcmp (tp->name, "C++") == 0)
          {
            result.flag_table = &flag_table_cxx_qt;
            result.formatstring_parser2 = &formatstring_qt;
            result.formatstring_parser3 = &formatstring_qt_plural;
          }
        /* Likewise for --kde.  */
        if (recognize_format_kde && strcmp (tp->name, "C++") == 0)
          {
            result.flag_table = &flag_table_cxx_kde;
            result.formatstring_parser2 = &formatstring_kde;
            result.formatstring_parser3 = &formatstring_kde_kuit;
          }
        /* Likewise for --boost.  */
        if (recognize_format_boost && strcmp (tp->name, "C++") == 0)
          {
            result.flag_table = &flag_table_cxx_boost;
            result.formatstring_parser2 = &formatstring_boost;
          }

        return result;
      }

  error (EXIT_FAILURE, 0, _("language '%s' unknown"), name);
  /* NOTREACHED */
  {
    extractor_ty result = { NULL, NULL, NULL, NULL };
    return result;
  }
}


static const char *
extension_to_language (const char *extension)
{
  struct table_ty
  {
    const char *extension;
    const char *language;
  };
  typedef struct table_ty table_ty;

  static table_ty table[] =
  {
    EXTENSIONS_C
    EXTENSIONS_PO
    EXTENSIONS_SH
    EXTENSIONS_PYTHON
    EXTENSIONS_LISP
    EXTENSIONS_ELISP
    EXTENSIONS_LIBREP
    EXTENSIONS_SCHEME
    EXTENSIONS_SMALLTALK
    EXTENSIONS_JAVA
    EXTENSIONS_PROPERTIES
    EXTENSIONS_CSHARP
    EXTENSIONS_AWK
    EXTENSIONS_YCP
    EXTENSIONS_TCL
    EXTENSIONS_PERL
    EXTENSIONS_PHP
    EXTENSIONS_STRINGTABLE
    EXTENSIONS_RST
    EXTENSIONS_GLADE
    EXTENSIONS_LUA
    EXTENSIONS_JAVASCRIPT
    EXTENSIONS_VALA
    EXTENSIONS_GSETTINGS
    EXTENSIONS_DESKTOP
    EXTENSIONS_APPDATA
    /* Here may follow more file extensions... */
  };

  table_ty *tp;

  for (tp = table; tp < ENDOF(table); ++tp)
    if (strcmp (extension, tp->extension) == 0)
      return tp->language;
  return NULL;
}
