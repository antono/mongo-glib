/* mongo-object-id.c
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

#include <string.h>

#include "mongo-object-id.h"

struct _MongoObjectId
{
   guint8 data[12];
};

MongoObjectId *
mongo_object_id_new_from_data (const guint8 *bytes)
{
   MongoObjectId *object_id;

   object_id = g_slice_new0(MongoObjectId);

   if (bytes) {
      memcpy(object_id, bytes, sizeof *object_id);
   }

   return object_id;
}

MongoObjectId *
mongo_object_id_copy (const MongoObjectId *object_id)
{
   MongoObjectId *copy;

   g_return_val_if_fail(object_id != NULL, NULL);

   copy = g_slice_new(MongoObjectId);
   memcpy(copy, object_id, sizeof *object_id);

   return copy;
}

void
mongo_object_id_free (MongoObjectId *object_id)
{
   if (object_id) {
      g_slice_free(MongoObjectId, object_id);
   }
}

GType
mongo_object_id_get_type (void)
{
   static GType type_id = 0;
   static gsize initialized = FALSE;

   if (g_once_init_enter(&initialized)) {
      type_id = g_boxed_type_register_static(
         "MongoObjectId",
         (GBoxedCopyFunc)mongo_object_id_copy,
         (GBoxedFreeFunc)mongo_object_id_free);
      g_once_init_leave(&initialized, TRUE);
   }

   return type_id;
}
