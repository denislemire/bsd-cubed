--- plugins/power/gsd-power-manager.c
+++ plugins/power/gsd-power-manager.c
@@ -2053,6 +2053,21 @@
                 return;
         }
 
+        /* BSD³ under SDDM can run without Shell's GDM-backed screen shield.
+         * Use the desktop blank-screen delay in that session even when the
+         * shield never activates. Keep the independent system sleep policy.
+         */
+        if (!manager->screensaver_active &&
+            g_strcmp0 (g_getenv ("BSD3_IDLE_DISPLAY"), "1") == 0) {
+                guint delay = g_settings_get_uint (manager->settings_bus, "idle-delay");
+                if (delay > 0)
+                        manager->idle_blank_id =
+                                gnome_idle_monitor_add_idle_watch (manager->idle_monitor,
+                                                                  (guint64) delay * 1000,
+                                                                  idle_triggered_idle_cb,
+                                                                  manager, NULL);
+        }
+
         /* only do the sleep timeout when the session is idle
          * and we aren't inhibited from sleeping (or logging out, etc.) */
         on_battery = up_client_get_on_battery (manager->up_client);
@@ -2707,6 +2722,7 @@
                 g_variant_unref (v);
 
                 sync_lid_inhibitor (manager);
+                idle_configure (manager);
         }
 
         v = g_variant_lookup_value (changed, "InhibitedActions", G_VARIANT_TYPE_UINT32);
