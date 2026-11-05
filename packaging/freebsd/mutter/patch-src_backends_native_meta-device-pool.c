--- src/backends/native/meta-device-pool.c.orig
+++ src/backends/native/meta-device-pool.c
@@ -22,7 +22,9 @@
 #include <fcntl.h>
 #include <gio/gunixfdlist.h>
 #include <sys/stat.h>
+#ifndef __FreeBSD__
 #include <sys/sysmacros.h>
+#endif
 #include <sys/types.h>
 
 #include "backends/native/meta-backend-native.h"
