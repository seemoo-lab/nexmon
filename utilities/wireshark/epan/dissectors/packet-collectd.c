/* packet-collectd.c
 * Routines for collectd (http://collectd.org/) network plugin dissection
 *
 * Copyright 2008 Bruno Premont <bonbons at linux-vserver.org>
 * Copyright 2009-2013 Florian Forster <octo at collectd.org>
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

#include <epan/packet.h>
#include <epan/prefs.h>
#include <epan/expert.h>
#include <epan/stats_tree.h>
#include <epan/to_str.h>

#include <wsutil/str_util.h>

#define STR_NONNULL(str) ((str) ? ((gchar*)str) : "(null)")

#define TYPE_HOST            0x0000
#define TYPE_TIME            0x0001
#define TYPE_TIME_HR         0x0008
#define TYPE_PLUGIN          0x0002
#define TYPE_PLUGIN_INSTANCE 0x0003
#define TYPE_TYPE            0x0004
#define TYPE_TYPE_INSTANCE   0x0005
#define TYPE_VALUES          0x0006
#define TYPE_INTERVAL        0x0007
#define TYPE_INTERVAL_HR     0x0009
#define TYPE_MESSAGE         0x0100
#define TYPE_SEVERITY        0x0101
#define TYPE_SIGN_SHA256     0x0200
#define TYPE_ENCR_AES256     0x0210

void proto_register_collectd(void);

typedef struct value_data_s {
	const guint8 *host;
	gint host_off;
	gint host_len;
	guint64 time_value;
	gint time_off;
	guint64 interval;
	gint interval_off;
	const guint8 *plugin;
	gint plugin_off;
	gint plugin_len;
	const guint8 *plugin_instance;
	gint plugin_instance_off;
	gint plugin_instance_len;
	const guint8 *type;
	gint type_off;
	gint type_len;
	const guint8 *type_instance;
	gint type_instance_off;
	gint type_instance_len;
} value_data_t;

typedef struct notify_data_s {
	const guint8 *host;
	gint host_off;
	gint host_len;
	guint64 time_value;
	gint time_off;
	guint64 severity;
	gint severity_off;
	const guint8 *message;
	gint message_off;
	gint message_len;
} notify_data_t;

struct string_counter_s;
typedef struct string_counter_s string_counter_t;
struct string_counter_s
{
	const gchar *string;
	gint   count;
	string_counter_t *next;
};

typedef struct tap_data_s {
	gint values_num;

	string_counter_t *hosts;
	string_counter_t *plugins;
	string_counter_t *types;
} tap_data_t;

static const value_string part_names[] = {
	{ TYPE_VALUES,          "VALUES" },
	{ TYPE_TIME,            "TIME" },
	{ TYPE_TIME_HR,         "TIME_HR" },
	{ TYPE_INTERVAL,        "INTERVAL" },
	{ TYPE_INTERVAL_HR,     "INTERVAL_HR" },
	{ TYPE_HOST,            "HOST" },
	{ TYPE_PLUGIN,          "PLUGIN" },
	{ TYPE_PLUGIN_INSTANCE, "PLUGIN_INSTANCE" },
	{ TYPE_TYPE,            "TYPE" },
	{ TYPE_TYPE_INSTANCE,   "TYPE_INSTANCE" },
	{ TYPE_MESSAGE,         "MESSAGE" },
	{ TYPE_SEVERITY,        "SEVERITY" },
	{ TYPE_SIGN_SHA256,     "SIGNATURE" },
	{ TYPE_ENCR_AES256,     "ENCRYPTED_DATA" },
	{ 0, NULL }
};

#define TYPE_VALUE_COUNTER  0x00
#define TYPE_VALUE_GAUGE    0x01
#define TYPE_VALUE_DERIVE   0x02
#define TYPE_VALUE_ABSOLUTE 0x03
static const value_string valuetypenames[] = {
	{ TYPE_VALUE_COUNTER,   "COUNTER" },
	{ TYPE_VALUE_GAUGE,     "GAUGE" },
	{ TYPE_VALUE_DERIVE,    "DERIVE" },
	{ TYPE_VALUE_ABSOLUTE,  "ABSOLUTE" },
	{ 0, NULL }
};

#define SEVERITY_FAILURE  0x01
#define SEVERITY_WARNING  0x02
#define SEVERITY_OKAY     0x04
static const val64_string severity_names[] = {
	{ SEVERITY_FAILURE,  "FAILURE" },
	{ SEVERITY_WARNING,  "WARNING" },
	{ SEVERITY_OKAY,     "OKAY" },
	{ 0, NULL }
};

#define UDP_PORT_COLLECTD 25826
static guint collectd_udp_port = UDP_PORT_COLLECTD;

static gint proto_collectd		= -1;
static gint tap_collectd                = -1;

static gint hf_collectd_type		= -1;
static gint hf_collectd_length		= -1;
static gint hf_collectd_data		= -1;
static gint hf_collectd_data_host	= -1;
static gint hf_collectd_data_time	= -1;
static gint hf_collectd_data_interval	= -1;
static gint hf_collectd_data_plugin	= -1;
static gint hf_collectd_data_plugin_inst= -1;
static gint hf_collectd_data_type	= -1;
static gint hf_collectd_data_type_inst	= -1;
static gint hf_collectd_data_valcnt	= -1;
static gint hf_collectd_val_type	= -1;
static gint hf_collectd_val_counter	= -1;
static gint hf_collectd_val_gauge	= -1;
static gint hf_collectd_val_derive	= -1;
static gint hf_collectd_val_absolute	= -1;
static gint hf_collectd_val_unknown	= -1;
static gint hf_collectd_data_severity	= -1;
static gint hf_collectd_data_message	= -1;
static gint hf_collectd_data_sighash    = -1;
static gint hf_collectd_data_initvec    = -1;
static gint hf_collectd_data_username_len = -1;
static gint hf_collectd_data_username   = -1;
static gint hf_collectd_data_encrypted  = -1;

static gint ett_collectd		= -1;
static gint ett_collectd_string		= -1;
static gint ett_collectd_integer	= -1;
static gint ett_collectd_part_value	= -1;
static gint ett_collectd_value		= -1;
static gint ett_collectd_valinfo	= -1;
static gint ett_collectd_signature	= -1;
static gint ett_collectd_encryption	= -1;
static gint ett_collectd_dispatch	= -1;
static gint ett_collectd_invalid_length	= -1;
static gint ett_collectd_unknown	= -1;

static gint st_collectd_packets = -1;
static gint st_collectd_values  = -1;
static gint st_collectd_values_hosts   = -1;
static gint st_collectd_values_plugins = -1;
static gint st_collectd_values_types   = -1;

static expert_field ei_collectd_type = EI_INIT;
static expert_field ei_collectd_invalid_length = EI_INIT;
static expert_field ei_collectd_data_valcnt = EI_INIT;
static expert_field ei_collectd_garbage = EI_INIT;

/* Prototype for the handoff function */
void proto_reg_handoff_collectd (void);

static nstime_t
collectd_time_to_nstime (guint64 t)
{
	nstime_t nstime = { 0, 0 };
	nstime.secs = (time_t) (t / 1073741824);
	nstime.nsecs = (int) (((double) (t % 1073741824)) / 1.073741824);

	return (nstime);
}

static void
collectd_stats_tree_init (stats_tree *st)
{
	st_collectd_packets = stats_tree_create_node (st, "Packets", 0, FALSE);
	st_collectd_values = stats_tree_create_node (st, "Values", 0, TRUE);

	st_collectd_values_hosts = stats_tree_create_pivot (st, "By host",
							   st_collectd_values);
	st_collectd_values_plugins = stats_tree_create_pivot (st, "By plugin",
							      st_collectd_values);
	st_collectd_values_types = stats_tree_create_pivot (st, "By type",
							    st_collectd_values);
} /* void collectd_stats_tree_init */

static int
collectd_stats_tree_packet (stats_tree *st, packet_info *pinfo _U_,
			    epan_dissect_t *edt _U_, const void *user_data)
{
	const tap_data_t *td;
	string_counter_t *sc;

	td = (const tap_data_t *)user_data;
	if (td == NULL)
		return (-1);

	tick_stat_node (st, "Packets", 0, FALSE);
	increase_stat_node (st, "Values", 0, TRUE, td->values_num);

	for (sc = td->hosts; sc != NULL; sc = sc->next)
	{
		gint i;
		for (i = 0; i < sc->count; i++)
			stats_tree_tick_pivot (st, st_collectd_values_hosts,
					       sc->string);
	}

	for (sc = td->plugins; sc != NULL; sc = sc->next)
	{
		gint i;
		for (i = 0; i < sc->count; i++)
			stats_tree_tick_pivot (st, st_collectd_values_plugins,
					       sc->string);
	}

	for (sc = td->types; sc != NULL; sc = sc->next)
	{
		gint i;
		for (i = 0; i < sc->count; i++)
			stats_tree_tick_pivot (st, st_collectd_values_types,
					       sc->string);
	}

	return (1);
} /* int collectd_stats_tree_packet */

static void
collectd_stats_tree_register (void)
{
	stats_tree_register ("collectd", "collectd", "Collectd", 0,
			     collectd_stats_tree_packet,
			     collectd_stats_tree_init, NULL);
} /* void register_collectd_stat_trees */

static void
collectd_proto_tree_add_assembled_metric (tvbuff_t *tvb,
		gint offset, gint length,
		value_data_t const *vdispatch, proto_tree *root)
{
	proto_item *root_item;
	proto_tree *subtree;
	nstime_t nstime;

	subtree = proto_tree_add_subtree(root, tvb, offset + 6, length - 6,
			ett_collectd_dispatch, &root_item, "Assembled metric");
	PROTO_ITEM_SET_GENERATED (root_item);

	proto_tree_add_string (subtree, hf_collectd_data_host, tvb,
			vdispatch->host_off, vdispatch->host_len,
			STR_NONNULL (vdispatch->host));

	proto_tree_add_string (subtree, hf_collectd_data_plugin, tvb,
			vdispatch->plugin_off, vdispatch->plugin_len,
			STR_NONNULL (vdispatch->plugin));

	if (vdispatch->plugin_instance)
		proto_tree_add_string (subtree,
				hf_collectd_data_plugin_inst, tvb,
				vdispatch->plugin_instance_off,
				vdispatch->plugin_instance_len,
				vdispatch->plugin_instance);

	proto_tree_add_string (subtree, hf_collectd_data_type, tvb,
			vdispatch->type_off, vdispatch->type_len,
			STR_NONNULL (vdispatch->type));

	if (vdispatch->type_instance)
		proto_tree_add_string (subtree,
				hf_collectd_data_type_inst, tvb,
				vdispatch->type_instance_off,
				vdispatch->type_instance_len,
				vdispatch->type_instance);

	nstime = collectd_time_to_nstime (vdispatch->time_value);
	proto_tree_add_time (subtree, hf_collectd_data_time, tvb,
			vdispatch->time_off, /* length = */ 8, &nstime);

	nstime = collectd_time_to_nstime (vdispatch->interval);
	proto_tree_add_time (subtree, hf_collectd_data_interval, tvb,
			vdispatch->interval_off, /* length = */ 8, &nstime);
}

static void
collectd_proto_tree_add_assembled_notification (tvbuff_t *tvb,
		gint offset, gint length,
		notify_data_t const *ndispatch, proto_tree *root)
{
	proto_item *root_item;
	proto_tree *subtree;
	nstime_t nstime;

	subtree = proto_tree_add_subtree(root, tvb, offset + 6, length - 6,
			ett_collectd_dispatch, &root_item, "Assembled notification");
	PROTO_ITEM_SET_GENERATED (root_item);

	proto_tree_add_string (subtree, hf_collectd_data_host, tvb,
			ndispatch->host_off, ndispatch->host_len,
			STR_NONNULL (ndispatch->host));

	nstime = collectd_time_to_nstime (ndispatch->time_value);
	proto_tree_add_time (subtree, hf_collectd_data_time, tvb,
			ndispatch->time_off, /* length = */ 8, &nstime);

	proto_tree_add_uint64 (subtree, hf_collectd_data_severity, tvb,
			ndispatch->severity_off, /* length = */ 8,
			ndispatch->severity);

	proto_tree_add_string (subtree, hf_collectd_data_message, tvb,
			ndispatch->message_off, ndispatch->message_len,
			ndispatch->message);
}

static int
dissect_collectd_string (tvbuff_t *tvb, packet_info *pinfo, gint type_hf,
			 gint offset, gint *ret_offset, gint *ret_length,
			 const guint8 **ret_string, proto_tree *tree_root,
			 proto_item **ret_item)
{
	proto_tree *pt;
	proto_item *pi;
	gint type;
	gint length;
	gint size;

	size = tvb_reported_length_remaining (tvb, offset);
	if (size < 4)
	{
		/* This should never happen, because `dissect_collectd' checks
		 * for this condition already. */
		return (-1);
	}

	type   = tvb_get_ntohs(tvb, offset);
	length = tvb_get_ntohs(tvb, offset + 2);

	pt = proto_tree_add_subtree_format(tree_root, tvb, offset, length,
				  ett_collectd_string, &pi, "collectd %s segment: ",
				  val_to_str_const (type, part_names, "UNKNOWN"));

	if (length > size)
	{
		proto_item_append_text(pt, "Length = %i <BAD>", length);
		expert_add_info_format(pinfo, pt, &ei_collectd_invalid_length,
					"String part with invalid part length: "
					"Part is longer than rest of package.");
		return (-1);
	}

	*ret_offset = offset + 4;
	*ret_length = length - 4;

	proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2, type);
	proto_tree_add_uint (pt, hf_collectd_length, tvb, offset + 2, 2, length);
	proto_tree_add_item_ret_string (pt, type_hf, tvb, *ret_offset, *ret_length, ENC_ASCII|ENC_NA, wmem_packet_scope(), ret_string);

	proto_item_append_text(pt, "\"%s\"", *ret_string);

	if (ret_item != NULL)
		*ret_item = pi;

	return (0);
} /* int dissect_collectd_string */

static int
dissect_collectd_integer (tvbuff_t *tvb, packet_info *pinfo, gint type_hf,
			  gint offset, gint *ret_offset, guint64 *ret_value,
			  proto_tree *tree_root, proto_item **ret_item)
{
	proto_tree *pt;
	proto_item *pi;
	gint type;
	gint length;
	gint size;

	size = tvb_reported_length_remaining (tvb, offset);
	if (size < 4)
	{
		/* This should never happen, because `dissect_collectd' checks
		 * for this condition already. */
		return (-1);
	}

	type   = tvb_get_ntohs(tvb, offset);
	length = tvb_get_ntohs(tvb, offset + 2);

	if (size < 12)
	{
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, -1,
					  ett_collectd_integer, NULL, "collectd %s segment: <BAD>",
					  val_to_str_const (type, part_names, "UNKNOWN"));

		proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2,
				     type);
		proto_tree_add_uint (pt, hf_collectd_length, tvb, offset + 2, 2,
				     length);
		proto_tree_add_expert_format(pt, pinfo, &ei_collectd_garbage, tvb, offset + 4, -1,
					  "Garbage at end of packet: Length = %i <BAD>",
					  size - 4);

		return (-1);
	}

	if (length != 12)
	{
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, -1,
					  ett_collectd_integer, &pi, "collectd %s segment: <BAD>",
					  val_to_str_const (type, part_names, "UNKNOWN"));

		proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2,
				     type);
		pi = proto_tree_add_uint (pt, hf_collectd_length, tvb,
					  offset + 2, 2, length);
		expert_add_info_format(pinfo, pi, &ei_collectd_invalid_length,
					"Invalid length field for an integer part.");

		return (-1);
	}

	*ret_offset = offset + 4;
	*ret_value = tvb_get_ntoh64 (tvb, offset + 4);

	/* Convert the version 4.* time format to the version 5.* time format. */
	if ((type == TYPE_TIME) || (type == TYPE_INTERVAL))
		*ret_value *= 1073741824;

	/* Create an entry in the protocol tree for this part. The value is
	 * printed depending on the "type" variable: TIME{,_HR} as absolute
	 * time, INTERVAL{,_HR} as relative time, uint64 otherwise. */
	if ((type == TYPE_TIME) || (type == TYPE_TIME_HR))
	{
		nstime_t nstime;
		gchar *strtime;

		nstime = collectd_time_to_nstime (*ret_value);
		strtime = abs_time_to_str (wmem_packet_scope(), &nstime, ABSOLUTE_TIME_LOCAL, /* show_zone = */ TRUE);
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, length,
					  ett_collectd_integer, &pi, "collectd %s segment: %s",
					  val_to_str_const (type, part_names, "UNKNOWN"),
					  STR_NONNULL (strtime));
	}
	else if ((type == TYPE_INTERVAL) || (type == TYPE_INTERVAL_HR))
	{
		nstime_t nstime;
		gchar *strtime;

		nstime = collectd_time_to_nstime (*ret_value);
		strtime = rel_time_to_str (wmem_packet_scope(), &nstime);
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, length,
					  ett_collectd_integer, &pi, "collectd %s segment: %s",
					  val_to_str_const (type, part_names, "UNKNOWN"),
					  strtime);
	}
	else
	{
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, length,
					  ett_collectd_integer, &pi, "collectd %s segment: %"G_GINT64_MODIFIER"u",
					  val_to_str_const (type, part_names, "UNKNOWN"),
					  *ret_value);
	}

	if (ret_item != NULL)
		*ret_item = pi;

	proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2, type);
	proto_tree_add_uint (pt, hf_collectd_length, tvb, offset + 2, 2,
			     length);
	if ((type == TYPE_TIME) || (type == TYPE_INTERVAL)
	    || (type == TYPE_TIME_HR) || (type == TYPE_INTERVAL_HR))
	{
		nstime_t nstime;

		nstime = collectd_time_to_nstime (*ret_value);
		proto_tree_add_time (pt, type_hf, tvb, offset + 4, 8, &nstime);
	}
	else
	{
		proto_tree_add_item (pt, type_hf, tvb, offset + 4, 8, ENC_BIG_ENDIAN);
	}

	return (0);
} /* int dissect_collectd_integer */

static void
dissect_collectd_values(tvbuff_t *tvb, gint msg_off, gint val_cnt,
			proto_tree *collectd_tree)
{
	proto_tree *values_tree, *value_tree;
	gint i;

	values_tree = proto_tree_add_subtree_format(collectd_tree, tvb, msg_off + 6, val_cnt * 9,
				  ett_collectd_value, NULL, "%d value%s", val_cnt,
				  plurality (val_cnt, "", "s"));

	for (i = 0; i < val_cnt; i++)
	{
		gint value_offset;

		gint value_type_offset;
		guint8 value_type;

		/* Calculate the offsets of the type byte and the actual value. */
		value_offset = msg_off + 6
				+ val_cnt  /* value types */
				+ (i * 8); /* previous values */

		value_type_offset = msg_off + 6 + i;
		value_type = tvb_get_guint8 (tvb, value_type_offset);

		switch (value_type) {
		case TYPE_VALUE_COUNTER:
		{
			guint64 val64;

			val64 = tvb_get_ntoh64 (tvb, value_offset);
			value_tree = proto_tree_add_subtree_format(values_tree, tvb, msg_off + 6,
						  val_cnt * 9, ett_collectd_valinfo, NULL,
						  "Counter: %"G_GINT64_MODIFIER"u", val64);

			proto_tree_add_item (value_tree, hf_collectd_val_type,
					     tvb, value_type_offset, 1, ENC_BIG_ENDIAN);
			proto_tree_add_item (value_tree,
					     hf_collectd_val_counter, tvb,
					     value_offset, 8, ENC_BIG_ENDIAN);
			break;
		}

		case TYPE_VALUE_GAUGE:
		{
			gdouble val;

			val = tvb_get_letohieee_double (tvb, value_offset);
			value_tree = proto_tree_add_subtree_format(values_tree, tvb, msg_off + 6,
						  val_cnt * 9, ett_collectd_valinfo, NULL,
						  "Gauge: %g", val);

			proto_tree_add_item (value_tree, hf_collectd_val_type,
					     tvb, value_type_offset, 1, ENC_BIG_ENDIAN);
			/* Set the `little endian' flag to TRUE here, because
			 * collectd stores doubles in x86 representation. */
			proto_tree_add_item (value_tree, hf_collectd_val_gauge,
					     tvb, value_offset, 8, ENC_LITTLE_ENDIAN);
			break;
		}

		case TYPE_VALUE_DERIVE:
		{
			gint64 val64;

			val64 = tvb_get_ntoh64 (tvb, value_offset);
			value_tree = proto_tree_add_subtree_format(values_tree, tvb, msg_off + 6,
						  val_cnt * 9, ett_collectd_valinfo, NULL,
						  "Derive: %"G_GINT64_MODIFIER"i", val64);

			proto_tree_add_item (value_tree, hf_collectd_val_type,
					     tvb, value_type_offset, 1, ENC_BIG_ENDIAN);
			proto_tree_add_item (value_tree,
					     hf_collectd_val_derive, tvb,
					     value_offset, 8, ENC_BIG_ENDIAN);
			break;
		}

		case TYPE_VALUE_ABSOLUTE:
		{
			guint64 val64;

			val64 = tvb_get_ntoh64 (tvb, value_offset);
			value_tree = proto_tree_add_subtree_format(values_tree, tvb, msg_off + 6,
						  val_cnt * 9, ett_collectd_valinfo, NULL,
						  "Absolute: %"G_GINT64_MODIFIER"u", val64);

			proto_tree_add_item (value_tree, hf_collectd_val_type,
					     tvb, value_type_offset, 1, ENC_BIG_ENDIAN);
			proto_tree_add_item (value_tree,
					     hf_collectd_val_absolute, tvb,
					     value_offset, 8, ENC_BIG_ENDIAN);
			break;
		}

		default:
		{
			guint64 val64;

			val64 = tvb_get_ntoh64 (tvb, value_offset);
			value_tree = proto_tree_add_subtree_format(values_tree, tvb, msg_off + 6,
						  val_cnt * 9, ett_collectd_valinfo, NULL,
						  "Unknown: %"G_GINT64_MODIFIER"x",
						  val64);

			proto_tree_add_item (value_tree, hf_collectd_val_type,
					     tvb, value_type_offset, 1, ENC_BIG_ENDIAN);
			proto_tree_add_item (value_tree, hf_collectd_val_unknown,
					     tvb, value_offset, 8, ENC_BIG_ENDIAN);
			break;
		}
		} /* switch (value_type) */
	} /* for (i = 0; i < val_cnt; i++) */
} /* void dissect_collectd_values */

static int
dissect_collectd_part_values (tvbuff_t *tvb, packet_info *pinfo, gint offset,
			      value_data_t *vdispatch, proto_tree *tree_root)
{
	proto_tree *pt;
	proto_item *pi;
	gint type;
	gint length;
	gint size;
	gint values_count;
	gint corrected_values_count;

	size = tvb_reported_length_remaining (tvb, offset);
	if (size < 4)
	{
		/* This should never happen, because `dissect_collectd' checks
		 * for this condition already. */
		return (-1);
	}

	type   = tvb_get_ntohs (tvb, offset);
	length = tvb_get_ntohs (tvb, offset + 2);

	if (size < 15)
	{
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, -1,
					  ett_collectd_part_value, NULL, "collectd %s segment: <BAD>",
					  val_to_str_const (type, part_names, "UNKNOWN"));

		proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2, type);
		proto_tree_add_uint (pt, hf_collectd_length, tvb, offset + 2, 2,
				     length);
		proto_tree_add_expert_format(pt, pinfo, &ei_collectd_garbage, tvb, offset + 4, -1,
					  "Garbage at end of packet: Length = %i <BAD>",
					  size - 4);
		return (-1);
	}

	if ((length < 15) || ((length % 9) != 6))
	{
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, -1,
					  ett_collectd_part_value, &pi, "collectd %s segment: <BAD>",
					  val_to_str_const (type, part_names, "UNKNOWN"));

		proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2, type);
		pi = proto_tree_add_uint (pt, hf_collectd_length, tvb,
					  offset + 2, 2, length);
		expert_add_info_format(pinfo, pi, &ei_collectd_invalid_length,
					"Invalid length field for a values part.");

		return (-1);
	}

	values_count = tvb_get_ntohs (tvb, offset + 4);
	corrected_values_count = (length - 6) / 9;

	if (values_count != corrected_values_count)
	{
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, length,
		      ett_collectd_part_value, NULL,
					  "collectd %s segment: %d (%d) value%s <BAD>",
					  val_to_str_const (type, part_names, "UNKNOWN"),
					  values_count, corrected_values_count,
					  plurality(values_count, "", "s"));
	}
	else
	{
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, length,
		      ett_collectd_part_value, NULL,
					  "collectd %s segment: %d value%s",
					  val_to_str_const (type, part_names, "UNKNOWN"),
					  values_count,
					  plurality(values_count, "", "s"));
	}

	proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2, type);
	proto_tree_add_uint (pt, hf_collectd_length, tvb, offset + 2, 2, length);

	pi = proto_tree_add_item (pt, hf_collectd_data_valcnt, tvb,
				  offset + 4, 2, ENC_BIG_ENDIAN);
	if (values_count != corrected_values_count)
		expert_add_info(pinfo, pi, &ei_collectd_data_valcnt);

	values_count = corrected_values_count;

	dissect_collectd_values (tvb, offset, values_count, pt);
	collectd_proto_tree_add_assembled_metric (tvb, offset + 6, length - 6,
			vdispatch, pt);

	return (0);
} /* void dissect_collectd_part_values */

static int
dissect_collectd_signature (tvbuff_t *tvb, packet_info *pinfo,
			    gint offset, proto_tree *tree_root)
{
	proto_item *pi;
	proto_tree *pt;
	gint type;
	gint length;
	gint size;

	size = tvb_reported_length_remaining (tvb, offset);
	if (size < 4)
	{
		/* This should never happen, because `dissect_collectd' checks
		 * for this condition already. */
		return (-1);
	}

	type   = tvb_get_ntohs (tvb, offset);
	length = tvb_get_ntohs (tvb, offset + 2);

	if (size < 36) /* remaining packet size too small for signature */
	{
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, -1,
					  ett_collectd_signature, NULL, "collectd %s segment: <BAD>",
					  val_to_str_const (type, part_names, "UNKNOWN"));

		proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2, type);
		proto_tree_add_uint (pt, hf_collectd_length, tvb, offset + 2, 2,
				     length);
		proto_tree_add_expert_format(pt, pinfo, &ei_collectd_garbage, tvb, offset + 4, -1,
					  "Garbage at end of packet: Length = %i <BAD>",
					  size - 4);
		return (-1);
	}

	if (length < 36)
	{
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, -1,
					  ett_collectd_signature, NULL, "collectd %s segment: <BAD>",
					  val_to_str_const (type, part_names, "UNKNOWN"));

		proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2, type);
		pi = proto_tree_add_uint (pt, hf_collectd_length, tvb,
					  offset + 2, 2, length);
		expert_add_info_format(pinfo, pi, &ei_collectd_invalid_length,
					"Invalid length field for a signature part.");

		return (-1);
	}

	pt = proto_tree_add_subtree_format(tree_root, tvb, offset, length,
				  ett_collectd_signature, NULL, "collectd %s segment: HMAC-SHA-256",
				  val_to_str_const (type, part_names, "UNKNOWN"));

	proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2, type);
	proto_tree_add_uint (pt, hf_collectd_length, tvb, offset + 2, 2,
			     length);
	proto_tree_add_item (pt, hf_collectd_data_sighash, tvb, offset + 4, 32, ENC_NA);
	proto_tree_add_item (pt, hf_collectd_data_username, tvb, offset + 36, length - 36, ENC_ASCII|ENC_NA);

	return (0);
} /* int dissect_collectd_signature */

static int
dissect_collectd_encrypted (tvbuff_t *tvb, packet_info *pinfo,
			    gint offset, proto_tree *tree_root)
{
	proto_item *pi;
	proto_tree *pt;
	gint type;
	gint length;
	gint size;
	gint username_length;

	size = tvb_reported_length_remaining (tvb, offset);
	if (size < 4)
	{
		/* This should never happen, because `dissect_collectd' checks
		 * for this condition already. */
		return (-1);
	}

	type   = tvb_get_ntohs (tvb, offset);
	length = tvb_get_ntohs (tvb, offset + 2);

	if (size < 42) /* remaining packet size too small for signature */
	{
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, -1,
					  ett_collectd_encryption, NULL, "collectd %s segment: <BAD>",
					  val_to_str_const (type, part_names, "UNKNOWN"));

		proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2, type);
		proto_tree_add_uint (pt, hf_collectd_length, tvb, offset + 2, 2,
				     length);
		proto_tree_add_expert_format(pt, pinfo, &ei_collectd_garbage, tvb, offset + 4, -1,
					  "Garbage at end of packet: Length = %i <BAD>",
					  size - 4);
		return (-1);
	}

	if (length < 42)
	{
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, -1,
					  ett_collectd_encryption, NULL, "collectd %s segment: <BAD>",
					  val_to_str_const (type, part_names, "UNKNOWN"));

		proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2, type);
		pi = proto_tree_add_uint (pt, hf_collectd_length, tvb,
					  offset + 2, 2, length);
		expert_add_info_format(pinfo, pi, &ei_collectd_invalid_length,
					"Invalid length field for an encryption part.");

		return (-1);
	}

	username_length = tvb_get_ntohs (tvb, offset + 4);
	if (username_length > (length - 42))
	{
		pt = proto_tree_add_subtree_format(tree_root, tvb, offset, -1,
					  ett_collectd_encryption, NULL, "collectd %s segment: <BAD>",
					  val_to_str_const (type, part_names, "UNKNOWN"));

		proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2, type);
		proto_tree_add_uint (pt, hf_collectd_length, tvb,
				     offset + 2, 2, length);
		pi = proto_tree_add_uint (pt, hf_collectd_data_username_len, tvb,
					  offset + 4, 2, length);
		expert_add_info_format(pinfo, pi, &ei_collectd_invalid_length,
					"Invalid username length field for an encryption part.");

		return (-1);
	}

	pt = proto_tree_add_subtree_format(tree_root, tvb, offset, length,
				  ett_collectd_encryption, NULL, "collectd %s segment: AES-256",
				  val_to_str_const (type, part_names, "UNKNOWN"));

	proto_tree_add_uint (pt, hf_collectd_type, tvb, offset, 2, type);
	proto_tree_add_uint (pt, hf_collectd_length, tvb, offset + 2, 2, length);
	proto_tree_add_uint (pt, hf_collectd_data_username_len, tvb, offset + 4, 2, username_length);
	proto_tree_add_item (pt, hf_collectd_data_username, tvb, offset + 6, username_length, ENC_ASCII|ENC_NA);
	proto_tree_add_item (pt, hf_collectd_data_initvec, tvb,
			     offset + (6 + username_length), 16, ENC_NA);
	proto_tree_add_item (pt, hf_collectd_data_encrypted, tvb,
			     offset + (22 + username_length),
			     length - (22 + username_length), ENC_NA);

	return (0);
} /* int dissect_collectd_encrypted */

static int
stats_account_string (string_counter_t **ret_list, const gchar *new_value)
{
	string_counter_t *entry;

	if (ret_list == NULL)
		return (-1);

	if (new_value == NULL)
		new_value = "(null)";

	for (entry = *ret_list; entry != NULL; entry = entry->next)
		if (strcmp (new_value, entry->string) == 0)
		{
			entry->count++;
			return (0);
		}

	entry = (string_counter_t *)wmem_alloc0 (wmem_packet_scope(), sizeof (*entry));
	entry->string = wmem_strdup (wmem_packet_scope(), new_value);
	entry->count = 1;
	entry->next = *ret_list;

	*ret_list = entry;

	return (0);
}

static int
dissect_collectd (tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree, void* data _U_)
{
	static tap_data_t tap_data;

	gint offset;
	gint size;
	const guint8 *pkt_host = NULL;
	gint pkt_plugins = 0, pkt_values = 0, pkt_messages = 0, pkt_unknown = 0, pkt_errors = 0;
	value_data_t vdispatch;
	notify_data_t ndispatch;
	int status;
	proto_item *pi;
	proto_tree *collectd_tree;
	proto_tree *pt;

	memset(&vdispatch, '\0', sizeof(vdispatch));
	memset(&ndispatch, '\0', sizeof(ndispatch));

	col_set_str(pinfo->cinfo, COL_PROTOCOL, "collectd");
	col_clear(pinfo->cinfo, COL_INFO);

	offset = 0;
	size = tvb_reported_length(tvb);

	/* create the collectd protocol tree */
	pi = proto_tree_add_item(tree, proto_collectd, tvb, 0, -1, ENC_NA);
	collectd_tree = proto_item_add_subtree(pi, ett_collectd);

	memset (&tap_data, 0, sizeof (tap_data));

	status = 0;
	while ((size > 0) && (status == 0))
	{

		gint part_type;
		gint part_length;

		/* Let's handle the easy case first real quick: All we do here
		 * is extract a host name and count the number of values,
		 * plugins and notifications. The payload is not checked at
		 * all, but the same checks are run on the part_length stuff -
		 * it's important to keep an eye on that. */
		if (!tree)
		{
			/* Check for garbage at end of packet. */
			if (size < 4)
			{
				pkt_errors++;
				break;
			}

			part_type = tvb_get_ntohs (tvb, offset);
			part_length  = tvb_get_ntohs (tvb, offset+2);

			/* Check if part_length is in the valid range. */
			if ((part_length < 4) || (part_length > size))
			{
				pkt_errors++;
				break;
			}

			switch (part_type) {
			case TYPE_HOST:
				vdispatch.host = tvb_get_string_enc(wmem_packet_scope(), tvb,
						offset + 4, part_length - 4, ENC_ASCII);
				if (pkt_host == NULL)
					pkt_host = vdispatch.host;
				break;
			case TYPE_TIME:
			case TYPE_TIME_HR:
				break;
			case TYPE_PLUGIN:
				vdispatch.plugin = tvb_get_string_enc(wmem_packet_scope(), tvb,
						offset + 4, part_length - 4, ENC_ASCII);
				pkt_plugins++;
				break;
			case TYPE_PLUGIN_INSTANCE:
				break;
			case TYPE_TYPE:
				vdispatch.type = tvb_get_string_enc(wmem_packet_scope(), tvb,
						offset + 4, part_length - 4, ENC_ASCII);
				break;
			case TYPE_TYPE_INSTANCE:
				break;
			case TYPE_INTERVAL:
			case TYPE_INTERVAL_HR:
				break;
			case TYPE_VALUES:
			{
				pkt_values++;

				tap_data.values_num++;
				stats_account_string (&tap_data.hosts,
						      vdispatch.host);
				stats_account_string (&tap_data.plugins,
						      vdispatch.plugin);
				stats_account_string (&tap_data.types,
						      vdispatch.type);

				break;
			}
			case TYPE_MESSAGE:
				pkt_messages++;
				break;
			case TYPE_SEVERITY:
				break;
			default:
				pkt_unknown++;
			}

			offset  += part_length;
			size    -= part_length;
			continue;
		} /* if (!tree) */

		/* Now we do the same steps again, but much more thoroughly. */

		/* Check if there are at least four bytes left first.
		 * Four bytes are used to read the type and the length
		 * of the next part. If there's less, there's some garbage
		 * at the end of the packet. */
		if (size < 4)
		{
			proto_tree_add_expert_format(pi, pinfo, &ei_collectd_garbage, tvb,
						  offset, -1,
						  "Garbage at end of packet: Length = %i <BAD>",
						  size);
			pkt_errors++;
			break;
		}

		/* dissect a message entry */
		part_type = tvb_get_ntohs (tvb, offset);
		part_length  = tvb_get_ntohs (tvb, offset + 2);

		/* Check if the length of the part is in the valid range. Don't
		 * confuse this with the above: Here we check the information
		 * provided in the packet.. */
		if ((part_length < 4) || (part_length > size))
		{
			pt = proto_tree_add_subtree_format(collectd_tree, tvb,
						  offset, part_length, ett_collectd_invalid_length, NULL,
						  "collectd %s segment: Length = %i <BAD>",
						  val_to_str_const (part_type, part_names, "UNKNOWN"),
						  part_length);

			proto_tree_add_uint (pt, hf_collectd_type, tvb, offset,
					     2, part_type);
			pi = proto_tree_add_uint (pt, hf_collectd_length, tvb,
					     offset + 2, 2, part_length);

			if (part_length < 4)
				expert_add_info_format(pinfo, pi, &ei_collectd_invalid_length,
							"Bad part length: Is %i, expected at least 4",
							part_length);
			else
				expert_add_info_format(pinfo, pi, &ei_collectd_invalid_length,
							"Bad part length: Larger than remaining packet size.");

			pkt_errors++;
			break;
		}

		/* The header information looks okay, let's tend to the actual
		 * payload in this part. */
		switch (part_type) {
		case TYPE_HOST:
		{
			status = dissect_collectd_string (tvb, pinfo,
					hf_collectd_data_host,
					offset,
					&vdispatch.host_off,
					&vdispatch.host_len,
					&vdispatch.host,
					collectd_tree, /* item = */ NULL);
			if (status != 0)
				pkt_errors++;
			else
			{
				if (pkt_host == NULL)
					pkt_host = vdispatch.host;
				ndispatch.host_off = vdispatch.host_off;
				ndispatch.host_len = vdispatch.host_len;
				ndispatch.host = vdispatch.host;
			}

			break;
		}

		case TYPE_PLUGIN:
		{
			status = dissect_collectd_string (tvb, pinfo,
					hf_collectd_data_plugin,
					offset,
					&vdispatch.plugin_off,
					&vdispatch.plugin_len,
					&vdispatch.plugin,
					collectd_tree, /* item = */ NULL);
			if (status != 0)
				pkt_errors++;
			else
				pkt_plugins++;

			break;
		}

		case TYPE_PLUGIN_INSTANCE:
		{
			status = dissect_collectd_string (tvb, pinfo,
					hf_collectd_data_plugin_inst,
					offset,
					&vdispatch.plugin_instance_off,
					&vdispatch.plugin_instance_len,
					&vdispatch.plugin_instance,
					collectd_tree, /* item = */ NULL);
			if (status != 0)
				pkt_errors++;

			break;
		}

		case TYPE_TYPE:
		{
			status = dissect_collectd_string (tvb, pinfo,
					hf_collectd_data_type,
					offset,
					&vdispatch.type_off,
					&vdispatch.type_len,
					&vdispatch.type,
					collectd_tree, /* item = */ NULL);
			if (status != 0)
				pkt_errors++;

			break;
		}

		case TYPE_TYPE_INSTANCE:
		{
			status = dissect_collectd_string (tvb, pinfo,
					hf_collectd_data_type_inst,
					offset,
					&vdispatch.type_instance_off,
					&vdispatch.type_instance_len,
					&vdispatch.type_instance,
					collectd_tree, /* item = */ NULL);
			if (status != 0)
				pkt_errors++;

			break;
		}

		case TYPE_TIME:
		case TYPE_TIME_HR:
		{
			pi = NULL;
			status = dissect_collectd_integer (tvb, pinfo,
					hf_collectd_data_time,
					offset,
					&vdispatch.time_off,
					&vdispatch.time_value,
					collectd_tree, &pi);
			if (status != 0)
				pkt_errors++;

			break;
		}

		case TYPE_INTERVAL:
		case TYPE_INTERVAL_HR:
		{
			status = dissect_collectd_integer (tvb, pinfo,
					hf_collectd_data_interval,
					offset,
					&vdispatch.interval_off,
					&vdispatch.interval,
					collectd_tree, /* item = */ NULL);
			if (status != 0)
				pkt_errors++;

			break;
		}

		case TYPE_VALUES:
		{
			status = dissect_collectd_part_values (tvb, pinfo,
					offset,
					&vdispatch,
					collectd_tree);
			if (status != 0)
				pkt_errors++;
			else
				pkt_values++;

			tap_data.values_num++;
			stats_account_string (&tap_data.hosts,
					      vdispatch.host);
			stats_account_string (&tap_data.plugins,
					      vdispatch.plugin);
			stats_account_string (&tap_data.types,
					      vdispatch.type);

			break;
		}

		case TYPE_MESSAGE:
		{
			pi = NULL;
			status = dissect_collectd_string (tvb, pinfo,
					hf_collectd_data_message,
					offset,
					&ndispatch.message_off,
					&ndispatch.message_len,
					&ndispatch.message,
					collectd_tree, &pi);
			if (status != 0)
			{
				pkt_errors++;
				break;
			}
			pkt_messages++;

			pt = proto_item_get_subtree (pi);

			collectd_proto_tree_add_assembled_notification (tvb,
					offset + 4, part_length - 1,
					&ndispatch, pt);

			break;
		}

		case TYPE_SEVERITY:
		{
			pi = NULL;
			status = dissect_collectd_integer (tvb, pinfo,
					hf_collectd_data_severity,
					offset,
					&ndispatch.severity_off,
					&ndispatch.severity,
					collectd_tree, &pi);
			if (status != 0)
				pkt_errors++;
			else
			{
				proto_item_set_text (pi,
						"collectd SEVERITY segment: "
						"%s (%"G_GINT64_MODIFIER"u)",
						val64_to_str_const (ndispatch.severity, severity_names, "UNKNOWN"),
						ndispatch.severity);
			}

			break;
		}

		case TYPE_SIGN_SHA256:
		{
			status = dissect_collectd_signature (tvb, pinfo,
							     offset,
							     collectd_tree);
			if (status != 0)
				pkt_errors++;

			break;
		}

		case TYPE_ENCR_AES256:
		{
			status = dissect_collectd_encrypted (tvb, pinfo,
					offset, collectd_tree);
			if (status != 0)
				pkt_errors++;

			break;
		}

		default:
		{
			pkt_unknown++;
			pt = proto_tree_add_subtree_format(collectd_tree, tvb,
						  offset, part_length, ett_collectd_unknown, NULL,
						  "collectd %s segment: %i bytes",
						  val_to_str_const(part_type, part_names, "UNKNOWN"),
						  part_length);

			pi = proto_tree_add_uint (pt, hf_collectd_type, tvb,
						  offset, 2, part_type);
			proto_tree_add_uint (pt, hf_collectd_length, tvb,
						  offset + 2, 2, part_length);
			proto_tree_add_item (pt, hf_collectd_data, tvb,
					     offset + 4, part_length - 4, ENC_NA);

			expert_add_info_format(pinfo, pi, &ei_collectd_type,
						"Unknown part type %#x. Cannot decode data.",
						part_type);
		}
		} /* switch (part_type) */

		offset  += part_length;
		size    -= part_length;
	} /* while ((size > 4) && (status == 0)) */

	if (pkt_errors && pkt_unknown)
		col_add_fstr (pinfo->cinfo, COL_INFO,
			      "Host=%s, %2d value%s for %d plugin%s %d message%s %d unknown, %d error%s",
			      pkt_host,
			      pkt_values, plurality (pkt_values, " ", "s"),
			      pkt_plugins, plurality (pkt_plugins, ", ", "s,"),
			      pkt_messages, plurality (pkt_messages, ", ", "s,"),
			      pkt_unknown,
			      pkt_errors, plurality (pkt_errors, "", "s"));
	else if (pkt_errors)
		col_add_fstr (pinfo->cinfo, COL_INFO, "Host=%s, %2d value%s for %d plugin%s %d message%s %d error%s",
			      pkt_host,
			      pkt_values, plurality (pkt_values, " ", "s"),
			      pkt_plugins, plurality (pkt_plugins, ", ", "s,"),
			      pkt_messages, plurality (pkt_messages, ", ", "s,"),
			      pkt_errors, plurality (pkt_errors, "", "s"));
	else if (pkt_unknown)
		col_add_fstr (pinfo->cinfo, COL_INFO,
			      "Host=%s, %2d value%s for %d plugin%s %d message%s %d unknown",
			      pkt_host,
			      pkt_values, plurality (pkt_values, " ", "s"),
			      pkt_plugins, plurality (pkt_plugins, ", ", "s,"),
			      pkt_messages, plurality (pkt_messages, ", ", "s,"),
			      pkt_unknown);
	else
		col_add_fstr (pinfo->cinfo, COL_INFO, "Host=%s, %2d value%s for %d plugin%s %d message%s",
			      pkt_host,
			      pkt_values, plurality (pkt_values, " ", "s"),
			      pkt_plugins, plurality (pkt_plugins, ", ", "s,"),
			      pkt_messages, plurality (pkt_messages, "", "s"));

	/* Dispatch tap data. */
	tap_queue_packet (tap_collectd, pinfo, &tap_data);
	return tvb_captured_length(tvb);
} /* void dissect_collectd */

void proto_register_collectd(void)
{
	module_t *collectd_module;
	expert_module_t* expert_collectd;

	/* Setup list of header fields */
	static hf_register_info hf[] = {
		{ &hf_collectd_type,
			{ "Type", "collectd.type", FT_UINT16, BASE_HEX,
				VALS(part_names), 0x0, NULL, HFILL }
		},
		{ &hf_collectd_length,
			{ "Length", "collectd.len", FT_UINT16, BASE_DEC,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data,
			{ "Payload", "collectd.data", FT_BYTES, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_host,
			{ "Host name", "collectd.data.host", FT_STRING, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_interval,
			{ "Interval", "collectd.data.interval", FT_RELATIVE_TIME, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_time,
			{ "Timestamp", "collectd.data.time", FT_ABSOLUTE_TIME, ABSOLUTE_TIME_LOCAL,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_plugin,
			{ "Plugin", "collectd.data.plugin", FT_STRING, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_plugin_inst,
			{ "Plugin instance", "collectd.data.plugin.inst", FT_STRING, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_type,
			{ "Type", "collectd.data.type", FT_STRING, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_type_inst,
			{ "Type instance", "collectd.data.type.inst", FT_STRING, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_valcnt,
			{ "Value count", "collectd.data.valcnt", FT_UINT16, BASE_DEC,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_val_type,
			{ "Value type", "collectd.val.type", FT_UINT8, BASE_HEX,
				VALS(valuetypenames), 0x0, NULL, HFILL }
		},
		{ &hf_collectd_val_counter,
			{ "Counter value", "collectd.val.counter", FT_UINT64, BASE_DEC,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_val_gauge,
			{ "Gauge value", "collectd.val.gauge", FT_DOUBLE, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_val_derive,
			{ "Derive value", "collectd.val.derive", FT_INT64, BASE_DEC,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_val_absolute,
			{ "Absolute value", "collectd.val.absolute", FT_UINT64, BASE_DEC,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_val_unknown,
			{ "Value of unknown type", "collectd.val.unknown", FT_UINT64, BASE_HEX,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_severity,
			{ "Severity", "collectd.data.severity", FT_UINT64, BASE_HEX | BASE_VAL64_STRING,
				VALS64(severity_names),
				0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_message,
			{ "Message", "collectd.data.message", FT_STRING, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_sighash,
			{ "Signature", "collectd.data.sighash", FT_BYTES, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_initvec,
			{ "Init vector", "collectd.data.initvec", FT_BYTES, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_username_len,
			{ "Username length", "collectd.data.username_length", FT_UINT16, BASE_DEC,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_username,
			{ "Username", "collectd.data.username", FT_STRING, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
		{ &hf_collectd_data_encrypted,
			{ "Encrypted data", "collectd.data.encrypted", FT_BYTES, BASE_NONE,
				NULL, 0x0, NULL, HFILL }
		},
	};

	/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_collectd,
		&ett_collectd_string,
		&ett_collectd_integer,
		&ett_collectd_part_value,
		&ett_collectd_value,
		&ett_collectd_valinfo,
		&ett_collectd_signature,
		&ett_collectd_encryption,
		&ett_collectd_dispatch,
		&ett_collectd_invalid_length,
		&ett_collectd_unknown,
	};

	static ei_register_info ei[] = {
		{ &ei_collectd_invalid_length, { "collectd.invalid_length", PI_MALFORMED, PI_ERROR, "Invalid length", EXPFILL }},
		{ &ei_collectd_garbage, { "collectd.garbage", PI_MALFORMED, PI_ERROR, "Garbage at end of packet", EXPFILL }},
		{ &ei_collectd_data_valcnt, { "collectd.data.valcnt.mismatch", PI_MALFORMED, PI_WARN, "Number of values and length of part do not match. Assuming length is correct.", EXPFILL }},
		{ &ei_collectd_type, { "collectd.type.unknown", PI_UNDECODED, PI_NOTE, "Unknown part type", EXPFILL }},
	};

	/* Register the protocol name and description */
	proto_collectd = proto_register_protocol("collectd network data", "collectd", "collectd");

	/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_collectd, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));
	expert_collectd = expert_register_protocol(proto_collectd);
	expert_register_field_array(expert_collectd, ei, array_length(ei));

	tap_collectd = register_tap ("collectd");

	/*
	 * Create an unsigned integer preference to allow the user to specify the
	 * UDP port on which to capture DIS packets.
	 */
	collectd_module = prefs_register_protocol (proto_collectd,
						   proto_reg_handoff_collectd);

	prefs_register_uint_preference (collectd_module, "udp.port",
					"collectd UDP port",
					"Set the UDP port for collectd messages",
					10, &collectd_udp_port);
} /* void proto_register_collectd */

void proto_reg_handoff_collectd (void)
{
	static gboolean first_run = TRUE;
	static gint registered_udp_port = -1;
	static dissector_handle_t collectd_handle;

	if (first_run)
		collectd_handle = create_dissector_handle (dissect_collectd,
							   proto_collectd);

	/* Change the dissector registration if the preferences have been
	 * changed. */
	if (registered_udp_port != -1)
		dissector_delete_uint ("udp.port", registered_udp_port,
				  collectd_handle);

	dissector_add_uint ("udp.port", collectd_udp_port, collectd_handle);
	registered_udp_port = collectd_udp_port;

	if (first_run)
		collectd_stats_tree_register ();

	first_run = FALSE;
} /* void proto_reg_handoff_collectd */

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
