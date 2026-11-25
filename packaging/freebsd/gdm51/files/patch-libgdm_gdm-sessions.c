--- libgdm/gdm-sessions.c.orig
+++ libgdm/gdm-sessions.c
@@ -35,7 +35,11 @@
 #include <glib/gi18n.h>
 #include <glib/gstdio.h>
 
+#ifdef HAVE_BSD3_LOGIN
+#include <bsd3/sd-login.h>
+#else
 #include <systemd/sd-login.h>
+#endif
 
 #include "gdm-sessions.h"
 
@@ -64,8 +68,11 @@
 get_systemd_session (void)
 {
         int ret;
+        g_autofree char *session_id = NULL;
+#ifdef HAVE_BSD3_LOGIN
+        ret = sd_pid_get_session (getpid (), &session_id);
+#else
         g_autofree char *systemd_unit = NULL;
-        g_autofree char *session_id = NULL;
         pid_t pid;
         uid_t uid;
 
@@ -78,6 +85,7 @@
                 ret = sd_uid_get_display (uid, &session_id);
         else
                 ret = sd_pid_get_session (pid, &session_id);
+#endif
 
         if (ret < 0)
                 return NULL;
