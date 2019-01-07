/* frequency-utils.h
 * Frequency conversion utility definitions
 *
 * $Id: frequency-utils.h 24538 2008-03-03 20:17:53Z gerald $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __FREQUENCY_UTILS_H__
#define __FREQUENCY_UTILS_H__

/** @file
 * Frequency and channel conversion utilities.
 */

/**
 * Given a center frequency in MHz, return a channel number.
 * @param freq Frequency in MHz.
 * @return The equivalent channel or -1 if no match is found.
 */
gint
ieee80211_mhz_to_chan(guint freq);

/**
 * Given a channel number and a band type, return a center frequency.
 * @param chan Channel number
 * @param is_bg TRUE if the channel is a b/g channel, FALSE otherwise.
 * @return The equivalent frequency or 0 if no match is found.
 */
guint
ieee80211_chan_to_mhz(gint chan, gboolean is_bg);

/**
 * Given a frequency in MHz, return a string representation.
 * @param freq Frequench in MHz.
 * @return A string showing the frequency, channel number, and type.  The string must be freed with g_free() after use.
 */
gchar*
ieee80211_mhz_to_str(guint freq);

/* Should this be "(freq < 4920)", or something else? */
#define FREQ_IS_BG(freq) (freq <= 2484)

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 4
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=4 tabstop=8 expandtab
 * :indentSize=4:tabSize=8:noTabs=true:
 */

#endif /* __FREQUENCY_UTILS_H__ */

