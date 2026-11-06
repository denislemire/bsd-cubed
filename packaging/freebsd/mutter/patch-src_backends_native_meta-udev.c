--- src/backends/native/meta-udev.c.orig
+++ src/backends/native/meta-udev.c
@@ -147,7 +147,37 @@
 
   device_type = g_udev_device_get_property (device, "DEVTYPE");
   if (g_strcmp0 (device_type, DRM_CARD_UDEV_DEVICE_TYPE) != 0)
-    return FALSE;
+    {
+#ifdef __FreeBSD__
+      const char *name = g_udev_device_get_name (device);
+      const char *path = g_udev_device_get_device_file (device);
+      const char *number;
+
+      /* libudev-devd supplies DRM nodes without Linux DEVTYPE metadata.
+       * Accept only canonical numeric DRM nodes in the DRM subsystem.
+       * Explicit incompatible metadata must still be rejected.
+       */
+      if (device_type || !name || !path ||
+          g_strcmp0 (g_udev_device_get_subsystem (device), "drm") != 0)
+        return FALSE;
+      if (g_str_has_prefix (name, "card"))
+        number = name + 4;
+      else if (g_str_has_prefix (name, "renderD"))
+        number = name + 7;
+      else
+        return FALSE;
+      if (!*number)
+        return FALSE;
+      for (; *number; number++)
+        if (*number < '0' || *number > '9')
+          return FALSE;
+      if (!g_str_has_prefix (path, "/dev/dri/") ||
+          g_strcmp0 (path + 9, name) != 0)
+        return FALSE;
+#else
+      return FALSE;
+#endif
+    }
 
   device_seat = g_udev_device_get_property (device, "ID_SEAT");
   if (!device_seat)
@@ -179,7 +209,12 @@
     {
     case META_UDEV_DEVICE_TYPE_CARD:
       g_udev_enumerator_add_match_name (enumerator, "card*");
+#ifndef __FreeBSD__
+      /* FreeBSD's devd compatibility layer does not assign udev tags.
+       * Character-device, DRM identity and seat checks remain below.
+       */
       g_udev_enumerator_add_match_tag (enumerator, "seat");
+#endif
       break;
     case META_UDEV_DEVICE_TYPE_RENDER_NODE:
       g_udev_enumerator_add_match_name (enumerator, "render*");
