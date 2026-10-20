(* High-level Modula-2 binding to the GNU libintl library.
   Copyright (C) 2025-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  *)

(* Written by Bruno Haible.  *)

IMPLEMENTATION MODULE Libintl;

FROM LibintlFromC IMPORT setlocale,
                         gettext, dgettext, dcgettext,
                         ngettext, dngettext, dcngettext,
                         textdomain, bindtextdomain, bind_textdomain_codeset;
FROM DynamicStrings IMPORT InitStringCharStar, CopyOut;
FROM SYSTEM IMPORT ADDRESS, ADR;

PROCEDURE SetLocale (category: INTEGER; locale: ARRAY OF CHAR);
VAR
  unused: ADDRESS;
BEGIN
  unused := setlocale(category, ADR(locale));
END SetLocale;

PROCEDURE Gettext (msgid: ARRAY OF CHAR) : String;
VAR
  ret: String;
BEGIN
  ret := InitStringCharStar(gettext(ADR(msgid)));
  RETURN ret;
END Gettext;

PROCEDURE DGettext (domainname: ARRAY OF CHAR; msgid: ARRAY OF CHAR) : String;
VAR
  ret: String;
BEGIN
  ret := InitStringCharStar(dgettext(ADR(domainname), ADR(msgid)));
  RETURN ret;
END DGettext;

PROCEDURE DCGettext (domainname: ARRAY OF CHAR; msgid: ARRAY OF CHAR; category: INTEGER) : String;
VAR
  ret: String;
BEGIN
  ret := InitStringCharStar(dcgettext(ADR(domainname), ADR(msgid), category));
  RETURN ret;
END DCGettext;

PROCEDURE NGettext (msgid: ARRAY OF CHAR; msgid_plural: ARRAY OF CHAR; n: CARDINAL) : String;
VAR
  ret: String;
BEGIN
  ret := InitStringCharStar(ngettext(ADR(msgid), ADR(msgid_plural), n));
  RETURN ret;
END NGettext;

PROCEDURE DNGettext (domainname: ARRAY OF CHAR; msgid: ARRAY OF CHAR; msgid_plural: ARRAY OF CHAR; n: CARDINAL) : String;
VAR
  ret: String;
BEGIN
  ret := InitStringCharStar(dngettext(ADR(domainname), ADR(msgid), ADR(msgid_plural), n));
  RETURN ret;
END DNGettext;

PROCEDURE DCNGettext (domainname: ARRAY OF CHAR; msgid: ARRAY OF CHAR; msgid_plural: ARRAY OF CHAR; n: CARDINAL; category: INTEGER) : String;
VAR
  ret: String;
BEGIN
  ret := InitStringCharStar(dcngettext(ADR(domainname), ADR(msgid), ADR(msgid_plural), n, category));
  RETURN ret;
END DCNGettext;

PROCEDURE TextDomain (domainname: ARRAY OF CHAR);
VAR
  unused: ADDRESS;
BEGIN
  unused := textdomain(ADR(domainname));
END TextDomain;

PROCEDURE BindTextDomain (domainname: ARRAY OF CHAR; dirname: ARRAY OF CHAR);
VAR
  unused: ADDRESS;
BEGIN
  unused := bindtextdomain(ADR(domainname), ADR(dirname));
END BindTextDomain;

PROCEDURE BindTextDomainCodeset (domainname: ARRAY OF CHAR; codeset: ARRAY OF CHAR);
VAR
  unused: ADDRESS;
BEGIN
  unused := bind_textdomain_codeset(ADR(domainname), ADR(codeset));
END BindTextDomainCodeset;

END Libintl.
