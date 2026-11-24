--- Source/WebKit/UIProcess/DisplayLinkProcessProxyClient.cpp.orig
+++ Source/WebKit/UIProcess/DisplayLinkProcessProxyClient.cpp
@@ -31,6 +31,7 @@
 #include "EventDispatcherMessages.h"
 #include "WebProcessMessages.h"
 #include <wtf/TZoneMallocInlines.h>
+#include <wtf/Threading.h>
 
 namespace WebKit {
 
