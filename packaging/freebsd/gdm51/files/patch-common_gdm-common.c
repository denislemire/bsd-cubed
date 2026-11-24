--- common/gdm-common.c.orig
+++ common/gdm-common.c
@@ -36,7 +36,11 @@
 
 #include "gdm-common.h"
 
+#ifdef HAVE_BSD_CUBED
+#include <bsd3/sd-login.h>
+#else
 #include <systemd/sd-login.h>
+#endif
 
 #define GDM_DBUS_NAME                            "org.gnome.DisplayManager"
 #define GDM_DBUS_LOCAL_DISPLAY_FACTORY_PATH      "/org/gnome/DisplayManager/LocalDisplayFactory"
@@ -442,7 +446,11 @@
 
         g_return_val_if_fail (session_id != NULL, FALSE);
 
+#ifdef HAVE_BSD_CUBED
+        res = sd_get_sessions (&sessions);
+#else
         res = sd_seat_get_sessions (seat_id, &sessions, NULL, NULL);
+#endif
         if (res < 0) {
                 g_debug ("Failed to determine sessions: %s", strerror (-res));
                 return FALSE;
@@ -459,6 +467,21 @@
                 char *service_class = NULL;
                 char *state = NULL;
 
+#ifdef HAVE_BSD_CUBED
+                g_autofree char *candidate_seat = NULL;
+
+                res = sd_session_get_seat (sessions[i], &candidate_seat);
+                if (res == -ENODATA || res == -ENXIO)
+                        continue;
+                if (res < 0) {
+                        g_debug ("Cannot query seat for %s: %s", sessions[i], strerror (-res));
+                        ret = FALSE;
+                        goto out;
+                }
+                if (g_strcmp0 (candidate_seat, seat_id) != 0)
+                        continue;
+#endif
+
                 res = sd_session_get_class (sessions[i], &service_class);
                 if (res < 0) {
                         if (res == -ENXIO)
