--- src/openfiles.cpp.orig
+++ src/openfiles.cpp
@@ -68,7 +68,11 @@
     goto failsafe;
 
   if (getnameinfo (res->ai_addr, res->ai_addrlen, hostname,
+#ifdef NI_IDN
                    sizeof hostname, service, sizeof service, NI_IDN))
+#else
+                   sizeof hostname, service, sizeof service, 0))
+#endif
     goto failsafe;
 
   if (res)
