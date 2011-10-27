/* mongo-client.c
 *
 * Copyright (C) 2011 Christian Hergert <christian@catch.com>
 *
 * This file is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <glib/gi18n.h>

#include "mongo-client.h"

G_DEFINE_TYPE(MongoClient, mongo_client, G_TYPE_OBJECT)

typedef struct
{
   gchar host[255];
   guint port;
} MongoClientPeer;

typedef enum
{
   MONGO_CLIENT_READY,
   MONGO_CLIENT_CONNECTING,
   MONGO_CLIENT_CONNECTED,
   MONGO_CLIENT_FAILED,
} MongoClientState;

struct _MongoClientPrivate
{
   MongoClientState   state;
   GArray            *peers;
   MongoClientPeer    primary;
   guint              timeout;
   GSocketConnection *connection;
   gint               next_id;
};

enum
{
   PROP_0,
   PROP_HOST,
   PROP_PORT,
   PROP_TIMEOUT,
   LAST_PROP
};

static GParamSpec *gParamSpecs[LAST_PROP];

void
mongo_client_add_peer (MongoClient *client,
                       const gchar *host,
                       guint        port)
{
   MongoClientPrivate *priv;
   MongoClientPeer peer = {{ 0 }};

   g_return_if_fail(MONGO_IS_CLIENT(client));
   g_return_if_fail(host != NULL);
   g_return_if_fail(strlen(host) < 255);

   priv = client->priv;

   if (priv->state != MONGO_CLIENT_READY) {
      g_warning("Cannot add peer after connecting.");
      return;
   }

   if (!priv->peers) {
      priv->peers = g_array_new(FALSE, FALSE, sizeof peer);
   }

   g_snprintf(peer.host, sizeof peer.host, "%s", host);
   peer.port = port ? port : 27017;
   g_array_append_val(priv->peers, peer);
}

const gchar *
mongo_client_get_host (MongoClient *client)
{
   g_return_val_if_fail(MONGO_IS_CLIENT(client), NULL);

   return client->priv->primary.host;
}

void
mongo_client_set_host (MongoClient *client,
                       const gchar *host)
{
   MongoClientPrivate *priv;

   g_return_if_fail(MONGO_IS_CLIENT(client));

   priv = client->priv;

   if (priv->state != MONGO_CLIENT_READY) {
      g_warning("Cannot set host after connecting.");
      return;
   }

   if (!host || !host[0]) {
      host = "localhost";
   }

   g_snprintf(priv->primary.host, sizeof priv->primary.host, "%s", host);
   g_object_notify_by_pspec(G_OBJECT(client), gParamSpecs[PROP_HOST]);
}

guint
mongo_client_get_port (MongoClient *client)
{
   g_return_val_if_fail(MONGO_IS_CLIENT(client), 0);

   return client->priv->primary.port;
}

void
mongo_client_set_port (MongoClient *client,
                       guint            port)
{
   MongoClientPrivate *priv;

   g_return_if_fail(MONGO_IS_CLIENT(client));
   g_return_if_fail(port <= G_MAXUSHORT);

   priv = client->priv;

   if (priv->state != MONGO_CLIENT_READY) {
      g_warning("Cannot set port after connecting.");
      return;
   }

   priv->primary.port = port ? port : 27017;
   g_object_notify_by_pspec(G_OBJECT(client), gParamSpecs[PROP_PORT]);
}

guint
mongo_client_get_timeout (MongoClient *client)
{
   g_return_val_if_fail(MONGO_IS_CLIENT(client), 0);

   return client->priv->timeout;
}

void
mongo_client_set_timeout (MongoClient *client,
                          guint            timeout)
{
   g_return_if_fail(MONGO_IS_CLIENT(client));

   client->priv->timeout = timeout;
   g_object_notify_by_pspec(G_OBJECT(client), gParamSpecs[PROP_TIMEOUT]);
}

static gint
mongo_client_get_next_id (MongoClient *client)
{
   g_return_val_if_fail(MONGO_IS_CLIENT(client), 0);
   return g_atomic_int_add(&client->priv->next_id, 1);
}

static void
mongo_client_send_write_cb (GObject      *object,
                            GAsyncResult *result,
                            gpointer      user_data)
{
   GSimpleAsyncResult *simple = user_data;
   GOutputStream *output = (GOutputStream *)object;
   gboolean want_reply;
   GError *error = NULL;
   gssize n_written;

   g_return_if_fail(G_IS_OUTPUT_STREAM(output));
   g_return_if_fail(G_IS_SIMPLE_ASYNC_RESULT(simple));

   n_written = g_output_stream_write_finish(output, result, &error);
   if (n_written <= 0) {
      g_simple_async_result_take_error(simple, error);
      g_simple_async_result_complete_in_idle(simple);
      g_object_unref(simple);
      return;
   }

   /*
    * TODO: Do we need to write more data from the buffer?
    */

   want_reply = GPOINTER_TO_INT(g_object_get_data(user_data, "want-reply"));
   if (want_reply) {
      g_debug("Waiting for reply from server!!");
      /*
       * TODO: Register simple for reply handler.
       */
      return;
   }

   g_simple_async_result_complete_in_idle(simple);
   g_object_unref(simple);
}

void
mongo_client_send_async (MongoClient         *client,
                         const gchar         *db,
                         MongoBson           *bson,
                         MongoOperation       operation,
                         gboolean             want_reply,
                         GAsyncReadyCallback  callback,
                         gpointer             user_data)
{
   MongoClientPrivate *priv;
   GSimpleAsyncResult *simple;
   GOutputStream *output;
   const guint8 *buffer;
   gsize buffer_length = 0;
#pragma pack(push, 4)
   struct {
      gint32 length;
      gint32 id;
      gint32 flags;
      gint32 operation;
      guint8 data[0];
   } *packed;
#pragma pack(pop)

   g_return_if_fail(MONGO_IS_CLIENT(client));
   g_return_if_fail(db != NULL);
   g_return_if_fail(bson != NULL);
   g_return_if_fail(callback != NULL);

   priv = client->priv;

   if (!priv->connection) {
      g_warning("Not connected, failed to send.");
      return;
   }

   /*
    * TODO: We should find a way to use an io_vec here.
    */
   packed = g_malloc((sizeof *packed) + buffer_length);
   packed->length = (sizeof *packed) + buffer_length;
   packed->id = GINT_TO_LE(mongo_client_get_next_id(client));
   packed->flags = GINT_TO_LE(0);
   packed->operation = GINT_TO_LE(operation);
   memcpy(packed->data, buffer, buffer_length);

   /*
    * XXX: MULTIPLE ASYNC WRITE CALLS IS INVALID API USE!!!
    */

   simple = g_simple_async_result_new(G_OBJECT(client), callback, user_data,
                                      mongo_client_send_async);
   output = g_io_stream_get_output_stream(G_IO_STREAM(priv->connection));
   g_object_set_data(G_OBJECT(simple),
                     "want-reply", GINT_TO_POINTER(want_reply));
   g_output_stream_write_async(output,
                               packed,
                               (sizeof *packed) + buffer_length,
                               G_PRIORITY_DEFAULT,
                               NULL,
                               mongo_client_send_write_cb,
                               simple);
   g_free(packed);
}

MongoBson *
mongo_client_send_finish (MongoClient   *client,
                          GAsyncResult  *result,
                          GError       **error)
{
   return NULL;
}

static void
mongo_client_ismaster_cb (GObject      *object,
                          GAsyncResult *result,
                          gpointer      user_data)
{
   GSimpleAsyncResult *simple = user_data;
#if 0
   //MongoBsonIter iter;
   MongoClient *client = (MongoClient *)object;
   MongoBson *bson;
   gboolean ret = FALSE;
   GError *error = NULL;

   g_return_if_fail(MONGO_IS_CLIENT(client));
   g_return_if_fail(G_IS_SIMPLE_ASYNC_RESULT(simple));

   if (!(bson = mongo_client_send_finish(client, result, &error))) {
      goto failure;
   }

   mongo_bson_iter_init(&iter, bson);

   if (mongo_bson_iter_find(&iter, "ismaster")) {
      ret = mongo_bson_iter_get_boolean(&iter);
   }

   if (!ret) {
      error = g_error_new(MONGO_CLIENT_ERROR, MONGO_CLIENT_ERROR_NOT_PRIMARY,
                          _("The target host is not primary."));
   }

   mongo_bson_unref(bson);

failure:
   if (error) {
      g_simple_async_result_take_error(simple, error);
   }
   g_simple_async_result_set_op_res_gboolean(simple, ret);
#endif
   g_simple_async_result_complete_in_idle(simple);
   g_object_unref(simple);
}

static void
mongo_client_connect_cb (GObject      *object,
                         GAsyncResult *result,
                         gpointer      user_data)
{
   GSimpleAsyncResult *simple = user_data;
   GSocketConnection *connection;
   GSocketClient *connector = (GSocketClient *)object;
   MongoClient *client;
   MongoBson *bson;
   GError *error = NULL;

   g_return_if_fail(G_IS_SOCKET_CLIENT(connector));
   g_return_if_fail(G_IS_ASYNC_RESULT(result));

   client = MONGO_CLIENT(g_async_result_get_source_object(user_data));

   if (!(connection = g_socket_client_connect_finish(connector, result, &error))) {
      g_simple_async_result_take_error(simple, error);
      g_simple_async_result_complete_in_idle(simple);
      g_object_unref(simple);
      return;
   }

   client->priv->state = MONGO_CLIENT_CONNECTED;
   client->priv->connection = connection;

   bson = mongo_bson_new();
   //mongo_bson_finish(bson);

   mongo_bson_append_int(bson, "isMaster", 1);
   mongo_client_send_async(client,
                           "admin",
                           bson,
                           MONGO_OPERATION_QUERY,
                           TRUE,
                           mongo_client_ismaster_cb,
                           simple);
}

void
mongo_client_connect_async (MongoClient         *client,
                            GCancellable        *cancellable,
                            GAsyncReadyCallback  callback,
                            gpointer             user_data)
{
   GSocketConnectable *connectable;
   MongoClientPrivate *priv;
   GSimpleAsyncResult *simple;
   GSocketClient *connector;

   g_return_if_fail(MONGO_IS_CLIENT(client));
   g_return_if_fail(callback != NULL);

   priv = client->priv;

   if (priv->state != MONGO_CLIENT_READY) {
      g_warning("Cannot connect client, not ready.");
      return;
   }

   connector = g_object_new(G_TYPE_SOCKET_CLIENT,
                            "family", G_SOCKET_FAMILY_IPV4,
                            "protocol", G_SOCKET_PROTOCOL_TCP,
                            "timeout", priv->timeout,
                            NULL);
   connectable = g_network_address_new(priv->primary.host,
                                       priv->primary.port);
   simple = g_simple_async_result_new(G_OBJECT(client), callback, user_data,
                                      mongo_client_connect_async);

   priv->state = MONGO_CLIENT_CONNECTING;

   g_socket_client_connect_async(connector, connectable, cancellable,
                                 mongo_client_connect_cb, simple);

   g_object_unref(connectable);
   g_object_unref(connector);
}

gboolean
mongo_client_connect_finish (MongoClient   *client,
                             GAsyncResult  *result,
                             GError       **error)
{
   GSimpleAsyncResult *simple = (GSimpleAsyncResult *)result;
   gboolean ret;

   g_return_val_if_fail(MONGO_IS_CLIENT(client), FALSE);
   g_return_val_if_fail(G_IS_SIMPLE_ASYNC_RESULT(simple), FALSE);

   if (!(ret = g_simple_async_result_get_op_res_gboolean(simple))) {
      g_simple_async_result_propagate_error(simple, error);
   }

   return ret;
}

/**
 * mongo_client_finalize:
 * @object: (in): A #MongoClient.
 *
 * Finalizer for a #MongoClient instance.  Frees any resources held by
 * the instance.
 */
static void
mongo_client_finalize (GObject *object)
{
   MongoClientPrivate *priv = MONGO_CLIENT(object)->priv;

   if (priv->peers) {
      g_array_unref(priv->peers);
   }

   g_clear_object(&priv->connection);

   G_OBJECT_CLASS(mongo_client_parent_class)->finalize(object);
}

/**
 * mongo_client_get_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (out): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Get a given #GObject property.
 */
static void
mongo_client_get_property (GObject    *object,
                           guint       prop_id,
                           GValue     *value,
                           GParamSpec *pspec)
{
   MongoClient *client = MONGO_CLIENT(object);

   switch (prop_id) {
   case PROP_HOST:
      g_value_set_string(value, mongo_client_get_host(client));
      break;
   case PROP_PORT:
      g_value_set_uint(value, mongo_client_get_port(client));
      break;
   case PROP_TIMEOUT:
      g_value_set_uint(value, mongo_client_get_timeout(client));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

/**
 * mongo_client_set_property:
 * @object: (in): A #GObject.
 * @prop_id: (in): The property identifier.
 * @value: (in): The given property.
 * @pspec: (in): A #ParamSpec.
 *
 * Set a given #GObject property.
 */
static void
mongo_client_set_property (GObject      *object,
                           guint         prop_id,
                           const GValue *value,
                           GParamSpec   *pspec)
{
   MongoClient *client = MONGO_CLIENT(object);

   switch (prop_id) {
   case PROP_HOST:
      mongo_client_set_host(client, g_value_get_string(value));
      break;
   case PROP_PORT:
      mongo_client_set_port(client, g_value_get_uint(value));
      break;
   case PROP_TIMEOUT:
      mongo_client_set_timeout(client, g_value_get_uint(value));
      break;
   default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, prop_id, pspec);
   }
}

/**
 * mongo_client_class_init:
 * @klass: (in): A #MongoClientClass.
 *
 * Initializes the #MongoClientClass and prepares the vtable.
 */
static void
mongo_client_class_init (MongoClientClass *klass)
{
   GObjectClass *object_class;

   object_class = G_OBJECT_CLASS(klass);
   object_class->finalize = mongo_client_finalize;
   object_class->get_property = mongo_client_get_property;
   object_class->set_property = mongo_client_set_property;
   g_type_class_add_private(object_class, sizeof(MongoClientPrivate));

   gParamSpecs[PROP_HOST] =
      g_param_spec_string("host",
                          _("Host"),
                          _("The host to connect to."),
                          "localhost",
                          G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_HOST,
                                   gParamSpecs[PROP_HOST]);

   gParamSpecs[PROP_PORT] =
      g_param_spec_uint("port",
                        _("Port"),
                        _("The port to connect to."),
                        0,
                        G_MAXUSHORT,
                        27017,
                        G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_PORT,
                                   gParamSpecs[PROP_PORT]);

   gParamSpecs[PROP_TIMEOUT] =
      g_param_spec_uint("timeout",
                        _("Timeout"),
                        _("A timeout for the client connection."),
                        0,
                        G_MAXUINT,
                        0,
                        G_PARAM_READWRITE);
   g_object_class_install_property(object_class, PROP_TIMEOUT,
                                   gParamSpecs[PROP_TIMEOUT]);
}

/**
 * mongo_client_init:
 * @client: (in): A #MongoClient.
 *
 * Initializes the newly created #MongoClient instance.
 */
static void
mongo_client_init (MongoClient *client)
{
   client->priv = G_TYPE_INSTANCE_GET_PRIVATE(client, MONGO_TYPE_CLIENT,
                                              MongoClientPrivate);
   client->priv->state = MONGO_CLIENT_READY;
   mongo_client_set_host(client, "localhost");
   mongo_client_set_port(client, 27017);
}

GQuark
mongo_client_error_quark (void)
{
   return g_quark_from_static_string("mongo_client_error_quark");
}

GType
mongo_operation_get_type (void)
{
   static GType type_id = 0;
   static gsize initialized = FALSE;
   static const GEnumValue values[] = {
      { MONGO_OPERATION_UPDATE,       "MONGO_OPERATION_UPDATE",       "UPDATE" },
      { MONGO_OPERATION_INSERT,       "MONGO_OPERATION_INSERT",       "INSERT" },
      { MONGO_OPERATION_QUERY,        "MONGO_OPERATION_QUERY",        "QUERY" },
      { MONGO_OPERATION_GET_MORE,     "MONGO_OPERATION_GET_MORE",     "GET_MORE" },
      { MONGO_OPERATION_DELETE,       "MONGO_OPERATION_DELETE",       "DELETE" },
      { MONGO_OPERATION_KILL_CURSORS, "MONGO_OPERATION_KILL_CURSORS", "KILL_CURSORS" },
      { 0 }
   };

   if (g_once_init_enter(&initialized)) {
      type_id = g_enum_register_static("MongoOperation", values);
      g_once_init_leave(&initialized, TRUE);
   }

   return type_id;
}
