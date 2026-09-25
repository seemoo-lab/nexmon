/*---------------------------------------------------------------
 * Copyright (c) 2023
 * Broadcom Corporation
 * All Rights Reserved.
 *---------------------------------------------------------------
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software
 * without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute,
 * sublicense, and/or sell copies of the Software, and to permit
 * persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 *
 * Redistributions of source code must retain the above
 * copyright notice, this list of conditions and
 * the following disclaimers.
 *
 *
 * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following
 * disclaimers in the documentation and/or other materials
 * provided with the distribution.
 *
 *
 * Neither the name of Broadcom Coporation,
 * nor the names of its contributors may be used to endorse
 * or promote products derived from this Software without
 * specific prior written permission.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE CONTIBUTORS OR COPYRIGHT
 * HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * ________________________________________________________________
 *
 * iperf_formattime
 * wrapper around strftime.c for iperf supported formatting
 *
 * by Robert J. McMahon (rjmcmahon@rjmcmahon.com, bob.mcmahon@broadcom.com)
 * -------------------------------------------------------------------
 */
#include "headers.h"
#include "util.h"
#include "iperf_formattime.h"

static inline void append_precision (char *timestr, int buflen, enum TimeFormatPrecision prec, int useconds) {
    if (prec != Seconds) {
	int currlen = strlen(timestr);
	if ((buflen - currlen) > 10) {
	    switch (prec) {
	    case Milliseconds:
		snprintf((timestr + currlen), 10, ".%03d", (useconds / 1000));
		break;
	    case Microseconds:
		snprintf((timestr + currlen), 10, ".%06d", useconds);
		break;
	    default:
		break;
	    }
	}
    }
}

inline void iperf_formattime (char *timestr, int buflen, struct timeval timestamp, \
			      enum TimeFormatPrecision prec, bool utc_time, enum TimeFormatType ftype) {
    if (buflen > 0) {
	struct tm ts ;
	time_t seconds = (time_t) timestamp.tv_sec;
	int useconds = (int) timestamp.tv_usec;
	ts = (utc_time ? *gmtime(&seconds) : *localtime(&seconds));
	switch (ftype) {
	case YearThruSec:
	    strftime(timestr, buflen, "%Y-%m-%d %H:%M:%S", &ts);
	    break;
	case YearThruSecTZ:
	    strftime(timestr, buflen, "%Y-%m-%d %H:%M:%S", &ts);
	    append_precision(timestr, buflen, prec, useconds);
	    int currlen = strlen(timestr);
	    if ((buflen - currlen) > 5) {
		strftime((timestr + currlen), (buflen - currlen), " (%Z)", &ts);
	    }
	    break;
	case CSV:
	    strftime(timestr, buflen, "%Y%m%d%H%M%S", &ts);
	    append_precision(timestr, buflen, prec, useconds);
	    break;
	case CSVTZ:
	    strftime(timestr, buflen, "%z:%Y%m%d%H%M%S", &ts);
	    append_precision(timestr, buflen, prec, useconds);
	    break;
	default:
	    FAIL_exit(1, "iperf_formattime program error");
	}
	timestr[buflen - 1] = '\0'; // make sure string is null terminated
    }
}
