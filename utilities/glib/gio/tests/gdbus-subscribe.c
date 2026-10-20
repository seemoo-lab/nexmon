/*
 * Copyright 2024 Collabora Ltd.
 * SPDX-License-Identifier: LGPL-2.1-or-later
 */

#include <gio/gio.h>

#include "gdbusprivate.h"
#include "gdbus-tests.h"

#define NAME_OWNER_CHANGED "NameOwnerChanged"

/* A signal that each connection emits to indicate that it has finished
 * emitting other signals */
#define FINISHED_PATH "/org/gtk/Test/Finished"
#define FINISHED_INTERFACE "org.gtk.Test.Finished"
#define FINISHED_SIGNAL "Finished"

/* A signal emitted during testing */
#define EXAMPLE_PATH "/org/gtk/GDBus/ExampleInterface"
#define EXAMPLE_INTERFACE "org.gtk.GDBus.ExampleInterface"
#define FOO_SIGNAL "Foo"

#define ALREADY_OWNED_NAME "org.gtk.Test.AlreadyOwned"
#define OWNED_LATER_NAME "org.gtk.Test.OwnedLater"

/* Log @s in a debug message. */
static inline const char *
nonnull (const char *s,
         const char *if_null)
{
  return (s == NULL) ? if_null : s;
}

typedef enum
{
  TEST_CONN_NONE,
  TEST_CONN_FIRST,
  /* A connection that subscribes to signals */
  TEST_CONN_SUBSCRIBER = TEST_CONN_FIRST,
  /* A mockup of a legitimate service */
  TEST_CONN_SERVICE,
  /* A mockup of a second legitimate service */
  TEST_CONN_SERVICE2,
  /* A connection that tries to trick @subscriber into processing its signals
   * as if they came from @service */
  TEST_CONN_ATTACKER,
  NUM_TEST_CONNS
} TestConn;

static const char * const test_conn_descriptions[NUM_TEST_CONNS] =
{
  "(unused)",
  "subscriber",
  "service",
  "service 2",
  "attacker"
};

typedef enum
{
  SUBSCRIPTION_MODE_CONN,
  SUBSCRIPTION_MODE_PROXY,
  SUBSCRIPTION_MODE_PARALLEL
} SubscriptionMode;

typedef struct
{
  GDBusProxy *received_by_proxy;
  TestConn sender;
  char *path;
  char *iface;
  char *member;
  GVariant *parameters;
  char *arg0;
  guint32 step;
} ReceivedMessage;

static void
received_message_free (ReceivedMessage *self)
{

  g_clear_object (&self->received_by_proxy);
  g_free (self->path);
  g_free (self->iface);
  g_free (self->member);
  g_clear_pointer (&self->parameters, g_variant_unref);
  g_free (self->arg0);
  g_free (self);
}

typedef struct
{
  TestConn sender;
  TestConn unicast_to;
  const char *path;
  const char *iface;
  const char *member;
  const char *arg0;
  const char *args;
  guint received_by_conn;
  guint received_by_proxy;
} TestEmitSignal;

typedef struct
{
  const char *string_sender;
  TestConn unique_sender;
  const char *path;
  const char *iface;
  const char *member;
  const char *arg0;
  GDBusSignalFlags flags;
  gboolean unsubscribe_immediately;
} TestSubscribe;

typedef struct
{
  const char *name;
  TestConn owner;
  guint received_by_conn;
  guint received_by_proxy;
} TestOwnName;

typedef enum
{
  TEST_ACTION_NONE = 0,
  TEST_ACTION_SUBSCRIBE,
  TEST_ACTION_EMIT_SIGNAL,
  TEST_ACTION_OWN_NAME,
} TestAction;

typedef struct
{
  TestAction action;
  union {
    TestEmitSignal signal;
    TestSubscribe subscribe;
    TestOwnName own_name;
    guint unsubscribe_undo_step;
  } u;
} TestStep;

/* Arbitrary, extend as necessary to accommodate the longest test */
#define MAX_TEST_STEPS 10

typedef struct
{
  const char *description;
  TestStep steps[MAX_TEST_STEPS];
} TestPlan;

static const TestPlan plan_simple =
{
  .description = "A broadcast is only received after subscribing to it",
  .steps = {
    {
      /* We don't receive a signal if we haven't subscribed yet */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_SERVICE,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 0,
        .received_by_proxy = 0
      },
    },
    {
      .action = TEST_ACTION_SUBSCRIBE,
      .u.subscribe = {
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
      },
    },
    {
      /* Now it works */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_SERVICE,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 1,
        /* The proxy can't be used in this case, because it needs
         * a bus name to subscribe to */
        .received_by_proxy = 0
      },
    },
  },
};

static const TestPlan plan_broadcast_from_anyone =
{
  .description = "A subscription with NULL sender accepts broadcast and unicast",
  .steps = {
    {
      /* Subscriber wants to receive signals from anyone */
      .action = TEST_ACTION_SUBSCRIBE,
      .u.subscribe = {
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
      },
    },
    {
      /* First service sends a broadcast */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_SERVICE,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 1,
        .received_by_proxy = 0
      },
    },
    {
      /* Second service also sends a broadcast */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_SERVICE2,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 1,
        .received_by_proxy = 0
      },
    },
    {
      /* First service sends a unicast signal */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_SERVICE,
        .unicast_to = TEST_CONN_SUBSCRIBER,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 1,
        .received_by_proxy = 0
      },
    },
    {
      /* Second service also sends a unicast signal */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_SERVICE2,
        .unicast_to = TEST_CONN_SUBSCRIBER,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 1,
        .received_by_proxy = 0
      },
    },
  },
};

static const TestPlan plan_match_twice =
{
  .description = "A message matching more than one subscription is received "
                 "once per subscription",
  .steps = {
    {
      .action = TEST_ACTION_SUBSCRIBE,
      .u.subscribe = {
        .unique_sender = TEST_CONN_SERVICE,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
      },
    },
    {
      .action = TEST_ACTION_SUBSCRIBE,
      .u.subscribe = {
        .path = EXAMPLE_PATH,
      },
    },
    {
      .action = TEST_ACTION_SUBSCRIBE,
      .u.subscribe = {
        .iface = EXAMPLE_INTERFACE,
      },
    },
    {
      .action = TEST_ACTION_SUBSCRIBE,
      .u.subscribe = {
        .unique_sender = TEST_CONN_SERVICE,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
      },
    },
    {
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_SERVICE,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 4,
        /* Only the first and last work with GDBusProxy */
        .received_by_proxy = 2
      },
    },
  },
};

static const TestPlan plan_limit_by_unique_name =
{
  .description = "A subscription via a unique name only accepts messages "
                 "sent by that same unique name",
  .steps = {
    {
      /* Subscriber wants to receive signals from service */
      .action = TEST_ACTION_SUBSCRIBE,
      .u.subscribe = {
        .unique_sender = TEST_CONN_SERVICE,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
      },
    },
    {
      /* Attacker wants to trick subscriber into thinking that service
       * sent a signal */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_ATTACKER,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 0,
        .received_by_proxy = 0
      },
    },
    {
      /* Attacker tries harder, by sending a signal unicast directly to
       * the subscriber */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_ATTACKER,
        .unicast_to = TEST_CONN_SUBSCRIBER,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 0,
        .received_by_proxy = 0
      },
    },
    {
      /* When the real service sends a signal, it should still get through */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_SERVICE,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 1,
        .received_by_proxy = 1
      },
    },
  },
};

static const TestPlan plan_nonexistent_unique_name =
{
  .description = "A subscription via a unique name that doesn't exist "
                 "accepts no messages",
  .steps = {
    {
      /* Subscriber wants to receive signals from service */
      .action = TEST_ACTION_SUBSCRIBE,
      .u.subscribe = {
        /* This relies on the implementation detail that the dbus-daemon
         * (and presumably other bus implementations) never actually generates
         * a unique name in this format */
        .string_sender = ":0.this.had.better.not.exist",
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
      },
    },
    {
      /* Attacker wants to trick subscriber into thinking that service
       * sent a signal */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_ATTACKER,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 0,
        .received_by_proxy = 0
      },
    },
    {
      /* Attacker tries harder, by sending a signal unicast directly to
       * the subscriber */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_ATTACKER,
        .unicast_to = TEST_CONN_SUBSCRIBER,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 0,
        .received_by_proxy = 0
      },
    },
  },
};

static const TestPlan plan_limit_by_well_known_name =
{
  .description = "A subscription via a well-known name only accepts messages "
                 "sent by the owner of that well-known name",
  .steps = {
    {
      /* Service already owns one name */
      .action = TEST_ACTION_OWN_NAME,
      .u.own_name = {
        .name = ALREADY_OWNED_NAME,
        .owner = TEST_CONN_SERVICE
      },
    },
    {
      /* Subscriber wants to receive signals from service */
      .action = TEST_ACTION_SUBSCRIBE,
      .u.subscribe = {
        .string_sender = ALREADY_OWNED_NAME,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
      },
    },
    {
      /* Subscriber wants to receive signals from service by another name */
      .action = TEST_ACTION_SUBSCRIBE,
      .u.subscribe = {
        .string_sender = OWNED_LATER_NAME,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
      },
    },
    {
      /* Attacker wants to trick subscriber into thinking that service
       * sent a signal */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_ATTACKER,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 0,
        .received_by_proxy = 0
      },
    },
    {
      /* Attacker tries harder, by sending a signal unicast directly to
       * the subscriber */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_ATTACKER,
        .unicast_to = TEST_CONN_SUBSCRIBER,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 0,
        .received_by_proxy = 0
      },
    },
    {
      /* When the service sends a signal with the name it already owns,
       * it should get through */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_SERVICE,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 1,
        .received_by_proxy = 1
      },
    },
    {
      /* Service claims another name */
      .action = TEST_ACTION_OWN_NAME,
      .u.own_name = {
        .name = OWNED_LATER_NAME,
        .owner = TEST_CONN_SERVICE
      },
    },
    {
      /* Now the subscriber gets this signal twice, once for each
       * subscription; and similarly each of the two proxies gets this
       * signal twice */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_SERVICE,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 2,
        .received_by_proxy = 2
      },
    },
  },
};

static const TestPlan plan_unsubscribe_immediately =
{
  .description = "Unsubscribing before GetNameOwner can return doesn't result in a crash",
  .steps = {
    {
      /* Service already owns one name */
      .action = TEST_ACTION_OWN_NAME,
      .u.own_name = {
        .name = ALREADY_OWNED_NAME,
        .owner = TEST_CONN_SERVICE
      },
    },
    {
      .action = TEST_ACTION_SUBSCRIBE,
      .u.subscribe = {
        .string_sender = ALREADY_OWNED_NAME,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .unsubscribe_immediately = TRUE
      },
    },
    {
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_SERVICE,
        .path = EXAMPLE_PATH,
        .iface = EXAMPLE_INTERFACE,
        .member = FOO_SIGNAL,
        .received_by_conn = 0,
        /* The proxy can't unsubscribe, except by destroying the proxy
         * completely, which we don't currently implement in this test */
        .received_by_proxy = 1
      },
    },
  },
};

static const TestPlan plan_limit_to_message_bus =
{
  .description = "A subscription to the message bus only accepts messages "
                 "from the message bus",
  .steps = {
    {
      /* Subscriber wants to receive signals from the message bus itself */
      .action = TEST_ACTION_SUBSCRIBE,
      .u.subscribe = {
        .string_sender = DBUS_SERVICE_DBUS,
        .path = DBUS_PATH_DBUS,
        .iface = DBUS_INTERFACE_DBUS,
      },
    },
    {
      /* Attacker wants to trick subscriber into thinking that the message
       * bus sent a signal */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .sender = TEST_CONN_ATTACKER,
        .path = DBUS_PATH_DBUS,
        .iface = DBUS_INTERFACE_DBUS,
        .member = NAME_OWNER_CHANGED,
        .arg0 = "would I lie to you?",
        .received_by_conn = 0,
        .received_by_proxy = 0
      },
    },
    {
      /* Attacker tries harder, by sending a signal unicast directly to
       * the subscriber, and using more realistic arguments */
      .action = TEST_ACTION_EMIT_SIGNAL,
      .u.signal = {
        .unicast_to = TEST_CONN_SUBSCRIBER,
        .sender = TEST_CONN_ATTACKER,
        .path = DBUS_PATH_DBUS,
        .iface = DBUS_INTERFACE_DBUS,
        .member = NAME_OWNER_CHANGED,
        .args = "('com.example.Name', '', ':1.12')",
        .received_by_conn = 0,
        .received_by_proxy = 0
      },
    },
    {
      /* When the message bus sends a signal (in this case triggered by
       * owning a name), it should still get through */
      .action = TEST_ACTION_OWN_NAME,
      .u.own_name = {
        .name = OWNED_LATER_NAME,
        .owner = TEST_CONN_SERVICE,
        .received_by_conn = 1,
        .received_by_proxy = 1
      },
    },
  },
};

typedef struct
{
  const TestPlan *plan;
  SubscriptionMode mode;
  GError *error;
  /* (element-type ReceivedMessage) */
  GPtrArray *received;
  /* conns[TEST_CONN_NONE] is unused and remains NULL */
  GDBusConnection *conns[NUM_TEST_CONNS];
  /* Proxies on conns[TEST_CONN_SUBSCRIBER] */
  GPtrArray *proxies;
  /* unique_names[TEST_CONN_NONE] is unused and remains NULL */
  const char *unique_names[NUM_TEST_CONNS];
  /* finished[TEST_CONN_NONE] is unused and remains FALSE */
  gboolean finished[NUM_TEST_CONNS];
  /* Remains 0 for any step that is not a subscription */
  guint subscriptions[MAX_TEST_STEPS];
  /* Number of times the signal from step n was received */
  guint received_by_conn[MAX_TEST_STEPS];
  /* Number of times the signal from step n was received */
  guint received_by_proxy[MAX_TEST_STEPS];
  guint finished_subscription;
} Fixture;

/*
 * Called when the subscriber receives a message from any connection
 * announcing that it has emitted all the signals that it plans to emit.
 */
static void
subscriber_finished_cb (GDBusConnection *conn,
                        const char      *sender_name,
                        const char      *path,
                        const char      *iface,
                        const char      *member,
                        GVariant        *parameters,
                        void            *user_data)
{
  Fixture *f = user_data;
  GDBusConnection *subscriber = f->conns[TEST_CONN_SUBSCRIBER];
  guint i;

  g_assert_true (conn == subscriber);

  for (i = TEST_CONN_FIRST; i < G_N_ELEMENTS (f->conns); i++)
    {
      if (g_str_equal (sender_name, f->unique_names[i]))
        {
          g_assert_false (f->finished[i]);
          f->finished[i] = TRUE;

          g_test_message ("Received Finished signal from %s %s",
                          test_conn_descriptions[i], sender_name);
          return;
        }
    }

  g_error ("Received Finished signal from unknown sender %s", sender_name);
}

/*
 * Called when we receive a signal, either via the GDBusProxy (proxy != NULL)
 * or via the GDBusConnection (proxy == NULL).
 */
static void
fixture_received_signal (Fixture    *f,
                         GDBusProxy *proxy,
                         const char *sender_name,
                         const char *path,
                         const char *iface,
                         const char *member,
                         GVariant   *parameters)
{
  guint i;
  ReceivedMessage *received;

  /* Ignore the Finished signal if it matches a wildcard subscription */
  if (g_str_equal (member, FINISHED_SIGNAL))
    return;

  received = g_new0 (ReceivedMessage, 1);

  if (proxy != NULL)
    received->received_by_proxy = g_object_ref (proxy);
  else
    received->received_by_proxy = NULL;

  received->path = g_strdup (path);
  received->iface = g_strdup (iface);
  received->member = g_strdup (member);
  received->parameters = g_variant_ref (parameters);

  for (i = TEST_CONN_FIRST; i < G_N_ELEMENTS (f->conns); i++)
    {
      if (g_str_equal (sender_name, f->unique_names[i]))
        {
          received->sender = i;
          g_assert_false (f->finished[i]);
          break;
        }
    }

  if (g_str_equal (sender_name, DBUS_SERVICE_DBUS))
    {
      g_test_message ("Signal received from message bus %s",
                      sender_name);
    }
  else
    {
      g_test_message ("Signal received from %s %s",
                      test_conn_descriptions[received->sender],
                      sender_name);
      g_assert_cmpint (received->sender, !=, TEST_CONN_NONE);
    }

  g_test_message ("Signal received from %s %s via %s",
                  test_conn_descriptions[received->sender],
                  sender_name,
                  proxy != NULL ? "proxy" : "connection");
  g_test_message ("\tPath: %s", path);
  g_test_message ("\tInterface: %s", iface);
  g_test_message ("\tMember: %s", member);

  if (g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(su)")))
    {
      g_variant_get (parameters, "(su)", &received->arg0, &received->step);
      g_test_message ("\tString argument 0: %s", received->arg0);
      g_test_message ("\tSent in step: %u", received->step);
    }
  else if (g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(uu)")))
    {
      g_variant_get (parameters, "(uu)", NULL, &received->step);
      g_test_message ("\tArgument 0: (not a string)");
      g_test_message ("\tSent in step: %u", received->step);
    }
  else if (g_variant_is_of_type (parameters, G_VARIANT_TYPE ("(sss)")))
    {
      const char *name;
      const char *old_owner;
      const char *new_owner;

      /* The only signal of this signature that we legitimately receive
       * during this test is NameOwnerChanged, so just assert that it
       * is from the message bus and can be matched to a plausible step.
       * (This is less thorough than the above, and will not work if we
       * add a test scenario where a name's ownership is repeatedly
       * changed while watching NameOwnerChanged - so don't do that.) */
      g_assert_cmpstr (sender_name, ==, DBUS_SERVICE_DBUS);
      g_assert_cmpstr (path, ==, DBUS_PATH_DBUS);
      g_assert_cmpstr (iface, ==, DBUS_INTERFACE_DBUS);
      g_assert_cmpstr (member, ==, NAME_OWNER_CHANGED);

      g_variant_get (parameters, "(&s&s&s)", &name, &old_owner, &new_owner);

      for (i = 0; i < G_N_ELEMENTS (f->plan->steps); i++)
        {
          const TestStep *step = &f->plan->steps[i];

          if (step->action == TEST_ACTION_OWN_NAME)
            {
              const TestOwnName *own_name = &step->u.own_name;

              if (g_str_equal (name, own_name->name)
                  && g_str_equal (new_owner, f->unique_names[own_name->owner])
                  && own_name->received_by_conn > 0)
                {
                  received->step = i;
                  break;
                }
            }

          if (i >= G_N_ELEMENTS (f->plan->steps))
            g_error ("Could not match message to a test step");
        }
    }
  else
    {
      g_error ("Unexpected message received");
    }

  g_ptr_array_add (f->received, g_steal_pointer (&received));
}

static void
proxy_signal_cb (GDBusProxy *proxy,
                 const char *sender_name,
                 const char *member,
                 GVariant   *parameters,
                 void       *user_data)
{
  Fixture *f = user_data;

  fixture_received_signal (f, proxy, sender_name,
                           g_dbus_proxy_get_object_path (proxy),
                           g_dbus_proxy_get_interface_name (proxy),
                           member, parameters);
}

static void
subscribed_signal_cb (GDBusConnection *conn,
                      const char      *sender_name,
                      const char      *path,
                      const char      *iface,
                      const char      *member,
                      GVariant        *parameters,
                      void            *user_data)
{
  Fixture *f = user_data;
  GDBusConnection *subscriber = f->conns[TEST_CONN_SUBSCRIBER];

  g_assert_true (conn == subscriber);

  fixture_received_signal (f, NULL, sender_name, path, iface, member, parameters);
}

static void
fixture_subscribe (Fixture             *f,
                   const TestSubscribe *subscribe,
                   guint                step_number)
{
  GDBusConnection *subscriber = f->conns[TEST_CONN_SUBSCRIBER];
  const char *sender;

  if (subscribe->string_sender != NULL)
    {
      sender = subscribe->string_sender;
      g_test_message ("\tSender: %s", sender);
    }
  else if (subscribe->unique_sender != TEST_CONN_NONE)
    {
      sender = f->unique_names[subscribe->unique_sender];
      g_test_message ("\tSender: %s %s",
                      test_conn_descriptions[subscribe->unique_sender],
                      sender);
    }
  else
    {
      sender = NULL;
      g_test_message ("\tSender: (any)");
    }

  g_test_message ("\tPath: %s", nonnull (subscribe->path, "(any)"));
  g_test_message ("\tInterface: %s",
                  nonnull (subscribe->iface, "(any)"));
  g_test_message ("\tMember: %s",
                  nonnull (subscribe->member, "(any)"));
  g_test_message ("\tString argument 0: %s",
                  nonnull (subscribe->arg0, "(any)"));
  g_test_message ("\tFlags: %x", subscribe->flags);

  if (f->mode != SUBSCRIPTION_MODE_PROXY)
    {
      /* CONN or PARALLEL */
      guint id;

      g_test_message ("\tSubscribing via connection");
      id = g_dbus_connection_signal_subscribe (subscriber,
                                               sender,
                                               subscribe->iface,
                                               subscribe->member,
                                               subscribe->path,
                                               subscribe->arg0,
                                               subscribe->flags,
                                               subscribed_signal_cb,
                                               f, NULL);

      g_assert_cmpuint (id, !=, 0);

      if (subscribe->unsubscribe_immediately)
        {
          g_test_message ("\tImmediately unsubscribing");
          g_dbus_connection_signal_unsubscribe (subscriber, g_steal_handle_id (&id));
        }
      else
        {
          f->subscriptions[step_number] = id;
        }
    }

  if (f->mode != SUBSCRIPTION_MODE_CONN)
    {
      /* PROXY or PARALLEL */

      if (sender == NULL)
        {
          g_test_message ("\tCannot subscribe via proxy: no bus name");
        }
      else if (subscribe->path == NULL)
        {
          g_test_message ("\tCannot subscribe via proxy: no path");
        }
      else if (subscribe->iface == NULL)
        {
          g_test_message ("\tCannot subscribe via proxy: no interface");
        }
      else
        {
          GDBusProxy *proxy;

          g_test_message ("\tSubscribing via proxy");
          proxy = g_dbus_proxy_new_sync (subscriber,
                                         (G_DBUS_PROXY_FLAGS_DO_NOT_LOAD_PROPERTIES
                                          | G_DBUS_PROXY_FLAGS_DO_NOT_AUTO_START),
                                         NULL,    /* GDBusInterfaceInfo */
                                         sender,
                                         subscribe->path,
                                         subscribe->iface,
                                         NULL,    /* GCancellable */
                                         &f->error);
          g_assert_no_error (f->error);
          g_assert_nonnull (proxy);
          g_signal_connect (proxy, "g-signal", G_CALLBACK (proxy_signal_cb), f);
          g_ptr_array_add (f->proxies, g_steal_pointer (&proxy));
        }
    }

  /* As in setup(), we need to wait for AddMatch to happen. */
  g_test_message ("Waiting for AddMatch to be processed");
  connection_wait_for_bus (subscriber);
}

static void
fixture_emit_signal (Fixture              *f,
                     const TestEmitSignal *signal,
                     guint                 step_number)
{
  GVariant *body;
  const char *destination;
  gboolean ok;

  g_test_message ("\tSender: %s",
                  test_conn_descriptions[signal->sender]);

  if (signal->unicast_to != TEST_CONN_NONE)
    {
      destination = f->unique_names[signal->unicast_to];
      g_test_message ("\tDestination: %s %s",
                      test_conn_descriptions[signal->unicast_to],
                      destination);
    }
  else
    {
      destination = NULL;
      g_test_message ("\tDestination: (broadcast)");
    }

  g_assert_nonnull (signal->path);
  g_test_message ("\tPath: %s", signal->path);
  g_assert_nonnull (signal->iface);
  g_test_message ("\tInterface: %s", signal->iface);
  g_assert_nonnull (signal->member);
  g_test_message ("\tMember: %s", signal->member);

  /* If arg0 is non-NULL, put it in the message's argument 0.
   * Otherwise put something that will not match any arg0.
   * Either way, put the sequence number in argument 1 so we can
   * correlate sent messages with received messages later. */
  if (signal->args != NULL)
    {
      /* floating */
      body = g_variant_new_parsed (signal->args);
      g_assert_nonnull (body);
    }
  else if (signal->arg0 != NULL)
    {
      g_test_message ("\tString argument 0: %s", signal->arg0);
      body = g_variant_new ("(su)", signal->arg0, (guint32) step_number);
    }
  else
    {
      g_test_message ("\tArgument 0: (not a string)");
      body = g_variant_new ("(uu)", (guint32) 0, (guint32) step_number);
    }

  ok = g_dbus_connection_emit_signal (f->conns[signal->sender],
                                      destination,
                                      signal->path,
                                      signal->iface,
                                      signal->member,
                                      /* steals floating reference */
                                      g_steal_pointer (&body),
                                      &f->error);
  g_assert_no_error (f->error);
  g_assert_true (ok);

  /* Emitting the signal is asynchronous, so if we want subsequent steps
   * to be guaranteed to happen after the signal from the message bus's
   * perspective, we have to do a round-trip to the message bus to sync up. */
  g_test_message ("Waiting for signal to reach message bus");
  connection_wait_for_bus (f->conns[signal->sender]);
}

static void
fixture_own_name (Fixture *f,
                  const TestOwnName *own_name)
{
  GVariant *call_result;
  guint32 flags;
  guint32 result_code;

  g_test_message ("\tName: %s", own_name->name);
  g_test_message ("\tOwner: %s",
                  test_conn_descriptions[own_name->owner]);

  /* For simplicity, we do this via a direct bus call rather than
   * using g_bus_own_name_on_connection(). The flags in
   * GBusNameOwnerFlags are numerically equal to those in the
   * D-Bus wire protocol. */
  flags = G_BUS_NAME_OWNER_FLAGS_DO_NOT_QUEUE;
  call_result = g_dbus_connection_call_sync (f->conns[own_name->owner],
                                             DBUS_SERVICE_DBUS,
                                             DBUS_PATH_DBUS,
                                             DBUS_INTERFACE_DBUS,
                                             "RequestName",
                                             g_variant_new ("(su)",
                                                           own_name->name,
                                                           flags),
                                             G_VARIANT_TYPE ("(u)"),
                                             G_DBUS_CALL_FLAGS_NONE,
                                             -1,
                                             NULL,
                                             &f->error);
  g_assert_no_error (f->error);
  g_assert_nonnull (call_result);
  g_variant_get (call_result, "(u)", &result_code);
  g_assert_cmpuint (result_code, ==, DBUS_REQUEST_NAME_REPLY_PRIMARY_OWNER);
  g_variant_unref (call_result);
}

static void
fixture_run_plan (Fixture          *f,
                  const TestPlan   *plan,
                  SubscriptionMode  mode)
{
  guint i;

  G_STATIC_ASSERT (G_N_ELEMENTS (plan->steps) == G_N_ELEMENTS (f->subscriptions));
  G_STATIC_ASSERT (G_N_ELEMENTS (plan->steps) == G_N_ELEMENTS (f->received_by_conn));
  G_STATIC_ASSERT (G_N_ELEMENTS (plan->steps) == G_N_ELEMENTS (f->received_by_proxy));

  f->mode = mode;
  f->plan = plan;

  g_test_summary (plan->description);

  for (i = 0; i < G_N_ELEMENTS (plan->steps); i++)
    {
      const TestStep *step = &plan->steps[i];

      switch (step->action)
        {
          case TEST_ACTION_SUBSCRIBE:
            g_test_message ("Step %u: adding subscription", i);
            fixture_subscribe (f, &step->u.subscribe, i);
            break;

          case TEST_ACTION_EMIT_SIGNAL:
            g_test_message ("Step %u: emitting signal", i);
            fixture_emit_signal (f, &step->u.signal, i);
            break;

          case TEST_ACTION_OWN_NAME:
            g_test_message ("Step %u: claiming bus name", i);
            fixture_own_name (f, &step->u.own_name);
            break;

          case TEST_ACTION_NONE:
            /* Padding to fill the rest of the array, do nothing */
            break;

          default:
            g_return_if_reached ();
        }
    }

  /* Now that we have done everything we wanted to do, emit Finished
   * from each connection. */
  for (i = TEST_CONN_FIRST; i < G_N_ELEMENTS (f->conns); i++)
    {
      gboolean ok;

      ok = g_dbus_connection_emit_signal (f->conns[i],
                                          NULL,
                                          FINISHED_PATH,
                                          FINISHED_INTERFACE,
                                          FINISHED_SIGNAL,
                                          NULL,
                                          &f->error);
      g_assert_no_error (f->error);
      g_assert_true (ok);
    }

  /* Wait until we have seen the Finished signal from each sender */
  while (TRUE)
    {
      gboolean all_finished = TRUE;

      for (i = TEST_CONN_FIRST; i < G_N_ELEMENTS (f->conns); i++)
        all_finished = all_finished && f->finished[i];

      if (all_finished)
        break;

      g_main_context_iteration (NULL, TRUE);
    }

  /* Assert that the correct things happened before each Finished signal */
  for (i = 0; i < f->received->len; i++)
    {
      const ReceivedMessage *received = g_ptr_array_index (f->received, i);

      g_assert_cmpuint (received->step, <, G_N_ELEMENTS (f->received_by_conn));
      g_assert_cmpuint (received->step, <, G_N_ELEMENTS (f->received_by_proxy));

      if (received->received_by_proxy != NULL)
        f->received_by_proxy[received->step] += 1;
      else
        f->received_by_conn[received->step] += 1;
    }

  for (i = 0; i < G_N_ELEMENTS (plan->steps); i++)
    {
      const TestStep *step = &plan->steps[i];

      if (step->action == TEST_ACTION_EMIT_SIGNAL)
        {
          const TestEmitSignal *signal = &plan->steps[i].u.signal;

          if (mode != SUBSCRIPTION_MODE_PROXY)
            {
              g_test_message ("Signal from step %u was received %u times by "
                              "GDBusConnection, expected %u",
                              i, f->received_by_conn[i], signal->received_by_conn);
              g_assert_cmpuint (f->received_by_conn[i], ==, signal->received_by_conn);
            }
          else
            {
              g_assert_cmpuint (f->received_by_conn[i], ==, 0);
            }

          if (mode != SUBSCRIPTION_MODE_CONN)
            {
              g_test_message ("Signal from step %u was received %u times by "
                              "GDBusProxy, expected %u",
                              i, f->received_by_proxy[i], signal->received_by_proxy);
              g_assert_cmpuint (f->received_by_proxy[i], ==, signal->received_by_proxy);
            }
          else
            {
              g_assert_cmpuint (f->received_by_proxy[i], ==, 0);
            }
        }
      else if (step->action == TEST_ACTION_OWN_NAME)
        {
          const TestOwnName *own_name = &plan->steps[i].u.own_name;

          if (mode != SUBSCRIPTION_MODE_PROXY)
            {
              g_test_message ("NameOwnerChanged from step %u was received %u "
                              "times by GDBusConnection, expected %u",
                              i, f->received_by_conn[i], own_name->received_by_conn);
              g_assert_cmpuint (f->received_by_conn[i], ==, own_name->received_by_conn);
            }
          else
            {
              g_assert_cmpuint (f->received_by_conn[i], ==, 0);
            }

          if (mode != SUBSCRIPTION_MODE_CONN)
            {
              g_test_message ("NameOwnerChanged from step %u was received %u "
                              "times by GDBusProxy, expected %u",
                              i, f->received_by_proxy[i], own_name->received_by_proxy);
              g_assert_cmpuint (f->received_by_proxy[i], ==, own_name->received_by_proxy);
            }
          else
            {
              g_assert_cmpuint (f->received_by_proxy[i], ==, 0);
            }
        }
    }
}

static void
setup (Fixture *f,
       G_GNUC_UNUSED const void *context)
{
  GDBusConnection *subscriber;
  guint i;

  session_bus_up ();

  f->proxies = g_ptr_array_new_full (MAX_TEST_STEPS, g_object_unref);
  f->received = g_ptr_array_new_full (MAX_TEST_STEPS,
                                      (GDestroyNotify) received_message_free);

  for (i = TEST_CONN_FIRST; i < G_N_ELEMENTS (f->conns); i++)
    {
      f->conns[i] = _g_bus_get_priv (G_BUS_TYPE_SESSION, NULL, &f->error);
      g_assert_no_error (f->error);
      g_assert_nonnull (f->conns[i]);

      f->unique_names[i] = g_dbus_connection_get_unique_name (f->conns[i]);
      g_assert_nonnull (f->unique_names[i]);
      g_test_message ("%s is %s",
                      test_conn_descriptions[i],
                      f->unique_names[i]);
    }

  subscriber = f->conns[TEST_CONN_SUBSCRIBER];

  /* Used to wait for all connections to finish sending whatever they
   * wanted to send */
  f->finished_subscription = g_dbus_connection_signal_subscribe (subscriber,
                                                                 NULL,
                                                                 FINISHED_INTERFACE,
                                                                 FINISHED_SIGNAL,
                                                                 FINISHED_PATH,
                                                                 NULL,
                                                                 G_DBUS_SIGNAL_FLAGS_NONE,
                                                                 subscriber_finished_cb,
                                                                 f, NULL);
  /* AddMatch is sent asynchronously, so we don't know how
   * soon it will be processed. Before emitting signals, we
   * need to wait for the message bus to get as far as processing
   * AddMatch. */
  g_test_message ("Waiting for AddMatch to be processed");
  connection_wait_for_bus (subscriber);
}

static void
test_conn_subscribe (Fixture *f,
                     const void *context)
{
  fixture_run_plan (f, context, SUBSCRIPTION_MODE_CONN);
}

static void
test_proxy_subscribe (Fixture *f,
                      const void *context)
{
  fixture_run_plan (f, context, SUBSCRIPTION_MODE_PROXY);
}

static void
test_parallel_subscribe (Fixture *f,
                         const void *context)
{
  fixture_run_plan (f, context, SUBSCRIPTION_MODE_PARALLEL);
}

static void
teardown (Fixture *f,
          G_GNUC_UNUSED const void *context)
{
  GDBusConnection *subscriber = f->conns[TEST_CONN_SUBSCRIBER];
  guint i;

  g_ptr_array_unref (f->proxies);

  if (f->finished_subscription != 0)
    g_dbus_connection_signal_unsubscribe (subscriber, g_steal_handle_id (&f->finished_subscription));

  for (i = 0; i < G_N_ELEMENTS (f->subscriptions); i++)
    {
      if (f->subscriptions[i] != 0)
        g_dbus_connection_signal_unsubscribe (subscriber, g_steal_handle_id (&f->subscriptions[i]));
    }

  g_ptr_array_unref (f->received);

  for (i = TEST_CONN_FIRST; i < G_N_ELEMENTS (f->conns); i++)
    g_clear_object (&f->conns[i]);

  g_clear_error (&f->error);

  session_bus_down ();
}

int
main (int   argc,
      char *argv[])
{
  g_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);

  g_test_dbus_unset ();

#define ADD_SUBSCRIBE_TEST(name) \
  do { \
    g_test_add ("/gdbus/subscribe/conn/" #name, \
                Fixture, &plan_ ## name, \
                setup, test_conn_subscribe, teardown); \
    g_test_add ("/gdbus/subscribe/proxy/" #name, \
                Fixture, &plan_ ## name, \
                setup, test_proxy_subscribe, teardown); \
    g_test_add ("/gdbus/subscribe/parallel/" #name, \
                Fixture, &plan_ ## name, \
                setup, test_parallel_subscribe, teardown); \
  } while (0)

  ADD_SUBSCRIBE_TEST (simple);
  ADD_SUBSCRIBE_TEST (broadcast_from_anyone);
  ADD_SUBSCRIBE_TEST (match_twice);
  ADD_SUBSCRIBE_TEST (limit_by_unique_name);
  ADD_SUBSCRIBE_TEST (nonexistent_unique_name);
  ADD_SUBSCRIBE_TEST (limit_by_well_known_name);
  ADD_SUBSCRIBE_TEST (limit_to_message_bus);
  ADD_SUBSCRIBE_TEST (unsubscribe_immediately);

  return g_test_run();
}
