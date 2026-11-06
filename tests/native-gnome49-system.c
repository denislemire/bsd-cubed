/* SPDX-License-Identifier: BSD-2-Clause */
/* Run only inside the temporary real CK session created by native-session. */
#include "gnome49-system.h"
#include <unistd.h>
#include <string.h>
#define CK "org.freedesktop.ConsoleKit"
#define ROOT "/org/freedesktop/ConsoleKit/Manager"
static GVariant *call(GDBusConnection *b,const char *path,const char *iface,
    const char *method,GVariant *args,const char *type)
{
    GError *error=NULL;
    GVariant *r=g_dbus_connection_call_sync(b,CK,path,iface,method,args,
        G_VARIANT_TYPE(type),0,3000,NULL,&error);
    g_assert_no_error(error);g_assert_nonnull(r);return r;
}
static void held(GDBusConnection *b,gboolean expected)
{
    gint64 end=g_get_monotonic_time()+3000000;
    gboolean found=FALSE;
    do {
        GError *error=NULL;GVariantIter *entries=NULL;
        GVariant *r=g_dbus_connection_call_sync(b,CK,ROOT,CK ".Manager",
            "ListInhibitors",NULL,G_VARIANT_TYPE("(a(ssssuu))"),0,3000,NULL,&error);
        found=FALSE;
        if(r){
            const char *what,*who,*why,*mode;guint32 uid,pid;
            g_variant_get(r,"(a(ssssuu))",&entries);
            while(g_variant_iter_loop(entries,"(&s&s&s&suu)",&what,&who,&why,&mode,&uid,&pid)){
                (void)what;(void)why;(void)mode;
                if(uid==getuid()&&pid==(guint32)getpid()&&!strcmp(who,"GNOME session"))found=TRUE;
            }
            g_variant_iter_free(entries);g_variant_unref(r);
        }else{
            char *remote=g_dbus_error_get_remote_error(error);
            g_assert_nonnull(remote);g_assert_true(g_str_has_suffix(remote,"NothingInhibited"));
            g_free(remote);g_error_free(error);
        }
        if(found==expected)break;
        g_usleep(25000);
    }while(g_get_monotonic_time()<end);
    g_assert_cmpint(found,==,expected);
}
int main(void)
{
    GError *error=NULL;GDBusConnection *b;GsmSystem *s;
    GVariant *r;char *path;gboolean before,after,active;
    g_assert_cmpuint(getuid(),>,0);
    g_assert_nonnull(g_getenv("XDG_SESSION_COOKIE"));
    b=g_bus_get_sync(G_BUS_TYPE_SYSTEM,NULL,&error);g_assert_no_error(error);
    s=gsm_bsd3_system_new(&error);g_assert_no_error(error);g_assert_nonnull(s);
    r=call(b,ROOT,CK ".Manager","GetSessionForUnixProcess",
        g_variant_new("(u)",(guint32)getpid()),"(o)");
    g_variant_get(r,"(o)",&path);g_variant_unref(r);
    r=call(b,path,CK ".Session","IsActive",NULL,"(b)");
    g_variant_get(r,"(b)",&active);g_variant_unref(r);
    g_assert_cmpint(gsm_system_is_active(s),==,active);
    r=call(b,path,CK ".Session","GetIdleHint",NULL,"(b)");
    g_variant_get(r,"(b)",&before);g_variant_unref(r);
    g_assert_true(gsm_bsd3_system_idle_checked(s,!before,&error));g_assert_no_error(error);
    r=call(b,path,CK ".Session","GetIdleHint",NULL,"(b)");
    g_variant_get(r,"(b)",&after);g_variant_unref(r);g_assert_cmpint(after,==,!before);
    g_assert_true(gsm_bsd3_system_idle_checked(s,before,&error));g_assert_no_error(error);
    g_assert_false(gsm_system_can_stop(s));g_assert_false(gsm_system_can_restart(s));
    g_assert_true(gsm_bsd3_system_inhibit_checked(s,GSM_INHIBITOR_FLAG_SUSPEND,&error));
    g_assert_no_error(error);held(b,TRUE);
    g_assert_true(gsm_bsd3_system_inhibit_checked(s,GSM_INHIBITOR_FLAG_IDLE,&error));
    g_assert_no_error(error);held(b,TRUE);
    g_assert_true(gsm_bsd3_system_inhibit_checked(s,0,&error));
    g_assert_no_error(error);held(b,FALSE);
    g_assert_true(gsm_bsd3_system_inhibit_checked(s,GSM_INHIBITOR_FLAG_IDLE,&error));
    g_assert_no_error(error);held(b,TRUE);
    g_object_unref(s);held(b,FALSE);
    g_free(path);g_object_unref(b);
    g_print("PASS: real CK current-process binding/activity, reversible idle, inhibitor replacement/release/finalization; privileged power unsupported\n");
    return 0;
}
