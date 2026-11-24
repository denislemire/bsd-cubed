--- test/gjs-tests.cpp.orig
+++ test/gjs-tests.cpp
@@ -1068,4 +1068,4 @@
-    char32_t random_char32 = get_random_number<char32_t>();
+    char32_t random_char32 = static_cast<char32_t>(get_random_number<uint32_t>());
     gjs_arg_set(&arg, random_char32);
     assert_equal(static_cast<char32_t>(arg.v_uint32), random_char32);
     assert_equal(gjs_arg_get<char32_t>(&arg), random_char32);
