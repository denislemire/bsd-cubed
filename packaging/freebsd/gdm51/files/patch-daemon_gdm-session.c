--- daemon/gdm-session.c.orig
+++ daemon/gdm-session.c
@@ -45,7 +45,7 @@
 
 #include <json-glib/json-glib.h>
 
-#include <systemd/sd-login.h>
+#include <bsd3/sd-login.h>
 
 #include "gdm-session.h"
 #include "gdm-session-glue.h"
@@ -3298,28 +3298,7 @@
 gboolean
 gdm_session_is_frozen (GdmSession *self)
 {
-        g_autofree char *cgroup = NULL, *path = NULL, *data = NULL;
-        g_auto (GStrv) arr = NULL;
-
-        g_return_val_if_fail (GDM_IS_SESSION (self), FALSE);
-
-        if (self->session_pid <= 0)
-                return FALSE;
-
-        if (sd_pid_get_cgroup (self->session_pid, &cgroup) < 0)
-                return FALSE;
-
-        path = g_build_filename ("/sys/fs/cgroup", cgroup, "cgroup.events", NULL);
-
-        if (!g_file_get_contents (path, &data, NULL, NULL))
-                return FALSE;
-
-        arr = g_strsplit_set (data, " \n", -1);
-
-        for (gsize i = 0; arr[i] != NULL; i++) {
-                if (g_str_equal (arr[i], "frozen"))
-                        return g_strcmp0 (arr[i + 1], "1") == 0;
-        }
+        /* FreeBSD's native supervisor does not freeze session cgroups. */
         return FALSE;
 }
 
