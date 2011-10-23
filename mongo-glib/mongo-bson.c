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

enum
{
   BSON_DOUBLE    = 0x01,
   BSON_UTF8      = 0x02,
   BSON_DOCUMENT  = 0x03,
   BSON_ARRAY     = 0x04,

   BSON_UNDEFINED = 0x06,
   BSON_OBJECT_ID = 0x07,
   BSON_BOOLEAN   = 0x08,
   BSON_DATE_TIME = 0x09,
   BSON_NULL      = 0x0A,
   BSON_REGEX     = 0x0B,

   BSON_INT32     = 0x10,

   BSON_INT64     = 0x12,
};

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

static void
mongo_bson_append (MongoBson    *bson,
                   guint8        type,
                   const gchar  *field,
                   const guint8 *data1,
                   gsize         len1,
                   const guint8 *data2,
                   gsize         len2)
{
   const guint8 trailing = 0;
   gint32 doc_len;
   gsize field_len;

   g_return_if_fail(bson != NULL);
   g_return_if_fail(type != 0);
   g_return_if_fail(field != NULL);
   g_return_if_fail(data1 != NULL || len1 == 0);
   g_return_if_fail(data2 != NULL || len2 == 0);

   /* Drop our trailing null byte. */
   bson->buf->len--;

   /* Type declaration */
   g_byte_array_append(bson->buf, &type, 1);

   /* Field name as cstring */
   field_len = strlen(field) + 1;
   g_byte_array_append(bson->buf, (guint8 *)field, field_len);

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
   doc_len += 1 + field_len + len1 + len2;
   *(gint32 *)bson->buf->data = GINT_TO_LE(doc_len);

   /* Add trailing NULL byte */
   g_byte_array_append(bson->buf, &trailing, 1);
}

void
mongo_bson_append_array (MongoBson   *bson,
                         const gchar *field,
                         MongoBson   *value)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);
   g_return_if_fail(value != NULL);

   mongo_bson_append(bson, BSON_ARRAY, field,
                     value->buf->data, value->buf->len,
                     NULL, 0);
}

void
mongo_bson_append_boolean (MongoBson   *bson,
                           const gchar *field,
                           gboolean     value)
{
   guint8 b = !!value;

   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);

   mongo_bson_append(bson, BSON_BOOLEAN, field, &b, 1, NULL, 0);
}

void
mongo_bson_append_bson (MongoBson   *bson,
                        const gchar *field,
                        MongoBson   *value)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);
   g_return_if_fail(value != NULL);

   mongo_bson_append(bson, BSON_DOCUMENT, field,
                     value->buf->data, value->buf->len,
                     NULL, 0);
}

void
mongo_bson_append_date_time (MongoBson   *bson,
                             const gchar *field,
                             GDateTime   *value)
{
   GTimeVal tv;

   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);
   g_return_if_fail(value != NULL);

   if (!g_date_time_to_timeval(value, &tv)) {
      g_warning("GDateTime is outside of storable range, ignoring!");
      return;
   }

   mongo_bson_append_timeval(bson, field, &tv);
}

void
mongo_bson_append_double (MongoBson   *bson,
                          const gchar *field,
                          gdouble      value)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);

   mongo_bson_append(bson, BSON_DOUBLE, field,
                     (const guint8 *)&value, sizeof value,
                     NULL, 0);
}

void
mongo_bson_append_int (MongoBson   *bson,
                       const gchar *field,
                       gint32       value)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);

   mongo_bson_append(bson, BSON_INT32, field,
                     (const guint8 *)&value, sizeof value,
                     NULL, 0);
}

void
mongo_bson_append_int64 (MongoBson   *bson,
                         const gchar *field,
                         gint64       value)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);

   mongo_bson_append(bson, BSON_INT64, field,
                     (const guint8 *)&value, sizeof value,
                     NULL, 0);
}

void
mongo_bson_append_null (MongoBson   *bson,
                        const gchar *field)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);

   mongo_bson_append(bson, BSON_NULL, field, NULL, 0, NULL, 0);
}

void
mongo_bson_append_object_id (MongoBson     *bson,
                             const gchar   *field,
                             MongoObjectId *object_id)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);
   g_return_if_fail(object_id != NULL);

   mongo_bson_append(bson, BSON_OBJECT_ID, field,
                     (const guint8 *)object_id, 12,
                     NULL, 0);
}

void
mongo_bson_append_regex (MongoBson   *bson,
                         const gchar *field,
                         const gchar *regex,
                         const gchar *options)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);
   g_return_if_fail(regex != NULL);

   if (!options) {
      options = "";
   }

   mongo_bson_append(bson, BSON_REGEX, field,
                     (const guint8 *)regex, strlen(regex) + 1,
                     (const guint8 *)options, strlen(options) + 1);
}

void
mongo_bson_append_string (MongoBson   *bson,
                          const gchar *field,
                          const gchar *value)
{
   gint32 value_len;

   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);

   value = value ? value : "";
   value_len = strlen(value) + 1;

   mongo_bson_append(bson, BSON_UTF8, field,
                     (const guint8 *)&value_len, sizeof value_len,
                     (const guint8 *)value, value_len);
}

void
mongo_bson_append_timeval (MongoBson   *bson,
                           const gchar *field,
                           GTimeVal    *value)
{
   guint64 msec;

   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);
   g_return_if_fail(value != NULL);

   msec = (value->tv_sec * 1000) + (value->tv_usec / 1000);
   mongo_bson_append(bson, BSON_DATE_TIME, field,
                     (const guint8 *)&msec, sizeof msec,
                     NULL, 0);
}

void
mongo_bson_append_undefined (MongoBson   *bson,
                             const gchar *field)
{
   g_return_if_fail(bson != NULL);
   g_return_if_fail(field != NULL);

   mongo_bson_append(bson, BSON_UNDEFINED, field,
                     NULL, 0, NULL, 0);
}
