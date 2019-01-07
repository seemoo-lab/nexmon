/* tnef.h
 *
 * Transport-Neutral Encapsulation Format (TNEF) file reading
 *
 * $Id: tnef.h 26864 2008-11-27 16:40:45Z stig $
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
 *
 */

#ifndef __TNEF_H__
#define __TNEF_H__

#define TNEF_SIGNATURE 0x223E9F78

int tnef_open(wtap *wth, int *err, gchar **err_info);

#endif
