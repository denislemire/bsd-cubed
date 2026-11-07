# Consumer source and changes retain GPL-3.0-or-later licensing.
--- test/automated/displayless/test-thumbnails.c.orig
+++ test/automated/displayless/test-thumbnails.c
@@ -251,7 +251,13 @@
     gtk_test_init (&argc, &argv, G_TEST_OPTION_ISOLATE_DIRS, NULL);
     g_test_set_nonfatal_assertions ();
 
-    if (nautilus_application_is_sandboxed () || !can_run_bwrap ())
+    /* GNOME Desktop requires bubblewrap only on Linux. FreeBSD uses its
+     * native non-bwrap thumbnailer; exercise that actual implementation. */
+    if (nautilus_application_is_sandboxed ()
+#ifdef __linux__
+        || !can_run_bwrap ()
+#endif
+       )
     {
         /* Can't thumbnail in flatpak-builder sandbox. */
         return 77;
