--- daemon/gdm-manager.c.orig
+++ daemon/gdm-manager.c
@@ -36,7 +36,11 @@
 
 #include <act/act-user-manager.h>
 
+#ifdef HAVE_BSD_CUBED
+#include <bsd3/sd-login.h>
+#else
 #include <systemd/sd-login.h>
+#endif
 
 #include "gdm-common.h"
 #include "gdm-file-utils.h"
