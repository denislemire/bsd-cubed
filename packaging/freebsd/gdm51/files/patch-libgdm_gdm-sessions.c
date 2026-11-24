--- libgdm/gdm-sessions.c.orig
+++ libgdm/gdm-sessions.c
@@ -35,7 +35,11 @@
 #include <glib/gi18n.h>
 #include <glib/gstdio.h>
 
+#ifdef HAVE_BSD_CUBED
+#include <bsd3/sd-login.h>
+#else
 #include <systemd/sd-login.h>
+#endif
 
 #include "gdm-sessions.h"
 
@@ -61,23 +65,24 @@
 }
 
 static char *
-get_systemd_session (void)
+get_login_session (void)
 {
         int ret;
-        g_autofree char *systemd_unit = NULL;
         g_autofree char *session_id = NULL;
-        pid_t pid;
-        uid_t uid;
 
-        pid = getpid ();
-        uid = getuid ();
+#ifdef HAVE_BSD_CUBED
+        ret = sd_pid_get_session (0, &session_id);
+#else
+        g_autofree char *systemd_unit = NULL;
+        pid_t pid = getpid ();
 
         ret = sd_pid_get_user_unit (pid, &systemd_unit);
 
         if (ret == 0)
-                ret = sd_uid_get_display (uid, &session_id);
+                ret = sd_uid_get_display (getuid (), &session_id);
         else
                 ret = sd_pid_get_session (pid, &session_id);
+#endif
 
         if (ret < 0)
                 return NULL;
@@ -86,13 +91,13 @@
 }
 
 static char *
-get_systemd_seat (void)
+get_login_seat (void)
 {
         g_autofree char *session_id = NULL;
         g_autofree char *seat = NULL;
         int ret;
 
-        session_id = get_systemd_session ();
+        session_id = get_login_session ();
 
         if (session_id == NULL)
                 return NULL;
@@ -138,7 +143,7 @@
                 g_error_free (error);
         }
 
-        seat = get_systemd_seat ();
+        seat = get_login_seat ();
 
         only_headless_allowed = seat == NULL;
 
