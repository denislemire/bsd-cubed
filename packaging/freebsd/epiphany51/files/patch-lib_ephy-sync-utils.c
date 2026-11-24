--- lib/ephy-sync-utils.c.orig
+++ lib/ephy-sync-utils.c
@@ -32,9 +32,9 @@
 #include <webkit/webkit.h>
 
 #include "ephy-settings.h"
-#if defined(__linux__)
+#if defined(__linux__) || defined(__FreeBSD__)
 #include <sys/random.h>
-#elif defined(__FreeBSD__) || defined(__OpenBSD__)
+#elif defined(__OpenBSD__)
 #include <unistd.h>
 #endif
 
