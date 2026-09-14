/* XML resource locating rules
   Copyright (C) 2015-2026 Free Software Foundation, Inc.

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

/* Written by Daiki Ueno and Bruno Haible.  */

#ifndef _LOCATING_RULE_H
#define _LOCATING_RULE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/* This file deals with how to find the ITS file for a given XML input.
   The caller needs to supply the following information:
     - The "language name", coming from xgettext's -L option or guessed
       from the input file's extension.
     - The XML file name.
   After opening the XML file, we get the top-level XML element name;
   this is called the 'localName'.

   The its/ directory contains a set a *.loc files; these are all read
   into memory and form a rule list.

   For example, this piece of XML:

     <locatingRule name="Glade" pattern="*.glade">
       <documentRule localName="GTK-Interface" target="glade1.its"/>
       <documentRule localName="glade-interface" target="glade2.its"/>
       <documentRule localName="interface" target="gtkbuilder.its"/>
     </locatingRule>
     <locatingRule name="Glade" pattern="*.glade2">
       <documentRule localName="glade-interface" target="glade2.its"/>
     </locatingRule>
     <locatingRule name="Glade" pattern="*.ui">
       <documentRule localName="interface" target="gtkbuilder.its"/>
     </locatingRule>
     <locatingRule name="AppData" pattern="*.appdata.xml">
       <documentRule localName="component" target="metainfo.its"/>
     </locatingRule>

   means:

     - If the language is "Glade" or the file name matches "*.glade",
       then look at the top-level XML element name:
       - If it's <GTK-Interface>, use the file glade1.its.
       - If it's <glade-interface>, use the file glade2.its.
       - If it's <interface>, use the file gtkbuilder.its.
     - If the language is "Glade" or the file name matches "*.glade2",
       then look at the top-level XML element name:
       - If it's <glade-interface>, use the file glade2.its.
     - If the language is "Glade" or the file name matches "*.ui",
       then look at the top-level XML element name:
       - If it's <interface>, use the file gtkbuilder.its.
     - If the language is "AppData" or the file name matches "*.appdata.xml",
       then look at the top-level XML element name:
       - If it's <component>, use the file metainfo.its.

   See the documentation node "Preparing Rules for XML Internationalization".
 */

/* The 'locating_rule_list_ty *' type represents a locating rule list.  */
typedef struct locating_rule_list_ty locating_rule_list_ty;

/* Creates a fresh locating_rule_list_ty and returns it.  */
extern struct locating_rule_list_ty *
       locating_rule_list_alloc (void);

/* Adds all rules from all *.loc files in the given DIRECTORY to the RULES
   object.  */
extern bool
       locating_rule_list_add_from_directory (locating_rule_list_ty *rules,
                                              const char *directory);

/* Determines the location of the .its file to be used for FILENAME,
   when the "language name" is NAME (can be NULL if not provided),
   according to the locating rules in the RULES object.
   The result is just the base name of the .its file; the caller then
   needs to find it, using "search-path.h".
   The lifetime of the result is limited by the lifetime of the RULES
   object.  */
extern const char *
       locating_rule_list_locate (const locating_rule_list_ty *rules,
                                  const char *filename,
                                  const char *name);

/* Releases memory allocated for the RULES object.  */
extern void
       locating_rule_list_free (locating_rule_list_ty *rules);

#ifdef __cplusplus
}
#endif

#endif  /* _LOCATING_RULE_H */
