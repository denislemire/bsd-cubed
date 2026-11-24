--- panels/system/about/cc-about-page.c.orig
+++ panels/system/about/cc-about-page.c
@@ -513,6 +513,9 @@
 static char *
 get_kernel_version_string ()
 {
+#ifdef __FreeBSD__
+    return g_get_os_info (G_OS_INFO_KEY_VERSION);
+#else
     g_autofree char *kernel_name = NULL;
     g_autofree char *kernel_release = NULL;
 
@@ -525,6 +528,7 @@
         return NULL;
 
     return g_strdup_printf ("%s %s", kernel_name, kernel_release);
+#endif
 }
 
 char *
