Title: TLS Overview
SPDX-License-Identifier: LGPL-2.1-or-later
SPDX-FileCopyrightText: 2010 Dan Winship
SPDX-FileCopyrightText: 2015 Collabora, Ltd.

# TLS Overview

[class@Gio.TlsConnection] and related classes provide TLS (Transport Layer
Security, previously known as SSL, Secure Sockets Layer) support for GIO-based
network streams.

[iface@Gio.DtlsConnection] and related classes provide DTLS (Datagram TLS)
support for GIO-based network sockets, using the [iface@Gio.DatagramBased]
interface. The TLS and DTLS APIs are almost identical, except TLS is
stream-based and DTLS is datagram-based. They share certificate and backend
infrastructure.

In the simplest case, for a client TLS connection, you can just set the
[property@Gio.SocketClient:tls] flag on a [class@Gio.SocketClient], and then any
connections created by that client will have TLS negotiated automatically, using
appropriate default settings, and rejecting any invalid or self-signed
certificates (unless you change that default by setting the
[property@Gio.SocketClient:tls-validation-flags] property). The returned object
will be a [class@Gio.TcpWrapperConnection], which wraps the underlying
[iface@Gio.TlsClientConnection].

For greater control, you can create your own [iface@Gio.TlsClientConnection],
wrapping a [class@Gio.SocketConnection] (or an arbitrary [class@Gio.IOStream]
with pollable input and output streams) and then connect to its signals,
such as [signal@Gio.TlsConnection::accept-certificate], before starting the
handshake.

Server-side TLS is similar, using [iface@Gio.TlsServerConnection]. At the
moment, there is no support for automatically wrapping server-side
connections in the way [class@Gio.SocketClient] does for client-side
connections.

