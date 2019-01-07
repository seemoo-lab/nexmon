/* packet-usb.h
 *
 * $Id: packet-usb.h 32828 2010-05-16 03:19:40Z guy $
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 */

#ifndef __PACKET_USB_H__
#define __PACKET_USB_H__

typedef struct _usb_address_t {
    guint32 device;
    guint32 endpoint;
} usb_address_t;
#define USB_ADDR_LEN (sizeof(usb_address_t))


typedef struct _usb_conv_info_t usb_conv_info_t;

/* there is one such structure for each request/response */
typedef struct _usb_trans_info_t {
    guint32 request_in;
    guint32 response_in;
    nstime_t req_time;
    guint8 requesttype;
    guint8 request;
    union {
        struct {
            guint8 type;
            guint8 index;
        } get_descriptor;
    } u;


    /* used to pass the interface class from the
     * interface descriptor onto the endpoint
     * descriptors so that we can create a
     * conversation with the appropriate class
     * once we know the endpoint.
     */
    usb_conv_info_t *interface_info;
} usb_trans_info_t;

/* there is one such structure for each device/endpoint conversation */
struct _usb_conv_info_t {
    guint16 interfaceClass;		/* class for this conversation */
    emem_tree_t *transactions;
    usb_trans_info_t *usb_trans_info; /* pointer to the current transaction */
    void *class_data;	/* private class/id decode data */
};

/* This is what a tap will tap */
typedef struct _usb_tap_data_t {
    guint8 urb_type;
    guint8 transfer_type;
    usb_conv_info_t *conv_info;
    usb_trans_info_t *trans_info;
} usb_tap_data_t;

/* This is the endpoint number user for "no endpoint" or the fake endpoint 
 * for the host side since we need two endpoints to manage conversations
 * properly.
 */
#define NO_ENDPOINT 0xffff

/*
 * Values from the Linux USB pseudo-header.
 */

/*
 * event_type values
 */
#define URB_SUBMIT        'S'
#define URB_COMPLETE      'C'
#define URB_ERROR         'E'

/*
 * transfer_type values
 */
#define URB_ISOCHRONOUS   0x0
#define URB_INTERRUPT     0x1
#define URB_CONTROL       0x2
#define URB_BULK          0x3

#define URB_TRANSFER_IN   0x80		/* to host */

#define IF_CLASS_UNKNOWN		0xffff
#define IF_CLASS_FROM_INTERFACE_DESC	0x00
#define IF_CLASS_AUDIO			0x01
#define IF_CLASS_COMMUNICATIONS		0x02
#define IF_CLASS_HID			0x03
#define IF_CLASS_PHYSICAL		0x05
#define IF_CLASS_IMAGE			0x06
#define IF_CLASS_PRINTER		0x07
#define IF_CLASS_MASSTORAGE		0x08
#define IF_CLASS_HUB			0x09
#define IF_CLASS_CDC_DATA		0x0a
#define IF_CLASS_SMART_CARD		0x0b
#define IF_CLASS_CONTENT_SECURITY	0x0d
#define IF_CLASS_VIDEO			0x0e
#define IF_CLASS_DIAGNOSTIC_DEVICE	0xdc
#define IF_CLASS_WIRELESS_CONTROLLER	0xe0
#define IF_CLASS_MISCELLANEOUS		0xef
#define IF_CLASS_APPLICATION_SPECIFIC	0xfe
#define IF_CLASS_VENDOR_SPECIFIC	0xff


/* bmRequestType values */
#define USB_DIR_OUT                     0               /* to device */
#define USB_DIR_IN                      0x80            /* to host */

#define USB_TYPE_MASK                   (0x03 << 5)
#define USB_TYPE(type)                  (((type) & USB_TYPE_MASK) >> 5)
#define RQT_SETUP_TYPE_STANDARD	0
#define RQT_SETUP_TYPE_CLASS	1
#define RQT_SETUP_TYPE_VENDOR	2

#define RQT_SETUP_RECIPIENT_DEVICE      0
#define RQT_SETUP_RECIPIENT_INTERFACE   1
#define RQT_SETUP_RECIPIENT_ENDPOINT    2
#define RQT_SETUP_RECIPIENT_OTHER       3

#endif
