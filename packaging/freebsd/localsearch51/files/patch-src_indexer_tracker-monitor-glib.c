--- src/indexer/tracker-monitor-glib.c.orig
+++ src/indexer/tracker-monitor-glib.c
@@ -269,18 +269,18 @@
 		 * negative maximum.
 		 */
 		priv->monitor_limit = MAX (priv->monitor_limit, 0);
-	}
+	} else
 #endif /* __linux__ */
 #ifdef TRACKER_MONITOR_KQUEUE
-	else if (strcmp (name, "GKqueueDirectoryMonitor") == 0 ||
+	if (strcmp (name, "GKqueueDirectoryMonitor") == 0 ||
 	           strcmp (name, "GKqueueFileMonitor") == 0) {
 		/* Using kqueue(2) */
 		TRACKER_NOTE (MONITORS, g_message ("Monitor backend is kqueue"));
 
 		priv->monitor_limit = get_kqueue_limit ();
-	}
+	} else
 #endif /* TRACKER_MONITOR_KQUEUE */
-	else {
+	{
 		/* Unknown */
 		g_warning ("Monitor backend:'%s' is unhandled. Monitoring will be disabled",
 		           name);
