/* i4btrace.h
 *
 * $Id: i4btrace.h 11400 2004-07-18 00:24:25Z guy $
 *
 * Wiretap Library
 * Copyright (c) 1999 by Bert Driehuis <driehuis@playbeing.org>
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

#ifndef __I4BTRACE_H__
#define __I4BTRACE_H__

int i4btrace_open(wtap *wth, int *err, gchar **err_info);

#endif
