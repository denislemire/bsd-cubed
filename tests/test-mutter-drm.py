#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Compile the actual patched Mutter filter with deterministic GUdev fixtures."""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

source = Path(sys.argv[1]).read_text()
start = source.index("gboolean\nmeta_udev_is_drm_device (")
end = source.index("\nGList *", start)
predicate = source[start:end]
start = source.index("  switch (device_type)", end)
end = source.index("\n  /*", start)
enumeration = source[start:end]
stub = r'''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef int gboolean;
#define TRUE 1
#define FALSE 0
#define G_UDEV_DEVICE_TYPE_CHAR 1
#define DRM_CARD_UDEV_DEVICE_TYPE "drm_minor"
typedef struct { const char *seat; } MetaBackendNative;
typedef struct { MetaBackendNative *backend_native; } MetaUdev;
typedef struct {
 int type;
 const char *devtype, *seat, *name, *path, *subsystem;
} GUdevDevice;
static int g_strcmp0(const char *a,const char *b) {
 return a && b ? strcmp(a,b) : a != b;
}
static int g_str_has_prefix(const char *s,const char *p) {
 return strncmp(s,p,strlen(p))==0;
}
static int g_udev_device_get_device_type(GUdevDevice *d) {return d->type;}
static const char *g_udev_device_get_property(GUdevDevice *d,const char *p) {
 return strcmp(p,"DEVTYPE")==0 ? d->devtype : d->seat;
}
static const char *g_udev_device_get_name(GUdevDevice *d) {return d->name;}
static const char *g_udev_device_get_device_file(GUdevDevice *d) {return d->path;}
static const char *g_udev_device_get_subsystem(GUdevDevice *d) {return d->subsystem;}
static const char *meta_backend_native_get_seat_id(MetaBackendNative *u) {return u->seat;}
static int tags;
static void g_udev_enumerator_add_match_name(void *e,const char *s) {(void)e;(void)s;}
static void g_udev_enumerator_add_match_tag(void *e,const char *s) {(void)e;(void)s;tags++;}
enum { META_UDEV_DEVICE_TYPE_CARD, META_UDEV_DEVICE_TYPE_RENDER_NODE };
'''
main = r'''
static void check(MetaUdev *u, GUdevDevice d,int want,const char *label) {
 if (meta_udev_is_drm_device(u,&d)!=want) {
  fprintf(stderr,"FAIL: %s\n",label); exit(1);
 }
}
int main(void) {
 MetaBackendNative owner={"seat0"};
 MetaUdev u={&owner};
 GUdevDevice d={1,NULL,NULL,"card0","/dev/dri/card0","drm"};
#ifdef __FreeBSD__
 check(&u,d,1,"FreeBSD card without Linux metadata");
 d.name="renderD128"; d.path="/dev/dri/renderD128";
 check(&u,d,1,"canonical render node");
#else
 check(&u,d,0,"Linux still requires DEVTYPE");
#endif
 d=(GUdevDevice){1,"drm_minor",NULL,"card0","/dev/dri/card0","drm"};
 check(&u,d,1,"existing Linux metadata");
 d.seat="seat1"; check(&u,d,0,"foreign seat");
 d.seat=NULL; d.type=0; check(&u,d,0,"not a character device");
 d.type=1; d.devtype="other"; check(&u,d,0,"conflicting DEVTYPE");
 d.devtype=NULL;
 const char *names[]={"card","card0-HDMI-A-1","card-1","card1/../../ttyv0","renderD","renderD128x",NULL};
 for(int i=0;names[i];i++) {d.name=names[i];check(&u,d,0,"invalid DRM name");}
 d.name="card0"; d.path="/dev/input/event0"; check(&u,d,0,"wrong device path");
 d.path="/dev/dri/card1"; check(&u,d,0,"different node identity");
 d.path=NULL; check(&u,d,0,"missing path");
 d.path="/dev/dri/card0"; d.name=NULL; check(&u,d,0,"missing name");
 d.name="card0"; d.subsystem="input"; check(&u,d,0,"foreign subsystem");
 d.subsystem=NULL; check(&u,d,0,"missing subsystem");
 d.subsystem="drm"; d.seat="seat1";check(&u,d,0,"foreign FreeBSD seat");
 enumerate_card();
#ifdef __FreeBSD__
 if(tags!=0) return 2;
#else
 if(tags!=1) return 3;
#endif
 puts("DRM discovery fixtures passed"); return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="bsd3-drm-test-") as temp:
    for freebsd in (False, True):
        # Define the target selector after host headers, so the host libc's
        # declarations remain correct when testing FreeBSD behavior on macOS.
        define = "\n#define __FreeBSD__ 1\n" if freebsd else "\n#undef __FreeBSD__\n"
        text = stub + define + predicate
        text += "\nstatic void enumerate_card(void) { void *enumerator=NULL; int device_type=META_UDEV_DEVICE_TYPE_CARD;\n"
        text += enumeration + "\n}\n" + main
        c = Path(temp) / "fixture.c"
        binary = Path(temp) / "fixture"
        c.write_text(text)
        subprocess.run([os.environ.get("CC", "cc"), "-std=c11", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function", str(c), "-o", str(binary)], check=True)
        subprocess.run([str(binary)], check=True)
