--- src/backends/native/meta-launcher.c.orig
+++ src/backends/native/meta-launcher.c
@@ -22,14 +22,16 @@
 #include <gio/gunixfdlist.h>
 #include <sys/types.h>
 #include <sys/stat.h>
+#ifndef __FreeBSD__
 #include <sys/sysmacros.h>
+#endif
 #include <malloc.h>
 #include <fcntl.h>
 #include <errno.h>
 #include <stdlib.h>
 #include <string.h>
 #include <unistd.h>
-#include <systemd/sd-login.h>
+#include <bsd3/sd-login.h>
 
 #include "backends/meta-backend-private.h"
 #include "backends/native/dbus-utils.h"
