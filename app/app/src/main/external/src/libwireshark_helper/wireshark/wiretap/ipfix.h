/* ipfix.h
 *
 * $Id: ipfix.h 34589 2010-10-20 17:20:56Z wmeier $
 *
 * Wiretap Library
 * Copyright (c) 2010 by Hadriel Kaplan <hadrielk@yahoo.com>
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

#ifndef __W_IPFIX_H__
#define __W_IPFIX_H__

int ipfix_open(wtap *wth, int *err, gchar **err_info);

#endif
