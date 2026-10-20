#include <glib.h>
#include <string.h>

typedef struct _HNVC HasNonVoidCleanup;
HasNonVoidCleanup * non_void_cleanup (HasNonVoidCleanup *);

/* Should not cause any warnings with -Wextra */
G_DEFINE_AUTOPTR_CLEANUP_FUNC(HasNonVoidCleanup, non_void_cleanup)

static void
test_autofree (void)
{
#ifdef __clang_analyzer__
  g_test_skip ("autofree tests aren’t understood by the clang analyser");
#else
  g_autofree gchar *p = NULL;
  g_autofree gchar *p2 = NULL;
  g_autofree gchar *alwaysnull = NULL;

  p = g_malloc (10);
  p2 = g_malloc (42);

  p[0] = 1;
  p2[0] = 1;

  if (TRUE)
    {
      g_autofree guint8 *buf = g_malloc (128);
      g_autofree gchar *alwaysnull_again = NULL;

      buf[0] = 1;

      g_assert_null (alwaysnull_again);
    }

  if (TRUE)
    {
      g_autofree guint8 *buf2 = g_malloc (256);

      buf2[255] = 42;
    }

  g_assert_null (alwaysnull);
#endif  /* __clang_analyzer__ */
}

static void
test_g_async_queue (void)
{
  g_autoptr(GAsyncQueue) val = g_async_queue_new ();
  g_assert_nonnull (val);
}

static void
test_g_bookmark_file (void)
{
  g_autoptr(GBookmarkFile) val = g_bookmark_file_new ();
  g_assert_nonnull (val);
}

static void
test_g_bytes (void)
{
  g_autoptr(GBytes) val = g_bytes_new ("foo", 3);
  g_assert_nonnull (val);
}

static void
test_g_checksum (void)
{
  g_autoptr(GChecksum) val = g_checksum_new (G_CHECKSUM_SHA256);
  g_assert_nonnull (val);
}

static void
test_g_date (void)
{
  g_autoptr(GDate) val = g_date_new ();
  g_assert_nonnull (val);
}

static void
test_g_date_time (void)
{
  g_autoptr(GDateTime) val = g_date_time_new_now_utc ();
  g_assert_nonnull (val);
}

static void
test_g_dir (void)
{
  g_autoptr(GDir) val = g_dir_open (".", 0, NULL);
  g_assert_nonnull (val);
}

static void
test_g_error (void)
{
  g_autoptr(GError) val = g_error_new_literal (G_FILE_ERROR, G_FILE_ERROR_FAILED, "oops");
  g_assert_nonnull (val);
}

static void
test_g_hash_table (void)
{
  g_autoptr(GHashTable) val = g_hash_table_new (NULL, NULL);
  g_assert_nonnull (val);
}

static void
test_g_hmac (void)
{
  g_autoptr(GHmac) val = g_hmac_new (G_CHECKSUM_SHA256, (guint8*)"hello", 5);
  g_assert_nonnull (val);
}

static void
test_g_io_channel (void)
{
#ifdef G_OS_WIN32
  const gchar *devnull = "nul";
#else
  const gchar *devnull = "/dev/null";
#endif

  g_autoptr(GIOChannel) val = g_io_channel_new_file (devnull, "r", NULL);
  g_assert_nonnull (val);
}

static void
test_g_key_file (void)
{
  g_autoptr(GKeyFile) val = g_key_file_new ();
  g_assert_nonnull (val);
}

static void
test_g_list (void)
{
  g_autoptr(GList) val = NULL;
  g_autoptr(GList) val2 = g_list_prepend (NULL, "foo");
  g_assert_null (val);
  g_assert_nonnull (val2);
}

static void
test_g_array (void)
{
  g_autoptr(GArray) val = g_array_new (0, 0, sizeof (gpointer));
  g_assert_nonnull (val);
}

static void
test_g_ptr_array (void)
{
  g_autoptr(GPtrArray) val = g_ptr_array_new ();
  g_assert_nonnull (val);
}

static void
test_g_byte_array (void)
{
  g_autoptr(GByteArray) val = g_byte_array_new ();
  g_assert_nonnull (val);
}

static void
test_g_main_context (void)
{
  g_autoptr(GMainContext) val = g_main_context_new ();
  g_assert_nonnull (val);
}

static void
test_g_main_context_pusher (void)
{
  GMainContext *context, *old_thread_default;

  context = g_main_context_new ();
  old_thread_default = g_main_context_get_thread_default ();
  g_assert_false (old_thread_default == context);

  if (TRUE)
    {
      g_autoptr(GMainContextPusher) val = g_main_context_pusher_new (context);
      g_assert_nonnull (val);

      /* Check it’s now the thread-default main context */
      g_assert_true (g_main_context_get_thread_default () == context);
    }

  /* Check it’s now the old thread-default main context */
  g_assert_false (g_main_context_get_thread_default () == context);
  g_assert_true (g_main_context_get_thread_default () == old_thread_default);

  g_main_context_unref (context);
}

static void
test_g_main_loop (void)
{
  g_autoptr(GMainLoop) val = g_main_loop_new (NULL, TRUE);
  g_assert_nonnull (val);
}

static void
test_g_source (void)
{
  g_autoptr(GSource) val = g_timeout_source_new_seconds (2);
  g_assert_nonnull (val);
}

static void
test_g_mapped_file (void)
{
  g_autoptr(GMappedFile) val = g_mapped_file_new (g_test_get_filename (G_TEST_DIST, "keyfiletest.ini", NULL), FALSE, NULL);
  g_assert_nonnull (val);
}

static void
parser_start (GMarkupParseContext  *context,
              const gchar          *element_name,
              const gchar         **attribute_names,
              const gchar         **attribute_values,
              gpointer              user_data,
              GError              **error)
{
}

static void
parser_end (GMarkupParseContext  *context,
            const gchar          *element_name,
            gpointer              user_data,
            GError              **error)
{
}

static GMarkupParser parser = {
  .start_element = parser_start,
  .end_element = parser_end
};

static void
test_g_markup_parse_context (void)
{
  g_autoptr(GMarkupParseContext) val = g_markup_parse_context_new (&parser,
                                                                   G_MARKUP_DEFAULT_FLAGS,
                                                                   NULL, NULL);
  g_assert_nonnull (val);
}

static void
test_g_node (void)
{
  g_autoptr(GNode) val = g_node_new ("hello");
  g_assert_nonnull (val);
}

static void
test_g_option_context (void)
{
  g_autoptr(GOptionContext) val = g_option_context_new ("hello");
  g_assert_nonnull (val);
}

static void
test_g_option_group (void)
{
  g_autoptr(GOptionGroup) val = g_option_group_new ("hello", "world", "helpme", NULL, NULL);
  g_assert_nonnull (val);
}

static void
test_g_pattern_spec (void)
{
  g_autoptr(GPatternSpec) val = g_pattern_spec_new ("plaid");
  g_assert_nonnull (val);
}

static void
test_g_queue (void)
{
  g_autoptr(GQueue) val = g_queue_new ();
  g_auto(GQueue) stackval = G_QUEUE_INIT;
  g_assert_nonnull (val);
  g_assert_null (stackval.head);
}

static void
test_g_rand (void)
{
  g_autoptr(GRand) val = g_rand_new ();
  g_assert_nonnull (val);
}

static void
test_g_regex (void)
{
  g_autoptr(GRegex) val = g_regex_new (".*", G_REGEX_DEFAULT,
                                       G_REGEX_MATCH_DEFAULT, NULL);
  g_assert_nonnull (val);
}

static void
test_g_match_info (void)
{
  g_autoptr(GRegex) regex = g_regex_new (".*", G_REGEX_DEFAULT,
                                         G_REGEX_MATCH_DEFAULT, NULL);
  g_autoptr(GMatchInfo) match = NULL;

  if (!g_regex_match (regex, "hello", 0, &match))
    g_assert_not_reached ();
}

static void
test_g_scanner (void)
{
  GScannerConfig config = { 0, };
  g_autoptr(GScanner) val = g_scanner_new (&config);
  g_assert_nonnull (val);
}

static void
test_g_sequence (void)
{
  g_autoptr(GSequence) val = g_sequence_new (NULL);
  g_assert_nonnull (val);
}

static void
test_g_slist (void)
{
  g_autoptr(GSList) val = NULL;
  g_autoptr(GSList) nonempty_val = g_slist_prepend (NULL, "hello");
  g_assert_null (val);
  g_assert_nonnull (nonempty_val);
}

static void
test_g_string (void)
{
  g_autoptr(GString) val = g_string_new ("");
  g_assert_nonnull (val);
}

static void
test_g_string_chunk (void)
{
  g_autoptr(GStringChunk) val = g_string_chunk_new (42);
  g_assert_nonnull (val);
}

static gpointer
mythread (gpointer data)
{
  g_usleep (G_USEC_PER_SEC);
  return NULL;
}

static void
test_g_thread (void)
{
  g_autoptr(GThread) val = g_thread_new ("bob", mythread, NULL);
  g_assert_nonnull (val);
}

static void
test_g_mutex (void)
{
  g_auto(GMutex) val;
  
  g_mutex_init (&val);
}

/* Thread function to check that a mutex given in @data is locked */
static gpointer
mutex_locked_thread (gpointer data)
{
  GMutex *mutex = (GMutex *) data;
  g_assert_false (g_mutex_trylock (mutex));
  return NULL;
}

/* Thread function to check that a mutex given in @data is unlocked */
static gpointer
mutex_unlocked_thread (gpointer data)
{
  GMutex *mutex = (GMutex *) data;
  g_assert_true (g_mutex_trylock (mutex));
  g_mutex_unlock (mutex);
  return NULL;
}

static void
test_g_mutex_locker (void)
{
  GMutex mutex;
  GThread *thread;

  g_mutex_init (&mutex);

  if (TRUE)
    {
      /* val is unused in this scope but compiler should not warn. */
      G_MUTEX_AUTO_LOCK (&mutex, val);
    }

  if (TRUE)
    {
      g_autoptr(GMutexLocker) val = g_mutex_locker_new (&mutex);
      
      g_assert_nonnull (val);

      /* Verify that the mutex is actually locked */
      thread = g_thread_new ("mutex locked", mutex_locked_thread, &mutex);
      g_thread_join (thread);
    }

    /* Verify that the mutex is unlocked again */
    thread = g_thread_new ("mutex unlocked", mutex_unlocked_thread, &mutex);
    g_thread_join (thread);
}

/* Thread function to check that a recursive mutex given in @data is locked */
static gpointer
rec_mutex_locked_thread (gpointer data)
{
  GRecMutex *rec_mutex = (GRecMutex *) data;
  g_assert_false (g_rec_mutex_trylock (rec_mutex));
  return NULL;
}

/* Thread function to check that a recursive mutex given in @data is unlocked */
static gpointer
rec_mutex_unlocked_thread (gpointer data)
{
  GRecMutex *rec_mutex = (GRecMutex *) data;
  g_assert_true (g_rec_mutex_trylock (rec_mutex));
  g_rec_mutex_unlock (rec_mutex);
  return NULL;
}

static void
test_g_rec_mutex_locker (void)
{
  GRecMutex rec_mutex;
  GThread *thread;

  g_rec_mutex_init (&rec_mutex);

  if (TRUE)
    {
      /* val is unused in this scope but compiler should not warn. */
      G_REC_MUTEX_AUTO_LOCK (&rec_mutex, val);
    }

  if (TRUE)
    {
      g_autoptr(GRecMutexLocker) val = g_rec_mutex_locker_new (&rec_mutex);
      g_autoptr(GRecMutexLocker) other = NULL;

      g_assert_nonnull (val);

      /* Verify that the mutex is actually locked */
      thread = g_thread_new ("rec mutex locked", rec_mutex_locked_thread, &rec_mutex);
      g_thread_join (g_steal_pointer (&thread));

      other = g_rec_mutex_locker_new (&rec_mutex);
      thread = g_thread_new ("rec mutex locked", rec_mutex_locked_thread, &rec_mutex);
      g_thread_join (g_steal_pointer (&thread));
    }

  /* Verify that the mutex is unlocked again */
  thread = g_thread_new ("rec mutex unlocked", rec_mutex_unlocked_thread, &rec_mutex);
  g_thread_join (thread);

  g_rec_mutex_clear (&rec_mutex);
}

/* Thread function to check that an rw lock given in @data cannot be writer locked */
static gpointer
rw_lock_cannot_take_writer_lock_thread (gpointer data)
{
  GRWLock *lock = (GRWLock *) data;
  g_assert_false (g_rw_lock_writer_trylock (lock));
  return NULL;
}

/* Thread function to check that an rw lock given in @data can be reader locked */
static gpointer
rw_lock_can_take_reader_lock_thread (gpointer data)
{
  GRWLock *lock = (GRWLock *) data;
  g_assert_true (g_rw_lock_reader_trylock (lock));
  g_rw_lock_reader_unlock (lock);
  return NULL;
}

static void
test_g_rw_lock_lockers (void)
{
  GRWLock lock;
  GThread *thread;

  g_rw_lock_init (&lock);

  if (TRUE)
    {
      /* val is unused in this scope but compiler should not warn. */
      G_RW_LOCK_WRITER_AUTO_LOCK (&lock, val);
    }

  if (TRUE)
    {
      /* val is unused in this scope but compiler should not warn. */
      G_RW_LOCK_READER_AUTO_LOCK (&lock, val);
    }

  if (TRUE)
    {
      g_autoptr(GRWLockWriterLocker) val = g_rw_lock_writer_locker_new (&lock);

      g_assert_nonnull (val);

      /* Verify that we cannot take another writer lock as a writer lock is currently held */
      thread = g_thread_new ("rw lock cannot take writer lock", rw_lock_cannot_take_writer_lock_thread, &lock);
      g_thread_join (thread);

      /* Verify that we cannot take a reader lock as a writer lock is currently held */
      g_assert_false (g_rw_lock_reader_trylock (&lock));
    }

  if (TRUE)
    {
      g_autoptr(GRWLockReaderLocker) val = g_rw_lock_reader_locker_new (&lock);

      g_assert_nonnull (val);

      /* Verify that we can take another reader lock from another thread */
      thread = g_thread_new ("rw lock can take reader lock", rw_lock_can_take_reader_lock_thread, &lock);
      g_thread_join (thread);

      /* ... and also that recursive reader locking from the same thread works */
      g_assert_true (g_rw_lock_reader_trylock (&lock));
      g_rw_lock_reader_unlock (&lock);

      /* Verify that we cannot take a writer lock as a reader lock is currently held */
      thread = g_thread_new ("rw lock cannot take writer lock", rw_lock_cannot_take_writer_lock_thread, &lock);
      g_thread_join (thread);
    }

  /* Verify that we can take a writer lock again: this can only work if all of
   * the locks taken above have been correctly released. */
  g_assert_true (g_rw_lock_writer_trylock (&lock));
  g_rw_lock_writer_unlock (&lock);

  g_rw_lock_clear (&lock);
}

G_LOCK_DEFINE (test_g_auto_lock);

static void
test_g_auto_lock (void)
{
  GThread *thread;

  if (TRUE)
    {
      G_AUTO_LOCK (test_g_auto_lock);

      /* Verify that the mutex is actually locked */
      thread = g_thread_new ("mutex locked", mutex_locked_thread, &G_LOCK_NAME (test_g_auto_lock));
      g_thread_join (thread);
    }

  /* Verify that the mutex is unlocked again */
  thread = g_thread_new ("mutex unlocked", mutex_unlocked_thread, &G_LOCK_NAME (test_g_auto_lock));
  g_thread_join (thread);
}

static void
test_g_cond (void)
{
  g_auto(GCond) val;
  g_cond_init (&val);
}

static void
test_g_timer (void)
{
  g_autoptr(GTimer) val = g_timer_new ();
  g_assert_nonnull (val);
}

static void
test_g_time_zone (void)
{
  g_autoptr(GTimeZone) val = g_time_zone_new_utc ();
  g_assert_nonnull (val);
}

static void
test_g_tree (void)
{
  g_autoptr(GTree) val = g_tree_new ((GCompareFunc)strcmp);
  g_assert_nonnull (val);
}

static void
test_g_variant (void)
{
  g_autoptr(GVariant) val = g_variant_new_string ("hello");
  g_assert_nonnull (val);
}

static void
test_g_variant_builder (void)
{
  g_autoptr(GVariantBuilder) val = g_variant_builder_new (G_VARIANT_TYPE ("as"));
  g_auto(GVariantBuilder) stackval;

  g_assert_nonnull (val);
  g_variant_builder_init (&stackval, G_VARIANT_TYPE ("as"));
}

static void
test_g_variant_iter (void)
{
  g_autoptr(GVariant) var = g_variant_new_fixed_array (G_VARIANT_TYPE_UINT32, "", 0, sizeof(guint32));
  g_autoptr(GVariantIter) val = g_variant_iter_new (var);
  g_assert_nonnull (val);
}

static void
test_g_variant_dict (void)
{
  g_autoptr(GVariant) data = g_variant_new_from_data (G_VARIANT_TYPE ("a{sv}"), "", 0, FALSE, NULL, NULL);
  g_auto(GVariantDict) stackval;
  g_autoptr(GVariantDict) val = g_variant_dict_new (data);

  g_variant_dict_init (&stackval, data);
  g_assert_nonnull (val);
}

static void
test_g_variant_type (void)
{
  g_autoptr(GVariantType) val = g_variant_type_new ("s");
  g_assert_nonnull (val);
}

static void
test_strv (void)
{
  g_auto(GStrv) val = g_strsplit("a:b:c", ":", -1);
  g_assert_nonnull (val);
}

static void
test_refstring (void)
{
  g_autoptr(GRefString) str = g_ref_string_new ("hello, world");
  g_assert_nonnull (str);
}

static void
test_pathbuf (void)
{
#if defined(G_OS_UNIX)
  g_autoptr(GPathBuf) buf1 = g_path_buf_new_from_path ("/bin/sh");
  g_auto(GPathBuf) buf2 = G_PATH_BUF_INIT;

  g_path_buf_push (&buf2, "/bin/sh");
#elif defined(G_OS_WIN32)
  g_autoptr(GPathBuf) buf1 = g_path_buf_new_from_path ("C:\\windows\\system32.dll");
  g_auto(GPathBuf) buf2 = G_PATH_BUF_INIT;

  g_path_buf_push (&buf2, "C:\\windows\\system32.dll");
#else
  g_test_skip ("Unsupported platform");
  return;
#endif

  g_autofree char *path1 = g_path_buf_to_path (buf1);
  g_autofree char *path2 = g_path_buf_to_path (&buf2);

  g_assert_cmpstr (path1, ==, path2);
}

static void
mark_freed (gpointer ptr)
{
  gboolean *freed = ptr;
  *freed = TRUE;
}

static void
test_autolist (void)
{
  char data[1] = {0};
  gboolean freed1 = FALSE;
  gboolean freed2 = FALSE;
  gboolean freed3 = FALSE;
  GBytes *b1 = g_bytes_new_with_free_func (data, sizeof(data), mark_freed, &freed1);
  GBytes *b2 = g_bytes_new_with_free_func (data, sizeof(data), mark_freed, &freed2);
  GBytes *b3 = g_bytes_new_with_free_func (data, sizeof(data), mark_freed, &freed3);

  {
    g_autolist(GBytes) l = NULL;

    l = g_list_prepend (l, b1);
    l = g_list_prepend (l, b3);

    /* Squash warnings about dead stores */
    (void) l;
  }

  /* Only assert if autoptr works */
#ifdef __GNUC__
  g_assert_true (freed1);
  g_assert_true (freed3);
#endif
  g_assert_false (freed2);

  g_bytes_unref (b2);
  g_assert_true (freed2);
}

static void
test_autoslist (void)
{
  char data[1] = {0};
  gboolean freed1 = FALSE;
  gboolean freed2 = FALSE;
  gboolean freed3 = FALSE;
  GBytes *b1 = g_bytes_new_with_free_func (data, sizeof(data), mark_freed, &freed1);
  GBytes *b2 = g_bytes_new_with_free_func (data, sizeof(data), mark_freed, &freed2);
  GBytes *b3 = g_bytes_new_with_free_func (data, sizeof(data), mark_freed, &freed3);

  {
    g_autoslist(GBytes) l = NULL;

    l = g_slist_prepend (l, b1);
    l = g_slist_prepend (l, b3);
  }

  /* Only assert if autoptr works */
#ifdef __GNUC__
  g_assert_true (freed1);
  g_assert_true (freed3);
#endif
  g_assert_false (freed2);

  g_bytes_unref (b2);
  g_assert_true (freed2);
}

static void
test_autoqueue (void)
{
  char data[1] = {0};
  gboolean freed1 = FALSE;
  gboolean freed2 = FALSE;
  gboolean freed3 = FALSE;
  GBytes *b1 = g_bytes_new_with_free_func (data, sizeof(data), mark_freed, &freed1);
  GBytes *b2 = g_bytes_new_with_free_func (data, sizeof(data), mark_freed, &freed2);
  GBytes *b3 = g_bytes_new_with_free_func (data, sizeof(data), mark_freed, &freed3);

  {
    g_autoqueue(GBytes) q = g_queue_new ();

    g_queue_push_head (q, b1);
    g_queue_push_tail (q, b3);
  }

  /* Only assert if autoptr works */
#ifdef __GNUC__
  g_assert_true (freed1);
  g_assert_true (freed3);
#endif
  g_assert_false (freed2);

  g_bytes_unref (b2);
  g_assert_true (freed2);
}

int
main (int argc, gchar *argv[])
{
  g_test_init (&argc, &argv, NULL);

  g_test_add_func ("/autoptr/autofree", test_autofree);
  g_test_add_func ("/autoptr/g_async_queue", test_g_async_queue);
  g_test_add_func ("/autoptr/g_bookmark_file", test_g_bookmark_file);
  g_test_add_func ("/autoptr/g_bytes", test_g_bytes);
  g_test_add_func ("/autoptr/g_checksum", test_g_checksum);
  g_test_add_func ("/autoptr/g_date", test_g_date);
  g_test_add_func ("/autoptr/g_date_time", test_g_date_time);
  g_test_add_func ("/autoptr/g_dir", test_g_dir);
  g_test_add_func ("/autoptr/g_error", test_g_error);
  g_test_add_func ("/autoptr/g_hash_table", test_g_hash_table);
  g_test_add_func ("/autoptr/g_hmac", test_g_hmac);
  g_test_add_func ("/autoptr/g_io_channel", test_g_io_channel);
  g_test_add_func ("/autoptr/g_key_file", test_g_key_file);
  g_test_add_func ("/autoptr/g_list", test_g_list);
  g_test_add_func ("/autoptr/g_array", test_g_array);
  g_test_add_func ("/autoptr/g_ptr_array", test_g_ptr_array);
  g_test_add_func ("/autoptr/g_byte_array", test_g_byte_array);
  g_test_add_func ("/autoptr/g_main_context", test_g_main_context);
  g_test_add_func ("/autoptr/g_main_context_pusher", test_g_main_context_pusher);
  g_test_add_func ("/autoptr/g_main_loop", test_g_main_loop);
  g_test_add_func ("/autoptr/g_source", test_g_source);
  g_test_add_func ("/autoptr/g_mapped_file", test_g_mapped_file);
  g_test_add_func ("/autoptr/g_markup_parse_context", test_g_markup_parse_context);
  g_test_add_func ("/autoptr/g_node", test_g_node);
  g_test_add_func ("/autoptr/g_option_context", test_g_option_context);
  g_test_add_func ("/autoptr/g_option_group", test_g_option_group);
  g_test_add_func ("/autoptr/g_pattern_spec", test_g_pattern_spec);
  g_test_add_func ("/autoptr/g_queue", test_g_queue);
  g_test_add_func ("/autoptr/g_rand", test_g_rand);
  g_test_add_func ("/autoptr/g_regex", test_g_regex);
  g_test_add_func ("/autoptr/g_match_info", test_g_match_info);
  g_test_add_func ("/autoptr/g_scanner", test_g_scanner);
  g_test_add_func ("/autoptr/g_sequence", test_g_sequence);
  g_test_add_func ("/autoptr/g_slist", test_g_slist);
  g_test_add_func ("/autoptr/g_string", test_g_string);
  g_test_add_func ("/autoptr/g_string_chunk", test_g_string_chunk);
  g_test_add_func ("/autoptr/g_thread", test_g_thread);
  g_test_add_func ("/autoptr/g_mutex", test_g_mutex);
  g_test_add_func ("/autoptr/g_mutex_locker", test_g_mutex_locker);
  g_test_add_func ("/autoptr/g_rec_mutex_locker", test_g_rec_mutex_locker);
  g_test_add_func ("/autoptr/g_rw_lock_lockers", test_g_rw_lock_lockers);
  g_test_add_func ("/autoptr/g_auto_lock", test_g_auto_lock);
  g_test_add_func ("/autoptr/g_cond", test_g_cond);
  g_test_add_func ("/autoptr/g_timer", test_g_timer);
  g_test_add_func ("/autoptr/g_time_zone", test_g_time_zone);
  g_test_add_func ("/autoptr/g_tree", test_g_tree);
  g_test_add_func ("/autoptr/g_variant", test_g_variant);
  g_test_add_func ("/autoptr/g_variant_builder", test_g_variant_builder);
  g_test_add_func ("/autoptr/g_variant_iter", test_g_variant_iter);
  g_test_add_func ("/autoptr/g_variant_dict", test_g_variant_dict);
  g_test_add_func ("/autoptr/g_variant_type", test_g_variant_type);
  g_test_add_func ("/autoptr/strv", test_strv);
  g_test_add_func ("/autoptr/refstring", test_refstring);
  g_test_add_func ("/autoptr/pathbuf", test_pathbuf);
  g_test_add_func ("/autoptr/autolist", test_autolist);
  g_test_add_func ("/autoptr/autoslist", test_autoslist);
  g_test_add_func ("/autoptr/autoqueue", test_autoqueue);

  return g_test_run ();
}
