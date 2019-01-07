/* packet-icmp.h
 * Definitions for ICMP: http://tools.ietf.org/html/rfc792.
 *
 * $Id: packet-icmp.h 36485 2011-04-05 23:26:56Z wmeier $
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

#ifndef __PACKET_ICMP_H__
#define __PACKET_ICMP_H__

/* ICMP echo request/reply transaction statistics ... used by ICMP tap(s) */
typedef struct _icmp_transaction_t {
    guint32 rqst_frame;
    guint32 resp_frame;
    nstime_t rqst_time;
    double resp_time;
} icmp_transaction_t;

#endif
