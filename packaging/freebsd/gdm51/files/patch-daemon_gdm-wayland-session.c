--- daemon/gdm-wayland-session.c.orig
+++ daemon/gdm-wayland-session.c
@@ -553,7 +553,7 @@
                 goto out;
         }
 
-        import_environment (state, state->cancellable);
+        /* The native service supervisor inherits the PAM environment. */
 
         ret = spawn_session (state, state->cancellable);
 
