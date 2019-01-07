/*
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 2001 Gerald Combs
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "config.h"

#include <ftypes-int.h>
#include <epan/to_str-int.h>
#include <string.h>

#include <epan/exceptions.h>

#define CMP_MATCHES cmp_matches

#define tvb_is_private	fvalue_gboolean1

static void
value_new(fvalue_t *fv)
{
	fv->value.protocol.tvb = NULL;
	fv->value.protocol.proto_string = NULL;
	fv->tvb_is_private = FALSE;
}

static void
value_free(fvalue_t *fv)
{
	if (fv->value.protocol.tvb && fv->tvb_is_private) {
		tvb_free_chain(fv->value.protocol.tvb);
	}
	g_free(fv->value.protocol.proto_string);
}

static void
value_set(fvalue_t *fv, tvbuff_t *value, const gchar *name)
{
	/* Free up the old value, if we have one */
	value_free(fv);

	fv->value.protocol.tvb = value;
	fv->value.protocol.proto_string = g_strdup(name);
}

static void
free_tvb_data(void *data)
{
	g_free(data);
}

static gboolean
val_from_string(fvalue_t *fv, const char *s, gchar **err_msg _U_)
{
	tvbuff_t *new_tvb;
	guint8 *private_data;

	/* Free up the old value, if we have one */
	value_free(fv);

	/* Make a tvbuff from the string. We can drop the
	 * terminating NUL. */
	private_data = (guint8 *)g_memdup(s, (guint)strlen(s));
	new_tvb = tvb_new_real_data(private_data,
			(guint)strlen(s), (gint)strlen(s));

	/* Let the tvbuff know how to delete the data. */
	tvb_set_free_cb(new_tvb, free_tvb_data);

	/* And let us know that we need to free the tvbuff */
	fv->tvb_is_private = TRUE;
	fv->value.protocol.tvb = new_tvb;
	fv->value.protocol.proto_string = g_strdup(s);
	return TRUE;
}

static gboolean
val_from_unparsed(fvalue_t *fv, const char *s, gboolean allow_partial_value _U_, gchar **err_msg)
{
	fvalue_t *fv_bytes;
	tvbuff_t *new_tvb;
	guint8 *private_data;

	/* Free up the old value, if we have one */
	value_free(fv);

	/* Does this look like a byte string? */
	fv_bytes = fvalue_from_unparsed(FT_BYTES, s, TRUE, NULL);
	if (fv_bytes) {
		/* Make a tvbuff from the bytes */
		private_data = (guint8 *)g_memdup(fv_bytes->value.bytes->data,
				fv_bytes->value.bytes->len);
		new_tvb = tvb_new_real_data(private_data,
				fv_bytes->value.bytes->len,
				fv_bytes->value.bytes->len);

		/* Let the tvbuff know how to delete the data. */
		tvb_set_free_cb(new_tvb, free_tvb_data);

		/* And let us know that we need to free the tvbuff */
		fv->tvb_is_private = TRUE;
		fv->value.protocol.tvb = new_tvb;
		return TRUE;
	}

	/* Treat it as a string. */
	return val_from_string(fv, s, err_msg);
}

static int
val_repr_len(fvalue_t *fv, ftrepr_t rtype, int field_display _U_)
{
	volatile guint length = 0;

	if (rtype != FTREPR_DFILTER) return -1;

	TRY {
		/* 3 bytes for each byte of the byte "NN:" minus 1 byte
		 * as there's no trailing ":". */
		length = tvb_captured_length(fv->value.protocol.tvb) * 3 - 1;
	}
	CATCH_ALL {
		/* nothing */
	}
	ENDTRY;

	return (int) length;
}

static void
val_to_repr(fvalue_t *fv, ftrepr_t rtype, int field_display _U_, char * volatile buf, unsigned int size _U_)
{
	guint length;

	g_assert(rtype == FTREPR_DFILTER);

	TRY {
		length = tvb_captured_length(fv->value.protocol.tvb);

		if (length)
			buf = bytes_to_hexstr_punct(buf, tvb_get_ptr(fv->value.protocol.tvb, 0, length), length, ':');
		*buf = '\0';
	}
	CATCH_ALL {
		/* nothing */
	}
	ENDTRY;
}

static gpointer
value_get(fvalue_t *fv)
{
	return fv->value.protocol.tvb;
}

static guint
len(fvalue_t *fv)
{
	volatile guint length = 0;

	TRY {
		if (fv->value.protocol.tvb)
			length = tvb_captured_length(fv->value.protocol.tvb);
	}
	CATCH_ALL {
		/* nothing */
	}
	ENDTRY;

	return length;
}

static void
slice(fvalue_t *fv, GByteArray *bytes, guint offset, guint length)
{
	const guint8* data;

	if (fv->value.protocol.tvb) {
		TRY {
			data = tvb_get_ptr(fv->value.protocol.tvb, offset, length);
			g_byte_array_append(bytes, data, length);
		}
		CATCH_ALL {
			/* nothing */
		}
		ENDTRY;

	}
}

static gboolean
cmp_eq(const fvalue_t *fv_a, const fvalue_t *fv_b)
{
	const protocol_value_t	*a = (const protocol_value_t *)&fv_a->value.protocol;
	const protocol_value_t	*b = (const protocol_value_t *)&fv_b->value.protocol;
	volatile gboolean	eq = FALSE;

	TRY {
		if ((a->tvb != NULL) && (b->tvb != NULL)) {
			guint	a_len = tvb_captured_length(a->tvb);

			if (a_len == tvb_captured_length(b->tvb))
				eq = (memcmp(tvb_get_ptr(a->tvb, 0, a_len), tvb_get_ptr(b->tvb, 0, a_len), a_len) == 0);
		} else {
			eq = (strcmp(a->proto_string, b->proto_string) == 0);
		}
	}
	CATCH_ALL {
		/* nothing */
	}
	ENDTRY;

	return eq;
}

static gboolean
cmp_ne(const fvalue_t *fv_a, const fvalue_t *fv_b)
{
	const protocol_value_t	*a = (const protocol_value_t *)&fv_a->value.protocol;
	const protocol_value_t	*b = (const protocol_value_t *)&fv_b->value.protocol;
	volatile gboolean	ne = TRUE;

	TRY {
		if ((a->tvb != NULL) && (b->tvb != NULL)) {
			guint	a_len = tvb_captured_length(a->tvb);

			if (a_len == tvb_captured_length(b->tvb))
				ne = (memcmp(tvb_get_ptr(a->tvb, 0, a_len), tvb_get_ptr(b->tvb, 0, a_len), a_len) != 0);
		} else {
			ne = (strcmp(a->proto_string, b->proto_string) != 0);
		}
	}
	CATCH_ALL {
		/* nothing */
	}
	ENDTRY;

	return ne;
}

static gboolean
cmp_gt(const fvalue_t *fv_a, const fvalue_t *fv_b)
{
	const protocol_value_t	*a = (const protocol_value_t *)&fv_a->value.protocol;
	const protocol_value_t	*b = (const protocol_value_t *)&fv_b->value.protocol;
	volatile gboolean	gt = FALSE;

	TRY {
		if ((a->tvb != NULL) && (b->tvb != NULL)) {
			guint	a_len = tvb_captured_length(a->tvb);
			guint	b_len = tvb_captured_length(b->tvb);

			if (a_len > b_len) {
				gt = TRUE;
			} else if (a_len == b_len) {
				gt = (memcmp(tvb_get_ptr(a->tvb, 0, a_len), tvb_get_ptr(b->tvb, 0, a_len), a_len) > 0);
			}
		} else {
			return (strcmp(a->proto_string, b->proto_string) > 0);
		}
	}
	CATCH_ALL {
		/* nothing */
	}
	ENDTRY;

	return gt;
}

static gboolean
cmp_ge(const fvalue_t *fv_a, const fvalue_t *fv_b)
{
	const protocol_value_t	*a = (const protocol_value_t *)&fv_a->value.protocol;
	const protocol_value_t	*b = (const protocol_value_t *)&fv_b->value.protocol;
	volatile gboolean	ge = FALSE;

	TRY {
		if ((a->tvb != NULL) && (b->tvb != NULL)) {
			guint	a_len = tvb_captured_length(a->tvb);
			guint	b_len = tvb_captured_length(b->tvb);

			if (a_len > b_len) {
				ge = TRUE;
			} else if (a_len == b_len) {
				ge = (memcmp(tvb_get_ptr(a->tvb, 0, a_len), tvb_get_ptr(b->tvb, 0, a_len), a_len) >= 0);
			}
		} else {
			return (strcmp(a->proto_string, b->proto_string) >= 0);
		}
	}
	CATCH_ALL {
		/* nothing */
	}
	ENDTRY;

	return ge;
}

static gboolean
cmp_lt(const fvalue_t *fv_a, const fvalue_t *fv_b)
{
	const protocol_value_t	*a = (const protocol_value_t *)&fv_a->value.protocol;
	const protocol_value_t	*b = (const protocol_value_t *)&fv_b->value.protocol;
	volatile gboolean	lt = FALSE;

	TRY {
		if ((a->tvb != NULL) && (b->tvb != NULL)) {
			guint	a_len = tvb_captured_length(a->tvb);
			guint	b_len = tvb_captured_length(b->tvb);

			if (a_len < b_len) {
				lt = TRUE;
			} else if (a_len == b_len) {
				lt = (memcmp(tvb_get_ptr(a->tvb, 0, a_len), tvb_get_ptr(b->tvb, 0, a_len), a_len) < 0);
			}
		} else {
			return (strcmp(a->proto_string, b->proto_string) < 0);
		}
	}
	CATCH_ALL {
		/* nothing */
	}
	ENDTRY;

	return lt;
}

static gboolean
cmp_le(const fvalue_t *fv_a, const fvalue_t *fv_b)
{
	const protocol_value_t	*a = (const protocol_value_t *)&fv_a->value.protocol;
	const protocol_value_t	*b = (const protocol_value_t *)&fv_b->value.protocol;
	volatile gboolean	le = FALSE;

	TRY {
		if ((a->tvb != NULL) && (b->tvb != NULL)) {
			guint	a_len = tvb_captured_length(a->tvb);
			guint	b_len = tvb_captured_length(b->tvb);

			if (a_len < b_len) {
				le = TRUE;
			} else if (a_len == b_len) {
				le = (memcmp(tvb_get_ptr(a->tvb, 0, a_len), tvb_get_ptr(b->tvb, 0, a_len), a_len) <= 0);
			}
		} else {
			return (strcmp(a->proto_string, b->proto_string) <= 0);
		}
	}
	CATCH_ALL {
		/* nothing */
	}
	ENDTRY;

	return le;
}

static gboolean
cmp_contains(const fvalue_t *fv_a, const fvalue_t *fv_b)
{
	volatile gboolean contains = FALSE;

	TRY {
		/* First see if tvb exists for both sides */
		if ((fv_a->value.protocol.tvb != NULL) && (fv_b->value.protocol.tvb != NULL)) {
			if (tvb_find_tvb(fv_a->value.protocol.tvb, fv_b->value.protocol.tvb, 0) > -1) {
				contains = TRUE;
			}
		} else {
			/* Otherwise just compare strings */
			if ((strlen(fv_b->value.protocol.proto_string) != 0) &&
				strstr(fv_a->value.protocol.proto_string, fv_b->value.protocol.proto_string)) {
				contains = TRUE;
			}
		}
	}
	CATCH_ALL {
		/* nothing */
	}
	ENDTRY;

	return contains;
}

static gboolean
cmp_matches(const fvalue_t *fv_a, const fvalue_t *fv_b)
{
	const protocol_value_t *a = (const protocol_value_t *)&fv_a->value.protocol;
	GRegex *regex = fv_b->value.re;
	volatile gboolean rc = FALSE;
	const char *data = NULL; /* tvb data */
	guint32 tvb_len; /* tvb length */

	/* fv_b is always a FT_PCRE, otherwise the dfilter semcheck() would have
	 * warned us. For the same reason (and because we're using g_malloc()),
	 * fv_b->value.re is not NULL.
	 */
	if (strcmp(fv_b->ftype->name, "FT_PCRE") != 0) {
		return FALSE;
	}
	if (! regex) {
		return FALSE;
	}
	TRY {
		if (a->tvb != NULL) {
			tvb_len = tvb_captured_length(a->tvb);
			data = (const char *)tvb_get_ptr(a->tvb, 0, tvb_len);
			rc = g_regex_match_full(
				regex,		/* Compiled PCRE */
				data,		/* The data to check for the pattern... */
				tvb_len,	/* ... and its length */
				0,		/* Start offset within data */
				(GRegexMatchFlags)0,		/* GRegexMatchFlags */
				NULL,		/* We are not interested in the match information */
				NULL		/* We don't want error information */
				);
			/* NOTE - DO NOT g_free(data) */
		} else {
			rc = g_regex_match_full(
			regex,		/* Compiled PCRE */
			a->proto_string,		/* The data to check for the pattern... */
			(int)strlen(a->proto_string),	/* ... and its length */
			0,		/* Start offset within data */
			(GRegexMatchFlags)0,		/* GRegexMatchFlags */
			NULL,		/* We are not interested in the match information */
			NULL		/* We don't want error information */
			);
		}
	}
	CATCH_ALL {
		return FALSE;
	}
	ENDTRY;
	return rc;
}

void
ftype_register_tvbuff(void)
{

	static ftype_t protocol_type = {
		FT_PROTOCOL,			/* ftype */
		"FT_PROTOCOL",			/* name */
		"Protocol",			/* pretty_name */
		0,				/* wire_size */
		value_new,			/* new_value */
		value_free,			/* free_value */
		val_from_unparsed,		/* val_from_unparsed */
		val_from_string,		/* val_from_string */
		val_to_repr,			/* val_to_string_repr */
		val_repr_len,			/* len_string_repr */

		NULL,				/* set_value_byte_array */
		NULL,				/* set_value_bytes */
		NULL,				/* set_value_guid */
		NULL,				/* set_value_time */
		NULL,				/* set_value_string */
		value_set,			/* set_value_protocol */
		NULL,				/* set_value_uinteger */
		NULL,				/* set_value_sinteger */
		NULL,				/* set_value_uinteger64 */
		NULL,				/* set_value_sinteger64 */
		NULL,				/* set_value_floating */

		value_get,			/* get_value */
		NULL,				/* get_value_uinteger */
		NULL,				/* get_value_sinteger */
		NULL,				/* get_value_uinteger64 */
		NULL,				/* get_value_sinteger64 */
		NULL,				/* get_value_floating */

		cmp_eq,
		cmp_ne,
		cmp_gt,
		cmp_ge,
		cmp_lt,
		cmp_le,
		NULL,				/* cmp_bitwise_and */
		cmp_contains,
		CMP_MATCHES,

		len,
		slice,

	};


	ftype_register(FT_PROTOCOL, &protocol_type);
}

/*
 * Editor modelines  -  http://www.wireshark.org/tools/modelines.html
 *
 * Local variables:
 * c-basic-offset: 8
 * tab-width: 8
 * indent-tabs-mode: t
 * End:
 *
 * vi: set shiftwidth=8 tabstop=8 noexpandtab:
 * :indentSize=8:tabSize=8:noTabs=false:
 */
