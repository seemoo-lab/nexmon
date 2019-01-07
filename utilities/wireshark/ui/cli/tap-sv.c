/* tap-sv.c
 * Copyright 2008 Michael Bernhard
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */


#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <glib.h>

#include <epan/packet_info.h>
#include <epan/tap.h>
#include <epan/stat_tap_ui.h>
#include <epan/dissectors/packet-sv.h>

void register_tap_listener_sv(void);

static int
sv_packet(void *prs _U_, packet_info *pinfo, epan_dissect_t *edt _U_, const void *pri)
{
	int i;
	const sv_frame_data * sv_data = (const sv_frame_data *)pri;

	printf("%f %u ", nstime_to_sec(&pinfo->rel_ts), sv_data->smpCnt);

	for (i = 0; i < sv_data->num_phsMeas; i++) {
		printf("%d ", sv_data->phsMeas[i].value);
	}

	printf("\n");

	return 0;
}

static void
svstat_init(const char *opt_arg _U_, void *userdata _U_)
{
	GString	*error_string;

	error_string = register_tap_listener(
		"sv",
		NULL,
		NULL,
		0,
		NULL,
		sv_packet,
		NULL);
	if (error_string) {
		/* error, we failed to attach to the tap. clean up */
		fprintf(stderr, "tshark: Couldn't register sv,stat tap: %s\n",
				error_string->str);
		g_string_free(error_string, TRUE);
		exit(1);
	}
}

static stat_tap_ui svstat_ui = {
	REGISTER_STAT_GROUP_GENERIC,
	NULL,
	"sv",
	svstat_init,
	0,
	NULL
};

void
register_tap_listener_sv(void)
{
	register_stat_tap_ui(&svstat_ui, NULL);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
