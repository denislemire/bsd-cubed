--- src/ck-manager.c.orig
+++ src/ck-manager.c
@@ -2909,7 +2909,7 @@ verify_and_open_session_for_leader (CkManager             *manager,
 
         while (!is_local && g_variant_iter_next (iter, "{sv}", &prop_name, &value)) {
                 if (g_strcmp0 (prop_name, "is-local") == 0) {
-                        is_local = TRUE;
+                        is_local = g_variant_get_boolean (value);
                 }
                 if (g_strcmp0 (prop_name, "login-session-id") == 0) {
                         is_local = _verify_login_session_id_is_local (manager, g_variant_get_string (value, 0));
