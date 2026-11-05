--- src/backends/native/meta-kms-impl-device.c.orig
+++ src/backends/native/meta-kms-impl-device.c
@@ -22,7 +22,9 @@
 #include <errno.h>
 #include <fcntl.h>
 #include <glib/gstdio.h>
+#ifdef __linux__
 #include <linux/dma-buf.h>
+#endif
 #include <poll.h>
 #include <sys/ioctl.h>
 #include <sys/timerfd.h>
