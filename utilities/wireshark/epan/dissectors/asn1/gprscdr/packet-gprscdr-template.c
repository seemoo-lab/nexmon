/* packet-gprscdr-template.c
 * Copyright 2011 , Anders Broman <anders.broman [AT] ericsson.com>
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
 * References: 3GPP TS 32.298
 */

#include "config.h"

#include <epan/packet.h>
#include <epan/expert.h>
#include <epan/asn1.h>

#include "packet-ber.h"
#include "packet-gsm_map.h"
#include "packet-e212.h"
#include "packet-gprscdr.h"

#define PNAME  "GPRS CDR"
#define PSNAME "GPRSCDR"
#define PFNAME "gprscdr"

void proto_register_gprscdr(void);

/* Define the GPRS CDR proto */
static int proto_gprscdr = -1;

#include "packet-gprscdr-hf.c"

static int ett_gprscdr = -1;
static int ett_gprscdr_timestamp = -1;
static int ett_gprscdr_plmn_id = -1;
static int ett_gprscdr_managementextension_information = -1;
#include "packet-gprscdr-ett.c"

static expert_field ei_gprscdr_not_dissected = EI_INIT;
static expert_field ei_gprscdr_choice_not_found = EI_INIT;

/* Global variables */
static const char *obj_id = NULL;

static const value_string gprscdr_daylight_saving_time_vals[] = {
    {0, "No adjustment"},
    {1, "+1 hour adjustment for Daylight Saving Time"},
    {2, "+2 hours adjustment for Daylight Saving Time"},
    {3, "Reserved"},
    {0, NULL}
};

#include "packet-gprscdr-fn.c"



/* Register all the bits needed with the filtering engine */
void
proto_register_gprscdr(void)
{
  /* List of fields */
  static hf_register_info hf[] = {
#include "packet-gprscdr-hfarr.c"
  };

  /* List of subtrees */
  static gint *ett[] = {
    &ett_gprscdr,
    &ett_gprscdr_timestamp,
    &ett_gprscdr_plmn_id,
    &ett_gprscdr_managementextension_information,
#include "packet-gprscdr-ettarr.c"
        };

  static ei_register_info ei[] = {
    { &ei_gprscdr_not_dissected, { "gprscdr.not_dissected", PI_UNDECODED, PI_WARN, "Not dissected", EXPFILL }},
    { &ei_gprscdr_choice_not_found, { "gprscdr.error.choice_not_found", PI_MALFORMED, PI_WARN, "GPRS CDR Error: This choice field(Record type) was not found", EXPFILL }},
  };

  expert_module_t* expert_gprscdr;

  proto_gprscdr = proto_register_protocol(PNAME, PSNAME, PFNAME);

  proto_register_field_array(proto_gprscdr, hf, array_length(hf));
  proto_register_subtree_array(ett, array_length(ett));
  expert_gprscdr = expert_register_protocol(proto_gprscdr);
  expert_register_field_array(expert_gprscdr, ei, array_length(ei));
}

/* The registration hand-off routine */

/*
 * Editor modelines
 *
 * Local Variables:
 * c-basic-offset: 2
 * tab-width: 8
 * indent-tabs-mode: nil
 * End:
 *
 * ex: set shiftwidth=2 tabstop=8 expandtab:
 * :indentSize=2:tabSize=8:noTabs=true:
 */
