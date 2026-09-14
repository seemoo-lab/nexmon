/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Adapted from mpls_ntop and mpls_pton copied from iproute2,
 * lib/mpls_ntop.c and lib/mpls_pton.c
 */

#include "nl-default.h"

#include <stdio.h>

#include <linux/mpls.h>

#include <netlink/netlink-compat.h>

#include "mpls.h"

static const char *mpls_ntop1(const struct mpls_label *addr,
			      char *buf, size_t buflen)
{
	size_t destlen = buflen;
	char *dest = buf;
	int count = 0;

	while (1) {
		uint32_t entry = ntohl(addr[count++].entry);
		uint32_t label = (entry & MPLS_LS_LABEL_MASK) >> MPLS_LS_LABEL_SHIFT;
		int len = snprintf(dest, destlen, "%u", label);

		if (len < 0 || (unsigned)len >= destlen)
			break;

		/* Is this the end? */
		if (entry & MPLS_LS_S_MASK)
			return buf;

		dest += len;
		destlen -= len;
		if (destlen) {
			*dest = '/';
			dest++;
			destlen--;
		}
	}
	errno = E2BIG;

	return NULL;
}

const char *mpls_ntop(int af, const void *addr, char *buf, size_t buflen)
{
	switch(af) {
	case AF_MPLS:
		errno = 0;
		return mpls_ntop1((struct mpls_label *)addr, buf, buflen);
	}

	errno = EINVAL;
	return NULL;
}

static int mpls_pton1(const char *name, struct mpls_label *addr,
		      unsigned int maxlabels)
{
	char *endp;
	unsigned count;

	for (count = 0; count < maxlabels; count++) {
		unsigned long label;

		label = strtoul(name, &endp, 0);
		/* Fail when the label value is out or range */
		if (label >= (1 << 20))
			return 0;

		if (endp == name) /* no digits */
			return 0;

		addr->entry = htonl(label << MPLS_LS_LABEL_SHIFT);
		if (*endp == '\0') {
			addr->entry |= htonl(1 << MPLS_LS_S_SHIFT);
			return (count + 1) * sizeof(struct mpls_label);
		}

		/* Bad character in the address */
		if (*endp != '/')
			return 0;

		name = endp + 1;
		addr += 1;
	}

	/* The address was too long */
	return 0;
}

int mpls_pton(int af, const char *src, void *addr, size_t alen)
{
	unsigned int maxlabels = alen / sizeof(struct mpls_label);
	int err;

	switch(af) {
	case AF_MPLS:
		errno = 0;
		err = mpls_pton1(src, (struct mpls_label *)addr, maxlabels);
		break;
	default:
		errno = EAFNOSUPPORT;
		err = -1;
	}

	return err;
}
