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

typedef struct _MongoBson MongoBson;

GType         mongo_bson_get_type         (void) G_GNUC_CONST;
const guint8 *mongo_bson_get_data         (MongoBson *bson,
                                           gsize     *length);
MongoBson    *mongo_bson_new              (void);
MongoBson    *mongo_bson_ref              (MongoBson *bson);
void          mongo_bson_unref            (MongoBson *bson);

void          mongo_bson_append_array     (MongoBson     *bson,
                                           const gchar   *field,
                                           MongoBson     *value);
void          mongo_bson_append_boolean   (MongoBson     *bson,
                                           const gchar   *field,
                                           gboolean       value);
void          mongo_bson_append_bson      (MongoBson     *bson,
                                           const gchar   *field,
                                           MongoBson     *value);
void          mongo_bson_append_date_time (MongoBson     *bson,
                                           const gchar   *field,
                                           GDateTime     *value);
void          mongo_bson_append_double    (MongoBson     *bson,
                                           const gchar   *field,
                                           gdouble        value);
void          mongo_bson_append_int       (MongoBson     *bson,
                                           const gchar   *field,
                                           gint32         value);
void          mongo_bson_append_int64     (MongoBson     *bson,
                                           const gchar   *field,
                                           gint64         value);
void          mongo_bson_append_null      (MongoBson     *bson,
                                           const gchar   *field);
void          mongo_bson_append_object_id (MongoBson     *bson,
                                           const gchar   *field,
                                           MongoObjectId *object_id);
void          mongo_bson_append_regex     (MongoBson     *bson,
                                           const gchar   *field,
                                           const gchar   *regex,
                                           const gchar   *options);
void          mongo_bson_append_string    (MongoBson     *bson,
                                           const gchar   *field,
                                           const gchar   *value);
void          mongo_bson_append_timeval   (MongoBson     *bson,
                                           const gchar   *field,
                                           GTimeVal      *value);
void          mongo_bson_append_undefined (MongoBson     *bson,
                                           const gchar   *field);

G_END_DECLS

#endif /* MONGO_BSON_H */

