--- common/gdm-common.c.orig
+++ common/gdm-common.c
@@ -36,7 +36,12 @@
 
 #include "gdm-common.h"
 
+#ifdef HAVE_BSD3_LOGIN
+#include <bsd3/sd-login.h>
+#include <bsd3/greeter-session.h>
+#else
 #include <systemd/sd-login.h>
+#endif
 
 #define GDM_DBUS_NAME                            "org.gnome.DisplayManager"
 #define GDM_DBUS_LOCAL_DISPLAY_FACTORY_PATH      "/org/gnome/DisplayManager/LocalDisplayFactory"
@@ -379,6 +384,14 @@
         g_return_val_if_fail (seat_id != NULL, FALSE);
         g_return_val_if_fail (session_id != NULL, FALSE);
 
+#ifdef HAVE_BSD3_LOGIN
+        if (!bsd3_activate_display_session (connection, seat_id, session_id, cancellable, &local_error)) {
+                g_warning ("Unable to activate session: %s", local_error->message);
+                g_error_free (local_error);
+                return FALSE;
+        }
+        return TRUE;
+#else
         reply = g_dbus_connection_call_sync (connection,
                                              "org.freedesktop.login1",
                                              "/org/freedesktop/login1",
@@ -398,6 +411,7 @@
         g_variant_unref (reply);
 
         return TRUE;
+#endif
 }
 
 gboolean
@@ -442,7 +456,11 @@
 
         g_return_val_if_fail (session_id != NULL, FALSE);
 
+#ifdef HAVE_BSD3_LOGIN
+        res = sd_get_sessions (&sessions);
+#else
         res = sd_seat_get_sessions (seat_id, &sessions, NULL, NULL);
+#endif
         if (res < 0) {
                 g_debug ("Failed to determine sessions: %s", strerror (-res));
                 return FALSE;
@@ -475,6 +493,18 @@
                 }
 
                 free (service_class);
+#ifdef HAVE_BSD3_LOGIN
+                g_autofree char *candidate_seat = NULL;
+                res = sd_session_get_seat (sessions[i], &candidate_seat);
+                if (res == -ENXIO || res == -ENODATA)
+                        continue;
+                if (res < 0) {
+                        ret = FALSE;
+                        goto out;
+                }
+                if (g_strcmp0 (candidate_seat, seat_id) != 0)
+                        continue;
+#endif
 
                 res = sd_session_get_state (sessions[i], &state);
                 if (res < 0) {
