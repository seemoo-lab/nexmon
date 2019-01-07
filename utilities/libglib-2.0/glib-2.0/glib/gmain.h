/* gmain.h - the GLib Main loop
 * Copyright (C) 1998-2000 Red Hat, Inc.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __G_MAIN_H__
#define __G_MAIN_H__

#if !defined (__GLIB_H_INSIDE__) && !defined (GLIB_COMPILATION)
#error "Only <glib.h> can be included directly."
#endif

#include <glib/gpoll.h>
#include <glib/gslist.h>
#include <glib/gthread.h>

G_BEGIN_DECLS

typedef enum /*< flags >*/
{
  G_IO_IN	GLIB_SYSDEF_POLLIN,
  G_IO_OUT	GLIB_SYSDEF_POLLOUT,
  G_IO_PRI	GLIB_SYSDEF_POLLPRI,
  G_IO_ERR	GLIB_SYSDEF_POLLERR,
  G_IO_HUP	GLIB_SYSDEF_POLLHUP,
  G_IO_NVAL	GLIB_SYSDEF_POLLNVAL
} GIOCondition;


/**
 * GMainContext:
 *
 * The `GMainContext` struct is an opaque data
 * type representing a set of sources to be handled in a main loop.
 */
typedef struct _GMainContext            GMainContext;

/**
 * GMainLoop:
 *
 * The `GMainLoop` struct is an opaque data type
 * representing the main event loop of a GLib or GTK+ application.
 */
typedef struct _GMainLoop               GMainLoop;

/**
 * GSource:
 *
 * The `GSource` struct is an opaque data type
 * representing an event source.
 */
typedef struct _GSource                 GSource;
typedef struct _GSourcePrivate          GSourcePrivate;

/**
 * GSourceCallbackFuncs:
 * @ref: Called when a reference is added to the callback object
 * @unref: Called when a reference to the callback object is dropped
 * @get: Called to extract the callback function and data from the
 *     callback object.

 * The `GSourceCallbackFuncs` struct contains
 * functions for managing callback objects.
 */
typedef struct _GSourceCallbackFuncs    GSourceCallbackFuncs;

/**
 * GSourceFuncs:
 * @prepare: Called before all the file descriptors are polled. If the
 *     source can determine that it is ready here (without waiting for the
 *     results of the poll() call) it should return %TRUE. It can also return
 *     a @timeout_ value which should be the maximum timeout (in milliseconds)
 *     which should be passed to the poll() call. The actual timeout used will
 *     be -1 if all sources returned -1, or it will be the minimum of all
 *     the @timeout_ values returned which were >= 0.  Since 2.36 this may
 *     be %NULL, in which case the effect is as if the function always returns
 *     %FALSE with a timeout of -1.  If @prepare returns a
 *     timeout and the source also has a 'ready time' set then the
 *     nearer of the two will be used.
 * @check: Called after all the file descriptors are polled. The source
 *     should return %TRUE if it is ready to be dispatched. Note that some
 *     time may have passed since the previous prepare function was called,
 *     so the source should be checked again here.  Since 2.36 this may
 *     be %NULL, in which case the effect is as if the function always returns
 *     %FALSE.
 * @dispatch: Called to dispatch the event source, after it has returned
 *     %TRUE in either its @prepare or its @check function. The @dispatch
 *     function is passed in a callback function and data. The callback
 *     function may be %NULL if the source was never connected to a callback
 *     using g_source_set_callback(). The @dispatch function should call the
 *     callback function with @user_data and whatever additional parameters
 *     are needed for this type of event source. The return value of the
 *     @dispatch function should be #G_SOURCE_REMOVE if the source should be
 *     removed or #G_SOURCE_CONTINUE to keep it.
 * @finalize: Called when the source is finalized.
 *
 * The `GSourceFuncs` struct contains a table of
 * functions used to handle event sources in a generic manner.
 *
 * For idle sources, the prepare and check functions always return %TRUE
 * to indicate that the source is always ready to be processed. The prepare
 * function also returns a timeout value of 0 to ensure that the poll() call
 * doesn't block (since that would be time wasted which could have been spent
 * running the idle function).
 *
 * For timeout sources, the prepare and check functions both return %TRUE
 * if the timeout interval has expired. The prepare function also returns
 * a timeout value to ensure that the poll() call doesn't block too long
 * and miss the next timeout.
 *
 * For file descriptor sources, the prepare function typically returns %FALSE,
 * since it must wait until poll() has been called before it knows whether
 * any events need to be processed. It sets the returned timeout to -1 to
 * indicate that it doesn't mind how long the poll() call blocks. In the
 * check function, it tests the results of the poll() call to see if the
 * required condition has been met, and returns %TRUE if so.
 */
typedef struct _GSourceFuncs            GSourceFuncs;

/**
 * GPid:
 *
 * A type which is used to hold a process identification.
 *
 * On UNIX, processes are identified by a process id (an integer),
 * while Windows uses process handles (which are pointers).
 *
 * GPid is used in GLib only for descendant processes spawned with
 * the g_spawn functions.
 */

/**
 * GSourceFunc:
 * @user_data: data passed to the function, set when the source was
 *     created with one of the above functions
 *
 * Specifies the type of function passed to g_timeout_add(),
 * g_timeout_add_full(), g_idle_add(), and g_idle_add_full().
 *
 * Returns: %FALSE if the source should be removed. #G_SOURCE_CONTINUE and
 * #G_SOURCE_REMOVE are more memorable names for the return value.
 */
typedef gboolean (*GSourceFunc)       (gpointer user_data);

/**
 * GChildWatchFunc:
 * @pid: the process id of the child process
 * @status: Status information about the child process, encoded
 *     in a platform-specific manner
 * @user_data: user data passed to g_child_watch_add()
 *
 * Prototype of a #GChildWatchSource callback, called when a child
 * process has exited.  To interpret @status, see the documentation
 * for g_spawn_check_exit_status().
 */
typedef void     (*GChildWatchFunc)   (GPid     pid,
                                       gint     status,
                                       gpointer user_data);
struct _GSource
{
  /*< private >*/
  gpointer callback_data;
  GSourceCallbackFuncs *callback_funcs;

  const GSourceFuncs *source_funcs;
  guint ref_count;

  GMainContext *context;

  gint priority;
  guint flags;
  guint source_id;

  GSList *poll_fds;
  
  GSource *prev;
  GSource *next;

  char    *name;

  GSourcePrivate *priv;
};

struct _GSourceCallbackFuncs
{
  void (*ref)   (gpointer     cb_data);
  void (*unref) (gpointer     cb_data);
  void (*get)   (gpointer     cb_data,
                 GSource     *source, 
                 GSourceFunc *func,
                 gpointer    *data);
};

/**
 * GSourceDummyMarshal:
 *
 * This is just a placeholder for #GClosureMarshal,
 * which cannot be used here for dependency reasons.
 */
typedef void (*GSourceDummyMarshal) (void);

struct _GSourceFuncs
{
  gboolean (*prepare)  (GSource    *source,
                        gint       *timeout_);
  gboolean (*check)    (GSource    *source);
  gboolean (*dispatch) (GSource    *source,
                        GSourceFunc callback,
                        gpointer    user_data);
  void     (*finalize) (GSource    *source); /* Can be NULL */

  /*< private >*/
  /* For use by g_source_set_closure */
  GSourceFunc     closure_callback;        
  GSourceDummyMarshal closure_marshal; /* Really is of type GClosureMarshal */
};

/* Standard priorities */

/**
 * G_PRIORITY_HIGH:
 *
 * Use this for high priority event sources.
 *
 * It is not used within GLib or GTK+.
 */
#define G_PRIORITY_HIGH            -100

/**
 * G_PRIORITY_DEFAULT:
 *
 * Use this for default priority event sources.
 *
 * In GLib this priority is used when adding timeout functions
 * with g_timeout_add(). In GDK this priority is used for events
 * from the X server.
 */
#define G_PRIORITY_DEFAULT          0

/**
 * G_PRIORITY_HIGH_IDLE:
 *
 * Use this for high priority idle functions.
 *
 * GTK+ uses #G_PRIORITY_HIGH_IDLE + 10 for resizing operations,
 * and #G_PRIORITY_HIGH_IDLE + 20 for redrawing operations. (This is
 * done to ensure that any pending resizes are processed before any
 * pending redraws, so that widgets are not redrawn twice unnecessarily.)
 */
#define G_PRIORITY_HIGH_IDLE        100

/**
 * G_PRIORITY_DEFAULT_IDLE:
 *
 * Use this for default priority idle functions.
 *
 * In GLib this priority is used when adding idle functions with
 * g_idle_add().
 */
#define G_PRIORITY_DEFAULT_IDLE     200

/**
 * G_PRIORITY_LOW:
 *
 * Use this for very low priority background tasks.
 *
 * It is not used within GLib or GTK+.
 */
#define G_PRIORITY_LOW              300

/**
 * G_SOURCE_REMOVE:
 *
 * Use this macro as the return value of a #GSourceFunc to remove
 * the #GSource from the main loop.
 *
 * Since: 2.32
 */
#define G_SOURCE_REMOVE         FALSE

/**
 * G_SOURCE_CONTINUE:
 *
 * Use this macro as the return value of a #GSourceFunc to leave
 * the #GSource in the main loop.
 *
 * Since: 2.32
 */
#define G_SOURCE_CONTINUE       TRUE

/* GMainContext: */

GLIB_AVAILABLE_IN_ALL
GMainContext *g_main_context_new       (void);
GLIB_AVAILABLE_IN_ALL
GMainContext *g_main_context_ref       (GMainContext *context);
GLIB_AVAILABLE_IN_ALL
void          g_main_context_unref     (GMainContext *context);
GLIB_AVAILABLE_IN_ALL
GMainContext *g_main_context_default   (void);

GLIB_AVAILABLE_IN_ALL
gboolean      g_main_context_iteration (GMainContext *context,
                                        gboolean      may_block);
GLIB_AVAILABLE_IN_ALL
gboolean      g_main_context_pending   (GMainContext *context);

/* For implementation of legacy interfaces
 */
GLIB_AVAILABLE_IN_ALL
GSource      *g_main_context_find_source_by_id              (GMainContext *context,
                                                             guint         source_id);
GLIB_AVAILABLE_IN_ALL
GSource      *g_main_context_find_source_by_user_data       (GMainContext *context,
                                                             gpointer      user_data);
GLIB_AVAILABLE_IN_ALL
GSource      *g_main_context_find_source_by_funcs_user_data (GMainContext *context,
                                                             GSourceFuncs *funcs,
                                                             gpointer      user_data);

/* Low level functions for implementing custom main loops.
 */
GLIB_AVAILABLE_IN_ALL
void     g_main_context_wakeup  (GMainContext *context);
GLIB_AVAILABLE_IN_ALL
gboolean g_main_context_acquire (GMainContext *context);
GLIB_AVAILABLE_IN_ALL
void     g_main_context_release (GMainContext *context);
GLIB_AVAILABLE_IN_ALL
gboolean g_main_context_is_owner (GMainContext *context);
GLIB_AVAILABLE_IN_ALL
gboolean g_main_context_wait    (GMainContext *context,
                                 GCond        *cond,
                                 GMutex       *mutex);

GLIB_AVAILABLE_IN_ALL
gboolean g_main_context_prepare  (GMainContext *context,
                                  gint         *priority);
GLIB_AVAILABLE_IN_ALL
gint     g_main_context_query    (GMainContext *context,
                                  gint          max_priority,
                                  gint         *timeout_,
                                  GPollFD      *fds,
                                  gint          n_fds);
GLIB_AVAILABLE_IN_ALL
gint     g_main_context_check    (GMainContext *context,
                                  gint          max_priority,
                                  GPollFD      *fds,
                                  gint          n_fds);
GLIB_AVAILABLE_IN_ALL
void     g_main_context_dispatch (GMainContext *context);

GLIB_AVAILABLE_IN_ALL
void     g_main_context_set_poll_func (GMainContext *context,
                                       GPollFunc     func);
GLIB_AVAILABLE_IN_ALL
GPollFunc g_main_context_get_poll_func (GMainContext *context);

/* Low level functions for use by source implementations
 */
GLIB_AVAILABLE_IN_ALL
void     g_main_context_add_poll    (GMainContext *context,
                                     GPollFD      *fd,
                                     gint          priority);
GLIB_AVAILABLE_IN_ALL
void     g_main_context_remove_poll (GMainContext *context,
                                     GPollFD      *fd);

GLIB_AVAILABLE_IN_ALL
gint     g_main_depth               (void);
GLIB_AVAILABLE_IN_ALL
GSource *g_main_current_source      (void);

/* GMainContexts for other threads
 */
GLIB_AVAILABLE_IN_ALL
void          g_main_context_push_thread_default (GMainContext *context);
GLIB_AVAILABLE_IN_ALL
void          g_main_context_pop_thread_default  (GMainContext *context);
GLIB_AVAILABLE_IN_ALL
GMainContext *g_main_context_get_thread_default  (void);
GLIB_AVAILABLE_IN_ALL
GMainContext *g_main_context_ref_thread_default  (void);

/* GMainLoop: */

GLIB_AVAILABLE_IN_ALL
GMainLoop *g_main_loop_new        (GMainContext *context,
                                   gboolean      is_running);
GLIB_AVAILABLE_IN_ALL
void       g_main_loop_run        (GMainLoop    *loop);
GLIB_AVAILABLE_IN_ALL
void       g_main_loop_quit       (GMainLoop    *loop);
GLIB_AVAILABLE_IN_ALL
GMainLoop *g_main_loop_ref        (GMainLoop    *loop);
GLIB_AVAILABLE_IN_ALL
void       g_main_loop_unref      (GMainLoop    *loop);
GLIB_AVAILABLE_IN_ALL
gboolean   g_main_loop_is_running (GMainLoop    *loop);
GLIB_AVAILABLE_IN_ALL
GMainContext *g_main_loop_get_context (GMainLoop    *loop);

/* GSource: */

GLIB_AVAILABLE_IN_ALL
GSource *g_source_new             (GSourceFuncs   *source_funcs,
                                   guint           struct_size);
GLIB_AVAILABLE_IN_ALL
GSource *g_source_ref             (GSource        *source);
GLIB_AVAILABLE_IN_ALL
void     g_source_unref           (GSource        *source);

GLIB_AVAILABLE_IN_ALL
guint    g_source_attach          (GSource        *source,
                                   GMainContext   *context);
GLIB_AVAILABLE_IN_ALL
void     g_source_destroy         (GSource        *source);

GLIB_AVAILABLE_IN_ALL
void     g_source_set_priority    (GSource        *source,
                                   gint            priority);
GLIB_AVAILABLE_IN_ALL
gint     g_source_get_priority    (GSource        *source);
GLIB_AVAILABLE_IN_ALL
void     g_source_set_can_recurse (GSource        *source,
                                   gboolean        can_recurse);
GLIB_AVAILABLE_IN_ALL
gboolean g_source_get_can_recurse (GSource        *source);
GLIB_AVAILABLE_IN_ALL
guint    g_source_get_id          (GSource        *source);

GLIB_AVAILABLE_IN_ALL
GMainContext *g_source_get_context (GSource       *source);

GLIB_AVAILABLE_IN_ALL
void     g_source_set_callback    (GSource        *source,
                                   GSourceFunc     func,
                                   gpointer        data,
                                   GDestroyNotify  notify);

GLIB_AVAILABLE_IN_ALL
void     g_source_set_funcs       (GSource        *source,
                                   GSourceFuncs   *funcs);
GLIB_AVAILABLE_IN_ALL
gboolean g_source_is_destroyed    (GSource        *source);

GLIB_AVAILABLE_IN_ALL
void                 g_source_set_name       (GSource        *source,
                                              const char     *name);
GLIB_AVAILABLE_IN_ALL
const char *         g_source_get_name       (GSource        *source);
GLIB_AVAILABLE_IN_ALL
void                 g_source_set_name_by_id (guint           tag,
                                              const char     *name);

GLIB_AVAILABLE_IN_2_36
void                 g_source_set_ready_time (GSource        *source,
                                              gint64          ready_time);
GLIB_AVAILABLE_IN_2_36
gint64               g_source_get_ready_time (GSource        *source);

#ifdef G_OS_UNIX
GLIB_AVAILABLE_IN_2_36
gpointer             g_source_add_unix_fd    (GSource        *source,
                                              gint            fd,
                                              GIOCondition    events);
GLIB_AVAILABLE_IN_2_36
void                 g_source_modify_unix_fd (GSource        *source,
                                              gpointer        tag,
                                              GIOCondition    new_events);
GLIB_AVAILABLE_IN_2_36
void                 g_source_remove_unix_fd (GSource        *source,
                                              gpointer        tag);
GLIB_AVAILABLE_IN_2_36
GIOCondition         g_source_query_unix_fd  (GSource        *source,
                                              gpointer        tag);
#endif

/* Used to implement g_source_connect_closure and internally*/
GLIB_AVAILABLE_IN_ALL
void g_source_set_callback_indirect (GSource              *source,
                                     gpointer              callback_data,
                                     GSourceCallbackFuncs *callback_funcs);

GLIB_AVAILABLE_IN_ALL
void     g_source_add_poll            (GSource        *source,
				       GPollFD        *fd);
GLIB_AVAILABLE_IN_ALL
void     g_source_remove_poll         (GSource        *source,
				       GPollFD        *fd);

GLIB_AVAILABLE_IN_ALL
void     g_source_add_child_source    (GSource        *source,
				       GSource        *child_source);
GLIB_AVAILABLE_IN_ALL
void     g_source_remove_child_source (GSource        *source,
				       GSource        *child_source);

GLIB_DEPRECATED_IN_2_28_FOR(g_source_get_time)
void     g_source_get_current_time (GSource        *source,
                                    GTimeVal       *timeval);

GLIB_AVAILABLE_IN_ALL
gint64   g_source_get_time         (GSource        *source);

 /* void g_source_connect_closure (GSource        *source,
                                  GClosure       *closure);
 */

/* Specific source types
 */
GLIB_AVAILABLE_IN_ALL
GSource *g_idle_source_new        (void);
GLIB_AVAILABLE_IN_ALL
GSource *g_child_watch_source_new (GPid pid);
GLIB_AVAILABLE_IN_ALL
GSource *g_timeout_source_new     (guint interval);
GLIB_AVAILABLE_IN_ALL
GSource *g_timeout_source_new_seconds (guint interval);

/* Miscellaneous functions
 */
GLIB_AVAILABLE_IN_ALL
void   g_get_current_time                 (GTimeVal       *result);
GLIB_AVAILABLE_IN_ALL
gint64 g_get_monotonic_time               (void);
GLIB_AVAILABLE_IN_ALL
gint64 g_get_real_time                    (void);


/* Source manipulation by ID */
GLIB_AVAILABLE_IN_ALL
gboolean g_source_remove                     (guint          tag);
GLIB_AVAILABLE_IN_ALL
gboolean g_source_remove_by_user_data        (gpointer       user_data);
GLIB_AVAILABLE_IN_ALL
gboolean g_source_remove_by_funcs_user_data  (GSourceFuncs  *funcs,
                                              gpointer       user_data);

/* Idles, child watchers and timeouts */
GLIB_AVAILABLE_IN_ALL
guint    g_timeout_add_full         (gint            priority,
                                     guint           interval,
                                     GSourceFunc     function,
                                     gpointer        data,
                                     GDestroyNotify  notify);
GLIB_AVAILABLE_IN_ALL
guint    g_timeout_add              (guint           interval,
                                     GSourceFunc     function,
                                     gpointer        data);
GLIB_AVAILABLE_IN_ALL
guint    g_timeout_add_seconds_full (gint            priority,
                                     guint           interval,
                                     GSourceFunc     function,
                                     gpointer        data,
                                     GDestroyNotify  notify);
GLIB_AVAILABLE_IN_ALL
guint    g_timeout_add_seconds      (guint           interval,
                                     GSourceFunc     function,
                                     gpointer        data);
GLIB_AVAILABLE_IN_ALL
guint    g_child_watch_add_full     (gint            priority,
                                     GPid            pid,
                                     GChildWatchFunc function,
                                     gpointer        data,
                                     GDestroyNotify  notify);
GLIB_AVAILABLE_IN_ALL
guint    g_child_watch_add          (GPid            pid,
                                     GChildWatchFunc function,
                                     gpointer        data);
GLIB_AVAILABLE_IN_ALL
guint    g_idle_add                 (GSourceFunc     function,
                                     gpointer        data);
GLIB_AVAILABLE_IN_ALL
guint    g_idle_add_full            (gint            priority,
                                     GSourceFunc     function,
                                     gpointer        data,
                                     GDestroyNotify  notify);
GLIB_AVAILABLE_IN_ALL
gboolean g_idle_remove_by_data      (gpointer        data);

GLIB_AVAILABLE_IN_ALL
void     g_main_context_invoke_full (GMainContext   *context,
                                     gint            priority,
                                     GSourceFunc     function,
                                     gpointer        data,
                                     GDestroyNotify  notify);
GLIB_AVAILABLE_IN_ALL
void     g_main_context_invoke      (GMainContext   *context,
                                     GSourceFunc     function,
                                     gpointer        data);

/* Hook for GClosure / GSource integration. Don't touch */
GLIB_VAR GSourceFuncs g_timeout_funcs;
GLIB_VAR GSourceFuncs g_child_watch_funcs;
GLIB_VAR GSourceFuncs g_idle_funcs;
#ifdef G_OS_UNIX
GLIB_VAR GSourceFuncs g_unix_signal_funcs;
GLIB_VAR GSourceFuncs g_unix_fd_source_funcs;
#endif

G_END_DECLS

#endif /* __G_MAIN_H__ */
