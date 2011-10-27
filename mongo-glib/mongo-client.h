/* mongo-client.h
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

#ifndef MONGO_CLIENT_H
#define MONGO_CLIENT_H

#include <gio/gio.h>

#include "mongo-bson.h"

G_BEGIN_DECLS

#define MONGO_TYPE_CLIENT            (mongo_client_get_type())
#define MONGO_TYPE_OPERATION         (mongo_operation_get_type())
#define MONGO_CLIENT(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MONGO_TYPE_CLIENT, MongoClient))
#define MONGO_CLIENT_CONST(obj)      (G_TYPE_CHECK_INSTANCE_CAST ((obj), MONGO_TYPE_CLIENT, MongoClient const))
#define MONGO_CLIENT_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass),  MONGO_TYPE_CLIENT, MongoClientClass))
#define MONGO_IS_CLIENT(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MONGO_TYPE_CLIENT))
#define MONGO_IS_CLIENT_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass),  MONGO_TYPE_CLIENT))
#define MONGO_CLIENT_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj),  MONGO_TYPE_CLIENT, MongoClientClass))
#define MONGO_CLIENT_ERROR           (mongo_client_error_quark())

typedef struct _MongoClient        MongoClient;
typedef struct _MongoClientClass   MongoClientClass;
typedef struct _MongoClientPrivate MongoClientPrivate;
typedef enum   _MongoClientError   MongoClientError;
typedef enum   _MongoOperation     MongoOperation;

enum _MongoClientError
{
   MONGO_CLIENT_ERROR_NOT_PRIMARY = 1,
};

enum _MongoOperation
{
   MONGO_OPERATION_UPDATE       = 2001,
   MONGO_OPERATION_INSERT       = 2002,
   MONGO_OPERATION_QUERY        = 2004,
   MONGO_OPERATION_GET_MORE     = 2005,
   MONGO_OPERATION_DELETE       = 2006,
   MONGO_OPERATION_KILL_CURSORS = 2007,
};

struct _MongoClient
{
   GObject parent;

   /*< private >*/
   MongoClientPrivate *priv;
};

struct _MongoClientClass
{
   GObjectClass parent_class;
};

void         mongo_client_add_peer       (MongoClient          *client,
                                          const gchar          *host,
                                          guint                 port);
void         mongo_client_connect_async  (MongoClient          *client,
                                          GCancellable         *cancellable,
                                          GAsyncReadyCallback   callback,
                                          gpointer              user_data);
gboolean     mongo_client_connect_finish (MongoClient          *client,
                                          GAsyncResult         *result,
                                          GError              **error);
GQuark       mongo_client_error_quark    (void) G_GNUC_CONST;
const gchar *mongo_client_get_host       (MongoClient          *client);
guint        mongo_client_get_port       (MongoClient          *client);
guint        mongo_client_get_timeout    (MongoClient          *client);
GType        mongo_client_get_type       (void) G_GNUC_CONST;
MongoClient *mongo_client_new            (void);
void         mongo_client_send_async     (MongoClient          *client,
                                          const gchar          *db,
                                          MongoBson            *bson,
                                          MongoOperation        operation,
                                          gboolean              want_reply,
                                          GAsyncReadyCallback   callback,
                                          gpointer              user_data);
MongoBson   *mongo_client_send_finish    (MongoClient          *client,
                                          GAsyncResult         *result,
                                          GError              **error);
void         mongo_client_set_host       (MongoClient          *client,
                                          const gchar          *host);
void         mongo_client_set_port       (MongoClient          *client,
                                          guint                 port);
void         mongo_client_set_timeout    (MongoClient          *client,
                                          guint                 timeout_msec);
GType        mongo_operation_get_type    (void) G_GNUC_CONST;

G_END_DECLS

#endif /* MONGO_CLIENT_H */
