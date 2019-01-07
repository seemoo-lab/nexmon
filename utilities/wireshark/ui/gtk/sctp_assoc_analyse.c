/*
 * Copyright 2004, Irene Ruengeler <i.ruengeler [AT] fh-muenster.de>
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

#include <gtk/gtk.h>

#include <epan/epan_dissect.h>
#include <epan/to_str.h>

#include "ui/simple_dialog.h"
#include <epan/stat_groups.h>

#include "ui/gtk/gui_stat_menu.h"
#include "ui/gtk/gui_utils.h"
#include "ui/gtk/main.h"
#include "ui/gtk/sctp_stat_gtk.h"
#include "ui/gtk/gtkglobals.h"
#include "ui/gtk/stock_icons.h"

#include "frame_tvbuff.h"

static sctp_assoc_info_t static_assoc;

void
decrease_childcount(struct sctp_analyse *parent)
{
	if (parent->num_children > 0)
		parent->num_children--;
}

void
increase_childcount(struct sctp_analyse *parent)
{
	parent->num_children++;
}

void
set_child(struct sctp_udata *child, struct sctp_analyse *parent)
{
	parent->children = g_list_append(parent->children, child);
}

void
remove_child(struct sctp_udata *child, struct sctp_analyse *parent)
{
	parent->children = g_list_remove(parent->children, child);
}

static void
on_destroy(GObject *object _U_, gpointer user_data)
{
	struct sctp_analyse *u_data;
	struct sctp_udata   *child_data;
	guint16	 i, j;
	GList	*list;

	u_data = (struct sctp_analyse*)user_data;

	if (u_data->window)
	{
		gtk_grab_remove(GTK_WIDGET(u_data->window));
		gtk_widget_destroy(GTK_WIDGET(u_data->window));
	}
	if (u_data->num_children > 0)
	{
		j = u_data->num_children;
		for (i=0; i<j; i++)
		{
			list = g_list_last(u_data->children);
			child_data = (struct sctp_udata *)list->data;
			gtk_grab_remove(GTK_WIDGET(child_data->io->window));
			gtk_widget_destroy(GTK_WIDGET(child_data->io->window));
		}
		g_list_free(u_data->children);
		u_data->children = NULL;
	}

	g_free(u_data->analyse_nb->page2);
	g_free(u_data->analyse_nb->page3);
	g_free(u_data->analyse_nb);
	if (sctp_stat_get_info()->children != NULL)
	{
		remove_analyse_child(u_data);
		decrease_analyse_childcount();
	}
	g_free(u_data);
}

static void
on_notebook_switch_page(void)
{
}

static void
on_chunk_stat_bt(GtkWidget *widget _U_, struct sctp_analyse *u_data)
{
	sctp_assoc_info_t *assinfo;
	int i;

	assinfo = &static_assoc;
	assinfo->addr_chunk_count = static_assoc.addr_chunk_count;
	for (i=0; i<NUM_CHUNKS; i++)
	{
		assinfo->chunk_count[i]     = static_assoc.chunk_count[i];
		assinfo->ep1_chunk_count[i] = static_assoc.ep1_chunk_count[i];
		assinfo->ep2_chunk_count[i] = static_assoc.ep2_chunk_count[i];
	}
	u_data->assoc = assinfo;
	sctp_chunk_dlg_show(u_data);
}

static void
on_close_dlg(GtkWidget *widget _U_, struct sctp_analyse *u_data)
{

	if (u_data->window)
	{
		gtk_grab_remove(GTK_WIDGET(u_data->window));
		gtk_widget_destroy(GTK_WIDGET(u_data->window));
	}
}

static void
on_chunk1_dlg(GtkWidget *widget _U_, struct sctp_analyse *u_data)
{
	sctp_assoc_info_t *assinfo;

	assinfo = &static_assoc;
	assinfo->addr_chunk_count = static_assoc.addr_chunk_count;
	u_data->assoc = assinfo;
	sctp_chunk_stat_dlg_show(1, u_data);
}

static void on_chunk2_dlg(GtkWidget *widget _U_, struct sctp_analyse *u_data)
{
	sctp_assoc_info_t *assinfo;

	assinfo = &static_assoc;
	assinfo->addr_chunk_count = static_assoc.addr_chunk_count;
	u_data->assoc = assinfo;
	sctp_chunk_stat_dlg_show(2, u_data);
}

static void
on_graph1_dlg(GtkWidget *widget _U_, struct sctp_analyse *u_data)
{
	create_graph(1, u_data);
}

static void
on_graph2_dlg(GtkWidget *widget _U_, struct sctp_analyse *u_data)
{
	create_graph(2, u_data);
}

static void
on_graph_byte1_dlg(GtkWidget *widget _U_, struct sctp_analyse *u_data)
{
	create_byte_graph(1, u_data);
}

static void
on_graph_byte2_dlg(GtkWidget *widget _U_, struct sctp_analyse *u_data)
{
	create_byte_graph(2, u_data);
}

void
update_analyse_dlg(struct sctp_analyse *u_data)
{
	gchar label_txt[50];

	if (u_data->assoc == NULL)
		return;

	if (u_data->window != NULL)
	{
		gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW
				(u_data->analyse_nb->page2->clist))));
		gtk_list_store_clear(GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW
			(u_data->analyse_nb->page3->clist))));
	}


	g_snprintf(label_txt, sizeof(label_txt),
		   "Checksum Type: %s",
		   u_data->assoc->checksum_type);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->checktype), label_txt);
	g_snprintf(label_txt, sizeof(label_txt),
		   "Checksum Errors: %u",
		   u_data->assoc->n_checksum_errors);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->checksum), label_txt);
	g_snprintf(label_txt, sizeof(label_txt),
		   "Bundling Errors: %u",
		   u_data->assoc->n_bundling_errors);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->bundling), label_txt);
	g_snprintf(label_txt, sizeof(label_txt),
		   "Padding Errors: %u",
		   u_data->assoc->n_padding_errors);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->padding), label_txt);
	g_snprintf(label_txt, sizeof(label_txt),
		   "Length Errors: %u",
		   u_data->assoc->n_length_errors);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->length), label_txt);
	g_snprintf(label_txt, sizeof(label_txt),
		   "Value Errors: %u",
		   u_data->assoc->n_value_errors);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->value), label_txt);
	g_snprintf(label_txt, sizeof(label_txt),
		   "No of Data Chunks from EP1 to EP2: %u",
		   u_data->assoc->n_data_chunks_ep1);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->chunks_ep1), label_txt);
	g_snprintf(label_txt, sizeof(label_txt),
		   "No of Data Bytes from EP1 to EP2: %u",
		   u_data->assoc->n_data_bytes_ep1);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->bytes_ep1), label_txt);
	g_snprintf(label_txt, sizeof(label_txt),
		   "No of Data Chunks from EP2 to EP1: %u",
		   u_data->assoc->n_data_chunks_ep2);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->chunks_ep2), label_txt);
	g_snprintf(label_txt, sizeof(label_txt),
		   "No of Data Bytes from EP2 to EP1: %u",
		   u_data->assoc->n_data_bytes_ep2);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->bytes_ep2), label_txt);

	if (u_data->assoc->init == TRUE)
		gtk_frame_set_label(GTK_FRAME(u_data->analyse_nb->page2->addr_frame),
				    "Complete list of IP-Addresses as provided in the INIT-Chunk");
	else if ((u_data->assoc->initack == TRUE) && (u_data->assoc->initack_dir == 1))
		gtk_frame_set_label(GTK_FRAME(u_data->analyse_nb->page2->addr_frame),
				    "Complete list of IP-Addresses as provided in the INITACK-Chunk");
	else
		gtk_frame_set_label(GTK_FRAME(u_data->analyse_nb->page2->addr_frame),
				    "List of used IP-Addresses");



	if (u_data->assoc->addr1 != NULL)
	{
		GList *list;

		list = g_list_first(u_data->assoc->addr1);
		while (list)
		{
			gchar	     *field;
			address	     *store;
			GtkListStore *list_store;

			store = (address *)(list->data);
			if (store->type != AT_NONE) {
				if ((store->type == AT_IPv4) || (store->type == AT_IPv6))
				{
					field = (gchar*)address_to_str(NULL, store);
				}
				else
				{
					field = NULL;
				}

				list_store = GTK_LIST_STORE(
					gtk_tree_view_get_model(GTK_TREE_VIEW(u_data->analyse_nb->page2->clist))); /* Get store */

				gtk_list_store_insert_with_values( list_store , NULL, G_MAXINT,
									 0, field, -1);
				wmem_free(NULL, field);
			}
			list = g_list_next(list);
		}
	}
	else
	{
		return;
	}
	g_snprintf(label_txt, sizeof(label_txt), "Port: %u", u_data->assoc->port1);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page2->port), label_txt);
	g_snprintf(label_txt, sizeof(label_txt), "Sent Verification Tag: 0x%x", u_data->assoc->verification_tag1);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page2->veritag), label_txt);

	if ((u_data->assoc->init == TRUE) ||
	    ((u_data->assoc->initack == TRUE) && (u_data->assoc->initack_dir == 1)))
	{
		g_snprintf(label_txt, sizeof(label_txt),
			   "Requested Number of Inbound Streams: %u",
			   u_data->assoc->instream1);
		gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page2->max_in), label_txt);
		g_snprintf(label_txt, sizeof(label_txt),
			   "Minimum Number of Inbound Streams: %u",
			   ((u_data->assoc->instream1 > u_data->assoc->outstream2) ?
			     u_data->assoc->outstream2 : u_data->assoc->instream1));
		gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page2->min_in), label_txt);

		g_snprintf(label_txt, sizeof(label_txt),
			   "Provided Number of Outbound Streams: %u",
			   u_data->assoc->outstream1);
		gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page2->max_out), label_txt);
		g_snprintf(label_txt, sizeof(label_txt),
			   "Minimum Number of Outbound Streams: %u",
			   ((u_data->assoc->outstream1 > u_data->assoc->instream2) ?
			    u_data->assoc->instream2 : u_data->assoc->outstream1));
		gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page2->min_out), label_txt);
	}
	else
	{
		g_snprintf(label_txt, sizeof(label_txt),
			   "Used Number of Inbound Streams: %u",
			   u_data->assoc->instream1);
		gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page2->max_in), label_txt);
		g_snprintf(label_txt, sizeof(label_txt),
			   "Used Number of Outbound Streams: %u",
			   u_data->assoc->outstream1);
		gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page2->max_out), label_txt);
	}

	if ((u_data->assoc->initack == TRUE) && (u_data->assoc->initack_dir == 2))
		gtk_frame_set_label(GTK_FRAME(u_data->analyse_nb->page3->addr_frame),
				    "Complete list of IP-Addresses as provided in the INITACK-Chunk");
	else
		gtk_frame_set_label(GTK_FRAME(u_data->analyse_nb->page3->addr_frame),
				    "List of used IP-Addresses");


	if (u_data->assoc->addr2 != NULL)
	{
		GList *list;

		list = g_list_first(u_data->assoc->addr2);
		while (list)
		{
			gchar	     *field;
			address	     *store;
			GtkListStore *list_store;

			store = (address *)(list->data);
			if (store->type != AT_NONE) {
				if ((store->type == AT_IPv4) || (store->type == AT_IPv6))
				{
					field = (gchar*)address_to_str(NULL, store);
				}
				else
				{
					field = NULL;
				}

				list_store = GTK_LIST_STORE(
					gtk_tree_view_get_model(GTK_TREE_VIEW(u_data->analyse_nb->page3->clist))); /* Get store */

				gtk_list_store_insert_with_values( list_store , NULL, G_MAXINT,
									 0, field, -1);
				wmem_free(NULL, field);
			}
			list = g_list_next(list);
		}
	}
		else
	{
		return;
	}

	g_snprintf(label_txt, sizeof(label_txt), "Port: %u", u_data->assoc->port2);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page3->port), label_txt);
	g_snprintf(label_txt, sizeof(label_txt), "Sent Verification Tag: 0x%x", u_data->assoc->verification_tag2);
	gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page3->veritag), label_txt);

	if (u_data->assoc->initack == TRUE)
	{
		g_snprintf(label_txt, sizeof(label_txt),
			   "Requested Number of Inbound Streams: %u",
			   u_data->assoc->instream2);
		gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page3->max_in), label_txt);
		g_snprintf(label_txt, sizeof(label_txt),
			   "Minimum Number of Inbound Streams: %u",
			   ((u_data->assoc->instream2 > u_data->assoc->outstream1) ?
			    u_data->assoc->outstream1 : u_data->assoc->instream2));
		gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page3->min_in), label_txt);

		g_snprintf(label_txt, sizeof(label_txt),
			   "Provided Number of Outbound Streams: %u",
			   u_data->assoc->outstream2);
		gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page3->max_out), label_txt);
		g_snprintf(label_txt, sizeof(label_txt),
			   "Minimum Number of Outbound Streams: %u",
			   ((u_data->assoc->outstream2 > u_data->assoc->instream1) ?
			    u_data->assoc->instream1 : u_data->assoc->outstream2));
		gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page3->min_out), label_txt);
	}
	else
	{
		g_snprintf(label_txt, sizeof(label_txt),
			   "Used Number of Inbound Streams: %u",
			   u_data->assoc->instream2);
		gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page3->max_in), label_txt);
		g_snprintf(label_txt, sizeof(label_txt),
			   "Used Number of Outbound Streams: %u",
			   u_data->assoc->outstream2);
		gtk_label_set_text(GTK_LABEL(u_data->analyse_nb->page3->min_out), label_txt);
	}
}


static void
sctp_set_filter(GtkButton *button _U_, struct sctp_analyse *u_data)
{
	sctp_assoc_info_t *selected_stream;
	gchar *filter_string = NULL;

	selected_stream = u_data->assoc;
	if (selected_stream == NULL)
		return;

	if (selected_stream->check_address == FALSE)
	{
		gchar *f_string;

		f_string = g_strdup_printf(
			"((sctp.srcport==%u && sctp.dstport==%u &&"
			" ((sctp.verification_tag==0x%x && sctp.verification_tag!=0x0) ||"
			" (sctp.verification_tag==0x0 && sctp.initiate_tag==0x%x) ||"
			" (sctp.verification_tag==0x%x &&"
			" (sctp.abort_t_bit==1 || sctp.shutdown_complete_t_bit==1)))) ||"
			" (sctp.srcport==%u && sctp.dstport==%u &&"
			" ((sctp.verification_tag==0x%x && sctp.verification_tag!=0x0) ||"
			" (sctp.verification_tag==0x0 && sctp.initiate_tag==0x%x) ||"
			" (sctp.verification_tag==0x%x &&"
			" (sctp.abort_t_bit==1 || sctp.shutdown_complete_t_bit==1)))))",
			selected_stream->port1,
			selected_stream->port2,
			selected_stream->verification_tag1,
			selected_stream->initiate_tag,
			selected_stream->verification_tag2,
			selected_stream->port2,
			selected_stream->port1,
			selected_stream->verification_tag2,
			selected_stream->initiate_tag,
			selected_stream->verification_tag1);
		filter_string = f_string;
	}
	else
	{
		GList	*srclist, *dstlist;
		gchar	*str;
		GString *gstring;
		struct sockaddr_in *infosrc;
		struct sockaddr_in *infodst;
		address addr;
		char    *addr_str;

		srclist = g_list_first(selected_stream->addr1);
		infosrc = (struct sockaddr_in *)(srclist->data);
		set_address(&addr, AT_IPv4, 4, &(infosrc->sin_addr.s_addr));
		addr_str = (char*)address_to_str(NULL, &addr);
		gstring = g_string_new(g_strdup_printf(
					       "((sctp.srcport==%u && sctp.dstport==%u && (ip.src==%s",
					       selected_stream->port1,
					       selected_stream->port2,
					       addr_str));
		srclist = g_list_next(srclist);
		wmem_free(NULL, addr_str);

		while (srclist)
		{
			infosrc = (struct sockaddr_in *)(srclist->data);
			set_address(&addr, AT_IPv4, 4, &(infosrc->sin_addr.s_addr));
			addr_str = (char*)address_to_str(NULL, &addr);
			str = g_strdup_printf("|| ip.src==%s", addr_str);
			g_string_append(gstring, str);
			srclist = g_list_next(srclist);
			wmem_free(NULL, addr_str);
		}

		dstlist = g_list_first(selected_stream->addr2);
		infodst = (struct sockaddr_in *)(dstlist->data);
		set_address(&addr, AT_IPv4, 4, &(infodst->sin_addr.s_addr));
		addr_str = (char*)address_to_str(NULL, &addr);
		str = g_strdup_printf(") && (ip.dst==%s", addr_str);
		g_string_append(gstring, str);
		dstlist = g_list_next(dstlist);
		wmem_free(NULL, addr_str);

		while (dstlist)
		{
			infodst = (struct sockaddr_in *)(dstlist->data);
			set_address(&addr, AT_IPv4, 4, &(infodst->sin_addr.s_addr));
			addr_str = (char*)address_to_str(NULL, &addr);
			str = g_strdup_printf("|| ip.dst==%s", addr_str);
			g_string_append(gstring, str);
			dstlist = g_list_next(dstlist);
			wmem_free(NULL, addr_str);
		}

		srclist = g_list_first(selected_stream->addr1);
		infosrc = (struct sockaddr_in *)(srclist->data);
		set_address(&addr, AT_IPv4, 4, &(infosrc->sin_addr.s_addr));
		addr_str = (char*)address_to_str(NULL, &addr);
		str = g_strdup_printf(")) || (sctp.dstport==%u && sctp.srcport==%u && (ip.dst==%s",
				      selected_stream->port1,
				      selected_stream->port2,
				      addr_str);
		g_string_append(gstring, str);
		srclist = g_list_next(srclist);
		wmem_free(NULL, addr_str);

		while (srclist)
		{
			infosrc = (struct sockaddr_in *)(srclist->data);
			set_address(&addr, AT_IPv4, 4, &(infosrc->sin_addr.s_addr));
			addr_str = (char*)address_to_str(NULL, &addr);
			str = g_strdup_printf("|| ip.dst==%s", addr_str);
			g_string_append(gstring, str);
			srclist = g_list_next(srclist);
			wmem_free(NULL, addr_str);
		}

		dstlist = g_list_first(selected_stream->addr2);
		infodst = (struct sockaddr_in *)(dstlist->data);
		set_address(&addr, AT_IPv4, 4, &(infodst->sin_addr.s_addr));
		addr_str = (char*)address_to_str(NULL, &addr);
		str = g_strdup_printf(") && (ip.src==%s", addr_str);
		g_string_append(gstring, str);
		dstlist = g_list_next(dstlist);
		wmem_free(NULL, addr_str);
		while (dstlist)
		{
			infodst = (struct sockaddr_in *)(dstlist->data);
			set_address(&addr, AT_IPv4, 4, &(infodst->sin_addr.s_addr));
			addr_str = (char*)address_to_str(NULL, &addr);
			str = g_strdup_printf("|| ip.src==%s", addr_str);
			g_string_append(gstring, str);
			dstlist = g_list_next(dstlist);
			wmem_free(NULL, addr_str);
		}
		str = g_strdup(")))");
		g_string_append(gstring, str);
		filter_string = g_string_free(gstring, FALSE);
	}

	if (filter_string != NULL) {
		gtk_entry_set_text(GTK_ENTRY(main_display_filter_widget), filter_string);
	} else {
		g_assert_not_reached();
	}
}

static void
analyse_window_set_title(struct sctp_analyse *u_data)
{
	char *display_name;
	char *title;

	if (!u_data->window) {
		return;
	}
	display_name = cf_get_display_name(&cfile);
	title = g_strdup_printf("SCTP Analyse Association: %s Port1 %u  Port2 %u",
				display_name, u_data->assoc->port1, u_data->assoc->port2);
	g_free(display_name);
	gtk_window_set_title(GTK_WINDOW(u_data->window), title);
	g_free(title);
}

static GtkWidget *
create_list(void)
{
	GtkListStore	  *list_store;
	GtkWidget	  *list;
	GtkTreeViewColumn *column;
	GtkCellRenderer	  *renderer;
	GtkTreeView	  *list_view;

	list_store = gtk_list_store_new(1,
					G_TYPE_STRING /* IP address */
		);

	/* Create a view */
	list = gtk_tree_view_new_with_model (GTK_TREE_MODEL(list_store));

	list_view = GTK_TREE_VIEW(list);

	/* Speed up the list display */
	gtk_tree_view_set_fixed_height_mode(list_view, TRUE);

	/* The view now holds a reference.  We can get rid of our own reference */
	g_object_unref (G_OBJECT(list_store));

	/*
	 * Create the first column packet, associating the "text" attribute of the
	 * cell_renderer to the first column of the model
	 */
 	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes ("Address", renderer,
							   "text", 0,
							   NULL);

	gtk_tree_view_column_set_resizable(column, TRUE);
	gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_FIXED);
	gtk_tree_view_column_set_min_width(column, 300);

	/* Add the column to the view. */
	gtk_tree_view_append_column (list_view, column);

	gtk_tree_view_set_headers_visible(list_view, FALSE);
	return list;
}

static void
create_analyse_window(struct sctp_analyse *u_data)
{
	GtkWidget *window;
	GtkWidget *notebook;
	GtkWidget *main_vb, *page1, *page2, *page3, *hbox, *vbox_l, *vbox_r, *addr_hb, *stat_fr;
	GtkWidget *hbox_l1, *hbox_l2, *label, *h_button_box;
	GtkWidget *chunk_stat_bt, *close_bt, *graph_bt1, *graph_bt2, *chunk_bt1, *bt_filter;

	u_data->analyse_nb = (struct notes *)g_malloc(sizeof(struct notes));
	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	gtk_window_set_position (GTK_WINDOW(window), GTK_WIN_POS_CENTER);
	g_signal_connect(window, "destroy", G_CALLBACK(on_destroy), u_data);

	/* Container for each row of widgets */
	main_vb = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 2, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(main_vb), 2);
	gtk_container_add(GTK_CONTAINER(window), main_vb);
	gtk_widget_show(main_vb);

	/* Start a notebook for flipping between sets of changes */
	notebook = gtk_notebook_new();
	gtk_box_pack_start(GTK_BOX(main_vb), notebook, TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(window), "notebook", notebook);
	g_signal_connect(notebook, "switch_page", G_CALLBACK(on_notebook_switch_page), NULL);

	page1 = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 8, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(page1), 8);

	u_data->analyse_nb->checktype = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(page1), u_data->analyse_nb->checktype, TRUE, TRUE, 0);

	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->checktype), 0, 0);

	u_data->analyse_nb->checksum = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(page1), u_data->analyse_nb->checksum, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->checksum), 0, 0);

	u_data->analyse_nb->bundling = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(page1), u_data->analyse_nb->bundling, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->bundling), 0, 0);

	u_data->analyse_nb->padding = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(page1), u_data->analyse_nb->padding, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->padding), 0, 0);

	u_data->analyse_nb->length = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(page1), u_data->analyse_nb->length, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->length), 0, 0);

	u_data->analyse_nb->value = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(page1), u_data->analyse_nb->value, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->value), 0, 0);


	u_data->analyse_nb->chunks_ep1 = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(page1), u_data->analyse_nb->chunks_ep1, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->chunks_ep1), 0, 0);
	u_data->analyse_nb->bytes_ep1 = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(page1), u_data->analyse_nb->bytes_ep1, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->bytes_ep1), 0, 0);
	u_data->analyse_nb->chunks_ep2 = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(page1), u_data->analyse_nb->chunks_ep2, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->chunks_ep2), 0, 0);
	u_data->analyse_nb->bytes_ep2 = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(page1), u_data->analyse_nb->bytes_ep2, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->bytes_ep2), 0, 0);

	hbox = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(page1), hbox, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 10);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbox), GTK_BUTTONBOX_SPREAD);
	gtk_box_set_spacing(GTK_BOX(hbox), 0);
	gtk_widget_show(hbox);

	chunk_stat_bt = gtk_button_new_with_label ("Chunk Statistics");
	gtk_box_pack_start(GTK_BOX(hbox), chunk_stat_bt, FALSE, FALSE, 0);
	gtk_widget_show(chunk_stat_bt);
	g_signal_connect(chunk_stat_bt, "clicked", G_CALLBACK(on_chunk_stat_bt), u_data);

	bt_filter = gtk_button_new_with_label ("Set filter");
	gtk_box_pack_start(GTK_BOX(hbox), bt_filter, FALSE, FALSE, 0);
	gtk_widget_show (bt_filter);
	g_signal_connect(bt_filter, "clicked", G_CALLBACK(sctp_set_filter), u_data);

	close_bt = ws_gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_box_pack_start(GTK_BOX(hbox), close_bt, FALSE, FALSE, 0);
	gtk_widget_show(close_bt);
	g_signal_connect(close_bt, "clicked", G_CALLBACK(on_close_dlg), u_data);

	/* tab */
	label = gtk_label_new(" Statistics ");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page1, label);


	/*  page for endpoint 1 */
	page2 = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 8, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(page2), 8);

	u_data->analyse_nb->page2 = (struct page *)g_malloc(sizeof(struct page));

	u_data->analyse_nb->page2->addr_frame = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(page2), u_data->analyse_nb->page2->addr_frame, TRUE, TRUE, 0);

	addr_hb = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(addr_hb), 5);
	gtk_container_add(GTK_CONTAINER(u_data->analyse_nb->page2->addr_frame), addr_hb);

	u_data->analyse_nb->page2->scrolled_window = scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(u_data->analyse_nb->page2->scrolled_window, 560, 100);

	u_data->analyse_nb->page2->clist = create_list();
	gtk_widget_show(u_data->analyse_nb->page2->clist);

	gtk_container_add(GTK_CONTAINER(u_data->analyse_nb->page2->scrolled_window), u_data->analyse_nb->page2->clist);

	gtk_box_pack_start(GTK_BOX(addr_hb), u_data->analyse_nb->page2->scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show(u_data->analyse_nb->page2->scrolled_window);

	stat_fr = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(page2), stat_fr, TRUE, TRUE, 0);

	hbox = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_container_add(GTK_CONTAINER(stat_fr), hbox);

	vbox_l = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), vbox_l, TRUE, TRUE, 0);



	hbox_l1 = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox_l), hbox_l1, TRUE, TRUE, 0);

	u_data->analyse_nb->page2->port = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox_l1), u_data->analyse_nb->page2->port, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->page2->port), 0, 0);

	hbox_l2 = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox_l), hbox_l2, TRUE, TRUE, 0);


	u_data->analyse_nb->page2->veritag = gtk_label_new("");

	gtk_box_pack_start(GTK_BOX(hbox_l2), u_data->analyse_nb->page2->veritag, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->page2->veritag), 0, 0);
	gtk_widget_show(vbox_l);

	vbox_r = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), vbox_r, TRUE, TRUE, 0);

	u_data->analyse_nb->page2->max_in = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox_r), u_data->analyse_nb->page2->max_in, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->page2->max_in), 0, 0);
	u_data->analyse_nb->page2->min_in = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox_r), u_data->analyse_nb->page2->min_in, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->page2->min_in), 0, 0);

	u_data->analyse_nb->page2->max_out = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox_r), u_data->analyse_nb->page2->max_out, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->page2->max_out), 0, 0);
	u_data->analyse_nb->page2->min_out = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox_r), u_data->analyse_nb->page2->min_out, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->page2->min_out), 0, 0);


	gtk_widget_show(vbox_r);

	h_button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(page2), h_button_box, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(h_button_box), 10);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(h_button_box), GTK_BUTTONBOX_SPREAD);
	gtk_box_set_spacing(GTK_BOX(h_button_box), 0);
	gtk_widget_show(h_button_box);

	chunk_bt1 = gtk_button_new_with_label("Chunk Statistics");
	gtk_box_pack_start(GTK_BOX(h_button_box), chunk_bt1, FALSE, FALSE, 0);
	gtk_widget_show(chunk_bt1);
	g_signal_connect(chunk_bt1, "clicked", G_CALLBACK(on_chunk1_dlg), u_data);

	graph_bt1 = gtk_button_new_with_label("Graph TSN");
	gtk_box_pack_start(GTK_BOX(h_button_box), graph_bt1, FALSE, FALSE, 0);
	gtk_widget_show(graph_bt1);
	g_signal_connect(graph_bt1, "clicked", G_CALLBACK(on_graph1_dlg), u_data);

	graph_bt2 = gtk_button_new_with_label("Graph Bytes");
	gtk_box_pack_start(GTK_BOX(h_button_box), graph_bt2, FALSE, FALSE, 0);
	gtk_widget_show(graph_bt2);
	g_signal_connect(graph_bt2, "clicked", G_CALLBACK(on_graph_byte1_dlg), u_data);
	if (u_data->assoc->n_array_tsn1 == 0)
	{
		gtk_widget_set_sensitive(graph_bt1, FALSE);
		gtk_widget_set_sensitive(graph_bt2, FALSE);
	}
	close_bt = ws_gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_box_pack_start(GTK_BOX(h_button_box), close_bt, FALSE, FALSE, 0);
	gtk_widget_show(close_bt);
	g_signal_connect(close_bt, "clicked", G_CALLBACK(on_close_dlg), u_data);

	label = gtk_label_new(" Endpoint 1 ");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page2, label);

	/* same page for endpoint 2*/

	page3 = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 8, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(page3), 8);

	u_data->analyse_nb->page3 = (struct page *)g_malloc(sizeof(struct page));

	u_data->analyse_nb->page3->addr_frame = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(page3), u_data->analyse_nb->page3->addr_frame, TRUE, TRUE, 0);

	addr_hb = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(addr_hb), 5);
	gtk_container_add(GTK_CONTAINER(u_data->analyse_nb->page3->addr_frame), addr_hb);

	u_data->analyse_nb->page3->scrolled_window = scrolled_window_new(NULL, NULL);
	gtk_widget_set_size_request(u_data->analyse_nb->page3->scrolled_window, 560, 100);

	u_data->analyse_nb->page3->clist = create_list();
	gtk_widget_show(u_data->analyse_nb->page3->clist);

	gtk_container_add(GTK_CONTAINER(u_data->analyse_nb->page3->scrolled_window),
	u_data->analyse_nb->page3->clist);

	gtk_box_pack_start(GTK_BOX(addr_hb), u_data->analyse_nb->page3->scrolled_window, TRUE, TRUE, 0);
	gtk_widget_show(u_data->analyse_nb->page3->scrolled_window);

	stat_fr = gtk_frame_new(NULL);
	gtk_box_pack_start(GTK_BOX(page3), stat_fr, TRUE, TRUE, 0);

	hbox = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
	gtk_container_set_border_width(GTK_CONTAINER(hbox), 5);
	gtk_container_add(GTK_CONTAINER(stat_fr), hbox);

	vbox_l = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), vbox_l, TRUE, TRUE, 0);

	hbox_l1 = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox_l), hbox_l1, TRUE, TRUE, 0);

	u_data->analyse_nb->page3->port = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox_l1), u_data->analyse_nb->page3->port, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->page3->port), 0, 0);

	hbox_l2 = ws_gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(vbox_l), hbox_l2, TRUE, TRUE, 0);


	u_data->analyse_nb->page3->veritag = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(hbox_l2), u_data->analyse_nb->page3->veritag, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->page3->veritag), 0, 0);
	gtk_widget_show(vbox_l);

	vbox_r = ws_gtk_box_new(GTK_ORIENTATION_VERTICAL, 3, FALSE);
	gtk_box_pack_start(GTK_BOX(hbox), vbox_r, TRUE, TRUE, 0);

	u_data->analyse_nb->page3->max_in = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox_r), u_data->analyse_nb->page3->max_in, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->page3->max_in), 0, 0);
	u_data->analyse_nb->page3->min_in = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox_r), u_data->analyse_nb->page3->min_in, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->page3->min_in), 0, 0);

	u_data->analyse_nb->page3->max_out = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox_r), u_data->analyse_nb->page3->max_out, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->page3->max_out), 0, 0);
	u_data->analyse_nb->page3->min_out = gtk_label_new("");
	gtk_box_pack_start(GTK_BOX(vbox_r), u_data->analyse_nb->page3->min_out, TRUE, TRUE, 0);
	gtk_misc_set_alignment (GTK_MISC(u_data->analyse_nb->page3->min_out), 0, 0);

	gtk_widget_show(vbox_r);

	h_button_box = gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL);
	gtk_box_pack_start(GTK_BOX(page3), h_button_box, FALSE, FALSE, 0);
	gtk_container_set_border_width(GTK_CONTAINER(h_button_box), 10);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(h_button_box), GTK_BUTTONBOX_SPREAD);
	gtk_box_set_spacing(GTK_BOX(h_button_box), 0);
	gtk_widget_show(h_button_box);

	chunk_bt1 = gtk_button_new_with_label("Chunk Statistics");
	gtk_box_pack_start(GTK_BOX(h_button_box), chunk_bt1, FALSE, FALSE, 0);
	gtk_widget_show(chunk_bt1);
	g_signal_connect(chunk_bt1, "clicked", G_CALLBACK(on_chunk2_dlg), u_data);

	graph_bt1 = gtk_button_new_with_label("Graph TSN");
	gtk_box_pack_start(GTK_BOX(h_button_box), graph_bt1, FALSE, FALSE, 0);
	gtk_widget_show(graph_bt1);
	g_signal_connect(graph_bt1, "clicked", G_CALLBACK(on_graph2_dlg), u_data);
	graph_bt2 = gtk_button_new_with_label("Graph Bytes");
	gtk_box_pack_start(GTK_BOX(h_button_box), graph_bt2, FALSE, FALSE, 0);
	gtk_widget_show(graph_bt2);
	g_signal_connect(graph_bt2, "clicked", G_CALLBACK(on_graph_byte2_dlg), u_data);
	if (u_data->assoc->n_array_tsn2 == 0)
	{
		gtk_widget_set_sensitive(graph_bt1, FALSE);
		gtk_widget_set_sensitive(graph_bt2, FALSE);
	}
	close_bt = ws_gtk_button_new_from_stock(GTK_STOCK_CLOSE);
	gtk_box_pack_start(GTK_BOX(h_button_box), close_bt, FALSE, FALSE, 0);
	gtk_widget_show(close_bt);
	g_signal_connect(close_bt, "clicked", G_CALLBACK(on_close_dlg), u_data);

	label = gtk_label_new(" Endpoint 2 ");
	gtk_notebook_append_page(GTK_NOTEBOOK(notebook), page3, label);

	/* show all notebooks */
	gtk_widget_show_all(notebook);

	gtk_widget_show(window);

	u_data->window = window;
	analyse_window_set_title(u_data);

	update_analyse_dlg(u_data);
}



void
assoc_analyse(sctp_assoc_info_t* assoc)
{
	struct sctp_analyse *u_data;
	int i;

	u_data = (struct sctp_analyse *)g_malloc(sizeof(struct sctp_analyse));
	u_data->assoc			= assoc;
	u_data->assoc->addr_chunk_count = assoc->addr_chunk_count;
	u_data->window			= NULL;
	u_data->analyse_nb		= NULL;
	u_data->children		= NULL;
	u_data->num_children		= 0;
	static_assoc.addr_chunk_count = assoc->addr_chunk_count;
	static_assoc.port1 = assoc->port1;
	static_assoc.port2 = assoc->port2;
	for (i=0; i<NUM_CHUNKS; i++)
	{
		static_assoc.chunk_count[i]     = assoc->chunk_count[i];
		static_assoc.ep1_chunk_count[i] = assoc->ep1_chunk_count[i];
		static_assoc.ep2_chunk_count[i] = assoc->ep2_chunk_count[i];
	}
	set_analyse_child(u_data);
	increase_analyse_childcount();
	static_assoc.addr_chunk_count = assoc->addr_chunk_count;
	create_analyse_window(u_data);
}


static void
sctp_analyse_cb(struct sctp_analyse *u_data, gboolean ext)
{
	GList	       *list;
	dfilter_t      *sfcode;
	gchar          *err_msg;
	capture_file   *cf;
	epan_dissect_t	edt;
	gboolean	frame_found = FALSE;
	frame_data     *fdata;
	gchar		filter_text[256];

	g_strlcpy(filter_text, "sctp", 250);
	if (!dfilter_compile(filter_text, &sfcode, &err_msg)) {
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "%s", err_msg);
		g_free(err_msg);
		return;
	}

	cf    = &cfile;
	fdata = cf->current_frame;

	/* we are on the selected frame now */
	if (fdata == NULL)
		return; /* if we exit here it's an error */

	/* dissect the current record */
	if (!cf_read_record(cf, fdata))
		return;	/* error reading the record */

	epan_dissect_init(&edt, cf->epan, TRUE, FALSE);
	epan_dissect_prime_dfilter(&edt, sfcode);
	epan_dissect_run(&edt, cf->cd_t, &cf->phdr, frame_tvbuff_new_buffer(fdata, &cf->buf), fdata, NULL);

	/* if it is not an sctp packet, show the dialog */
	if (!dfilter_apply_edt(sfcode, &edt)) {
		epan_dissect_cleanup(&edt);
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK,
		    "Please choose an SCTP packet.");
		return;
	}

	list = g_list_first(sctp_stat_get_info()->assoc_info_list);

	while (list)
	{
		sctp_assoc_info_t *assoc;
		GList		  *framelist;

		assoc = (sctp_assoc_info_t*)(list->data);
		framelist = g_list_first(assoc->frame_numbers);
		while (framelist)
		{
			guint32	*fn;
			fn = (guint32 *)framelist->data;
			if (*fn == fdata->num)
			{
				frame_found = TRUE;
				break;
			}
			framelist = g_list_next(framelist);
		}
		if (frame_found)
		{
			int i;
			u_data->assoc = assoc;
			u_data->assoc->addr_chunk_count = assoc->addr_chunk_count;
			static_assoc.addr_chunk_count = assoc->addr_chunk_count;
			static_assoc.port1 = assoc->port1;
			static_assoc.port2 = assoc->port2;
			for (i=0; i<NUM_CHUNKS; i++)
			{
				static_assoc.chunk_count[i]     = assoc->chunk_count[i];
				static_assoc.ep1_chunk_count[i] = assoc->ep1_chunk_count[i];
				static_assoc.ep2_chunk_count[i] = assoc->ep2_chunk_count[i];
			}
			if (ext == FALSE)
				create_analyse_window(u_data);
			return;
		}
		else
			list = g_list_next(list);

	}
	if (!frame_found)
		simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "Assoc not found.");
}

void
sctp_set_assoc_filter(void)
{
struct sctp_analyse *u_data;

	/* Register the tap listener */
	if (sctp_stat_get_info()->is_registered == FALSE)
		register_tap_listener_sctp_stat();
	/* (redissect all packets) */

	u_data = (struct sctp_analyse *)g_malloc(sizeof(struct sctp_analyse));
	u_data->assoc        = NULL;
	u_data->children     = NULL;
	u_data->analyse_nb   = NULL;
	u_data->window       = NULL;
	u_data->num_children = 0;
	cf_retap_packets(&cfile);
	sctp_analyse_cb(u_data, TRUE);
	sctp_set_filter(NULL, u_data);
}

void
sctp_analyse_start(GtkAction *action _U_, gpointer user_data _U_)
{
	struct sctp_analyse *u_data;

	/* Register the tap listener */
	if (sctp_stat_get_info()->is_registered == FALSE)
		register_tap_listener_sctp_stat();
	/* (redissect all packets) */


	u_data = (struct sctp_analyse *)g_malloc(sizeof(struct sctp_analyse));
	u_data->assoc        = NULL;
	u_data->children     = NULL;
	u_data->analyse_nb   = NULL;
	u_data->window       = NULL;
	u_data->num_children = 0;

	cf_retap_packets(&cfile);
	sctp_analyse_cb(u_data, FALSE);
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
