--- src/backends/native/meta-thread.c.orig
+++ src/backends/native/meta-thread.c
@@ -21,6 +21,10 @@
 
 #include <glib.h>
 #include <sys/resource.h>
+#ifdef __FreeBSD__
+#include <pthread.h>
+#include <pthread_np.h>
+#endif
 
 #include "backends/meta-backend-private.h"
 #include "backends/meta-backend-types.h"
@@ -303,6 +307,13 @@
 request_realtime_scheduling (MetaThread  *thread,
                              GError     **error)
 {
+#ifndef RLIMIT_RTTIME
+  /* Do not request unbounded realtime scheduling without its CPU watchdog. */
+  (void) thread;
+  g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
+                       "Realtime scheduling requires RLIMIT_RTTIME support");
+  return FALSE;
+#else
   MetaThreadPrivate *priv = meta_thread_get_instance_private (thread);
   g_autoptr (GError) local_error = NULL;
   int64_t rttime;
@@ -369,6 +380,7 @@
     }
 
   return TRUE;
+#endif
 }
 
 static gboolean
@@ -536,7 +548,11 @@
   meta_profiler_register_thread (profiler, thread_context, priv->name);
 #endif
 
+#ifdef __FreeBSD__
+  priv->kernel.thread_id = pthread_getthreadid_np ();
+#else
   priv->kernel.thread_id = gettid ();
+#endif
   priv->kernel.realtime_inhibit_count = 0;
 
   meta_thread_impl_setup (impl);
