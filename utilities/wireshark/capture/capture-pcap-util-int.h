/** @file
 *
 * Definitions of routines internal to the libpcap/WinPcap/Npcap utilities
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __PCAP_UTIL_INT_H__
#define __PCAP_UTIL_INT_H__

/**
 * @brief Adds an address to an interface information structure.
 *
 * @param if_info Pointer to the interface information structure.
 * @param addr Pointer to the sockaddr structure containing the address to add.
 */
extern void if_info_add_address(if_info_t *if_info, struct sockaddr *addr);
#ifdef HAVE_PCAP_REMOTE
/**
 * @brief Retrieve a remote interface list from a capture server.
 *
 * @param hostname   Remote host name.
 * @param port       Remote capture port.
 * @param auth_type  Authentication type to use.
 * @param username   Username for authentication (may be NULL).
 * @param passwd     Password for authentication (may be NULL).
 * @param err        Output parameter set to an error code on failure.
 * @param err_str    Output parameter set to an error message on failure.
 *
 * @return A GList* of remote interfaces on success, or NULL on failure.
 */
extern GList *get_remote_interface_list_common(const char *hostname,
    const char *port, int auth_type, const char *username, const char *passwd,
    int *err, char **err_str);
#endif /* HAVE_PCAP_REMOTE */

/**
 * @brief Retrieves a list of local network interfaces.
 *
 * @param err Pointer to an integer that receives an error code.
 * @param err_str Pointer to a string that receives an error message.
 * @return A GList containing pointers to if_info_t structures representing the local interfaces, or NULL on failure.
 */
extern GList *get_local_interface_list(int *err, char **err_str);

/**
 * @brief Retrieves local interface capabilities.
 *
 * @param interface_opts Pointer to the interface options structure.
 * @param status Pointer to a cap_device_open_status structure that receives the open status.
 * @param status_str Pointer to a string that receives an error message if any.
 * @return A pointer to an if_capabilities_t structure representing the local interface capabilities, or NULL on failure.
 */
extern if_capabilities_t *get_if_capabilities_local(interface_options *interface_opts,
    cap_device_open_status *status, char **status_str);

/**
 * @brief Opens a capture device using pcap_create.
 *
 * @param capture_opts Pointer to the capture options structure.
 * @param interface_opts Pointer to the interface options structure.
 * @param timeout Timeout for the capture operation.
 * @param open_status Pointer to the capture device open status structure.
 * @param open_status_str Pointer to the buffer for the open status string.
 * @return pcap_t* Pointer to the opened pcap_t structure, or NULL on failure.
 */
extern pcap_t *open_capture_device_local(capture_options *capture_opts,
    interface_options *interface_opts, int timeout,
    cap_device_open_status *open_status,
    char (*open_status_str)[PCAP_ERRBUF_SIZE]);

/**
 * @brief Retrieves capabilities for a network interface using pcap.
 *
 * @param interface_opts Pointer to the interface options structure.
 * @param status Pointer to a cap_device_open_status structure that receives the open status.
 * @param status_str Pointer to a string that receives an error message if any.
 * @return A pointer to an if_capabilities_t structure containing the capabilities, or NULL on failure.
 */
extern if_capabilities_t *get_if_capabilities_pcap_create(interface_options *interface_opts,
    cap_device_open_status *status, char **status_str);

/**
 * @brief Open a capture device using pcap_create() with the given options.
 *
 * @param capture_opts      Global capture options.
 * @param interface_opts    Per‑interface capture options.
 * @param timeout           Timeout (in milliseconds) for packet buffering.
 * @param open_status       Output parameter describing the open result.
 * @param open_status_str   Output buffer for a human‑readable status message.
 *
 * @return A pcap_t* handle on success, or NULL on failure.
 */
extern pcap_t *open_capture_device_pcap_create(capture_options *capture_opts,
    interface_options *interface_opts, int timeout,
    cap_device_open_status *open_status,
    char (*open_status_str)[PCAP_ERRBUF_SIZE]);

/**
 * @brief Get an error message string for a get_interface_list() CANT_GET_INTERFACE_LIST error.
 *
 * Get an error message string for a CANT_GET_INTERFACE_LIST error from
 * "get_interface_list()".  This is used to let the error message string
 * be platform-dependent.
 *
 * @param err_str The error string from the underlying system.
 * @return A pointer to a string containing the error message, or NULL if no additional information is available.
 */
extern char *cant_get_if_list_error_message(const char *err_str);

/**
 * @brief Retrieves a secondary error message for a given capture device open status.
 *
 * Get a longer, secondary error message corresponding to why getting
 * capabilities or opening a device failed. This is used to let the error
 * message string be platform-dependent.
 * @param open_status The capture device open status code.
 * @param open_status_str A string representation of the open status.
 * @return A pointer to a constant character string representing the secondary error message, or NULL if none is available.
 */
extern const char *get_pcap_failure_secondary_error_message(cap_device_open_status open_status,
    const char *open_status_str);

#endif /* __PCAP_UTIL_INT_H__ */
