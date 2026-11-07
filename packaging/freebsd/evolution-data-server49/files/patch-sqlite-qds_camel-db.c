--- src/camel/camel-db.c.orig
+++ src/camel/camel-db.c
@@ -871,6 +871,10 @@
 	cdb->priv->filename = g_strdup (filename);
 	d (g_print ("\nDatabase successfully opened  \n"));
 
+	/* Retain SQL compatibility with the FreeBSD SQLite build. */
+	sqlite3_db_config (db, SQLITE_DBCONFIG_DQS_DDL, 1, NULL);
+	sqlite3_db_config (db, SQLITE_DBCONFIG_DQS_DML, 1, NULL);
+
 	sqlite3_create_function (db, "CAMELCOMPAREDATE", 2, SQLITE_UTF8 | SQLITE_DETERMINISTIC, NULL, cdb_camel_compare_date_func, NULL, NULL);
 
 	/* Which is big / costlier ? A Stack frame or a pointer */
