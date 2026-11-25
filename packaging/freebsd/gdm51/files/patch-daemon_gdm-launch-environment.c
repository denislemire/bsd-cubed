--- daemon/gdm-launch-environment.c.orig
+++ daemon/gdm-launch-environment.c
@@ -112,125 +112,6 @@
 static void     gdm_launch_environment_finalize      (GObject                   *object);
 
 G_DEFINE_TYPE (GdmLaunchEnvironment, gdm_launch_environment, G_TYPE_OBJECT)
-
-#define GID_NOBODY ((gid_t) 65534)
-#define MIGRATED_STAMPFILE_PATH (GDM_WORKING_DIR "/.migrated-dyn-users")
-
-static gboolean
-migrate_working_dir (GError **error)
-{
-        /* Makes sure GDM_WORKING_DIR has the right permissions and ownership.
-         * If it existed before the migration, g_mkdir_with_parents won't set
-         * the right mode. On some systems it'll be set to 0700 and gdm:gdm,
-         * which prevents the greeter session from accessing the subdirs within */
-        if (g_file_test (GDM_WORKING_DIR, G_FILE_TEST_IS_DIR)) {
-                if (g_chmod (GDM_WORKING_DIR, 0755) < 0) {
-                        int errsv = errno;
-                        g_set_error (error,
-                                     G_IO_ERROR,
-                                     g_io_error_from_errno (errsv),
-                                     "Failed to chmod directory '%s': %s",
-                                     GDM_WORKING_DIR,
-                                     g_strerror (errsv));
-                        return FALSE;
-                }
-
-                if (chown (GDM_WORKING_DIR, 0, 0) < 0) {
-                        int errsv = errno;
-                        g_set_error (error,
-                                     G_IO_ERROR,
-                                     g_io_error_from_errno (errsv),
-                                     "Failed to chown directory '%s': %s",
-                                     GDM_WORKING_DIR,
-                                     g_strerror (errsv));
-                        return FALSE;
-                }
-        }
-
-        if (g_file_test (MIGRATED_STAMPFILE_PATH, G_FILE_TEST_EXISTS))
-                return TRUE;
-
-        if (g_mkdir_with_parents (GDM_WORKING_DIR "/seat0", 0755) < 0) {
-                int errsv = errno;
-                g_set_error (error,
-                             G_IO_ERROR,
-                             g_io_error_from_errno (errsv),
-                             "Failed to create directory '%s': %s",
-                             GDM_WORKING_DIR "/seat0",
-                             g_strerror (errsv));
-                return FALSE;
-        }
-
-        if (!gdm_copy_dir_recursively (GDM_WORKING_DIR "/.local/state",
-                                       GDM_WORKING_DIR "/seat0/state",
-                                       error)) {
-               if (!g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
-                       return FALSE;
-
-                /* The directory not existing isn't an error */
-                g_clear_error (error);
-       }
-
-        if (!gdm_copy_dir_recursively (GDM_WORKING_DIR "/.config",
-                                       GDM_WORKING_DIR "/seat0/config",
-                                       error)) {
-                if (!g_error_matches (*error, G_IO_ERROR, G_IO_ERROR_NOT_FOUND))
-                       return FALSE;
-
-                g_clear_error (error);
-        }
-
-        if (!g_file_set_contents (MIGRATED_STAMPFILE_PATH, "1", -1, error))
-                return FALSE;
-
-        return TRUE;
-}
-
-static gboolean
-setup_seat_persist_dirs (const char  *seat_id,
-                         uid_t        owner,
-                         char       **out_config_dir,
-                         char       **out_state_dir)
-{
-        g_autoptr (GError) error = NULL;
-        g_autofree char *config_dir = NULL;
-        g_autofree char *state_dir = NULL;
-
-        /* HACK: This is a hack to address a regression introduced by the
-         *       transition to dynamic users in GDM 49. GDM would no longer
-         *       store settings changes, and thus lose the screen-reader enabled
-         *       state and similar settings. The long-term fix would be to
-         *       transition to userdb, store user settings in userdb, and then
-         *       load these settings from GDM. Until then, this hack will do.
-         *       Note that this hack will break once we implement homed support!
-         *       This hack operates on the assumption that there's only ever one
-         *       login screen per seat, which is true for now. This isn't
-         *       necessarily true in the future, however, because the greeter
-         *       and homed lock screen may coexist */
-
-        if (!migrate_working_dir (&error)) {
-                g_warning ("Failed to migrate " GDM_WORKING_DIR ": %s", error->message);
-                g_clear_error (&error);
-        }
-
-        config_dir = g_strdup_printf (GDM_WORKING_DIR "/%s/config", seat_id);
-        if (!gdm_ensure_dir (config_dir, owner, GID_NOBODY, 0700, TRUE, &error)) {
-                g_warning ("Failed to initialize persist config dir for seat %s: %s",
-                           seat_id, error->message);
-                return FALSE;
-        }
-
-        state_dir = g_strdup_printf (GDM_WORKING_DIR "/%s/state", seat_id);
-        if (!gdm_ensure_dir (state_dir, owner, GID_NOBODY, 0700, TRUE, &error)) {
-                g_warning ("Failed to initialize persist state dir for seat %s: %s",
-                           seat_id, error->message);
-                return FALSE;
-        }
-
-        *out_config_dir = g_steal_pointer (&config_dir);
-        *out_state_dir = g_steal_pointer (&state_dir);
-        return TRUE;
-}
 
 static char *
 get_var_cb (const char *var,
@@ -326,19 +207,8 @@
         g_hash_table_insert (hash, g_strdup ("SHELL"), g_strdup (NOLOGIN_PATH));
 
         if (start_session && launch_environment->display_seat_id != NULL) {
-                char *seat_id;
-                char *config_dir;
-                char *state_dir;
-
-                seat_id = launch_environment->display_seat_id;
-
-                g_hash_table_insert (hash, g_strdup ("GDM_SEAT_ID"), g_strdup (seat_id));
-
-                if (!is_initial_setup &&
-                    setup_seat_persist_dirs(seat_id, launch_environment->dyn_uid, &config_dir, &state_dir)) {
-                        g_hash_table_insert (hash, g_strdup ("XDG_CONFIG_HOME"), config_dir);
-                        g_hash_table_insert (hash, g_strdup ("XDG_STATE_HOME"), state_dir);
-                }
+                g_hash_table_insert (hash, g_strdup ("GDM_SEAT_ID"),
+                                     g_strdup (launch_environment->display_seat_id));
         }
 
         g_hash_table_insert (hash, g_strdup ("RUNNING_UNDER_GDM"), g_strdup ("true"));
@@ -367,83 +237,16 @@
         return hash;
 }
 
-static GDataOutputStream *
-get_environment_file_output_stream (GdmLaunchEnvironment  *launch_environment,
-                                    GError               **error)
-{
-        g_autofree char *config_dir = NULL;
-        g_autofree char *environment_dir = NULL;
-        g_autofree char *environment_conf = NULL;
-        g_autoptr(GFile) file = NULL;
-        g_autoptr(GFileOutputStream) file_stream = NULL;
-        uid_t uid = launch_environment->dyn_uid;
-        gid_t gid = GID_NOBODY;
-
-        config_dir = g_build_filename (launch_environment->dyn_user_home, ".config", NULL);
-        if (!gdm_ensure_dir (config_dir, uid, gid, 0700, FALSE, error))
-                return NULL;
-
-        environment_dir = g_build_filename (config_dir, "environment.d", NULL);
-        if (!gdm_ensure_dir (environment_dir, uid, gid, 0700, FALSE, error))
-                return NULL;
-
-        environment_conf = g_build_filename (environment_dir, "gdm.conf", NULL);
-        file = g_file_new_for_path (environment_conf);
-        file_stream = g_file_replace (file,
-                                      NULL,
-                                      FALSE,
-                                      G_FILE_CREATE_REPLACE_DESTINATION,
-                                      NULL,
-                                      error);
-        if (!file_stream)
-                return NULL;
-
-        if (!G_IS_FILE_DESCRIPTOR_BASED (file_stream)) {
-                g_set_error (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
-                             "Failed to chown stream for %s: file backing type is not fd",
-                             environment_conf);
-                return NULL;
-        }
-
-        int fd = g_file_descriptor_based_get_fd (G_FILE_DESCRIPTOR_BASED (file_stream));
-        if (fchown (fd, uid, gid) != 0) {
-                int errsv = errno;
-                g_set_error (error, G_IO_ERROR, g_io_error_from_errno (errsv),
-                            "Failed to chown stream for %s: %s", environment_conf, g_strerror (errsv));
-                return NULL;
-        }
-
-        return g_data_output_stream_new (G_OUTPUT_STREAM (file_stream));
-}
-
-static void
-on_session_setup_complete (GdmSession        *session,
-                           const char        *service_name,
+static void
+on_session_setup_complete (GdmSession *session, const char *service_name,
                            GdmLaunchEnvironment *launch_environment)
 {
-        g_autoptr(GHashTable) hash = NULL;
-        g_autoptr(GDataOutputStream) env_output_stream = NULL;
-        g_autoptr(GError) error = NULL;
-        GHashTableIter    iter;
-        gpointer          key, value;
-
-        hash = build_launch_environment (launch_environment, TRUE);
-        env_output_stream = get_environment_file_output_stream (launch_environment, &error);
-        if (!env_output_stream) {
-                g_critical ("Failed to set up environment file: %s", error->message);
-                return;
-        }
-
-        g_hash_table_iter_init (&iter, hash);
-        while (g_hash_table_iter_next (&iter, &key, &value)) {
-                g_autofree char *line = NULL;
-
-                line = g_strdup_printf ("%s=%s\n", (const char *)key, (const char *)value);
-                if (!g_data_output_stream_put_string (env_output_stream, line, NULL, &error)) {
-                        g_critical ("Failed to write environment variable to file: %s", error->message);
-                        g_clear_error (&error);
-                }
-        }
+        g_autoptr(GHashTable) environment = build_launch_environment (launch_environment, TRUE);
+        GHashTableIter iter;
+        gpointer key, value;
+        g_hash_table_iter_init (&iter, environment);
+        while (g_hash_table_iter_next (&iter, &key, &value))
+                gdm_session_set_environment_variable (session, key, value);
 }
 
 static void
