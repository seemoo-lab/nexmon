/* Table of known plural form expressions.
   Copyright (C) 2001-2006, 2009-2010, 2015-2016 Free Software Foundation, Inc.
   Written by Bruno Haible <haible@clisp.cons.org>, 2002.

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
# include "config.h"
#endif

/* Specification.  */
#include "plural-table.h"

/* Formulas taken from the documentation, node "Plural forms".  */
struct plural_table_entry plural_table[] =
  {
    { "ja", "Japanese",          "nplurals=1; plural=0;" },
    { "vi", "Vietnamese",        "nplurals=1; plural=0;" },
    { "ko", "Korean",            "nplurals=1; plural=0;" },
    { "en", "English",           "nplurals=2; plural=(n != 1);" },
    { "de", "German",            "nplurals=2; plural=(n != 1);" },
    { "nl", "Dutch",             "nplurals=2; plural=(n != 1);" },
    { "sv", "Swedish",           "nplurals=2; plural=(n != 1);" },
    { "da", "Danish",            "nplurals=2; plural=(n != 1);" },
    { "no", "Norwegian",         "nplurals=2; plural=(n != 1);" },
    { "nb", "Norwegian Bokmal",  "nplurals=2; plural=(n != 1);" },
    { "nn", "Norwegian Nynorsk", "nplurals=2; plural=(n != 1);" },
    { "fo", "Faroese",           "nplurals=2; plural=(n != 1);" },
    { "es", "Spanish",           "nplurals=2; plural=(n != 1);" },
    { "pt", "Portuguese",        "nplurals=2; plural=(n != 1);" },
    { "it", "Italian",           "nplurals=2; plural=(n != 1);" },
    { "bg", "Bulgarian",         "nplurals=2; plural=(n != 1);" },
    { "el", "Greek",             "nplurals=2; plural=(n != 1);" },
    { "fi", "Finnish",           "nplurals=2; plural=(n != 1);" },
    { "et", "Estonian",          "nplurals=2; plural=(n != 1);" },
    { "he", "Hebrew",            "nplurals=2; plural=(n != 1);" },
    { "eo", "Esperanto",         "nplurals=2; plural=(n != 1);" },
    { "hu", "Hungarian",         "nplurals=2; plural=(n != 1);" },
    { "tr", "Turkish",           "nplurals=2; plural=(n != 1);" },
    { "pt_BR", "Brazilian",      "nplurals=2; plural=(n > 1);" },
    { "fr", "French",            "nplurals=2; plural=(n > 1);" },
    { "lv", "Latvian",           "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n != 0 ? 1 : 2);" },
    { "ga", "Irish",             "nplurals=3; plural=n==1 ? 0 : n==2 ? 1 : 2;" },
    { "ro", "Romanian",          "nplurals=3; plural=n==1 ? 0 : (n==0 || (n%100 > 0 && n%100 < 20)) ? 1 : 2;" },
    { "lt", "Lithuanian",        "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "ru", "Russian",           "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "uk", "Ukrainian",         "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "be", "Belarusian",        "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "sr", "Serbian",           "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "hr", "Croatian",          "nplurals=3; plural=(n%10==1 && n%100!=11 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "cs", "Czech",             "nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;" },
    { "sk", "Slovak",            "nplurals=3; plural=(n==1) ? 0 : (n>=2 && n<=4) ? 1 : 2;" },
    { "pl", "Polish",            "nplurals=3; plural=(n==1 ? 0 : n%10>=2 && n%10<=4 && (n%100<10 || n%100>=20) ? 1 : 2);" },
    { "sl", "Slovenian",         "nplurals=4; plural=(n%100==1 ? 0 : n%100==2 ? 1 : n%100==3 || n%100==4 ? 2 : 3);" }
  };
const size_t plural_table_size = sizeof (plural_table) / sizeof (plural_table[0]);
