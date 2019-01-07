/* dissector_filters.h
 * Routines for dissector generated display filters
 *
 * $Id: dissector_filters.h 33981 2010-08-28 19:27:19Z etxrab $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __DISSECTOR_FILTERS_H__
#define __DISSECTOR_FILTERS_H__


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/** @file 
 */

/** callback function definition: is a filter available for this packet? */
typedef gboolean (*is_filter_valid_func)(packet_info *pinfo);

/** callback function definition: return the available filter for this packet or NULL if no filter is available */
typedef const gchar* (*build_filter_string_func)(packet_info *pinfo);


/** register a dissector filter */
extern void register_dissector_filter(const char *name, is_filter_valid_func is_filter_valid, build_filter_string_func build_filter_string);



/*** THE FOLLOWING SHOULD NOT BE USED BY ANY DISSECTORS!!! ***/

typedef struct dissector_filter_s {
    const char *                name;
    is_filter_valid_func        is_filter_valid;
    build_filter_string_func    build_filter_string;
} dissector_filter_t;

WS_VAR_IMPORT GList *dissector_filter_list;

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* dissector_filters.h */
