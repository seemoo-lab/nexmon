/* Support for locale names in BCP 47 syntax.
   Copyright (C) 2024-2026 Free Software Foundation, Inc.

   This file is free software: you can redistribute it and/or modify
   it under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation, either version 3 of the
   License, or (at your option) any later version.

   This file is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* Written by Bruno Haible <bruno@clisp.org>, 2024.  */

#ifndef _BCP47_H
#define _BCP47_H

/* A locale name can exist in three possible forms:

     * The XPG syntax
         language[_territory][.codeset][@modifier]
       where
         - The language is an ISO 639 (two-letter) language code.
         - The territory is an ISO 3166 (two-letter) country code.
         - The codeset is typically UTF-8.
         - The supported @modifiers are usually something like
             @euro
             a script indicator, such as: @latin, @cyrillic, @devanagari

     * The locale name understood by setlocale().
       On glibc and many other Unix-like systems, this is the XPG syntax.
       On native Windows, it is similar to XPG syntax, with English names
       (instead of ISO codes) for the language and territory and with a
       number for the codeset (e.g. 65001 for UTF-8).

     * The BCP 47 syntax
         language[-script][-region]{-variant}*{-extension}*
       defined in
         <https://www.ietf.org/rfc/bcp/bcp47.html>
         = <https://www.rfc-editor.org/rfc/bcp/bcp47.txt>
       which consists of RFC 5646 and RFC 4647.
       See also <https://en.wikipedia.org/wiki/IETF_language_tag>.
       Note: The BCP 47 syntax does not include a codeset.

   This file provides conversions between the XPG syntax and the BCP 47
   syntax.  */

#ifdef __cplusplus
extern "C" {
#endif

/* Required size of buffer for a locale name.  */
#define BCP47_MAX 100

/* Converts a locale name in XPG syntax to a locale name in BCP 47 syntax.
   Returns the result in bcp47, which must be at least BCP47_MAX bytes
   large.  */
extern void xpg_to_bcp47 (char *bcp47, const char *xpg);

/* Converts a locale name in BCP 47 syntax (optionally with a codeset)
   to a locale name in XPG syntax.
   The specified codeset may be NULL.
   Returns the result in xpg, which must be at least BCP47_MAX bytes
   large.  */
extern void bcp47_to_xpg (char *xpg, const char *bcp47, const char *codeset);

#ifdef __cplusplus
}
#endif

#endif /* _BCP47_H */
