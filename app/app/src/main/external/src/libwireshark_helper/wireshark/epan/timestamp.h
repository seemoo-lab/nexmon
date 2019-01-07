/* timestamp.h
 * Defines for packet timestamps
 *
 * $Id: timestamp.h 32683 2010-05-06 10:32:59Z stig $
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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

/*
 * Type of time-stamp shown in the summary display.
 */
typedef enum {
	TS_RELATIVE,            /* Since start of capture */
	TS_ABSOLUTE,
	TS_ABSOLUTE_WITH_DATE,
	TS_DELTA,               /* Since previous captured packet */
	TS_DELTA_DIS,           /* Since previous displayed packet */
	TS_EPOCH,               /* Seconds (and fractions) since epoch */

/*
 * Special value used for the command-line setting in Wireshark, to indicate
 * that no value has been set from the command line.
 */
	TS_NOT_SET
} ts_type;

typedef enum {
	TS_PREC_AUTO,		/* recent */
	TS_PREC_FIXED_SEC,	/* recent and internal */
	TS_PREC_FIXED_DSEC,	/* recent and internal */
	TS_PREC_FIXED_CSEC,	/* recent and internal */
	TS_PREC_FIXED_MSEC,	/* recent and internal */
	TS_PREC_FIXED_USEC,	/* recent and internal */
	TS_PREC_FIXED_NSEC,	/* recent and internal */
	TS_PREC_AUTO_SEC,	/* internal */
	TS_PREC_AUTO_DSEC,	/* internal */
	TS_PREC_AUTO_CSEC,	/* internal */
	TS_PREC_AUTO_MSEC,	/* internal */
	TS_PREC_AUTO_USEC,	/* internal */
	TS_PREC_AUTO_NSEC	/* internal */
} ts_precision;

typedef enum {
	TS_SECONDS_DEFAULT,	/* recent */
	TS_SECONDS_HOUR_MIN_SEC,/* recent */

/*
 * Special value used for the command-line setting in Wireshark, to indicate
 * that no value has been set from the command line.
 */
	TS_SECONDS_NOT_SET
} ts_seconds_type;

extern ts_type timestamp_get_type(void);
extern void timestamp_set_type(ts_type);

extern int timestamp_get_precision(void);
extern void timestamp_set_precision(int tsp);

extern ts_seconds_type timestamp_get_seconds_type(void);
extern void timestamp_set_seconds_type(ts_seconds_type);

#endif /* timestamp.h */
