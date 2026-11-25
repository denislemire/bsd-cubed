/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "config.h"
#include <pwd.h>
#include "gdm-common.h"
#include "gdm-dynamic-user-store.h"

/* A single local greeter holds a lease on the ports-owned account. */
struct _GdmDynamicUserStore { GObject parent; uid_t active_uid; };
G_DEFINE_TYPE (GdmDynamicUserStore, gdm_dynamic_user_store, G_TYPE_OBJECT)
static void gdm_dynamic_user_store_class_init (GdmDynamicUserStoreClass *klass) {}
static void gdm_dynamic_user_store_init (GdmDynamicUserStore *self) {}
GdmDynamicUserStore *gdm_dynamic_user_store_new (void)
{
        return g_object_new (GDM_TYPE_DYNAMIC_USER_STORE, NULL);
}
gboolean
gdm_dynamic_user_store_create (GdmDynamicUserStore *store, const char *preferred,
                               const char *display_name, const char *member_of,
                               char **name, uid_t *uid, char **home, GError **error)
{
        struct passwd *pw;
        if (store->active_uid || g_strcmp0 (preferred, "gdm-greeter") != 0) {
                g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_NOT_SUPPORTED,
                                     "Only one local GDM greeter is supported");
                return FALSE;
        }
        if (!gdm_get_pwent_for_name ("gdm", &pw) || pw->pw_uid == 0 ||
            pw->pw_gid == 0 || !g_path_is_absolute (pw->pw_dir) || !strcmp (pw->pw_dir, "/")) {
                g_set_error_literal (error, G_IO_ERROR, G_IO_ERROR_FAILED,
                                     "The unprivileged gdm account is unavailable");
                return FALSE;
        }
        *name = g_strdup (pw->pw_name);
        *home = g_strdup (pw->pw_dir);
        *uid = store->active_uid = pw->pw_uid;
        return TRUE;
}
void gdm_dynamic_user_store_remove (GdmDynamicUserStore *store, uid_t uid)
{
        g_return_if_fail (uid != 0 && uid == store->active_uid);
        store->active_uid = 0;
}
