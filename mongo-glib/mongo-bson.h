/* mongo-bson.h
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

#ifndef MONGO_BSON_H
#define MONGO_BSON_H

#include <glib-object.h>

#include "mongo-object-id.h"

G_BEGIN_DECLS

#define MONGO_TYPE_BSON (mongo_bson_get_type())

typedef struct _MongoBson     MongoBson;
typedef struct _MongoBsonIter MongoBsonIter;
typedef enum   _MongoBsonType MongoBsonType;

enum _MongoBsonType
{
   MONGO_BSON_DOUBLE    = 0x01,
   MONGO_BSON_UTF8      = 0x02,
   MONGO_BSON_DOCUMENT  = 0x03,
   MONGO_BSON_ARRAY     = 0x04,

   MONGO_BSON_UNDEFINED = 0x06,
   MONGO_BSON_OBJECT_ID = 0x07,
   MONGO_BSON_BOOLEAN   = 0x08,
   MONGO_BSON_DATE_TIME = 0x09,
   MONGO_BSON_NULL      = 0x0A,
   MONGO_BSON_REGEX     = 0x0B,

   MONGO_BSON_INT32     = 0x10,

   MONGO_BSON_INT64     = 0x12,
};

struct _MongoBsonIter
{
   /*< private >*/
   gpointer user_data1;
   gpointer user_data2;
   gpointer user_data3;
   gpointer user_data4;
};

GType          mongo_bson_get_type                 (void) G_GNUC_CONST;
GType          mongo_bson_type_get_type            (void) G_GNUC_CONST;
const guint8  *mongo_bson_get_data                 (MongoBson      *bson,
                                                    gsize          *length);
MongoBson     *mongo_bson_new                      (void);
MongoBson     *mongo_bson_new_from_data            (const guint8   *buffer,
                                                    gsize           length);
MongoBson     *mongo_bson_ref                      (MongoBson      *bson);
void           mongo_bson_unref                    (MongoBson      *bson);
void           mongo_bson_append_array             (MongoBson      *bson,
                                                    const gchar    *key,
                                                    MongoBson      *value);
void           mongo_bson_append_boolean           (MongoBson      *bson,
                                                    const gchar    *key,
                                                    gboolean       value);
void           mongo_bson_append_bson              (MongoBson      *bson,
                                                    const gchar    *key,
                                                    MongoBson      *value);
void           mongo_bson_append_date_time         (MongoBson      *bson,
                                                    const gchar    *key,
                                                    GDateTime      *value);
void           mongo_bson_append_double            (MongoBson      *bson,
                                                    const gchar    *key,
                                                    gdouble         value);
void           mongo_bson_append_int               (MongoBson      *bson,
                                                    const gchar    *key,
                                                    gint32          value);
void           mongo_bson_append_int64             (MongoBson      *bson,
                                                    const gchar    *key,
                                                    gint64          value);
void           mongo_bson_append_null              (MongoBson      *bson,
                                                    const gchar    *key);
void           mongo_bson_append_object_id         (MongoBson      *bson,
                                                    const gchar    *key,
                                                    MongoObjectId  *object_id);
void           mongo_bson_append_regex             (MongoBson      *bson,
                                                    const gchar    *key,
                                                    const gchar    *regex,
                                                    const gchar    *options);
void           mongo_bson_append_string            (MongoBson      *bson,
                                                    const gchar    *key,
                                                    const gchar    *value);
void           mongo_bson_append_timeval           (MongoBson      *bson,
                                                    const gchar    *key,
                                                    GTimeVal       *value);
void           mongo_bson_append_undefined         (MongoBson      *bson,
                                                    const gchar    *key);
void           mongo_bson_iter_init                (MongoBsonIter  *iter,
                                                    MongoBson      *bson);
gboolean       mongo_bson_iter_find                (MongoBsonIter  *iter,
                                                    const gchar    *key);
const gchar   *mongo_bson_iter_get_key             (MongoBsonIter  *iter);
MongoBson     *mongo_bson_iter_get_value_array     (MongoBsonIter  *iter);
gboolean       mongo_bson_iter_get_value_boolean   (MongoBsonIter  *iter);
MongoBson     *mongo_bson_iter_get_value_bson      (MongoBsonIter  *iter);
GDateTime     *mongo_bson_iter_get_value_date_time (MongoBsonIter  *iter);
gdouble        mongo_bson_iter_get_value_double    (MongoBsonIter  *iter);
MongoObjectId *mongo_bson_iter_get_value_object_id (MongoBsonIter  *iter);
gint32         mongo_bson_iter_get_value_int       (MongoBsonIter  *iter);
gint64         mongo_bson_iter_get_value_int64     (MongoBsonIter  *iter);
void           mongo_bson_iter_get_value_regex     (MongoBsonIter  *iter,
                                                    const gchar   **regex,
                                                    const gchar   **options);
const gchar   *mongo_bson_iter_get_value_string    (MongoBsonIter  *iter);
void           mongo_bson_iter_get_value_timeval   (MongoBsonIter  *iter,
                                                    GTimeVal       *value);
MongoBsonType  mongo_bson_iter_get_value_type      (MongoBsonIter  *iter);
gboolean       mongo_bson_iter_next                (MongoBsonIter  *iter);
gboolean       mongo_bson_iter_recurse             (MongoBsonIter  *iter,
                                                    MongoBsonIter  *child);


G_END_DECLS

#endif /* MONGO_BSON_H */

