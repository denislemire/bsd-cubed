--- daemon/gdm-session-worker-job.c.orig
+++ daemon/gdm-session-worker-job.c
@@ -36,9 +36,10 @@
 #include <sys/prctl.h>
 #endif
 
+
+
+#ifdef ENABLE_SYSTEMD_JOURNAL
 #include <systemd/sd-daemon.h>
-
-#ifdef ENABLE_SYSTEMD_JOURNAL
 #include <systemd/sd-journal.h>
 #endif
 
