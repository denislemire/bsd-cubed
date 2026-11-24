--- daemon/gdm-dynamic-user-store.c.orig
+++ daemon/gdm-dynamic-user-store.c
@@ -20,7 +20,9 @@
 #include "config.h"
 
 #include <grp.h>
+#ifdef HAVE_USERDB
 #include <shadow.h>
+#endif
 #include <sys/types.h>
 #ifdef HAVE_USERDB
 #include <systemd/sd-varlink.h>
@@ -160,6 +162,7 @@
 
 G_DEFINE_AUTOPTR_CLEANUP_FUNC (WorkerContext, worker_context_free)
 
+#ifdef HAVE_USERDB
 typedef gboolean PwdLock;
 
 static PwdLock
@@ -182,6 +185,7 @@
 }
 
 G_DEFINE_AUTO_CLEANUP_FREE_FUNC (PwdLock, unlock_pwd_db, FALSE)
+#endif
 
 #ifdef HAVE_USERDB
 G_DEFINE_AUTOPTR_CLEANUP_FUNC (sd_varlink_server, sd_varlink_server_unref)
@@ -295,7 +299,9 @@
                                GError              **error)
 {
         g_autofree char *username = NULL;
+#ifdef HAVE_USERDB
         g_auto (PwdLock) pwd_lock = FALSE;
+#endif
         uid_t uid;
         struct group *grp;
         g_autofree char *home = NULL;
@@ -310,7 +316,9 @@
          * race conditions when checking for used UIDs. Otherwise: someone might
          * claim a UID between us deciding that it's available and us starting
          * to advertise it over userdb. */
+#ifdef HAVE_USERDB
         pwd_lock = lock_pwd_db ();
+#endif
 
         if (!pick_uid (store, username, &uid, error))
                 return FALSE;
