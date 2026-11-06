#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Exercise the actual FreeBSD wallpaper decoder with real GdkPixbuf/GTask."""
from pathlib import Path
import subprocess
import sys
import tempfile

source = Path(sys.argv[1]).read_text()
start = source.index("static void\nload_file (")
end = source.index("\nstatic void\nfile_loaded", start)
function = source[start:end]
notice = source[:source.index('#include "config.h"')]
fixture = r'''
#include <gio/gio.h>
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <glib/gstdio.h>
#include <assert.h>
typedef struct { GFile *file; } MetaBackgroundImage;
'''
main = r'''
static GdkPixbuf *decode(const char *path, GCancellable *cancel, GError **error) {
 MetaBackgroundImage source={g_file_new_for_path(path)};
 GTask *task=g_task_new(NULL,NULL,NULL,NULL);
 load_file(task,&source,NULL,cancel);
 GdkPixbuf *result=g_task_propagate_pointer(task,error);
 g_object_unref(source.file);g_object_unref(task);return result;
}
int main(void) {
 GError *error=NULL;
 char *dir=g_dir_make_tmp("bsd3-background-XXXXXX",&error);assert(dir && !error);
 char *path=g_build_filename(dir,"fixture.png",NULL);
 for(int alpha=0;alpha<2;alpha++) {
  GdkPixbuf *pixels=gdk_pixbuf_new(GDK_COLORSPACE_RGB,alpha,8,3,2);assert(pixels);
  gdk_pixbuf_fill(pixels,0x20408080);
  assert(gdk_pixbuf_save(pixels,path,"png",&error,NULL) && !error);
  GdkPixbuf *loaded=decode(path,NULL,&error);assert(loaded && !error);
  assert(gdk_pixbuf_get_width(loaded)==3 && gdk_pixbuf_get_height(loaded)==2);
  assert(gdk_pixbuf_get_has_alpha(loaded)==alpha);
  const guchar *data=gdk_pixbuf_get_pixels(loaded);
  assert(data[0]==0x20 && data[1]==0x40 && data[2]==0x80);
  if(alpha)assert(data[3]==0x80);
  g_object_unref(loaded);g_object_unref(pixels);
 }
 GCancellable *cancel=g_cancellable_new();g_cancellable_cancel(cancel);
 assert(!decode(path,cancel,&error));assert(g_error_matches(error,G_IO_ERROR,G_IO_ERROR_CANCELLED));
 g_clear_error(&error);g_object_unref(cancel);
 assert(g_file_set_contents(path,"not an image",-1,&error) && !error);
 assert(!decode(path,NULL,&error) && error);g_clear_error(&error);
 assert(g_remove(path)==0);
 assert(!decode(path,NULL,&error));assert(g_error_matches(error,G_IO_ERROR,G_IO_ERROR_NOT_FOUND));
 g_clear_error(&error);assert(g_rmdir(dir)==0);g_free(path);g_free(dir);
 return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="bsd3-mutter49-background-") as directory:
    root = Path(directory)
    (root / "fixture.c").write_text(notice + fixture + function + main)
    flags = subprocess.check_output(
        ["pkg-config", "--cflags", "--libs", "gdk-pixbuf-2.0", "gio-2.0"], text=True).split()
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror",
                    "-Wno-unused-parameter", str(root / "fixture.c"), "-o", str(root / "test"), *flags], check=True)
    subprocess.run([str(root / "test")], check=True)
print("PASS: actual Mutter49 FreeBSD background decoder, RGB/RGBA pixels, cancellation, malformed/missing files")
