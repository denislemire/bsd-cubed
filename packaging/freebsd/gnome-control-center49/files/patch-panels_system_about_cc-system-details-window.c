--- panels/system/about/cc-system-details-window.c.orig
+++ panels/system/about/cc-system-details-window.c
@@ -29,6 +29,10 @@
 #include <gio/gunixmounts.h>
 #include <gio/gdesktopappinfo.h>
 
+#ifdef __FreeBSD__
+#include <unistd.h>
+#endif
+
 #include <glibtop/fsusage.h>
 #include <glibtop/mountlist.h>
 #include <glibtop/mem.h>
@@ -328,10 +332,13 @@
 get_os_name (void)
 {
   g_autofree gchar *name = NULL;
+#ifdef __linux__
   g_autofree gchar *version_id = NULL;
   g_autofree gchar *pretty_name = NULL;
+#endif
 
   name = g_get_os_info (G_OS_INFO_KEY_NAME);
+#ifdef __linux__
   version_id = g_get_os_info (G_OS_INFO_KEY_VERSION_ID);
   pretty_name = g_get_os_info (G_OS_INFO_KEY_PRETTY_NAME);
 
@@ -339,6 +346,10 @@
     return g_steal_pointer (&pretty_name);
   else if (name && version_id)
     return g_strdup_printf ("%s %s", name, version_id);
+#else
+  if (name)
+    return g_steal_pointer (&name);
+#endif
   else
     return g_strdup (_("Unknown"));
 }
@@ -444,6 +455,7 @@
   g_autofree char *kernel_name = NULL;
   g_autofree char *kernel_release = NULL;
 
+#ifdef __linux__
   kernel_name = cc_hostname_get_property (cc_hostname_get_default (), "KernelName");
   if (!kernel_name || g_strcmp0 (kernel_name, "") == 0)
     return NULL;
@@ -453,6 +465,14 @@
     return NULL;
 
   return g_strdup_printf ("%s %s", kernel_name, kernel_release);
+#endif
+
+#ifdef __FreeBSD__
+  kernel_release = g_get_os_info (G_OS_INFO_KEY_VERSION);
+  if (kernel_release)
+    return g_strdup_printf ("%s", kernel_release);
+#endif
+  return NULL;
 }
 
 char *
