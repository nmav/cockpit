/*
 * This file is part of Cockpit.
 *
 * Copyright (C) 2013 Red Hat, Inc.
 *
 * Cockpit is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or
 * (at your option) any later version.
 *
 * Cockpit is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Cockpit; If not, see <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include "cockpitchannel.h"
#include "mock-transport.h"

#include "common/cockpitjson.h"
#include "common/cockpittest.h"

#include <json-glib/json-glib.h>

#include <gio/gio.h>

/* ----------------------------------------------------------------------------
 * Mock
 */

static GType mock_echo_channel_get_type (void) G_GNUC_CONST;

typedef struct {
    CockpitChannel parent;
    gboolean close_called;
} MockEchoChannel;

typedef CockpitChannelClass MockEchoChannelClass;

G_DEFINE_TYPE (MockEchoChannel, mock_echo_channel, COCKPIT_TYPE_CHANNEL);

static void
mock_echo_channel_recv (CockpitChannel *channel,
                        GBytes *message)
{
  cockpit_channel_send (channel, message, FALSE);
}

static void
mock_echo_channel_close (CockpitChannel *channel,
                         const gchar *problem)
{
  MockEchoChannel *self = (MockEchoChannel *)channel;
  self->close_called = TRUE;
  COCKPIT_CHANNEL_CLASS (mock_echo_channel_parent_class)->close (channel, problem);
}

static void
mock_echo_channel_init (MockEchoChannel *self)
{

}

static void
mock_echo_channel_class_init (MockEchoChannelClass *klass)
{
  CockpitChannelClass *channel_class = COCKPIT_CHANNEL_CLASS (klass);
  channel_class->recv = mock_echo_channel_recv;
  channel_class->close = mock_echo_channel_close;
}

static CockpitChannel *
mock_echo_channel_open (CockpitTransport *transport,
                        const gchar *channel_id)
{
  CockpitChannel *channel;
  JsonObject *options;

  g_assert (channel_id != NULL);

  options = json_object_new ();
  channel = g_object_new (mock_echo_channel_get_type (),
                          "transport", transport,
                          "id", channel_id,
                          "options", options,
                          NULL);

  json_object_unref (options);
  return channel;
}

/* ----------------------------------------------------------------------------
 * Testing
 */

typedef struct {
  MockTransport *transport;
  CockpitChannel *channel;
} TestCase;

static void
setup (TestCase *tc,
       gconstpointer unused)
{
  tc->transport = g_object_new (mock_transport_get_type (), NULL);
  tc->channel = mock_echo_channel_open (COCKPIT_TRANSPORT (tc->transport), "554");
  while (g_main_context_iteration (NULL, FALSE));
}

static void
teardown (TestCase *tc,
          gconstpointer unused)
{
  g_object_add_weak_pointer (G_OBJECT (tc->channel), (gpointer *)&tc->channel);
  g_object_add_weak_pointer (G_OBJECT (tc->transport), (gpointer *)&tc->transport);
  g_object_unref (tc->channel);
  g_object_unref (tc->transport);
  g_assert (tc->channel == NULL);
  g_assert (tc->transport == NULL);
}

static void
test_recv_and_send (TestCase *tc,
                    gconstpointer unused)
{
  GBytes *sent;
  GBytes *payload;

  /* Ready to go */
  cockpit_channel_ready (tc->channel);

  payload = g_bytes_new ("Yeehaw!", 7);
  cockpit_transport_emit_recv (COCKPIT_TRANSPORT (tc->transport), "554", payload);

  sent = mock_transport_pop_channel (tc->transport, "554");
  g_assert (sent != NULL);

  g_assert (g_bytes_equal (payload, sent));
  g_bytes_unref (payload);
}

static void
test_recv_and_queue (TestCase *tc,
                     gconstpointer unused)
{
  GBytes *payload;
  GBytes *sent;

  payload = g_bytes_new ("Yeehaw!", 7);
  cockpit_transport_emit_recv (COCKPIT_TRANSPORT (tc->transport), "554", payload);

  /* Shouldn't have received it yet */
  g_assert_cmpuint (mock_transport_count_sent (tc->transport), ==, 0);

  /* Ready to go */
  cockpit_channel_ready (tc->channel);

  sent = mock_transport_pop_channel (tc->transport, "554");
  g_assert (sent != NULL);

  g_assert (g_bytes_equal (payload, sent));
  g_bytes_unref (payload);
}

static void
test_close_immediately (TestCase *tc,
                        gconstpointer unused)
{
  GBytes *payload;
  JsonObject *sent;

  payload = g_bytes_new ("Yeehaw!", 7);
  cockpit_transport_emit_recv (COCKPIT_TRANSPORT (tc->transport), "554", payload);
  g_bytes_unref (payload);

  /* Shouldn't have received it yet */
  g_assert_cmpuint (mock_transport_count_sent (tc->transport), ==, 0);

  /* Now close without getting anything */
  cockpit_channel_close (tc->channel, "bad-boy");

  g_assert (mock_transport_pop_channel (tc->transport, "554") == NULL);
  g_assert_cmpuint (mock_transport_count_sent (tc->transport), ==, 1);

  sent = mock_transport_pop_control (tc->transport);
  g_assert (sent != NULL);

  cockpit_assert_json_eq (sent,
                  "{ \"command\": \"close\", \"channel\": \"554\", \"problem\": \"bad-boy\"}");
}

static void
test_close_option (TestCase *tc,
                   gconstpointer unused)
{
  JsonObject *sent;
  JsonObject *options;

  options = cockpit_channel_close_options (tc->channel);
  json_object_set_string_member (options, "option", "four");
  cockpit_channel_close (tc->channel, "bad-boy");

  g_assert_cmpuint (mock_transport_count_sent (tc->transport), ==, 1);

  sent = mock_transport_pop_control (tc->transport);
  g_assert (sent != NULL);

  cockpit_assert_json_eq (sent,
                  "{ \"command\": \"close\", \"channel\": \"554\", \"problem\": \"bad-boy\", \"option\": \"four\" }");
}

static void
test_close_json_option (TestCase *tc,
                        gconstpointer unused)
{
  JsonObject *sent;
  JsonObject *obj;
  JsonObject *options;

  obj = json_object_new ();
  json_object_set_string_member (obj, "test", "value");
  options = cockpit_channel_close_options (tc->channel);
  json_object_set_object_member (options, "option", obj);

  cockpit_channel_close (tc->channel, "bad-boy");

  g_assert_cmpuint (mock_transport_count_sent (tc->transport), ==, 1);

  sent = mock_transport_pop_control (tc->transport);
  g_assert (sent != NULL);

  cockpit_assert_json_eq (sent,
                  "{ \"command\": \"close\", \"channel\": \"554\", \"problem\": \"bad-boy\", \"option\": { \"test\": \"value\" } }");
}

static void
on_closed_get_problem (CockpitChannel *channel,
                       const gchar *problem,
                       gpointer user_data)
{
  gchar **retval = user_data;
  g_assert (*retval == NULL);
  *retval = g_strdup (problem);
}

static void
test_close_transport (TestCase *tc,
                      gconstpointer unused)
{
  MockEchoChannel *chan;
  GBytes *sent;
  gchar *problem = NULL;

  chan = (MockEchoChannel *)tc->channel;
  cockpit_channel_ready (tc->channel);

  sent = g_bytes_new ("Yeehaw!", 7);
  cockpit_transport_emit_recv (COCKPIT_TRANSPORT (tc->transport), "554", sent);
  g_bytes_unref (sent);

  g_assert (chan->close_called == FALSE);

  g_signal_connect (tc->channel, "closed", G_CALLBACK (on_closed_get_problem), &problem);
  cockpit_transport_close (COCKPIT_TRANSPORT (tc->transport), "boooo");

  g_assert (chan->close_called == TRUE);

  g_assert_cmpstr (problem, ==, "boooo");
  g_assert (mock_transport_pop_control (tc->transport) == NULL);

  g_free (problem);
}

static void
test_get_option (void)
{
  JsonObject *options;
  CockpitTransport *transport;
  CockpitChannel *channel;

  options = json_object_new ();
  json_object_set_string_member (options, "scruffy", "janitor");
  json_object_set_int_member (options, "age", 5);

  transport = g_object_new (mock_transport_get_type (), NULL);
  channel = g_object_new (mock_echo_channel_get_type (),
                          "transport", transport,
                          "id", "55",
                          "options", options,
                          NULL);
  g_object_unref (transport);
  json_object_unref (options);

  options = cockpit_channel_get_options (channel);
  g_assert_cmpstr (json_object_get_string_member (options, "scruffy"), ==, "janitor");
  g_assert_cmpint (json_object_get_int_member (options, "age"), ==, 5);
  g_assert (json_object_get_member (options, "marmalade") == NULL);

  g_object_unref (channel);
}

static void
test_properties (void)
{
  JsonObject *options;
  CockpitTransport *transport;
  CockpitTransport *check;
  CockpitChannel *channel;
  gchar *channel_id;

  options = json_object_new ();
  transport = g_object_new (mock_transport_get_type (), NULL);
  channel = g_object_new (mock_echo_channel_get_type (),
                          "transport", transport,
                          "id", "55",
                          "options", options,
                          NULL);
  g_object_unref (transport);
  json_object_unref (options);

  g_object_get (channel, "transport", &check, "id", &channel_id, NULL);
  g_assert (check == transport);
  g_assert_cmpstr (cockpit_channel_get_id (channel), ==, "55");
  g_assert_cmpstr (channel_id, ==, "55");
  g_free (channel_id);

  g_object_unref (channel);
}

static void
test_close_not_capable (void)
{
  JsonObject *options;
  JsonObject *sent;
  JsonArray *capabilities;
  MockTransport *transport;
  CockpitChannel *channel;
  CockpitChannel *channel2;
  const gchar *cap[] = { "supported", NULL };

  options = json_object_new ();
  capabilities = json_array_new ();
  json_array_add_string_element (capabilities, "unsupported1");
  json_array_add_string_element (capabilities, "unsupported2");
  json_object_set_array_member (options, "capabilities", capabilities);
  transport = g_object_new (mock_transport_get_type (), NULL);

  channel = g_object_new (mock_echo_channel_get_type (),
                          "transport", transport,
                          "id", "55",
                          "options", options,
                          NULL);

  while (g_main_context_iteration (NULL, FALSE));

  sent = mock_transport_pop_control (transport);
  g_assert (sent != NULL);

  cockpit_assert_json_eq (sent,
                  "{ \"command\": \"close\", \"channel\": \"55\", \"problem\": \"not-supported\", \"capabilities\":[]}");
  g_object_unref (channel);

  channel2 = g_object_new (mock_echo_channel_get_type (),
                           "transport", transport,
                           "id", "55",
                           "options", options,
                           "capabilities", cap,
                           NULL);
  json_object_unref (options);

  while (g_main_context_iteration (NULL, FALSE));

  sent = mock_transport_pop_control (transport);
  g_assert (sent != NULL);

  cockpit_assert_json_eq (sent,
                  "{ \"command\": \"close\", \"channel\": \"55\", \"problem\": \"not-supported\", \"capabilities\":[\"supported\"]}");

  g_object_unref (channel2);
  g_object_unref (transport);
}

static void
test_capable (void)
{
  JsonObject *options;
  JsonObject *sent;
  JsonArray *capabilities;
  MockTransport *transport;
  CockpitChannel *channel;
  const gchar *cap[] = { "supported", NULL };

  options = json_object_new ();
  capabilities = json_array_new ();
  json_array_add_string_element (capabilities, "supported");
  json_object_set_array_member (options, "capabilities", capabilities);
  transport = g_object_new (mock_transport_get_type (), NULL);

  channel = g_object_new (mock_echo_channel_get_type (),
                          "transport", transport,
                          "id", "55",
                          "options", options,
                          "capabilities", cap,
                          NULL);
  json_object_unref (options);

  while (g_main_context_iteration (NULL, FALSE));

  sent = mock_transport_pop_control (transport);
  g_assert (sent == NULL);

  g_object_unref (channel);
  g_object_unref (transport);
}

static void
test_invalid_internal (void)
{
  JsonObject *options;
  MockTransport *transport;
  CockpitChannel *channel;
  JsonObject *sent;
  GSocketConnectable *connectable;

  options = json_object_new ();
  json_object_set_string_member (options, "internal", "test");
  transport = g_object_new (mock_transport_get_type (), NULL);

  channel = g_object_new (mock_echo_channel_get_type (),
                          "transport", transport,
                          "id", "55",
                          "options", options,
                          NULL);
  json_object_unref (options);
  connectable = cockpit_channel_parse_connectable (channel, NULL);
  g_assert (connectable == NULL);
  while (g_main_context_iteration (NULL, FALSE));

  sent = mock_transport_pop_control (transport);
  g_assert (sent != NULL);

  cockpit_assert_json_eq (sent,
                  "{ \"command\": \"close\", \"channel\": \"55\", \"problem\": \"not-found\"}");
  g_object_unref (channel);
  g_object_unref (transport);
  cockpit_assert_expected ();
}

static void
test_internal_not_registered (void)
{
  cockpit_expect_warning ("couldn't find internal address: test");
  cockpit_channel_internal_address ("other", NULL);
  test_invalid_internal ();
  cockpit_channel_remove_internal_address ("other");
}

static void
test_internal_null_registered (void)
{
  cockpit_channel_internal_address ("test", NULL);
  test_invalid_internal ();
  cockpit_channel_remove_internal_address ("test");
}

int
main (int argc,
      char *argv[])
{
  cockpit_test_init (&argc, &argv);

  g_test_add_func ("/channel/get-option", test_get_option);
  g_test_add_func ("/channel/properties", test_properties);
  g_test_add_func ("/channel/test_close_not_capable",
                   test_close_not_capable);
  g_test_add_func ("/channel/test_capable",
                   test_capable);
  g_test_add_func ("/channel/internal-null-registered",
                   test_internal_null_registered);
  g_test_add_func ("/channel/internal-not-registered",
                   test_internal_not_registered);
  g_test_add ("/channel/recv-send", TestCase, NULL,
              setup, test_recv_and_send, teardown);
  g_test_add ("/channel/recv-queue", TestCase, NULL,
              setup, test_recv_and_queue, teardown);
  g_test_add ("/channel/close-immediately", TestCase, NULL,
              setup, test_close_immediately, teardown);
  g_test_add ("/channel/close-option", TestCase, NULL,
              setup, test_close_option, teardown);
  g_test_add ("/channel/close-json-option", TestCase, NULL,
              setup, test_close_json_option, teardown);
  g_test_add ("/channel/close-transport", TestCase, NULL,
              setup, test_close_transport, teardown);

  return g_test_run ();
}
