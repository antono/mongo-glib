#include <mongo-glib/mongo-glib.h>

static void
assert_bson (MongoBson   *bson,
             const gchar *name)
{
   gchar *filename;
   const guint8 *bson_buffer = NULL;
   guint8 *buffer = NULL;
   GError *error = NULL;
   gsize bson_length = 0;
   gsize length = 0;
   guint i;

   filename = g_build_filename("tests", "bson", name, NULL);

   if (!g_file_get_contents(filename, (gchar **)&buffer, &length, &error)) {
      g_assert_no_error(error);
      g_assert(FALSE);
   }

   bson_buffer = mongo_bson_get_data(bson, &bson_length);

   g_assert_cmpint(length, ==, bson_length);

   for (i = 0; i < length; i++) {
      if (bson_buffer[i] != buffer[i]) {
         g_error("Expected 0x%02x at offset %d got 0x%02x.", buffer[i], i, bson_buffer[i]);
      }
   }

   g_free(filename);
   g_free(buffer);
}

static void
append_tests (void)
{
   MongoBson *bson;
   MongoBson *array;
   MongoBson *subdoc;
   GDateTime *dt;
   GTimeZone *tz;
   GTimeVal tv;

   bson = mongo_bson_new();
   mongo_bson_append_int(bson, "int", 1);
   assert_bson(bson, "test1.bson");
   mongo_bson_unref(bson);

   bson = mongo_bson_new();
   mongo_bson_append_int64(bson, "int64", 1);
   assert_bson(bson, "test2.bson");
   mongo_bson_unref(bson);

   bson = mongo_bson_new();
   mongo_bson_append_double(bson, "double", 1.123);
   assert_bson(bson, "test3.bson");
   mongo_bson_unref(bson);

   bson = mongo_bson_new();
   tz = g_time_zone_new("Z");
   dt = g_date_time_new(tz, 2011, 10, 22, 12, 13, 14.123);
   mongo_bson_append_date_time(bson, "utc", dt);
   assert_bson(bson, "test4.bson");
   mongo_bson_unref(bson);

   bson = mongo_bson_new();
   g_date_time_to_timeval(dt, &tv);
   mongo_bson_append_timeval(bson, "utc", &tv);
   assert_bson(bson, "test4.bson");
   mongo_bson_unref(bson);
   g_date_time_unref(dt);
   g_time_zone_unref(tz);

   bson = mongo_bson_new();
   mongo_bson_append_string(bson, "string", "some string");
   assert_bson(bson, "test5.bson");
   mongo_bson_unref(bson);

   bson = mongo_bson_new();
   array = mongo_bson_new();
   mongo_bson_append_int(array, "0", 1);
   mongo_bson_append_int(array, "1", 2);
   mongo_bson_append_int(array, "2", 3);
   mongo_bson_append_int(array, "3", 4);
   mongo_bson_append_int(array, "4", 5);
   mongo_bson_append_int(array, "5", 6);
   mongo_bson_append_array(bson, "array[int]", array);
   assert_bson(bson, "test6.bson");
   mongo_bson_unref(array);
   mongo_bson_unref(bson);

   bson = mongo_bson_new();
   array = mongo_bson_new();
   mongo_bson_append_double(array, "0", 1.123);
   mongo_bson_append_double(array, "1", 2.123);
   mongo_bson_append_array(bson, "array[double]", array);
   assert_bson(bson, "test7.bson");
   mongo_bson_unref(array);
   mongo_bson_unref(bson);

   bson = mongo_bson_new();
   subdoc = mongo_bson_new();
   mongo_bson_append_int(subdoc, "int", 1);
   mongo_bson_append_bson(bson, "document", subdoc);
   assert_bson(bson, "test8.bson");
   mongo_bson_unref(subdoc);
   mongo_bson_unref(bson);

   bson = mongo_bson_new();
   mongo_bson_append_null(bson, "null");
   assert_bson(bson, "test9.bson");
   mongo_bson_unref(bson);

   bson = mongo_bson_new();
   mongo_bson_append_regex(bson, "regex", "1234", "i");
   assert_bson(bson, "test10.bson");
   mongo_bson_unref(bson);

   bson = mongo_bson_new();
   mongo_bson_append_string(bson, "hello", "world");
   assert_bson(bson, "test11.bson");
   mongo_bson_unref(bson);

   bson = mongo_bson_new();
   array = mongo_bson_new();
   mongo_bson_append_string(array, "0", "awesome");
   mongo_bson_append_double(array, "1", 5.05);
   mongo_bson_append_int(array, "2", 1986);
   mongo_bson_append_array(bson, "BSON", array);
   assert_bson(bson, "test12.bson");
   mongo_bson_unref(bson);
   mongo_bson_unref(array);
}

gint
main (gint   argc,
      gchar *argv[])
{
   g_test_init(&argc, &argv, NULL);
   g_test_add_func("/MongoBson/append_tests", append_tests);
   return g_test_run();
}