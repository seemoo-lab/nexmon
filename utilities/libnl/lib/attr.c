/* SPDX-License-Identifier: LGPL-2.1-only */
/*
 * Copyright (c) 2003-2013 Thomas Graf <tgraf@suug.ch>
 */

#include "nl-default.h"

#include <linux/socket.h>

#include <netlink/netlink.h>
#include <netlink/utils.h>
#include <netlink/addr.h>
#include <netlink/attr.h>
#include <netlink/msg.h>

#include "nl-priv-dynamic-core/nl-core.h"
#include "nl-aux-core/nl-core.h"

/**
 * @ingroup msg
 * @defgroup attr Attributes
 * Netlink Attributes Construction/Parsing Interface
 *
 * Related sections in the development guide:
 * - @core_doc{core_attr,Netlink Attributes}
 *
 * @{
 *
 * Header
 * ------
 * ~~~~{.c}
 * #include <netlink/attr.h>
 * ~~~~
 */

/**
 * @name Attribute Size Calculation
 * @{
 */

/**
 * Return size of attribute whithout padding.
 * @arg payload		Payload length of attribute.
 *
 * @code
 *    <-------- nla_attr_size(payload) --------->
 *   +------------------+- - -+- - - - - - - - - +- - -+
 *   | Attribute Header | Pad |     Payload      | Pad |
 *   +------------------+- - -+- - - - - - - - - +- - -+
 * @endcode
 *
 * @return Size of attribute in bytes without padding.
 */
int nla_attr_size(int payload)
{
	return NLA_HDRLEN + payload;
}

/**
 * Return size of attribute including padding.
 * @arg payload		Payload length of attribute.
 *
 * @code
 *    <----------- nla_total_size(payload) ----------->
 *   +------------------+- - -+- - - - - - - - - +- - -+
 *   | Attribute Header | Pad |     Payload      | Pad |
 *   +------------------+- - -+- - - - - - - - - +- - -+
 * @endcode
 *
 * @return Size of attribute in bytes.
 */
int nla_total_size(int payload)
{
	return NLA_ALIGN(nla_attr_size(payload));
}

/**
 * Return length of padding at the tail of the attribute.
 * @arg payload		Payload length of attribute.
 *
 * @code
 *   +------------------+- - -+- - - - - - - - - +- - -+
 *   | Attribute Header | Pad |     Payload      | Pad |
 *   +------------------+- - -+- - - - - - - - - +- - -+
 *                                                <--->  
 * @endcode
 *
 * @return Length of padding in bytes.
 */
int nla_padlen(int payload)
{
	return nla_total_size(payload) - nla_attr_size(payload);
}

/** @} */

/**
 * @name Parsing Attributes
 * @{
 */

/**
 * Return type of the attribute.
 * @arg nla		Attribute.
 *
 * @return Type of attribute.
 */
int nla_type(const struct nlattr *nla)
{
	return nla->nla_type & NLA_TYPE_MASK;
}

/**
 * Return pointer to the payload section.
 * @arg nla		Attribute.
 *
 * @return Pointer to start of payload section.
 */
void *nla_data(const struct nlattr *nla)
{
	return (char *) nla + NLA_HDRLEN;
}

/**
 * Return length of the payload .
 * @arg nla		Attribute
 *
 * @return Length of payload in bytes.
 */
int nla_len(const struct nlattr *nla)
{
	return _nla_len(nla);
}

/**
 * Check if the attribute header and payload can be accessed safely.
 * @arg nla		Attribute of any kind.
 * @arg remaining	Number of bytes remaining in attribute stream.
 *
 * Verifies that the header and payload do not exceed the number of
 * bytes left in the attribute stream. This function must be called
 * before access the attribute header or payload when iterating over
 * the attribute stream using nla_next().
 *
 * @return True if the attribute can be accessed safely, false otherwise.
 */
int nla_ok(const struct nlattr *nla, int remaining)
{
	_NL_STATIC_ASSERT(sizeof(*nla) == NLA_HDRLEN);

	return remaining >= (int) sizeof(*nla) &&
	       nla->nla_len >= sizeof(*nla) &&
	       nla->nla_len <= remaining;
}

/**
 * Return next attribute in a stream of attributes.
 * @arg nla		Attribute of any kind.
 * @arg remaining	Variable to count remaining bytes in stream.
 *
 * Calculates the offset to the next attribute based on the attribute
 * given. The attribute provided is assumed to be accessible, the
 * caller is responsible to use nla_ok() beforehand. The offset (length
 * of specified attribute including padding) is then subtracted from
 * the remaining bytes variable and a pointer to the next attribute is
 * returned.
 *
 * nla_next() can be called as long as remainig is >0.
 *
 * @return Pointer to next attribute.
 */
struct nlattr *nla_next(const struct nlattr *nla, int *remaining)
{
	int totlen = NLA_ALIGN(nla->nla_len);

	*remaining -= totlen;
	return (struct nlattr *) ((char *) nla + totlen);
}

static uint16_t nla_attr_minlen[NLA_TYPE_MAX+1] = {
	[NLA_U8]	= sizeof(uint8_t),
	[NLA_U16]	= sizeof(uint16_t),
	[NLA_U32]	= sizeof(uint32_t),
	[NLA_U64]	= sizeof(uint64_t),
	[NLA_STRING]	= 1,
	[NLA_FLAG]	= 0,
	[NLA_SINT]	= sizeof(uint32_t),
	[NLA_UINT]	= sizeof(uint32_t),
};

static int validate_nla(const struct nlattr *nla, int maxtype,
			const struct nla_policy *policy)
{
	const struct nla_policy *pt;
	unsigned int minlen = 0;
	int type = nla_type(nla);

	if (type < 0 || type > maxtype)
		return 0;

	pt = &policy[type];

	if (pt->type > NLA_TYPE_MAX)
		BUG();

	if (pt->minlen)
		minlen = pt->minlen;
	else if (pt->type != NLA_UNSPEC)
		minlen = nla_attr_minlen[pt->type];

	if (_nla_len(nla) < minlen)
		return -NLE_RANGE;

	if (pt->maxlen && nla_len(nla) > pt->maxlen)
		return -NLE_RANGE;

	if (pt->type == NLA_STRING) {
		const char *data = nla_data(nla);
		if (data[nla_len(nla) - 1] != '\0')
			return -NLE_INVAL;
	}

	return 0;
}


/**
 * Create attribute index based on a stream of attributes.
 * @arg tb		Index array to be filled (maxtype+1 elements).
 * @arg maxtype		Maximum attribute type expected and accepted.
 * @arg head		Head of attribute stream.
 * @arg len		Length of attribute stream.
 * @arg policy		Attribute validation policy.
 *
 * Iterates over the stream of attributes and stores a pointer to each
 * attribute in the index array using the attribute type as index to
 * the array. Attribute with a type greater than the maximum type
 * specified will be silently ignored in order to maintain backwards
 * compatibility. If \a policy is not NULL, the attribute will be
 * validated using the specified policy.
 *
 * @see nla_validate
 * @return 0 on success or a negative error code.
 */
int nla_parse(struct nlattr *tb[], int maxtype, struct nlattr *head, int len,
	      const struct nla_policy *policy)
{
	struct nlattr *nla;
	int rem, err;

	memset(tb, 0, sizeof(struct nlattr *) * (maxtype + 1));

	nla_for_each_attr(nla, head, len, rem) {
		int type = nla_type(nla);

		if (type > maxtype)
			continue;

		if (policy) {
			err = validate_nla(nla, maxtype, policy);
			if (err < 0)
				return err;
		}

		if (tb[type])
			NL_DBG(1, "Attribute of type %#x found multiple times in message, "
				  "previous attribute is being ignored.\n", type);

		tb[type] = nla;
	}

	if (rem > 0) {
		NL_DBG(1, "netlink: %d bytes leftover after parsing "
		       "attributes.\n", rem);
	}

	return 0;
}

/**
 * Validate a stream of attributes.
 * @arg head		Head of attributes stream.
 * @arg len		Length of attributes stream.
 * @arg maxtype		Maximum attribute type expected and accepted.
 * @arg policy		Validation policy.
 *
 * Iterates over the stream of attributes and validates each attribute
 * one by one using the specified policy. Attributes with a type greater
 * than the maximum type specified will be silently ignored in order to
 * maintain backwards compatibility.
 *
 * See section @core_doc{core_attr_parse,Attribute Parsing} for more details.
 *
 * @return 0 on success or a negative error code.
 */
int nla_validate(const struct nlattr *head, int len, int maxtype,
		 const struct nla_policy *policy)
{
	const struct nlattr *nla;
	int rem, err;

	nla_for_each_attr(nla, head, len, rem) {
		err = validate_nla(nla, maxtype, policy);
		if (err < 0)
			goto errout;
	}

	err = 0;
errout:
	return err;
}

/**
 * Find a single attribute in a stream of attributes.
 * @arg head		Head of attributes stream.
 * @arg len		Length of attributes stream.
 * @arg attrtype	Attribute type to look for.
 *
 * Iterates over the stream of attributes and compares each type with
 * the type specified. Returns the first attribute which matches the
 * type.
 *
 * @return Pointer to attribute found or NULL.
 */
struct nlattr *nla_find(const struct nlattr *head, int len, int attrtype)
{
	const struct nlattr *nla;
	int rem;

	nla_for_each_attr(nla, head, len, rem)
		if (nla_type(nla) == attrtype)
			return (struct nlattr*)nla;

	return NULL;
}

/** @} */

/**
 * @name Helper Functions
 * @{
 */

/**
 * Copy attribute payload to another memory area.
 * @arg dest		Pointer to destination memory area.
 * @arg src		Attribute
 * @arg count		Number of bytes to copy at most.
 *
 * Note: The number of bytes copied is limited by the length of
 *       the attribute payload.
 *
 * @return The number of bytes copied to dest.
 */
int nla_memcpy(void *dest, const struct nlattr *src, int count)
{
	int minlen;

	if (!src)
		return 0;

	minlen = _NL_MIN(count, nla_len(src));

	if (minlen <= 0)
		return 0;

	memcpy(dest, nla_data(src), minlen);
	return minlen;
}

/**
 * Copy string attribute payload to a buffer.
 * @arg dst		Pointer to destination buffer.
 * @arg nla		Attribute of type NLA_STRING.
 * @arg dstsize		Size of destination buffer in bytes.
 *
 * Copies at most dstsize - 1 bytes to the destination buffer.
 * The result is always a valid NUL terminated string. Unlike
 * strlcpy the destination buffer is always padded out.
 *
 * @return The length of string attribute without the terminating NUL.
 */
size_t nla_strlcpy(char *dst, const struct nlattr *nla, size_t dstsize)
{
	size_t srclen = nla_len(nla);
	const char *src = nla_data(nla);

	if (srclen > 0 && src[srclen - 1] == '\0')
		srclen--;

	if (dstsize > 0) {
		size_t len = (srclen >= dstsize) ? dstsize - 1 : srclen;

		memset(dst, 0, dstsize);
		memcpy(dst, src, len);
	}

	return srclen;
}

/**
 * Compare attribute payload with memory area.
 * @arg nla		Attribute.
 * @arg data		Memory area to compare to.
 * @arg size		Number of bytes to compare.
 *
 * @see memcmp(3)
 * @return An integer less than, equal to, or greater than zero.
 */
int nla_memcmp(const struct nlattr *nla, const void *data, size_t size)
{
	int d = nla_len(nla) - size;

	if (d == 0)
		d = memcmp(nla_data(nla), data, size);

	return d;
}

/**
 * Compare string attribute payload with string
 * @arg nla		Attribute of type NLA_STRING.
 * @arg str		NUL terminated string.
 *
 * @see strcmp(3)
 * @return An integer less than, equal to, or greater than zero.
 */
int nla_strcmp(const struct nlattr *nla, const char *str)
{
	int len = strlen(str) + 1;
	int d = nla_len(nla) - len;

	if (d == 0)
		d = memcmp(nla_data(nla), str, len);

	return d;
}

/** @} */

/**
 * @name Unspecific Attribute
 * @{
 */

/**
 * Reserve space for a attribute.
 * @arg msg		Netlink Message.
 * @arg attrtype	Attribute Type.
 * @arg attrlen		Length of payload.
 *
 * Reserves room for a attribute in the specified netlink message and
 * fills in the attribute header (type, length). Returns NULL if there
 * is unsuficient space for the attribute.
 *
 * Any padding between payload and the start of the next attribute is
 * zeroed out.
 *
 * @return Pointer to start of attribute or NULL on failure.
 */
struct nlattr *nla_reserve(struct nl_msg *msg, int attrtype, int attrlen)
{
	struct nlattr *nla;
	size_t tlen;

	if (attrlen < 0)
		return NULL;

	tlen = NLMSG_ALIGN(msg->nm_nlh->nlmsg_len) + nla_total_size(attrlen);

	if (tlen > msg->nm_size || tlen > UINT32_MAX)
		return NULL;

	nla = (struct nlattr *) nlmsg_tail(msg->nm_nlh);
	nla->nla_type = attrtype;
	nla->nla_len = nla_attr_size(attrlen);

	if (attrlen)
		memset((unsigned char *) nla + nla->nla_len, 0, nla_padlen(attrlen));
	msg->nm_nlh->nlmsg_len = tlen;

	NL_DBG(2, "msg %p: attr <%p> %d: Reserved %d (%d) bytes at offset +%td "
		  "nlmsg_len=%d\n", msg, nla, nla->nla_type,
		  nla_total_size(attrlen), attrlen,
		  (char *) nla - (char *) nlmsg_data(msg->nm_nlh),
		  msg->nm_nlh->nlmsg_len);

	return nla;
}

/**
 * Add a unspecific attribute to netlink message.
 * @arg msg		Netlink message.
 * @arg attrtype	Attribute type.
 * @arg datalen		Length of data to be used as payload.
 * @arg data		Pointer to data to be used as attribute payload.
 *
 * Reserves room for a unspecific attribute and copies the provided data
 * into the message as payload of the attribute. Returns an error if there
 * is insufficient space for the attribute.
 *
 * @see nla_reserve
 * @return 0 on success or a negative error code.
 */
int nla_put(struct nl_msg *msg, int attrtype, int datalen, const void *data)
{
	struct nlattr *nla;

	nla = nla_reserve(msg, attrtype, datalen);
	if (!nla) {
		if (datalen < 0)
			return -NLE_INVAL;

		return -NLE_NOMEM;
	}

	if (datalen > 0) {
		memcpy(nla_data(nla), data, datalen);
		NL_DBG(2, "msg %p: attr <%p> %d: Wrote %d bytes at offset +%td\n",
		       msg, nla, nla->nla_type, datalen,
		       (char *) nla - (char *) nlmsg_data(msg->nm_nlh));
	}

	return 0;
}

/**
 * Add abstract data as unspecific attribute to netlink message.
 * @arg msg		Netlink message.
 * @arg attrtype	Attribute type.
 * @arg data		Abstract data object.
 *
 * Equivalent to nla_put() except that the length of the payload is
 * derived from the abstract data object.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_data(struct nl_msg *msg, int attrtype, const struct nl_data *data)
{
	return nla_put(msg, attrtype, nl_data_get_size(data),
		       nl_data_get(data));
}

/**
 * Add abstract address as unspecific attribute to netlink message.
 * @arg msg		Netlink message.
 * @arg attrtype	Attribute type.
 * @arg addr		Abstract address object.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_addr(struct nl_msg *msg, int attrtype, struct nl_addr *addr)
{
	if (nl_addr_get_len(addr) == 0)
		return -NLE_INVAL;

	return nla_put(msg, attrtype, nl_addr_get_len(addr),
		       nl_addr_get_binary_addr(addr));
}

/** @} */

/**
 * @name Integer Attributes
 */

/**
 * Add 8 bit signed integer attribute to netlink message.
 * @arg msg             Netlink message.
 * @arg attrtype        Attribute type.
 * @arg value           Numeric value to store as payload.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_s8(struct nl_msg *msg, int attrtype, int8_t value)
{
	return nla_put(msg, attrtype, sizeof(int8_t), &value);
}

/**
 * Return value of 8 bit signed integer attribute.
 * @arg nla             8 bit integer attribute
 *
 * @return Payload as 8 bit integer.
 */
int8_t nla_get_s8(const struct nlattr *nla)
{
	return *(const int8_t *) nla_data(nla);
}

/**
 * Add 8 bit integer attribute to netlink message.
 * @arg msg		Netlink message.
 * @arg attrtype	Attribute type.
 * @arg value		Numeric value to store as payload.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_u8(struct nl_msg *msg, int attrtype, uint8_t value)
{
	return nla_put(msg, attrtype, sizeof(uint8_t), &value);
}

/**
 * Return value of 8 bit integer attribute.
 * @arg nla		8 bit integer attribute
 *
 * @return Payload as 8 bit integer.
 */
uint8_t nla_get_u8(const struct nlattr *nla)
{
	return *(const uint8_t *) nla_data(nla);
}

/**
 * Add 16 bit signed integer attribute to netlink message.
 * @arg msg             Netlink message.
 * @arg attrtype        Attribute type.
 * @arg value           Numeric value to store as payload.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_s16(struct nl_msg *msg, int attrtype, int16_t value)
{
	return nla_put(msg, attrtype, sizeof(int16_t), &value);
}

/**
 * Return payload of 16 bit signed integer attribute.
 * @arg nla             16 bit integer attribute
 *
 * @return Payload as 16 bit integer.
 */
int16_t nla_get_s16(const struct nlattr *nla)
{
	return *(const int16_t *) nla_data(nla);
}

/**
 * Add 16 bit integer attribute to netlink message.
 * @arg msg		Netlink message.
 * @arg attrtype	Attribute type.
 * @arg value		Numeric value to store as payload.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_u16(struct nl_msg *msg, int attrtype, uint16_t value)
{
	return nla_put(msg, attrtype, sizeof(uint16_t), &value);
}

/**
 * Return payload of 16 bit integer attribute.
 * @arg nla		16 bit integer attribute
 *
 * @return Payload as 16 bit integer.
 */
uint16_t nla_get_u16(const struct nlattr *nla)
{
	return *(const uint16_t *) nla_data(nla);
}

/**
 * Add 32 bit signed integer attribute to netlink message.
 * @arg msg             Netlink message.
 * @arg attrtype        Attribute type.
 * @arg value           Numeric value to store as payload.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_s32(struct nl_msg *msg, int attrtype, int32_t value)
{
	return nla_put(msg, attrtype, sizeof(int32_t), &value);
}

/**
 * Return payload of 32 bit signed integer attribute.
 * @arg nla             32 bit integer attribute.
 *
 * @return Payload as 32 bit integer.
 */
int32_t nla_get_s32(const struct nlattr *nla)
{
	return *(const int32_t *) nla_data(nla);
}

/**
 * Add 32 bit integer attribute to netlink message.
 * @arg msg		Netlink message.
 * @arg attrtype	Attribute type.
 * @arg value		Numeric value to store as payload.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_u32(struct nl_msg *msg, int attrtype, uint32_t value)
{
	return nla_put(msg, attrtype, sizeof(uint32_t), &value);
}

/**
 * Return payload of 32 bit integer attribute.
 * @arg nla		32 bit integer attribute.
 *
 * @return Payload as 32 bit integer.
 */
uint32_t nla_get_u32(const struct nlattr *nla)
{
	return *(const uint32_t *) nla_data(nla);
}

/**
 * Add 64 bit signed integer attribute to netlink message.
 * @arg msg             Netlink message.
 * @arg attrtype        Attribute type.
 * @arg value           Numeric value to store as payload.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_s64(struct nl_msg *msg, int attrtype, int64_t value)
{
	return nla_put(msg, attrtype, sizeof(int64_t), &value);
}

/**
 * Return payload of s64 attribute
 * @arg nla             s64 netlink attribute
 *
 * @return Payload as 64 bit integer.
 */
int64_t nla_get_s64(const struct nlattr *nla)
{
	int64_t tmp = 0;

	if (nla && _nla_len(nla) >= sizeof(tmp))
		memcpy(&tmp, nla_data(nla), sizeof(tmp));

	return tmp;
}

/**
 * Add 64 bit integer attribute to netlink message.
 * @arg msg		Netlink message.
 * @arg attrtype	Attribute type.
 * @arg value		Numeric value to store as payload.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_u64(struct nl_msg *msg, int attrtype, uint64_t value)
{
	return nla_put(msg, attrtype, sizeof(uint64_t), &value);
}

/**
 * Return payload of u64 attribute
 * @arg nla		u64 netlink attribute
 *
 * @return Payload as 64 bit integer.
 */
uint64_t nla_get_u64(const struct nlattr *nla)
{
	uint64_t tmp = 0;

	if (nla && _nla_len(nla) >= sizeof(tmp))
		memcpy(&tmp, nla_data(nla), sizeof(tmp));

	return tmp;
}

/**
 * Add variable-length signed integer attribute to netlink message.
 * @arg msg             Netlink message.
 * @arg attrtype        Attribute type.
 * @arg value           Numeric value to store as payload.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_sint(struct nl_msg *msg, int attrtype, int64_t value)
{
	int64_t tmp64 = value;
	int32_t tmp32 = value;

	if (tmp64 == tmp32)
		return nla_put_s32(msg, attrtype, tmp32);
	return nla_put(msg, attrtype, sizeof(int64_t), &value);
}

/**
 * Return payload of variable-length signed integer attribute
 * @arg nla		variable-length signed integer attribute
 *
 * @return Payload as 64 bit integer.
 */
int64_t nla_get_sint(const struct nlattr *nla)
{
	if (nla && _nla_len(nla) == sizeof(int32_t))
		return nla_get_s32(nla);
	return nla_get_s64(nla);
}

/**
 * Add variable-length unsigned integer attribute to netlink message.
 * @arg msg             Netlink message.
 * @arg attrtype        Attribute type.
 * @arg value           Numeric value to store as payload.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_uint(struct nl_msg *msg, int attrtype, uint64_t value)
{
	uint64_t tmp64 = value;
	uint32_t tmp32 = value;

	if (tmp64 == tmp32)
		return nla_put_u32(msg, attrtype, tmp32);
	return nla_put(msg, attrtype, sizeof(uint64_t), &value);
}

/**
 * Return payload of variable-length unsigned integer attribute
 * @arg nla		variable-length unsigned integer attribute
 *
 * @return Payload as 64 bit integer.
 */
uint64_t nla_get_uint(const struct nlattr *nla)
{
	if (nla && _nla_len(nla) == sizeof(uint32_t))
		return nla_get_u32(nla);
	return nla_get_u64(nla);
}

/** @} */

/**
 * @name String Attribute
 */

/**
 * Add string attribute to netlink message.
 * @arg msg		Netlink message.
 * @arg attrtype	Attribute type.
 * @arg str		NUL terminated string.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_string(struct nl_msg *msg, int attrtype, const char *str)
{
	return nla_put(msg, attrtype, strlen(str) + 1, str);
}

/**
 * Return payload of string attribute.
 * @arg nla		String attribute.
 *
 * @return Pointer to attribute payload.
 */
char *nla_get_string(const struct nlattr *nla)
{
	return (char *) nla_data(nla);
}

char *nla_strdup(const struct nlattr *nla)
{
	return strdup(nla_get_string(nla));
}

/** @} */

/**
 * @name Flag Attribute
 */

/**
 * Add flag netlink attribute to netlink message.
 * @arg msg		Netlink message.
 * @arg attrtype	Attribute type.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_flag(struct nl_msg *msg, int attrtype)
{
	return nla_put(msg, attrtype, 0, NULL);
}

/**
 * Return true if flag attribute is set.
 * @arg nla		Flag netlink attribute.
 *
 * @return True if flag is set, otherwise false.
 */
int nla_get_flag(const struct nlattr *nla)
{
	return !!nla;
}

/** @} */

/**
 * @name Microseconds Attribute
 */

/**
 * Add a msecs netlink attribute to a netlink message
 * @arg n		netlink message
 * @arg attrtype	attribute type
 * @arg msecs 		number of msecs
 */
int nla_put_msecs(struct nl_msg *n, int attrtype, unsigned long msecs)
{
	return nla_put_u64(n, attrtype, msecs);
}

/**
 * Return payload of msecs attribute
 * @arg nla		msecs netlink attribute
 *
 * @return the number of milliseconds.
 */
unsigned long nla_get_msecs(const struct nlattr *nla)
{
	return nla_get_u64(nla);
}

/** @} */

/**
 * @name Nested Attribute
 */

/**
 * Add nested attributes to netlink message.
 * @arg msg		Netlink message.
 * @arg attrtype	Attribute type.
 * @arg nested		Message containing attributes to be nested.
 *
 * Takes the attributes found in the \a nested message and appends them
 * to the message \a msg nested in a container of the type \a attrtype.
 * The \a nested message may not have a family specific header.
 *
 * @see nla_put
 * @return 0 on success or a negative error code.
 */
int nla_put_nested(struct nl_msg *msg, int attrtype,
		   const struct nl_msg *nested)
{
	NL_DBG(2, "msg %p: attr <> %d: adding msg %p as nested attribute\n",
		msg, attrtype, nested);

	return nla_put(msg, attrtype, nlmsg_datalen(nested->nm_nlh),
		       nlmsg_data(nested->nm_nlh));
}


/**
 * Start a new level of nested attributes.
 * @arg msg		Netlink message.
 * @arg attrtype	Attribute type of container.
 *
 * @return Pointer to container attribute.
 */
struct nlattr *nla_nest_start(struct nl_msg *msg, int attrtype)
{
	struct nlattr *start = (struct nlattr *) nlmsg_tail(msg->nm_nlh);

	if (nla_put(msg, NLA_F_NESTED | attrtype, 0, NULL) < 0)
		return NULL;

	NL_DBG(2, "msg %p: attr <%p> %d: starting nesting\n",
		msg, start, start->nla_type);

	return start;
}

static int _nest_end(struct nl_msg *msg, struct nlattr *start, int keep_empty)
{
	size_t pad, len;

	len = (char *) nlmsg_tail(msg->nm_nlh) - (char *) start;

	if (   len > USHRT_MAX
	    || (!keep_empty && len == NLA_HDRLEN)) {
		/*
		 * Max nlattr size exceeded or empty nested attribute, trim the
		 * attribute header again
		 */
		nla_nest_cancel(msg, start);

		/* Return error only if nlattr size was exceeded */
		return (len == NLA_HDRLEN) ? 0 : -NLE_ATTRSIZE;
	}

	start->nla_len = len;

	pad = NLMSG_ALIGN(msg->nm_nlh->nlmsg_len) - msg->nm_nlh->nlmsg_len;
	if (pad > 0) {
		/*
		 * Data inside attribute does not end at a alignment boundry.
		 * Pad accordingly and accoun for the additional space in
		 * the message. nlmsg_reserve() may never fail in this situation,
		 * the allocate message buffer must be a multiple of NLMSG_ALIGNTO.
		 */
		if (!nlmsg_reserve(msg, pad, 0))
			BUG();

		NL_DBG(2, "msg %p: attr <%p> %d: added %zu bytes of padding\n",
			msg, start, start->nla_type, pad);
	}

	NL_DBG(2, "msg %p: attr <%p> %d: closing nesting, len=%u\n",
		msg, start, start->nla_type, start->nla_len);

	return 0;
}

/**
 * Finalize nesting of attributes.
 * @arg msg		Netlink message.
 * @arg start		Container attribute as returned from nla_nest_start().
 *
 * Corrects the container attribute header to include the appeneded attributes.
 *
 * @return 0 on success or a negative error code.
 */
int nla_nest_end(struct nl_msg *msg, struct nlattr *start)
{
	return _nest_end (msg, start, 0);
}

/**
 * Finalize nesting of attributes without stripping off empty attributes.
 * @arg msg		Netlink message.
 * @arg start		Container attribute as returned from nla_nest_start().
 *
 * Corrects the container attribute header to include the appeneded attributes.
 * Keep empty attribute if NO actual attribute payload exists.
 *
 * @return 0 on success or a negative error code.
 */
int nla_nest_end_keep_empty(struct nl_msg *msg, struct nlattr *start)
{
	return _nest_end (msg, start, 1);
}

/**
 * Cancel the addition of a nested attribute
 * @arg msg		Netlink message
 * @arg attr		Nested netlink attribute
 *
 * Removes any partially added nested Netlink attribute from the message
 * by resetting the message to the size before the call to nla_nest_start()
 * and by overwriting any potentially touched message segments with 0.
 */
void nla_nest_cancel(struct nl_msg *msg, const struct nlattr *attr)
{
	ssize_t len;

	if (!attr) {
		/* For robustness, allow a NULL attr to do nothing. NULL is also
		 * what nla_nest_start() when out of buffer space.
		 *
		 * Warning, before libnl-3.8, the function did not accept NULL!
		 * If you care, catch NULL yourself. */
		return;
	}

	len = (char *) nlmsg_tail(msg->nm_nlh) - (char *) attr;
	if (len < 0)
		BUG();
	else if (len > 0) {
		msg->nm_nlh->nlmsg_len -= len;
		memset(nlmsg_tail(msg->nm_nlh), 0, len);
	}
}

/**
 * Create attribute index based on nested attribute
 * @arg tb		Index array to be filled (maxtype+1 elements).
 * @arg maxtype		Maximum attribute type expected and accepted.
 * @arg nla		Nested Attribute.
 * @arg policy		Attribute validation policy.
 *
 * Feeds the stream of attributes nested into the specified attribute
 * to nla_parse().
 *
 * @see nla_parse
 * @return 0 on success or a negative error code.
 */
int nla_parse_nested(struct nlattr *tb[], int maxtype, struct nlattr *nla,
		     const struct nla_policy *policy)
{
	return nla_parse(tb, maxtype, nla_data(nla), nla_len(nla), policy);
}

/**
 * Return true if attribute has NLA_F_NESTED flag set
 * @arg attr		Netlink attribute
 *
 * @return True if attribute has NLA_F_NESTED flag set, oterhwise False.
 */
int nla_is_nested(const struct nlattr *attr)
{
	return !!(attr->nla_type & NLA_F_NESTED);
}

/** @} */

/** @} */
