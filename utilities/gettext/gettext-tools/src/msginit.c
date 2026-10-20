/* Initializes a new PO file.
   Copyright (C) 2001-2026 Free Software Foundation, Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible.  */


#include <config.h>
#include <alloca.h>

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <locale.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#if HAVE_PWD_H
# include <pwd.h>
#endif
#ifdef _OPENMP
# include <omp.h>
#endif

#include <textstyle.h>

#include <error.h>
#include "options.h"
#include "noreturn.h"
#include "closeout.h"
#include "error-progname.h"
#include "progname.h"
#include "relocatable.h"
#include "basename-lgpl.h"
#include "c-strstr.h"
#include "c-strcase.h"
#include "message.h"
#include "msgl-merge.h"
#include "read-catalog-file.h"
#include "read-po.h"
#include "read-properties.h"
#include "read-stringtable.h"
#include "write-catalog.h"
#include "write-po.h"
#include "write-properties.h"
#include "write-stringtable.h"
#include "msgl-charset.h"
#include "xerror-handler.h"
#include "po-charset.h"
#include "localcharset.h"
#include "localename.h"
#include "po-time.h"
#include "plural-table.h"
#include "lang-table.h"
#include "xalloc.h"
#include "xmalloca.h"
#include "concat-filename.h"
#include "xerror.h"
#include "xvasprintf.h"
#include "msgl-english.h"
#include "plural-count.h"
#include "spawn-pipe.h"
#include "wait-process.h"
#include "backupfile.h"
#include "copy-file.h"
#include "xsetenv.h"
#include "xstriconv.h"
#include "str-list.h"
#include "propername.h"
#include "gettext.h"

#define _(str) gettext (str)
#define N_(str) (str)

/* Get F_OK.  It is lacking from <fcntl.h> on Woe32.  */
#ifndef F_OK
# define F_OK 0
#endif

#define SIZEOF(a) (sizeof(a) / sizeof(a[0]))

extern const char * _nl_expand_alias (const char *name);

/* Locale name.  */
static const char *locale;

/* Language (ISO-639 code).  */
static const char *language;

/* If true, the user is not considered to be the translator.  */
static bool no_translator;

/* Forward declaration of local functions.  */
_GL_NORETURN_FUNC static void usage (int status);
static const char *find_pot (void);
static const char *catalogname_for_locale (const char *locale);
static const char *language_of_locale (const char *locale);
static char *get_field (const char *header, const char *field);
static msgdomain_list_ty *fill_header (msgdomain_list_ty *mdlp, bool fresh);
static msgdomain_list_ty *update_msgstr_plurals (msgdomain_list_ty *mdlp);


int
main (int argc, char **argv)
{
  /* Set program name for messages.  */
  set_program_name (argv[0]);
  error_print_progname = maybe_print_progname;
  gram_max_allowed_errors = 20;

  /* Set locale via LC_ALL.  */
  setlocale (LC_ALL, "");

  /* Set the text message domain.  */
  bindtextdomain (PACKAGE, relocate (LOCALEDIR));
  bindtextdomain ("gnulib", relocate (GNULIB_LOCALEDIR));
  bindtextdomain ("bison-runtime", relocate (BISON_LOCALEDIR));
  textdomain (PACKAGE);

  /* Ensure that write errors on stdout are detected.  */
  atexit (close_stdout);

  /* Default values for command line options.  */
  bool do_help = false;
  bool do_version = false;
  char *output_file = NULL;
  const char *input_file = NULL;
  catalog_input_format_ty input_syntax = &input_format_po;
  catalog_input_format_ty output_file_input_syntax = &input_format_po;
  catalog_output_format_ty output_syntax = &output_format_po;
  locale = NULL;

  /* Parse command line options.  */
  BEGIN_ALLOW_OMITTING_FIELD_INITIALIZERS
  static const struct program_option options[] =
  {
    { "color",              CHAR_MAX + 5, optional_argument },
    { "help",               'h',          no_argument       },
    { "input",              'i',          required_argument },
    { "locale",             'l',          required_argument },
    { "no-translator",      CHAR_MAX + 1, no_argument       },
    { "no-wrap",            CHAR_MAX + 2, no_argument       },
    { "output-file",        'o',          required_argument },
    { "properties-input",   'P',          no_argument       },
    { "properties-output",  'p',          no_argument       },
    { "stringtable-input",  CHAR_MAX + 3, no_argument       },
    { "stringtable-output", CHAR_MAX + 4, no_argument       },
    { "style",              CHAR_MAX + 6, required_argument },
    { "version",            'V',          no_argument       },
    { "width",              'w',          required_argument },
  };
  END_ALLOW_OMITTING_FIELD_INITIALIZERS
  start_options (argc, argv, options, MOVE_OPTIONS_FIRST, 0);
  {
    int opt;
    while ((opt = get_next_option ()) != -1)
      switch (opt)
        {
        case '\0':                /* Long option with key == 0.  */
          break;

        case 'h':
          do_help = true;
          break;

        case 'i':
          if (input_file != NULL)
            {
              error (EXIT_SUCCESS, 0, _("at most one input file allowed"));
              usage (EXIT_FAILURE);
            }
          input_file = optarg;
          break;

        case 'l':
          locale = optarg;
          break;

        case 'o':
          output_file = optarg;
          break;

        case 'p':
          output_file_input_syntax = &input_format_properties;
          output_syntax = &output_format_properties;
          break;

        case 'P':
          input_syntax = &input_format_properties;
          break;

        case 'V':
          do_version = true;
          break;

        case 'w':
          {
            char *endp;
            int value = strtol (optarg, &endp, 10);
            if (endp != optarg)
              message_page_width_set (value);
          }
          break;

        case CHAR_MAX + 1:
          no_translator = true;
          break;

        case CHAR_MAX + 2: /* --no-wrap */
          message_page_width_ignore ();
          break;

        case CHAR_MAX + 3: /* --stringtable-input */
          input_syntax = &input_format_stringtable;
          break;

        case CHAR_MAX + 4: /* --stringtable-output */
          output_file_input_syntax = &input_format_stringtable;
          output_syntax = &output_format_stringtable;
          break;

        case CHAR_MAX + 5: /* --color */
          if (handle_color_option (optarg) || color_test_mode)
            usage (EXIT_FAILURE);
          break;

        case CHAR_MAX + 6: /* --style */
          handle_style_option (optarg);
          break;

        default:
          usage (EXIT_FAILURE);
          break;
        }
  }

  /* Version information is requested.  */
  if (do_version)
    {
      printf ("%s (GNU %s) %s\n", last_component (program_name),
              PACKAGE, VERSION);
      /* xgettext: no-wrap */
      printf (_("Copyright (C) %s Free Software Foundation, Inc.\n\
License GPLv3+: GNU GPL version 3 or later <%s>\n\
This is free software: you are free to change and redistribute it.\n\
There is NO WARRANTY, to the extent permitted by law.\n\
"),
              "2001-2026", "https://gnu.org/licenses/gpl.html");
      printf (_("Written by %s.\n"), proper_name ("Bruno Haible"));
      exit (EXIT_SUCCESS);
    }

  /* Help is requested.  */
  if (do_help)
    usage (EXIT_SUCCESS);

  /* Test for extraneous arguments.  */
  if (optind != argc)
    error (EXIT_FAILURE, 0, _("too many arguments"));

  /* Search for the input file.  */
  if (input_file == NULL)
    input_file = find_pot ();

  /* Determine target locale.  */
  if (locale == NULL)
    {
      locale = gl_locale_name (LC_MESSAGES, "LC_MESSAGES");
      if (strcmp (locale, "C") == 0)
        {
          const char *doc_url =
            "https://www.gnu.org/software/gettext/manual/html_node/Setting-the-POSIX-Locale.html";
          multiline_error (xstrdup (""),
                           xasprintf (_("\
You are in a language indifferent environment.  Please set\n\
your LANG environment variable, as described in\n\
<%s>.\n\
This is necessary so you can test your translations.\n"),
                                      doc_url));
          exit (EXIT_FAILURE);
        }
    }
  {
    const char *alias = _nl_expand_alias (locale);
    if (alias != NULL)
      locale = alias;
  }
  catalogname = catalogname_for_locale (locale);
  language = language_of_locale (locale);

  /* Default output file name is CATALOGNAME.po.  */
  if (output_file == NULL)
    output_file = xasprintf ("%s.po", catalogname);

  msgdomain_list_ty *result;
  if (strcmp (output_file, "-") != 0
      && access (output_file, F_OK) == 0)
    {
      /* The output PO file already exists.  Assume the translator wants to
         continue, based on these translations.  */

      /* First, create a backup file.  */
      if (!no_translator)
        {
          {
            const char *backup_suffix_string = getenv ("SIMPLE_BACKUP_SUFFIX");
            if (backup_suffix_string != NULL && backup_suffix_string[0] != '\0')
              simple_backup_suffix = backup_suffix_string;
          }
          {
            char *backup_file = find_backup_file_name (output_file, simple);
            xcopy_file_preserving (output_file, backup_file);
          }
        }

      /* Initialize OpenMP.  */
      #ifdef _OPENMP
      openmp_init ();
      #endif

      /* Read both files and merge them.  */
      quiet = true;
      keep_previous = true;
      msgdomain_list_ty *def;
      result = merge (output_file, output_file_input_syntax,
                      input_file, input_syntax,
                      &def);

      /* Update the header entry.  */
      result = fill_header (result, false);
    }
  else
    {
      /* Read input file.  */
      result = read_catalog_file (input_file, input_syntax);
      check_pot_charset (result, input_file);

      /* Fill the header entry.  */
      result = fill_header (result, true);

      /* Initialize translations.  */
      if (strcmp (language, "en") == 0)
        result = msgdomain_list_english (result);
      else
        result = update_msgstr_plurals (result);
    }

  /* Write the modified message list out.  */
  msgdomain_list_print (result, output_file, output_syntax,
                        textmode_xerror_handler, true, false);

  if (!no_translator)
    fprintf (stderr, "\n");
  fprintf (stderr, _("Created %s.\n"), output_file);

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
Usage: %s [OPTION]\n\
"), program_name);
      printf ("\n");
      /* xgettext: no-wrap */
      printf (_("\
Creates a new PO file, initializing the meta information with values from the\n\
user's environment.\n\
"));
      printf ("\n");
      printf (_("\
Mandatory arguments to long options are mandatory for short options too.\n"));
      printf ("\n");
      printf (_("\
Input file location:\n"));
      printf (_("\
  -i, --input=INPUTFILE       input POT file\n"));
      printf (_("\
If no input file is given, the current directory is searched for the POT file.\n\
If it is -, standard input is read.\n"));
      printf ("\n");
      printf (_("\
Output file location:\n"));
      printf (_("\
  -o, --output-file=FILE      write output to specified PO file\n"));
      printf (_("\
If no output file is given, it depends on the --locale option or the user's\n\
locale setting.\n\
If the output file already exists, it is merged with the input file,\n\
as if through '%s'.\n\
If it is -, the results are written to standard output.\n"),
              "msgmerge");
      printf ("\n");
      printf (_("\
Input file syntax:\n"));
      printf (_("\
  -P, --properties-input      input file is in Java .properties syntax\n"));
      printf (_("\
      --stringtable-input     input file is in NeXTstep/GNUstep .strings syntax\n"));
      printf ("\n");
      printf (_("\
Output details:\n"));
      printf (_("\
  -l, --locale=LL_CC[.ENCODING]  set target locale\n"));
      printf (_("\
      --no-translator         assume the PO file is automatically generated\n"));
      printf (_("\
      --color                 use colors and other text attributes always\n\
      --color=WHEN            use colors and other text attributes if WHEN.\n\
                              WHEN may be 'always', 'never', 'auto', or 'html'.\n"));
      printf (_("\
      --style=STYLEFILE       specify CSS style rule file for --color\n"));
      printf (_("\
  -p, --properties-output     write out a Java .properties file\n"));
      printf (_("\
      --stringtable-output    write out a NeXTstep/GNUstep .strings file\n"));
      printf (_("\
  -w, --width=NUMBER          set output page width\n"));
      printf (_("\
      --no-wrap               do not break long message lines, longer than\n\
                              the output page width, into several lines\n"));
      printf ("\n");
      printf (_("\
Informative output:\n"));
      printf (_("\
  -h, --help                  display this help and exit\n"));
      printf (_("\
  -V, --version               output version information and exit\n"));
      printf ("\n");
      /* TRANSLATORS: The first placeholder is the web address of the Savannah
         project of this package.  The second placeholder is the bug-reporting
         email address for this package.  Please add _another line_ saying
         "Report translation bugs to <...>\n" with the address for translation
         bugs (typically your translation team's web or email address).  */
      printf (_("\
Report bugs in the bug tracker at <%s>\n\
or by email to <%s>.\n"),
             "https://savannah.gnu.org/projects/gettext",
             "bug-gettext@gnu.org");
    }

  exit (status);
}


/* Search for the POT file and return its name.  */
static const char *
find_pot ()
{
  DIR *dirp = opendir (".");
  if (dirp != NULL)
    {
      char *found = NULL;

      for (;;)
        {
          errno = 0;
          struct dirent *dp = readdir (dirp);
          if (dp != NULL)
            {
              const char *name = dp->d_name;
              size_t namlen = strlen (name);

              if (namlen > 4 && memcmp (name + namlen - 4, ".pot", 4) == 0)
                {
                  if (found == NULL)
                    found = xstrdup (name);
                  else
                    {
                      multiline_error (xstrdup (""),
                                       xstrdup (_("\
Found more than one .pot file.\n\
Please specify the input .pot file through the --input option.\n")));
                      usage (EXIT_FAILURE);
                    }
                }
            }
          else if (errno != 0)
            error (EXIT_FAILURE, errno, _("error reading current directory"));
          else
            break;
        }
      if (closedir (dirp))
        error (EXIT_FAILURE, errno, _("error reading current directory"));

      if (found != NULL)
        return found;
    }

  multiline_error (xstrdup (""),
                   xstrdup (_("\
Found no .pot file in the current directory.\n\
Please specify the input .pot file through the --input option.\n")));
  usage (EXIT_FAILURE);
  /* NOTREACHED */
  return NULL;
}


/* Return the gettext catalog name corresponding to a locale.  If the locale
   consists of a language and a territory, and the language is mainly spoken
   in that territory, the territory is removed from the locale name.
   For example, "de_DE" or "de_DE.ISO-8859-1" are simplified to "de",
   because the resulting catalog can be used as a default for all "de_XX",
   such as "de_AT".  */
static const char *
catalogname_for_locale (const char *locale)
{
  static const char *locales_with_principal_territory[] = {
                /* Language     Main territory */
    "ace_ID",   /* Achinese     Indonesia */
    "af_ZA",    /* Afrikaans    South Africa */
    "ak_GH",    /* Akan         Ghana */
    "am_ET",    /* Amharic      Ethiopia */
    "an_ES",    /* Aragonese    Spain */
    "ang_GB",   /* Old English  Britain */
    "arn_CL",   /* Mapudungun   Chile */
    "as_IN",    /* Assamese     India */
    "ast_ES",   /* Asturian     Spain */
    "av_RU",    /* Avaric       Russia */
    "awa_IN",   /* Awadhi       India */
    "az_AZ",    /* Azerbaijani  Azerbaijan */
    "ban_ID",   /* Balinese     Indonesia */
    "be_BY",    /* Belarusian   Belarus */
    "bej_SD",   /* Beja         Sudan */
    "bem_ZM",   /* Bemba        Zambia */
    "bg_BG",    /* Bulgarian    Bulgaria */
    "bho_IN",   /* Bhojpuri     India */
    "bi_VU",    /* Bislama      Vanuatu */
    "bik_PH",   /* Bikol        Philippines */
    "bin_NG",   /* Bini         Nigeria */
    "bm_ML",    /* Bambara      Mali */
    "bn_IN",    /* Bengali      India */
    "bo_CN",    /* Tibetan      China */
    "br_FR",    /* Breton       France */
    "bs_BA",    /* Bosnian      Bosnia */
    "bug_ID",   /* Buginese     Indonesia */
    "ca_ES",    /* Catalan      Spain */
    "ce_RU",    /* Chechen      Russia */
    "ceb_PH",   /* Cebuano      Philippines */
    "co_FR",    /* Corsican     France */
    "cr_CA",    /* Cree         Canada */
    /* Don't put "crh_UZ" or "crh_UA" here.  That would be asking for fruitless
       political discussion.  */
    "cs_CZ",    /* Czech        Czechia */
    "csb_PL",   /* Kashubian    Poland */
    "cy_GB",    /* Welsh        Britain */
    "da_DK",    /* Danish       Denmark */
    "de_DE",    /* German       Germany */
    "din_SD",   /* Dinka        Sudan */
    "doi_IN",   /* Dogri        India */
    "dsb_DE",   /* Lower Sorbian        Germany */
    "dv_MV",    /* Divehi       Maldives */
    "dz_BT",    /* Dzongkha     Bhutan */
    "ee_GH",    /* Éwé          Ghana */
    "el_GR",    /* Greek        Greece */
    /* Don't put "en_GB" or "en_US" here.  That would be asking for fruitless
       political discussion.  */
    "es_ES",    /* Spanish      Spain */
    "et_EE",    /* Estonian     Estonia */
    "fa_IR",    /* Persian      Iran */
    "fi_FI",    /* Finnish      Finland */
    "fil_PH",   /* Filipino     Philippines */
    "fj_FJ",    /* Fijian       Fiji */
    "fo_FO",    /* Faroese      Faeroe Islands */
    "fon_BJ",   /* Fon          Benin */
    "fr_FR",    /* French       France */
    "fur_IT",   /* Friulian     Italy */
    "fy_NL",    /* Western Frisian      Netherlands */
    "ga_IE",    /* Irish        Ireland */
    "gd_GB",    /* Scottish Gaelic      Britain */
    "gl_ES",    /* Galician     Spain */
    "gon_IN",   /* Gondi        India */
    "gsw_CH",   /* Swiss German Switzerland */
    "gu_IN",    /* Gujarati     India */
    "he_IL",    /* Hebrew       Israel */
    "hi_IN",    /* Hindi        India */
    "hil_PH",   /* Hiligaynon   Philippines */
    "hr_HR",    /* Croatian     Croatia */
    "hsb_DE",   /* Upper Sorbian        Germany */
    "ht_HT",    /* Haitian      Haiti */
    "hu_HU",    /* Hungarian    Hungary */
    "hy_AM",    /* Armenian     Armenia */
    "id_ID",    /* Indonesian   Indonesia */
    "ig_NG",    /* Igbo         Nigeria */
    "ii_CN",    /* Sichuan Yi   China */
    "ilo_PH",   /* Iloko        Philippines */
    "is_IS",    /* Icelandic    Iceland */
    "it_IT",    /* Italian      Italy */
    "ja_JP",    /* Japanese     Japan */
    "jab_NG",   /* Hyam         Nigeria */
    "jv_ID",    /* Javanese     Indonesia */
    "ka_GE",    /* Georgian     Georgia */
    "kab_DZ",   /* Kabyle       Algeria */
    "kaj_NG",   /* Jju          Nigeria */
    "kam_KE",   /* Kamba        Kenya */
    "kmb_AO",   /* Kimbundu     Angola */
    "kcg_NG",   /* Tyap         Nigeria */
    "kdm_NG",   /* Kagoma       Nigeria */
    "kg_CD",    /* Kongo        Democratic Republic of Congo */
    "kk_KZ",    /* Kazakh       Kazakhstan */
    "kl_GL",    /* Kalaallisut  Greenland */
    "km_KH",    /* Central Khmer        Cambodia */
    "kn_IN",    /* Kannada      India */
    "ko_KR",    /* Korean       Korea (South) */
    "kok_IN",   /* Konkani      India */
    "kr_NG",    /* Kanuri       Nigeria */
    "kru_IN",   /* Kurukh       India */
    "ky_KG",    /* Kyrgyz       Kyrgyzstan */
    "lg_UG",    /* Ganda        Uganda */
    "li_BE",    /* Limburgish   Belgium */
    "lo_LA",    /* Laotian      Laos */
    "lt_LT",    /* Lithuanian   Lithuania */
    "lu_CD",    /* Luba-Katanga Democratic Republic of Congo */
    "lua_CD",   /* Luba-Lulua   Democratic Republic of Congo */
    "luo_KE",   /* Luo          Kenya */
    "lv_LV",    /* Latvian      Latvia */
    "mad_ID",   /* Madurese     Indonesia */
    "mag_IN",   /* Magahi       India */
    "mai_IN",   /* Maithili     India */
    "mak_ID",   /* Makasar      Indonesia */
    "man_ML",   /* Mandingo     Mali */
    "men_SL",   /* Mende        Sierra Leone */
    "mfe_MU",   /* Mauritian Creole     Mauritius */
    "mg_MG",    /* Malagasy     Madagascar */
    "mi_NZ",    /* Maori        New Zealand */
    "min_ID",   /* Minangkabau  Indonesia */
    "mk_MK",    /* Macedonian   North Macedonia */
    "ml_IN",    /* Malayalam    India */
    "mn_MN",    /* Mongolian    Mongolia */
    "mni_IN",   /* Manipuri     India */
    "mos_BF",   /* Mossi        Burkina Faso */
    "mr_IN",    /* Marathi      India */
    "ms_MY",    /* Malay        Malaysia */
    "mt_MT",    /* Maltese      Malta */
    "mwr_IN",   /* Marwari      India */
    "my_MM",    /* Burmese      Myanmar */
    "na_NR",    /* Nauru        Nauru */
    "nah_MX",   /* Nahuatl      Mexico */
    "nap_IT",   /* Neapolitan   Italy */
    "nb_NO",    /* Norwegian Bokmål    Norway */
    "nds_DE",   /* Low Saxon    Germany */
    "ne_NP",    /* Nepali       Nepal */
    "nl_NL",    /* Dutch        Netherlands */
    "nn_NO",    /* Norwegian Nynorsk    Norway */
    "no_NO",    /* Norwegian    Norway */
    "nr_ZA",    /* South Ndebele        South Africa */
    "nso_ZA",   /* Northern Sotho       South Africa */
    "ny_MW",    /* Chichewa     Malawi */
    "nym_TZ",   /* Nyamwezi     Tanzania */
    "nyn_UG",   /* Nyankole     Uganda */
    "oc_FR",    /* Occitan      France */
    "oj_CA",    /* Ojibwa       Canada */
    "or_IN",    /* Oriya        India */
    "pa_IN",    /* Punjabi      India */
    "pag_PH",   /* Pangasinan   Philippines */
    "pam_PH",   /* Pampanga     Philippines */
    "pap_AN",   /* Papiamento   Netherlands Antilles - this line can be removed in 2018 */
    "pbb_CO",   /* Páez                Colombia */
    "pl_PL",    /* Polish       Poland */
    "ps_AF",    /* Pashto       Afghanistan */
    "pt_PT",    /* Portuguese   Portugal */
    "raj_IN",   /* Rajasthani   India */
    "rm_CH",    /* Romansh      Switzerland */
    "rn_BI",    /* Kirundi      Burundi */
    "ro_RO",    /* Romanian     Romania */
    "ru_RU",    /* Russian      Russia */
    "rw_RW",    /* Kinyarwanda  Rwanda */
    "sa_IN",    /* Sanskrit     India */
    "sah_RU",   /* Yakut        Russia */
    "sas_ID",   /* Sasak        Indonesia */
    "sat_IN",   /* Santali      India */
    "sc_IT",    /* Sardinian    Italy */
    "scn_IT",   /* Sicilian     Italy */
    "sg_CF",    /* Sango        Central African Republic */
    "shn_MM",   /* Shan         Myanmar */
    "si_LK",    /* Sinhala      Sri Lanka */
    "sid_ET",   /* Sidamo       Ethiopia */
    "sk_SK",    /* Slovak       Slovakia */
    "sl_SI",    /* Slovenian    Slovenia */
    "smn_FI",   /* Inari Sami   Finland */
    "sms_FI",   /* Skolt Sami   Finland */
    "so_SO",    /* Somali       Somalia */
    "sq_AL",    /* Albanian     Albania */
    "sr_RS",    /* Serbian      Serbia */
    "srr_SN",   /* Serer        Senegal */
    "suk_TZ",   /* Sukuma       Tanzania */
    "sus_GN",   /* Susu         Guinea */
    "sv_SE",    /* Swedish      Sweden */
    "ta_IN",    /* Tamil        India */
    "te_IN",    /* Telugu       India */
    "tem_SL",   /* Timne        Sierra Leone */
    "tet_ID",   /* Tetum        Indonesia */
    "tg_TJ",    /* Tajik        Tajikistan */
    "th_TH",    /* Thai         Thailand */
    "tiv_NG",   /* Tiv          Nigeria */
    "tk_TM",    /* Turkmen      Turkmenistan */
    "tl_PH",    /* Tagalog      Philippines */
    "to_TO",    /* Tonga        Tonga */
    "tpi_PG",   /* Tok Pisin    Papua New Guinea */
    "tr_TR",    /* Turkish      Türkiye */
    "tum_MW",   /* Tumbuka      Malawi */
    "ug_CN",    /* Uighur       China */
    "uk_UA",    /* Ukrainian    Ukraine */
    "umb_AO",   /* Umbundu      Angola */
    "ur_PK",    /* Urdu         Pakistan */
    "uz_UZ",    /* Uzbek        Uzbekistan */
    "ve_ZA",    /* Venda        South Africa */
    "vi_VN",    /* Vietnamese   Vietnam */
    "wa_BE",    /* Walloon      Belgium */
    "wal_ET",   /* Walamo       Ethiopia */
    "war_PH",   /* Waray        Philippines */
    "wen_DE",   /* Sorbian      Germany */
    "yao_MW",   /* Yao          Malawi */
    "zap_MX"    /* Zapotec      Mexico */
  };

  /* Remove the ".codeset" part from the locale.  */
  {
    const char *dot = strchr (locale, '.');
    if (dot != NULL)
      {
        const char *codeset_end = strpbrk (dot + 1, "_@");
        if (codeset_end == NULL)
          codeset_end = dot + strlen (dot);

        char *shorter_locale = XNMALLOC (strlen (locale), char);
        memcpy (shorter_locale, locale, dot - locale);
        strcpy (shorter_locale + (dot - locale), codeset_end);
        locale = shorter_locale;
      }
  }

  /* If the territory is the language's principal territory, drop it.  */
  for (size_t i = 0; i < SIZEOF (locales_with_principal_territory); i++)
    if (strcmp (locale, locales_with_principal_territory[i]) == 0)
      {
        const char *language_end = strchr (locale, '_');
        if (language_end == NULL)
          abort ();

        size_t len = language_end - locale;
        char *shorter_locale = XNMALLOC (len + 1, char);
        memcpy (shorter_locale, locale, len);
        shorter_locale[len] = '\0';
        locale = shorter_locale;
        break;
      }

  return locale;
}


/* Return the language of a locale.  */
static const char *
language_of_locale (const char *locale)
{
  const char *language_end = strpbrk (locale, "_.@");
  if (language_end != NULL)
    {
      size_t len = language_end - locale;
      char *result = XNMALLOC (len + 1, char);
      memcpy (result, locale, len);
      result[len] = '\0';

      return result;
    }
  else
    return locale;
}


/* ---------------------- fill_header and subroutines ---------------------- */

/* Return the most likely desired charset for the PO file, as a portable
   charset name.  */
static const char *
canonical_locale_charset ()
{
  /* Save LC_ALL environment variable.  */
  char *old_LC_ALL;
  {
    const char *tmp = getenv ("LC_ALL");
    old_LC_ALL = (tmp != NULL ? xstrdup (tmp) : NULL);
  }

  xsetenv ("LC_ALL", locale, 1);

  const char *charset;
  if (setlocale (LC_ALL, "") == NULL)
    /* Nonexistent locale.  Use anything.  */
    charset = "";
  else
    /* Get the locale's charset.  */
    charset = locale_charset ();

  /* Restore LC_ALL environment variable.  */

  if (old_LC_ALL != NULL)
    xsetenv ("LC_ALL", old_LC_ALL, 1), free (old_LC_ALL);
  else
    unsetenv ("LC_ALL");

  setlocale (LC_ALL, "");

  /* Canonicalize it.  */
  charset = po_charset_canonicalize (charset);
  if (charset == NULL)
    charset = po_charset_ascii;

  return charset;
}


/* The desired charset for the PO file.
   Determined at the beginning of fill_header().  */
static const char *output_charset;


/* Return the English name of the language.  */
static const char *
englishname_of_language ()
{
  for (size_t i = 0; i < language_table_size; i++)
    if (strcmp (language_table[i].code, language) == 0)
      return language_table[i].english;

  return xasprintf ("Language %s", language);
}


/* Construct the value for the PACKAGE name.  */
static const char *
project_id (const char *header)
{
  /* Return the first part of the Project-Id-Version field if present, assuming
     it was already filled in by xgettext.  */
  {
    const char *old_field = get_field (header, "Project-Id-Version");
    if (old_field != NULL && strcmp (old_field, "PACKAGE VERSION") != 0)
      {
        /* Remove the last word from old_field.  */
        const char *last_space = strrchr (old_field, ' ');
        if (last_space != NULL)
          {
            while (last_space > old_field && last_space[-1] == ' ')
              last_space--;
            if (last_space > old_field)
              {
                size_t package_len = last_space - old_field;
                char *package = XNMALLOC (package_len + 1, char);
                memcpy (package, old_field, package_len);
                package[package_len] = '\0';

                return package;
              }
          }
        /* It contains no version, just a package name.  */
        return old_field;
      }
  }

  /* On native Windows, a Bourne shell is generally not available.
     Avoid error messages such as
     "msginit.exe: subprocess ... failed: No such file or directory"  */
#if !(defined _WIN32 && ! defined __CYGWIN__)
  {
    const char *gettextlibdir = getenv ("GETTEXTLIBEXECDIR_SRCDIR");
    if (gettextlibdir == NULL || gettextlibdir[0] == '\0')
      gettextlibdir = relocate (LIBEXECDIR "/gettext");

    char *prog = xconcatenated_filename (gettextlibdir, "project-id", NULL);

    /* Call the project-id shell script.  */
    const char *argv[3];
    argv[0] = BOURNE_SHELL;
    argv[1] = prog;
    argv[2] = NULL;

    int fd[1];
    pid_t child = create_pipe_in (prog, BOURNE_SHELL, argv, NULL, NULL,
                                  DEV_NULL, false, true, false, fd);
    if (child == -1)
      goto failed;

    /* Retrieve its result.  */
    FILE *fp = fdopen (fd[0], "r");
    if (fp == NULL)
      {
        error (0, errno, _("fdopen() failed"));
        goto failed;
      }

    char *line = NULL;
    size_t linesize = 0;
    size_t linelen = getline (&line, &linesize, fp);
    if (linelen == (size_t)(-1))
      {
        error (0, 0, _("%s subprocess I/O error"), prog);
        fclose (fp);
        goto failed;
      }
    if (linelen > 0 && line[linelen - 1] == '\n')
      line[linelen - 1] = '\0';

    fclose (fp);

    /* Remove zombie process from process list, and retrieve exit status.  */
    int exitstatus =
      wait_subprocess (child, prog, false, false, true, false, NULL);
    if (exitstatus != 0)
      {
        error (0, 0, _("%s subprocess failed with exit code %d"),
               prog, exitstatus);
        goto failed;
      }

    return line;
  }

failed:
#endif
  return "PACKAGE";
}


/* Construct the value for the Project-Id-Version field.  */
static const char *
project_id_version (const char *header)
{
  /* Return the old value if present, assuming it was already filled in by
     xgettext.  */
  {
    const char *old_field = get_field (header, "Project-Id-Version");
    if (old_field != NULL && strcmp (old_field, "PACKAGE VERSION") != 0)
      return old_field;
  }

  /* On native Windows, a Bourne shell is generally not available.
     Avoid error messages such as
     "msginit.exe: subprocess ... failed: No such file or directory"  */
#if !(defined _WIN32 && ! defined __CYGWIN__)
  {
    const char *gettextlibdir = getenv ("GETTEXTLIBEXECDIR_SRCDIR");
    if (gettextlibdir == NULL || gettextlibdir[0] == '\0')
      gettextlibdir = relocate (LIBEXECDIR "/gettext");

    char *prog = xconcatenated_filename (gettextlibdir, "project-id", NULL);

    /* Call the project-id shell script.  */
    const char *argv[4];
    argv[0] = BOURNE_SHELL;
    argv[1] = prog;
    argv[2] = "yes";
    argv[3] = NULL;

    int fd[1];
    pid_t child = create_pipe_in (prog, BOURNE_SHELL, argv, NULL, NULL,
                                  DEV_NULL, false, true, false, fd);
    if (child == -1)
      goto failed;

    /* Retrieve its result.  */
    FILE *fp = fdopen (fd[0], "r");
    if (fp == NULL)
      {
        error (0, errno, _("fdopen() failed"));
        goto failed;
      }

    char *line = NULL;
    size_t linesize = 0;
    size_t linelen = getline (&line, &linesize, fp);
    if (linelen == (size_t)(-1))
      {
        error (0, 0, _("%s subprocess I/O error"), prog);
        fclose (fp);
        goto failed;
      }
    if (linelen > 0 && line[linelen - 1] == '\n')
      line[linelen - 1] = '\0';

    fclose (fp);

    /* Remove zombie process from process list, and retrieve exit status.  */
    int exitstatus =
      wait_subprocess (child, prog, false, false, true, false, NULL);
    if (exitstatus != 0)
      {
        error (0, 0, _("%s subprocess failed with exit code %d"),
               prog, exitstatus);
        goto failed;
      }

    return line;
  }

failed:
#endif
  return "PACKAGE VERSION";
}


/* Construct the value for the PO-Revision-Date field.  */
static const char *
po_revision_date (const char *header)
{
  if (no_translator)
    /* Because the PO file is automatically generated, we use the
       POT-Creation-Date, not the current time.  */
    return get_field (header, "POT-Creation-Date");
  else
    {
      /* Assume the translator will modify the PO file now.  */
      time_t now;
      time (&now);

      return po_strftime (&now);
    }
}


#if HAVE_PWD_H  /* Only Unix, not native Windows.  */

/* Returns the struct passwd entry for the current user.  */
static struct passwd *
get_user_pwd ()
{
  /* 1. attempt: getpwnam(getenv("USER"))  */
  {
    const char *username = getenv ("USER");
    if (username != NULL)
      {
        errno = 0;
        struct passwd *userpasswd = getpwnam (username);
        if (userpasswd != NULL)
          return userpasswd;
        if (errno != 0)
          error (EXIT_FAILURE, errno, "getpwnam(\"%s\")", username);
      }
  }

  /* 2. attempt: getpwnam(getlogin())  */
  {
    const char *username = getlogin ();
    if (username != NULL)
      {
        errno = 0;
        struct passwd *userpasswd = getpwnam (username);
        if (userpasswd != NULL)
          return userpasswd;
        if (errno != 0)
          error (EXIT_FAILURE, errno, "getpwnam(\"%s\")", username);
      }
  }

  /* 3. attempt: getpwuid(getuid())  */
  errno = 0;
  struct passwd *userpasswd = getpwuid (getuid ());
  if (userpasswd != NULL)
    return userpasswd;
  if (errno != 0)
    error (EXIT_FAILURE, errno, "getpwuid(%ju)", (uintmax_t) getuid ());

  return NULL;
}

#endif


/* Return the user's full name.  */
static const char *
get_user_fullname ()
{
#if HAVE_PWD_H
  struct passwd *pwd = get_user_pwd ();
  if (pwd != NULL)
    {
      /* Return the pw_gecos field, up to the first comma (if any).  */
      const char *fullname = pwd->pw_gecos;
      const char *fullname_end = strchr (fullname, ',');
      if (fullname_end == NULL)
        fullname_end = fullname + strlen (fullname);

      char *result = XNMALLOC (fullname_end - fullname + 1, char);
      memcpy (result, fullname, fullname_end - fullname);
      result[fullname_end - fullname] = '\0';

      return result;
    }
#endif

  return NULL;
}


/* Return the user's email address.  */
static const char *
get_user_email ()
{
  /* On native Windows, a Bourne shell is generally not available.
     Avoid error messages such as
     "msginit.exe: subprocess ... failed: No such file or directory"  */
#if !(defined _WIN32 && ! defined __CYGWIN__)
  {
    const char *prog = relocate (LIBEXECDIR "/gettext/user-email");

    /* The program 'hostname', that 'user-email' may invoke, is installed in
       gettextlibdir and depends on libintl and libgettextlib.  On Windows,
       in installations with shared libraries, these DLLs are installed in
       ${bindir}.  Make sure that the program can find them, even if
       ${bindir} is not in $PATH.  */
    const char *dll_dirs[2];
    dll_dirs[0] = relocate (BINDIR);
    dll_dirs[1] = NULL;

    /* Ask the user for his email address.  */
    const char *argv[4];
    argv[0] = BOURNE_SHELL;
    argv[1] = prog;
    argv[2] = _("\
The new message catalog should contain your email address, so that users can\n\
give you feedback about the translations, and so that maintainers can contact\n\
you in case of unexpected technical problems.\n");
    argv[3] = NULL;

    int fd[1];
    pid_t child = create_pipe_in (prog, BOURNE_SHELL, argv, dll_dirs, NULL,
                                  DEV_NULL, false, true, false, fd);
    if (child == -1)
      goto failed;

    /* Retrieve his answer.  */
    FILE *fp = fdopen (fd[0], "r");
    if (fp == NULL)
      {
        error (0, errno, _("fdopen() failed"));
        goto failed;
      }

    char *line = NULL;
    size_t linesize = 0;
    size_t linelen = getline (&line, &linesize, fp);
    if (linelen == (size_t)(-1))
      {
        error (0, 0, _("%s subprocess I/O error"), prog);
        fclose (fp);
        goto failed;
      }
    if (linelen > 0 && line[linelen - 1] == '\n')
      line[linelen - 1] = '\0';

    fclose (fp);

    /* Remove zombie process from process list, and retrieve exit status.  */
    int exitstatus =
      wait_subprocess (child, prog, false, false, true, false, NULL);
    if (exitstatus != 0)
      {
        error (0, 0, _("%s subprocess failed with exit code %d"),
               prog, exitstatus);
        goto failed;
      }

    return line;
  }

failed:
#endif
  return "EMAIL@ADDRESS";
}


/* Construct the value for the Last-Translator field.  */
static const char *
last_translator ()
{
  if (no_translator)
    return "Automatically generated";
  else
    {
      const char *fullname = get_user_fullname ();
      const char *email = get_user_email ();

      if (fullname != NULL)
        return xasprintf ("%s <%s>", fullname, email);
      else
        return xasprintf ("<%s>", email);
    }
}


/* Return the name of the language used by the language team, in English.  */
static const char *
language_team_englishname ()
{
  /* Search for a name depending on the catalogname.  */
  for (size_t i = 0; i < language_variant_table_size; i++)
    if (strcmp (language_variant_table[i].code, catalogname) == 0)
      return language_variant_table[i].english;

  /* Search for a name depending on the language only.  */
  return englishname_of_language ();
}


/* Return the language team's mailing list address or homepage URL.  */
static const char *
language_team_address ()
{
  /* On native Windows, a Bourne shell is generally not available.
     Avoid error messages such as
     "msginit.exe: subprocess ... failed: No such file or directory"  */
#if !(defined _WIN32 && ! defined __CYGWIN__)
  {
    const char *prog = relocate (PROJECTSDIR "/team-address");

    /* The program 'urlget', that 'team-address' may invoke, is installed in
       gettextlibdir and depends on libintl and libgettextlib.  On Windows,
       in installations with shared libraries, these DLLs are installed in
       ${bindir}.  Make sure that the program can find them, even if
       ${bindir} is not in $PATH.  */
    const char *dll_dirs[2];
    dll_dirs[0] = relocate (BINDIR);
    dll_dirs[1] = NULL;

    /* Call the team-address shell script.  */
    const char *argv[7];
    argv[0] = BOURNE_SHELL;
    argv[1] = prog;
    argv[2] = relocate (PROJECTSDIR);
    argv[3] = relocate (LIBEXECDIR "/gettext");
    argv[4] = catalogname;
    argv[5] = language;
    argv[6] = NULL;

    int fd[1];
    pid_t child = create_pipe_in (prog, BOURNE_SHELL, argv, dll_dirs, NULL,
                                  DEV_NULL, false, true, false, fd);
    if (child == -1)
      goto failed;

    /* Retrieve its result.  */
    FILE *fp = fdopen (fd[0], "r");
    if (fp == NULL)
      {
        error (0, errno, _("fdopen() failed"));
        goto failed;
      }

    char *line = NULL;
    size_t linesize = 0;
    size_t linelen = getline (&line, &linesize, fp);
    const char *result;
    if (linelen == (size_t)(-1))
      result = "";
    else
      {
        if (linelen > 0 && line[linelen - 1] == '\n')
          line[linelen - 1] = '\0';
        result = line;
      }

    fclose (fp);

    /* Remove zombie process from process list, and retrieve exit status.  */
    int exitstatus =
      wait_subprocess (child, prog, false, false, true, false, NULL);
    if (exitstatus != 0)
      {
        error (0, 0, _("%s subprocess failed with exit code %d"),
               prog, exitstatus);
        goto failed;
      }

    return result;
  }

failed:
#endif
  return "";
}


/* Construct the value for the Language-Team field.  */
static const char *
language_team ()
{
  if (no_translator)
    return "none";
  else
    {
      const char *englishname = language_team_englishname ();
      const char *address = language_team_address ();

      if (address != NULL && address[0] != '\0')
        return xasprintf ("%s %s", englishname, address);
      else
        return englishname;
    }
}


/* Construct the value for the Language field.  */
static const char *
language_value ()
{
  return catalogname;
}


/* Construct the value for the MIME-Version field.  */
static const char *
mime_version ()
{
  return "1.0";
}


/* Construct the value for the Content-Type field.  */
static const char *
content_type (const char *header)
{
  return xasprintf ("text/plain; charset=%s", output_charset);
}


/* Construct the value for the Content-Transfer-Encoding field.  */
static const char *
content_transfer_encoding ()
{
  return "8bit";
}


/* Construct the value for the Plural-Forms field.  */
static const char *
plural_forms ()
{
  /* Search for a formula depending on the catalogname.  */
  for (size_t i = 0; i < plural_table_size; i++)
    if (strcmp (plural_table[i].lang, catalogname) == 0)
      return plural_table[i].value;

  /* Search for a formula depending on the language only.  */
  for (size_t i = 0; i < plural_table_size; i++)
    if (strcmp (plural_table[i].lang, language) == 0)
      return plural_table[i].value;

  const char *gettextcldrdir = getenv ("GETTEXTCLDRDIR");
  if (gettextcldrdir != NULL && gettextcldrdir[0] != '\0')
    {
      const char *gettextlibdir = getenv ("GETTEXTLIBEXECDIR_BUILDDIR");
      if (gettextlibdir == NULL || gettextlibdir[0] == '\0')
        gettextlibdir = relocate (LIBEXECDIR "/gettext");

      char *prog =
        xconcatenated_filename (gettextlibdir, "cldr-plurals", EXEEXT);

      char *last_dir;
      {
        last_dir = xstrdup (gettextcldrdir);
        const char *dirs[3];
        dirs[0] = "common";
        dirs[1] = "supplemental";
        dirs[2] = "plurals.xml";
        for (size_t i = 0; i < SIZEOF (dirs); i++)
          {
            char *dir = xconcatenated_filename (last_dir, dirs[i], NULL);
            free (last_dir);
            last_dir = dir;
          }
      }

      /* The program 'cldr-plurals', that we invoke here, is installed in
         gettextlibdir and depends on libintl and libgettextlib.  On Windows,
         in installations with shared libraries, these DLLs are installed in
         ${bindir}.  Make sure that the program can find them, even if
         ${bindir} is not in $PATH.  */
      const char *dll_dirs[2];
      dll_dirs[0] = relocate (BINDIR);
      dll_dirs[1] = NULL;

      /* Call the cldr-plurals command.
         argv[0] must be prog, not just the base name "cldr-plurals",
         because on Cygwin in a build with --enable-shared, the libtool
         wrapper of cldr-plurals.exe apparently needs this.  */
      const char *argv[4];
      argv[0] = prog;
      argv[1] = language;
      argv[2] = last_dir;
      argv[3] = NULL;

      int fd[1];
      pid_t child = create_pipe_in (prog, prog, argv, dll_dirs, NULL,
                                    DEV_NULL, false, true, false, fd);
      free (last_dir);
      if (child == -1)
        goto failed;

      /* Retrieve its result.  */
      FILE *fp = fdopen (fd[0], "r");
      if (fp == NULL)
        {
          error (0, errno, _("fdopen() failed"));
          goto failed;
        }

      char *line = NULL;
      size_t linesize = 0;
      size_t linelen = getline (&line, &linesize, fp);
      if (linelen == (size_t)(-1))
        {
          error (0, 0, _("%s subprocess I/O error"), prog);
          fclose (fp);
          goto failed;
        }
      if (linelen > 0 && line[linelen - 1] == '\n')
        {
          line[linelen - 1] = '\0';
#if defined _WIN32 && ! defined __CYGWIN__
          if (linelen > 1 && line[linelen - 2] == '\r')
            line[linelen - 2] = '\0';
#endif
        }

      fclose (fp);

      /* Remove zombie process from process list, and retrieve exit status.  */
      int exitstatus =
        wait_subprocess (child, prog, false, false, true, false, NULL);
      if (exitstatus != 0)
        {
          error (0, 0, _("%s subprocess failed with exit code %d"),
                 prog, exitstatus);
          goto failed;
        }

      return line;

     failed:
      free (prog);
    }
  return NULL;
}


struct header_entry_field
{
  const char *name;
  const char * (*getter0) (void);
  const char * (*getter1) (const char *header);
};

static struct header_entry_field fresh_fields[] =
  {
    { "Project-Id-Version", NULL, project_id_version },
    { "PO-Revision-Date", NULL, po_revision_date },
    { "Last-Translator", last_translator, NULL },
    { "Language-Team", language_team, NULL },
    { "Language", language_value, NULL },
    { "MIME-Version", mime_version, NULL },
    { "Content-Type", NULL, content_type },
    { "Content-Transfer-Encoding", content_transfer_encoding, NULL },
    { "Plural-Forms", plural_forms, NULL }
  };
#define FRESH_FIELDS_LAST_TRANSLATOR 2

static struct header_entry_field update_fields[] =
  {
    { "Last-Translator", last_translator, NULL }
  };
#define UPDATE_FIELDS_LAST_TRANSLATOR 0


/* Retrieve a freshly allocated copy of a field's value.  */
static char *
get_field (const char *header, const char *field)
{
  size_t len = strlen (field);

  for (const char *line = header;;)
    {
      if (strncmp (line, field, len) == 0 && line[len] == ':')
        {
          const char *value_start = line + len + 1;
          if (*value_start == ' ')
            value_start++;

          const char *value_end = strchr (value_start, '\n');
          if (value_end == NULL)
            value_end = value_start + strlen (value_start);

          char *value = XNMALLOC (value_end - value_start + 1, char);
          memcpy (value, value_start, value_end - value_start);
          value[value_end - value_start] = '\0';

          return value;
        }

      line = strchr (line, '\n');
      if (line != NULL)
        line++;
      else
        break;
    }

  return NULL;
}

/* Add a field with value to a header, and return the new header.  */
static char *
put_field (const char *old_header, const char *field, const char *value)
{
  size_t len = strlen (field);

  for (const char *line = old_header;;)
    {
      if (strncmp (line, field, len) == 0 && line[len] == ':')
        {
          const char *value_start = line + len + 1;
          if (*value_start == ' ')
            value_start++;

          const char *value_end = strchr (value_start, '\n');
          if (value_end == NULL)
            value_end = value_start + strlen (value_start);

          char *new_header = XNMALLOC (strlen (old_header)
                                       - (value_end - value_start)
                                       + strlen (value)
                                       + (*value_end != '\n' ? 1 : 0)
                                       + 1,
                                       char);
          {
            char *p = new_header;
            memcpy (p, old_header, value_start - old_header);
            p += value_start - old_header;
            memcpy (p, value, strlen (value));
            p += strlen (value);
            if (*value_end != '\n')
              *p++ = '\n';
            strcpy (p, value_end);
          }

          return new_header;
        }

      line = strchr (line, '\n');
      if (line != NULL)
        line++;
      else
        break;
    }

  char *new_header = XNMALLOC (strlen (old_header) + 1
                               + len + 2 + strlen (value) + 1
                               + 1,
                               char);
  {
    char *p = new_header;
    memcpy (p, old_header, strlen (old_header));
    p += strlen (old_header);
    if (p > new_header && p[-1] != '\n')
      *p++ = '\n';
    memcpy (p, field, len);
    p += len;
    *p++ = ':';
    *p++ = ' ';
    memcpy (p, value, strlen (value));
    p += strlen (value);
    *p++ = '\n';
    *p = '\0';
  }

  return new_header;
}


/* Return the title format string.  */
static const char *
get_title ()
{
  /* This is tricky.  We want the translation in the given locale specified by
     the command line, not the current locale.  But we want it in the encoding
     that we put into the header entry, not the encoding of that locale.
     We could avoid the use of xstr_iconv() by using a separate message catalog
     and bind_textdomain_codeset(), but that doesn't seem worth the trouble
     for one single message.  */

  /* First, the English title.  */
  const char *english = xasprintf ("%s translations for %%s package",
                                   englishname_of_language ());

  /* Save LC_ALL, LANGUAGE environment variables.  */
  char *old_LC_ALL;
  {
    const char *tmp = getenv ("LC_ALL");
    old_LC_ALL = (tmp != NULL ? xstrdup (tmp) : NULL);
  }
  char *old_LANGUAGE;
  {
    const char *tmp = getenv ("LANGUAGE");
    old_LANGUAGE = (tmp != NULL ? xstrdup (tmp) : NULL);
  }

  xsetenv ("LC_ALL", locale, 1);
  unsetenv ("LANGUAGE");

  const char *result;
  if (setlocale (LC_ALL, "") == NULL)
    /* Nonexistent locale.  Use the English title.  */
    result = english;
  else
    {
      /* Fetch the translation.  */
      /* TRANSLATORS: "English" needs to be replaced by your language.
         For example in it.po write "Traduzioni italiani ...",
         *not* "Traduzioni inglesi ...".  */
      const char *msgid = N_("English translations for %s package");
      result = gettext (msgid);
      if (result != msgid && strcmp (result, msgid) != 0)
        /* Use the English and the foreign title.  */
        result = xasprintf ("%s\n%s", english,
                            xstr_iconv (result, locale_charset (),
                                        output_charset));
      else
        /* No translation found.  Use the English title.  */
        result = english;
    }

  /* Restore LC_ALL, LANGUAGE environment variables.  */

  if (old_LC_ALL != NULL)
    xsetenv ("LC_ALL", old_LC_ALL, 1), free (old_LC_ALL);
  else
    unsetenv ("LC_ALL");

  if (old_LANGUAGE != NULL)
    xsetenv ("LANGUAGE", old_LANGUAGE, 1), free (old_LANGUAGE);
  else
    unsetenv ("LANGUAGE");

  setlocale (LC_ALL, "");

  return result;
}


/* Perform a set of substitutions in a string and return the resulting
   string.  When subst[j][0] found, it is replaced with subst[j][1].
   subst[j][0] must not be the empty string.  */
static const char *
subst_string (const char *str,
              unsigned int nsubst, const char *(*subst)[2])
{
  if (nsubst > 0)
    {
      size_t *substlen = (size_t *) xmalloca (nsubst * sizeof (size_t));
      for (unsigned int j = 0; j < nsubst; j++)
        {
          substlen[j] = strlen (subst[j][0]);
          if (substlen[j] == 0)
            abort ();
        }

      char *malloced = NULL;

      for (size_t i = 0;;)
        {
          if (str[i] == '\0')
            break;
          unsigned int j;
          for (j = 0; j < nsubst; j++)
            if (*(str + i) == *subst[j][0]
                && strncmp (str + i, subst[j][0], substlen[j]) == 0)
              {
                size_t replacement_len = strlen (subst[j][1]);
                size_t new_len = strlen (str) - substlen[j] + replacement_len;
                char *new_str = XNMALLOC (new_len + 1, char);
                memcpy (new_str, str, i);
                memcpy (new_str + i, subst[j][1], replacement_len);
                strcpy (new_str + i + replacement_len, str + i + substlen[j]);
                if (malloced != NULL)
                  free (malloced);
                str = new_str;
                malloced = new_str;
                i += replacement_len;
                break;
              }
          if (j == nsubst)
            i++;
        }

      freea (substlen);
    }

  return str;
}

/* Perform a set of substitutions on each string of a string list.
   When subst[j][0] found, it is replaced with subst[j][1].  subst[j][0]
   must not be the empty string.  */
static void
subst_string_list (string_list_ty *slp,
                   unsigned int nsubst, const char *(*subst)[2])
{
  for (size_t j = 0; j < slp->nitems; j++)
    slp->item[j] = subst_string (slp->item[j], nsubst, subst);
}


/* Fill the templates in all fields of the header entry.  */
static msgdomain_list_ty *
fill_header (msgdomain_list_ty *mdlp, bool fresh)
{
  /* Determine the desired encoding to the PO file.
     If the POT file contains charset=UTF-8, it means that the POT file
     contains non-ASCII characters, and we keep the UTF-8 encoding.
     Otherwise, when the POT file is plain ASCII, we use the locale's
     encoding.  */
  bool was_utf8 = false;
  for (size_t k = 0; k < mdlp->nitems; k++)
    {
      message_list_ty *mlp = mdlp->item[k]->messages;

      if (mlp->nitems > 0)
        {
          message_ty *header_mp = NULL;

          /* Search the header entry.  */
          for (size_t j = 0; j < mlp->nitems; j++)
            if (is_header (mlp->item[j]) && !mlp->item[j]->obsolete)
              {
                header_mp = mlp->item[j];
                break;
              }

          if (header_mp != NULL)
            {
              const char *header = header_mp->msgstr;
              const char *old_field = get_field (header, "Content-Type");

              if (old_field != NULL)
                {
                  const char *charsetstr = c_strstr (old_field, "charset=");
                  if (charsetstr != NULL)
                    {
                      charsetstr += strlen ("charset=");
                      if (c_strcasecmp (charsetstr, "UTF-8") == 0)
                        was_utf8 = true;
                    }
                }
            }
        }
    }

  output_charset = (was_utf8 ? "UTF-8" : canonical_locale_charset ());

  /* Cache the strings filled in, for use when there are multiple domains
     and a header entry for each domain.  */
  struct header_entry_field *fields;
  size_t nfields;
  size_t field_last_translator;
  if (fresh)
    {
      fields = fresh_fields;
      nfields = SIZEOF (fresh_fields);
      field_last_translator = FRESH_FIELDS_LAST_TRANSLATOR;
    }
  else
    {
      fields = update_fields;
      nfields = SIZEOF (update_fields);
      field_last_translator = UPDATE_FIELDS_LAST_TRANSLATOR;
    }

  const char **field_value = XNMALLOC (nfields, const char *);
  for (size_t i = 0; i < nfields; i++)
    field_value[i] = NULL;

  for (size_t k = 0; k < mdlp->nitems; k++)
    {
      message_list_ty *mlp = mdlp->item[k]->messages;

      if (mlp->nitems > 0)
        {
          message_ty *header_mp = NULL;

          /* Search the header entry.  */
          for (size_t j = 0; j < mlp->nitems; j++)
            if (is_header (mlp->item[j]) && !mlp->item[j]->obsolete)
              {
                header_mp = mlp->item[j];
                break;
              }

          /* If it wasn't found, provide one.  */
          if (header_mp == NULL)
            {
              static lex_pos_ty pos = { __FILE__, __LINE__ };

              header_mp = message_alloc (NULL, "", NULL, "", 1, &pos);
              message_list_prepend (mlp, header_mp);
            }

          char *header = xstrdup (header_mp->msgstr);

          /* Fill in the fields.  */
          for (size_t i = 0; i < nfields; i++)
            {
              if (field_value[i] == NULL)
                field_value[i] =
                  (fields[i].getter1 != NULL
                   ? fields[i].getter1 (header)
                   : fields[i].getter0 ());

              if (field_value[i] != NULL)
                {
                  char *old_header = header;
                  header = put_field (header, fields[i].name, field_value[i]);
                  free (old_header);
                }
            }

          /* Replace the old translation in the header entry.  */
          header_mp->msgstr = header;
          header_mp->msgstr_len = strlen (header) + 1;

          /* Update the comments in the header entry.  */
          if (header_mp->comment != NULL)
            {
              const char *id = project_id (header);

              const char *subst[4][2];
              subst[0][0] = "SOME DESCRIPTIVE TITLE";
              subst[0][1] = xasprintf (get_title (), id, id);
              subst[1][0] = "PACKAGE";
              subst[1][1] = id;
              subst[2][0] = "FIRST AUTHOR <EMAIL@ADDRESS>";
              subst[2][1] = field_value[field_last_translator];
              subst[3][0] = "YEAR";
              {
                time_t now;
                subst[3][1] =
                  xasprintf ("%d",
                             (time (&now), (localtime (&now))->tm_year + 1900));
              }

              subst_string_list (header_mp->comment, SIZEOF (subst), subst);
            }

          /* Finally remove the fuzzy attribute.  */
          header_mp->is_fuzzy = false;
        }
    }

  free (field_value);

  return mdlp;
}

/* ------------------------------------------------------------------------- */


/* Update the msgstr plural entries according to the nplurals count.  */
static msgdomain_list_ty *
update_msgstr_plurals (msgdomain_list_ty *mdlp)
{
  for (size_t k = 0; k < mdlp->nitems; k++)
    {
      message_list_ty *mlp = mdlp->item[k]->messages;

      message_ty *header_entry = message_list_search (mlp, NULL, "");

      unsigned long int nplurals =
        get_plural_count (header_entry ? header_entry->msgstr : NULL);

      char *untranslated_plural_msgstr = XNMALLOC (nplurals, char);
      memset (untranslated_plural_msgstr, '\0', nplurals);

      for (size_t j = 0; j < mlp->nitems; j++)
        {
          message_ty *mp = mlp->item[j];

          if (mp->msgid_plural != NULL)
            {
              /* Test if mp is untranslated.  (It most likely is.)  */
              bool is_untranslated = true;
              {
                const char *p = mp->msgstr;
                const char *pend = p + mp->msgstr_len;
                for (; p < pend; p++)
                  if (*p != '\0')
                    {
                      is_untranslated = false;
                      break;
                    }
              }
              if (is_untranslated)
                {
                  /* Change mp->msgstr_len consecutive empty strings into
                     nplurals consecutive empty strings.  */
                  if (nplurals > mp->msgstr_len)
                    mp->msgstr = untranslated_plural_msgstr;
                  mp->msgstr_len = nplurals;
                }
            }
        }
    }
  return mdlp;
}
