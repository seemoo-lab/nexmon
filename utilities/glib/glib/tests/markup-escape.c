#undef G_DISABLE_ASSERT
#undef G_LOG_DOMAIN

#include <stdarg.h>
#include <string.h>
#include <glib.h>

typedef struct _EscapeTest EscapeTest;

struct _EscapeTest
{
  const gchar *original;
  const gchar *expected;
  gboolean expected_markup_parse_error;  /* we expect this iff the original is not valid UTF-8 */
  GMarkupError expected_markup_parse_error_code;
};

static EscapeTest escape_tests[] =
{
  { "&", "&amp;", FALSE, 0 },
  { "<", "&lt;", FALSE, 0 },
  { ">", "&gt;", FALSE, 0 },
  { "'", "&apos;", FALSE, 0 },
  { "\"", "&quot;", FALSE, 0 },
  { "\"\"", "&quot;&quot;", FALSE, 0 },
  { "\"അ\"", "&quot;അ&quot;", FALSE, 0 },
  { "", "", FALSE, 0 },
  { "A", "A", FALSE, 0 },
  { "A&", "A&amp;", FALSE, 0 },
  { "&A", "&amp;A", FALSE, 0 },
  { "A&A", "A&amp;A", FALSE, 0 },
  { "&&A", "&amp;&amp;A", FALSE, 0 },
  { "A&&", "A&amp;&amp;", FALSE, 0 },
  { "A&&A", "A&amp;&amp;A", FALSE, 0 },
  { "A&A&A", "A&amp;A&amp;A", FALSE, 0 },
  { "A&#23;A", "A&amp;#23;A", FALSE, 0 },
  { "A&#xa;A", "A&amp;#xa;A", FALSE, 0 },
  { "N\x2N", "N&#x2;N", FALSE, 0 },
  { "N\xc2\x80N", "N&#x80;N", FALSE, 0 },
  { "N\xc2\x79N", "N\xc2\x79N", TRUE, G_MARKUP_ERROR_BAD_UTF8 },
  { "N\xc2\x9fN", "N&#x9f;N", FALSE, 0 },
  { "\xc2", "\xc2", TRUE, G_MARKUP_ERROR_BAD_UTF8 },

  /* As per g_markup_escape_text()'s documentation, whitespace is not escaped: */
  { "\t", "\t", FALSE, 0 },
};

static void
start_element_unreachable_cb (GMarkupParseContext  *context,
                              const char           *element_name,
                              const char          **attribute_names,
                              const char          **attribute_values,
                              void                 *user_data,
                              GError              **error)
{
  if (g_strcmp0 (element_name, "wrapper") == 0)
    return;

  g_assert_not_reached ();
}

static void
end_element_unreachable_cb (GMarkupParseContext  *context,
                            const char           *element_name,
                            void                 *user_data,
                            GError              **error)
{
  if (g_strcmp0 (element_name, "wrapper") == 0)
    return;

  g_assert_not_reached ();
}

static void
escape_text_cb (GMarkupParseContext  *context,
                const char           *text,
                size_t                text_len,
                void                 *user_data,
                GError              **error)
{
  char **parsed_text_inout = user_data;

  if (*parsed_text_inout == NULL)
    *parsed_text_inout = g_strdup (text);
  else
    g_set_str_take (parsed_text_inout, g_strconcat (*parsed_text_inout, text, NULL));
}

static void
passthrough_unreachable_cb (GMarkupParseContext  *context,
                            const char           *passthrough_text,
                            size_t                text_len,
                            void                 *user_data,
                            GError              **error)
{
  g_assert_not_reached ();
}

static void
escape_test (gconstpointer d)
{
  const EscapeTest *test = d;
  char *non_nul_terminated_original = NULL;
  size_t non_nul_terminated_original_len = 0;
  char *result = NULL, *result2 = NULL;
  GMarkupParseContext *context;
  GError *local_error = NULL;
  const GMarkupParser parser = {
    .start_element = start_element_unreachable_cb,
    .end_element = end_element_unreachable_cb,
    .text = escape_text_cb,
    .passthrough = passthrough_unreachable_cb,
    .error = NULL,  /* handle these via g_markup_parse_context_parse() */
  };
  char *parsed_text = NULL, *wrapped_result = NULL;

  /* Try once nul-terminated */
  result = g_markup_escape_text (test->original, -1);
  g_assert_cmpstr (result, ==, test->expected);

  /* And try again with a newly allocated original without a nul-terminator,
   * and using a fixed length. This can help catch buffer overflows. */
  non_nul_terminated_original_len = strlen (test->original);
  non_nul_terminated_original = (non_nul_terminated_original_len > 0) ? g_memdup2 (test->original, non_nul_terminated_original_len) : g_strdup (test->original);
  result2 = g_markup_escape_text (non_nul_terminated_original, non_nul_terminated_original_len);
  g_assert_cmpstr (result2, ==, test->expected);

  /* Check that the escaped output gets parsed as text by the markup parser. i.e.
   * the escaping worked. We have to wrap it in a single element, as XML
   * requires that. */
  context = g_markup_parse_context_new (&parser, G_MARKUP_DEFAULT_FLAGS, &parsed_text, NULL);
  wrapped_result = g_strdup_printf ("<wrapper>%s</wrapper>", result);
  g_markup_parse_context_parse (context, wrapped_result, -1, &local_error);

  if (test->expected_markup_parse_error)
    {
      g_assert_error (local_error, G_MARKUP_ERROR, (int) test->expected_markup_parse_error_code);
    }
  else
    {
      /* We can’t just check that `parsed_text == test->original`, as the XML
       * parser will normalise newlines to \n. */
      char *normalised_original = g_strdelimit (g_strdup (test->original), "\r", '\n');
      g_assert_no_error (local_error);
      g_assert_cmpstr (parsed_text, ==, normalised_original);
      g_free (normalised_original);
    }

  g_clear_error (&local_error);
  g_markup_parse_context_free (context);
  g_free (parsed_text);
  g_free (wrapped_result);
  g_free (result);
  g_free (result2);
  g_free (non_nul_terminated_original);
}

typedef struct _UnicharTest UnicharTest;

struct _UnicharTest
{
  gunichar c;
  gboolean entity;
};

static UnicharTest unichar_tests[] =
{
  { 0x1, TRUE },
  { 0x8, TRUE },
  { 0x9, FALSE },
  { 0xa, FALSE },
  { 0xb, TRUE },
  { 0xc, TRUE },
  { 0xd, FALSE },
  { 0xe, TRUE },
  { 0x1f, TRUE },
  { 0x20, FALSE },
  { 0x7e, FALSE },
  { 0x7f, TRUE },
  { 0x84, TRUE },
  { 0x85, FALSE },
  { 0x86, TRUE },
  { 0x9f, TRUE },
  { 0xa0, FALSE }
};

static void
unichar_test (gconstpointer d)
{
  const UnicharTest *test = d;
  EscapeTest t;
  gint len;
  gchar outbuf[7], expected[12];

  len = g_unichar_to_utf8 (test->c, outbuf);
  outbuf[len] = 0;

  if (test->entity)
    g_snprintf (expected, 12, "&#x%x;", test->c);
  else
    strcpy (expected, outbuf);

  t.original = outbuf;
  t.expected = expected;
  t.expected_markup_parse_error = FALSE;
  t.expected_markup_parse_error_code = 0;

  escape_test (&t);
}

G_GNUC_PRINTF(1, 3)
static void
test_format (const gchar *format,
	     const gchar *expected,
	     ...)
{
  gchar *result;
  va_list args;

  va_start (args, expected);
  result = g_markup_vprintf_escaped (format, args);
  va_end (args);

  g_assert_cmpstr (result, ==, expected);

  g_free (result);
}

static void
format_test (void)
{
  test_format ("A", "A");
  test_format ("A%s", "A&amp;", "&");
  test_format ("%sA", "&amp;A", "&");
  test_format ("A%sA", "A&amp;A", "&");
  test_format ("%s%sA", "&amp;&amp;A", "&", "&");
  test_format ("A%s%s", "A&amp;&amp;", "&", "&");
  test_format ("A%s%sA", "A&amp;&amp;A", "&", "&");
  test_format ("A%sA%sA", "A&amp;A&amp;A", "&", "&");
  test_format ("%s", "&lt;B&gt;&amp;", "<B>&");
  test_format ("%c%c", "&lt;&amp;", '<', '&');
  test_format (".%c.%c.", ".&lt;.&amp;.", '<', '&');
  test_format ("%s", "", "");
  test_format ("%-5s", "A    ", "A");
  test_format ("%2$s%1$s", "B.A.", "A.", "B.");
}

int main (int argc, char **argv)
{
  gsize i;
  gchar *path;

  g_test_init (&argc, &argv, NULL);

  for (i = 0; i < G_N_ELEMENTS (escape_tests); i++)
    {
      path = g_strdup_printf ("/markup/escape-text/%" G_GSIZE_FORMAT, i);
      g_test_add_data_func (path, &escape_tests[i], escape_test);
      g_free (path);
    }

  for (i = 0; i < G_N_ELEMENTS (unichar_tests); i++)
    {
      path = g_strdup_printf ("/markup/escape-unichar/%" G_GSIZE_FORMAT, i);
      g_test_add_data_func (path, &unichar_tests[i], unichar_test);
      g_free (path);
    }

  g_test_add_func ("/markup/format", format_test);

  return g_test_run ();
}
