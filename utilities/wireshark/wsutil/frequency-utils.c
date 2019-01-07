/* frequency-utils.c
 * Frequency conversion utility definitions
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2007 Gerald Combs
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

#include "frequency-utils.h"

typedef struct freq_cvt_s {
    guint fmin;         /* Minimum frequency in MHz */
    guint fmax;         /* Maximum frequency in MHz */
    gint cmin;          /* Minimum/base channel */
    gboolean is_bg;     /* B/G channel? */
} freq_cvt_t;

#define FREQ_STEP 5     /* MHz. This seems to be consistent, thankfully */

/*
 * From IEEE Std 802.11-2012:
 *
 *    section 16.4.6.3 "Channel Numbering of operating channels";
 *
 *    section 17.4.6.3 "Channel Numbering of operating channels";
 *
 *    section 18.3.8.4.2 "Channel numbering";
 *
 *    Annex E.
 *
 * XXX - Japanese channels 182 through 196 actually have center
 * frequencies that are off by 2.5 MHz from these values, according
 * to the IEEE standard, although the table in ARIB STD T-71 version 5.2:
 *
 *     http://www.arib.or.jp/english/html/overview/doc/1-STD-T71v5_2.pdf
 *
 * section 5.3.8.3.3 doesn't show that.
 *
 * XXX - what about the U.S. public safety 4.9 GHz band?
 *
 * XXX - what about 802.11ad?
 */
static freq_cvt_t freq_cvt[] = {
    { 2412, 2472,   1, TRUE },
    { 2484, 2484,  14, TRUE },
    { 5000, 5995,   0, FALSE },
    { 4910, 4980, 182, FALSE }
};

#define NUM_FREQ_CVT (sizeof(freq_cvt) / sizeof(freq_cvt_t))
#define MAX_CHANNEL(fc) ( (gint) ((fc.fmax - fc.fmin) / FREQ_STEP) + fc.cmin )

/*
 * Get channel number given a Frequency
 */
gint
ieee80211_mhz_to_chan(guint freq) {
    guint i;

    for (i = 0; i < NUM_FREQ_CVT; i++) {
        if (freq >= freq_cvt[i].fmin && freq <= freq_cvt[i].fmax) {
            return ((freq - freq_cvt[i].fmin) / FREQ_STEP) + freq_cvt[i].cmin;
        }
    }
    return -1;
}

/*
 * Get Frequency given a Channel number
 */
guint
ieee80211_chan_to_mhz(gint chan, gboolean is_bg) {
    guint i;

    for (i = 0; i < NUM_FREQ_CVT; i++) {
        if (is_bg == freq_cvt[i].is_bg &&
                chan >= freq_cvt[i].cmin && chan <= MAX_CHANNEL(freq_cvt[i])) {
            return ((chan - freq_cvt[i].cmin) * FREQ_STEP) + freq_cvt[i].fmin;
        }
    }
    return 0;
}

/*
 * Get channel representation string given a Frequency
 */
gchar*
ieee80211_mhz_to_str(guint freq){
    gint chan = ieee80211_mhz_to_chan(freq);
    gboolean is_bg = FREQ_IS_BG(freq);

    if (chan < 0) {
        return g_strdup_printf("%u", freq);
    } else {
        return g_strdup_printf("%u [%s %u]", freq, is_bg ? "BG" : "A",
            chan);
    }
}

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=4 tabstop=8 expandtab:
 * :indentSize=4:tabSize=8:noTabs=true:
 */
