/* jpeg_jfif.h
 *
 * JPEG/JFIF file format decoder for the Wiretap library.
 * Written by Marton Nemeth <nm127@freemail.hu>
 * Copyright 2009 Marton Nemeth
 *
 * $Id: jpeg_jfif.h 30590 2009-10-18 08:02:28Z stig $
 *
 * Wiretap Library
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

#ifndef __W_JPEG_JFIF_H__
#define __W_JPEG_JFIF_H__

#include "wtap-int.h"

int jpeg_jfif_open(wtap *wth, int *err, gchar **err_info);

#endif
