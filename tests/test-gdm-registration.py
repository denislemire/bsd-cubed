#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
# Compile the actual consumer registration block with real typed GVariants.
import pathlib, subprocess, sys, tempfile
source = pathlib.Path(sys.argv[1]).read_text()
start = source.index('        g_variant_builder_init (&builder, G_VARIANT_TYPE ("a(sv)"));')
end = source.index('        g_variant_builder_add_parsed (&builder, "(\'x11-display\',', start)
block = source[start:end]
header = source[:source.index('#include')]
fixture = r"""
#include <gio/gio.h>
#include <assert.h>
#include <string.h>
typedef struct { int session_vt, session_tty_fd; const char *type; } Worker;
static const char *gdm_session_worker_get_environment_variable(Worker *w,const char *name)
{ assert(!strcmp(name,"XDG_SESSION_TYPE")); return w->type; }
#ifdef __FreeBSD__
#undef __FreeBSD__
#endif
#define __FreeBSD__ 1
static GVariant *build(Worker *worker)
{
 GVariantBuilder builder;gint32 uid=92;
 const char *display_device="/dev/ttyv8";
"""
main = r"""
 return g_variant_ref_sink(g_variant_builder_end(&builder));
}
int main(void) {
 const char *types[]={"wayland","x11",NULL};
 for(unsigned i=0;i<3;i++)for(int valid=0;valid<2;valid++) {
  Worker w={9,valid?4:-1,types[i]};GVariant *params=build(&w);
  GVariantIter it;const char *name;GVariant *value;int tty=0,x11=0,vt=0;
  g_variant_iter_init(&it,params);
  while(g_variant_iter_next(&it,"(&sv)",&name,&value)) {
   if(!strcmp(name,"display-device")){assert(!strcmp(g_variant_get_string(value,NULL),"/dev/ttyv8"));tty++;}
   if(!strcmp(name,"x11-display-device")){assert(!strcmp(g_variant_get_string(value,NULL),i==0?"":"/dev/ttyv8"));x11++;}
   if(!strcmp(name,"vtnr")){assert(valid && g_variant_get_uint32(value)==9);vt++;}
   g_variant_unref(value);
  }
  assert(tty==1 && x11==1 && vt==valid);g_variant_unref(params);
 }
 return 0;
}
"""
with tempfile.TemporaryDirectory(prefix="bsd3-gdm-registration-") as directory:
    root = pathlib.Path(directory)
    (root / "fixture.c").write_text(header + fixture + block + main)
    flags = subprocess.check_output(["pkg-config", "--cflags", "--libs", "gio-2.0"], text=True).split()
    subprocess.run(["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", str(root / "fixture.c"), "-o", str(root / "fixture"), *flags], check=True)
    subprocess.run([str(root / "fixture")], check=True)
print("PASS: actual GDM registration, native display device, Wayland/X11 separation, owned VT number")
