#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Execute the actual region/swap branches with an absent EGL region callback."""
from pathlib import Path
import os
import subprocess
import sys
import tempfile

source = Path(sys.argv[1]).read_text()
start = source.index("  /* swap_region does not need damage history")
end = source.index("\n  swap_with_damage = FALSE;", start)
region = source[start:end]
start = source.index("      if (n_rects > 0 && !swap_with_damage)")

def block_end(text, start):
    opening = text.index("{", start)
    depth = 0
    for i in range(opening, len(text)):
        if text[i] == "{":
            depth += 1
        elif text[i] == "}":
            depth -= 1
            if depth == 0:
                return i + 1
    raise ValueError("Unclosed source block")

end = block_end(source, start)
end = block_end(source, source.index("else", end))
swap = source[start:end]
stub = r'''
#include <stdio.h>
#include <stdlib.h>
typedef struct { int native; } ClutterStageWindow;
typedef struct { int shadow; } ClutterStageView;
typedef struct { int rectangles; } MtkRegion;
static MtkRegion empty={0},filled={1};
static MtkRegion *mtk_region_create(void) {return &empty;}
static MtkRegion *mtk_region_ref(MtkRegion *r) {return r;}
static MtkRegion *mtk_region_copy(MtkRegion *r) {return r;}
static int clutter_stage_view_has_shadowfb(ClutterStageView *v) {return v->shadow;}
#define META_IS_STAGE_NATIVE(w) ((w)->native)
#define meta_topic(...) ((void)0)
static int available, route;
static void cogl_onscreen_swap_region(void *o,int *d,int n,void *i,void *f) {
 (void)o;(void)d;(void)n;(void)i;(void)f;
 if(!available) {fputs("FAIL: selected absent EGL region-swap callback\n",stderr);exit(1);}
 route=1;
}
static void cogl_onscreen_swap_buffers_with_damage(void *o,int *d,int n,void *i,void *f) {
 (void)o;(void)d;(void)n;(void)i;(void)f;route=2;
}
static void check(int native,int clipped,int age,int shadow,int supported,int want) {
 ClutterStageWindow window={native}, *stage_window=&window;
 ClutterStageView view={shadow}, *stage_view=&view;
 int use_clipped_redraw=clipped;
 MtkRegion *fb_clip_region=&filled, *swap_region=NULL;
'''
after_region = r'''
 /* A clipped redraw with valid buffer age uses the damage-swap path.
  * Full redraws leave swap_with_damage false in the actual stage code.
  */
 int swap_with_damage=age && clipped;
 int n_rects=swap_region->rectangles, damage[4]={0,0,1920,1080};
 void *onscreen=NULL,*frame_info=NULL,*frame=NULL;
 available=supported;route=0;
'''
main = r'''
 if(route!=want) {fprintf(stderr,"FAIL route native=%d clipped=%d age=%d shadow=%d: %d != %d\n",native,clipped,age,shadow,route,want);exit(2);}
}
int main(void) {
#ifdef HAVE_NATIVE_BACKEND
 for(int shadow=0;shadow<2;shadow++) {
  check(1,0,0,shadow,0,2);
  check(1,0,1,shadow,0,2);
  check(1,1,1,shadow,0,2);
  check(1,1,0,shadow,1,1);
 }
#endif
 /* Preserve both shadowfb cases of the existing non-native workaround. */
 for(int shadow=0;shadow<2;shadow++) {
  check(0,0,0,shadow,1,1);
  check(0,0,1,shadow,1,1);
  check(0,1,1,shadow,0,2);
  check(0,1,0,shadow,1,1);
 }
 puts("Actual stage region/swap fixtures passed");return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="bsd3-frame-test-") as directory:
    for native in (False, True):
        define = "\n#define HAVE_NATIVE_BACKEND 1\n" if native else "\n#undef HAVE_NATIVE_BACKEND\n"
        c = Path(directory) / "fixture.c"
        binary = Path(directory) / "fixture"
        c.write_text(define + stub + region + after_region + swap + main)
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function", "-Wno-unused-variable", str(c), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
