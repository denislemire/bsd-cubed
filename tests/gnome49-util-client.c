/* SPDX-License-Identifier: BSD-2-Clause
 * Original harness. Linking this test to GNOME's GPL source produces a GPL
 * executable, not a BSD-licensed replacement for that source.
 */
#include "config.h"
#include <gio/gio.h>
#include "gsm-util.h"
#include <stdio.h>

int
main(int argc, char **argv)
{
    GError *error = NULL;
    if (argc != 2)
        return 64;
    if (!gsm_util_export_user_environment(&error)) {
        fprintf(stderr, "%s\n", error->message);
        g_error_free(error);
        return 1;
    }
    if (!gsm_util_setenv_native("BSD3_TEST_VALUE", argv[1], &error)) {
        fprintf(stderr, "%s\n", error->message);
        g_error_free(error);
        return 1;
    }
    return 0;
}
