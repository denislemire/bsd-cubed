--- src/ck-session.c.orig
+++ src/ck-session.c
@@ -1879,7 +1879,7 @@ dbus_take_device (ConsoleKitSession *object,
 
         console_kit_session_complete_take_device (object, invocation,
                                                   out_fd_list, g_variant_new_handle (0),
-                                                  console_kit_session_get_active (object));
+                                                  !ck_device_get_active (device));
         return TRUE;
 }
 
