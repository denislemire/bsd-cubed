--- src/ck-device-udev.c.orig
+++ src/ck-device-udev.c
@@ -84,7 +84,11 @@
 /* We may not have the evdev header because of license issues, so
  * we'll define the symbol here */
 #ifndef EVIOCREVOKE
+#ifdef __FreeBSD__
+#define EVIOCREVOKE _IOWINT('E', 0x91)
+#else
 #define EVIOCREVOKE _IOW('E', 0x91, int)
+#endif
 #endif
 
 
@@ -247,7 +251,11 @@
         case DEVICE_EVDEV:
                 if (active == FALSE)
                 {
-                        ck_device_evdev_revoke (fd);
+                        if (!ck_device_evdev_revoke (fd))
+                        {
+                                g_close (fd, NULL);
+                                return -1;
+                        }
                 }
                 break;
         default:
@@ -420,6 +428,7 @@
                 {
                         /* try again in an inactive state */
                         device->fd = ck_device_open (device, FALSE);
+                        device->state = FALSE;
                 }
         }
 
