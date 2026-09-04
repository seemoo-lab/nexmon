/* GIO - GLib Input, Output and Streaming Library
 * 
 * Copyright (C) 2006-2007 Red Hat, Inc.
 * Copyright (C) 2022-2024 Canonical, Ltd.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Author: Alexander Larsson <alexl@redhat.com>
 * Author: Marco Trevisan <marco.trevisan@canonical.com>
 */

#include "config.h"
#include "glib.h"
#include <gioerror.h>
#include "glib-private.h"
#include "gcancellable.h"
#include "glibintl.h"


/**
 * GCancellable:
 *
 * `GCancellable` allows operations to be cancelled.
 *
 * `GCancellable` is a thread-safe operation cancellation stack used
 * throughout GIO to allow for cancellation of synchronous and
 * asynchronous operations.
 */

enum {
  CANCELLED,
  LAST_SIGNAL
};

struct _GCancellablePrivate
{
  /* Atomic so that we don't require holding global mutexes for independent ops. */
  gboolean cancelled;
  int cancelled_running;

  /* Access to fields below is protected by cancellable's mutex. */
  GMutex mutex;
  guint fd_refcount;
  GWakeup *wakeup;
};

static guint signals[LAST_SIGNAL] = { 0 };

G_DEFINE_TYPE_WITH_PRIVATE (GCancellable, g_cancellable, G_TYPE_OBJECT)

static GPrivate current_cancellable;
static GCond cancellable_cond;

static void
g_cancellable_finalize (GObject *object)
{
  GCancellable *cancellable = G_CANCELLABLE (object);

  /* We're at finalization phase, so only one thread can be here.
   * Thus there's no need to lock. In case something is locking us, then we've
   * a bug, and g_mutex_clear() will make this clear aborting.
   */
  if (cancellable->priv->wakeup)
    GLIB_PRIVATE_CALL (g_wakeup_free) (cancellable->priv->wakeup);

  g_mutex_clear (&cancellable->priv->mutex);

  G_OBJECT_CLASS (g_cancellable_parent_class)->finalize (object);
}

static void
g_cancellable_class_init (GCancellableClass *klass)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (klass);

  gobject_class->finalize = g_cancellable_finalize;

  /**
   * GCancellable::cancelled:
   * @cancellable: a #GCancellable.
   * 
   * Emitted when the operation has been cancelled.
   * 
   * Can be used by implementations of cancellable operations. If the
   * operation is cancelled from another thread, the signal will be
   * emitted in the thread that cancelled the operation, not the
   * thread that is running the operation.
   *
   * Note that disconnecting from this signal (or any signal) in a
   * multi-threaded program is prone to race conditions. For instance
   * it is possible that a signal handler may be invoked even after
   * a call to g_signal_handler_disconnect() for that handler has
   * already returned.
   * 
   * There is also a problem when cancellation happens right before
   * connecting to the signal. If this happens the signal will
   * unexpectedly not be emitted, and checking before connecting to
   * the signal leaves a race condition where this is still happening.
   *
   * In order to make it safe and easy to connect handlers there
   * are two helper functions: g_cancellable_connect() and
   * g_cancellable_disconnect() which protect against problems
   * like this.
   *
   * An example of how to us this:
   * |[<!-- language="C" -->
   *     // Make sure we don't do unnecessary work if already cancelled
   *     if (g_cancellable_set_error_if_cancelled (cancellable, error))
   *       return;
   *
   *     // Set up all the data needed to be able to handle cancellation
   *     // of the operation
   *     my_data = my_data_new (...);
   *
   *     id = 0;
   *     if (cancellable)
   *       id = g_cancellable_connect (cancellable,
   *     			      G_CALLBACK (cancelled_handler)
   *     			      data, NULL);
   *
   *     // cancellable operation here...
   *
   *     g_cancellable_disconnect (cancellable, id);
   *
   *     // cancelled_handler is never called after this, it is now safe
   *     // to free the data
   *     my_data_free (my_data);  
   * ]|
   *
   * Note that the cancelled signal is emitted in the thread that
   * the user cancelled from, which may be the main thread. So, the
   * cancellable signal should not do something that can block.
   */
  signals[CANCELLED] =
    g_signal_new (I_("cancelled"),
		  G_TYPE_FROM_CLASS (gobject_class),
		  G_SIGNAL_RUN_LAST,
		  G_STRUCT_OFFSET (GCancellableClass, cancelled),
		  NULL, NULL,
		  NULL,
		  G_TYPE_NONE, 0);
  
}

static void
g_cancellable_init (GCancellable *cancellable)
{
  cancellable->priv = g_cancellable_get_instance_private (cancellable);

  g_mutex_init (&cancellable->priv->mutex);
}

/**
 * g_cancellable_new:
 * 
 * Creates a new #GCancellable object.
 *
 * Applications that want to start one or more operations
 * that should be cancellable should create a #GCancellable
 * and pass it to the operations.
 *
 * One #GCancellable can be used in multiple consecutive
 * operations or in multiple concurrent operations.
 *  
 * Returns: a #GCancellable.
 **/
GCancellable *
g_cancellable_new (void)
{
  return g_object_new (G_TYPE_CANCELLABLE, NULL);
}

/**
 * g_cancellable_push_current:
 * @cancellable: (not nullable): a #GCancellable object
 *
 * Pushes @cancellable onto the cancellable stack. The current
 * cancellable can then be received using g_cancellable_get_current().
 *
 * This is useful when implementing cancellable operations in
 * code that does not allow you to pass down the cancellable object.
 *
 * This is typically called automatically by e.g. #GFile operations,
 * so you rarely have to call this yourself.
 **/
void
g_cancellable_push_current (GCancellable *cancellable)
{
  GSList *l;

  g_return_if_fail (cancellable != NULL);

  l = g_private_get (&current_cancellable);
  l = g_slist_prepend (l, cancellable);
  g_private_set (&current_cancellable, l);
}

/**
 * g_cancellable_pop_current:
 * @cancellable: (not nullable): a #GCancellable object
 *
 * Pops @cancellable off the cancellable stack (verifying that @cancellable
 * is on the top of the stack).
 **/
void
g_cancellable_pop_current (GCancellable *cancellable)
{
  GSList *l;

  l = g_private_get (&current_cancellable);

  g_return_if_fail (l != NULL);
  g_return_if_fail (l->data == cancellable);

  l = g_slist_delete_link (l, l);
  g_private_set (&current_cancellable, l);
}

/**
 * g_cancellable_get_current:
 *
 * Gets the top cancellable from the stack.
 *
 * Returns: (nullable) (transfer none): a #GCancellable from the top
 * of the stack, or %NULL if the stack is empty.
 **/
GCancellable *
g_cancellable_get_current  (void)
{
  GSList *l;

  l = g_private_get (&current_cancellable);
  if (l == NULL)
    return NULL;

  return G_CANCELLABLE (l->data);
}

/**
 * g_cancellable_reset:
 * @cancellable: (not nullable): a #GCancellable object.
 * 
 * Resets @cancellable to its uncancelled state.
 *
 * If cancellable is currently in use by any cancellable operation
 * then the behavior of this function is undefined.
 *
 * Note that it is generally not a good idea to reuse an existing
 * cancellable for more operations after it has been cancelled once,
 * as this function might tempt you to do. The recommended practice
 * is to drop the reference to a cancellable after cancelling it,
 * and let it die with the outstanding async operations. You should
 * create a fresh cancellable for further async operations.
 *
 * In the event that a [signal@Gio.Cancellable::cancelled] signal handler is currently
 * running, this call will block until the handler has finished.
 * Calling this function from a signal handler will therefore result in a
 * deadlock.
 **/
void 
g_cancellable_reset (GCancellable *cancellable)
{
  GCancellablePrivate *priv;

  g_return_if_fail (G_IS_CANCELLABLE (cancellable));

  priv = cancellable->priv;

  g_mutex_lock (&priv->mutex);

  if (g_atomic_int_compare_and_exchange (&priv->cancelled, TRUE, FALSE))
    {
      if (priv->wakeup)
        GLIB_PRIVATE_CALL (g_wakeup_acknowledge) (priv->wakeup);
    }

  g_mutex_unlock (&priv->mutex);
}

/**
 * g_cancellable_is_cancelled:
 * @cancellable: (nullable): a #GCancellable or %NULL
 *
 * Checks if a cancellable job has been cancelled.
 *
 * Returns: %TRUE if @cancellable is cancelled,
 * FALSE if called with %NULL or if item is not cancelled.
 **/
gboolean
g_cancellable_is_cancelled (GCancellable *cancellable)
{
  return cancellable != NULL && g_atomic_int_get (&cancellable->priv->cancelled);
}

/**
 * g_cancellable_set_error_if_cancelled:
 * @cancellable: (nullable): a #GCancellable or %NULL
 * @error: #GError to append error state to
 *
 * If the @cancellable is cancelled, sets the error to notify
 * that the operation was cancelled.
 *
 * Returns: %TRUE if @cancellable was cancelled, %FALSE if it was not
 */
gboolean
g_cancellable_set_error_if_cancelled (GCancellable  *cancellable,
                                      GError       **error)
{
  if (g_cancellable_is_cancelled (cancellable))
    {
      g_set_error_literal (error,
                           G_IO_ERROR,
                           G_IO_ERROR_CANCELLED,
                           _("Operation was cancelled"));
      return TRUE;
    }

  return FALSE;
}

/**
 * g_cancellable_get_fd:
 * @cancellable: (nullable): a #GCancellable.
 * 
 * Gets the file descriptor for a cancellable job. This can be used to
 * implement cancellable operations on Unix systems. The returned fd will
 * turn readable when @cancellable is cancelled.
 *
 * You are not supposed to read from the fd yourself, just check for
 * readable status. Reading to unset the readable status is done
 * with g_cancellable_reset().
 * 
 * After a successful return from this function, you should use 
 * g_cancellable_release_fd() to free up resources allocated for 
 * the returned file descriptor.
 *
 * See also g_cancellable_make_pollfd().
 *
 * Returns: A valid file descriptor. `-1` if the file descriptor
 * is not supported, or on errors. 
 **/
int
g_cancellable_get_fd (GCancellable *cancellable)
{
  GPollFD pollfd;
#ifndef G_OS_WIN32
  gboolean retval G_GNUC_UNUSED  /* when compiling with G_DISABLE_ASSERT */;
#endif

  if (cancellable == NULL)
	  return -1;

#ifdef G_OS_WIN32
  pollfd.fd = -1;
#else
  retval = g_cancellable_make_pollfd (cancellable, &pollfd);
  g_assert (retval);
#endif

  return pollfd.fd;
}

/**
 * g_cancellable_make_pollfd:
 * @cancellable: (nullable): a #GCancellable or %NULL
 * @pollfd: a pointer to a #GPollFD
 * 
 * Creates a #GPollFD corresponding to @cancellable; this can be passed
 * to g_poll() and used to poll for cancellation. This is useful both
 * for unix systems without a native poll and for portability to
 * windows.
 *
 * When this function returns %TRUE, you should use 
 * g_cancellable_release_fd() to free up resources allocated for the 
 * @pollfd. After a %FALSE return, do not call g_cancellable_release_fd().
 *
 * If this function returns %FALSE, either no @cancellable was given or
 * resource limits prevent this function from allocating the necessary 
 * structures for polling. (On Linux, you will likely have reached 
 * the maximum number of file descriptors.) The suggested way to handle
 * these cases is to ignore the @cancellable.
 *
 * You are not supposed to read from the fd yourself, just check for
 * readable status. Reading to unset the readable status is done
 * with g_cancellable_reset().
 *
 * Note that in the event that a [signal@Gio.Cancellable::cancelled] signal handler is
 * currently running, this call will block until the handler has finished.
 * Calling this function from a signal handler will therefore result in a
 * deadlock.
 *
 * Returns: %TRUE if @pollfd was successfully initialized, %FALSE on 
 *          failure to prepare the cancellable.
 * 
 * Since: 2.22
 **/
gboolean
g_cancellable_make_pollfd (GCancellable *cancellable, GPollFD *pollfd)
{
  GCancellablePrivate *priv;

  g_return_val_if_fail (pollfd != NULL, FALSE);
  if (cancellable == NULL)
    return FALSE;
  g_return_val_if_fail (G_IS_CANCELLABLE (cancellable), FALSE);

  priv = cancellable->priv;

  g_mutex_lock (&priv->mutex);

  if ((priv->fd_refcount++) == 0)
    {
      priv->wakeup = GLIB_PRIVATE_CALL (g_wakeup_new) ();

      if (g_atomic_int_get (&priv->cancelled))
        GLIB_PRIVATE_CALL (g_wakeup_signal) (priv->wakeup);
    }

  g_assert (priv->wakeup);
  GLIB_PRIVATE_CALL (g_wakeup_get_pollfd) (priv->wakeup, pollfd);

  g_mutex_unlock (&priv->mutex);

  return TRUE;
}

/**
 * g_cancellable_release_fd:
 * @cancellable: (nullable): a #GCancellable
 *
 * Releases a resources previously allocated by g_cancellable_get_fd()
 * or g_cancellable_make_pollfd().
 *
 * For compatibility reasons with older releases, calling this function 
 * is not strictly required, the resources will be automatically freed
 * when the @cancellable is finalized. However, the @cancellable will
 * block scarce file descriptors until it is finalized if this function
 * is not called. This can cause the application to run out of file 
 * descriptors when many #GCancellables are used at the same time.
 *
 * Note that in the event that a [signal@Gio.Cancellable::cancelled] signal handler is
 * currently running, this call will block until the handler has finished.
 * Calling this function from a signal handler will therefore result in a
 * deadlock.
 *
 * Since: 2.22
 **/
void
g_cancellable_release_fd (GCancellable *cancellable)
{
  if (cancellable == NULL)
    return;

  g_return_if_fail (G_IS_CANCELLABLE (cancellable));

  g_mutex_lock (&cancellable->priv->mutex);

  g_assert (cancellable->priv->fd_refcount > 0);

  if ((cancellable->priv->fd_refcount--) == 1)
    {
      GLIB_PRIVATE_CALL (g_wakeup_free) (cancellable->priv->wakeup);
      cancellable->priv->wakeup = NULL;
    }

  g_mutex_unlock (&cancellable->priv->mutex);
}

/**
 * g_cancellable_cancel:
 * @cancellable: (nullable): a #GCancellable object.
 * 
 * Will set @cancellable to cancelled, and will emit the
 * #GCancellable::cancelled signal. (However, see the warning about
 * race conditions in the documentation for that signal if you are
 * planning to connect to it.)
 *
 * This function is thread-safe. In other words, you can safely call
 * it from a thread other than the one running the operation that was
 * passed the @cancellable.
 *
 * If @cancellable is %NULL, this function returns immediately for convenience.
 *
 * The convention within GIO is that cancelling an asynchronous
 * operation causes it to complete asynchronously. That is, if you
 * cancel the operation from the same thread in which it is running,
 * then the operation's #GAsyncReadyCallback will not be invoked until
 * the application returns to the main loop.
 *
 * It is safe (although useless, since it will be a no-op) to call
 * this function from a [signal@Gio.Cancellable::cancelled] signal handler.
 **/
void
g_cancellable_cancel (GCancellable *cancellable)
{
  GCancellablePrivate *priv;

  if (cancellable == NULL || g_atomic_int_get (&cancellable->priv->cancelled))
    return;

  priv = cancellable->priv;

  /* We add a reference before locking, to avoid that potential toggle
   * notifications on the object might happen while we're locked.
   */
  g_object_ref (cancellable);
  g_mutex_lock (&priv->mutex);

  if (!g_atomic_int_compare_and_exchange (&priv->cancelled, FALSE, TRUE))
    {
      g_mutex_unlock (&priv->mutex);
      g_object_unref (cancellable);
      return;
    }

  g_atomic_int_inc (&priv->cancelled_running);

  if (priv->wakeup)
    GLIB_PRIVATE_CALL (g_wakeup_signal) (priv->wakeup);

  /* Adding another reference, in case the callback is unreffing the
   * cancellable and there are toggle references, so that the second to last
   * reference (that would lead a toggle notification) won't be released
   * while we're locked.
   */
  g_object_ref (cancellable);

  g_signal_emit (cancellable, signals[CANCELLED], 0);

  if (g_atomic_int_dec_and_test (&priv->cancelled_running))
    g_cond_broadcast (&cancellable_cond);

  g_mutex_unlock (&priv->mutex);

  g_object_unref (cancellable);
  g_object_unref (cancellable);
}

/**
 * g_cancellable_connect:
 * @cancellable: (not nullable): A #GCancellable.
 * @callback: The #GCallback to connect.
 * @data: Data to pass to @callback.
 * @data_destroy_func: (nullable): Free function for @data or %NULL.
 *
 * Convenience function to connect to the #GCancellable::cancelled
 * signal. Also handles the race condition that may happen
 * if the cancellable is cancelled right before connecting.
 *
 * @callback is called exactly once each time @cancellable is cancelled,
 * either directly at the time of the connect if @cancellable is already
 * cancelled, or when @cancellable is cancelled in some thread.
 * In case the cancellable is reset via [method@Gio.Cancellable.reset]
 * then the callback can be called again if the @cancellable is cancelled and
 * if it had not been previously cancelled at the time
 * [method@Gio.Cancellable.connect] was called (e.g. if the connection actually
 * took place, returning a non-zero value).
 *
 * @data_destroy_func will be called when the handler is
 * disconnected, or immediately if the cancellable is already
 * cancelled.
 *
 * See #GCancellable::cancelled for details on how to use this.
 *
 * Since GLib 2.40, the lock protecting @cancellable is not held when
 * @callback is invoked. This lifts a restriction in place for
 * earlier GLib versions which now makes it easier to write cleanup
 * code that unconditionally invokes e.g. [method@Gio.Cancellable.cancel].
 * Note that since 2.82 GLib still holds a lock during the callback but it’s
 * designed in a way that most of the [class@Gio.Cancellable] methods can be
 * called, including [method@Gio.Cancellable.cancel] or
 * [method@GObject.Object.unref].
 *
 * There are still some methods that will deadlock (by design) when
 * called from the [signal@Gio.Cancellable::cancelled] callbacks:
 *  - [method@Gio.Cancellable.connect]
 *  - [method@Gio.Cancellable.disconnect]
 *  - [method@Gio.Cancellable.reset]
 *  - [method@Gio.Cancellable.make_pollfd]
 *  - [method@Gio.Cancellable.release_fd]
 *
 * Returns: The id of the signal handler or 0 if @cancellable has already
 *          been cancelled.
 *
 * Since: 2.22
 */
gulong
g_cancellable_connect (GCancellable   *cancellable,
		       GCallback       callback,
		       gpointer        data,
		       GDestroyNotify  data_destroy_func)
{
  GCancellable *extra_ref = NULL;
  gulong id;

  g_return_val_if_fail (G_IS_CANCELLABLE (cancellable), 0);

  /* If the cancellable is already cancelled we may end up calling the callback
   * immediately, and the callback may unref the Cancellable, so we need to add
   * an extra reference here. We can't do it only in the case the cancellable
   * is already cancelled because it can be potentially be reset, so we can't
   * rely on the atomic value only, but we need to be locked to be really sure.
   * At the same time we don't want to wake up the ToggleNotify if toggle
   * references are enabled while we're locked.
   */
  g_object_ref (cancellable);

  g_mutex_lock (&cancellable->priv->mutex);

  if (g_atomic_int_get (&cancellable->priv->cancelled))
    {
      void (*_callback) (GCancellable *cancellable,
                         gpointer      user_data);

      /* Adding another reference, in case the callback is unreffing the
       * cancellable and there are toggle references, so that the second to last
       * reference (that would lead a toggle notification) won't be released
       * while we're locked.
       */
      extra_ref = g_object_ref (cancellable);

      _callback = (void *)callback;
      id = 0;

      _callback (cancellable, data);
    }
  else
    {
      GClosure *closure;

      closure = g_cclosure_new (callback, g_steal_pointer (&data),
                                (GClosureNotify) g_steal_pointer (&data_destroy_func));

      id = g_signal_connect_closure_by_id (cancellable, signals[CANCELLED],
                                           0, closure, FALSE);
    }

  g_mutex_unlock (&cancellable->priv->mutex);

  if (data_destroy_func)
    data_destroy_func (data);

  g_object_unref (cancellable);
  g_clear_object (&extra_ref);

  return id;
}

/**
 * g_cancellable_disconnect:
 * @cancellable: (nullable): A #GCancellable or %NULL.
 * @handler_id: Handler id of the handler to be disconnected, or `0`.
 *
 * Disconnects a handler from a cancellable instance similar to
 * g_signal_handler_disconnect().  Additionally, in the event that a
 * signal handler is currently running, this call will block until the
 * handler has finished.  Calling this function from a
 * #GCancellable::cancelled signal handler will therefore result in a
 * deadlock.
 *
 * This avoids a race condition where a thread cancels at the
 * same time as the cancellable operation is finished and the
 * signal handler is removed. See #GCancellable::cancelled for
 * details on how to use this.
 *
 * If @cancellable is %NULL or @handler_id is `0` this function does
 * nothing.
 *
 * Since: 2.22
 */
void
g_cancellable_disconnect (GCancellable  *cancellable,
			  gulong         handler_id)
{
  GCancellablePrivate *priv;

  if (handler_id == 0 ||  cancellable == NULL)
    return;

  priv = cancellable->priv;

  g_mutex_lock (&priv->mutex);

  while (g_atomic_int_get (&priv->cancelled_running) != 0)
    g_cond_wait (&cancellable_cond, &priv->mutex);

  g_mutex_unlock (&priv->mutex);

  g_signal_handler_disconnect (cancellable, handler_id);
}

typedef struct {
  GSource       source;

  /* Atomic: */
  GSource     **self_ptr;
  /* Atomic: */
  GCancellable *cancellable;
  gulong        cancelled_handler;
  /* Atomic: */
  gboolean      cancelled_callback_called;
} GCancellableSource;

/*
 * The reference count of the GSource might be 0 at this point but it is not
 * finalized yet and its dispose function did not run yet, or otherwise we
 * would have disconnected the signal handler already and due to the signal
 * emission lock it would be impossible to call the signal handler at that
 * point. That is: at this point we either have a fully valid GSource, or
 * it's not disposed or finalized yet and we can still resurrect it as needed.
 *
 * As such we first ensure that we have a strong reference to the GSource in
 * here before calling any other GSource API.
 */
static void
cancellable_source_cancelled (GCancellable *cancellable,
			      gpointer      user_data)
{
  GSource *source = g_atomic_pointer_exchange ((GSource **) user_data, NULL);
  GCancellableSource *cancellable_source;
  gboolean callback_was_not_called G_GNUC_UNUSED;

  /* The source is being disposed, so don't bother marking it as ready */
  if (source == NULL)
    return;

  cancellable_source = (GCancellableSource *) source;

  g_source_ref (source);
  g_source_set_ready_time (source, 0);

  callback_was_not_called = g_atomic_int_compare_and_exchange (
    &cancellable_source->cancelled_callback_called, FALSE, TRUE);
  g_assert (callback_was_not_called);

  g_source_unref (source);
}

static gboolean
cancellable_source_prepare (GSource *source,
                            gint    *timeout)
{
  GCancellableSource *cancellable_source = (GCancellableSource *) source;
  GCancellable *cancellable;

  if (timeout)
    *timeout = -1;

  cancellable = g_atomic_pointer_get (&cancellable_source->cancellable);
  if (cancellable && !g_atomic_int_get (&cancellable->priv->cancelled_running))
    {
      g_atomic_int_set (&cancellable_source->cancelled_callback_called, FALSE);
      g_atomic_pointer_set (cancellable_source->self_ptr, source);
    }

  return FALSE;
}

static gboolean
cancellable_source_dispatch (GSource     *source,
			     GSourceFunc  callback,
			     gpointer     user_data)
{
  GCancellableSourceFunc func = (GCancellableSourceFunc)callback;
  GCancellableSource *cancellable_source = (GCancellableSource *)source;

  g_source_set_ready_time (source, -1);
  return (*func) (cancellable_source->cancellable, user_data);
}

static void
cancellable_source_dispose (GSource *source)
{
  GCancellableSource *cancellable_source = (GCancellableSource *)source;
  GCancellable *cancellable;

  cancellable = g_atomic_pointer_exchange (&cancellable_source->cancellable, NULL);

  if (cancellable)
    {
      GSource *self_ptr =
        g_atomic_pointer_exchange (cancellable_source->self_ptr, NULL);

      if (self_ptr == NULL)
        {
          /* There can be a race here: if thread A has called
           * g_cancellable_cancel() and has got as far as committing to call
           * cancellable_source_cancelled(), then thread B drops the final
           * ref on the GCancellableSource before g_source_ref() is called in
           * cancellable_source_cancelled(), then cancellable_source_dispose()
           * will run through and the GCancellableSource will be finalised
           * before cancellable_source_cancelled() gets to g_source_ref(). It
           * will then be left in a state where it’s committed to using a
           * dangling GCancellableSource pointer.
           *
           * Eliminate that race by waiting to ensure that our cancelled
           * callback has been called, so that there's no risk that we're
           * unreffing something that is still going to be used.
           */

          while (!g_atomic_int_get (&cancellable_source->cancelled_callback_called))
            ;
        }

      g_clear_signal_handler (&cancellable_source->cancelled_handler, cancellable);
      g_object_unref (cancellable);
    }
}

static gboolean
cancellable_source_closure_callback (GCancellable *cancellable,
				     gpointer      data)
{
  GClosure *closure = data;

  GValue params = G_VALUE_INIT;
  GValue result_value = G_VALUE_INIT;
  gboolean result;

  g_value_init (&result_value, G_TYPE_BOOLEAN);

  g_value_init (&params, G_TYPE_CANCELLABLE);
  g_value_set_object (&params, cancellable);

  g_closure_invoke (closure, &result_value, 1, &params, NULL);

  result = g_value_get_boolean (&result_value);
  g_value_unset (&result_value);
  g_value_unset (&params);

  return result;
}

static GSourceFuncs cancellable_source_funcs =
{
  cancellable_source_prepare,
  NULL,
  cancellable_source_dispatch,
  NULL,
  (GSourceFunc)cancellable_source_closure_callback,
  NULL,
};

/**
 * g_cancellable_source_new:
 * @cancellable: (nullable): a #GCancellable, or %NULL
 *
 * Creates a source that triggers if @cancellable is cancelled and
 * calls its callback of type #GCancellableSourceFunc. This is
 * primarily useful for attaching to another (non-cancellable) source
 * with g_source_add_child_source() to add cancellability to it.
 *
 * For convenience, you can call this with a %NULL #GCancellable,
 * in which case the source will never trigger.
 *
 * The new #GSource will hold a reference to the #GCancellable.
 *
 * Returns: (transfer full): the new #GSource.
 *
 * Since: 2.28
 */
GSource *
g_cancellable_source_new (GCancellable *cancellable)
{
  GSource *source;
  GCancellableSource *cancellable_source;

  source = g_source_new (&cancellable_source_funcs, sizeof (GCancellableSource));
  g_source_set_static_name (source, "GCancellable");
  g_source_set_dispose_function (source, cancellable_source_dispose);
  cancellable_source = (GCancellableSource *)source;

  if (cancellable)
    {
      cancellable_source->cancellable = g_object_ref (cancellable);
      cancellable_source->self_ptr = g_new (GSource *, 1);
      g_atomic_pointer_set (cancellable_source->self_ptr, source);

      /* We intentionally don't use g_cancellable_connect() here,
       * because we don't want the "at most once" behavior.
       */
      cancellable_source->cancelled_handler =
        g_signal_connect_data (cancellable, "cancelled",
                               G_CALLBACK (cancellable_source_cancelled),
                               cancellable_source->self_ptr,
                               (GClosureNotify) g_free, G_CONNECT_DEFAULT);
      if (g_cancellable_is_cancelled (cancellable))
        g_source_set_ready_time (source, 0);
    }

  return source;
}
