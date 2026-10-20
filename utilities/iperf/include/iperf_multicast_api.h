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
 * multicast_join.h
 * pull out multicast join code for maitainability
 *
 * by Robert J. McMahon (rjmcmahon@rjmcmahon.com, bob.mcmahon@broadcom.com)
 * -------------------------------------------------------------------
 */
#ifndef MULTICASTJOIN_H
#define MULTICASTJOIN_H

#include "headers.h"

#ifdef __cplusplus
extern "C" {
#endif

enum McastJoinResponse {
    IPERF_MULTICAST_JOIN_SUCCESS = 1,
    IPERF_MULTICAST_JOIN_FAIL,
    IPERF_MULTICAST_JOIN_UNSUPPORTED
};

#define IPERF_MULTICAST_JOIN_SUCCESS 1

enum McastJoinResponse iperf_multicast_join (struct thread_Settings *inSettings);
bool iperf_multicast_sendif_v4 (struct thread_Settings *inSettings);
bool iperf_multicast_sendif_v6 (struct thread_Settings *inSettings);

#ifdef __cplusplus
} /* end extern "C" */
#endif

#endif // MULTICASTJOIN_H
