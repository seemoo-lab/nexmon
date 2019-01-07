/* pint.h
 * Definitions for extracting and translating integers safely and portably
 * via pointers.
 *
 * $Id: pint.h 35538 2011-01-14 19:02:38Z cmaynard $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
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

#ifndef __PINT_H__
#define __PINT_H__

#include <glib.h>

/* Pointer versions of g_ntohs and g_ntohl.  Given a pointer to a member of a
 * byte array, returns the value of the two or four bytes at the pointer.
 * The pletoh[sl] versions return the little-endian representation.
 */

#define pntohs(p)   ((guint16)                       \
                     ((guint16)*((const guint8 *)(p)+0)<<8|  \
                      (guint16)*((const guint8 *)(p)+1)<<0))

#define pntoh24(p)  ((guint32)*((const guint8 *)(p)+0)<<16|  \
                     (guint32)*((const guint8 *)(p)+1)<<8|   \
                     (guint32)*((const guint8 *)(p)+2)<<0)

#define pntohl(p)   ((guint32)*((const guint8 *)(p)+0)<<24|  \
                     (guint32)*((const guint8 *)(p)+1)<<16|  \
                     (guint32)*((const guint8 *)(p)+2)<<8|   \
                     (guint32)*((const guint8 *)(p)+3)<<0)

#define pntoh40(p)  ((guint64)*((const guint8 *)(p)+0)<<32|  \
                     (guint64)*((const guint8 *)(p)+1)<<24|  \
                     (guint64)*((const guint8 *)(p)+2)<<16|  \
                     (guint64)*((const guint8 *)(p)+3)<<8|   \
                     (guint64)*((const guint8 *)(p)+4)<<0)

#define pntoh48(p)  ((guint64)*((const guint8 *)(p)+0)<<40|  \
                     (guint64)*((const guint8 *)(p)+1)<<32|  \
                     (guint64)*((const guint8 *)(p)+2)<<24|  \
                     (guint64)*((const guint8 *)(p)+3)<<16|  \
                     (guint64)*((const guint8 *)(p)+4)<<8|   \
                     (guint64)*((const guint8 *)(p)+5)<<0)

#define pntoh56(p)  ((guint64)*((const guint8 *)(p)+0)<<48|  \
                     (guint64)*((const guint8 *)(p)+1)<<40|  \
                     (guint64)*((const guint8 *)(p)+2)<<32|  \
                     (guint64)*((const guint8 *)(p)+3)<<24|  \
                     (guint64)*((const guint8 *)(p)+4)<<16|  \
                     (guint64)*((const guint8 *)(p)+5)<<8|   \
                     (guint64)*((const guint8 *)(p)+6)<<0)

#define pntoh64(p)  ((guint64)*((const guint8 *)(p)+0)<<56|  \
                     (guint64)*((const guint8 *)(p)+1)<<48|  \
                     (guint64)*((const guint8 *)(p)+2)<<40|  \
                     (guint64)*((const guint8 *)(p)+3)<<32|  \
                     (guint64)*((const guint8 *)(p)+4)<<24|  \
                     (guint64)*((const guint8 *)(p)+5)<<16|  \
                     (guint64)*((const guint8 *)(p)+6)<<8|   \
                     (guint64)*((const guint8 *)(p)+7)<<0)


#define pletohs(p)  ((guint16)                       \
                     ((guint16)*((const guint8 *)(p)+1)<<8|  \
                      (guint16)*((const guint8 *)(p)+0)<<0))

#define pletoh24(p) ((guint32)*((const guint8 *)(p)+2)<<16|  \
                     (guint32)*((const guint8 *)(p)+1)<<8|   \
                     (guint32)*((const guint8 *)(p)+0)<<0)

#define pletohl(p)  ((guint32)*((const guint8 *)(p)+3)<<24|  \
                     (guint32)*((const guint8 *)(p)+2)<<16|  \
                     (guint32)*((const guint8 *)(p)+1)<<8|   \
                     (guint32)*((const guint8 *)(p)+0)<<0)

#define pletoh40(p) ((guint64)*((const guint8 *)(p)+4)<<32|  \
                     (guint64)*((const guint8 *)(p)+3)<<24|  \
                     (guint64)*((const guint8 *)(p)+2)<<16|  \
                     (guint64)*((const guint8 *)(p)+1)<<8|   \
                     (guint64)*((const guint8 *)(p)+0)<<0)

#define pletoh48(p) ((guint64)*((const guint8 *)(p)+5)<<40|  \
                     (guint64)*((const guint8 *)(p)+4)<<32|  \
                     (guint64)*((const guint8 *)(p)+3)<<24|  \
                     (guint64)*((const guint8 *)(p)+2)<<16|  \
                     (guint64)*((const guint8 *)(p)+1)<<8|   \
                     (guint64)*((const guint8 *)(p)+0)<<0)

#define pletoh56(p) ((guint64)*((const guint8 *)(p)+6)<<48|  \
                     (guint64)*((const guint8 *)(p)+5)<<40|  \
                     (guint64)*((const guint8 *)(p)+4)<<32|  \
                     (guint64)*((const guint8 *)(p)+3)<<24|  \
                     (guint64)*((const guint8 *)(p)+2)<<16|  \
                     (guint64)*((const guint8 *)(p)+1)<<8|   \
                     (guint64)*((const guint8 *)(p)+0)<<0)

#define pletoh64(p) ((guint64)*((const guint8 *)(p)+7)<<56|  \
                     (guint64)*((const guint8 *)(p)+6)<<48|  \
                     (guint64)*((const guint8 *)(p)+5)<<40|  \
                     (guint64)*((const guint8 *)(p)+4)<<32|  \
                     (guint64)*((const guint8 *)(p)+3)<<24|  \
                     (guint64)*((const guint8 *)(p)+2)<<16|  \
                     (guint64)*((const guint8 *)(p)+1)<<8|   \
                     (guint64)*((const guint8 *)(p)+0)<<0)

/* Pointer routines to put items out in a particular byte order.
 * These will work regardless of the byte alignment of the pointer.
 */

#define phtons(p, v) \
	{ 				\
	((guint8*)(p))[0] = (guint8)((v) >> 8);	\
	((guint8*)(p))[1] = (guint8)((v) >> 0);	\
	}

#define phtonl(p, v) \
	{ 				\
	((guint8*)(p))[0] = (guint8)((v) >> 24);	\
	((guint8*)(p))[1] = (guint8)((v) >> 16);	\
	((guint8*)(p))[2] = (guint8)((v) >> 8);	\
	((guint8*)(p))[3] = (guint8)((v) >> 0);	\
	}


/* Macros to byte-swap 32-bit and 16-bit quantities. */
#define	BSWAP32(x) \
	((((x)&0xFF000000)>>24) | \
	 (((x)&0x00FF0000)>>8) | \
	 (((x)&0x0000FF00)<<8) | \
	 (((x)&0x000000FF)<<24))
#define	BSWAP16(x) \
	 ((((x)&0xFF00)>>8) | \
	  (((x)&0x00FF)<<8))

/* Turn host-byte-order values into little-endian values. */
#define htoles(s) GUINT16_TO_LE(s)
#define htolel(l) GUINT32_TO_LE(l)

#endif /* PINT_H */
