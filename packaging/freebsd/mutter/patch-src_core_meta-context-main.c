--- src/core/meta-context-main.c.orig
+++ src/core/meta-context-main.c
@@ -25,7 +25,9 @@
 #include <glib.h>
 #include <gio/gio.h>
 
-#ifdef HAVE_WAYLAND
+#if defined(HAVE_WAYLAND) && defined(HAVE_NATIVE_BACKEND) && defined(__FreeBSD__)
+#include <bsd3/sd-login.h>
+#elif defined(HAVE_LIBSYSTEMD) && defined(HAVE_WAYLAND)
 #include <systemd/sd-login.h>
 #endif
 
@@ -348,8 +350,10 @@
 #ifdef HAVE_WAYLAND
       if (context_main->options.no_x11)
         return META_X11_DISPLAY_POLICY_DISABLED;
+#ifdef HAVE_LIBSYSTEMD
       else if (sd_pid_get_user_unit (0, &unit) < 0)
         return META_X11_DISPLAY_POLICY_MANDATORY;
+#endif
       else
         return META_X11_DISPLAY_POLICY_ON_DEMAND;
 #else /* HAVE_WAYLAND */
