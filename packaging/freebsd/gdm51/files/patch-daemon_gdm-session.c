--- daemon/gdm-session.c.orig
+++ daemon/gdm-session.c
@@ -45,7 +45,14 @@
 
 #include <json-glib/json-glib.h>
 
+#ifdef HAVE_BSD_CUBED
+#include <sys/param.h>
+#include <sys/sysctl.h>
+#include <sys/user.h>
+#include <sys/proc.h>
+#else
 #include <systemd/sd-login.h>
+#endif
 
 #include "gdm-session.h"
 #include "gdm-session-glue.h"
@@ -3298,6 +3305,23 @@
 gboolean
 gdm_session_is_frozen (GdmSession *self)
 {
+#ifdef HAVE_BSD_CUBED
+        struct kinfo_proc process;
+        size_t length = sizeof process;
+        int mib[] = { CTL_KERN, KERN_PROC, KERN_PROC_PID, 0 };
+
+        g_return_val_if_fail (GDM_IS_SESSION (self), TRUE);
+        if (self->session_pid <= 0)
+                return FALSE;
+
+        /* A stopped session leader cannot service a reauthentication request.
+         * Use the temporary PAM channel if its state cannot be inspected. */
+        mib[3] = self->session_pid;
+        if (sysctl (mib, G_N_ELEMENTS (mib), &process, &length, NULL, 0) < 0 ||
+            length != sizeof process || process.ki_pid != self->session_pid)
+                return TRUE;
+        return process.ki_stat == SSTOP;
+#else
         g_autofree char *cgroup = NULL, *path = NULL, *data = NULL;
         g_auto (GStrv) arr = NULL;
 
@@ -3321,6 +3345,7 @@
                         return g_strcmp0 (arr[i + 1], "1") == 0;
         }
         return FALSE;
+#endif
 }
 
 gboolean
