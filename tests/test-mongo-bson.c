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

static MongoBson *
get_bson (const gchar *name)
{
   const gchar *filename = g_build_filename("tests", "bson", name, NULL);
   GError *error = NULL;
   gchar *buffer;
   gsize length;
   MongoBson *bson;

   if (!g_file_get_contents(filename, (gchar **)&buffer, &length, &error)) {
      g_assert_no_error(error);
      g_assert(FALSE);
   }

   bson = mongo_bson_new_from_data((const guint8 *)buffer, length);
   g_free(buffer);
   g_assert(bson);

   return bson;
}

static void
iter_tests (void)
{
   MongoBson *bson;
   MongoBsonIter iter;
   MongoBsonIter iter2;
   GDateTime *dt;
   GTimeVal tv;
   const gchar *regex = NULL;
   const gchar *options = NULL;

   bson = get_bson("test1.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_INT32, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("int", ==, mongo_bson_iter_get_key(&iter));
   g_assert_cmpint(1, ==, mongo_bson_iter_get_value_int(&iter));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test2.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_INT64, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("int64", ==, mongo_bson_iter_get_key(&iter));
   g_assert_cmpint(1L, ==, mongo_bson_iter_get_value_int64(&iter));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test3.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_DOUBLE, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("double", ==, mongo_bson_iter_get_key(&iter));
   g_assert_cmpint(1.123, ==, mongo_bson_iter_get_value_double(&iter));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test4.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_DATE_TIME, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("utc", ==, mongo_bson_iter_get_key(&iter));
   mongo_bson_iter_get_value_timeval(&iter, &tv);
   g_assert_cmpint(tv.tv_sec, ==, 1319285594);
   g_assert_cmpint(tv.tv_usec, ==, 123);
   dt = mongo_bson_iter_get_value_date_time(&iter);
   g_assert_cmpint(g_date_time_get_year(dt), ==, 2011);
   g_assert_cmpint(g_date_time_get_month(dt), ==, 10);
   g_assert_cmpint(g_date_time_get_day_of_month(dt), ==, 22);
   g_assert_cmpint(g_date_time_get_hour(dt), ==, 12);
   g_assert_cmpint(g_date_time_get_minute(dt), ==, 13);
   g_assert_cmpint(g_date_time_get_second(dt), ==, 14);
   g_assert_cmpint(g_date_time_get_microsecond(dt), ==, 123);
   g_date_time_unref(dt);
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test5.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_UTF8, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("string", ==, mongo_bson_iter_get_key(&iter));
   g_assert_cmpstr("some string", ==, mongo_bson_iter_get_value_string(&iter, NULL));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test6.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_ARRAY, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("array[int]", ==, mongo_bson_iter_get_key(&iter));
   g_assert(mongo_bson_iter_recurse(&iter, &iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("0", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpint(1, ==, mongo_bson_iter_get_value_int(&iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("1", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpint(2, ==, mongo_bson_iter_get_value_int(&iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("2", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpint(3, ==, mongo_bson_iter_get_value_int(&iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("3", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpint(4, ==, mongo_bson_iter_get_value_int(&iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("4", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpint(5, ==, mongo_bson_iter_get_value_int(&iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("5", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpint(6, ==, mongo_bson_iter_get_value_int(&iter2));
   g_assert(!mongo_bson_iter_next(&iter2));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test7.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_ARRAY, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("array[double]", ==, mongo_bson_iter_get_key(&iter));
   g_assert(mongo_bson_iter_recurse(&iter, &iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("0", ==, mongo_bson_iter_get_key(&iter2));
   g_assert(1.123 == mongo_bson_iter_get_value_double(&iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("1", ==, mongo_bson_iter_get_key(&iter2));
   g_assert(2.123 == mongo_bson_iter_get_value_double(&iter2));
   g_assert(!mongo_bson_iter_next(&iter2));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test8.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_DOCUMENT, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("document", ==, mongo_bson_iter_get_key(&iter));
   g_assert(mongo_bson_iter_recurse(&iter, &iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("int", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpint(1, ==, mongo_bson_iter_get_value_int(&iter2));
   g_assert(!mongo_bson_iter_next(&iter2));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test9.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_NULL, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("null", ==, mongo_bson_iter_get_key(&iter));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test10.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_REGEX, ==, mongo_bson_iter_get_value_type(&iter));
   mongo_bson_iter_get_value_regex(&iter, &regex, &options);
   g_assert_cmpstr(regex, ==, "1234");
   g_assert_cmpstr(options, ==, "i");
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test11.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_UTF8, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("hello", ==, mongo_bson_iter_get_key(&iter));
   g_assert_cmpstr("world", ==, mongo_bson_iter_get_value_string(&iter, NULL));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test12.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_ARRAY, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("BSON", ==, mongo_bson_iter_get_key(&iter));
   g_assert(mongo_bson_iter_recurse(&iter, &iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("0", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpstr("awesome", ==, mongo_bson_iter_get_value_string(&iter2, NULL));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("1", ==, mongo_bson_iter_get_key(&iter2));
   g_assert(5.05 == mongo_bson_iter_get_value_double(&iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("2", ==, mongo_bson_iter_get_key(&iter2));
   g_assert(1986 == mongo_bson_iter_get_value_int(&iter2));
   g_assert(!mongo_bson_iter_next(&iter2));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test13.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_ARRAY, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("array[bool]", ==, mongo_bson_iter_get_key(&iter));
   g_assert(mongo_bson_iter_recurse(&iter, &iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("0", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpint(TRUE, ==, mongo_bson_iter_get_value_boolean(&iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("1", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpint(FALSE, ==, mongo_bson_iter_get_value_boolean(&iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("2", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpint(TRUE, ==, mongo_bson_iter_get_value_boolean(&iter2));
   g_assert(!mongo_bson_iter_next(&iter2));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test14.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_ARRAY, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("array[string]", ==, mongo_bson_iter_get_key(&iter));
   g_assert(mongo_bson_iter_recurse(&iter, &iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("0", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpstr("hello", ==, mongo_bson_iter_get_value_string(&iter2, NULL));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("1", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpstr("world", ==, mongo_bson_iter_get_value_string(&iter2, NULL));
   g_assert(!mongo_bson_iter_next(&iter2));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test15.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_ARRAY, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("array[datetime]", ==, mongo_bson_iter_get_key(&iter));
   g_assert(mongo_bson_iter_recurse(&iter, &iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("0", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpint(MONGO_BSON_DATE_TIME, ==, mongo_bson_iter_get_value_type(&iter2));
   mongo_bson_iter_get_value_timeval(&iter2, &tv);
   g_assert_cmpint(tv.tv_sec, ==, 0);
   g_assert_cmpint(tv.tv_usec, ==, 0);
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpstr("1", ==, mongo_bson_iter_get_key(&iter2));
   g_assert_cmpint(MONGO_BSON_DATE_TIME, ==, mongo_bson_iter_get_value_type(&iter2));
   mongo_bson_iter_get_value_timeval(&iter2, &tv);
   g_assert_cmpint(tv.tv_sec, ==, 1319285594);
   g_assert_cmpint(tv.tv_usec, ==, 123);
   g_assert(!mongo_bson_iter_next(&iter2));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);

   bson = get_bson("test16.bson");
   mongo_bson_iter_init(&iter, bson);
   g_assert(mongo_bson_iter_next(&iter));
   g_assert_cmpint(MONGO_BSON_ARRAY, ==, mongo_bson_iter_get_value_type(&iter));
   g_assert_cmpstr("array[null]", ==, mongo_bson_iter_get_key(&iter));
   g_assert(mongo_bson_iter_recurse(&iter, &iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpint(MONGO_BSON_NULL, ==, mongo_bson_iter_get_value_type(&iter2));
   g_assert_cmpstr("0", ==, mongo_bson_iter_get_key(&iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpint(MONGO_BSON_NULL, ==, mongo_bson_iter_get_value_type(&iter2));
   g_assert_cmpstr("1", ==, mongo_bson_iter_get_key(&iter2));
   g_assert(mongo_bson_iter_next(&iter2));
   g_assert_cmpint(MONGO_BSON_NULL, ==, mongo_bson_iter_get_value_type(&iter2));
   g_assert_cmpstr("2", ==, mongo_bson_iter_get_key(&iter2));
   g_assert(!mongo_bson_iter_next(&iter2));
   g_assert(!mongo_bson_iter_next(&iter));
   mongo_bson_unref(bson);
}

gint
main (gint   argc,
      gchar *argv[])
{
   g_test_init(&argc, &argv, NULL);
   g_test_add_func("/MongoBson/append_tests", append_tests);
   g_test_add_func("/MongoBson/iter_tests", iter_tests);
   return g_test_run();
}
