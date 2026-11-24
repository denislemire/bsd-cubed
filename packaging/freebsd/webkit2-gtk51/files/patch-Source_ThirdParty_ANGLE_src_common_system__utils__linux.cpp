--- Source/ThirdParty/ANGLE/src/common/system_utils_linux.cpp.orig
+++ Source/ThirdParty/ANGLE/src/common/system_utils_linux.cpp
@@ -18,6 +18,12 @@
 
 #include <array>
 
+#if defined(__FreeBSD__)
+constexpr clockid_t angleMonotonicClock = CLOCK_MONOTONIC;
+#else
+constexpr clockid_t angleMonotonicClock = CLOCK_MONOTONIC_RAW;
+#endif
+
 namespace angle
 {
 std::string GetExecutablePath()
@@ -51,14 +57,14 @@
 double GetCurrentSystemTime()
 {
     struct timespec currentTime;
-    clock_gettime(CLOCK_MONOTONIC_RAW, &currentTime);
+    clock_gettime(angleMonotonicClock, &currentTime);
     return currentTime.tv_sec + currentTime.tv_nsec * 1e-9;
 }
 
 uint64_t GetCurrentSystemTimeNs()
 {
     struct timespec currentTime;
-    clock_gettime(CLOCK_MONOTONIC_RAW, &currentTime);
+    clock_gettime(angleMonotonicClock, &currentTime);
     return static_cast<uint64_t>(currentTime.tv_sec) * 1000'000'000 + currentTime.tv_nsec;
 }
 
