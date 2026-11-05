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
@@ -536,7 +540,11 @@
   meta_profiler_register_thread (profiler, thread_context, priv->name);
 #endif
 
+#ifdef __FreeBSD__
+  priv->kernel.thread_id = pthread_getthreadid_np ();
+#else
   priv->kernel.thread_id = gettid ();
+#endif
   priv->kernel.realtime_inhibit_count = 0;
 
   meta_thread_impl_setup (impl);
