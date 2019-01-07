/* G711utable.h
 * Exponent table for mu-law G.711 codec
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

gint16 ulaw_exp_table[256] = {
   -32124,-31100,-30076,-29052,-28028,-27004,-25980,-24956,
   -23932,-22908,-21884,-20860,-19836,-18812,-17788,-16764,
   -15996,-15484,-14972,-14460,-13948,-13436,-12924,-12412,
   -11900,-11388,-10876,-10364, -9852, -9340, -8828, -8316,
    -7932, -7676, -7420, -7164, -6908, -6652, -6396, -6140,
    -5884, -5628, -5372, -5116, -4860, -4604, -4348, -4092,
    -3900, -3772, -3644, -3516, -3388, -3260, -3132, -3004,
    -2876, -2748, -2620, -2492, -2364, -2236, -2108, -1980,
    -1884, -1820, -1756, -1692, -1628, -1564, -1500, -1436,
    -1372, -1308, -1244, -1180, -1116, -1052,  -988,  -924,
     -876,  -844,  -812,  -780,  -748,  -716,  -684,  -652,
     -620,  -588,  -556,  -524,  -492,  -460,  -428,  -396,
     -372,  -356,  -340,  -324,  -308,  -292,  -276,  -260,
     -244,  -228,  -212,  -196,  -180,  -164,  -148,  -132,
     -120,  -112,  -104,   -96,   -88,   -80,   -72,   -64,
      -56,   -48,   -40,   -32,   -24,   -16,    -8,     0,
    32124, 31100, 30076, 29052, 28028, 27004, 25980, 24956,
    23932, 22908, 21884, 20860, 19836, 18812, 17788, 16764,
    15996, 15484, 14972, 14460, 13948, 13436, 12924, 12412,
    11900, 11388, 10876, 10364,  9852,  9340,  8828,  8316,
     7932,  7676,  7420,  7164,  6908,  6652,  6396,  6140,
     5884,  5628,  5372,  5116,  4860,  4604,  4348,  4092,
     3900,  3772,  3644,  3516,  3388,  3260,  3132,  3004,
     2876,  2748,  2620,  2492,  2364,  2236,  2108,  1980,
     1884,  1820,  1756,  1692,  1628,  1564,  1500,  1436,
     1372,  1308,  1244,  1180,  1116,  1052,   988,   924,
      876,   844,   812,   780,   748,   716,   684,   652,
      620,   588,   556,   524,   492,   460,   428,   396,
      372,   356,   340,   324,   308,   292,   276,   260,
      244,   228,   212,   196,   180,   164,   148,   132,
      120,   112,   104,    96,    88,    80,    72,    64,
       56,    48,    40,    32,    24,    16,     8,     0};

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
