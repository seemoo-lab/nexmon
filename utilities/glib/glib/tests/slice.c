#include <string.h>
#include <glib.h>

/* We test deprecated functionality here */
G_GNUC_BEGIN_IGNORE_DEPRECATIONS

static void
test_slice_copy (void)
{
  const gchar *block = "0123456789ABCDEF";
  gpointer p;

  p = g_slice_copy (12, block);
  g_assert (memcmp (p, block, 12) == 0);
  g_slice_free1 (12, p);
}

typedef struct {
  gint int1;
  gint int2;
  gchar byte;
  gpointer next;
  gint64 more;
} TestStruct;

static void
test_chain (void)
{
  TestStruct *ts, *head;

  head = ts = g_slice_new (TestStruct);
  ts->next = g_slice_new (TestStruct);
  ts = ts->next;
  ts->next = g_slice_new (TestStruct);
  ts = ts->next;
  ts->next = NULL;

  g_slice_free_chain (TestStruct, head, next);
}

static gpointer chunks[4096][30];

static gpointer
thread_allocate (gpointer data)
{
  gint i;
  gint b;
  gint size;
  gpointer p;
  gpointer *loc;  /* (atomic) */

  for (i = 0; i < 10000; i++)
    {
      b = g_random_int_range (0, 30);
      size = g_random_int_range (0, 4096);
      loc = &(chunks[size][b]);

      p = g_atomic_pointer_get (loc);
      if (p == NULL)
        {
          p = g_slice_alloc (size + 1);
          if (!g_atomic_pointer_compare_and_exchange (loc, NULL, p))
            g_slice_free1 (size + 1, p);
        }
      else
        {
          if (g_atomic_pointer_compare_and_exchange (loc, p, NULL))
            g_slice_free1 (size + 1, p);
        }
    }

  return NULL;
}

static void
test_allocate (void)
{
  GThread *threads[30];
  gint size;
  gsize i;

  for (i = 0; i < 30; i++)
    for (size = 1; size <= 4096; size++)
      chunks[size - 1][i] = NULL;

  for (i = 0; i < G_N_ELEMENTS(threads); i++)
    threads[i] = g_thread_create (thread_allocate, NULL, TRUE, NULL);

  for (i = 0; i < G_N_ELEMENTS(threads); i++)
    g_thread_join (threads[i]);
}

static void
test_new_empty (void)
{
#if !defined(_MSC_VER) || defined(__clang__)
  typedef struct {
    /* empty */
  } EmptyStruct;
  EmptyStruct *p1, *p2, *p3;

  g_test_summary ("Test that g_slice_new() never returns NULL, even with a zero-size allocation");
  g_test_bug ("https://gitlab.gnome.org/GNOME/glib/-/issues/3688");

  if (sizeof (EmptyStruct) > 0)
    {
      g_test_skip ("Compiler doesn’t support empty structs");
      return;
    }

  p1 = g_slice_new (EmptyStruct);
  g_assert_nonnull (p1);

  p2 = g_slice_new0 (EmptyStruct);
  g_assert_nonnull (p2);

  p3 = g_slice_dup (EmptyStruct, p1);
  g_assert_nonnull (p3);

  g_free (p3);
  g_free (p2);
  g_free (p1);
#else
  g_test_skip ("MSVC requires that structs are non-empty");
#endif
}

int
main (int argc, char **argv)
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/slice/copy", test_slice_copy);
  g_test_add_func ("/slice/chain", test_chain);
  g_test_add_func ("/slice/allocate", test_allocate);
  g_test_add_func ("/slice/new-empty", test_new_empty);

  return g_test_run ();
}
