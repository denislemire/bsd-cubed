--- gi/info.h.orig
+++ gi/info.h
@@ -883,7 +883,7 @@
         return gi_callable_info_can_throw_gerror(ptr());
     }
     [[nodiscard]]
-    void** closure_native_address(ffi_closure* closure) const {
+    void* closure_native_address(ffi_closure* closure) const {
         return gi_callable_info_get_closure_native_address(ptr(), closure);
     }
     [[nodiscard]]
