/** @file
 *
 * Utility definitions for packet capture
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef __CAPTURE_PCAP_UTIL_H__
#define __CAPTURE_PCAP_UTIL_H__

#include <wsutil/feature_list.h>

#ifdef HAVE_LIBPCAP

#include <pcap/pcap.h>

#include "ui/capture_opts.h"

#endif

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef HAVE_LIBPCAP
/*
 * A snapshot length of 0 is useless - and libpcap/WinPcap/Npcap don't guarantee
 * that a snapshot length of 0 will work, and, on some platforms, it won't
 * (with BPF, for example, the kernel is told the snapshot length via the
 * return value of the BPF program, and a return value of 0 means "drop
 * the packet"), so the minimum packet size is 1 byte.
 */
#define MIN_PACKET_SIZE 1	/* minimum amount of packet data we can read */

/**
 * @brief Get the local interface list from the system.
 *
 * @param err       Output parameter set to an error code on failure.
 * @param err_str   Output parameter set to an error message on failure.
 *
 * @return A GList* of local interfaces on success, or NULL on failure.
 */
GList *get_local_interface_list_ws(int *err, char **err_str);

/**
 * @brief Get the local interface list using the Stratoshark backend.
 *
 * @param err       Output parameter set to an error code on failure.
 * @param err_str   Output parameter set to an error message on failure.
 *
 * @return A GList* of local interfaces on success, or NULL on failure.
 */
GList* get_local_interface_list_ss(int* err, char** err_str);

#ifdef HAVE_PCAP_REMOTE
/**
 * @brief Retrieve a remote interface list.
 *
 * @param hostname         Remote host name.
 * @param port             Remote capture port.
 * @param wireshark_remote Whether to use the Wireshark remote‑capture protocol.
 * @param auth_type        Authentication type.
 * @param username         Username for authentication (may be NULL).
 * @param passwd           Password for authentication (may be NULL).
 * @param err              Output parameter set to an error code on failure.
 * @param err_str          Output parameter set to an error message on failure.
 *
 * @return A list of remote interfaces on success, or NULL on failure.
 */
GList *get_remote_interface_list(const char *hostname, const char *port,
                                 bool wireshark_remote,
                                 int auth_type, const char *username,
                                 const char *passwd,
                                 int *err, char **err_str);
#endif /* HAVE_PCAP_REMOTE */

/**
 * @brief Convert a DLT_/LINKTYPE_ value to its corresponding name string.
 *
 * @param dlt The numeric link‑layer type value.
 * @return A constant string naming the link‑layer type, or "UNKNOWN"
 *         if the value has no recognized mapping.
 */
const char *linktype_val_to_name(int dlt);

/**
 * @brief Convert a LINKTYPE/DLT name string to its numeric value.
 * @param linktype  The textual name of the link‑layer type.
 * @return The numeric DLT_/LINKTYPE_ value on success, or -1 if the
 *         name is unrecognized.
 */
int linktype_name_to_val(const char *linktype);

/**
 * @brief Retrieve the datalink (DLT_/LINKTYPE_) value for a pcap handle.
 *
 * @param pch          The pcap handle obtained from pcap_create/pcap_open.
 * @param devicename   Name of the device, used for error reporting.
 * @return The datalink type on success, or -1 on failure.
 */

int get_pcap_datalink(pcap_t *pch, const char *devicename);

/**
 * @brief Set the datalink (DLT_/LINKTYPE_) value for a pcap handle.
 *
 * @param pcap_h                The pcap handle to configure.
 * @param datalink              The desired DLT_/LINKTYPE_ numeric value.
 * @param name                  Optional buffer to receive the datalink name.
 * @param errmsg                Buffer for the primary error message.
 * @param errmsg_len            Size of @p errmsg.
 * @param secondary_errmsg      Buffer for additional error information.
 * @param secondary_errmsg_len  Size of @p secondary_errmsg.
 * @return true on success, false on failure.
 */
bool set_pcap_datalink(pcap_t *pcap_h, int datalink, char *name,
    char *errmsg, size_t errmsg_len,
    char *secondary_errmsg, size_t secondary_errmsg_len);

/**
 * @brief Determine whether a pcap handle provides high‑resolution timestamps.
 *
 * Return true if the pcap_t in question is set up for high‑precision
 * time stamps, false otherwise.
 *
 * @param pcap_h   The pcap handle to query.
 * @return true if high‑resolution timestamps are enabled, false otherwise.
 */
bool have_high_resolution_timestamp(pcap_t *pcap_h);

/*
 * Capture device open status values.
 */
typedef enum {
    /* No error and no warning */
    CAP_DEVICE_OPEN_NO_ERR,

    /* Errors corresponding to libpcap errors */
    CAP_DEVICE_OPEN_ERROR_NO_SUCH_DEVICE,
    CAP_DEVICE_OPEN_ERROR_RFMON_NOTSUP,
    CAP_DEVICE_OPEN_ERROR_PERM_DENIED,
    CAP_DEVICE_OPEN_ERROR_IFACE_NOT_UP,
    CAP_DEVICE_OPEN_ERROR_PROMISC_PERM_DENIED,

    /* Error, none of the above */
    CAP_DEVICE_OPEN_ERROR_OTHER,

    /* Error from pcap_open_live() or pcap_open() rather than pcap_activate() */
    CAP_DEVICE_OPEN_ERROR_GENERIC,

    /* Warnings corresponding to libpcap warnings */
    CAP_DEVICE_OPEN_WARNING_PROMISC_NOTSUP,
    CAP_DEVICE_OPEN_WARNING_TSTAMP_TYPE_NOTSUP,

    /* Warning, none of the above */
    CAP_DEVICE_OPEN_WARNING_OTHER
} cap_device_open_status;
extern if_capabilities_t *get_if_capabilities(interface_options *interface_opts,
    cap_device_open_status *status, char **status_str);
extern pcap_t *open_capture_device(capture_options *capture_opts,
    interface_options *interface_opts,
    int timeout, cap_device_open_status *open_status,
    char (*open_status_str)[PCAP_ERRBUF_SIZE]);

#endif /* HAVE_LIBPCAP */

/**
 * @brief Append to a GString an indication of the version of capture libraries with which we're running, or an indication that we're not running with capture libraries if we were compiled with WinPcap or Npcap but WinPcap/Npcap wasn't loaded, or nothing if we weren't compiled with libpcap/WinPcap/Npcap.
 * @param l Feature list to which the information will be appended.
 */
extern void gather_caplibs_compile_info(feature_list l);

/**
 * @brief Append runtime capture‑library version information to a feature list.
 *
 * Append to a GString an indication of the version of capture libraries
 * with which we're running, or an indication that we're not running
 * with capture libraries, if we were compiled with WinPcap or Npcap but
 * WinPcap/Npcap wasn't loaded, or nothing, if we weren't compiled with
 * libpcap/WinPcap/Npcap.
 *
 * @param l  The feature list to which the runtime information is appended.
 */
extern void gather_caplibs_runtime_info(feature_list l);

#ifdef _WIN32
extern bool caplibs_have_npcap(void);

/**
 * @brief Retrieves the version of Npcap.
 *
 * @param major Pointer to store the major version number.
 * @param minor Pointer to store the minor version number.
 * @return true if successful, false otherwise.
 */
extern bool caplibs_get_npcap_version(unsigned int *major,
    unsigned int *minor);
#endif

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __CAPTURE_PCAP_UTIL_H__ */
