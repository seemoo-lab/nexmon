/* We are testing some deprecated APIs here */
#ifndef GLIB_DISABLE_DEPRECATION_WARNINGS
#define GLIB_DISABLE_DEPRECATION_WARNINGS
#endif

#include "glib.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <locale.h>
#include <time.h>

#ifdef G_OS_WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
/* mingw defines it while msvc doesn't */
#ifndef SUBLANG_LITHUANIAN_LITHUANIA
#define SUBLANG_LITHUANIAN_LITHUANIA 0x01
#endif
#endif

static void
test_basic (void)
{
  GDate *d;
  struct tm tm = { 0 };

  /* g_date_valid (d) */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_false (g_date_valid (NULL));
      g_test_assert_expected_messages ();
    }

  /* g_date_new_dmy (d, m, y) */
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_null (g_date_new_dmy (0, 0, 0));
      g_test_assert_expected_messages ();
    }

  d = g_date_new ();
  if (g_test_undefined ())
    {
      /* g_date_get_weekday (d) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_weekday (d), ==, G_DATE_BAD_WEEKDAY);
      g_test_assert_expected_messages ();

      /* g_date_get_day (d) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_day (d), ==, G_DATE_BAD_DAY);
      g_test_assert_expected_messages ();

      /* g_date_get_month (d) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_month (d), ==, G_DATE_BAD_MONTH);
      g_test_assert_expected_messages ();

      /* g_date_get_year (d) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_year (d), ==, G_DATE_BAD_YEAR);
      g_test_assert_expected_messages ();

      /* g_date_to_struct_tm (d, tm) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_to_struct_tm (d, &tm);
      g_test_assert_expected_messages ();

      /* g_is_leap_year (y) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_false (g_date_is_leap_year (0));
      g_test_assert_expected_messages ();

      /* g_date_get_days_in_month (m, y) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_days_in_month (0, 1), ==, 0);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_days_in_month (1, 0), ==, 0);
      g_test_assert_expected_messages ();

      /* g_date_set_time_t (d, t) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_set_time_t (NULL, 1);
      g_test_assert_expected_messages ();

      /* g_date_is_first_of_month (d) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_false (g_date_is_first_of_month (d));
      g_test_assert_expected_messages ();

      /* g_date_is_last_of_month (d) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_false (g_date_is_last_of_month (d));
      g_test_assert_expected_messages ();

      /* g_date_add_days (d, n) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_add_days (d, 1);
      g_test_assert_expected_messages ();

      /* g_date_subtract_days (d, n) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_subtract_days (d, 1);
      g_test_assert_expected_messages ();

      /* g_date_add_months (d, n) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_add_months (d, 1);
      g_test_assert_expected_messages ();

      /* g_date_subtract_months (d, n) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_subtract_months (d, 1);
      g_test_assert_expected_messages ();

      /* g_date_add_years (d, y) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_add_years (d, 1);
      g_test_assert_expected_messages ();

      /* g_date_subtract_years (d, y) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_subtract_years (d, 1);
      g_test_assert_expected_messages ();

      /* g_date_set_month (d, m) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_set_month (NULL, 1);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_set_month (d, 13);
      g_test_assert_expected_messages ();

      /* g_date_set_day (d, day) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_set_day (NULL, 1);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_set_day (d, 32);
      g_test_assert_expected_messages ();

      /* g_date_set_year (d, y) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_set_year (NULL, 1);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_set_year (d, (GDateYear) (G_MAXUINT16 + 1));
      g_test_assert_expected_messages ();

      /* g_date_set_dmy (date, d, m, y) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_set_dmy (NULL, 1, 1, 1);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_set_dmy (d, 0, 0, 0);
      g_test_assert_expected_messages ();

      /* g_date_set_julian (date, d) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_set_julian (NULL, 1);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_set_julian (d, 0);
      g_test_assert_expected_messages ();

      /* g_date_clear (d, n) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_clear (d, 0);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_clear (NULL, 1);
      g_test_assert_expected_messages ();
    }

  g_date_set_dmy (d, 1, 1, 1);
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_to_struct_tm (d, NULL);
      g_test_assert_expected_messages ();
   }
  g_date_free (d);

  g_assert_cmpint (sizeof (GDate), <,  9);
  g_assert_false (g_date_valid_month (G_DATE_BAD_MONTH));
  g_assert_false (g_date_valid_month (13));
  g_assert_false (g_date_valid_day (G_DATE_BAD_DAY));
  g_assert_false (g_date_valid_day (32));
  g_assert_false (g_date_valid_year (G_DATE_BAD_YEAR));
  g_assert_false (g_date_valid_julian (G_DATE_BAD_JULIAN));
  g_assert_false (g_date_valid_weekday (G_DATE_BAD_WEEKDAY));
  g_assert_true (g_date_valid_weekday ((GDateWeekday) 1));
  g_assert_false (g_date_valid_weekday ((GDateWeekday) 8));
  g_assert_true (g_date_is_leap_year (2000));
  g_assert_false (g_date_is_leap_year (1999));
  g_assert_true (g_date_is_leap_year (1996));
  g_assert_true (g_date_is_leap_year (1600));
  g_assert_false (g_date_is_leap_year (2100));
  g_assert_false (g_date_is_leap_year (1800));
}

static void
test_empty_constructor (void)
{
  GDate *d;

  d = g_date_new ();
  g_assert_false (g_date_valid (d));
  g_date_free (d);

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_free (NULL);
      g_test_assert_expected_messages ();
    }

}

static void
test_dmy_constructor (void)
{
  GDate *d;
  guint32 j;

  d = g_date_new_dmy (1, 1, 1);
  g_assert_true (g_date_valid (d));

  j = g_date_get_julian (d);
  g_assert_cmpint (j, ==, 1);
  g_assert_cmpint (g_date_get_month (d), ==, G_DATE_JANUARY);
  g_assert_cmpint (g_date_get_day (d), ==, 1);
  g_assert_cmpint (g_date_get_year (d), ==, 1);
  g_date_free (d);
}

static void
test_date_compare (void)
{
  GDate *d1;
  GDate *d2;

  d1 = g_date_new ();
  d2 = g_date_new ();
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_days_between (d1, d2), ==, 0);
      g_test_assert_expected_messages ();

      g_date_set_dmy (d1, 1, 1, 1);
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_days_between (d1, d2), ==, 0);
      g_test_assert_expected_messages ();
    }
  g_date_free (d1);
  g_date_free (d2);

  d1 = g_date_new ();
  d2 = g_date_new ();
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_compare (NULL, d2), ==, 0);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_compare (d1, NULL), ==, 0);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_compare (d1, d2), ==, 0);
      g_test_assert_expected_messages ();

      g_date_set_dmy (d1, 1, 1, 1);
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_compare (d1, d2), ==, 0);
      g_test_assert_expected_messages ();
    }
  g_date_free (d1);
  g_date_free (d2);

  d1 = g_date_new ();
  d2 = g_date_new ();

  /* DMY format */
  g_date_set_dmy (d1, 1, 1, 1);
  g_date_set_dmy (d2, 10, 1, 1);

  g_assert_cmpint (g_date_compare (d1, d1), ==, 0);

  g_assert_cmpint (g_date_compare (d1, d2), ==, -1);
  g_assert_cmpint (g_date_compare (d2, d1), >, 0);

  g_date_set_dmy (d2, 1, 10, 1);
  g_assert_cmpint (g_date_compare (d1, d2), ==, -1);
  g_assert_cmpint (g_date_compare (d2, d1), >, 0);

  g_date_set_dmy (d2, 1, 1, 10);
  g_assert_cmpint (g_date_compare (d1, d2), ==, -1);
  g_assert_cmpint (g_date_compare (d2, d1), >, 0);

  /* Julian format */
  g_date_set_julian (d1, 1);
  g_date_set_julian (d2, 10);

  g_assert_cmpint (g_date_compare (d1, d1), ==, 0);

  g_assert_cmpint (g_date_compare (d1, d2), ==, -1);
  g_assert_cmpint (g_date_compare (d2, d1), >, 0);

  g_date_set_julian (d2, 32);
  g_assert_cmpint (g_date_compare (d1, d2), ==, -1);
  g_assert_cmpint (g_date_compare (d2, d1), >, 0);

  g_date_set_julian (d2, 366);
  g_assert_cmpint (g_date_compare (d1, d2), ==, -1);
  g_assert_cmpint (g_date_compare (d2, d1), >, 0);

  g_date_free (d1);
  g_date_free (d2);
}

static void
test_julian_constructor (void)
{
  GDate *d1;
  GDate *d2;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_null (g_date_new_julian (0));
      g_test_assert_expected_messages ();
    }

  d1 = g_date_new ();
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_julian (d1), ==, G_DATE_BAD_JULIAN);
      g_test_assert_expected_messages ();
    }
  g_date_free (d1);

  d1 = g_date_new_julian (4000);
  d2 = g_date_new_julian (5000);
  g_assert_cmpint (g_date_get_julian (d1), ==, 4000);
  g_assert_cmpint (g_date_days_between (d1, d2), ==, 1000);
  g_assert_cmpint (g_date_get_year (d1), ==, 11);
  g_assert_cmpint (g_date_get_day (d2), ==, 9);
  g_date_free (d1);
  g_date_free (d2);
}

static void
test_dates (void)
{
  GDate *d;
  GTimeVal tv;
  time_t now;

  d = g_date_new ();

  /* getters on empty date */
  if (g_test_undefined ())
    {
      /* g_date_get_day_of_year (d) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_day_of_year (d), ==, 0);
      g_test_assert_expected_messages ();

      /* g_date_get_monday_week_of_year (d) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_monday_week_of_year (d), ==, 0);
      g_test_assert_expected_messages ();

      /* g_date_get_monday_weeks_in_year (y) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_monday_weeks_in_year (0), ==, 0);
      g_test_assert_expected_messages ();

      /* g_date_get_sunday_week_of_year (d) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_sunday_week_of_year (d), ==, 0);
      g_test_assert_expected_messages ();

      /* g_date_get_sunday_weeks_in_year (y) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_sunday_weeks_in_year (0), ==, 0);
      g_test_assert_expected_messages ();

      /* g_date_get_week_of_year (d) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_week_of_year (d, G_DATE_MONDAY), ==, 0);
      g_test_assert_expected_messages ();

      /* g_date_get_weeks_in_year (y) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_weeks_in_year (0, G_DATE_MONDAY), ==, 0);
      g_test_assert_expected_messages ();

      /* g_date_get_iso8601_week_of_year (d) */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_cmpint (g_date_get_iso8601_week_of_year (d), ==, 0);
      g_test_assert_expected_messages ();
    }

  g_date_free (d);

  /* Remove more time than we have */
  d = g_date_new_julian (1);
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_subtract_days (d, 103);
      g_test_assert_expected_messages ();
    }
  g_date_free (d);

  d = g_date_new_julian (375);
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_subtract_months (d, 13);
      g_test_assert_expected_messages ();
    }
  g_date_free (d);

  d = g_date_new_julian (375);
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_subtract_years (d, 2);
      g_test_assert_expected_messages ();
    }
  g_date_free (d);

  /* Test on leap years */
  g_assert_cmpint (g_date_get_monday_weeks_in_year (1764), ==, 53);
  g_assert_cmpint (g_date_get_monday_weeks_in_year (1776), ==, 53);

  g_assert_cmpint (g_date_get_sunday_weeks_in_year (1792), ==, 53);

  g_assert_cmpint (g_date_get_weeks_in_year (2000, G_DATE_SATURDAY), ==, 53);

  /* Trigger the update of the dmy/julian parts */
  d = g_date_new_julian (1);
  g_assert_cmpint (g_date_get_day_of_year (d), ==, 1);
  g_date_free (d);

  d = g_date_new_julian (1);
  g_assert_cmpint (g_date_get_monday_week_of_year (d), ==, 1);
  g_date_free (d);

  d = g_date_new_julian (1);
  g_assert_cmpint (g_date_get_sunday_week_of_year (d), ==, 0);
  g_date_free (d);

  d = g_date_new_julian (1);
  g_assert_cmpint (g_date_is_first_of_month (d), ==, 1);
  g_date_free (d);

  d = g_date_new_dmy (31, 3, 8);
  g_date_subtract_months (d, 1);
  g_assert_cmpint (g_date_get_month (d), ==, 2);
  g_assert_cmpint (g_date_get_day (d), ==, 29);
  g_assert_cmpint (g_date_get_year (d), ==, 8);
  g_date_free (d);

  d = g_date_new_julian (375);
  g_date_add_months (d, 1);
  g_assert_cmpint (g_date_get_month (d), ==, 2);
  g_assert_cmpint (g_date_get_day (d), ==, 10);
  g_assert_cmpint (g_date_get_year (d), ==, 2);
  g_date_free (d);

  d = g_date_new_julian (375);
  g_date_subtract_months (d, 1);
  g_assert_cmpint (g_date_get_month (d), ==, 12);
  g_assert_cmpint (g_date_get_day (d), ==, 10);
  g_assert_cmpint (g_date_get_year (d), ==, 1);
  g_date_free (d);

  d = g_date_new_julian (375);
  g_date_add_years (d, 1);
  g_assert_cmpint (g_date_get_month (d), ==, 1);
  g_assert_cmpint (g_date_get_day (d), ==, 10);
  g_assert_cmpint (g_date_get_year (d), ==, 3);
  g_date_free (d);

  d = g_date_new_julian (675);
  g_date_subtract_years (d, 1);
  g_assert_cmpint (g_date_get_month (d), ==, 11);
  g_assert_cmpint (g_date_get_day (d), ==, 6);
  g_assert_cmpint (g_date_get_year (d), ==, 1);
  g_date_free (d);

  d = g_date_new_dmy (28, 2, 7);
  g_date_subtract_years (d, 1);
  g_assert_cmpint (g_date_get_month (d), ==, 2);
  g_assert_cmpint (g_date_get_day (d), ==, 28);
  g_assert_cmpint (g_date_get_year (d), ==, 6);
  g_date_free (d);

  d = g_date_new_dmy (29, 2, 8);
  g_date_subtract_years (d, 1);
  g_assert_cmpint (g_date_get_month (d), ==, 2);
  g_assert_cmpint (g_date_get_day (d), ==, 28);
  g_assert_cmpint (g_date_get_year (d), ==, 7);
  g_date_free (d);

  d = g_date_new_dmy (1, 1, 1);
  g_assert_cmpint (g_date_get_iso8601_week_of_year (d), ==, 1);
  g_date_free (d);

  d = g_date_new_julian (1);
  g_date_set_year (d, 6);
  g_assert_cmpint (g_date_get_month (d), ==, 1);
  g_assert_cmpint (g_date_get_day (d), ==, 1);
  g_assert_cmpint (g_date_get_year (d), ==, 6);
  g_date_free (d);

  d = g_date_new_dmy (1, 1, 1);
  g_date_set_year (d, 6);
  g_assert_cmpint (g_date_get_month (d), ==, 1);
  g_assert_cmpint (g_date_get_day (d), ==, 1);
  g_assert_cmpint (g_date_get_year (d), ==, 6);
  g_date_free (d);

  d = g_date_new_julian (1);
  g_date_set_month (d, 6);
  g_assert_cmpint (g_date_get_month (d), ==, 6);
  g_assert_cmpint (g_date_get_day (d), ==, 1);
  g_assert_cmpint (g_date_get_year (d), ==, 1);
  g_date_free (d);

  d = g_date_new_dmy (1, 1, 1);
  g_date_set_month (d, 6);
  g_assert_cmpint (g_date_get_month (d), ==, 6);
  g_assert_cmpint (g_date_get_day (d), ==, 1);
  g_assert_cmpint (g_date_get_year (d), ==, 1);
  g_date_free (d);

  d = g_date_new_julian (1);
  g_date_set_day (d, 6);
  g_assert_cmpint (g_date_get_month (d), ==, 1);
  g_assert_cmpint (g_date_get_day (d), ==, 6);
  g_assert_cmpint (g_date_get_year (d), ==, 1);
  g_date_free (d);

  d = g_date_new_dmy (1, 1, 1);
  g_date_set_day (d, 6);
  g_assert_cmpint (g_date_get_month (d), ==, 1);
  g_assert_cmpint (g_date_get_day (d), ==, 6);
  g_assert_cmpint (g_date_get_year (d), ==, 1);
  g_date_free (d);

  d = g_date_new_julian (1);
  g_assert_cmpint (g_date_get_month (d), ==, 1);
  g_date_free (d);

  /* Correct usage */
  d = g_date_new ();

  /* today */
  now = time (NULL);
  g_assert_cmpint (now, !=, (time_t) -1);
  g_date_set_time (d, now);
  g_assert_true (g_date_valid (d));

  /* Unix epoch */
  g_date_set_time (d, 1);
  g_assert_true (g_date_valid (d));

  tv.tv_sec = 0;
  tv.tv_usec = 0;
  g_date_set_time_val (d, &tv);
  g_assert_true (g_date_valid (d));

  /* Julian day 1 */
  g_date_set_julian (d, 1);
  g_assert_true (g_date_valid (d));

  g_date_set_year (d, 3);
  g_date_set_day (d, 3);
  g_date_set_month (d, 3);
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_year (d), ==, 3);
  g_assert_cmpint (g_date_get_month (d), ==, 3);
  g_assert_cmpint (g_date_get_day (d), ==, 3);
  g_assert_false (g_date_is_first_of_month (d));
  g_assert_false (g_date_is_last_of_month (d));
  g_date_set_day (d, 1);
  g_assert_true (g_date_is_first_of_month (d));
  g_date_subtract_days (d, 1);
  g_assert_true (g_date_is_last_of_month (d));

  /* Testing some other corner cases */
  g_date_set_dmy (d, 29, 2, 2000);
  g_date_subtract_months (d, 2);
  g_assert_cmpint (g_date_get_month (d), ==, 12);
  g_assert_cmpint (g_date_get_day (d), ==, 29);
  g_assert_cmpint (g_date_get_year (d), ==, 1999);

  /* Attempt to assign a February 29 to a non-leap year */
  g_date_set_month (d, 2);
  g_date_set_day (d, 29);
  g_assert_false (g_date_valid (d));
  g_date_set_year (d, 3);
  g_assert_false (g_date_valid (d));

  g_date_free (d);
}

static void
test_strftime (void)
{
  gsize i;
  GDate *d;
  gchar buf[101];
  const gchar invalid[] = "hello\xffworld%x";
  gchar *oldlocale;
#ifdef G_OS_WIN32
  LCID old_lcid;
#endif

  struct
  {
    const gchar *format;
    const gchar *expect;
  } strftime_checks[] = {
    { "%A", "Monday" },
    { "%a", "Mon" },
    { "%D", "01/01/01" },
    { "%d", "01" },
    { "%e", " 1" },
    { "%H", "00" },
    { "%I", "12" },
    { "%j", "001" },
    { "%M", "00" },
    { "%m", "01" },
    { "%n", "\n" },
    { "%OB", "January" },
    { "%Ob", "Jan" },
    { "%p", "AM" },
    { "%R", "00:00" },
    { "%S", "00" },
    { "%T", "00:00:00" },
    { "%t", "\t" },
    { "%U", "00" },
    { "%u", "1" },
    { "%V", "01" },
    { "%W", "01" },
    { "%w", "1" },
    { "%y", "01" },
    { "%z", "" },
    { "%%", "%" },
#if defined(G_OS_WIN32)
    { "%C", "00" },
    { "%c", " 12:00:00 AM" },
    { "%E", "" },
    { "%F", "" },
    { "%G", "" },
    { "%g", "" },
    { "%h", "" },
    { "%k", "" },
    { "%l", "" },
    { "%O", "" },
    { "%P", "" },
    { "%r", "12:00:00AM" },
    { "%X", "12:00:00 AM" },
    { "%x", "" },
    { "%Y", "0001" },
#else
    { "%B", "January" },
    { "%b", "Jan" },
#if defined(__FreeBSD__) || defined(__OpenBSD__) || defined(__APPLE__)
    { "%C", "00" },
    { "%c", "Mon Jan  1 00:00:00 0001" },
    { "%E", "E" },
    { "%F", "0001-01-01" },
    { "%G", "0001" },
    { "%O", "O" },
    { "%P", "P" },
    { "%Y", "0001" },
#else
    { "%C", "0" },
    { "%c", "Mon Jan  1 00:00:00 1" },
    { "%E", "%E" },
    { "%F", "1-01-01" },
    { "%G", "1" },
    { "%O", "%O" },
    { "%P", "am" },
    { "%Y", "1" },
#endif
    { "%g", "01" },
    { "%h", "Jan" },
    { "%k", " 0" },
    { "%l", "12" },
    { "%r", "12:00:00 AM" },
    { "%X", "00:00:00" },
    { "%x", "01/01/01" },
    { "%Z", "" },
#endif
  };

  oldlocale = g_strdup (setlocale (LC_ALL, NULL));
#ifdef G_OS_WIN32
  old_lcid = GetThreadLocale ();
#endif

  /* Make sure that nothing has been changed in the original locales.  */
  setlocale (LC_ALL, "C");
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
#endif

  d = g_date_new ();

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_strftime (buf, sizeof (buf), "%x", d);
      g_test_assert_expected_messages ();
    }

  /* Trying invalid character */
#ifndef G_OS_WIN32
  if (g_test_undefined ())
    {
      g_date_set_dmy (d, 10, 1, 2000);
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_WARNING, "*Error*");
      g_date_strftime (buf, sizeof (buf), invalid, d);
      g_test_assert_expected_messages ();
      g_assert_cmpstr (buf, ==, "");
    }
#else
  g_date_set_dmy (d, 10, 1, 2000);
  g_date_strftime (buf, sizeof (buf), invalid, d);
  g_assert_cmpstr (buf, ==, "");
#endif

  /* Test positive cases */
  g_date_set_dmy (d, 1, 1, 1);

  for (i = 0; i < G_N_ELEMENTS (strftime_checks); i++)
    {
      g_date_strftime (buf, sizeof (buf), strftime_checks[i].format, d);
      g_assert_cmpstr (buf, ==, strftime_checks[i].expect);
    }

#ifdef G_OS_WIN32
  /*
   * Time zone is too versatile on OS_WIN32 to be checked precisely,
   * According to msdn: "Either the locale's time-zone name
   * or time zone abbreviation, depending on registry settings; no characters
   * if time zone is unknown".
   */
  g_assert_cmpint (g_date_strftime (buf, sizeof (buf), "%Z", d), !=, 0);
#endif

  g_date_free (d);

  setlocale (LC_ALL, oldlocale);
  g_free (oldlocale);
#ifdef G_OS_WIN32
  SetThreadLocale (old_lcid);
#endif
}

static void
test_two_digit_years (void)
{
  GDate *d;
  gchar buf[101];
  gchar *old_locale;
  gboolean use_alternative_format = FALSE;

#ifdef G_OS_WIN32
  LCID old_lcid;
#endif

  old_locale = g_strdup (setlocale (LC_ALL, NULL));
#ifdef G_OS_WIN32
  old_lcid = GetThreadLocale ();
#endif

  /* Make sure that nothing has been changed in the original locales.  */
  setlocale (LC_ALL, "C");
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT), SORT_DEFAULT));
#endif

  d = g_date_new ();

  /* Check two digit years */
  g_date_set_dmy (d, 10, 10, 1976);
  g_date_strftime (buf, sizeof (buf), "%D", d);
  g_assert_cmpstr (buf, ==, "10/10/76");
  g_date_set_parse (d, buf);

#ifdef G_OS_WIN32
  /*
   * It depends on the locale setting whether the dd/mm/yy
   * format is allowed for g_date_set_parse() on Windows, which
   * corresponds to whether there is an d/M/YY or d/M/YYYY (or so)
   * option in the "Date and Time Format" setting for the selected
   * locale in the Control Panel settings.  If g_date_set_parse()
   * renders the GDate invalid with the dd/mm/yy format, use an
   * alternative format (yy/mm/dd) for g_date_set_parse() for the
   * 2-digit year tests.
   */
  if (!g_date_valid (d))
    use_alternative_format = TRUE;
#endif

  if (use_alternative_format)
    g_date_set_parse (d, "76/10/10");

  g_assert_cmpint (g_date_get_month (d), ==, 10);
  g_assert_cmpint (g_date_get_day (d), ==, 10);
  g_assert_true ((g_date_get_year (d) == 1976) ||
                 (g_date_get_year (d) == 76));

  /* Check two digit years below 100 */
  g_date_set_dmy (d, 10, 10, 29);
  g_date_strftime (buf, sizeof (buf), "%D", d);
  g_assert_cmpstr (buf, ==, "10/10/29");
  g_date_set_parse (d, use_alternative_format ? "29/10/10" : buf);
  g_assert_cmpint (g_date_get_month (d), ==, 10);
  g_assert_cmpint (g_date_get_day (d), ==, 10);
  g_assert_true ((g_date_get_year (d) == 2029) ||
                 (g_date_get_year (d) == 29));

  g_date_free (d);

  setlocale (LC_ALL, old_locale);
  g_free (old_locale);
#ifdef G_OS_WIN32
  SetThreadLocale (old_lcid);
#endif
}

static void
test_parse (void)
{
  GDate *d;
  gchar buf[101];

  d = g_date_new ();

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_set_parse (NULL, "");
      g_test_assert_expected_messages ();
    }

  g_date_set_time (d, 1);
  g_assert_true (g_date_valid (d));
  g_date_strftime (buf, sizeof (buf), "Today is a %A, in the month of %B, %x", d);
  g_date_set_parse (d, buf);

  if (g_test_undefined ())
    {
      /* g_date_strftime() */
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_strftime (NULL, 100, "%x", d);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_strftime (buf, 0, "%x", d);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_strftime (buf, sizeof (buf), NULL, d);
      g_test_assert_expected_messages ();
    }

  g_date_set_julian (d, 1);
  g_assert_true (g_date_valid (d));
#ifndef G_OS_WIN32
  /* Windows FILETIME does not support dates before Jan 1 1601,
     so we can't strftime() the beginning of the "Julian" epoch. */
  g_date_strftime (buf, sizeof (buf), "Today is a %A, in the month of %B, %x", d);
  g_date_set_parse (d, buf);
#endif

  g_date_set_dmy (d, 10, 1, 2000);
  g_assert_true (g_date_valid (d));
  g_date_strftime (buf, sizeof (buf), "%x", d);
  g_date_set_parse (d, buf);
  g_assert_cmpint (g_date_get_month (d), ==, 1);
  g_assert_cmpint (g_date_get_day (d), ==, 10);
  g_assert_cmpint (g_date_get_year (d), ==, 2000);

  g_date_set_parse (d, "2001 10 1");
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_month (d), ==, 10);
  g_assert_cmpint (g_date_get_day (d), ==, 1);
  g_assert_cmpint (g_date_get_year (d), ==, 2001);

  g_date_set_parse (d, "2001 10");
  g_assert_false (g_date_valid (d));

  g_date_set_parse (d, "2001 10 1 1");
  g_assert_false (g_date_valid (d));

  g_date_set_parse (d, "2001-10-01");
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_month (d), ==, 10);
  g_assert_cmpint (g_date_get_day (d), ==, 1);
  g_assert_cmpint (g_date_get_year (d), ==, 2001);

  g_date_set_parse (d, "March 1999");
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_month (d), ==, 3);
  g_assert_cmpint (g_date_get_day (d), ==, 1);
  g_assert_cmpint (g_date_get_year (d), ==, 1999);

  g_date_set_parse (d, "October 98");
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_month (d), ==, 10);
  g_assert_cmpint (g_date_get_day (d), ==, 1);
  g_assert_cmpint (g_date_get_year (d), ==, 98);

  g_date_set_parse (d, "oCT 98");
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_month (d), ==, 10);
  g_assert_cmpint (g_date_get_day (d), ==, 1);
  g_assert_cmpint (g_date_get_year (d), ==, 98);

  g_date_set_parse (d, "10/24/98");
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_month (d), ==, 10);
  g_assert_cmpint (g_date_get_day (d), ==, 24);
  g_assert_true (g_date_get_year (d) == 1998 ||
                 g_date_get_year (d) == 98);

  g_date_set_parse (d, "10 -- 24 -- 98");
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_month (d), ==, 10);
  g_assert_cmpint (g_date_get_day (d), ==, 24);
  g_assert_true (g_date_get_year (d) == 1998 ||
                 g_date_get_year (d) == 98);

  g_date_set_parse (d, "10/24/1998");
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_month (d), ==, 10);
  g_assert_cmpint (g_date_get_day (d), ==, 24);
  g_assert_cmpint (g_date_get_year (d), ==, 1998);

  g_date_set_parse (d, "October 24, 1998");
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_month (d), ==, 10);
  g_assert_cmpint (g_date_get_day (d), ==, 24);
  g_assert_cmpint (g_date_get_year (d), ==, 1998);

  g_date_set_parse (d, "10 Sep 1087");
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_month (d), ==, 9);
  g_assert_cmpint (g_date_get_day (d), ==, 10);
  g_assert_cmpint (g_date_get_year (d), ==, 1087);

  g_date_set_parse (d, "19990301");
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_month (d), ==, 3);
  g_assert_cmpint (g_date_get_day (d), ==, 1);
  g_assert_cmpint (g_date_get_year (d), ==, 1999);

  g_date_set_parse (d, "981024");
  g_assert_true (g_date_valid (d));
  g_assert_cmpint (g_date_get_month (d), ==, 10);
  g_assert_cmpint (g_date_get_day (d), ==, 24);
  g_assert_true (g_date_get_year (d) == 1998 ||
                 g_date_get_year (d) == 98);

  /* Catching some invalid dates */
  g_date_set_parse (d, "20011320");
  g_assert_false (g_date_valid (d));

  g_date_set_parse (d, "19998 10 1");
  g_assert_false (g_date_valid (d));

  g_date_free (d);
}

static void
test_parse_invalid (void)
{
  const gchar * const strs[] =
    {
      /* Incomplete UTF-8 sequence */
      "\xfd",
      /* Ridiculously long input */
      "12345678901234567890123456789012345678901234567890123456789012345678901234567890"
      "12345678901234567890123456789012345678901234567890123456789012345678901234567890"
      "12345678901234567890123456789012345678901234567890123456789012345678901234567890",
    };
  gsize i;

  for (i = 0; i < G_N_ELEMENTS (strs); i++)
    {
      GDate *d = g_date_new ();

      g_test_message ("Test %" G_GSIZE_FORMAT, i);
      g_date_set_parse (d, strs[i]);

      g_assert_false (g_date_valid (d));

      g_date_free (d);
    }
}

static void
test_parse_locale_change (void)
{
  /* Checks that g_date_set_parse correctly changes locale specific data as
   * necessary. In this particular case year adjustment, as Thai calendar is
   * 543 years ahead of the Gregorian calendar. */

  GDate date;

  if (setlocale (LC_ALL, "th_TH") == NULL)
    {
      g_test_skip ("locale th_TH not available");
      return;
    }

  g_date_set_parse (&date, "04/07/2519");

  setlocale (LC_ALL, "C");
  g_date_set_parse (&date, "07/04/76");
  g_assert_cmpint (g_date_get_day (&date), ==, 4);
  g_assert_cmpint (g_date_get_month (&date), ==, 7);
#ifdef G_OS_WIN32
  /* Windows g_date_strftime() implementation doesn't use twodigit_years */
  /* FIXME: check if the function can be changed to return 4 digit years instead
   * See https://gitlab.gnome.org/GNOME/glib/-/issues/2604 */
  g_assert_cmpint (g_date_get_year (&date), ==, 76);
#else
  g_assert_cmpint (g_date_get_year (&date), ==, 1976);
#endif

  setlocale (LC_ALL, "");
}

static void
test_month_substring (void)
{
  GDate date;
#ifdef G_OS_WIN32
  LCID old_lcid;
#endif

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=793550");

  if (setlocale (LC_ALL, "pl_PL") == NULL)
    {
      g_test_skip ("pl_PL locale not available");
      return;
    }

#ifdef G_OS_WIN32
  /* after the crt, set also the win32 thread locale: */
  /* https://www.codeproject.com/Articles/9600/Windows-SetThreadLocale-and-CRT-setlocale */
  old_lcid = GetThreadLocale ();
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_POLISH, SUBLANG_POLISH_POLAND), SORT_DEFAULT));
#endif

  /* In Polish language September is "wrzesień" and August is "sierpień"
   * abbreviated as "sie". The former used to be confused with the latter
   * because "sie" is a substring of "wrzesień" and was matched first. */

  g_date_set_parse (&date, "wrzesień 2018");
  g_assert_true (g_date_valid (&date));
  g_assert_cmpint (g_date_get_month (&date), ==, G_DATE_SEPTEMBER);

  g_date_set_parse (&date, "sie 2018");
  g_assert_true (g_date_valid (&date));
  g_assert_cmpint (g_date_get_month (&date), ==, G_DATE_AUGUST);

  g_date_set_parse (&date, "sierpień 2018");
  g_assert_true (g_date_valid (&date));
  g_assert_cmpint (g_date_get_month (&date), ==, G_DATE_AUGUST);

#ifdef G_OS_WIN32
  SetThreadLocale (old_lcid);
#endif
  setlocale (LC_ALL, "");
}


static void
test_month_names (void)
{
#if defined(HAVE_LANGINFO_ABALTMON) || defined(G_OS_WIN32)
  GDate *gdate;
  gchar buf[101];
  gchar *oldlocale;
#ifdef G_OS_WIN32
  LCID old_lcid;
#endif
#endif  /* defined(HAVE_LANGINFO_ABALTMON) || defined(G_OS_WIN32) */

  g_test_bug ("https://bugzilla.gnome.org/show_bug.cgi?id=749206");

  /* If running uninstalled (G_TEST_BUILDDIR is set), skip this test, since we
   * need the translations to be installed. We can’t mess around with
   * bindtextdomain() here, as the compiled .gmo files in po/ are not in the
   * right installed directory hierarchy to be successfully loaded by gettext. */
  if (g_getenv ("G_TEST_BUILDDIR") != NULL)
    {
      g_test_skip ("Skipping due to running uninstalled. "
                   "This test can only be run when the translations are installed.");
      return;
    }

  /* This test can only work (on non-Windows platforms) if libc supports
   * the %OB (etc.) format placeholders. If it doesn’t, strftime() (and hence
   * g_date_strftime()) will return the placeholder unsubstituted.
   * g_date_strftime() explicitly documents that it doesn’t provide any more
   * format placeholders than the system strftime(), so we should skip the test
   * in that case. If people need %OB support, they should depend on a suitable
   * version of libc, or use g_date_time_format(). Note: a test for a support
   * of _NL_ABALTMON_* is not strictly the same as checking for %OB support.
   * Some platforms (BSD, OS X) support %OB while _NL_ABALTMON_* and %Ob
   * are supported only by glibc 2.27 and newer. But we don’t care about BSD
   * here, the aim of this test is to make sure that our custom implementation
   * for Windows works the same as glibc 2.27 native implementation. */
#if !defined(HAVE_LANGINFO_ABALTMON) && !defined(G_OS_WIN32)
  g_test_skip ("libc doesn’t support all alternative month names");
#else

#define TEST_DATE(d, m, y, f, o)                       \
  G_STMT_START                                         \
  {                                                    \
    gchar *o_casefold, *buf_casefold;                  \
    g_date_set_dmy (gdate, d, m, y);                   \
    g_date_strftime (buf, sizeof (buf), f, gdate);     \
    buf_casefold = g_utf8_casefold (buf, -1);          \
    o_casefold = g_utf8_casefold ((o), -1);            \
    g_assert_cmpstr (buf_casefold, ==, o_casefold);    \
    g_free (buf_casefold);                             \
    g_free (o_casefold);                               \
    g_date_set_parse (gdate, buf);                     \
    g_assert_true (g_date_valid (gdate));              \
    g_assert_cmpint (g_date_get_day (gdate), ==, d);   \
    g_assert_cmpint (g_date_get_month (gdate), ==, m); \
    g_assert_cmpint (g_date_get_year (gdate), ==, y);  \
  }                                                    \
  G_STMT_END

  oldlocale = g_strdup (setlocale (LC_ALL, NULL));
#ifdef G_OS_WIN32
  old_lcid = GetThreadLocale ();
#endif

  gdate = g_date_new ();

  /* Note: Windows implementation of g_date_strftime() does not support
   * "-" format modifier (e.g., "%-d", "%-e") so we will not use it.
   */

  /* Make sure that nothing has been changed in western European languages.  */
  setlocale (LC_ALL, "en_GB.utf-8");
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_ENGLISH, SUBLANG_ENGLISH_UK), SORT_DEFAULT));
#endif
  if (strstr (setlocale (LC_ALL, NULL), "en_GB") != NULL)
    {
      TEST_DATE (1,  1, 2018, "%B %d, %Y", "January 01, 2018");
      TEST_DATE (1,  2, 2018,    "%OB %Y",    "February 2018");
      TEST_DATE (1,  3, 2018,  "%e %b %Y",      " 1 Mar 2018");
      TEST_DATE (1,  4, 2018,    "%Ob %Y",         "Apr 2018");
      TEST_DATE (1,  5, 2018,  "%d %h %Y",      "01 May 2018");
      TEST_DATE (1,  6, 2018,    "%Oh %Y",         "Jun 2018");
    }
  else
    g_test_skip ("locale en_GB not available, skipping English month names test");

  setlocale (LC_ALL, "de_DE.utf-8");
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_GERMAN, SUBLANG_GERMAN), SORT_DEFAULT));
#endif
  if (strstr (setlocale (LC_ALL, NULL), "de_DE") != NULL)
    {
      TEST_DATE (16,  7, 2018, "%d. %B %Y", "16. Juli 2018");
      TEST_DATE ( 1,  8, 2018,    "%OB %Y",   "August 2018");
      TEST_DATE (18,  9, 2018, "%e. %b %Y",  "18. Sep 2018");
      TEST_DATE ( 1, 10, 2018,    "%Ob %Y",      "Okt 2018");
      TEST_DATE (20, 11, 2018, "%d. %h %Y",  "20. Nov 2018");
      TEST_DATE ( 1, 12, 2018,    "%Oh %Y",      "Dez 2018");
    }
  else
    g_test_skip ("locale de_DE not available, skipping German month names test");


  setlocale (LC_ALL, "es_ES.utf-8");
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_SPANISH, SUBLANG_SPANISH_MODERN), SORT_DEFAULT));
#endif
  if (strstr (setlocale (LC_ALL, NULL), "es_ES") != NULL)
    {
      TEST_DATE ( 9,  1, 2018, "%d de %B de %Y", "09 de enero de 2018");
      TEST_DATE ( 1,  2, 2018,      "%OB de %Y",     "febrero de 2018");
#if defined(G_OS_WIN32) && defined(_UCRT)
      TEST_DATE (10,  3, 2018, "%e de %b de %Y",   "10 de mar. de 2018");
      TEST_DATE ( 1,  4, 2018,      "%Ob de %Y",         "abr. de 2018");
      TEST_DATE (11,  5, 2018, "%d de %h de %Y",   "11 de may. de 2018");
      TEST_DATE ( 1,  6, 2018,      "%Oh de %Y",         "jun. de 2018");
#else
      TEST_DATE (10,  3, 2018, "%e de %b de %Y",   "10 de mar de 2018");
      TEST_DATE ( 1,  4, 2018,      "%Ob de %Y",         "abr de 2018");
      TEST_DATE (11,  5, 2018, "%d de %h de %Y",   "11 de may de 2018");
      TEST_DATE ( 1,  6, 2018,      "%Oh de %Y",         "jun de 2018");
#endif
    }
  else
    g_test_skip ("locale es_ES not available, skipping Spanish month names test");

  setlocale (LC_ALL, "fr_FR.utf-8");
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_FRENCH, SUBLANG_FRENCH), SORT_DEFAULT));
#endif
  if (strstr (setlocale (LC_ALL, NULL), "fr_FR") != NULL)
    {
      TEST_DATE (31,  7, 2018, "%d %B %Y", "31 juillet 2018");
      TEST_DATE ( 1,  8, 2018,   "%OB %Y",       "août 2018");
      TEST_DATE (30,  9, 2018, "%e %b %Y",   "30 sept. 2018");
      TEST_DATE ( 1, 10, 2018,   "%Ob %Y",       "oct. 2018");
      TEST_DATE (29, 11, 2018, "%d %h %Y",    "29 nov. 2018");
      TEST_DATE ( 1, 12, 2018,   "%Oh %Y",       "déc. 2018");
    }
  else
    g_test_skip ("locale fr_FR not available, skipping French month names test");

  /* Make sure that there are visible changes in some European languages.  */
  setlocale (LC_ALL, "el_GR.utf-8");
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_GREEK, SUBLANG_GREEK_GREECE), SORT_DEFAULT));
#endif
  if (strstr (setlocale (LC_ALL, NULL), "el_GR") != NULL)
    {
      TEST_DATE ( 2,  1, 2018, "%d %B %Y",  "02 Ιανουαρίου 2018");
      TEST_DATE ( 4,  2, 2018, "%e %B %Y", " 4 Φεβρουαρίου 2018");
      TEST_DATE (15,  3, 2018, "%d %B %Y",     "15 Μαρτίου 2018");
      TEST_DATE ( 1,  4, 2018,   "%OB %Y",       "Απρίλιος 2018");
      TEST_DATE ( 1,  5, 2018,   "%OB %Y",          "Μάιος 2018");
      TEST_DATE ( 1,  6, 2018,   "%OB %Y",        "Ιούνιος 2018");
      TEST_DATE (16,  7, 2018, "%e %b %Y",        "16 Ιουλ 2018");
#if defined(G_OS_WIN32) && defined(_UCRT)
      TEST_DATE ( 1,  8, 2018,   "%Ob %Y",            "Αυγ 2018");
#else
      TEST_DATE ( 1,  8, 2018,   "%Ob %Y",            "Αύγ 2018");
#endif
    }
  else
    g_test_skip ("locale el_GR not available, skipping Greek month names test");

  setlocale (LC_ALL, "hr_HR.utf-8");
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_CROATIAN, SUBLANG_CROATIAN_CROATIA), SORT_DEFAULT));
#endif
  if (strstr (setlocale (LC_ALL, NULL), "hr_HR") != NULL)
    {
      TEST_DATE ( 8,  5, 2018, "%d. %B %Y", "08. svibnja 2018");
      TEST_DATE ( 9,  6, 2018, "%e. %B %Y",  " 9. lipnja 2018");
      TEST_DATE (10,  7, 2018, "%d. %B %Y",  "10. srpnja 2018");
      TEST_DATE ( 1,  8, 2018,    "%OB %Y",     "Kolovoz 2018");
      TEST_DATE ( 1,  9, 2018,    "%OB %Y",       "Rujan 2018");
      TEST_DATE ( 1, 10, 2018,    "%OB %Y",    "Listopad 2018");
      TEST_DATE (11, 11, 2018, "%e. %b %Y",     "11. Stu 2018");
      TEST_DATE ( 1, 12, 2018,    "%Ob %Y",         "Pro 2018");
    }
  else
    g_test_skip ("locale hr_HR not available, skipping Croatian month names test");

  setlocale (LC_ALL, "lt_LT.utf-8");
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_LITHUANIAN, SUBLANG_LITHUANIAN_LITHUANIA), SORT_DEFAULT));
#endif
  if (strstr (setlocale (LC_ALL, NULL), "lt_LT") != NULL)
    {
      TEST_DATE ( 1,  1, 2018, "%Y m. %B %d d.",  "2018 m. sausio 01 d.");
      TEST_DATE ( 2,  2, 2018, "%Y m. %B %e d.", "2018 m. vasario  2 d.");
      TEST_DATE ( 3,  3, 2018, "%Y m. %B %d d.",    "2018 m. kovo 03 d.");
      TEST_DATE ( 1,  4, 2018,      "%Y m. %OB",      "2018 m. balandis");
      TEST_DATE ( 1,  5, 2018,      "%Y m. %OB",        "2018 m. gegužė");
      TEST_DATE ( 1,  6, 2018,      "%Y m. %OB",      "2018 m. birželis");
      TEST_DATE (17,  7, 2018, "%Y m. %b %e d.",   "2018 m. liep. 17 d.");
      TEST_DATE ( 1,  8, 2018,      "%Y m. %Ob",         "2018 m. rugp.");
    }
  else
    g_test_skip ("locale lt_LT not available, skipping Lithuanian month names test");

  setlocale (LC_ALL, "pl_PL.utf-8");
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_POLISH, SUBLANG_POLISH_POLAND), SORT_DEFAULT));
#endif
  if (strstr (setlocale (LC_ALL, NULL), "pl_PL") != NULL)
    {
      TEST_DATE ( 3,  5, 2018, "%d %B %Y",     "03 maja 2018");
      TEST_DATE ( 4,  6, 2018, "%e %B %Y",  " 4 czerwca 2018");
      TEST_DATE (20,  7, 2018, "%d %B %Y",    "20 lipca 2018");
      TEST_DATE ( 1,  8, 2018,   "%OB %Y",    "sierpień 2018");
      TEST_DATE ( 1,  9, 2018,   "%OB %Y",    "wrzesień 2018");
      TEST_DATE ( 1, 10, 2018,   "%OB %Y", "październik 2018");
      TEST_DATE (25, 11, 2018, "%e %b %Y",      "25 lis 2018");
      TEST_DATE ( 1, 12, 2018,   "%Ob %Y",         "gru 2018");
    }
  else
    g_test_skip ("locale pl_PL not available, skipping Polish month names test");

  setlocale (LC_ALL, "ru_RU.utf-8");
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_RUSSIAN, SUBLANG_RUSSIAN_RUSSIA), SORT_DEFAULT));
#endif
  if (strstr (setlocale (LC_ALL, NULL), "ru_RU") != NULL)
    {
      TEST_DATE ( 3,  1, 2018,      "%d %B %Y",  "03 января 2018");
      TEST_DATE ( 4,  2, 2018,      "%e %B %Y", " 4 февраля 2018");
      TEST_DATE (23,  3, 2018,      "%d %B %Y",   "23 марта 2018");
      TEST_DATE ( 1,  4, 2018,        "%OB %Y",     "Апрель 2018");
      TEST_DATE ( 1,  5, 2018,        "%OB %Y",        "Май 2018");
      TEST_DATE ( 1,  6, 2018,        "%OB %Y",       "Июнь 2018");
      TEST_DATE (24,  7, 2018,      "%e %b %Y",     "24 июл 2018");
      TEST_DATE ( 1,  8, 2018,        "%Ob %Y",        "авг 2018");
      /* This difference is very important in Russian:  */
      TEST_DATE (19,  5, 2018,      "%e %b %Y",     "19 мая 2018");
      TEST_DATE (20,  5, 2018, "%Ob, %d-е, %Y", "май, 20-е, 2018");
    }
  else
    g_test_skip ("locale ru_RU not available, skipping Russian month names test");

  g_date_free (gdate);

  setlocale (LC_ALL, oldlocale);
#ifdef G_OS_WIN32
  SetThreadLocale (old_lcid);
#endif
  g_free (oldlocale);
#endif  /* defined(HAVE_LANGINFO_ABALTMON) || defined(G_OS_WIN32) */
}

static void
test_year (gconstpointer t)
{
  GDateYear y = GPOINTER_TO_INT (t);
  GDateMonth m;
  GDateDay day;
  guint32 j = 0;
  GDate *d;
  gint i;
  GDate tmp;

  guint32 first_day_of_year = G_DATE_BAD_JULIAN;
  guint16 days_in_year = g_date_is_leap_year (y) ? 366 : 365;
  guint   saturday_week_of_year = 0;
  guint   saturday_weeks_in_year = g_date_get_weeks_in_year (y, G_DATE_SATURDAY);
  guint   sunday_week_of_year = 0;
  guint   sunday_weeks_in_year = g_date_get_sunday_weeks_in_year (y);
  guint   monday_week_of_year = 0;
  guint   monday_weeks_in_year = g_date_get_monday_weeks_in_year (y);
  guint   iso8601_week_of_year = 0;

  g_assert_true (g_date_valid_year (y));
  /* Years ought to have roundabout 52 weeks */
  g_assert (saturday_weeks_in_year == 52 || saturday_weeks_in_year == 53);
  g_assert (sunday_weeks_in_year == 52 || sunday_weeks_in_year == 53);
  g_assert (monday_weeks_in_year == 52 || monday_weeks_in_year == 53);

  m = 1;
  while (m < 13)
    {
      guint8 dim = g_date_get_days_in_month (m, y);
      GDate days[31];

      g_date_clear (days, 31);

      g_assert (dim > 0 && dim < 32);
      g_assert_true (g_date_valid_month (m));

      day = 1;
      while (day <= dim)
        {
          g_assert_true (g_date_valid_dmy (day, m, y));

          d = &days[day - 1];
          g_assert_false (g_date_valid (d));

          g_date_set_dmy (d, day, m, y);

          g_assert_true (g_date_valid (d));

          if (m == G_DATE_JANUARY && day == 1)
            first_day_of_year = g_date_get_julian (d);

          g_assert_cmpint (first_day_of_year, !=, G_DATE_BAD_JULIAN);

          g_assert_cmpint (g_date_get_month (d), ==, m);
          g_assert_cmpint (g_date_get_year (d), ==, y);
          g_assert_cmpint (g_date_get_day (d), ==, day);

          g_assert_cmpint (g_date_get_julian (d) + 1 - first_day_of_year,
                           ==,
                           g_date_get_day_of_year (d));

          if (m == G_DATE_DECEMBER && day == 31)
            g_assert_cmpint (g_date_get_day_of_year (d), ==, days_in_year);

          g_assert_cmpint (g_date_get_day_of_year (d), <=, days_in_year);
          g_assert_cmpint (g_date_get_monday_week_of_year (d),
                           <=, monday_weeks_in_year);
          g_assert_cmpint (g_date_get_monday_week_of_year (d),
                           >=, monday_week_of_year);

          if (g_date_get_weekday(d) == G_DATE_MONDAY)
            {
              g_assert_cmpint (g_date_get_monday_week_of_year (d) -
                                   monday_week_of_year,
                               ==, 1);
              if ((m == G_DATE_JANUARY && day <= 4) ||
                  (m == G_DATE_DECEMBER && day >= 29))
                g_assert_cmpint (g_date_get_iso8601_week_of_year (d),
                                 ==, 1);
              else
                g_assert_cmpint (g_date_get_iso8601_week_of_year (d) -
                                     iso8601_week_of_year,
                                 ==, 1);
            }
          else
            {
              g_assert_cmpint (g_date_get_monday_week_of_year (d) -
                                   monday_week_of_year,
                               ==, 0);
              if (!(day == 1 && m == G_DATE_JANUARY))
                g_assert_cmpint (g_date_get_iso8601_week_of_year (d) -
                                     iso8601_week_of_year,
                                 ==, 0);
            }

          monday_week_of_year = g_date_get_monday_week_of_year (d);
          iso8601_week_of_year = g_date_get_iso8601_week_of_year (d);

          g_assert_cmpint (g_date_get_sunday_week_of_year (d),
                           <=, sunday_weeks_in_year);
          g_assert_cmpint (g_date_get_sunday_week_of_year (d),
                           >=, sunday_week_of_year);
          if (g_date_get_weekday(d) == G_DATE_SUNDAY)
            g_assert_cmpint (g_date_get_sunday_week_of_year (d) -
                                 sunday_week_of_year,
                             ==, 1);
          else
            g_assert_cmpint (g_date_get_sunday_week_of_year (d) -
                                 sunday_week_of_year,
                             ==, 0);

          sunday_week_of_year = g_date_get_sunday_week_of_year (d);

          g_assert_cmpint (g_date_get_week_of_year (d, G_DATE_SATURDAY),
                           <=, saturday_weeks_in_year);
          g_assert_cmpint (g_date_get_week_of_year (d, G_DATE_SATURDAY),
                           >=, saturday_week_of_year);
          if (g_date_get_weekday (d) == G_DATE_SATURDAY)
            g_assert_cmpint (g_date_get_week_of_year (d, G_DATE_SATURDAY) -
                                 saturday_week_of_year,
                             ==, 1);
          else
            g_assert_cmpint (g_date_get_week_of_year (d, G_DATE_SATURDAY) -
                                 saturday_week_of_year,
                             ==, 0);

          saturday_week_of_year = g_date_get_week_of_year (d, G_DATE_SATURDAY);

          g_assert_cmpint (g_date_compare (d, d), ==, 0);

          i = 1;
          while (i < 402) /* Need to get 400 year increments in */
            {
              tmp = *d;
              g_date_add_days (d, i);
              g_assert_cmpint (g_date_compare (d, &tmp), >, 0);
              g_date_subtract_days (d, i);
              g_assert_cmpint (g_date_get_day (d), ==, day);
              g_assert_cmpint (g_date_get_month (d), ==, m);
              g_assert_cmpint (g_date_get_year (d), ==, y);

              tmp = *d;
              g_date_add_months (d, i);
              g_assert_cmpint (g_date_compare (d, &tmp), >, 0);
              g_date_subtract_months (d, i);
              g_assert_cmpint (g_date_get_month (d), ==, m);
              g_assert_cmpint (g_date_get_year (d), ==, y);

              if (day < 29)
                g_assert_cmpint (g_date_get_day (d), ==, day);
              else
                g_date_set_day (d, day);

              tmp = *d;
              g_date_add_years (d, i);
              g_assert_cmpint (g_date_compare (d, &tmp), >, 0);
              g_date_subtract_years (d, i);
              g_assert_cmpint (g_date_get_month (d), ==, m);
              g_assert_cmpint (g_date_get_year (d), ==, y);

              if (m != 2 && day != 29)
                g_assert_cmpint (g_date_get_day (d), ==, day);
              else
                g_date_set_day (d, day); /* reset */

              i += 10;
            }

          j = g_date_get_julian (d);

          ++day;
        }
      ++m;
   }

  /* at this point, d is the last day of year y */
  g_date_set_dmy (&tmp, 1, 1, y + 1);
  g_assert_cmpint (j + 1, ==, g_date_get_julian (&tmp));

  g_date_add_days (&tmp, 1);
  g_assert_cmpint (j + 2, ==, g_date_get_julian (&tmp));
}

static void
test_clamp (void)
{
  GDate *d, *d1, *d2, *o;

  d = g_date_new ();
  d1 = g_date_new ();
  d2 = g_date_new ();
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_clamp (d, d1, d2);
      g_test_assert_expected_messages ();
    }

  g_date_set_dmy (d, 1, 1, 1);
  g_date_clamp (d, NULL, NULL);
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_clamp (d, d1, NULL);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_clamp (d, d1, d2);
      g_test_assert_expected_messages ();
    }

  g_date_set_dmy (d1, 1, 1, 1970);
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_clamp (d, d1, d2);
      g_test_assert_expected_messages ();

      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_clamp (d, NULL, d2);
      g_test_assert_expected_messages ();
    }

  g_date_set_dmy (d2, 1, 1, 1980);
  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_clamp (d, d2, d1);
      g_test_assert_expected_messages ();
    }

  o = g_date_copy (d);
  g_date_clamp (o, NULL, NULL);
  g_assert_cmpint (g_date_compare (o, d), ==, 0);

  g_date_clamp (o,  d1, d2);
  g_assert_cmpint (g_date_compare (o, d1), ==, 0);

  g_date_set_dmy (o, 1, 1, 2000);

  g_date_clamp (o,  d1, d2);
  g_assert_cmpint (g_date_compare (o, d2), ==, 0);

  g_date_free (d);
  g_date_free (d1);
  g_date_free (d2);
  g_date_free (o);
}

static void
test_order (void)
{
  GDate *d1, *d2;

  d1 = g_date_new ();
  d2 = g_date_new ();

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_order (d1, d2);
      g_test_assert_expected_messages ();
    }

  g_date_set_dmy (d1, 1, 1, 1970);

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_date_order (d1, d2);
      g_test_assert_expected_messages ();
    }

  g_date_set_dmy (d2, 1, 1, 1980);

  g_date_order (d1, d2);
  g_assert_cmpint (g_date_compare (d1, d2), ==, -1);
  g_date_order (d2, d1);
  g_assert_cmpint (g_date_compare (d1, d2), ==, 1);

  g_date_free (d1);
  g_date_free (d2);
}

static void
test_copy (void)
{
  GDate *d;
  GDate *c;

  if (g_test_undefined ())
    {
      g_test_expect_message (G_LOG_DOMAIN, G_LOG_LEVEL_CRITICAL,
                             "*assertion *failed*");
      g_assert_null (g_date_copy (NULL));
      g_test_assert_expected_messages ();
    }

  d = g_date_new ();
  g_assert_false (g_date_valid (d));

  c = g_date_copy (d);
  g_assert_nonnull (c);
  g_assert_false (g_date_valid (c));
  g_date_free (c);

  g_date_set_day (d, 10);

  c = g_date_copy (d);
  g_date_set_month (c, 1);
  g_date_set_year (c, 2015);
  g_assert_true (g_date_valid (c));
  g_assert_cmpuint (g_date_get_day (c), ==, 10);
  g_date_free (c);

  g_date_free (d);
}

/* Check the results of g_date_valid_dmy() for various inputs. */
static void
test_valid_dmy (void)
{
  const struct
    {
      GDateDay day;
      GDateMonth month;
      GDateYear year;
      gboolean expected_valid;
    }
  vectors[] =
    {
      /* Lower bounds */
      { 0, 0, 0, FALSE },
      { 1, 1, 1, TRUE },
      { 1, 1, 0, FALSE },
      /* Leap year month lengths */
      { 30, 2, 2000, FALSE },
      { 29, 2, 2000, TRUE },
      { 29, 2, 2001, FALSE },
      /* Maximum year */
      { 1, 1, G_MAXUINT16, TRUE },
    };
  gsize i;

  for (i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      gboolean valid;
      g_test_message ("Vector %" G_GSIZE_FORMAT ": %04u-%02u-%02u, %s",
                      i, vectors[i].year, vectors[i].month, vectors[i].day,
                      vectors[i].expected_valid ? "valid" : "invalid");

      valid = g_date_valid_dmy (vectors[i].day,
                                vectors[i].month,
                                vectors[i].year);

      if (vectors[i].expected_valid)
        g_assert_true (valid);
      else
        g_assert_false (valid);
    }
}

static void
test_week_of_year (void)
{
  const struct
    {
      unsigned int year;
      unsigned int month;
      unsigned int day;
      unsigned int expected_saturday_week_of_year;
      unsigned int expected_sunday_week_of_year;
      unsigned int expected_monday_week_of_year;
    }
  vectors[] =
    {
      { 2000, 1, 1, 1, 0, 0 },  /* first day of the year is a Saturday */
      { 2000, 1, 7, 1, 1, 1 },
      { 2000, 1, 8, 2, 1, 1 },
      { 2001, 1, 1, 0, 0, 1 },  /* first day of the year is a Monday */
      { 2001, 1, 7, 1, 1, 1 },
      { 2001, 1, 8, 1, 1, 2 },
      { 2002, 1, 1, 0, 0, 0 },  /* first day of the year is a Tuesday */
      { 2002, 1, 7, 1, 1, 1 },
      { 2002, 1, 8, 1, 1, 1 },
      { 2003, 1, 1, 0, 0, 0 },  /* first day of the year is a Wednesday */
      { 2003, 1, 7, 1, 1, 1 },
      { 2003, 1, 8, 1, 1, 1 },
      { 2004, 1, 1, 0, 0, 0 },  /* first day of the year is a Thursday */
      { 2004, 1, 7, 1, 1, 1 },
      { 2004, 1, 8, 1, 1, 1 },
      { 2006, 1, 1, 0, 1, 0 },  /* first day of the year is a Sunday */
      { 2006, 1, 7, 1, 1, 1 },
      { 2006, 1, 8, 1, 2, 1 },
      { 2010, 1, 1, 0, 0, 0 },  /* first day of the year is a Friday */
      { 2010, 1, 7, 1, 1, 1 },
      { 2010, 1, 8, 1, 1, 1 },
    };

  for (size_t i = 0; i < G_N_ELEMENTS (vectors); i++)
    {
      GDate date;

      g_date_clear (&date, 1);
      g_date_set_dmy (&date, vectors[i].day, vectors[i].month, vectors[i].year);
      g_test_message ("Considering %04u-%02u-%02u",
                      vectors[i].year, vectors[i].month, vectors[i].day);

      g_assert_cmpuint (g_date_get_week_of_year (&date, G_DATE_SATURDAY), ==, vectors[i].expected_saturday_week_of_year);
      g_assert_cmpuint (g_date_get_week_of_year (&date, G_DATE_SUNDAY), ==, vectors[i].expected_sunday_week_of_year);
      g_assert_cmpuint (g_date_get_week_of_year (&date, G_DATE_MONDAY), ==, vectors[i].expected_monday_week_of_year);
      g_assert_cmpuint (g_date_get_sunday_week_of_year (&date), ==, vectors[i].expected_sunday_week_of_year);
      g_assert_cmpuint (g_date_get_monday_week_of_year (&date), ==, vectors[i].expected_monday_week_of_year);
    }
}

int
main (int argc, char** argv)
{
  gchar *path;
  gsize i;

  /* Try to get all the leap year cases. */
  int check_years[] = {
    1, 2, 3, 4, 5, 6, 7, 8, 9, 10,
    11, 12, 13, 14, 98, 99, 100, 101, 102, 103, 397,
    398, 399, 400, 401, 402, 403, 404, 405, 406,
    1598, 1599, 1600, 1601, 1602, 1650, 1651,
    1897, 1898, 1899, 1900, 1901, 1902, 1903,
    1961, 1962, 1963, 1964, 1965, 1967,
    1968, 1969, 1970, 1971, 1972, 1973, 1974, 1975, 1976,
    1977, 1978, 1979, 1980, 1981, 1982, 1983, 1984, 1985,
    1986, 1987, 1988, 1989, 1990, 1991, 1992, 1993, 1994,
    1995, 1996, 1997, 1998, 1999, 2000, 2001, 2002, 2003,
    2004, 2005, 2006, 2007, 2008, 2009, 2010, 2011, 2012,
    3000, 3001, 3002, 3998, 3999, 4000, 4001, 4002, 4003
  };

  g_setenv ("LC_ALL", "en_US.utf-8", TRUE);
  setlocale (LC_ALL, "");
#ifdef G_OS_WIN32
  SetThreadLocale (MAKELCID (MAKELANGID (LANG_ENGLISH, SUBLANG_ENGLISH_US), SORT_DEFAULT));
#endif

  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/date/basic", test_basic);
  g_test_add_func ("/date/empty", test_empty_constructor);
  g_test_add_func ("/date/dmy", test_dmy_constructor);
  g_test_add_func ("/date/julian", test_julian_constructor);
  g_test_add_func ("/date/compare", test_date_compare);
  g_test_add_func ("/date/dates", test_dates);
  g_test_add_func ("/date/strftime", test_strftime);
  g_test_add_func ("/date/two-digit-years", test_two_digit_years);
  g_test_add_func ("/date/parse", test_parse);
  g_test_add_func ("/date/parse/invalid", test_parse_invalid);
  g_test_add_func ("/date/parse_locale_change", test_parse_locale_change);
  g_test_add_func ("/date/month_substring", test_month_substring);
  g_test_add_func ("/date/month_names", test_month_names);
  g_test_add_func ("/date/clamp", test_clamp);
  g_test_add_func ("/date/order", test_order);
  for (i = 0; i < G_N_ELEMENTS (check_years); i++)
    {
      path = g_strdup_printf ("/date/year/%d", check_years[i]);
      g_test_add_data_func (path, GINT_TO_POINTER(check_years[i]), test_year);
      g_free (path);
    }
  g_test_add_func ("/date/copy", test_copy);
  g_test_add_func ("/date/valid-dmy", test_valid_dmy);
  g_test_add_func ("/date/week-of-year", test_week_of_year);

  return g_test_run ();
}
