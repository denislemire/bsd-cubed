/* SPDX-License-Identifier: BSD-2-Clause */
#include <gio/gdesktopappinfo.h>
#include <glib/gstdio.h>
#include "gsm-app.h"
static char *directory;
static void entry(const char *id, const char *phase, gboolean accepted, gboolean disabled)
{
    char *path = g_build_filename(directory, id, NULL);
    char *text = g_strdup_printf("[Desktop Entry]\nType=Application\nName=Fixture\nExec=/usr/bin/true\nOnlyShowIn=GNOME;\nX-GNOME-Autostart-Phase=%s\n", phase);
    GError *error = NULL;
    GsmApp *app;
    g_assert_true(g_file_set_contents(path, text, -1, &error));
    g_assert_no_error(error);
    app = gsm_app_new_for_path(path, &error);
    if (accepted) {
        g_assert_no_error(error); g_assert_nonnull(app);
        g_assert_cmpint(gsm_app_peek_is_disabled(app), ==, disabled);
        g_object_unref(app);
    } else {
        g_assert_null(app); g_assert_error(error, G_IO_ERROR, G_IO_ERROR_FAILED);
        g_clear_error(&error);
    }
    g_assert_cmpint(g_remove(path), ==, 0);
    g_free(text); g_free(path);
}
int main(void)
{
    const char *names[] = {"A11ySettings", "Color", "Keyboard", "MediaKeys",
                          "Power", "ScreensaverProxy", "Sound"};
    GError *error = NULL;
    g_setenv("XDG_CURRENT_DESKTOP", "GNOME", TRUE);
    directory = g_dir_make_tmp("bsd3-app49-XXXXXX", &error);
    g_assert_no_error(error);
    for (gsize i = 0; i < G_N_ELEMENTS(names); i++) {
        char *id = g_strdup_printf("org.gnome.SettingsDaemon.%s.desktop", names[i]);
        entry(id, "Initialization", TRUE, TRUE);
        entry(id, "DisplayServer", FALSE, FALSE);
        g_free(id);
    }
    entry("org.bsd3.Application.desktop", "Application", TRUE, FALSE);
    entry("org.bsd3.UnsupportedService.desktop", "Initialization", FALSE, FALSE);
    entry("org.gnome.SettingsDaemon.KeyboardExtra.desktop", "Initialization", FALSE, FALSE);
    g_assert_cmpint(g_rmdir(directory), ==, 0);
    g_free(directory);
    g_print("PASS: actual GsmApp accepts/disables exact seven managed initialization services, ordinary app enabled, unsupported phases/near-matches refused\n");
    return 0;
}
