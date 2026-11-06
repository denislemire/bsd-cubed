--- block.cpp.orig
+++ block.cpp
@@ -142,6 +142,11 @@
 
 bool Block::hintIgnore() const
 {
+    // GEOM partition metadata takes precedence over stale filesystem signatures.
+    // Swap must stay hidden even when it retains an old NTFS boot sector.
+    if(bPartition && bPartition->partitionType == QStringLiteral("freebsd-swap"))
+        return true;
+
     if(bPartition && parentDrive)
     {
         // there should be better predicate for this, probably
