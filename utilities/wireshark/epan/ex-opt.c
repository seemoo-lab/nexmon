/*
 *  ex-opt.c
 *
 * Extension command line options
 *
 * (c) 2006, Luis E. Garcia Ontanon <luis@ontanon.org>
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <glib.h>
#include "ex-opt.h"

static GHashTable* ex_opts = NULL;

gboolean ex_opt_add(const gchar* optarg) {
    gchar** splitted;

    if (!ex_opts)
        ex_opts = g_hash_table_new(g_str_hash,g_str_equal);

    splitted = g_strsplit(optarg,":",2);

    if (splitted[0] && splitted[1]) {
        GPtrArray* this_opts = (GPtrArray *)g_hash_table_lookup(ex_opts,splitted[0]);

        if (this_opts) {
            g_ptr_array_add(this_opts,splitted[1]);
            g_free(splitted[0]);
        } else {
            this_opts = g_ptr_array_new();
            g_ptr_array_add(this_opts,splitted[1]);
            g_hash_table_insert(ex_opts,splitted[0],this_opts);
        }

        g_free(splitted);

        return TRUE;
    } else {
        g_strfreev(splitted);
        return FALSE;
    }
}

gint ex_opt_count(const gchar* key) {
    GPtrArray* this_opts;

    if (! ex_opts)
        return 0;

    this_opts = (GPtrArray *)g_hash_table_lookup(ex_opts,key);

    if (this_opts) {
        return this_opts->len;
    } else {
        return 0;
    }
}

const gchar* ex_opt_get_nth(const gchar* key, guint key_index) {
    GPtrArray* this_opts;

    if (! ex_opts)
        return 0;

    this_opts = (GPtrArray *)g_hash_table_lookup(ex_opts,key);

    if (this_opts) {
        if (this_opts->len > key_index) {
            return (const gchar *)g_ptr_array_index(this_opts,key_index);
        } else {
            /* XXX: assert? */
            return NULL;
        }
    } else {
        return NULL;
    }

}

extern const gchar* ex_opt_get_next(const gchar* key) {
    GPtrArray* this_opts;

    if (! ex_opts)
        return 0;

    this_opts = (GPtrArray *)g_hash_table_lookup(ex_opts,key);

    if (this_opts) {
        if (this_opts->len)
            return (const gchar *)g_ptr_array_remove_index(this_opts,0);
        else
            return NULL;
    } else {
        return NULL;
    }
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * vi: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
