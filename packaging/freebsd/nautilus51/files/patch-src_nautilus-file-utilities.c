# Consumer source and changes retain GPL-3.0-or-later licensing.
--- src/nautilus-file-utilities.c.orig
+++ src/nautilus-file-utilities.c
@@ -41,7 +41,12 @@
 #include <gio/gio.h>
 #include <unistd.h>
 #include <stdlib.h>
+#ifdef __FreeBSD__
+#include <sys/param.h>
+#include <sys/mount.h>
+#else
 #include <sys/vfs.h>
+#endif
 
 #define NAUTILUS_USER_DIRECTORY_NAME "nautilus"
 #define DEFAULT_NAUTILUS_DIRECTORY_MODE (0755)
@@ -1123,5 +1128,10 @@
 
     close (fd);
 
+#ifdef __FreeBSD__
+    /* FreeBSD filesystem type numbers do not use Linux superblock magic. */
+    return g_str_equal (buf.f_fstypename, "autofs");
+#else
     return (buf.f_type == AUTOFS_SUPER_MAGIC);
-}
+#endif
+}
