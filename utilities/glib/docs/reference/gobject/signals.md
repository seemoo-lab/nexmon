Title: Signals
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2000, 2001 Tim Janik
SPDX-FileCopyrightText: 2000 Owen Taylor
SPDX-FileCopyrightText: 2002, 2014 Matthias Clasen
SPDX-FileCopyrightText: 2014 Collabora, Ltd.
SPDX-FileCopyrightText: 2019 Endless Mobile, Inc.

# Signals

The basic concept of the signal system is that of the emission
of a signal. Signals are introduced per-type and are identified
through strings. Signals introduced for a parent type are available
in derived types as well, so basically they are a per-type facility
that is inherited.

## Handlers

A signal emission mainly involves invocation of a certain set of callbacks
in precisely defined manner. There are two main categories of such
callbacks, per-class ones and user provided ones. (Although signals can deal
with any kind of instantiatable type, those types are referred to as ‘object
types’ in the following, simply because that is the context most users will
encounter signals in.) The per-class callbacks are most often referred to as
"class (signal) handler" or "default (signal) handler", while user provided
callbacks are usually just called "signal handler".

The object method handler is provided at signal creation time (this most
frequently happens at the end of an object class' creation), while user
provided handlers are frequently connected and disconnected to/from a
certain signal on certain object instances. Note that it does not matter if
a signal handler is connected within the class implementation itself: the
default signal handler is always the one set at **signal** creation time; if
none is set, the signal has no default handler.

A handler must match the type defined by the signal in both its arguments and
return value (which is often `void`). All handlers take a pointer to the type
instance as their first argument, and a `gpointer user_data` as their final
argument, with signal-defined arguments in-between. The `user_data` is always
filled with the user data provided when the handler was connected to the signal.
Handlers are documented as having type [type@GObject.Callback], but this is
simply a generic placeholder type — an artifact of the GSignal APIs being
polymorphic.

When a signal handler is connected, its first (‘instance’) and final (‘user
data’) arguments may be swapped if [func@GObject.signal_connect_swapped] is used
rather than [func@GObject.signal_connect]. This can sometimes be convenient for
avoiding defining wrapper functions as signal handlers, instead just directly
passing a function which takes the user data as its first argument.

## Emissions

A signal emission consists of five stages, unless prematurely stopped:

1. Invocation of the object method handler for [`flags@GObject.SignalFlags.RUN_FIRST`] 
   signals

2. Invocation of normal user-provided signal handlers (where the `after`
   flag is not set)

3. Invocation of the object method handler for [`flags@GObject.SignalFlags.RUN_LAST`]
   signals

4. Invocation of user provided signal handlers (where the `after` flag is set)

5. Invocation of the object method handler for [`flags@GObject.SignalFlags.RUN_CLEANUP`]
   signals

The user-provided signal handlers are called in the order they were
connected in.

All handlers may prematurely stop a signal emission, and any number of
handlers may be connected, disconnected, blocked or unblocked during a
signal emission.

There are certain criteria for skipping user handlers in stages 2 and 4 of a
signal emission.

First, user handlers may be blocked. Blocked handlers are omitted during
callback invocation, to return from the blocked state, a handler has to get
unblocked exactly the same amount of times it has been blocked before.

Second, upon emission of a [`flags@GObject.SignalFlags.DETAILED`] signal, an
additional `detail` argument passed in to [func@GObject.signal_emit] has to
match the detail argument of the signal handler currently subject to
invocation. Specification of no detail argument for signal handlers
(omission of the detail part of the signal specification upon connection)
serves as a wildcard and matches any detail argument passed in to emission.

While the `detail` argument is typically used to pass an object property
name (as with [signal@GObject.Object::notify]), no specific format is
mandated for the detail string, other than that it must be non-empty.

## Memory management of signal handlers

If you are connecting handlers to signals and using a `GObject` instance as
your signal handler user data, you should remember to pair calls to
[func@GObject.signal_connect] with calls to [func@GObject.signal_handler_disconnect]
or [func@GObject.signal_handlers_disconnect_by_func]. While signal handlers are
automatically disconnected when the object emitting the signal is finalised,
they are not automatically disconnected when the signal handler user data is
destroyed. If this user data is a `GObject` instance, using it from a
signal handler after it has been finalised is an error.

There are two strategies for managing such user data. The first is to
disconnect the signal handler (using [func@GObject.signal_handler_disconnect]
or [func@GObject.signal_handlers_disconnect_by_func]) when the user data (object)
is finalised; this has to be implemented manually. For non-threaded programs,
[func@GObject.signal_connect_object] can be used to implement this automatically.
Currently, however, it is unsafe to use in threaded programs.

The second is to hold a strong reference on the user data until after the
signal is disconnected for other reasons. This can be implemented
automatically using [func@GObject.signal_connect_data].

The first approach is recommended, as the second approach can result in
effective memory leaks of the user data if the signal handler is never
disconnected for some reason.
