#include <glib-object.h>

typedef struct {
  GObject instance;
} MyObj;

typedef struct {
  GObjectClass parent_class;
} MyObjClass;

enum {
  SIGNAL1,
  SIGNAL2,
  LAST_SIGNAL
};

guint signals[LAST_SIGNAL];

GType my_obj_get_type (void);

G_DEFINE_TYPE (MyObj, my_obj, G_TYPE_OBJECT)

static void
my_obj_init (MyObj *o)
{
}

static void
my_obj_class_init (MyObjClass *class)
{
  signals[SIGNAL1] =
    g_signal_new ("signal1",
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE, 0);
  signals[SIGNAL2] =
    g_signal_new ("signal2",
                  G_TYPE_FROM_CLASS (class),
                  G_SIGNAL_RUN_LAST,
                  0, NULL, NULL, NULL, G_TYPE_NONE, 0);
}

static void
nop (void)
{
}

static guint
choose_n_handlers (void)
{
  return g_test_perf () ? 500000 : 1;
}

static void
test_connect_many (void)
{
  MyObj *o;
  gdouble time_elapsed;
  guint i;
  const guint n_handlers = choose_n_handlers ();

  o = g_object_new (my_obj_get_type (), NULL);

  g_test_timer_start ();

  for (i = 0; i < n_handlers; i++)
    g_signal_connect (o, "signal1", G_CALLBACK (nop), NULL); 

  time_elapsed = g_test_timer_elapsed ();

  g_object_unref (o);

  g_test_minimized_result (time_elapsed, "connected %u handlers in %6.3f seconds", n_handlers, time_elapsed);
}

static void
test_disconnect_many_ordered (void)
{
  MyObj *o;
  gulong *handlers;
  gdouble time_elapsed;
  guint i;
  const guint n_handlers = choose_n_handlers ();

  handlers = g_malloc_n (n_handlers, sizeof (*handlers));
  o = g_object_new (my_obj_get_type (), NULL);

  for (i = 0; i < n_handlers; i++)
    handlers[i] = g_signal_connect (o, "signal1", G_CALLBACK (nop), NULL); 

  g_test_timer_start ();

  for (i = 0; i < n_handlers; i++)
    g_signal_handler_disconnect (o, handlers[i]); 

  time_elapsed = g_test_timer_elapsed ();

  g_object_unref (o);
  g_free (handlers);

  g_test_minimized_result (time_elapsed, "disconnected %u handlers in %6.3f seconds", n_handlers, time_elapsed);
}

static void
test_disconnect_many_inverse (void)
{
  MyObj *o;
  gulong *handlers;
  gdouble time_elapsed;
  guint i;
  const guint n_handlers = choose_n_handlers ();

  handlers = g_malloc_n (n_handlers, sizeof (*handlers));
  o = g_object_new (my_obj_get_type (), NULL);

  for (i = 0; i < n_handlers; i++)
    handlers[i] = g_signal_connect (o, "signal1", G_CALLBACK (nop), NULL); 

  g_test_timer_start ();

  for (i = n_handlers; i > 0; i--)
    g_signal_handler_disconnect (o, handlers[i - 1]);

  time_elapsed = g_test_timer_elapsed ();

  g_object_unref (o);
  g_free (handlers);

  g_test_minimized_result (time_elapsed, "disconnected %u handlers in %6.3f seconds", n_handlers, time_elapsed);
}

static void
test_disconnect_many_random (void)
{
  MyObj *o;
  gulong *handlers;
  gulong id;
  gdouble time_elapsed;
  guint i, j;
  const guint n_handlers = choose_n_handlers ();

  handlers = g_malloc_n (n_handlers, sizeof (*handlers));
  o = g_object_new (my_obj_get_type (), NULL);

  for (i = 0; i < n_handlers; i++)
    handlers[i] = g_signal_connect (o, "signal1", G_CALLBACK (nop), NULL); 

  for (i = 0; i < n_handlers; i++)
    {
      j = (guint) g_test_rand_int_range (0, (int) n_handlers);
      id = handlers[i];
      handlers[i] = handlers[j];
      handlers[j] = id;
    }

  g_test_timer_start ();

  for (i = 0; i < n_handlers; i++)
    g_signal_handler_disconnect (o, handlers[i]); 

  time_elapsed = g_test_timer_elapsed ();

  g_object_unref (o);
  g_free (handlers);

  g_test_minimized_result (time_elapsed, "disconnected %u handlers in %6.3f seconds", n_handlers, time_elapsed);
}

static void
test_disconnect_2_signals (void)
{
  MyObj *o;
  gulong *handlers;
  gulong id;
  gdouble time_elapsed;
  guint i, j;
  const guint n_handlers = choose_n_handlers ();

  handlers = g_malloc_n (n_handlers, sizeof (*handlers));
  o = g_object_new (my_obj_get_type (), NULL);

  for (i = 0; i < n_handlers; i++)
    {
      if (i % 2 == 0)
        handlers[i] = g_signal_connect (o, "signal1", G_CALLBACK (nop), NULL); 
      else
        handlers[i] = g_signal_connect (o, "signal2", G_CALLBACK (nop), NULL); 
    }

  for (i = 0; i < n_handlers; i++)
    {
      j = (guint) g_test_rand_int_range (0, (int) n_handlers);
      id = handlers[i];
      handlers[i] = handlers[j];
      handlers[j] = id;
    }

  g_test_timer_start ();

  for (i = 0; i < n_handlers; i++)
    g_signal_handler_disconnect (o, handlers[i]); 

  time_elapsed = g_test_timer_elapsed ();

  g_object_unref (o);
  g_free (handlers);

  g_test_minimized_result (time_elapsed, "disconnected %u handlers in %6.3f seconds", n_handlers, time_elapsed);
}

static void
test_disconnect_2_objects (void)
{
  MyObj *o1, *o2, *o;
  gulong *handlers;
  MyObj **objects;
  gulong id;
  gdouble time_elapsed;
  guint i, j;
  const guint n_handlers = choose_n_handlers ();

  handlers = g_malloc_n (n_handlers, sizeof (*handlers));
  objects = g_malloc_n (n_handlers, sizeof (*objects));
  o1 = g_object_new (my_obj_get_type (), NULL);
  o2 = g_object_new (my_obj_get_type (), NULL);

  for (i = 0; i < n_handlers; i++)
    {
      if (i % 2 == 0)
        {
          handlers[i] = g_signal_connect (o1, "signal1", G_CALLBACK (nop), NULL); 
          objects[i] = o1;
        }
      else
        {
          handlers[i] = g_signal_connect (o2, "signal1", G_CALLBACK (nop), NULL); 
          objects[i] = o2;
        }
    }

  for (i = 0; i < n_handlers; i++)
    {
      j = (guint) g_test_rand_int_range (0, (int) n_handlers);
      id = handlers[i];
      handlers[i] = handlers[j];
      handlers[j] = id;
      o = objects[i];
      objects[i] = objects[j];
      objects[j] = o;
    }

  g_test_timer_start ();

  for (i = 0; i < n_handlers; i++)
    g_signal_handler_disconnect (objects[i], handlers[i]); 

  time_elapsed = g_test_timer_elapsed ();

  g_object_unref (o1);
  g_object_unref (o2);
  g_free (objects);
  g_free (handlers);

  g_test_minimized_result (time_elapsed, "disconnected %u handlers in %6.3f seconds", n_handlers, time_elapsed);
}

static void
test_block_many (void)
{
  MyObj *o;
  gulong *handlers;
  gulong id;
  gdouble time_elapsed;
  guint i, j;
  const guint n_handlers = choose_n_handlers ();

  handlers = g_malloc_n (n_handlers, sizeof (*handlers));
  o = g_object_new (my_obj_get_type (), NULL);

  for (i = 0; i < n_handlers; i++)
    handlers[i] = g_signal_connect (o, "signal1", G_CALLBACK (nop), NULL); 

  for (i = 0; i < n_handlers; i++)
    {
      j = (guint) g_test_rand_int_range (0, (int) n_handlers);
      id = handlers[i];
      handlers[i] = handlers[j];
      handlers[j] = id;
    }

  g_test_timer_start ();

  for (i = 0; i < n_handlers; i++)
    g_signal_handler_block (o, handlers[i]); 

  for (i = n_handlers; i > 0; i--)
    g_signal_handler_unblock (o, handlers[i - 1]);

  time_elapsed = g_test_timer_elapsed ();

  g_object_unref (o);
  g_free (handlers);

  g_test_minimized_result (time_elapsed, "blocked and unblocked %u handlers in %6.3f seconds", n_handlers, time_elapsed);
}

int
main (int argc, char *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/signal/handler/connect-many", test_connect_many);
  g_test_add_func ("/signal/handler/disconnect-many-ordered", test_disconnect_many_ordered);
  g_test_add_func ("/signal/handler/disconnect-many-inverse", test_disconnect_many_inverse);
  g_test_add_func ("/signal/handler/disconnect-many-random", test_disconnect_many_random);
  g_test_add_func ("/signal/handler/disconnect-2-signals", test_disconnect_2_signals);
  g_test_add_func ("/signal/handler/disconnect-2-objects", test_disconnect_2_objects);
  g_test_add_func ("/signal/handler/block-many", test_block_many);

  return g_test_run ();
}
