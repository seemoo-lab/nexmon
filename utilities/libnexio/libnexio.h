/***************************************************************************
 *                                                                         *
 * This file is part of NexMon.                                            *
 *                                                                         *
 * Copyright (c) 2016 NexMon Team                                          *
 *                                                                         *
 * NexMon is free software: you can redistribute it and/or modify          *
 * it under the terms of the GNU General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or       *
 * (at your option) any later version.                                     *
 *                                                                         *
 * NexMon is distributed in the hope that it will be useful,               *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of          *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License       *
 * along with NexMon. If not, see <http://www.gnu.org/licenses/>.          *
 *                                                                         *
 **************************************************************************/

#ifndef LIBNEXIO_H
#define LIBNEXIO_H

#include <stdbool.h>

/* struct nexio is opaque to callers: its full layout is private to
 * libnexio.c. Callers only ever hold a pointer returned by a nex_init_*
 * constructor and pass it back to nex_ioctl()/nex_free(). Keeping it opaque
 * avoids the divergent hand-rolled copies that previously lived in each
 * consumer and could read the wrong field offsets. */
struct nexio;

#ifdef __cplusplus
extern "C" {
#endif

int nex_ioctl(struct nexio *nexio, int cmd, void *buf, int len, bool set);

struct nexio *nex_init_ioctl(const char *ifname);
struct nexio *nex_init_udp(unsigned int securitycookie, unsigned int txip);
struct nexio *nex_init_netlink(void);
struct nexio *nex_init_netlink_ifname(const char *ifname);
#ifdef USE_VENDOR_CMD
struct nexio *nex_init_vendor_cmd(const char *ifname);
#endif

/* Release a handle returned by any nex_init_* constructor: closes its
 * sockets, frees its ifreq, and (vendor-cmd path) its netlink socket and
 * callback. Safe on NULL and on a partially-initialised handle. */
void nex_free(struct nexio *nexio);

#ifndef _bcmwifi_channels_h_
#ifndef _bcmwifi_channels_types_h_
typedef unsigned short chanspec_t;
#endif
#endif

#ifndef CHANSPEC_STR_LEN
#define CHANSPEC_STR_LEN 20
#endif

/* Chanspec parsing and conversion helpers */
chanspec_t wf_chspec_aton(const char *a);
char *wf_chspec_ntoa(chanspec_t chspec, char *buf);
char *wf_chspec_ntoa_ex(chanspec_t chspec, char *buf);
bool wf_chspec_is_6g_psc(chanspec_t chspec);

#ifdef __cplusplus
}
#endif

#endif /* LIBNEXIO_H */
