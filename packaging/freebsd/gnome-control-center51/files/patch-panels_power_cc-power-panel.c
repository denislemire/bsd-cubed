--- panels/power/cc-power-panel.c.orig
+++ panels/power/cc-power-panel.c
@@ -543,7 +543,7 @@
     const char *s;
 
     variant = g_dbus_connection_call_sync (
-        connection, "org.freedesktop.login1", "/org/freedesktop/login1", "org.freedesktop.login1.Manager", method_name,
+        connection, "org.freedesktop.ConsoleKit", "/org/freedesktop/ConsoleKit/Manager", "org.freedesktop.ConsoleKit.Manager", method_name,
         NULL, NULL, G_DBUS_CALL_FLAGS_NONE, -1, cc_panel_get_cancellable (CC_PANEL (self)), &error);
 
     if (!variant) {
