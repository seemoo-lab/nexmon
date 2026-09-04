#include <glib.h>

#include "gtestutils.h"

static void
test_bitlocks (void)
{
  guint64 start = g_get_monotonic_time ();
  gint lock = 0;
  guint i;
  guint n_iterations;

  n_iterations = g_test_perf () ? 100000000 : 1;

  for (i = 0; i < n_iterations; i++)
    {
      g_bit_lock (&lock, 0);
      g_bit_unlock (&lock, 0);
    }

  {
    gdouble elapsed;
    gdouble rate;

    elapsed = g_get_monotonic_time () - start;
    elapsed /= 1000000;
    rate = n_iterations / elapsed;

    g_test_maximized_result (rate, "iterations per second");
  }
}

#define PARALLEL_N_THREADS 5
#define PARALLEL_LOCKBIT 31
#define PARALLEL_TOGGLEBIT 30
#define PARALLEL_SETBIT 29
#define PARALLEL_LOCKMASK (1U << PARALLEL_LOCKBIT)
#define PARALLEL_TOGGLEMASK (1U << PARALLEL_TOGGLEBIT)
#define PARALLEL_SETMASK (1U << PARALLEL_SETBIT)
#define PARALLEL_MAX_COUNT_SELF 500
#define PARALLEL_MAX_COUNT_ALL (10 * PARALLEL_MAX_COUNT_SELF)
static int parallel_thread_ready = 0;
static int parallel_atomic = 0;

static void
_test_parallel_randomly_toggle (void)
{
  if (g_random_boolean ())
    g_atomic_int_or (&parallel_atomic, PARALLEL_TOGGLEMASK);
  else
    g_atomic_int_and (&parallel_atomic, ~PARALLEL_TOGGLEMASK);
}

static gpointer
_test_parallel_run (gpointer thread_arg)
{
  const int IDX = GPOINTER_TO_INT (thread_arg);
  int count_self = 0;

  (void) IDX;

  g_atomic_int_inc (&parallel_thread_ready);
  while (g_atomic_int_get (&parallel_thread_ready) != PARALLEL_N_THREADS)
    g_usleep (10);

  while (TRUE)
    {
      gint val;
      gint val2;
      gint new_val;
      gint count_all;

      _test_parallel_randomly_toggle ();

      /* take a lock. */
      if (g_random_boolean ())
        {
          g_bit_lock (&parallel_atomic, PARALLEL_LOCKBIT);
          val = g_atomic_int_get (&parallel_atomic);
        }
      else
        {
          g_bit_lock_and_get (&parallel_atomic, PARALLEL_LOCKBIT, &val);
        }

      _test_parallel_randomly_toggle ();

      /* the toggle bit is random. Clear it. */
      val &= ~PARALLEL_TOGGLEMASK;

      /* these bits must be set. */
      g_assert_true (val & PARALLEL_LOCKMASK);
      g_assert_true (val & PARALLEL_SETMASK);

      /* If we fetch again, we must get the same value. Nobody changes the
       * value while we hold the lock, except for the toggle bit. */
      val2 = g_atomic_int_get (&parallel_atomic);
      val2 &= ~PARALLEL_TOGGLEMASK;
      g_assert_cmpint (val, ==, val2);

      count_all = (val & ~(PARALLEL_LOCKMASK | PARALLEL_SETMASK));

      if ((g_random_int () % 5) == 0)
        {
          /* regular unlock without any changes. */
          g_bit_unlock (&parallel_atomic, PARALLEL_LOCKBIT);
          continue;
        }

      /* unlock-and-set with an increased counter. */
      new_val = MIN (count_all + 1, PARALLEL_MAX_COUNT_ALL);
      if (g_random_boolean ())
        new_val |= PARALLEL_SETMASK;
      if (g_random_boolean ())
        new_val |= PARALLEL_TOGGLEMASK;

      g_bit_unlock_and_set (&parallel_atomic, PARALLEL_LOCKBIT, new_val, ((new_val & PARALLEL_SETMASK) && g_random_boolean ()) ? 0 : PARALLEL_SETMASK);

      count_self++;

      if (count_self < PARALLEL_MAX_COUNT_SELF)
        continue;
      if (count_all < PARALLEL_MAX_COUNT_ALL)
        continue;

      break;
    }

  /* To indicate success, we return a pointer to &parallel_atomic. */
  return &parallel_atomic;
}

static void
test_parallel (void)
{
  GThread *threads[PARALLEL_N_THREADS];
  gpointer ptr;
  int i;
  gint val;

  g_atomic_int_or (&parallel_atomic, PARALLEL_SETMASK);

  for (i = 0; i < PARALLEL_N_THREADS; i++)
    {
      threads[i] = g_thread_new ("bitlock-parallel", _test_parallel_run, GINT_TO_POINTER (i));
    }

  for (i = 0; i < PARALLEL_N_THREADS; i++)
    {
      ptr = g_thread_join (threads[i]);
      g_assert_true (ptr == &parallel_atomic);

      /* After we join the first thread, we already expect that the resulting
       * atomic's counter is set to PARALLEL_MAX_COUNT_ALL. This stays until
       * the end. */
      val = g_atomic_int_get (&parallel_atomic);
      if (i == PARALLEL_N_THREADS - 1)
        {
          /* at last, the atomic must be unlocked. */
          g_assert_true (!(val & PARALLEL_LOCKMASK));
        }
      val &= ~(PARALLEL_LOCKMASK | PARALLEL_TOGGLEMASK | PARALLEL_SETMASK);
      g_assert_cmpint (val, ==, PARALLEL_MAX_COUNT_ALL);
    }
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/bitlock/performance/uncontended", test_bitlocks);
  g_test_add_func ("/bitlock/performance/parallel", test_parallel);

  return g_test_run ();
}
