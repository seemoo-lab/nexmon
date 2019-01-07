/* tap.c
 * packet tap interface   2002 Ronnie Sahlberg
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

#include <config.h>

#include <stdio.h>

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#ifdef HAVE_NETINET_IN_H
# include <netinet/in.h>
#endif

#include <string.h>

#include <glib.h>

#include <epan/packet_info.h>
#include <epan/dfilter/dfilter.h>
#include <epan/tap.h>

static gboolean tapping_is_active=FALSE;

typedef struct _tap_dissector_t {
	struct _tap_dissector_t *next;
	char *name;
} tap_dissector_t;
static tap_dissector_t *tap_dissector_list=NULL;

/*
 * This is the list of free and used packets queued for a tap.
 * It is implemented here explicitly instead of using GLib objects
 * in order to be as fast as possible as we need to build and tear down the
 * queued list at least once for each packet we see and thus we must be able
 * to build and tear it down as fast as possible.
 *
 * XXX - some fields in packet_info get overwritten in the dissection
 * process, such as the addresses and the "this is an error packet" flag.
 * A packet may be queued at multiple protocol layers, but the packet_info
 * structure will, when the tap listeners are run, contain the values as
 * set by the topmost protocol layers.
 *
 * This means that the tap listener code can't rely on pinfo->flags.in_error_pkt
 * to determine whether the packet should be handed to the listener, as, for
 * a protocol with error report packets that include a copy of the
 * packet in error (ICMP, ICMPv6, CLNP), that flag changes during the
 * processing of the packet depending on whether we're currently dissecting
 * the packet in error or not.
 *
 *
 * It also means that a tap listener can't depend on the source and destination
 * addresses being the correct ones for the packet being processed if, for
 * example, you have some tunneling that causes multiple layers of the same
 * protocol.
 *
 * For now, we handle the error packet flag by setting a bit in the flags
 * field of the tap_packet_t structure.  We may ultimately want stacks of
 * addresses for this and other reasons.
 */
typedef struct _tap_packet_t {
	int tap_id;
	guint32 flags;
	packet_info *pinfo;
	const void *tap_specific_data;
} tap_packet_t;

#define TAP_PACKET_IS_ERROR_PACKET	0x00000001	/* packet being queued is an error packet */

#define TAP_PACKET_QUEUE_LEN 5000
static tap_packet_t tap_packet_array[TAP_PACKET_QUEUE_LEN];
static guint tap_packet_index;

typedef struct _tap_listener_t {
	volatile struct _tap_listener_t *next;
	int tap_id;
	gboolean needs_redraw;
	guint flags;
	gchar *fstring;
	dfilter_t *code;
	void *tapdata;
	tap_reset_cb reset;
	tap_packet_cb packet;
	tap_draw_cb draw;
} tap_listener_t;
static volatile tap_listener_t *tap_listener_queue=NULL;

#ifdef HAVE_PLUGINS

#include <gmodule.h>

#include <wsutil/plugins.h>

/*
 * List of tap plugins.
 */
typedef struct {
	void (*register_tap_listener_fn)(void);   /* routine to call to register tap listener */
} tap_plugin;

static GSList *tap_plugins = NULL;

/*
 * Callback for each plugin found.
 */
static gboolean
check_for_tap_plugin(GModule *handle)
{
	gpointer gp;
	void (*register_tap_listener_fn)(void);
	tap_plugin *plugin;

	/*
	 * Do we have a register_tap_listener routine?
	 */
	if (!g_module_symbol(handle, "plugin_register_tap_listener", &gp)) {
		/* No, so this isn't a tap plugin. */
		return FALSE;
	}

	/*
	 * Yes - this plugin includes one or more taps.
	 */
DIAG_OFF(pedantic)
	register_tap_listener_fn = (void (*)(void))gp;
DIAG_ON(pedantic)

	/*
	 * Add this one to the list of tap plugins.
	 */
	plugin = (tap_plugin *)g_malloc(sizeof (tap_plugin));
	plugin->register_tap_listener_fn = register_tap_listener_fn;
	tap_plugins = g_slist_append(tap_plugins, plugin);
	return TRUE;
}

void
register_tap_plugin_type(void)
{
	add_plugin_type("tap", check_for_tap_plugin);
}

static void
register_tap_plugin_listener(gpointer data, gpointer user_data _U_)
{
	tap_plugin *plugin = (tap_plugin *)data;

	(plugin->register_tap_listener_fn)();
}

/*
 * For all tap plugins, call their register routines.
 */
void
register_all_plugin_tap_listeners(void)
{
	g_slist_foreach(tap_plugins, register_tap_plugin_listener, NULL);
}
#endif /* HAVE_PLUGINS */

/* **********************************************************************
 * Init routine only called from epan at application startup
 * ********************************************************************** */
/* This function is called once when wireshark starts up and is used
   to init any data structures we may need later.
*/
void
tap_init(void)
{
	tap_packet_index=0;
}

/* **********************************************************************
 * Functions called from dissector when made tappable
 * ********************************************************************** */
/* the following two functions are used from dissectors to
   1. register the ability to tap packets from this subdissector
   2. push packets encountered by the subdissector to anyone tapping
*/

/* This function registers that a dissector has the packet tap ability
   available.  The name parameter is the name of this tap and extensions can
   use open_tap(char *name,... to specify that it wants to receive packets/
   events from this tap.

   This function is only to be called once, when the dissector initializes.

   The return value from this call is later used as a parameter to the
   tap_packet(unsigned int *tap_id,...
   call so that the tap subsystem knows to which tap point this tapped
   packet is associated.
*/
int
register_tap(const char *name)
{
	tap_dissector_t *td, *tdl;
	int i, tap_id;

	if(tap_dissector_list){
		tap_id=find_tap_id(name);
		if (tap_id)
			return tap_id;
	}

	td=(tap_dissector_t *)g_malloc(sizeof(tap_dissector_t));
	td->next=NULL;
	td->name = g_strdup(name);

	if(!tap_dissector_list){
		tap_dissector_list=td;
		i=1;
	} else {
		for(i=2,tdl=tap_dissector_list;tdl->next;i++,tdl=tdl->next)
			;
		tdl->next=td;
	}
	return i;
}


/* Everytime the dissector has finished dissecting a packet (and all
   subdissectors have returned) and if the dissector has been made "tappable"
   it will push some data to everyone tapping this layer by a call
   to tap_queue_packet().
   The first parameter is the tap_id returned by the register_tap()
   call for this dissector (so the tap system can keep track of who it came
   from and who is listening to it)
   The second is the packet_info structure which many tap readers will find
   interesting.
   The third argument is specific to each tap point or NULL if no additional
   data is available to this tap.  A tap point in say IP will probably want to
   push the IP header structure here. Same thing for TCP and ONCRPC.

   The pinfo and the specific pointer are what is supplied to every listener
   in the read_callback() call made to every one currently listening to this
   tap.

   The tap reader is responsible to know how to parse any structure pointed
   to by the tap specific data pointer.
*/
void
tap_queue_packet(int tap_id, packet_info *pinfo, const void *tap_specific_data)
{
	tap_packet_t *tpt;

	if(!tapping_is_active){
		return;
	}
	/*
	 * XXX - should we allocate this with an ep_allocator,
	 * rather than having a fixed maximum number of entries?
	 */
	if(tap_packet_index >= TAP_PACKET_QUEUE_LEN){
		g_warning("Too many taps queued");
		return;
	}

	tpt=&tap_packet_array[tap_packet_index];
	tpt->tap_id=tap_id;
	tpt->flags = 0;
	if (pinfo->flags.in_error_pkt)
		tpt->flags |= TAP_PACKET_IS_ERROR_PACKET;
	tpt->pinfo=pinfo;
	tpt->tap_specific_data=tap_specific_data;
	tap_packet_index++;
}





/* **********************************************************************
 * Functions used by file.c to drive the tap subsystem
 * ********************************************************************** */

void tap_build_interesting (epan_dissect_t *edt)
{
	volatile tap_listener_t *tl;

	/* nothing to do, just return */
	if(!tap_listener_queue){
		return;
	}

	/* loop over all tap listeners and build the list of all
	   interesting hf_fields */
	for(tl=tap_listener_queue;tl;tl=tl->next){
		if(tl->code){
			epan_dissect_prime_dfilter(edt, tl->code);
		}
	}
}

/* This function is used to delete/initialize the tap queue and prime an
   epan_dissect_t with all the filters for tap listeners.
   To free the tap queue, we just prepend the used queue to the free queue.
*/
void
tap_queue_init(epan_dissect_t *edt)
{
	/* nothing to do, just return */
	if(!tap_listener_queue){
		return;
	}

	tapping_is_active=TRUE;

	tap_packet_index=0;

	tap_build_interesting (edt);
}

/* this function is called after a packet has been fully dissected to push the tapped
   data to all extensions that has callbacks registered.
*/
void
tap_push_tapped_queue(epan_dissect_t *edt)
{
	tap_packet_t *tp;
	volatile tap_listener_t *tl;
	guint i;

	/* nothing to do, just return */
	if(!tapping_is_active){
		return;
	}

	tapping_is_active=FALSE;

	/* nothing to do, just return */
	if(!tap_packet_index){
		return;
	}

	/* loop over all tap listeners and call the listener callback
	   for all packets that match the filter. */
	for(i=0;i<tap_packet_index;i++){
		for(tl=tap_listener_queue;tl;tl=tl->next){
			tp=&tap_packet_array[i];
			/* Don't tap the packet if it's an "error" unless the listener tells us to */
			if (!(tp->flags & TAP_PACKET_IS_ERROR_PACKET) || (tl->flags & TL_REQUIRES_ERROR_PACKETS))
			{
				if(tp->tap_id==tl->tap_id){
					gboolean passed=TRUE;
					if(tl->code){
						passed=dfilter_apply_edt(tl->code, edt);
					}
					if(passed && tl->packet){
						tl->needs_redraw|=tl->packet(tl->tapdata, tp->pinfo, edt, tp->tap_specific_data);
					}
				}
            }
		}
	}
}


/* This function can be used by a dissector to fetch any tapped data before
 * returning.
 * This can be useful if one wants to extract the data inside dissector  BEFORE
 * it exists as an alternative to the callbacks that are all called AFTER the
 * dissection has completed.
 *
 * Example: SMB2 uses this mechanism to extract the data tapped from NTLMSSP
 * containing the account and domain names before exiting.
 * Note that the SMB2 tap listener specifies all three callbacks as NULL.
 *
 * Beware: when using this mechanism to extract the tapped data you can not
 * use "filters" and should specify the "filter" as NULL when registering
 * the tap listener.
 */
const void *
fetch_tapped_data(int tap_id, int idx)
{
	tap_packet_t *tp;
	guint i;

	/* nothing to do, just return */
	if(!tapping_is_active){
		return NULL;
	}

	/* nothing to do, just return */
	if(!tap_packet_index){
		return NULL;
	}

	/* loop over all tapped packets and return the one with index idx */
	for(i=0;i<tap_packet_index;i++){
		tp=&tap_packet_array[i];
		if(tp->tap_id==tap_id){
			if(!idx--){
				return tp->tap_specific_data;
			}
		}
	}

	return NULL;
}

/* This function is called when we need to reset all tap listeners, for example
   when we open/start a new capture or if we need to rescan the packet list.
*/
void
reset_tap_listeners(void)
{
	volatile tap_listener_t *tl;

	for(tl=tap_listener_queue;tl;tl=tl->next){
		if(tl->reset){
			tl->reset(tl->tapdata);
		}
		tl->needs_redraw=TRUE;
	}

}


/* This function is called when we need to redraw all tap listeners, for example
   when we open/start a new capture or if we need to rescan the packet list.
   It should be called from a low priority thread say once every 3 seconds

   If draw_all is true, redraw all aplications regardless if they have
   changed or not.
*/
void
draw_tap_listeners(gboolean draw_all)
{
	volatile tap_listener_t *tl;

	for(tl=tap_listener_queue;tl;tl=tl->next){
		if(tl->needs_redraw || draw_all){
			if(tl->draw){
				tl->draw(tl->tapdata);
			}
		}
		tl->needs_redraw=FALSE;
	}
}

/* Gets a GList of the tap names. The content of the list
   is owned by the tap table and should not be modified or freed.
   Use g_list_free() when done using the list. */
GList*
get_tap_names(void)
{
	GList *list = NULL;
	tap_dissector_t *td;

	for(td=tap_dissector_list; td; td=td->next) {
		list = g_list_prepend(list, td->name);
	}

	return g_list_reverse(list);
}

/* **********************************************************************
 * Functions used by tap to
 * 1. register that a really simple extension is available for use by
 *    Wireshark.
 * 2. start tapping from a subdissector
 * 3. close an already open tap
 * ********************************************************************** */
/* this function will return the tap_id for the specific protocol tap
   or 0 if no such tap was found.
 */
int
find_tap_id(const char *name)
{
	tap_dissector_t *td;
	int i;

	for(i=1,td=tap_dissector_list;td;i++,td=td->next) {
		if(!strcmp(td->name,name)){
			return i;
		}
	}
	return 0;
}

static void
free_tap_listener(volatile tap_listener_t *tl)
{
	if(!tl)
		return;
	if(tl->code){
		dfilter_free(tl->code);
	}
	g_free(tl->fstring);
DIAG_OFF(cast-qual)
	g_free((gpointer)tl);
DIAG_ON(cast-qual)
}

/* this function attaches the tap_listener to the named tap.
 * function returns :
 *     NULL: ok.
 * non-NULL: error, return value points to GString containing error
 *           message.
 */
GString *
register_tap_listener(const char *tapname, void *tapdata, const char *fstring,
		      guint flags, tap_reset_cb reset, tap_packet_cb packet, tap_draw_cb draw)
{
	volatile tap_listener_t *tl;
	int tap_id;
	dfilter_t *code=NULL;
	GString *error_string;
	gchar *err_msg;

	tap_id=find_tap_id(tapname);
	if(!tap_id){
		error_string = g_string_new("");
		g_string_printf(error_string, "Tap %s not found", tapname);
		return error_string;
	}

	tl=(volatile tap_listener_t *)g_malloc0(sizeof(tap_listener_t));
	tl->needs_redraw=TRUE;
	tl->flags=flags;
	if(fstring){
		if(!dfilter_compile(fstring, &code, &err_msg)){
			error_string = g_string_new("");
			g_string_printf(error_string,
			    "Filter \"%s\" is invalid - %s",
			    fstring, err_msg);
			g_free(err_msg);
			free_tap_listener(tl);
			return error_string;
		}
	}
	tl->fstring=g_strdup(fstring);
	tl->code=code;

	tl->tap_id=tap_id;
	tl->tapdata=tapdata;
	tl->reset=reset;
	tl->packet=packet;
	tl->draw=draw;
	tl->next=tap_listener_queue;

	tap_listener_queue=tl;

	return NULL;
}

/* this function sets a new dfilter to a tap listener
 */
GString *
set_tap_dfilter(void *tapdata, const char *fstring)
{
	volatile tap_listener_t *tl=NULL,*tl2;
	dfilter_t *code=NULL;
	GString *error_string;
	gchar *err_msg;

	if(!tap_listener_queue){
		return NULL;
	}

	if(tap_listener_queue->tapdata==tapdata){
		tl=tap_listener_queue;
	} else {
		for(tl2=tap_listener_queue;tl2->next;tl2=tl2->next){
			if(tl2->next->tapdata==tapdata){
				tl=tl2->next;
				break;
			}

		}
	}

	if(tl){
		if(tl->code){
			dfilter_free(tl->code);
			tl->code=NULL;
		}
		tl->needs_redraw=TRUE;
		g_free(tl->fstring);
		if(fstring){
			if(!dfilter_compile(fstring, &code, &err_msg)){
				tl->fstring=NULL;
				error_string = g_string_new("");
				g_string_printf(error_string,
						 "Filter \"%s\" is invalid - %s",
						 fstring, err_msg);
				g_free(err_msg);
				return error_string;
			}
		}
		tl->fstring=g_strdup(fstring);
		tl->code=code;
	}

	return NULL;
}

/* this function recompiles dfilter for all registered tap listeners
 */
void
tap_listeners_dfilter_recompile(void)
{
	volatile tap_listener_t *tl;
	dfilter_t *code;
	gchar *err_msg;

	for(tl=tap_listener_queue;tl;tl=tl->next){
		if(tl->code){
			dfilter_free(tl->code);
			tl->code=NULL;
		}
		tl->needs_redraw=TRUE;
		code=NULL;
		if(tl->fstring){
			if(!dfilter_compile(tl->fstring, &code, &err_msg)){
				g_free(err_msg);
				err_msg = NULL;
				/* Not valid, make a dfilter matching no packets */
				if (!dfilter_compile("frame.number == 0", &code, &err_msg))
					g_free(err_msg);
			}
		}
		tl->code=code;
	}
}

/* this function removes a tap listener
 */
void
remove_tap_listener(void *tapdata)
{
	volatile tap_listener_t *tl=NULL,*tl2;

	if(!tap_listener_queue){
		return;
	}

	if(tap_listener_queue->tapdata==tapdata){
		tl=tap_listener_queue;
		tap_listener_queue=tap_listener_queue->next;
	} else {
		for(tl2=tap_listener_queue;tl2->next;tl2=tl2->next){
			if(tl2->next->tapdata==tapdata){
				tl=tl2->next;
				tl2->next=tl2->next->next;
				break;
			}

		}
	}
	free_tap_listener(tl);
}

/*
 * Return TRUE if we have one or more tap listeners that require dissection,
 * FALSE otherwise.
 */
gboolean
tap_listeners_require_dissection(void)
{
	volatile tap_listener_t *tap_queue = tap_listener_queue;

	while(tap_queue) {
		if(!(tap_queue->flags & TL_IS_DISSECTOR_HELPER))
			return TRUE;

		tap_queue = tap_queue->next;
	}

	return FALSE;

}

/* Returns TRUE there is an active tap listener for the specified tap id. */
gboolean
have_tap_listener(int tap_id)
{
	volatile tap_listener_t *tap_queue = tap_listener_queue;

	while(tap_queue) {
		if(tap_queue->tap_id == tap_id)
			return TRUE;

		tap_queue = tap_queue->next;
	}

	return FALSE;
}

/*
 * Return TRUE if we have any tap listeners with filters, FALSE otherwise.
 */
gboolean
have_filtering_tap_listeners(void)
{
	volatile tap_listener_t *tl;

	for(tl=tap_listener_queue;tl;tl=tl->next){
		if(tl->code)
			return TRUE;
	}
	return FALSE;
}

/*
 * Get the union of all the flags for all the tap listeners; that gives
 * an indication of whether the protocol tree, or the columns, are
 * required by any taps.
 */
guint
union_of_tap_listener_flags(void)
{
	volatile tap_listener_t *tl;
	guint flags = 0;

	for(tl=tap_listener_queue;tl;tl=tl->next){
		flags|=tl->flags;
	}
	return flags;
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
