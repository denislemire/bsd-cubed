#!/usr/bin/env python3
# SPDX-License-Identifier: BSD-2-Clause
"""Compile the actual consumer function; exercise native fd and type contracts.

The upstream GPL function is extracted at runtime, never copied into BSD3 code.
Synthetic filesystem metadata checks classification, not a real autofs mount.
"""
import os
from pathlib import Path
import subprocess
import sys
import tempfile

if not sys.platform.startswith('freebsd') or os.getuid() == 0:
    raise SystemExit('ordinary native FreeBSD user required')
if len(sys.argv) != 2:
    raise SystemExit('usage: native-nautilus49-autofs.py patched-nautilus-source')
source = (Path(sys.argv[1]) / 'src/nautilus-file-utilities.c').read_text()
start = source.index('gboolean\nnautilus_location_is_autofs_mountpoint (GFile *location)')
end = source.index('\n}', start) + 2
function = source[start:end]
prefix = r'''
#include <sys/param.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <gio/gio.h>
#include <glib/gstdio.h>
#define AUTOFS_SUPER_MAGIC 0x0187
static int mode, calls, observed_fd = -1;
static int fixture_fstatfs(int fd, struct statfs *buf)
{
    calls++;
    observed_fd = fd;
    /* Real O_PATH descriptor and real native syscall even in synthetic cases. */
    if (fstatfs(fd, buf) < 0)
        return -1;
    g_assert_true((fcntl(fd, F_GETFL) & O_PATH) != 0);
    if (mode == 1) {
        g_strlcpy(buf->f_fstypename, "autofs", sizeof(buf->f_fstypename));
        buf->f_type = 0; /* Deliberately not Linux's numeric magic. */
    } else if (mode == 2) {
        g_strlcpy(buf->f_fstypename, "ufs", sizeof(buf->f_fstypename));
        buf->f_type = AUTOFS_SUPER_MAGIC; /* Must not produce a false positive. */
    } else if (mode == 3) {
        errno = EIO;
        return -1;
    }
    return 0;
}
#define fstatfs fixture_fstatfs
'''
suffix = r'''
#undef fstatfs
static void check(GFile *file, int selected, gboolean expected, int expected_calls)
{
    mode = selected;
    calls = 0;
    observed_fd = -1;
    g_assert_cmpint(nautilus_location_is_autofs_mountpoint(file), ==, expected);
    g_assert_cmpint(calls, ==, expected_calls);
    if (observed_fd >= 0) {
        errno = 0;
        g_assert_cmpint(fcntl(observed_fd, F_GETFD), ==, -1);
        g_assert_cmpint(errno, ==, EBADF);
    }
}
int main(int argc, char **argv)
{
    struct statfs actual;
    g_assert_cmpint(argc, ==, 2);
    g_autoptr(GFile) directory = g_file_new_for_path(argv[1]);
    g_autoptr(GFile) missing = g_file_get_child(directory, "absent");
    g_autoptr(GFile) remote = g_file_new_for_uri("sftp://example.invalid/path");
    g_assert_cmpint(statfs(argv[1], &actual), ==, 0);
    check(directory, 0, g_str_equal(actual.f_fstypename, "autofs"), 1);
    check(directory, 1, TRUE, 1);
    check(directory, 2, FALSE, 1);
    check(directory, 3, FALSE, 1);
    check(missing, 0, FALSE, 0);
    check(remote, 0, FALSE, 0);
    g_print("PASS: actual Nautilus autofs function, native O_PATH/fstatfs, "
            "six classification/error cases and fd cleanup; real autofs mount untested\n");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix='bsd3-nautilus-autofs-') as directory:
    root = Path(directory)
    unit = root / 'consumer.c'
    unit.write_text(prefix + function + suffix)
    flags = subprocess.check_output(
        ['pkg-config', '--cflags', '--libs', 'gio-2.0'], text=True, timeout=10).split()
    binary = root / 'consumer'
    subprocess.run(['cc', '-std=c11', '-D_GNU_SOURCE', '-Wall', '-Wextra', '-Werror',
                    str(unit), '-o', str(binary), *flags], check=True, timeout=30)
    subprocess.run([str(binary), str(root)], check=True, timeout=10)
