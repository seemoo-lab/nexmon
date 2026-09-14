/*
 * Copyright (c) 2021 Igalia S.L.
 *
 * SPDX-License-Identifier: LGPL-2.1-or-later
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General
 * Public License along with this library; if not, see <http://www.gnu.org/licenses/>.
 *
 * Authors: Patrick Griffis <pgriffis@igalia.com>
 */

#include "config.h"

#include <glib.h>
#include <gio/gnetworking.h>

#define GIO_COMPILATION
#include "gthreadedresolver.h"
#include "gthreadedresolver-private.h"
#undef GIO_COMPILATION

#ifdef HAVE_DN_COMP
static void
dns_builder_add_uint8 (GByteArray *builder,
                       guint8      value)
{
  g_byte_array_append (builder, &value, 1);
}

static void
dns_builder_add_uint16 (GByteArray *builder,
                        guint16     value)
{
    dns_builder_add_uint8 (builder, (value >> 8)  & 0xFF);
    dns_builder_add_uint8 (builder, (value)       & 0xFF);
}

static void
dns_builder_add_uint32 (GByteArray *builder,
                        guint32     value)
{
    dns_builder_add_uint8 (builder, (value >> 24) & 0xFF);
    dns_builder_add_uint8 (builder, (value >> 16) & 0xFF);
    dns_builder_add_uint8 (builder, (value >> 8)  & 0xFF);
    dns_builder_add_uint8 (builder, (value)       & 0xFF);
}

static void
dns_builder_add_length_prefixed_string (GByteArray *builder,
                                        const char *string)
{
    guint8 length;

    g_assert (strlen (string) <= G_MAXUINT8);

    length = (guint8) strlen (string);
    dns_builder_add_uint8 (builder, length);

    /* Don't include trailing NUL */
    g_byte_array_append (builder, (const guchar *)string, length);
}

static void
dns_builder_add_domain (GByteArray *builder,
                        const char *string)
{
  int ret;
  guchar buffer[256];

  ret = dn_comp (string, buffer, sizeof (buffer), NULL, NULL);
  g_assert (ret != -1);

  g_byte_array_append (builder, buffer, ret);
}

/* Append an invalid domain name to the DNS response. This is implemented by
 * appending a single label followed by a pointer back to that label. This is
 * invalid regardless of any other context in the response as its expansion is
 * infinite.
 *
 * See https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.4
 *
 * In order to create a pointer to the label, the label’s final offset in the
 * DNS response must be known. The current length of @builder, plus @offset, is
 * used for this. Hence, @offset is the additional offset (in bytes) to add, and
 * typically corresponds to the length of the parent #GByteArray that @builder
 * will eventually be added to. Potentially plus 2 bytes for the rdlength, as
 * per dns_builder_add_answer_data(). */
static void
dns_builder_add_invalid_domain (GByteArray *builder,
                                gsize       offset)
{
  offset += builder->len;
  g_assert ((offset & 0xc0) == 0);

  dns_builder_add_uint8 (builder, 1);
  dns_builder_add_uint8 (builder, 'f');
  dns_builder_add_uint8 (builder, 0xc0 | offset);
}

static void
dns_builder_add_answer_data (GByteArray *builder,
                             GByteArray *answer)
{
  dns_builder_add_uint16 (builder, answer->len); /* rdlength */
  g_byte_array_append (builder, answer->data, answer->len);
}

static GByteArray *
dns_header (void)
{
  GByteArray *answer = g_byte_array_sized_new (2046);

  /* Start with a header, we ignore everything except ancount.
     https://datatracker.ietf.org/doc/html/rfc1035#section-4.1.1 */
  dns_builder_add_uint16 (answer, 0); /* ID */
  dns_builder_add_uint16 (answer, 0); /* |QR|   Opcode  |AA|TC|RD|RA|   Z    |   RCODE   | */
  dns_builder_add_uint16 (answer, 0); /* QDCOUNT */
  dns_builder_add_uint16 (answer, 1); /* ANCOUNT (1 answer) */
  dns_builder_add_uint16 (answer, 0); /* NSCOUNT */
  dns_builder_add_uint16 (answer, 0); /* ARCOUNT */

  return g_steal_pointer (&answer);
}

static void
assert_query_fails (const gchar         *rrname,
                    GResolverRecordType  record_type,
                    GByteArray          *answer)
{
  GList *records = NULL;
  GError *local_error = NULL;

  records = g_resolver_records_from_res_query (rrname,
                                               g_resolver_record_type_to_rrtype (record_type),
                                               answer->data,
                                               answer->len,
                                               0,
                                               &local_error);

  g_assert_error (local_error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_INTERNAL);
  g_assert_null (records);
  g_clear_error (&local_error);
}

static void
assert_query_succeeds (const gchar         *rrname,
                       GResolverRecordType  record_type,
                       GByteArray          *answer,
                       const gchar         *expected_answer_variant_str)
{
  GList *records = NULL;
  GVariant *answer_variant, *expected_answer_variant = NULL;
  GError *local_error = NULL;

  records = g_resolver_records_from_res_query (rrname,
                                               g_resolver_record_type_to_rrtype (record_type),
                                               answer->data,
                                               answer->len,
                                               0,
                                               &local_error);

  g_assert_no_error (local_error);
  g_assert_nonnull (records);

  /* Test the results. */
  answer_variant = records->data;
  expected_answer_variant = g_variant_new_parsed (expected_answer_variant_str);
  g_assert_cmpvariant (answer_variant, expected_answer_variant);
  g_assert_true (!g_variant_is_floating (answer_variant));

  g_variant_unref (expected_answer_variant);
  g_list_free_full (records, (GDestroyNotify) g_variant_unref);
}
#endif /* HAVE_DN_COMP */

static void
test_invalid_header (void)
{
  const struct
    {
      const guint8 *answer;
      gsize answer_len;
      GResolverError expected_error_code;
    }
  vectors[] =
    {
      /* No answer: */
      { (const guint8 *) "", 0, G_RESOLVER_ERROR_NOT_FOUND },
      /* Definitely too short to be a valid header: */
      { (const guint8 *) "\x20", 1, G_RESOLVER_ERROR_INTERNAL },
      /* One byte too short to be a valid header: */
      { (const guint8 *) "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 11, G_RESOLVER_ERROR_INTERNAL },
      /* Valid header indicating no answers: */
      { (const guint8 *) "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 12, G_RESOLVER_ERROR_NOT_FOUND },
    };
  gsize i;

  for (i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      GList *records = NULL;
      GError *local_error = NULL;

      records = g_resolver_records_from_res_query ("example.org",
                                                   g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_NS),
                                                   vectors[i].answer,
                                                   vectors[i].answer_len,
                                                   0,
                                                   &local_error);

      g_assert_error (local_error, G_RESOLVER_ERROR, (gint) vectors[i].expected_error_code);
      g_assert_null (records);
      g_clear_error (&local_error);
    }
}

static void
test_record_ownership (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *txt_rdata = NULL;
  GList *records = NULL;
  GError *local_error = NULL;
  uint16_t rrtype;

  rrtype = g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_TXT);
  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, rrtype);
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* TXT rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.14 */
  txt_rdata = g_byte_array_new ();
  dns_builder_add_length_prefixed_string (txt_rdata, "some test content");
  dns_builder_add_answer_data (answer, txt_rdata);
  g_byte_array_unref (txt_rdata);

  records = g_resolver_records_from_res_query ("example.org",
                                               rrtype,
                                               answer->data,
                                               answer->len,
                                               0,
                                               &local_error);
  g_assert_no_error (local_error);
  g_assert_nonnull (records);

  for (const GList *iter = records; iter != NULL; iter = iter->next)
    g_assert_true (!g_variant_is_floating (records->data));

  g_list_free_full (records, (GDestroyNotify) g_variant_unref);
  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_unknown_record_type (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL;
  GList *records = NULL;
  GError *local_error = NULL;
  const guint type_id = 20;  /* ISDN, not supported anywhere */

  /* An answer with an unsupported type chosen from
   * https://en.wikipedia.org/wiki/List_of_DNS_record_types#[1]_Obsolete_record_types */
  answer = dns_header ();
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, type_id);
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */
  dns_builder_add_uint16 (answer, 0); /* rdlength */

  records = g_resolver_records_from_res_query ("example.org",
                                               type_id,
                                               answer->data,
                                               answer->len,
                                               0,
                                               &local_error);

  g_assert_error (local_error, G_RESOLVER_ERROR, G_RESOLVER_ERROR_NOT_FOUND);
  g_assert_null (records);
  g_clear_error (&local_error);

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_mx_valid (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *mx_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_MX));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* MX rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.9 */
  mx_rdata = g_byte_array_new ();
  dns_builder_add_uint16 (mx_rdata, 0);  /* preference */
  dns_builder_add_domain (mx_rdata, "mail.example.org");
  dns_builder_add_answer_data (answer, mx_rdata);
  g_byte_array_unref (mx_rdata);

  assert_query_succeeds ("example.org", G_RESOLVER_RECORD_MX, answer,
                         "(@q 0, 'mail.example.org')");

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_mx_invalid (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *mx_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_MX));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* MX rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.9
   *
   * Use an invalid domain to trigger parsing failure. */
  mx_rdata = g_byte_array_new ();
  dns_builder_add_uint16 (mx_rdata, 0);  /* preference */
  dns_builder_add_invalid_domain (mx_rdata, answer->len + 2);
  dns_builder_add_answer_data (answer, mx_rdata);
  g_byte_array_unref (mx_rdata);

  assert_query_fails ("example.org", G_RESOLVER_RECORD_MX, answer);

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_mx_invalid_too_short (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *mx_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_MX));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* MX rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.9
   *
   * Miss out the domain field to trigger failure */
  mx_rdata = g_byte_array_new ();
  dns_builder_add_uint16 (mx_rdata, 0);  /* preference */
  /* missing domain field */
  dns_builder_add_answer_data (answer, mx_rdata);
  g_byte_array_unref (mx_rdata);

  assert_query_fails ("example.org", G_RESOLVER_RECORD_MX, answer);

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_mx_invalid_too_short2 (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *mx_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_MX));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* MX rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.9
   *
   * Miss out all fields to trigger failure */
  mx_rdata = g_byte_array_new ();
  /* missing preference and domain fields */
  dns_builder_add_answer_data (answer, mx_rdata);
  g_byte_array_unref (mx_rdata);

  assert_query_fails ("example.org", G_RESOLVER_RECORD_MX, answer);

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_ns_valid (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *ns_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_NS));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* NS rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.11 */
  ns_rdata = g_byte_array_new ();
  dns_builder_add_domain (ns_rdata, "ns.example.org");
  dns_builder_add_answer_data (answer, ns_rdata);
  g_byte_array_unref (ns_rdata);

  assert_query_succeeds ("example.org", G_RESOLVER_RECORD_NS, answer,
                         "('ns.example.org',)");

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_ns_invalid (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *ns_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_NS));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* NS rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.11
   *
   * Use an invalid domain to trigger parsing failure. */
  ns_rdata = g_byte_array_new ();
  dns_builder_add_invalid_domain (ns_rdata, answer->len + 2);
  dns_builder_add_answer_data (answer, ns_rdata);
  g_byte_array_unref (ns_rdata);

  assert_query_fails ("example.org", G_RESOLVER_RECORD_NS, answer);

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_soa_valid (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *soa_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_SOA));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* SOA rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.13 */
  soa_rdata = g_byte_array_new ();
  dns_builder_add_domain (soa_rdata, "mname.example.org");
  dns_builder_add_domain (soa_rdata, "rname.example.org");
  dns_builder_add_uint32 (soa_rdata, 0);  /* serial */
  dns_builder_add_uint32 (soa_rdata, 0);  /* refresh */
  dns_builder_add_uint32 (soa_rdata, 0);  /* retry */
  dns_builder_add_uint32 (soa_rdata, 0);  /* expire */
  dns_builder_add_uint32 (soa_rdata, 0);  /* minimum */
  dns_builder_add_answer_data (answer, soa_rdata);
  g_byte_array_unref (soa_rdata);

  assert_query_succeeds ("example.org", G_RESOLVER_RECORD_SOA, answer,
                         "('mname.example.org', 'rname.example.org', @u 0, @u 0, @u 0, @u 0, @u 0)");

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_soa_invalid_mname (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *soa_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_SOA));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* SOA rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.13
   *
   * Use an invalid domain to trigger parsing failure. */
  soa_rdata = g_byte_array_new ();
  dns_builder_add_invalid_domain (soa_rdata, answer->len + 2);  /* mname */
  dns_builder_add_domain (soa_rdata, "rname.example.org");
  dns_builder_add_uint32 (soa_rdata, 0);  /* serial */
  dns_builder_add_uint32 (soa_rdata, 0);  /* refresh */
  dns_builder_add_uint32 (soa_rdata, 0);  /* retry */
  dns_builder_add_uint32 (soa_rdata, 0);  /* expire */
  dns_builder_add_uint32 (soa_rdata, 0);  /* minimum */
  dns_builder_add_answer_data (answer, soa_rdata);
  g_byte_array_unref (soa_rdata);

  assert_query_fails ("example.org", G_RESOLVER_RECORD_SOA, answer);

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_soa_invalid_rname (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *soa_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_SOA));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* SOA rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.13
   *
   * Use an invalid domain to trigger parsing failure. */
  soa_rdata = g_byte_array_new ();
  dns_builder_add_domain (soa_rdata, "mname.example.org");
  dns_builder_add_invalid_domain (soa_rdata, answer->len + 2);  /* rname */
  dns_builder_add_uint32 (soa_rdata, 0);  /* serial */
  dns_builder_add_uint32 (soa_rdata, 0);  /* refresh */
  dns_builder_add_uint32 (soa_rdata, 0);  /* retry */
  dns_builder_add_uint32 (soa_rdata, 0);  /* expire */
  dns_builder_add_uint32 (soa_rdata, 0);  /* minimum */
  dns_builder_add_answer_data (answer, soa_rdata);
  g_byte_array_unref (soa_rdata);

  assert_query_fails ("example.org", G_RESOLVER_RECORD_SOA, answer);

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_soa_invalid_too_short (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *soa_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_SOA));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* SOA rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.13
   *
   * Miss out one of the fields to trigger a failure. */
  soa_rdata = g_byte_array_new ();
  dns_builder_add_domain (soa_rdata, "mname.example.org");
  dns_builder_add_domain (soa_rdata, "rname.example.org");
  dns_builder_add_uint32 (soa_rdata, 0);  /* serial */
  dns_builder_add_uint32 (soa_rdata, 0);  /* refresh */
  dns_builder_add_uint32 (soa_rdata, 0);  /* retry */
  dns_builder_add_uint32 (soa_rdata, 0);  /* expire */
  /* missing minimum field */
  dns_builder_add_answer_data (answer, soa_rdata);
  g_byte_array_unref (soa_rdata);

  assert_query_fails ("example.org", G_RESOLVER_RECORD_SOA, answer);

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_txt_valid (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *txt_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_TXT));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* TXT rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.14 */
  txt_rdata = g_byte_array_new ();
  dns_builder_add_length_prefixed_string (txt_rdata, "some test content");
  dns_builder_add_answer_data (answer, txt_rdata);
  g_byte_array_unref (txt_rdata);

  assert_query_succeeds ("example.org", G_RESOLVER_RECORD_TXT, answer,
                         "(['some test content'],)");

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_txt_valid_multiple_strings (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *txt_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_TXT));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* TXT rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.14 */
  txt_rdata = g_byte_array_new ();
  dns_builder_add_length_prefixed_string (txt_rdata, "some test content");
  dns_builder_add_length_prefixed_string (txt_rdata, "more test content");
  dns_builder_add_answer_data (answer, txt_rdata);
  g_byte_array_unref (txt_rdata);

  assert_query_succeeds ("example.org", G_RESOLVER_RECORD_TXT, answer,
                         "(['some test content', 'more test content'],)");

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_txt_invalid_empty (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *txt_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_TXT));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* TXT rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.14
   *
   * Provide zero character strings (i.e. an empty rdata section) to trigger
   * failure. */
  txt_rdata = g_byte_array_new ();
  dns_builder_add_answer_data (answer, txt_rdata);
  g_byte_array_unref (txt_rdata);

  assert_query_fails ("example.org", G_RESOLVER_RECORD_TXT, answer);

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_txt_invalid_overflow (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *txt_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_TXT));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* TXT rdata, https://datatracker.ietf.org/doc/html/rfc1035#section-3.3.14
   *
   * Use a character string whose length exceeds the remaining length in the
   * answer record, to trigger failure. */
  txt_rdata = g_byte_array_new ();
  dns_builder_add_uint8 (txt_rdata, 10);  /* length, but no content */
  dns_builder_add_answer_data (answer, txt_rdata);
  g_byte_array_unref (txt_rdata);

  assert_query_fails ("example.org", G_RESOLVER_RECORD_TXT, answer);

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_srv_valid (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *srv_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_SRV));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* SRV rdata, https://datatracker.ietf.org/doc/html/rfc2782 */
  srv_rdata = g_byte_array_new ();
  dns_builder_add_uint16 (srv_rdata, 0);  /* priority */
  dns_builder_add_uint16 (srv_rdata, 0);  /* weight */
  dns_builder_add_uint16 (srv_rdata, 0);  /* port */
  dns_builder_add_domain (srv_rdata, "target.example.org");
  dns_builder_add_answer_data (answer, srv_rdata);
  g_byte_array_unref (srv_rdata);

  assert_query_succeeds ("example.org", G_RESOLVER_RECORD_SRV, answer,
                         "(@q 0, @q 0, @q 0, 'target.example.org')");

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_srv_invalid (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *srv_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_SRV));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* SRV rdata, https://datatracker.ietf.org/doc/html/rfc2782
   *
   * Use an invalid domain to trigger parsing failure. */
  srv_rdata = g_byte_array_new ();
  dns_builder_add_uint16 (srv_rdata, 0);  /* priority */
  dns_builder_add_uint16 (srv_rdata, 0);  /* weight */
  dns_builder_add_uint16 (srv_rdata, 0);  /* port */
  dns_builder_add_invalid_domain (srv_rdata, answer->len + 2);
  dns_builder_add_answer_data (answer, srv_rdata);
  g_byte_array_unref (srv_rdata);

  assert_query_fails ("example.org", G_RESOLVER_RECORD_SRV, answer);

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_srv_invalid_too_short (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *srv_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_SRV));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* SRV rdata, https://datatracker.ietf.org/doc/html/rfc2782
   *
   * Miss out the target field to trigger failure */
  srv_rdata = g_byte_array_new ();
  dns_builder_add_uint16 (srv_rdata, 0);  /* priority */
  dns_builder_add_uint16 (srv_rdata, 0);  /* weight */
  dns_builder_add_uint16 (srv_rdata, 0);  /* port */
  /* missing target field */
  dns_builder_add_answer_data (answer, srv_rdata);
  g_byte_array_unref (srv_rdata);

  assert_query_fails ("example.org", G_RESOLVER_RECORD_SRV, answer);

  g_byte_array_free (answer, TRUE);
#endif
}

static void
test_srv_invalid_too_short2 (void)
{
#ifndef HAVE_DN_COMP
  g_test_skip ("The dn_comp() function was not available.");
  return;
#else
  GByteArray *answer = NULL, *srv_rdata = NULL;

  answer = dns_header ();

  /* Resource record */
  dns_builder_add_domain (answer, "example.org");
  dns_builder_add_uint16 (answer, g_resolver_record_type_to_rrtype (G_RESOLVER_RECORD_SRV));
  dns_builder_add_uint16 (answer, 1); /* qclass=C_IN */
  dns_builder_add_uint32 (answer, 0); /* ttl (ignored) */

  /* SRV rdata, https://datatracker.ietf.org/doc/html/rfc2782
   *
   * Miss out the target and port fields to trigger failure */
  srv_rdata = g_byte_array_new ();
  dns_builder_add_uint16 (srv_rdata, 0);  /* priority */
  dns_builder_add_uint16 (srv_rdata, 0);  /* weight */
  /* missing port and target fields */
  dns_builder_add_answer_data (answer, srv_rdata);
  g_byte_array_unref (srv_rdata);

  assert_query_fails ("example.org", G_RESOLVER_RECORD_SRV, answer);

  g_byte_array_free (answer, TRUE);
#endif
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_add_func ("/gresolver/invalid-header", test_invalid_header);
  g_test_add_func ("/gresolver/record-ownership", test_record_ownership);
  g_test_add_func ("/gresolver/unknown-record-type", test_unknown_record_type);
  g_test_add_func ("/gresolver/mx/valid", test_mx_valid);
  g_test_add_func ("/gresolver/mx/invalid", test_mx_invalid);
  g_test_add_func ("/gresolver/mx/invalid/too-short", test_mx_invalid_too_short);
  g_test_add_func ("/gresolver/mx/invalid/too-short2", test_mx_invalid_too_short2);
  g_test_add_func ("/gresolver/ns/valid", test_ns_valid);
  g_test_add_func ("/gresolver/ns/invalid", test_ns_invalid);
  g_test_add_func ("/gresolver/soa/valid", test_soa_valid);
  g_test_add_func ("/gresolver/soa/invalid/mname", test_soa_invalid_mname);
  g_test_add_func ("/gresolver/soa/invalid/rname", test_soa_invalid_rname);
  g_test_add_func ("/gresolver/soa/invalid/too-short", test_soa_invalid_too_short);
  g_test_add_func ("/gresolver/srv/valid", test_srv_valid);
  g_test_add_func ("/gresolver/srv/invalid", test_srv_invalid);
  g_test_add_func ("/gresolver/srv/invalid/too-short", test_srv_invalid_too_short);
  g_test_add_func ("/gresolver/srv/invalid/too-short2", test_srv_invalid_too_short2);
  g_test_add_func ("/gresolver/txt/valid", test_txt_valid);
  g_test_add_func ("/gresolver/txt/valid/multiple-strings", test_txt_valid_multiple_strings);
  g_test_add_func ("/gresolver/txt/invalid/empty", test_txt_invalid_empty);
  g_test_add_func ("/gresolver/txt/invalid/overflow", test_txt_invalid_overflow);

  return g_test_run ();
}
