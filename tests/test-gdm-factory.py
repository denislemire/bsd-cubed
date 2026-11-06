#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
# Execute GDM's actual display-construction block in each platform/build mode.
import argparse
from pathlib import Path
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('source', type=Path)
parser.add_argument('--expect-old-failure', action='store_true')
args = parser.parse_args()
source = args.source.read_text()
patterns = list(re.finditer(r'#(?:ifdef ENABLE_USER_DISPLAY_SERVER|if defined\(ENABLE_USER_DISPLAY_SERVER\) \|\| defined\(__FreeBSD__\))\n        if \(g_strcmp0 \(preferred_display_server', source))
if len(patterns) != 2:
    raise SystemExit('Unexpected GDM factory source layout')
start = patterns[-1].start()
end = source.index('\n        g_object_set (display, "seat-id"', start)
block = source[start:end]
# The generated fixture contains upstream GPL code; retain its license header.
upstream_notice = source[:source.index('#include')]
fixture = upstream_notice + r'''
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#ifdef __FreeBSD__
#undef __FreeBSD__
#endif
#if BSD3_TEST_FREEBSD
#define __FreeBSD__ 1
#endif
#if BSD3_TEST_USER_DISPLAY
#define ENABLE_USER_DISPLAY_SERVER 1
#endif
typedef uint32_t guint32;
typedef struct {int kind;} GdmDisplay;
static GdmDisplay native={1}, legacy={2};
static GdmDisplay *gdm_local_display_new(void) {return &native;}
static GdmDisplay *gdm_legacy_display_new(guint32 n) {(void)n;return &legacy;}
static guint32 take_next_display_number(void *p) {(void)p;return 0;}
#define g_strcmp0 strcmp
#define g_object_set(...) ((void)0)
static int choose(const char *preferred_display_server)
{
    GdmDisplay *display=NULL;
    void *factory=NULL;
    const char *session_types[]={"wayland",NULL};
    const char *legacy_session_types[]={"x11",NULL};
    (void)session_types;(void)legacy_session_types;(void)preferred_display_server;
''' + block + r'''
    return display->kind;
}
int main(void)
{
    const char *modes[]={"wayland","xorg","legacy-xorg"};
    for(unsigned i=0;i<3;i++) {
        int expected=2;
        if(BSD3_TEST_USER_DISPLAY && i<2) expected=1;
        if(BSD3_TEST_FREEBSD && i==0) expected=1;
        if(choose(modes[i])!=expected) return 1;
    }
    return 0;
}
'''
# Avoid unused-function diagnostics when the real conditional excludes native.
fixture = fixture.replace('static GdmDisplay *gdm_local_display_new', 'static __attribute__((unused)) GdmDisplay *gdm_local_display_new')
failed = []
with tempfile.TemporaryDirectory(prefix='bsd3-gdm-factory-') as directory:
    work=Path(directory)
    (work/'fixture.c').write_text(fixture)
    for bsd in (0,1):
        for user in (0,1):
            exe=work/f'factory-{bsd}-{user}'
            subprocess.run(['cc','-std=c11','-Wall','-Wextra','-Werror',f'-DBSD3_TEST_FREEBSD={bsd}',f'-DBSD3_TEST_USER_DISPLAY={user}',str(work/'fixture.c'),'-o',str(exe)],check=True)
            if subprocess.run([str(exe)]).returncode:
                failed.append((bsd,user))
if args.expect_old_failure:
    if failed != [(1,0)]:
        raise SystemExit(f'Unexpected old-source failures: {failed}')
    print('PASS: old factory fails the FreeBSD Wayland/native-display regression')
elif failed:
    raise SystemExit(f'Factory regressions: {failed}')
else:
    print('PASS: actual GDM factory, FreeBSD/Linux, user-display on/off, Wayland/Xorg/legacy')
