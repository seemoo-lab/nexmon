/* -*- mode: C; c-file-style: "gnu"; indent-tabs-mode: nil; -*- */
/* GIO - GLib Input, Output and Streaming Library
 *
 * Copyright © 2018 Endless Mobile, Inc.
 * Copyright © 2025 Oscar Pernia <oscarperniamoreno@gmail.com>
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
 * Public License along with this library; if not, see
 * <http://www.gnu.org/licenses/>.
 *
 * Authors:
 *  - Philip Withnall <withnall@endlessm.com>
 *  - Oscar Pernia <oscarperniamoreno@gmail.com> - Initial implementation using legacy Shell_NotifyIcon API
 */

#include "config.h"

#include <windows.h>

#include "gapplication.h"
#include "gnotificationbackend.h"

#include "giomodule-priv.h"
#include "glib-private.h"
#include "gnotification-private.h"

#define G_TYPE_WIN32_NOTIFICATION_BACKEND  (g_win32_notification_backend_get_type ())
#define G_WIN32_NOTIFICATION_BACKEND(o)    (G_TYPE_CHECK_INSTANCE_CAST ((o), G_TYPE_WIN32_NOTIFICATION_BACKEND, GWin32NotificationBackend))

extern IMAGE_DOS_HEADER __ImageBase;

static inline HMODULE
exe_module (void)
{
  return (HMODULE) GetModuleHandle (NULL);
}

static inline HMODULE
this_module (void)
{
  return (HMODULE) &__ImageBase;
}

static HWND hwnd;
static GMutex hwnd_mutex; /* Protects access to `hwnd`, `hwnd_refcount`, `hwnd_state` and `wnd_klass` */
static grefcount hwnd_refcount;
static ATOM wnd_klass;

typedef enum
{
  HWND_STATE_READY,
  HWND_STATE_FAILED,
  HWND_STATE_UNINITIALIZED,
  HWND_STATE_INITIALIZING,
  HWND_STATE_DESTROYING,

  /* Should set this state if Shell_NotifyIcon(NIM_MODIFY (or NIM_ADD on init method), ...)
   * failed. If this is the state set, the calling thread should wait on `hwnd_cond`
   * for `hwnd_state` to change.
   *
   * Should go back to READY after dummy_WndProc got TaskBarCreated message and succesfully called
   * Shell_NotifyIcon(NIM_ADD, ...), if that call also failed, the state changes to FAILED instead. */
  HWND_STATE_INITIALIZING_NOTIFY_ICON,
} HwndState;

/* Specifies the state in which `hwnd` initialization is in. */
static HwndState hwnd_state = HWND_STATE_UNINITIALIZED;

/* Condition variable for changes in `hwnd_state`. */
static GCond hwnd_cond;

typedef struct _GWin32NotificationBackend GWin32NotificationBackend;
typedef GNotificationBackendClass         GWin32NotificationBackendClass;

struct _GWin32NotificationBackend
{
  GNotificationBackend parent;

  /* Prevents double-frees when disposing of the backend, if it's non-NULL,
   * we decrement `hwnd_refcount` and set this to NULL */
  HWND hwnd;
};

GType g_win32_notification_backend_get_type (void);

G_DEFINE_TYPE_WITH_CODE (GWin32NotificationBackend, g_win32_notification_backend, G_TYPE_NOTIFICATION_BACKEND,
  _g_io_modules_ensure_extension_points_registered ();
  g_io_extension_point_implement (G_NOTIFICATION_BACKEND_EXTENSION_POINT_NAME,
                                  g_define_type_id, "win32", 0))

/* Maximum number of UTF-16 code units that can be written into
 * NOTIFYICONDATA.szInfoTitle array, it does not count one element
 * which is for the NUL-terminator */
#define MAX_TITLE_COUNT (G_N_ELEMENTS (((NOTIFYICONDATA *) 0)->szInfoTitle) - 1)

/* Maximum number of UTF-16 code units that can be written into
 * NOTIFYICONDATA.szInfo array, it does not count one element
 * which is for the NUL-terminator */
#define MAX_BODY_COUNT (G_N_ELEMENTS (((NOTIFYICONDATA *) 0)->szInfo) - 1)

/* Maximum number of UTF-16 code units that can be written into
 * NOTIFYICONDATA.szTip array, it does not count one element
 * which is for the NUL-terminator */
#define MAX_TIP_COUNT (G_N_ELEMENTS (((NOTIFYICONDATA *) 0)->szTip) - 1)

#define WM_APP_NOTIFYCALLBACK (WM_APP + 1)

/* Initializes `out` with a NOTIFYICONDATA struct, to be passed to
 * Shell_NotifyIcon (NIM_ADD, ...) calls. */
static void
fill_notify_icon_data (NOTIFYICONDATA *out)
{
  *out = (NOTIFYICONDATA){
    .cbSize = sizeof (NOTIFYICONDATA),
    .hWnd = hwnd,
    .uFlags = NIF_ICON | NIF_MESSAGE | NIF_TIP | NIF_SHOWTIP,
    .hIcon = LoadIcon (exe_module (), MAKEINTRESOURCE (1)),
    .uCallbackMessage = WM_APP_NOTIFYCALLBACK,
    .uVersion = NOTIFYICON_VERSION_4,
  };

  if (!out->hIcon)
    {
      /* Fallback if the application has no icon */
      out->hIcon = LoadIcon (NULL, IDI_APPLICATION);
    }

  const gchar *tip_utf8 = g_get_application_name ();

  glong items_written;
  GError *error = NULL;

  if (tip_utf8 && *tip_utf8)
    {
      WCHAR *tip_utf16 = g_utf8_to_utf16 (tip_utf8, -1, NULL, &items_written, &error);
      if (error)
        {
          g_critical ("Invalid UTF-8 in application name: %s", error->message);
          g_error_free (error);
        }
      else
        {
          g_assert (items_written >= 0);
          if ((size_t) items_written > MAX_TIP_COUNT)
            {
              g_warning ("Application name too long for notification tool-tip, truncating it");
              items_written = MAX_TIP_COUNT;
              if (IS_LOW_SURROGATE (tip_utf16[items_written]))
                items_written--;
            }

          memcpy (&out->szTip, tip_utf16, items_written * sizeof (WCHAR));
          out->szTip[items_written] = L'\0';
          g_free (tip_utf16);
        }
    }
}

static gboolean
g_win32_notification_backend_is_supported (void)
{
  /* This backend is always supported on Windows. */
  return TRUE;
}

/* Implementing send-and-forget notifications, only supporting W10/11 */
static void
g_win32_notification_backend_send_notification (GNotificationBackend *backend,
                                                const gchar          *id,
                                                GNotification        *notification)
{
  /* GIO users may expect notification actions to work, and expect the actions
   * to be activated, but because we cannot fullfil the request, it's appropriate
   * to warn about it at least once to let them know. */

  if (g_notification_get_n_buttons (notification) > 0 ||
      g_notification_get_default_action (notification, NULL, NULL))
    {
      g_warning_once ("Notification actions are unsupported by this Windows backend");
    }

  /* NOTE: Icon is unsupported for W10 or greater, trying to set an icon will
   * make the notification to not show up */
  /* NOTE: There is no category */
  /* NOTE: There is no priority */

  GWin32NotificationBackend *self = G_WIN32_NOTIFICATION_BACKEND (backend);

  g_mutex_lock (&hwnd_mutex);

  /* Return early if backend's initialization failed */
  if (hwnd_state != HWND_STATE_READY &&
      hwnd_state != HWND_STATE_INITIALIZING_NOTIFY_ICON)
    {
      goto err_out;
    }

  NOTIFYICONDATA notify_singleton = {
    .cbSize = sizeof (NOTIFYICONDATA),
    .hWnd = self->hwnd,
    .uFlags = NIF_INFO,
  };

  GError *error = NULL;
  glong items_written;

  const gchar *title_utf8 = g_notification_get_title (notification);
  WCHAR *title_utf16 = g_utf8_to_utf16 (title_utf8, -1, NULL, &items_written, &error);
  if (error)
    {
      g_critical ("Invalid UTF-8 in notification: %s", error->message);
      g_error_free (error);
      goto err_out; /* Cannot show a notification without a title */
    }

  g_assert (items_written >= 0);
  if ((size_t) items_written > MAX_TITLE_COUNT)
    {
      g_warning ("Notification title too long, truncating title");
      items_written = MAX_TITLE_COUNT;
      if (IS_LOW_SURROGATE (title_utf16[items_written]))
        items_written--;
    }

  if (items_written == 0)
    {
      g_critical ("Notification title is empty");
      g_free (title_utf16);
      goto err_out; /* Cannot show a notification without a title */
    }

  memcpy (&notify_singleton.szInfoTitle, title_utf16, items_written * sizeof (WCHAR));
  notify_singleton.szInfoTitle[items_written] = L'\0';
  g_free (title_utf16);

  const gchar *body_utf8 = g_notification_get_body (notification);
  if (!body_utf8 || !*body_utf8)
    {
      /* If you pass an empty body, Shell_NotifyIcon won't show the notification,
       * in order to fix this, here we are setting a string with a single
       * SPACE (' ') character, which makes the body look like empty.
       *
       * > To remove the balloon notification from the UI (...)
       * > set the `NIF_INFO` flag in `uFlags` and set `szInfo` to an empty string.
       *
       * https://learn.microsoft.com/en-us/windows/win32/api/shellapi/ns-shellapi-notifyicondataw */
      notify_singleton.szInfo[0] = L' ';
      notify_singleton.szInfo[1] = L'\0';
    }
  else
    {
      WCHAR *body_utf16 = g_utf8_to_utf16 (body_utf8, -1, NULL, &items_written, &error);
      if (error)
        {
          g_critical ("Invalid UTF-8 in notification: %s", error->message);
          g_error_free (error);
          goto err_out; /* Cannot show a notification without a body */
        }

      g_assert (items_written >= 0);
      if ((size_t) items_written > MAX_BODY_COUNT)
        {
          g_warning ("Notification body too long, truncating body");
          items_written = MAX_BODY_COUNT;
          if (IS_LOW_SURROGATE (body_utf16[items_written]))
            items_written--;
        }

      memcpy (&notify_singleton.szInfo, body_utf16, items_written * sizeof (WCHAR));
      notify_singleton.szInfo[items_written] = L'\0';
      g_free (body_utf16);
    }

  BOOL ret;

  /* Loops until Shell_NotifyIcon call reports success, the worker thread
   * failed to create the notification icon or it timed out waiting for it. */
  do
    {
      gint64 end_time = g_get_monotonic_time () + 5 * G_TIME_SPAN_SECOND;

      /* Wait until notification icon is initialized */
      while (hwnd_state == HWND_STATE_INITIALIZING_NOTIFY_ICON)
        if (!g_cond_wait_until (&hwnd_cond, &hwnd_mutex, end_time))
          {
            /* Timeout has passed */
            g_debug ("Timeout in send_notification while waiting for the notification icon to be created");
            goto err_out;
          }

      if (hwnd_state == HWND_STATE_FAILED)
        {
          goto err_out;
        }

      if (!(ret = Shell_NotifyIcon (NIM_MODIFY, &notify_singleton)))
        {
          /* Assume shell's task bar is still not ready, set INITIALIZING_NOTIFY_ICON state
           * and wait until dummy_WndProc receives TaskBarCreated message */
          hwnd_state = HWND_STATE_INITIALIZING_NOTIFY_ICON;
        }
    }
  while (!ret);

err_out:
  g_mutex_unlock (&hwnd_mutex);
}

static void
g_win32_notification_backend_withdraw_notification (GNotificationBackend *backend,
                                                    const gchar          *id)
{
}

/* Runs on GLib worker thread */
static gboolean
destroy_window_worker (void)
{
  g_mutex_lock (&hwnd_mutex);

  g_assert (hwnd_state == HWND_STATE_DESTROYING);

  GWeakRef *weak_ref = (GWeakRef *) GetWindowLongPtr (hwnd, GWLP_USERDATA);
  g_weak_ref_clear (weak_ref);
  g_free (weak_ref);

  DestroyWindow (hwnd);
  hwnd = NULL;
  UnregisterClass (MAKEINTATOM (wnd_klass), this_module ());
  wnd_klass = 0;
  hwnd_state = HWND_STATE_UNINITIALIZED;

  g_cond_broadcast (&hwnd_cond);
  g_mutex_unlock (&hwnd_mutex);

  return G_SOURCE_REMOVE;
}

static void
g_win32_notification_backend_dispose (GObject *self)
{
  GWin32NotificationBackend *backend = G_WIN32_NOTIFICATION_BACKEND (self);

  g_mutex_lock (&hwnd_mutex);

  if (backend->hwnd && g_ref_count_dec (&hwnd_refcount))
    {
      /* Window must be destroyed by the same thread that created it.
       *
       * https://learn.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-destroywindow#remarks */

      hwnd_state = HWND_STATE_DESTROYING;

      g_main_context_invoke (GLIB_PRIVATE_CALL (g_get_worker_context) (),
                             G_SOURCE_FUNC (destroy_window_worker), NULL);

      /* NOTE: threads that call *_init method wait for DESTROYING state to change to UNINITIALIZED,
       * there is no need to do that here. */
    }

  backend->hwnd = NULL;

  g_mutex_unlock (&hwnd_mutex);

  G_OBJECT_CLASS (g_win32_notification_backend_parent_class)->dispose (self);
}

static gboolean
activate_app (GApplication *app)
{
  g_application_activate (app);

  return G_SOURCE_REMOVE;
}

/* Not dummy anymore... */
static LRESULT CALLBACK
dummy_WndProc (HWND _hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
  static UINT msg_TaskbarCreated;

  switch (message)
    {
    case WM_CREATE:
      {
        msg_TaskbarCreated = RegisterWindowMessage (L"TaskbarCreated");
        if (msg_TaskbarCreated == 0)
          {
            g_warning ("win32-notification: RegisterWindowMessage failed: '%ld'", GetLastError ());
          }

        CREATESTRUCT *cs = (CREATESTRUCT *) lparam;
        SetWindowLongPtr (_hwnd, GWLP_USERDATA, (LONG_PTR) cs->lpCreateParams);
        break;
      }
    case WM_NULL:
      {
        break;
      }
    case WM_APP_NOTIFYCALLBACK:
      {
        switch (LOWORD (lparam))
          {
          /* Handles ENTER, SPACE and LEFT_CLICK keystrokes */
          case NIN_SELECT:
          case NIN_KEYSELECT:
            {
              GWeakRef *backend_weak = (GWeakRef *) GetWindowLongPtr (_hwnd, GWLP_USERDATA);
              GNotificationBackend *backend = (GNotificationBackend *) g_weak_ref_get (backend_weak);
              if (!backend)
                break;

              GApplication *app = g_notification_backend_dup_application (backend);
              if (app)
                {
                  g_main_context_invoke_full (NULL, G_PRIORITY_DEFAULT,
                                              G_SOURCE_FUNC (activate_app), app,
                                              g_object_unref);
                }

              g_object_unref (backend);

              break;
            }
          }
        break;
      }
    default:
      {
        if (message == msg_TaskbarCreated)
          {
            g_mutex_lock (&hwnd_mutex);

            if (hwnd_state != HWND_STATE_READY &&
                hwnd_state != HWND_STATE_INITIALIZING_NOTIFY_ICON)
              {
                g_mutex_unlock (&hwnd_mutex);
                break;
              }

            NOTIFYICONDATA notify_singleton;
            fill_notify_icon_data (&notify_singleton);

            if (!Shell_NotifyIcon (NIM_ADD, &notify_singleton) ||
                !Shell_NotifyIcon (NIM_SETVERSION, &notify_singleton))
              {
                hwnd_state = HWND_STATE_FAILED;
              }
            else
              {
                hwnd_state = HWND_STATE_READY;
              }

            g_cond_broadcast (&hwnd_cond);
            g_mutex_unlock (&hwnd_mutex);
          }
      }
    }

  return DefWindowProc (_hwnd, message, wparam, lparam);
}

/* Runs on GLib worker thread */
static gboolean
create_window_worker (gpointer user_data)
{
  GWeakRef *backend_weak = (GWeakRef *) user_data;

  g_mutex_lock (&hwnd_mutex);

  g_assert (hwnd_state == HWND_STATE_INITIALIZING);

  WNDCLASS wclass = {
    .lpszClassName = L"GWin32NotificationBackend",
    .hInstance = this_module (),
    .lpfnWndProc = dummy_WndProc,
  };

  wnd_klass = RegisterClass (&wclass);
  if (!wnd_klass)
    {
      g_critical ("win32-notification: RegisterClass failed: %ld", GetLastError ());
      hwnd_state = HWND_STATE_FAILED;
      g_weak_ref_clear (backend_weak);
      g_free (backend_weak);
      goto err_out;
    }

  hwnd = CreateWindow (MAKEINTATOM (wnd_klass), NULL, WS_POPUP,
                       0, 0, 0, 0, NULL, NULL, this_module (), backend_weak);

  if (!hwnd)
    {
      g_critical ("win32-notification: CreateWindow failed: %ld", GetLastError ());
      UnregisterClass (MAKEINTATOM (wnd_klass), this_module ());
      wnd_klass = 0;
      hwnd_state = HWND_STATE_FAILED;
      g_weak_ref_clear (backend_weak);
      g_free (backend_weak);
      goto err_out;
    }

  /* Create the notification icon for the first time */

  NOTIFYICONDATA notify_singleton;
  fill_notify_icon_data (&notify_singleton);

  if (!Shell_NotifyIcon (NIM_ADD, &notify_singleton))
    {
      /* Assume shell's task bar is still not ready, set INITIALIZING_NOTIFY_ICON state
       * and wait until dummy_WndProc receives TaskbarCreated message */
      hwnd_state = HWND_STATE_INITIALIZING_NOTIFY_ICON;
    }
  else if (!Shell_NotifyIcon (NIM_SETVERSION, &notify_singleton))
    {
      /* We require NOTIFYICON_VERSION_4 features, this is an unlikely path
       * since it can only fail in very old Windows systems (pre-Windows 2000) */

      g_warning ("Windows system unsupported for sending notifications");
      DestroyWindow (hwnd);
      hwnd = NULL;
      UnregisterClass (MAKEINTATOM (wnd_klass), this_module ());
      wnd_klass = 0;
      hwnd_state = HWND_STATE_FAILED;
      g_weak_ref_clear (backend_weak);
      g_free (backend_weak);
      goto err_out;
    }
  else
    {
      hwnd_state = HWND_STATE_READY;
    }

  g_ref_count_init (&hwnd_refcount);

err_out:
  g_cond_broadcast (&hwnd_cond);
  g_mutex_unlock (&hwnd_mutex);

  return G_SOURCE_REMOVE;
}

/* {{{ GWin32MessageSource
 *
 * Copied from https://gitlab.gnome.org/GNOME/gtk/-/blob/main/gdk/win32/gdkwin32messagesource.c
 *
 * FIXME: Move all of this code to centralized module. */

typedef struct
{
  GSource source;
  GPollFD poll_fd;
} GWin32MessageSource;

/* Runs on GLib worker thread */
static gboolean
g_win32_message_source_prepare (GSource *source,
                                int *timeout)
{
  *timeout = -1;

  return GetQueueStatus (QS_ALLINPUT) != 0;
}

/* Runs on GLib worker thread */
static gboolean
g_win32_message_source_check (GSource *source)
{
  GWin32MessageSource *self = (GWin32MessageSource *) source;

  self->poll_fd.revents = 0;

  return GetQueueStatus (QS_ALLINPUT) != 0;
}

/* Runs on GLib worker thread */
static gboolean
g_win32_message_source_dispatch (GSource *source,
                                 GSourceFunc callback,
                                 gpointer user_data)
{
  MSG msg;

  /* Message loop with upper limit, prevents GSource loop starvation and
   * allows to process big batches of messages in a single dispatch. */
  for (int i = 0; i < 100 && PeekMessage (&msg, NULL, 0, 0, PM_REMOVE); i++)
    {
      TranslateMessage (&msg);
      DispatchMessage (&msg);
    }

  return G_SOURCE_CONTINUE;
}

static void
g_win32_message_source_ensure_running (void)
{
  static gsize message_source_initialized = 0;

  if (g_once_init_enter (&message_source_initialized))
    {
      static GSourceFuncs source_funcs = {
        .prepare = g_win32_message_source_prepare,
        .check = g_win32_message_source_check,
        .dispatch = g_win32_message_source_dispatch,
        /* No finalize, the worker thread lasts until the end of the program,
         * so the worker's GMainContext isn't going to be unref-ed.
         * We don't return G_SOURCE_REMOVE from dispatch as well. */
        .finalize = NULL,
      };

      GSource *source = g_source_new (&source_funcs, sizeof (GWin32MessageSource));
      GWin32MessageSource *message_source = (GWin32MessageSource *) source;

      g_source_set_static_name (source, "GLib Win32 worker message source");
      g_source_set_priority (source, G_PRIORITY_DEFAULT);
      g_source_set_can_recurse (source, TRUE);

      message_source->poll_fd.events = G_IO_IN;
#if defined(G_WITH_CYGWIN)
      message_source->poll_fd.fd = open ("/dev/windows", O_RDONLY);
      if (message_source->poll_fd.fd == -1)
        g_error ("can't open \"/dev/windows\": %s", g_strerror (errno));
#else
      message_source->poll_fd.fd = G_WIN32_MSG_HANDLE;
#endif

      g_source_add_poll (source, &message_source->poll_fd);
      g_source_attach (source, GLIB_PRIVATE_CALL (g_get_worker_context) ());
      g_source_unref (source);

      g_once_init_leave (&message_source_initialized, 1);
    }
}

/* }}} */

static void
g_win32_notification_backend_constructed (GObject *self)
{
  G_OBJECT_CLASS (g_win32_notification_backend_parent_class)->constructed (self);

  GWin32NotificationBackend *backend = G_WIN32_NOTIFICATION_BACKEND (self);

  /* FIXME: Move this to centralized module */
  g_win32_message_source_ensure_running ();

  /* Don't increment on UNINITIALIZED or if `hwnd` is NULL, at every other case,
   * it should increment. */
  gboolean needs_inc = TRUE;

  g_mutex_lock (&hwnd_mutex);

  while (hwnd_state == HWND_STATE_DESTROYING)
    g_cond_wait (&hwnd_cond, &hwnd_mutex);

  if (hwnd_state == HWND_STATE_UNINITIALIZED)
    {
      hwnd_state = HWND_STATE_INITIALIZING;
      needs_inc = FALSE; /* Alredy incremented in worker */

      GWeakRef *backend_weak = g_new (GWeakRef, 1);
      g_weak_ref_init (backend_weak, backend);

      g_main_context_invoke (GLIB_PRIVATE_CALL (g_get_worker_context) (),
                             G_SOURCE_FUNC (create_window_worker), backend_weak);
    }

  while (hwnd_state == HWND_STATE_INITIALIZING)
    g_cond_wait (&hwnd_cond, &hwnd_mutex);

  if (hwnd)
    {
      backend->hwnd = hwnd;

      if (needs_inc)
        g_ref_count_inc (&hwnd_refcount);
    }

  g_mutex_unlock (&hwnd_mutex);
}

static void
g_win32_notification_backend_init (GWin32NotificationBackend *backend)
{
}

static void
g_win32_notification_backend_class_init (GWin32NotificationBackendClass *class)
{
  GObjectClass *object_class = G_OBJECT_CLASS (class);
  GNotificationBackendClass *backend_class = G_NOTIFICATION_BACKEND_CLASS (class);

  object_class->dispose = g_win32_notification_backend_dispose;
  object_class->constructed = g_win32_notification_backend_constructed;

  backend_class->is_supported = g_win32_notification_backend_is_supported;
  backend_class->send_notification = g_win32_notification_backend_send_notification;
  backend_class->withdraw_notification = g_win32_notification_backend_withdraw_notification;
}
