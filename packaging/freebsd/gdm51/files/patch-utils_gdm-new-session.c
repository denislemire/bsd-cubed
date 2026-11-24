--- utils/gdm-new-session.c.orig
+++ utils/gdm-new-session.c
@@ -8,7 +8,11 @@
 #include <gio/gio.h>
 #include <glib-unix.h>
 #include <sysexits.h>
+#ifdef HAVE_BSD_CUBED
+#include <bsd3/sd-login.h>
+#else
 #include <systemd/sd-login.h>
+#endif
 
 #define GDM_DBUS_NAME                        "org.gnome.DisplayManager"
 #define GDM_LOCAL_DISPLAY_FACTORY_PATH       "/org/gnome/DisplayManager/LocalDisplayFactory"
