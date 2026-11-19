--- plugins/power/gsd-power-manager.c.orig
+++ plugins/power/gsd-power-manager.c
@@ -233,6 +233,7 @@
 static void      idle_triggered_idle_cb (GnomeIdleMonitor *monitor, guint watch_id, gpointer user_data);
 static void      idle_became_active_cb (GnomeIdleMonitor *monitor, guint watch_id, gpointer user_data);
 static void      iio_proxy_changed (GsdPowerManager *manager);
+static void      idle_set_mode (GsdPowerManager *manager, GsdPowerIdleMode mode);
 static void      iio_proxy_changed_cb (GDBusProxy *proxy, GVariant *changed_properties, GStrv invalidated_properties, gpointer user_data);
 
 static void      initable_iface_init (GInitableIface *initable_iface);
@@ -1490,9 +1491,17 @@
         gsd_display_config_set_power_save_mode (display_config, mode);
 }
 
+static gboolean
+bsd3_display_policy_enabled (void)
+{
+        return g_strcmp0 (g_getenv ("BSD3_IDLE_DISPLAY"), "1") == 0;
+}
+
 static void
 enable_monitors (GsdPowerManager *manager)
 {
+        if (bsd3_display_policy_enabled () && manager->lid_is_closed)
+                return;
         set_power_saving_mode (manager, GSD_POWER_SAVE_MODE_ON);
         manager->screen_blanked = FALSE;
         iio_proxy_maybe_claim_light (manager);
@@ -1592,6 +1601,9 @@
 {
         ca_context *ca_context;
 
+        if (bsd3_display_policy_enabled () && manager->session_is_active)
+                idle_set_mode (manager, GSD_POWER_IDLE_MODE_NORMAL);
+
         ca_context = gsd_application_get_ca_context (GSD_APPLICATION (manager));
 
         /* play a sound, using sounds from the naming spec */
@@ -1633,6 +1645,9 @@
 {
         ca_context *ca_context;
 
+        if (bsd3_display_policy_enabled () && manager->session_is_active)
+                idle_set_mode (manager, GSD_POWER_IDLE_MODE_BLANK);
+
         ca_context = gsd_application_get_ca_context (GSD_APPLICATION (manager));
 
         /* play a sound, using sounds from the naming spec */
@@ -2053,6 +2068,21 @@
                 return;
         }
 
+        /* BSD³ under SDDM can run without Shell's GDM-backed screen shield.
+         * Use the desktop blank-screen delay in that session even when the
+         * shield never activates. Keep the independent system sleep policy.
+         */
+        if (!manager->screensaver_active &&
+            bsd3_display_policy_enabled ()) {
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
@@ -2707,6 +2737,7 @@
                 g_variant_unref (v);
 
                 sync_lid_inhibitor (manager);
+                idle_configure (manager);
         }
 
         v = g_variant_lookup_value (changed, "InhibitedActions", G_VARIANT_TYPE_UINT32);
@@ -2866,6 +2897,12 @@
          * and replugging an external monitor, not suspend right away.
          */
         inhibit_lid_switch (manager);
+        if (bsd3_display_policy_enabled ()) {
+                /* BSD³ maps lid state to display power, never system sleep. */
+                stop_inhibit_lid_switch_timer (manager);
+                return;
+        }
+
         restart_inhibit_lid_switch_timer (manager);
 }
 
