--- daemon/gdm-manager.c.orig
+++ daemon/gdm-manager.c
@@ -36,7 +36,8 @@
 
 #include <act/act-user-manager.h>
 
-#include <systemd/sd-login.h>
+#include <bsd3/sd-login.h>
+#include <bsd3/greeter-session.h>
 
 #include "gdm-common.h"
 #include "gdm-file-utils.h"
@@ -316,32 +317,11 @@
 session_unlock (GdmManager *manager,
                 const char *ssid)
 {
-        GError *error = NULL;
-        GVariant *reply;
-
-        g_debug ("Unlocking session %s", ssid);
-
-        reply = g_dbus_connection_call_sync (manager->connection,
-                                             "org.freedesktop.login1",
-                                             "/org/freedesktop/login1",
-                                             "org.freedesktop.login1.Manager",
-                                             "UnlockSession",
-                                             g_variant_new ("(s)", ssid),
-                                             NULL, /* expected reply */
-                                             G_DBUS_CALL_FLAGS_NONE,
-                                             -1,
-                                             NULL,
-                                             &error);
-        if (reply == NULL) {
-                g_debug ("GdmManager: logind 'UnlockSession' %s raised:\n %s\n\n",
-                         g_dbus_error_get_remote_error (error), error->message);
-                g_error_free (error);
-                return FALSE;
-        }
-
-        g_variant_unref (reply);
-
-        return TRUE;
+        g_autoptr(GError) error = NULL;
+        if (bsd3_unlock_display_session (manager->connection, ssid, &error))
+                return TRUE;
+        g_warning ("Unable to unlock authenticated session: %s", error->message);
+        return FALSE;
 }
 
 static GdmSession *
@@ -1910,7 +1890,15 @@
         int res;
         int i;
 
-        res = sd_session_get_username (opened_session_id, &username);
+        uid_t uid;
+        struct passwd *pw;
+        res = sd_session_get_uid (opened_session_id, &uid);
+        if (res >= 0) {
+                if (gdm_get_pwent_for_uid (uid, &pw))
+                        username = g_strdup (pw->pw_name);
+                else
+                        res = -ENOENT;
+        }
         if (res < 0) {
                 g_warning ("Failed to get username of opened session: %s", strerror (-res));
                 return;
@@ -2455,6 +2443,10 @@
 {
         g_return_if_fail (GDM_IS_MANAGER (manager));
 
+        if (enabled) {
+                g_warning ("Remote login is unavailable with the local BSD provider");
+                return;
+        }
         if (manager->remote_login_enabled != enabled) {
                 manager->remote_login_enabled = enabled;
                 if (manager->remote_login_enabled) {
