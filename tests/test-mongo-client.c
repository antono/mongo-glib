#include <mongo-glib/mongo-glib.h>

static GMainLoop *gMainLoop;

static void
connect_cb (GObject      *object,
            GAsyncResult *result,
            gpointer      user_data)
{
   MongoClient *client = (MongoClient *)object;
   gboolean *success = user_data;
   GError *error = NULL;

   *success = mongo_client_connect_finish(client, result, &error);
   g_assert_no_error(error);
   g_assert(*success);
   g_main_loop_quit(gMainLoop);
}

static void
test_mongo_client_connect_async (void)
{
   MongoClient *client;
   gboolean success = FALSE;

   client = g_object_new(MONGO_TYPE_CLIENT,
                         "host", "localhost",
                         NULL);
   mongo_client_connect_async(client, NULL, connect_cb, &success);
   g_main_loop_run(gMainLoop);
   g_assert(success);
}

gint
main (gint   argc,
      gchar *argv[])
{
   g_test_init(&argc, &argv, NULL);

   g_type_init();
   gMainLoop = g_main_loop_new(NULL, FALSE);

   g_test_add_func("/MongoClient/connect_async", test_mongo_client_connect_async);

   return g_test_run();
}
