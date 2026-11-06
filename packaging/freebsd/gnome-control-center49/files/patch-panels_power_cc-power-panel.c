--- panels/power/cc-power-panel.c.orig
+++ panels/power/cc-power-panel.c
@@ -608,9 +608,9 @@
     }
 
   variant = g_dbus_connection_call_sync (connection,
-                                         "org.freedesktop.login1",
-                                         "/org/freedesktop/login1",
-                                         "org.freedesktop.login1.Manager",
+                                         "org.freedesktop.ConsoleKit",
+                                         "/org/freedesktop/ConsoleKit/Manager",
+                                         "org.freedesktop.ConsoleKit.Manager",
                                          method_name,
                                          NULL,
                                          NULL,
@@ -1377,9 +1377,9 @@
   setup_power_profiles (self);
 
   setup_power_saving (self);
-  g_settings_bind (self->gsd_settings, "power-saver-profile-on-low-battery",
+  /*g_settings_bind (self->gsd_settings, "power-saver-profile-on-low-battery",
                    self->power_saver_low_battery_row, "active",
-                   G_SETTINGS_BIND_DEFAULT);
+                   G_SETTINGS_BIND_DEFAULT);*/
 
   setup_general_section (self);
 
