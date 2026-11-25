--- daemon/gdm-local-display-factory.c.orig
+++ daemon/gdm-local-display-factory.c
@@ -32,7 +32,11 @@
 #include <gudev/gudev.h>
 #endif
 
-#include <systemd/sd-login.h>
+#include <bsd3/sd-login.h>
+#include <bsd3/vt.h>
+#include <sys/consio.h>
+#include <sys/ioctl.h>
+#include <fcntl.h>
 
 #include "gdm-common.h"
 #include "gdm-manager.h"
@@ -279,7 +283,7 @@
                       "session-id", &session_id,
                       NULL);
 
-        sd_seat_get_active (seat_id, &seat_active_session, NULL);
+        bsd3_seat_get_active_session (seat_id, &seat_active_session);
 
         status = gdm_display_get_status (display);
 
@@ -543,6 +547,9 @@
 
         g_debug ("GdmLocalDisplayFactory: display for seat %s requested", seat_id);
 
+        if (g_strcmp0 (seat_id, "seat0") != 0)
+                return;
+
         /* If we already have a login window, switch to it */
         if (gdm_get_login_window_session_id (seat_id, &login_session_id)) {
                 GdmDisplay *display;
@@ -583,7 +590,7 @@
 
         g_clear_handle_id (&factory->seat0_graphics_check_timeout_id, g_source_remove);
 
-        ret = sd_seat_can_graphical (seat_id);
+        ret = bsd3_seat_can_graphical (seat_id);
 
         if (ret < 0) {
                 g_critical ("Failed to query CanGraphical information for seat %s", seat_id);
@@ -764,7 +771,7 @@
         if (!changed)
                 return;
 
-        ret = sd_seat_can_graphical (seat);
+        ret = bsd3_seat_can_graphical (seat);
         if (ret < 0)
                 return;
 
@@ -838,61 +845,20 @@
 }
 
 static gboolean
-on_vt_changed (GIOChannel    *source,
-               GIOCondition   condition,
-               GdmLocalDisplayFactory *factory)
-{
+on_vt_changed (gpointer user_data)
+{
+        GdmLocalDisplayFactory *factory = user_data;
         GdmDisplayStore *store;
-        g_autofree char *tty_of_active_vt = NULL;
-        g_autofree char *login_session_id = NULL;
+        g_autofree char *login_session_id = NULL, *tty_of_active_vt = NULL;
         unsigned int previous_vt, new_vt, login_window_vt = 0;
-        int n_returned;
-
-        g_debug ("GdmLocalDisplayFactory: received VT change event");
-        g_io_channel_seek_position (source, 0, G_SEEK_SET, NULL);
-
-        if (condition & G_IO_PRI) {
-                g_autoptr (GError) error = NULL;
-                GIOStatus status;
-
-                status = g_io_channel_read_line (source, &tty_of_active_vt, NULL, NULL, &error);
-
-                if (error != NULL) {
-                        g_warning ("could not read active VT from kernel: %s", error->message);
-                }
-                switch (status) {
-                        case G_IO_STATUS_ERROR:
-                            return G_SOURCE_REMOVE;
-                        case G_IO_STATUS_EOF:
-                            return G_SOURCE_REMOVE;
-                        case G_IO_STATUS_AGAIN:
-                            return G_SOURCE_CONTINUE;
-                        case G_IO_STATUS_NORMAL:
-                            break;
-                }
-        }
-
-        if ((condition & G_IO_ERR) || (condition & G_IO_HUP)) {
-                g_debug ("GdmLocalDisplayFactory: kernel hung up active vt watch");
-                return G_SOURCE_REMOVE;
-        }
-
-        if (tty_of_active_vt == NULL) {
-                g_debug ("GdmLocalDisplayFactory: unable to read active VT from kernel");
+        int fd = open ("/dev/ttyv0", O_RDONLY | O_NOCTTY | O_CLOEXEC);
+        if (fd < 0)
                 return G_SOURCE_CONTINUE;
-        }
-
-        g_strchomp (tty_of_active_vt);
-
-        errno = 0;
-        n_returned = sscanf (tty_of_active_vt, "tty%u", &new_vt);
-
-        if (n_returned != 1 || errno != 0) {
-                g_critical ("GdmLocalDisplayFactory: Couldn't read active VT (got '%s')",
-                            tty_of_active_vt);
+        int rc = ioctl (fd, VT_GETACTIVE, &new_vt);
+        close (fd);
+        if (rc < 0 || new_vt < 1 || new_vt > 32)
                 return G_SOURCE_CONTINUE;
-        }
-
+        tty_of_active_vt = g_strdup_printf ("ttyv%c", "0123456789abcdefghijklmnopqrstuv"[new_vt - 1]);
         /* don't do anything if we're on the same VT we were before */
         if (new_vt == factory->active_vt) {
                 g_debug ("GdmLocalDisplayFactory: VT changed to the same VT, ignoring");
@@ -1062,16 +1028,8 @@
                                                        factory);
 #endif
 
-        io_channel = g_io_channel_new_file ("/sys/class/tty/tty0/active", "r", NULL);
-
-        if (io_channel != NULL) {
-                factory->active_vt_watch_id =
-                        g_io_add_watch (io_channel,
-                                        G_IO_PRI,
-                                        (GIOFunc)
-                                        on_vt_changed,
-                                        factory);
-        }
+        on_vt_changed (factory);
+        factory->active_vt_watch_id = g_timeout_add_seconds (1, on_vt_changed, factory);
 }
 
 static void
