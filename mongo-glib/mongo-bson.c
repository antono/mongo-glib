/* mongo-bson.c
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

#include "mongo-bson.h"

struct _MongoBson
{
   volatile gint ref_count;
   GByteArray *buf;
};

#define ENSURE_TYPE(bson, offset, TYPE, ret) \
   G_STMT_START { \
      if ((bson)->buf->data[(offset)] != TYPE) { \
         g_warning("Request for invalid type"); \
         return ret; \
      } \
   } G_STMT_END

/**
 * mongo_bson_dispose:
 * @bson: A #MongoBson.
 *
 * Cleans up @bson and free allocated resources.
 */
static void
mongo_bson_dispose (MongoBson *bson)
{
   g_byte_array_free(bson->buf, TRUE);
}

MongoBson *
mongo_bson_new_from_data (const guint8 *buffer,
                          gsize         max_length)
{
   MongoBson *bson;
   gint32 length;

   g_return_val_if_fail(buffer != NULL, NULL);

   length = *(gint32 *)buffer;
   if (length > max_length) {
      return NULL;
   }

   bson = g_slice_new0(MongoBson);
   bson->ref_count = 1;

   bson->buf = g_byte_array_sized_new(length);
   g_byte_array_append(bson->buf, buffer, length);

   return bson;
}

/**
 * mongo_bson_new:
 *
 * Creates a new instance of #MongoBson.
 *
 * Returns: A #MongoBson that should be freed with mongo_bson_unref().
 */
MongoBson *
mongo_bson_new (void)
{
   MongoBson *bson;
   gint32 len = 5;
   guint8 trailing = 0;

   bson = g_slice_new0(MongoBson);
   bson->ref_count = 1;
   bson->buf = g_byte_array_sized_new(16);

   g_byte_array_append(bson->buf, (guint8 *)&len, sizeof len);
   g_byte_array_append(bson->buf, (guint8 *)&trailing, sizeof trailing);

   g_assert(bson);
   g_assert(bson->buf);
   g_assert_cmpint(bson->buf->len, ==, 5);

   return bson;
}

/**
 * mongo_bson_ref:
 * @bson: (in): A #MongoBson.
 *
 * Atomically increments the reference count of @bson by one.
 *
 * Returns: (transfer full): @bson.
 */
MongoBson *
mongo_bson_ref (MongoBson *bson)
{
   g_return_val_if_fail(bson != NULL, NULL);
   g_return_val_if_fail(bson->ref_count > 0, NULL);

   g_atomic_int_inc(&bson->ref_count);
   return bson;
}

/**
 * mongo_bson_unref:
 * @bson: A MongoBson.
 *
 * Atomically decrements the reference count of @bson by one.  When the
 * reference count reaches zero, the structure will be destroyed and
 * freed.
 */
void
mongo_bson_unref (MongoBson *bson)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(bson->ref_count > 0);

   if (g_atomic_int_dec_and_test(&bson->ref_count)) {
      mongo_bson_dispose(bson);
      g_slice_free(MongoBson, bson);
   }
}

/**
 * mongo_bson_get_data:
 * @bson: (in): A #MongoBson.
 * @length: (out): A location for the buffer length.
 *
 * Fetches the BSON buffer.
 *
 * Returns: (transfer none): The #MongoBson buffer.
 */
const guint8 *
mongo_bson_get_data (MongoBson *bson,
                     gsize     *length)
{
   g_return_val_if_fail(bson != NULL, NULL);
   g_return_val_if_fail(length != NULL, NULL);

   *length = bson->buf->len;
   return bson->buf->data;
}

/**
 * mongo_bson_get_type:
 *
 * Retrieve the #GType for the #MongoBson boxed type.
 *
 * Returns: A #GType.
 */
GType
mongo_bson_get_type (void)
{
   static GType type_id = 0;
   static gsize initialized = FALSE;

   if (g_once_init_enter(&initialized)) {
      type_id = g_boxed_type_register_static("MongoBson",
         (GBoxedCopyFunc)mongo_bson_ref,
         (GBoxedFreeFunc)mongo_bson_unref);
      g_once_init_leave(&initialized, TRUE);
   }

   return type_id;
}

GType
mongo_bson_type_get_type (void)
{
   static GType type_id = 0;
   static gsize initialized = FALSE;
   static GEnumValue values[] = {
      { MONGO_BSON_DOUBLE, "MONGO_BSON_DOUBLE", "DOUBLE" },
      { MONGO_BSON_UTF8, "MONGO_BSON_UTF8", "UTF8" },
      { MONGO_BSON_DOCUMENT, "MONGO_BSON_DOCUMENT", "DOCUMENT" },
      { MONGO_BSON_ARRAY, "MONGO_BSON_ARRAY", "ARRAY" },
      { MONGO_BSON_UNDEFINED, "MONGO_BSON_UNDEFINED", "UNDEFINED" },
      { MONGO_BSON_OBJECT_ID, "MONGO_BSON_OBJECT_ID", "OBJECT_ID" },
      { MONGO_BSON_BOOLEAN, "MONGO_BSON_BOOLEAN", "BOOLEAN" },
      { MONGO_BSON_DATE_TIME, "MONGO_BSON_DATE_TIME", "DATE_TIME" },
      { MONGO_BSON_NULL, "MONGO_BSON_NULL", "NULL" },
      { MONGO_BSON_REGEX, "MONGO_BSON_REGEX", "REGEX" },
      { MONGO_BSON_INT32, "MONGO_BSON_INT32", "INT32" },
      { MONGO_BSON_INT64, "MONGO_BSON_INT64", "INT64" },
      { 0 }
   };

   if (g_once_init_enter(&initialized)) {
      type_id = g_enum_register_static("MongoBsonType", values);
      g_once_init_leave(&initialized, TRUE);
   }

   return type_id;
}

static void
mongo_bson_append (MongoBson    *bson,
                   guint8        type,
                   const gchar  *key,
                   const guint8 *data1,
                   gsize         len1,
                   const guint8 *data2,
                   gsize         len2)
{
   const guint8 trailing = 0;
   gint32 doc_len;
   gsize key_len;

   g_return_if_fail(bson != NULL);
   g_return_if_fail(type != 0);
   g_return_if_fail(key != NULL);
   g_return_if_fail(g_utf8_validate(key, -1, NULL));
   g_return_if_fail(data1 != NULL || len1 == 0);
   g_return_if_fail(data2 != NULL || len2 == 0);

   /* Drop our trailing null byte. */
   bson->buf->len--;

   /* Type declaration */
   g_byte_array_append(bson->buf, &type, 1);

   /* Field name as cstring */
   key_len = strlen(key) + 1;
   g_byte_array_append(bson->buf, (guint8 *)key, key_len);

   /* Add first data section if needed */
   if (data1) {
      g_byte_array_append(bson->buf, data1, len1);
   }

   /* Add second data section if needed */
   if (data2) {
      g_byte_array_append(bson->buf, data2, len2);
   }

   /* Update document length */
   doc_len = GINT_FROM_LE(*(gint32 *)bson->buf->data);
   doc_len += 1 + key_len + len1 + len2;
   *(gint32 *)bson->buf->data = GINT_TO_LE(doc_len);

   /* Add trailing NULL byte */
   g_byte_array_append(bson->buf, &trailing, 1);
}

void
mongo_bson_append_array (MongoBson   *bson,
                         const gchar *key,
                         MongoBson   *value)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);
   g_return_if_fail(value != NULL);

   mongo_bson_append(bson, MONGO_BSON_ARRAY, key,
                     value->buf->data, value->buf->len,
                     NULL, 0);
}

void
mongo_bson_append_boolean (MongoBson   *bson,
                           const gchar *key,
                           gboolean     value)
{
   guint8 b = !!value;

   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);

   mongo_bson_append(bson, MONGO_BSON_BOOLEAN, key, &b, 1, NULL, 0);
}

void
mongo_bson_append_bson (MongoBson   *bson,
                        const gchar *key,
                        MongoBson   *value)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);
   g_return_if_fail(value != NULL);

   mongo_bson_append(bson, MONGO_BSON_DOCUMENT, key,
                     value->buf->data, value->buf->len,
                     NULL, 0);
}

void
mongo_bson_append_date_time (MongoBson   *bson,
                             const gchar *key,
                             GDateTime   *value)
{
   GTimeVal tv;

   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);
   g_return_if_fail(value != NULL);

   if (!g_date_time_to_timeval(value, &tv)) {
      g_warning("GDateTime is outside of storable range, ignoring!");
      return;
   }

   mongo_bson_append_timeval(bson, key, &tv);
}

void
mongo_bson_append_double (MongoBson   *bson,
                          const gchar *key,
                          gdouble      value)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);

   mongo_bson_append(bson, MONGO_BSON_DOUBLE, key,
                     (const guint8 *)&value, sizeof value,
                     NULL, 0);
}

void
mongo_bson_append_int (MongoBson   *bson,
                       const gchar *key,
                       gint32       value)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);

   mongo_bson_append(bson, MONGO_BSON_INT32, key,
                     (const guint8 *)&value, sizeof value,
                     NULL, 0);
}

void
mongo_bson_append_int64 (MongoBson   *bson,
                         const gchar *key,
                         gint64       value)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);

   mongo_bson_append(bson, MONGO_BSON_INT64, key,
                     (const guint8 *)&value, sizeof value,
                     NULL, 0);
}

void
mongo_bson_append_null (MongoBson   *bson,
                        const gchar *key)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);

   mongo_bson_append(bson, MONGO_BSON_NULL, key, NULL, 0, NULL, 0);
}

void
mongo_bson_append_object_id (MongoBson     *bson,
                             const gchar   *key,
                             MongoObjectId *object_id)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);
   g_return_if_fail(object_id != NULL);

   mongo_bson_append(bson, MONGO_BSON_OBJECT_ID, key,
                     (const guint8 *)object_id, 12,
                     NULL, 0);
}

void
mongo_bson_append_regex (MongoBson   *bson,
                         const gchar *key,
                         const gchar *regex,
                         const gchar *options)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);
   g_return_if_fail(regex != NULL);

   if (!options) {
      options = "";
   }

   mongo_bson_append(bson, MONGO_BSON_REGEX, key,
                     (const guint8 *)regex, strlen(regex) + 1,
                     (const guint8 *)options, strlen(options) + 1);
}

void
mongo_bson_append_string (MongoBson   *bson,
                          const gchar *key,
                          const gchar *value)
{
   gint32 value_len;

   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);
   g_return_if_fail(g_utf8_validate(value, -1, NULL));

   value = value ? value : "";
   value_len = strlen(value) + 1;

   mongo_bson_append(bson, MONGO_BSON_UTF8, key,
                     (const guint8 *)&value_len, sizeof value_len,
                     (const guint8 *)value, value_len);
}

void
mongo_bson_append_timeval (MongoBson   *bson,
                           const gchar *key,
                           GTimeVal    *value)
{
   guint64 msec;

   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);
   g_return_if_fail(value != NULL);

   msec = (value->tv_sec * 1000) + (value->tv_usec / 1000);
   mongo_bson_append(bson, MONGO_BSON_DATE_TIME, key,
                     (const guint8 *)&msec, sizeof msec,
                     NULL, 0);
}

void
mongo_bson_append_undefined (MongoBson   *bson,
                             const gchar *key)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(key != NULL);

   mongo_bson_append(bson, MONGO_BSON_UNDEFINED, key,
                     NULL, 0, NULL, 0);
}

/**
 * mongo_bson_iter_init:
 * @iter: an uninitialized #MongoBsonIter.
 * @bson: a #MongoBson.
 *
 * Initializes a #MongoBsonIter for iterating through a #MongoBson document.
 */
void
mongo_bson_iter_init (MongoBsonIter *iter,
                      MongoBson     *bson)
{
   g_return_if_fail(iter != NULL);
   g_return_if_fail(bson != NULL);

   memset(iter, 0, sizeof *iter);
   iter->user_data1 = bson;
}

gboolean
mongo_bson_iter_find (MongoBsonIter *iter,
                      const gchar   *key)
{
   const gchar *cur_key;

   g_return_val_if_fail(iter != NULL, FALSE);
   g_return_val_if_fail(key != NULL, FALSE);

   while (mongo_bson_iter_next(iter)) {
      cur_key = mongo_bson_iter_get_key(iter);
      if (!g_strcmp0(cur_key, key)) {
         return TRUE;
      }
   }

   return FALSE;
}

const gchar *
mongo_bson_iter_get_key (MongoBsonIter *iter)
{
   const gchar *key;
   MongoBson *bson;
   gsize offset;

   g_return_val_if_fail(iter != NULL, NULL);
   g_return_val_if_fail(iter->user_data1 != NULL, NULL);
   g_return_val_if_fail(iter->user_data2 != NULL, NULL);

   bson = iter->user_data1;
   offset = (gsize)iter->user_data2;
   key = (gchar *)&bson->buf->data[offset + 1];

   return key;
}

MongoBson *
mongo_bson_iter_get_value_array (MongoBsonIter *iter)
{
   //const guint8 *buf;
   //MongoBson *bson;
   MongoBson *array = NULL;
   //gsize value_offset;

   g_return_val_if_fail(iter != NULL, NULL);
   g_return_val_if_fail(iter->user_data1 != NULL, NULL);
   g_return_val_if_fail(iter->user_data2 != NULL, NULL);
   g_return_val_if_fail(iter->user_data3 != NULL, NULL);

   //bson = iter->user_data1;
   //value_offset = (gsize)iter->user_data3;
   //buf = (MongoBson *)&bson->buf->data[value_offset];

   //array = mongo_bson_new();
   //array->buf =

   /*
    * copy buffer into new structure
    */

   return array;
}

gboolean
mongo_bson_iter_get_value_boolean (MongoBsonIter *iter)
{
   MongoBson *bson;
   gboolean value;
   gsize value_offset;
   gsize offset;

   g_return_val_if_fail(iter != NULL, 0);
   g_return_val_if_fail(iter->user_data1 != NULL, 0);
   g_return_val_if_fail(iter->user_data2 != NULL, 0);
   g_return_val_if_fail(iter->user_data3 != NULL, 0);

   bson = iter->user_data1;
   offset = (gsize)iter->user_data2;
   ENSURE_TYPE(bson, offset, MONGO_BSON_BOOLEAN, 0);
   value_offset = (gsize)iter->user_data3;
   value = *(guint8 *)&bson->buf->data[value_offset];

   return value;
}

MongoBson *
mongo_bson_iter_get_value_bson (MongoBsonIter *iter)
{
   const guint8 *buf;
   MongoBson *bson;
   MongoBson *child;
   gsize offset;
   gsize value_offset;

   g_return_val_if_fail(iter != NULL, NULL);
   g_return_val_if_fail(iter->user_data1 != NULL, NULL);
   g_return_val_if_fail(iter->user_data2 != NULL, NULL);
   g_return_val_if_fail(iter->user_data3 != NULL, NULL);

   bson = iter->user_data1;
   offset = (gsize)iter->user_data2;
   ENSURE_TYPE(bson, offset, MONGO_BSON_DOCUMENT, NULL);
   value_offset = (gsize)iter->user_data3;
   buf = (const guint8 *)&bson->buf->data[value_offset];
   child = mongo_bson_new_from_data(buf, bson->buf->len - value_offset);

   return child;
}

GDateTime *
mongo_bson_iter_get_value_date_time (MongoBsonIter *iter)
{
   GTimeVal tv;

   g_return_val_if_fail(iter != NULL, NULL);

   mongo_bson_iter_get_value_timeval(iter, &tv);
   return g_date_time_new_from_timeval_utc(&tv);
}

gdouble
mongo_bson_iter_get_value_double (MongoBsonIter *iter)
{
   MongoBson *bson;
   gdouble value;
   gsize value_offset;
   gsize offset;

   g_return_val_if_fail(iter != NULL, 0);
   g_return_val_if_fail(iter->user_data1 != NULL, 0);
   g_return_val_if_fail(iter->user_data2 != NULL, 0);
   g_return_val_if_fail(iter->user_data3 != NULL, 0);

   bson = iter->user_data1;
   offset = (gsize)iter->user_data2;
   ENSURE_TYPE(bson, offset, MONGO_BSON_DOUBLE, 0);
   value_offset = (gsize)iter->user_data3;
   value = *(gdouble *)&bson->buf->data[value_offset];

   return value;
}

MongoObjectId *
mongo_bson_iter_get_value_object_id (MongoBsonIter *iter)
{
   MongoObjectId *object_id;
   MongoBson *bson;
   gsize offset;
   gsize value_offset;

   g_return_val_if_fail(iter != NULL, NULL);
   g_return_val_if_fail(iter->user_data1 != NULL, NULL);

   bson = iter->user_data1;
   offset = (gsize)iter->user_data2;
   ENSURE_TYPE(bson, offset, MONGO_BSON_INT32, 0);
   value_offset = (gsize)iter->user_data3;
   object_id = mongo_object_id_new_from_data(&bson->buf->data[value_offset]);

   return object_id;
}

gint32
mongo_bson_iter_get_value_int (MongoBsonIter *iter)
{
   MongoBson *bson;
   gint32 value;
   gsize value_offset;
   gsize offset;

   g_return_val_if_fail(iter != NULL, 0);
   g_return_val_if_fail(iter->user_data1 != NULL, 0);
   g_return_val_if_fail(iter->user_data2 != NULL, 0);
   g_return_val_if_fail(iter->user_data3 != NULL, 0);

   bson = iter->user_data1;
   offset = (gsize)iter->user_data2;
   ENSURE_TYPE(bson, offset, MONGO_BSON_INT32, 0);
   value_offset = (gsize)iter->user_data3;
   value = *(gint32 *)&bson->buf->data[value_offset];

   return value;
}

gint64
mongo_bson_iter_get_value_int64 (MongoBsonIter *iter)
{
   MongoBson *bson;
   gint64 value;
   gsize value_offset;
   gsize offset;

   g_return_val_if_fail(iter != NULL, 0);
   g_return_val_if_fail(iter->user_data1 != NULL, 0);
   g_return_val_if_fail(iter->user_data2 != NULL, 0);
   g_return_val_if_fail(iter->user_data3 != NULL, 0);

   bson = iter->user_data1;
   offset = (gsize)iter->user_data2;
   ENSURE_TYPE(bson, offset, MONGO_BSON_INT64, 0);
   value_offset = (gsize)iter->user_data3;
   value = *(gint64 *)&bson->buf->data[value_offset];

   return value;
}

void
mongo_bson_iter_get_value_regex (MongoBsonIter  *iter,
                                 const gchar   **regex,
                                 const gchar   **options)
{
   g_return_if_fail(iter != NULL);

}

const gchar *
mongo_bson_iter_get_value_string (MongoBsonIter *iter)
{
   const gchar *value;
   MongoBson *bson;
   gsize value_offset;
   gsize offset;

   g_return_val_if_fail(iter != NULL, NULL);
   g_return_val_if_fail(iter->user_data1 != NULL, NULL);
   g_return_val_if_fail(iter->user_data2 != NULL, NULL);
   g_return_val_if_fail(iter->user_data3 != NULL, NULL);

   bson = iter->user_data1;
   offset = (gsize)iter->user_data2;
   ENSURE_TYPE(bson, offset, MONGO_BSON_UTF8, NULL);
   value_offset = (gsize)iter->user_data3;
   value = (const gchar *)&bson->buf->data[value_offset];

   return value;
}

void
mongo_bson_iter_get_value_timeval (MongoBsonIter *iter,
                                   GTimeVal      *value)
{
   MongoBson *bson;
   gint64 v_int64;
   gsize value_offset;
   gsize offset;

   g_return_if_fail(iter != NULL);
   g_return_if_fail(value != NULL);
   g_return_if_fail(iter->user_data1 != NULL);
   g_return_if_fail(iter->user_data2 != NULL);
   g_return_if_fail(iter->user_data3 != NULL);

   bson = iter->user_data1;
   offset = (gsize)iter->user_data2;
   ENSURE_TYPE(bson, offset, MONGO_BSON_UTF8, );
   value_offset = (gsize)iter->user_data3;
   v_int64 = *(gint64 *)&bson->buf->data[value_offset];

   value->tv_sec = v_int64 / 1000;
   value->tv_usec = v_int64 % 1000;
}

MongoBsonType
mongo_bson_iter_get_value_type (MongoBsonIter *iter)
{
   MongoBsonType type;
   MongoBson *bson;
   gsize offset;

   g_return_val_if_fail(iter != NULL, 0);
   g_return_val_if_fail(iter->user_data1 != NULL, 0);
   g_return_val_if_fail(iter->user_data2 != NULL, 0);

   bson = iter->user_data1;
   offset = (gsize)iter->user_data2;
   type = bson->buf->data[offset];

   switch (type) {
   case MONGO_BSON_DOUBLE:
   case MONGO_BSON_UTF8:
   case MONGO_BSON_DOCUMENT:
   case MONGO_BSON_ARRAY:
   case MONGO_BSON_UNDEFINED:
   case MONGO_BSON_OBJECT_ID:
   case MONGO_BSON_BOOLEAN:
   case MONGO_BSON_DATE_TIME:
   case MONGO_BSON_NULL:
   case MONGO_BSON_REGEX:
   case MONGO_BSON_INT32:
   case MONGO_BSON_INT64:
      return type;
   default:
      g_warning("Unknown bson type %0d", type);
      return 0;
   }
}

/**
 * mongo_bson_iter_recurse:
 * @iter: (in): A #MongoBsonIter.
 * @child: (out): A #MongoBsonIter.
 *
 * Recurses into the child BSON document found at the key currently observed
 * by the #MongoBsonIter. The @child #MongoBsonIter is initialized.
 *
 * Returns: %TRUE if @child is initialized; otherwise %FALSE.
 */
gboolean
mongo_bson_iter_recurse (MongoBsonIter *iter,
                         MongoBsonIter *child)
{
   /*
    * TODO:
    */
}

gboolean
mongo_bson_iter_next (MongoBsonIter *iter)
{
   MongoBsonType cur_type;
   MongoBson *bson;
   gboolean ret = FALSE;
   gsize offset;
   gsize value_offset;

   g_return_val_if_fail(iter != NULL, FALSE);
   g_return_val_if_fail(iter->user_data1 != NULL, FALSE);

   bson = iter->user_data1;
   offset = (gsize)iter->user_data2;
   value_offset = (gsize)iter->user_data3;

   if (!offset) {
      offset = 4;
      value_offset = ...;
   }

   if ((offset >= bson->buf->len) || (bson->buf->data[offset] == '\0')) {
      return FALSE;
   }

   cur_type = bson->buf[offset];
   offset = value_offset;

   switch (cur_type) {
   case MONGO_BSON_STRING:
      break;
   case MONGO_BSON_INT32:
      offset += 4;
      break;
   case MONGO_BSON_INT64:
      offset += 8;
      break;
   default:
      /* Poorly formatted */
      return FALSE;
   }

   iter->user_data2 = GSIZE_TO_POINTER(offset);
   iter->user_data3 = GSIZE_TO_POINTER(value_offset);

   ret = TRUE;

   return ret;
}
