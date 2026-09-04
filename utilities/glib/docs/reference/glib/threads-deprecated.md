Title: Deprecated Thread API
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2011 Allison Lortie

# Deprecated Thread API

These APIs are deprecated.  You should not use them in new code.
This section remains only to assist with understanding code that was
written to use these APIs at some point in the past.

Deprecated thread creation/configuration functions:

 * [type@GLib.ThreadPriority]
 * [type@GLib.ThreadFunctions]
 * [func@GLib.Thread.init]
 * [func@GLib.Thread.get_initialized]
 * [method@GLib.Thread.set_priority]
 * [func@GLib.Thread.foreach]
 * [func@GLib.Thread.create]
 * [func@GLib.Thread.create_full]

Deprecated static variants of locking primitives:

 * [type@GLib.StaticMutex]
 * [type@GLib.StaticRecMutex]
 * [type@GLib.StaticRWLock]
 * [type@GLib.StaticPrivate]

Deprecated dynamic allocation of locking primitives:

 * [func@GLib.Private.new]
 * [func@GLib.Mutex.new]
 * [method@GLib.Mutex.free]
 * [func@GLib.Cond.new]
 * [method@GLib.Cond.free]
 * [method@GLib.Cond.timed_wait]

