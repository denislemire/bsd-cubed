--- shell/cc-panel-loader.c.orig
+++ shell/cc-panel-loader.c
@@ -52,14 +52,18 @@
 extern GType cc_printers_panel_get_type (void);
 extern GType cc_privacy_panel_get_type (void);
 extern GType cc_search_panel_get_type (void);
+#ifdef BUILD_SHARING
 extern GType cc_sharing_panel_get_type (void);
+#endif /* BUILD_SHARING */
 extern GType cc_sound_panel_get_type (void);
 extern GType cc_system_panel_get_type (void);
 extern GType cc_ua_panel_get_type (void);
 #ifdef BUILD_WACOM
 extern GType cc_wacom_panel_get_type (void);
 #endif /* BUILD_WACOM */
+#ifdef BUILD_WELLBEING
 extern GType cc_wellbeing_panel_get_type (void);
+#endif
 #ifdef BUILD_WWAN
 extern GType cc_wwan_panel_get_type (void);
 #endif /* BUILD_WWAN */
@@ -68,7 +72,9 @@
 #ifdef BUILD_NETWORK
 extern void cc_wifi_panel_static_init_func (void);
 #endif /* BUILD_NETWORK */
+#ifdef BUILD_SHARING
 extern void cc_sharing_panel_static_init_func (void);
+#endif /* BUILD_SHARING */
 #ifdef BUILD_WACOM
 extern void cc_wacom_panel_static_init_func (void);
 #endif /* BUILD_WACOM */
@@ -106,14 +112,18 @@
   PANEL_TYPE("printers",         cc_printers_panel_get_type,             NULL),
   PANEL_TYPE("privacy",          cc_privacy_panel_get_type,              NULL),
   PANEL_TYPE("search",           cc_search_panel_get_type,               NULL),
+#ifdef BUILD_SHARING
   PANEL_TYPE("sharing",          cc_sharing_panel_get_type,              cc_sharing_panel_static_init_func),
+#endif
   PANEL_TYPE("sound",            cc_sound_panel_get_type,                NULL),
   PANEL_TYPE("system",           cc_system_panel_get_type,               NULL),
   PANEL_TYPE("universal-access", cc_ua_panel_get_type,                   NULL),
 #ifdef BUILD_WACOM
   PANEL_TYPE("wacom",            cc_wacom_panel_get_type,                cc_wacom_panel_static_init_func),
 #endif
+#ifdef BUILD_WELLBEING
   PANEL_TYPE("wellbeing",        cc_wellbeing_panel_get_type,            NULL),
+#endif
 #ifdef BUILD_WWAN
   PANEL_TYPE("wwan",             cc_wwan_panel_get_type,                 cc_wwan_panel_static_init_func),
 #endif
