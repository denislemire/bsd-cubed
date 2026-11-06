--- gi/ns.cpp.orig
+++ gi/ns.cpp
@@ -99,8 +99,44 @@
             return true;  // not resolved, but no error
         }
 
+        bool legacy_alias = false;
         Maybe<GI::AutoBaseInfo> info{
             GI::Repository{}.find_by_name(get(), name.get())};
+        // GIRepository2 no longer resolves these legacy GLib aliases. Keep
+        // GJS's documented compatibility path and its existing deprecation.
+        if (!info && m_is_glib) {
+            const char* platform_ns = nullptr;
+            const char* alias = nullptr;
+#ifdef G_OS_UNIX
+            if (g_str_has_prefix(name.get(), "Unix")) {
+                platform_ns = "GLibUnix";
+                alias = name.get() + 4;
+            } else if (g_str_has_prefix(name.get(), "unix_")) {
+                platform_ns = "GLibUnix";
+                alias = name.get() + 5;
+            }
+#endif
+#ifdef G_OS_WIN32
+            if (g_str_has_prefix(name.get(), "Win32")) {
+                platform_ns = "GLibWin32";
+                alias = name.get() + 5;
+            } else if (g_str_has_prefix(name.get(), "win32_")) {
+                platform_ns = "GLibWin32";
+                alias = name.get() + 6;
+            }
+#endif
+            if (platform_ns) {
+                GI::Repository repo;
+                auto required = repo.require(platform_ns, "2.0");
+                if (!required.isOk()) {
+                    gjs_throw(cx, "Failed to require %s: %s", platform_ns,
+                              required.inspectErr()->message);
+                    return false;
+                }
+                info = repo.find_by_name(platform_ns, alias);
+                legacy_alias = bool(info);
+            }
+        }
         if (!info) {
             *resolved = false;  // No property defined, but no error either
             return true;
@@ -115,6 +151,17 @@
             platform_specific_warning_glib(cx, "unix_", "Unix", name.get());
             platform_specific_warning_glib(cx, "Win32", "Win32", name.get());
             platform_specific_warning_glib(cx, "win32_", "Win32", name.get());
+        }
+
+        if (legacy_alias) {
+            JS::RootedObject canonical(cx, gjs_lookup_namespace_object(cx, info.ref()));
+            JS::RootedValue value(cx);
+            if (!canonical ||
+                !JS_GetProperty(cx, canonical, info->name(), &value) ||
+                !JS_DefinePropertyById(cx, obj, id, value, GJS_MODULE_PROP_FLAGS))
+                return false;
+            *resolved = true;
+            return true;
         }
 
         bool defined;
