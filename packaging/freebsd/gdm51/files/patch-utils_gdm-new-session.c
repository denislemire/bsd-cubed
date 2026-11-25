--- utils/gdm-new-session.c.orig
+++ utils/gdm-new-session.c
@@ -8,7 +8,8 @@
 #include <gio/gio.h>
 #include <glib-unix.h>
 #include <sysexits.h>
-#include <systemd/sd-login.h>
+#include <bsd3/sd-login.h>
+#include <pwd.h>
 
 #define GDM_DBUS_NAME                        "org.gnome.DisplayManager"
 #define GDM_LOCAL_DISPLAY_FACTORY_PATH       "/org/gnome/DisplayManager/LocalDisplayFactory"
@@ -79,9 +80,12 @@
 
         g_variant_get (reply, "(s)", &session_id);
 
-        if (sd_session_get_username (session_id, &username) < 0)
+        uid_t uid;
+        struct passwd *pw;
+        if (sd_session_get_uid (session_id, &uid) < 0 || !(pw = getpwuid (uid)))
                 return;
 
+        username = g_strdup (pw->pw_name);
         if (g_strcmp0 (state->username, username) == 0) {
                 g_print ("Session started\n");
 
