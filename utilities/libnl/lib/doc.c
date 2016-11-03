/*
 * lib/doc.c		Documentation Purpose
 *
 *	This library is free software; you can redistribute it and/or
 *	modify it under the terms of the GNU Lesser General Public
 *	License as published by the Free Software Foundation version 2.1
 *	of the License.
 *
 * Copyright (c) 2003-2008 Thomas Graf <tgraf@suug.ch>
 */

/**
 * @mainpage
 *
 * @section intro Introduction
 *
 * libnl is a set of libraries to deal with the netlink protocol and some
 * of the high level protocols implemented on top of it. Its goal is to
 * simplify netlink protocol usage and to create an abstraction layer using
 * object based interfaces for various netlink based subsystems.The library
 * was developed and tested on the 2.6.x kernel releases but it may work with
 * older kernel series.
 *
 * @section toc Table of Contents
 *
 * - \subpage core_doc
 * - \subpage route_doc
 * - \subpage genl_doc
 * - \subpage nf_doc
 *
 * @section remarks Remarks
 *
 * @subsection cache_alloc Allocation of Caches
 *
 * Almost all subsystem provide a function to allocate a new cache
 * of some form. The function usually looks like this:
 * @code
 * struct nl_cache *<object name>_alloc_cache(struct nl_sock *sk);
 * @endcode
 *
 * These functions allocate a new cache for the own object type,
 * initializes it properly and updates it to represent the current
 * state of their master, e.g. a link cache would include all
 * links currently configured in the kernel.
 *
 * Some of the allocation functions may take additional arguments
 * to further specify what will be part of the cache.
 *
 * All such functions return a newly allocated cache or NULL
 * in case of an error.
 *
 * @subsection addr Setting of Addresses
 * @code
 * int <object name>_set_addr(struct nl_object *, struct nl_addr *)
 * @endcode
 *
 * All attribute functions avaiable for assigning addresses to objects
 * take a struct nl_addr argument. The provided address object is
 * validated against the address family of the object if known already.
 * The assignment fails if the address families mismatch. In case the
 * address family has not been specified yet, the address family of
 * the new address is elected to be the new requirement.
 *
 * The function will acquire a new reference on the address object
 * before assignment, the caller is NOT responsible for this.
 *
 * All functions return 0 on success or a negative error code.
 *
 * @subsection flags Flags to Character StringTranslations
 * All functions converting a set of flags to a character string follow
 * the same principles, therefore, the following information applies
 * to all functions convertings flags to a character string and vice versa.
 *
 * @subsubsection flags2str Flags to Character String
 * @code
 * char *<object name>_flags2str(int flags, char *buf, size_t len)
 * @endcode
 * @arg flags		Flags.
 * @arg buf		Destination buffer.
 * @arg len		Buffer length.
 *
 * Converts the specified flags to a character string separated by
 * commas and stores it in the specified destination buffer.
 *
 * @return The destination buffer
 *
 * @subsubsection str2flags Character String to Flags
 * @code
 * int <object name>_str2flags(const char *name)
 * @endcode
 * @arg name		Name of flag.
 *
 * Converts the provided character string specifying a flag
 * to the corresponding numeric value.
 *
 * @return Link flag or a negative value if none was found.
 *
 * @subsubsection type2str Type to Character String
 * @code
 * char *<object name>_<type>2str(int type, char *buf, size_t len)
 * @endcode
 * @arg type		Type as numeric value
 * @arg buf		Destination buffer.
 * @arg len		Buffer length.
 *
 * Converts an identifier (type) to a character string and stores
 * it in the specified destination buffer.
 *
 * @return The destination buffer or the type encoded in hexidecimal
 *         form if the identifier is unknown.
 *
 * @subsubsection str2type Character String to Type
 * @code
 * int <object name>_str2<type>(const char *name)
 * @endcode
 * @arg name		Name of identifier (type).
 *
 * Converts the provided character string specifying a identifier
 * to the corresponding numeric value.
 *
 * @return Identifier as numeric value or a negative value if none was found.
 *
 * @page core_doc Core Library (-lnl)
 * 
 * @section core_intro Introduction
 *
 * The core library contains the fundamentals required to communicate over
 * netlink sockets. It deals with connecting and unconnecting of sockets,
 * sending and receiving of data, provides a customizeable receiving state
 * machine, and provides a abstract data type framework which eases the
 * implementation of object based netlink protocols where objects are added,
 * removed, or modified with the help of netlink messages.
 *
 * @section core_toc Table of Contents
 * 
 * - \ref proto_fund
 * - \ref sk_doc
 * - \ref rxtx_doc
 * - \ref cb_doc
 *
 * @section proto_fund Netlink Protocol Fundamentals
 *
 * The netlink protocol is a socket based IPC mechanism used for communication
 * between userspace processes and the kernel. The netlink protocol uses the
 * \c AF_NETLINK address family and defines a protocol type for each subsystem
 * protocol (e.g. NETLINK_ROUTE, NETLINK_NETFILTER, etc). Its addressing
 * schema is based on a 32 bit port number, formerly referred to as PID, which
 * uniquely identifies each peer.
 *
 * The netlink protocol is based on messages each limited to the size of a
 * memory page and consists of the netlink message header (struct nlmsghdr)
 * plus the payload attached to it. The payload can consist of arbitary data
 * but often contains a fixed sized family specifc header followed by a
 * stream of \ref attr_doc. The use of attributes dramatically increases
 * the flexibility of the protocol and allows for the protocol to be
 * extended while maintaining backwards compatibility.
 *
 * The netlink message header (struct nlmsghdr):
 * @code   
 * 0                   1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-------------------------------------------------------------+
 * |                          Length                             |
 * +------------------------------+------------------------------+
 * |            Type              |           Flags              |
 * +------------------------------+------------------------------+
 * |                      Sequence Number                        |
 * +-------------------------------------------------------------+
 * |                       Port (Address)                        |
 * +-------------------------------------------------------------+
 * @endcode
 *
 * Netlink differs between requests, notifications, and replies. Requests
 * are messages which have the \c NLM_F_REQUEST flag set and are meant to
 * request an action from the receiver. A request is typically sent from
 * a userspace process to the kernel. Every request should be assigned a
 * sequence number which should be incremented for each request sent on the
 * sending side. Depending on the nature of the request, the receiver may
 * reply to the request with regular netlink messages which should contain
 * the same sequence number as the request it relates to. Notifications are
 * of informal nature and don't expect a reply, therefore the sequence number
 * is typically set to 0. It should be noted that unlike in protocols such as
 * TCP there is no strict enforcment of the sequence number. The sole purpose
 * of sequence numbers is to assist a sender in relating replies to the
 * corresponding requests.
 *
 * @msc
 * A,B;
 * A=>B [label="GET (seq=1, NLM_F_REQUEST)"];
 * A<=B [label="PUT (seq=1)"];
 * ...;
 * A<=B [label="NOTIFY (seq=0)"];
 * @endmsc
 *
 * If the size of a reply exceeds the size of a memory page and thus exceeds
 * the maximum message size, the reply can be split into a series of multipart
 * messages. A multipart message has the \c flag NLM_F_MULTI set and the
 * receiver is expected to continue parsing the reply until the special
 * message type \c NLMSG_DONE is received.
 *
 * @msc
 * A,B;
 * A=>B [label="GET (seq=1, NLM_F_REQUEST)"];
 * A<=B [label="PUT (seq=1, NLM_F_MULTI)"];
 * ...;
 * A<=B [label="PUT (seq=1, NLM_F_MULTI)"];
 * A<=B [label="NLMSG_DONE (seq=1)"];
 * @endmsc
 *
 * Errors can be reported using the standard message type \c NLMSG_ERROR which
 * can carry an error code and the netlink mesage header of the request.
 * Error messages should set their sequence number to the sequence number
 * of the message which caused the error.
 *
 * @msc
 * A,B;
 * A=>B [label="GET (seq=1, NLM_F_REQUEST)"];
 * A<=B [label="NLMSG_ERROR code=EINVAL (seq=1)"];
 * @endmsc
 *
 * The \c NLMSG_ERROR message type is also used to send acknowledge messages.
 * An acknowledge message can be requested by setting the \c NLM_F_ACK flag
 * message except that the error code is set to 0.
 *
 * @msc
 * A,B;
 * A=>B [label="GET (seq=1, NLM_F_REQUEST | NLM_F_ACK)"];
 * A<=B [label="ACK (seq=1)"];
 * @endmsc
 *
 * @section sk_doc Dealing with Netlink Sockets
 *
 * In order to use the netlink protocol, a netlink socket is required. Each
 * socket defines a completely independent context for sending and receiving
 * of messages. The netlink socket and all its related attributes are
 * represented by struct nl_sock.
 *
 * @code
 * nl_socket_alloc()                      Allocate new socket structure.
 * nl_socket_free(s)                      Free socket structure.
 * @endcode
 *
 * @subsection local_port Local Port
 * The local port number uniquely identifies the socket and is used to
 * address it. A unique local port is generated automatically when the socket
 * is allocated. It will consist of the Process ID (22 bits) and a random
 * number (10 bits) to allow up to 1024 sockets per process.
 *
 * @code
 * nl_socket_get_local_port(sk)           Return the peer's port number.
 * nl_socket_set_local_port(sk, port)     Set the peer's port number.
 * @endcode
 *
 * @subsection peer_port Peer Port
 * A peer port can be assigned to the socket which will result in all unicast
 * messages sent over the socket to be addresses to the corresponding peer. If
 * no peer is specified, the kernel will try to automatically bind the socket
 * to a kernel side socket of the same netlink protocol family. It is common
 * practice not to bind the socket to a peer port as typically only one kernel
 * side socket exists per netlink protocol family.
 *
 * @code
 * nl_socket_get_peer_port(sk)            Return the local port number.
 * nl_socket_set_peer_port(sk, port)      Set the local port number.
 * @endcode
 *
 * @subsection sock_fd File Descriptor
 * The file descriptor of the socket(2).
 *
 * @code
 * nl_socket_get_fd(sk)                   Return file descriptor.
 * nl_socket_set_buffer_size(sk, rx, tx)  Set buffer size of socket.
 * nl_socket_set_nonblocking(sk)          Set socket to non-blocking state.
 * @endcode
 *
 * @subsection group_sub Group Subscriptions
 * Each socket can subscribe to multicast groups of the netlink protocol
 * family it is bound to. The socket will then receive a copy of each
 * message sent to any of the groups. Multicast groups are commonly used
 * to implement event notifications. Prior to kernel 2.6.14 the group
 * subscription was performed using a bitmask which limited the number of
 * groups per protocol family to 32. This outdated interface can still be
 * accessed via the function nl_join_groups even though it is not recommended
 * for new code. Starting with 2.6.14 a new method was introduced which
 * supports subscribing to an almost unlimited number of multicast groups.
 *
 * @code
 * nl_socket_add_membership(sk, group)    Become a member of a multicast group.
 * nl_socket_drop_membership(sk, group)   Drop multicast group membership.
 * nl_join_groups(sk, groupmask)          Join a multicast group (obsolete).
 * @endcode
 *
 * @subsection seq_num Sequence Numbers
 * The socket keeps track of the sequence numbers used. The library will
 * automatically verify the sequence number of messages received unless
 * the check was disabled using the function nl_socket_disable_seq_check().
 * When a message is sent using nl_send_auto_complete(), the sequence number
 * is automatically filled in, and replies will be verified correctly.
 *
 * @code
 * nl_socket_disable_seq_check(sk)        Disable checking of sequece numbers.
 * nl_socket_use_seq(sk)                  Use sequence number and bump to next.
 * @endcode
 *
 * @subsection sock_cb Callback Configuration
 * Every socket is associated a callback configuration which enables the
 * applications to hook into various internal functions and control the
 * receiving and sendings semantics. For more information, see section
 * \ref cb_doc.
 *
 * @code
 * nl_socket_alloc_cb(cb)                 Allocate socket based on callback set.
 * nl_socket_get_cb(sk)                   Return callback configuration.
 * nl_socket_set_cb(sk, cb)               Replace callback configuration.
 * nl_socket_modify_cb(sk, ...)           Modify a specific callback function.
 * @endcode
 *
 * @subsection sk_other Other Functions
 * @code
 * nl_socket_enable_auto_ack(sock)        Enable automatic request of ACK.
 * nl_socket_disable_auto_ack(sock)       Disable automatic request of ACK.
 * nl_socket_enable_msg_peek(sock)        Enable message peeking.
 * nl_socket_disable_msg_peek(sock)       Disable message peeking.
 * nl_socket_set_passcred(sk, state)      Enable/disable credential passing.
 * nl_socket_recv_pktinfo(sk, state)      Enable/disable packet information.
 * @endcode
 *
 * @section rxtx_doc Sending and Receiving of Data
 *
 * @subsection recv_semantisc Receiving Semantics
 * @code
 *          nl_recvmsgs_default(set)
 *                 | cb = nl_socket_get_cb(sk)
 *                 v
 *          nl_recvmsgs(sk, cb)
 *                 |           [Application provides nl_recvmsgs() replacement]
 *                 |- - - - - - - - - - - - - - - v
 *                 |                     cb->cb_recvmsgs_ow()
 *                 |
 *                 |               [Application provides nl_recv() replacement]
 * +-------------->|- - - - - - - - - - - - - - - v
 * |           nl_recv()                   cb->cb_recv_ow()
 * |  +----------->|<- - - - - - - - - - - - - - -+
 * |  |            v
 * |  |      Parse Message
 * |  |            |- - - - - - - - - - - - - - - v
 * |  |            |                         NL_CB_MSG_IN()
 * |  |            |<- - - - - - - - - - - - - - -+
 * |  |            |
 * |  |            |- - - - - - - - - - - - - - - v
 * |  |      Sequence Check                NL_CB_SEQ_CHECK()
 * |  |            |<- - - - - - - - - - - - - - -+
 * |  |            |
 * |  |            |- - - - - - - - - - - - - - - v  [ NLM_F_ACK is set ]
 * |  |            |                      NL_CB_SEND_ACK()
 * |  |            |<- - - - - - - - - - - - - - -+
 * |  |            |
 * |  |      +-----+------+--------------+----------------+--------------+
 * |  |      v            v              v                v              v
 * |  | Valid Message    ACK       NO-OP Message  End of Multipart     Error
 * |  |      |            |              |                |              |
 * |  |      v            v              v                v              v
 * |  |NL_CB_VALID()  NL_CB_ACK()  NL_CB_SKIPPED()  NL_CB_FINISH()  cb->cb_err()
 * |  |      |            |              |                |              |
 * |  |      +------------+--------------+----------------+              v
 * |  |                                  |                           (FAILURE)
 * |  |                                  |  [Callback returned NL_SKIP]
 * |  |  [More messages to be parsed]    |<-----------
 * |  +----------------------------------|
 * |                                     |
 * |         [is Multipart message]      |
 * +-------------------------------------|  [Callback returned NL_STOP]
 *                                       |<-----------
 *                                       v
 *                                   (SUCCESS)
 *
 *                          At any time:
 *                                Message Format Error
 *                                         |- - - - - - - - - - - - v
 *                                         v                  NL_CB_INVALID()
 *                                     (FAILURE)
 *
 *                                Message Overrun (Kernel Lost Data)
 *                                         |- - - - - - - - - - - - v
 *                                         v                  NL_CB_OVERRUN()
 *                                     (FAILURE)
 *
 *                                Callback returned negative error code
 *                                     (FAILURE)
 * @endcode
 *
 * @subsection send_semantics Sending Semantisc
 *
 * @code
 *     nl_send_auto_complete(sk, msg)
 *             | [Automatically completes netlink message header]
 *             | [(local port, sequence number)                 ]
 *             |
 *             |                   [Application provies nl_send() replacement]
 *             |- - - - - - - - - - - - - - - - - - - - v
 *             v                                 cb->cb_send_ow()
 *         nl_send(sk, msg)
 *             | [If available, add peer port and credentials]
 *             v
 *        nl_sendmsg(sk, msg, msghdr)
 *             |- - - - - - - - - - - - - - - - - - - - v
 *             |                                 NL_CB_MSG_OUT()
 *             |<- - - - - - - - - - - - - - - - - - - -+
 *             v
 *         sendmsg()
 * @endcode
 *
 * @section cb_doc Callback Configurations
 * Callbacks and overwriting capabilities are provided to control various
 * semantics of the library. All callback functions are packed together in
 * struct nl_cb which is attached to a netlink socket or passed on to 
 * the respective functions directly.
 *
 * @subsection cb_ret_doc Callback Return Values
 * Callback functions can control the flow of the calling layer by returning
 * appropriate error codes:
 * @code
 * Action ID        | Description
 * -----------------+-------------------------------------------------------
 * NL_OK            | Proceed with whatever comes next.
 * NL_SKIP          | Skip message currently being processed and continue
 *                  | with next message.
 * NL_STOP          | Stop parsing and discard all remaining messages in
 *                  | this set of messages.
 * @endcode
 *
 * All callbacks are optional and a default action is performed if no 
 * application specific implementation is provided:
 *
 * @code
 * Callback ID       | Default Return Value
 * ------------------+----------------------
 * NL_CB_VALID       | NL_OK
 * NL_CB_FINISH      | NL_STOP
 * NL_CB_OVERRUN     | NL_STOP
 * NL_CB_SKIPPED     | NL_SKIP
 * NL_CB_ACK         | NL_STOP
 * NL_CB_MSG_IN      | NL_OK
 * NL_CB_MSG_OUT     | NL_OK
 * NL_CB_INVALID     | NL_STOP
 * NL_CB_SEQ_CHECK   | NL_OK
 * NL_CB_SEND_ACK    | NL_OK
 *                   |
 * Error Callback    | NL_STOP
 * @endcode
 *
 * In order to simplify typical usages of the library, different sets of
 * default callback implementations exist:
 * @code
 * NL_CB_DEFAULT: No additional actions
 * NL_CB_VERBOSE: Automatically print warning and error messages to a file
 *                descriptor as appropriate. This is useful for CLI based
 *                applications.
 * NL_CB_DEBUG:   Print informal debugging information for each message
 *                received. This will result in every message beint sent or
 *                received to be printed to the screen in a decoded,
 *                human-readable format.
 * @endcode
 *
 * @par 1) Setting up a callback set
 * @code
 * // Allocate a callback set and initialize it to the verbose default set
 * struct nl_cb *cb = nl_cb_alloc(NL_CB_VERBOSE);
 *
 * // Modify the set to call my_func() for all valid messages
 * nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, my_func, NULL);
 *
 * // Set the error message handler to the verbose default implementation
 * // and direct it to print all errors to the given file descriptor.
 * FILE *file = fopen(...);
 * nl_cb_err(cb, NL_CB_VERBOSE, NULL, file);
 * @endcode
 *
 * @page route_doc Routing Family
 *
 * @page genl_doc Generic Netlink Family
 *
 * @page nf_doc Netfilter Subsystem
 */
