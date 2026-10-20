#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>

typedef enum
{
  VALID,
  INCOMPLETE,
  NOTUNICODE,
  OVERLONG,
  MALFORMED
} Status;

static gboolean
ucs4_equal (gunichar *a, gunichar *b)
{
  while (*a && *b && (*a == *b))
    {
      a++;
      b++;
    }

  return (*a == *b);
}

static gboolean
utf16_equal (gunichar2 *a, gunichar2 *b)
{
  while (*a && *b && (*a == *b))
    {
      a++;
      b++;
    }

  return (*a == *b);
}

static gint
utf16_count (gunichar2 *a)
{
  gint result = 0;

  while (a[result])
    result++;

  return result;
}

static void
process (gint      line,
         gchar    *utf8,
         Status    status,
         gunichar *ucs4,
         gint      ucs4_len)
{
  const gchar *end;
  gboolean is_valid = g_utf8_validate (utf8, -1, &end);
  GError *error = NULL;
  glong items_read, items_written;

  switch (status)
    {
    case VALID:
      g_assert_true (is_valid);
      break;

    case NOTUNICODE:
    case INCOMPLETE:
    case OVERLONG:
    case MALFORMED:
      g_assert_false (is_valid);
      break;
    }

  if (status == INCOMPLETE)
    {
      gunichar *ucs4_result;

      ucs4_result = g_utf8_to_ucs4 (utf8, -1, NULL, NULL, &error);

      g_assert_error (error, G_CONVERT_ERROR, G_CONVERT_ERROR_PARTIAL_INPUT);

      g_clear_error (&error);

      ucs4_result = g_utf8_to_ucs4 (utf8, -1, &items_read, NULL, &error);
      g_assert_no_error (error);
      g_assert_nonnull (ucs4_result);
      g_assert_cmpint (items_read, !=, strlen (utf8));

      g_free (ucs4_result);
    }

  if (status == VALID || status == NOTUNICODE)
    {
      gunichar *ucs4_result;

      ucs4_result = g_utf8_to_ucs4 (utf8, -1, &items_read, &items_written, &error);
      g_assert_no_error (error);
      g_assert_nonnull (ucs4_result);

      g_assert_true (ucs4_equal (ucs4_result, ucs4));
      g_assert_cmpint (items_read, ==, strlen (utf8));
      g_assert_cmpint (items_written, ==, ucs4_len);

      g_free (ucs4_result);
    }

  if (status == VALID)
     {
      gunichar *ucs4_result;
      gchar *utf8_result;

      ucs4_result = g_utf8_to_ucs4_fast (utf8, -1, &items_written);
      g_assert_nonnull (ucs4_result);

      g_assert_true (ucs4_equal (ucs4_result, ucs4));
      g_assert_cmpint (items_written, ==, ucs4_len);

      utf8_result = g_ucs4_to_utf8 (ucs4_result, -1, &items_read, &items_written, &error);
      g_assert_no_error (error);
      g_assert_nonnull (utf8_result);

      g_assert_cmpstr ((char *) utf8_result, ==, (char *) utf8);
      g_assert_cmpint (items_read, ==, ucs4_len);
      g_assert_cmpint (items_written, ==, strlen (utf8));

      g_free (utf8_result);
      g_free (ucs4_result);
    }

  if (status == VALID)
    {
      gunichar2 *utf16_expected_tmp;
      gunichar2 *utf16_expected;
      gunichar2 *utf16_from_utf8;
      gunichar2 *utf16_from_ucs4;
      gunichar *ucs4_result;
      gsize bytes_written;
      gint n_chars;
      gchar *utf8_result;

#if G_BYTE_ORDER == G_LITTLE_ENDIAN
#define TARGET "UTF-16LE"
#else
#define TARGET "UTF-16"
#endif

      utf16_expected_tmp =
        (gunichar2 *) g_convert (utf8, -1, TARGET, "UTF-8", NULL, &bytes_written, NULL);
      g_assert_nonnull (utf16_expected_tmp);

      /* zero-terminate and remove BOM */
      n_chars = bytes_written / 2;
      if (utf16_expected_tmp[0] == 0xfeff) /* BOM */
        {
          n_chars--;
          utf16_expected = g_new (gunichar2, n_chars + 1);
          memcpy (utf16_expected, utf16_expected_tmp + 1, sizeof(gunichar2) * n_chars);
        }
      else
        {
          /* We expect the result of the conversion
             via iconv() to be native-endian. */
          g_assert_false (utf16_expected_tmp[0] == 0xfffe);
          utf16_expected = g_new (gunichar2, n_chars + 1);
          memcpy (utf16_expected, utf16_expected_tmp, sizeof(gunichar2) * n_chars);
        }

      utf16_expected[n_chars] = '\0';

      utf16_from_utf8 =
        g_utf8_to_utf16 (utf8, -1, &items_read, &items_written, &error);
      g_assert_no_error (error);
      g_assert_nonnull (utf16_from_utf8);

      g_assert_cmpint (items_read, ==, (glong) strlen (utf8));
      g_assert_cmpint (utf16_count (utf16_from_utf8), ==, items_written);

      utf16_from_ucs4 =
        g_ucs4_to_utf16 (ucs4, -1, &items_read, &items_written, &error);
      g_assert_no_error (error);
      g_assert_nonnull (utf16_from_ucs4);

      g_assert_cmpint (items_read, ==, ucs4_len);
      g_assert_cmpint (utf16_count (utf16_from_ucs4), ==, items_written);

      g_assert_true (utf16_equal (utf16_from_utf8, utf16_expected));
      g_assert_true (utf16_equal (utf16_from_ucs4, utf16_expected));
      g_assert_cmpstr ((char *) utf16_from_utf8, ==, (char *) utf16_expected);
      g_assert_cmpstr ((char *) utf16_from_ucs4, ==, (char *) utf16_expected);

      utf8_result =
        g_utf16_to_utf8 (utf16_from_utf8, -1, &items_read, &items_written, &error);
      g_assert_no_error (error);
      g_assert_nonnull (utf8_result);

      g_assert_cmpint (items_read, ==, utf16_count (utf16_from_utf8));
      g_assert_cmpint (items_written, ==, (glong) strlen (utf8));

      ucs4_result =
        g_utf16_to_ucs4 (utf16_from_ucs4, -1, &items_read, &items_written, &error);
      g_assert_no_error (error);
      g_assert_nonnull (ucs4_result);

      g_assert_cmpint (items_read, ==, utf16_count (utf16_from_utf8));
      g_assert_cmpint (items_written, ==, ucs4_len);

      g_assert_cmpstr (utf8, ==, utf8_result);
      g_assert_cmpstr ((char *) ucs4, ==, (char *) ucs4_result);

      g_free (utf16_expected_tmp);
      g_free (utf16_expected);
      g_free (utf16_from_utf8);
      g_free (utf16_from_ucs4);
      g_free (utf8_result);
      g_free (ucs4_result);
    }
}

static void
test_unicode_encoding (void)
{
  gchar *testfile;
  gchar *contents;
  GError *error = NULL;
  gchar *p, *end;
  char *tmp;
  gint state = 0;
  gint line = 1;
  gint start_line = 0;   /* Quiet GCC */
  gchar *utf8 = NULL;    /* Quiet GCC */
  GArray *ucs4;
  Status status = VALID; /* Quiet GCC */

  testfile = g_test_build_filename (G_TEST_DIST, "utf8.txt", NULL);

  g_file_get_contents (testfile, &contents, NULL, &error);
  g_assert_no_error (error);

  ucs4 = g_array_new (TRUE, FALSE, sizeof(gunichar));

  p = contents;

  /* Loop over lines */
  while (*p)
    {
      while (*p && (*p == ' ' || *p == '\t'))
        p++;

      end = p;
      while (*end && (*end != '\r' && *end != '\n'))
        end++;

      if (!*p || *p == '#' || *p == '\r' || *p == '\n')
        goto next_line;

      tmp = g_strstrip (g_strndup (p, end - p));

      switch (state)
        {
        case 0:
          /* UTF-8 string */
          start_line = line;
          utf8 = tmp;
          tmp = NULL;
          break;

        case 1:
          /* Status */
          if (!strcmp (tmp, "VALID"))
            status = VALID;
          else if (!strcmp (tmp, "INCOMPLETE"))
            status = INCOMPLETE;
          else if (!strcmp (tmp, "NOTUNICODE"))
            status = NOTUNICODE;
          else if (!strcmp (tmp, "OVERLONG"))
            status = OVERLONG;
          else if (!strcmp (tmp, "MALFORMED"))
            status = MALFORMED;
          else
            g_assert_not_reached ();

          if (status != VALID && status != NOTUNICODE)
            state++;  /* No UCS-4 data */
          break;

        case 2:
          /* UCS-4 version */
          p = strtok (tmp, " \t");
          while (p)
            {
              gchar *endptr;
              gunichar ch = strtoul (p, &endptr, 16);
              g_assert_cmpint (*endptr, == ,'\0');

              g_array_append_val (ucs4, ch);

              p = strtok (NULL, " \t");
            }
          break;
        }

      g_free (tmp);
      state = (state + 1) % 3;

      if (state == 0)
        {
          process (start_line, utf8, status, (gunichar *)ucs4->data, ucs4->len);
          g_array_set_size (ucs4, 0);
          g_free (utf8);
        }

    next_line:
      p = end;
      if (*p && *p == '\r')
        p++;
      if (*p && *p == '\n')
        p++;
      line++;
    }

  g_free (testfile);
  g_array_free (ucs4, TRUE);
  g_free (contents);
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/unicode/encoding", test_unicode_encoding);

  return g_test_run ();
}
