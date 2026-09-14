Title: Spawning Processes
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2014 Red Hat, Inc.
SPDX-FileCopyrightText: 2017 Philip Withnall

# Spawning Processes

GLib supports spawning of processes with an API that is more
convenient than the bare UNIX [`fork()`](man:fork(2)) and
[`exec()`](man:exec(3)).

The `g_spawn_…()` family of functions has synchronous ([func@GLib.spawn_sync])
and asynchronous variants ([func@GLib.spawn_async],
[func@GLib.spawn_async_with_pipes]), as well as convenience variants that take a
complete shell-like command line ([func@GLib.spawn_command_line_sync],
[func@GLib.spawn_command_line_async]).

See [`GSubprocess`](../gio/class.Subprocess.html) in GIO for a higher-level API
that provides stream interfaces for communication with child processes.

An example of using [func@GLib.spawn_async_with_pipes]:
```c
const gchar * const argv[] = { "my-favourite-program", "--args", NULL };
gint child_stdout, child_stderr;
GPid child_pid;
g_autoptr(GError) error = NULL;

// Spawn child process.
g_spawn_async_with_pipes (NULL, argv, NULL, G_SPAWN_DO_NOT_REAP_CHILD, NULL,
                          NULL, &child_pid, NULL, &child_stdout,
                          &child_stderr, &error);
if (error != NULL)
  {
    g_error ("Spawning child failed: %s", error->message);
    return;
  }

// Add a child watch function which will be called when the child process
// exits.
g_child_watch_add (child_pid, child_watch_cb, NULL);

// You could watch for output on @child_stdout and @child_stderr using
// #GUnixInputStream or #GIOChannel here.

static void
child_watch_cb (GPid     pid,
                gint     status,
                gpointer user_data)
{
  g_message ("Child %" G_PID_FORMAT " exited %s", pid,
             g_spawn_check_wait_status (status, NULL) ? "normally" : "abnormally");

  // Free any resources associated with the child here, such as I/O channels
  // on its stdout and stderr FDs. If you have no code to put in the
  // child_watch_cb() callback, you can remove it and the g_child_watch_add()
  // call, but you must also remove the G_SPAWN_DO_NOT_REAP_CHILD flag,
  // otherwise the child process will stay around as a zombie until this
  // process exits.

  g_spawn_close_pid (pid);
}
```

## Spawn Functions

 * [func@GLib.spawn_async_with_fds]
 * [func@GLib.spawn_async_with_pipes]
 * [func@GLib.spawn_async_with_pipes_and_fds]
 * [func@GLib.spawn_async]
 * [func@GLib.spawn_sync]
 * [func@GLib.spawn_check_wait_status]
 * [func@GLib.spawn_check_exit_status]
 * [func@GLib.spawn_command_line_async]
 * [func@GLib.spawn_command_line_sync]
 * [func@GLib.spawn_close_pid]

