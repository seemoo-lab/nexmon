/* proto_hier_stats.c
 * Routines for calculating statistics based on protocol.
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

#include "file.h"
#include "frame_tvbuff.h"
#include "ui/proto_hier_stats.h"
#include "ui/progress_dlg.h"
#include "epan/epan_dissect.h"
#include "epan/proto.h"

#include <string.h>

/* Update the progress bar this many times when scanning the packet list. */
#define N_PROGBAR_UPDATES	100

#define STAT_NODE_STATS(n)   ((ph_stats_node_t*)(n)->data)
#define STAT_NODE_HFINFO(n)  (STAT_NODE_STATS(n)->hfinfo)

static int pc_proto_id = -1;

static GNode*
find_stat_node(GNode *parent_stat_node, header_field_info *needle_hfinfo)
{
	GNode			*needle_stat_node, *up_parent_stat_node;
	header_field_info	*hfinfo;
	ph_stats_node_t		*stats;

	/* Look down the tree */
	needle_stat_node = g_node_first_child(parent_stat_node);

	while (needle_stat_node) {
		hfinfo = STAT_NODE_HFINFO(needle_stat_node);
		if (hfinfo &&  hfinfo->id == needle_hfinfo->id) {
			return needle_stat_node;
		}
		needle_stat_node = g_node_next_sibling(needle_stat_node);
	}

	/* Look up the tree */
	up_parent_stat_node = parent_stat_node;
	while (up_parent_stat_node && up_parent_stat_node->parent)
	{
		needle_stat_node = g_node_first_child(up_parent_stat_node->parent);
		while (needle_stat_node) {
			hfinfo = STAT_NODE_HFINFO(needle_stat_node);
			if (hfinfo &&  hfinfo->id == needle_hfinfo->id) {
				return needle_stat_node;
			}
			needle_stat_node = g_node_next_sibling(needle_stat_node);
		}

		up_parent_stat_node = up_parent_stat_node->parent;
	}

	/* None found. Create one. */
	stats = g_new(ph_stats_node_t, 1);

	/* Intialize counters */
	stats->hfinfo = needle_hfinfo;
	stats->num_pkts_total = 0;
	stats->num_pkts_last = 0;
	stats->num_bytes_total = 0;
	stats->num_bytes_last = 0;

	needle_stat_node = g_node_new(stats);
	g_node_append(parent_stat_node, needle_stat_node);
	return needle_stat_node;
}


static void
process_node(proto_node *ptree_node, GNode *parent_stat_node, ph_stats_t *ps)
{
	field_info		*finfo;
	ph_stats_node_t		*stats;
	proto_node		*proto_sibling_node;
	GNode			*stat_node;

	finfo = PNODE_FINFO(ptree_node);
	/* We don't fake protocol nodes we expect them to have a field_info.
	 * Dissection with faked proto tree? */
	g_assert(finfo);

	/* If the field info isn't related to a protocol but to a field,
	 * don't count them, as they don't belong to any protocol.
	 * (happens e.g. for toplevel tree item of desegmentation "[Reassembled TCP Segments]") */
	if (finfo->hfinfo->parent != -1) {
		/* Skip this element, use parent status node */
		stat_node = parent_stat_node;
		stats = STAT_NODE_STATS(stat_node);
	} else {
		stat_node = find_stat_node(parent_stat_node, finfo->hfinfo);

		stats = STAT_NODE_STATS(stat_node);
		stats->num_pkts_total++;
		stats->num_bytes_total += finfo->length;
	}

	proto_sibling_node = ptree_node->next;

	if (proto_sibling_node) {
		/* If the name does not exist for this proto_sibling_node, then it is
		 * not a normal protocol in the top-level tree.  It was instead
		 * added as a normal tree such as IPv6's Hop-by-hop Option Header and
		 * should be skipped when creating the protocol hierarchy display. */
		if(strlen(PNODE_FINFO(proto_sibling_node)->hfinfo->name) == 0 && ptree_node->next)
			proto_sibling_node = proto_sibling_node->next;

		process_node(proto_sibling_node, stat_node, ps);
	} else {
		stats->num_pkts_last++;
		stats->num_bytes_last += finfo->length;
	}
}



static void
process_tree(proto_tree *protocol_tree, ph_stats_t* ps)
{
	proto_node	*ptree_node;

	/*
	 * If our first item is a comment, skip over it. This keeps
	 * us from having a top-level "Packet comments" item that
	 * steals items from "Frame".
	 */
	ptree_node = ((proto_node *)protocol_tree)->first_child;
	if (ptree_node && ptree_node->finfo->hfinfo->id == pc_proto_id) {
		ptree_node = ptree_node->next;
	}

	if (!ptree_node) {
		return;
	}

	process_node(ptree_node, ps->stats_tree, ps);
}

static gboolean
process_record(capture_file *cf, frame_data *frame, column_info *cinfo, ph_stats_t* ps)
{
	epan_dissect_t			edt;
	struct wtap_pkthdr              phdr;
	Buffer				buf;
	double				cur_time;

	wtap_phdr_init(&phdr);

	/* Load the record from the capture file */
	ws_buffer_init(&buf, 1500);
	if (!cf_read_record_r(cf, frame, &phdr, &buf))
		return FALSE;	/* failure */

	/* Dissect the record   tree  not visible */
	epan_dissect_init(&edt, cf->epan, TRUE, FALSE);
	/* Don't fake protocols. We need them for the protocol hierarchy */
	epan_dissect_fake_protocols(&edt, FALSE);
	epan_dissect_run(&edt, cf->cd_t, &phdr, frame_tvbuff_new_buffer(frame, &buf), frame, cinfo);

	/* Get stats from this protocol tree */
	process_tree(edt.tree, ps);

	if (frame->flags.has_ts) {
		/* Update times */
		cur_time = nstime_to_sec(&frame->abs_ts);
		if (cur_time < ps->first_time)
			ps->first_time = cur_time;
		if (cur_time > ps->last_time)
			ps->last_time = cur_time;
	}

	/* Free our memory. */
	epan_dissect_cleanup(&edt);
	wtap_phdr_cleanup(&phdr);
	ws_buffer_free(&buf);

	return TRUE;	/* success */
}

ph_stats_t*
ph_stats_new(capture_file *cf)
{
	ph_stats_t	*ps;
	guint32		framenum;
	frame_data	*frame;
	guint		tot_packets, tot_bytes;
	progdlg_t	*progbar = NULL;
	gboolean	stop_flag;
	int		count;
	float		progbar_val;
	GTimeVal	start_time;
	gchar		status_str[100];
	int		progbar_nextstep;
	int		progbar_quantum;

	if (!cf) return NULL;

	pc_proto_id = proto_get_id_by_filter_name("pkt_comment");

	/* Initialize the data */
	ps = g_new(ph_stats_t, 1);
	ps->tot_packets = 0;
	ps->tot_bytes = 0;
	ps->stats_tree = g_node_new(NULL);
	ps->first_time = 0.0;
	ps->last_time = 0.0;

	/* Update the progress bar when it gets to this value. */
	progbar_nextstep = 0;
	/* When we reach the value that triggers a progress bar update,
	   bump that value by this amount. */
	progbar_quantum = cf->count/N_PROGBAR_UPDATES;
	/* Count of packets at which we've looked. */
	count = 0;
	/* Progress so far. */
	progbar_val = 0.0f;

	stop_flag = FALSE;
	g_get_current_time(&start_time);

	tot_packets = 0;
	tot_bytes = 0;

	for (framenum = 1; framenum <= cf->count; framenum++) {
		frame = frame_data_sequence_find(cf->frames, framenum);

		/* Create the progress bar if necessary.
		   We check on every iteration of the loop, so that
		   it takes no longer than the standard time to create
		   it (otherwise, for a large file, we might take
		   considerably longer than that standard time in order
		   to get to the next progress bar step). */
		if (progbar == NULL)
			progbar = delayed_create_progress_dlg(
			    cf->window, "Computing",
			    "protocol hierarchy statistics",
			    TRUE, &stop_flag, &start_time, progbar_val);

		/* Update the progress bar, but do it only N_PROGBAR_UPDATES
		   times; when we update it, we have to run the GTK+ main
		   loop to get it to repaint what's pending, and doing so
		   may involve an "ioctl()" to see if there's any pending
		   input from an X server, and doing that for every packet
		   can be costly, especially on a big file. */
		if (count >= progbar_nextstep) {
			/* let's not divide by zero. I should never be started
			 * with count == 0, so let's assert that
			 */
			g_assert(cf->count > 0);

			progbar_val = (gfloat) count / cf->count;

			if (progbar != NULL) {
				g_snprintf(status_str, sizeof(status_str),
					"%4u of %u frames", count, cf->count);
				update_progress_dlg(progbar, progbar_val, status_str);
			}

			progbar_nextstep += progbar_quantum;
		}

		if (stop_flag) {
			/* Well, the user decided to abort the statistics.
			   computation process  Just stop. */
			break;
		}

		/* Skip frames that are hidden due to the display filter.
		   XXX - should the progress bar count only packets that
		   passed the display filter?  If so, it should
		   probably do so for other loops (see "file.c") that
		   look only at those packets. */
		if (frame->flags.passed_dfilter) {

			if (frame->flags.has_ts) {
				if (tot_packets == 0) {
					double cur_time = nstime_to_sec(&frame->abs_ts);
					ps->first_time = cur_time;
					ps->last_time = cur_time;
				}
			}

			/* we don't care about colinfo */
			if (!process_record(cf, frame, NULL, ps)) {
				/*
				 * Give up, and set "stop_flag" so we
				 * just abort rather than popping up
				 * the statistics window.
				 */
				stop_flag = TRUE;
				break;
			}

			tot_packets++;
			tot_bytes += frame->pkt_len;
		}

		count++;
	}

	/* We're done calculating the statistics; destroy the progress bar
	   if it was created. */
	if (progbar != NULL)
		destroy_progress_dlg(progbar);

	if (stop_flag) {
		/*
		 * We quit in the middle; throw away the statistics
		 * and return NULL, so our caller doesn't pop up a
		 * window with the incomplete statistics.
		 */
		ph_stats_free(ps);
		return NULL;
	}

	ps->tot_packets = tot_packets;
	ps->tot_bytes = tot_bytes;

	return ps;
}

static gboolean
stat_node_free(GNode *node, gpointer data _U_)
{
	ph_stats_node_t	*stats = (ph_stats_node_t *)node->data;

	if (stats) {
		g_free(stats);
	}
	return FALSE;
}

void
ph_stats_free(ph_stats_t *ps)
{

	if (ps->stats_tree) {
		g_node_traverse(ps->stats_tree, G_IN_ORDER,
				G_TRAVERSE_ALL, -1,
				stat_node_free, NULL);
		g_node_destroy(ps->stats_tree);
	}

	g_free(ps);
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
