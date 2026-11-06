Existing non-native workaround reference: https://gitlab.gnome.org/GNOME/mutter/-/issues/2103

--- src/backends/meta-stage-impl.c.orig
+++ src/backends/meta-stage-impl.c
@@ -35,6 +35,9 @@
 #include <math.h>
 
 #include "backends/meta-renderer.h"
+#ifdef HAVE_NATIVE_BACKEND
+#include "backends/native/meta-stage-native.h"
+#endif
 #include "backends/meta-stage-view-private.h"
 #include "clutter/clutter-mutter.h"
 #include "cogl/cogl.h"
@@ -608,9 +611,16 @@
    * artefacts.
    */
   /* swap_region does not need damage history, set it up before that */
-  if (!use_clipped_redraw)
+  /* Preserve the port's existing region workaround on non-native stages.
+   * A native full redraw must use its buffer-swap path: the inherited EGL
+   * region-swap callback may be unavailable and bypasses native presentation.
+   */
+#ifdef HAVE_NATIVE_BACKEND
+  if (!use_clipped_redraw && META_IS_STAGE_NATIVE (stage_window))
     swap_region = mtk_region_create ();
-  else if (clutter_stage_view_has_shadowfb (stage_view))
+  else
+#endif
+  if (clutter_stage_view_has_shadowfb (stage_view))
     swap_region = mtk_region_ref (fb_clip_region);
   else
     swap_region = mtk_region_copy (fb_clip_region);
