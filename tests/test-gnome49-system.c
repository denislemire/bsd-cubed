/* SPDX-License-Identifier: BSD-2-Clause */
#include "gnome49-system.h"
#include <unistd.h>
#define CK "org.freedesktop.ConsoleKit"
#define ROOT "/org/freedesktop/ConsoleKit"
static GDBusConnection *bus;
static guint denied, accepted;
static void activity_notified(GObject *object,GParamSpec *spec,gpointer data)
{ (void)object;(void)spec;*(gboolean *)data=TRUE; }
static GVariant *control(const char *method, GVariant *args, const char *type)
{
    GError *error = NULL;
    GVariant *r = g_dbus_connection_call_sync(bus, CK, ROOT "/Manager",
        "org.bsd3.Test", method, args, G_VARIANT_TYPE(type), 0, 3000, NULL, &error);
    g_assert_no_error(error); g_assert_nonnull(r); return r;
}
static void mode(const char *name)
{ GVariant *r=control("SetMode",g_variant_new("(s)",name),"()");g_variant_unref(r); }
static void state(GsmSystem *s, gboolean expected)
{
    gint64 end=g_get_monotonic_time()+3000000;
    while (gsm_system_is_active(s)!=expected && g_get_monotonic_time()<end) {
        while (g_main_context_iteration(NULL,FALSE)) {}
        g_usleep(1000);
    }
    g_assert_cmpint(gsm_system_is_active(s),==,expected);
}
static void emit_active(gboolean active)
{ GVariant *r=control("EmitActive",g_variant_new("(b)",active),"()");g_variant_unref(r); }
static void counters(guint expected_idle, gboolean held, const char *what)
{
    guint a,b,c;gboolean h,idle;const char *w;
    GVariant *r=control("GetSystemCounters",NULL,"(uuubbs)");
    g_variant_get(r,"(uuubb&s)",&a,&b,&c,&h,&idle,&w);
    (void)b;(void)c;(void)idle;
    g_assert_cmpuint(a,==,expected_idle);g_assert_cmpint(h,==,held);
    if(what)g_assert_cmpstr(w,==,what);
    g_variant_unref(r);
}
static void rejected(GsmSystem *s, GError *error, gpointer data)
{
    (void)s;(void)data;
    g_assert_error(error,G_IO_ERROR,G_IO_ERROR_NOT_SUPPORTED);
    denied++;
}
static void succeeded(GsmSystem *s, GError *error, gpointer data)
{
    (void)s;(void)data;
    g_assert_null(error);
    accepted++;
}
static void test_native_system(void)
{
    GError *error=NULL;GsmSystem *s;GVariant *r;guint32 pid;
    mode("system");
    s=gsm_bsd3_system_new_for_connection(bus,getuid()+1,&error);
    g_assert_null(s);g_assert_error(error,G_IO_ERROR,G_IO_ERROR_PERMISSION_DENIED);
    g_clear_error(&error);
    s=gsm_bsd3_system_new(&error);
    g_assert_null(s);g_assert_error(error,G_IO_ERROR,G_IO_ERROR_PERMISSION_DENIED);
    g_clear_error(&error);
    s=gsm_bsd3_system_new_for_connection(bus,getuid(),&error);
    g_assert_no_error(error);g_assert_nonnull(s);state(s,TRUE);
    r=control("GetLastPid",NULL,"(u)");g_variant_get(r,"(u)",&pid);
    g_variant_unref(r);g_assert_cmpuint(pid,==,(guint32)getpid());
    g_assert_true(gsm_bsd3_system_idle_checked(s,TRUE,&error));g_assert_no_error(error);
    counters(1,FALSE,NULL);
    g_assert_true(gsm_bsd3_system_inhibit_checked(s,GSM_INHIBITOR_FLAG_SUSPEND,&error));
    g_assert_no_error(error);counters(1,TRUE,"sleep");
    g_assert_true(gsm_bsd3_system_inhibit_checked(s,GSM_INHIBITOR_FLAG_IDLE,&error));
    g_assert_no_error(error);counters(1,TRUE,"idle");
    mode("system-extra-fd");
    g_assert_false(gsm_bsd3_system_inhibit_checked(s,GSM_INHIBITOR_FLAG_SUSPEND,&error));
    g_assert_error(error,G_IO_ERROR,G_IO_ERROR_INVALID_DATA);g_clear_error(&error);
    counters(1,FALSE,NULL);
    mode("system");
    g_assert_true(gsm_bsd3_system_inhibit_checked(s,GSM_INHIBITOR_FLAG_IDLE,&error));
    g_assert_no_error(error);
    emit_active(FALSE);state(s,FALSE);emit_active(TRUE);state(s,TRUE);
    g_assert_false(gsm_system_can_stop(s));g_assert_false(gsm_system_can_restart(s));
    g_assert_true(gsm_system_can_suspend(s));g_assert_true(gsm_system_can_hibernate(s));
    g_signal_connect(s,"request-completed",G_CALLBACK(succeeded),NULL);
    gsm_system_suspend(s);gsm_system_hibernate(s);
    g_assert_cmpuint(accepted,==,2);
    g_signal_handlers_disconnect_by_func(s,succeeded,NULL);
    g_signal_connect(s,"request-completed",G_CALLBACK(rejected),NULL);
    gsm_system_attempt_stop(s);gsm_system_attempt_restart(s);
    g_assert_cmpuint(denied,==,2);
    mode("restart");state(s,FALSE);
    g_assert_false(gsm_bsd3_system_idle_checked(s,FALSE,&error));
    g_assert_error(error,G_IO_ERROR,G_IO_ERROR_NOT_CONNECTED);g_clear_error(&error);
    counters(1,FALSE,NULL);
    g_object_unref(s);
    const char *bad[]={"uid-mismatch","system-bad-class","system-tty"};
    for(guint i=0;i<G_N_ELEMENTS(bad);i++){
        mode(bad[i]);s=gsm_bsd3_system_new_for_connection(bus,getuid(),&error);
        g_assert_null(s);g_assert_nonnull(error);g_clear_error(&error);
    }
    mode("system");emit_active(TRUE);
    s=gsm_bsd3_system_new_for_connection(bus,getuid(),&error);
    g_assert_no_error(error);g_assert_nonnull(s);state(s,TRUE);
    g_assert_true(gsm_bsd3_system_inhibit_checked(s,GSM_INHIBITOR_FLAG_LOGOUT,&error));
    g_assert_no_error(error);counters(1,TRUE,"shutdown");
    r=control("EmitSystemRemoved",NULL,"()");g_variant_unref(r);state(s,FALSE);
    g_assert_false(gsm_bsd3_system_idle_checked(s,FALSE,&error));
    g_assert_error(error,G_IO_ERROR,G_IO_ERROR_NOT_CONNECTED);g_clear_error(&error);
    counters(1,FALSE,NULL);g_object_unref(s);
    /* Closing the transport does not generate NameOwnerChanged. Check the
     * same-flags fast path before dispatching its queued closed notification. */
    GDBusConnection *other=g_dbus_connection_new_for_address_sync(
        g_getenv("DBUS_SYSTEM_BUS_ADDRESS"),
        G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
        G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,NULL,NULL,&error);
    g_assert_no_error(error);g_assert_nonnull(other);
    s=gsm_bsd3_system_new_for_connection(other,getuid(),&error);
    g_assert_no_error(error);g_assert_nonnull(s);state(s,TRUE);
    g_assert_true(gsm_bsd3_system_inhibit_checked(s,GSM_INHIBITOR_FLAG_IDLE,&error));
    g_assert_no_error(error);counters(1,TRUE,"idle");
    g_assert_true(g_dbus_connection_close_sync(other,NULL,&error));g_assert_no_error(error);
    g_assert_false(gsm_bsd3_system_inhibit_checked(s,GSM_INHIBITOR_FLAG_IDLE,&error));
    g_assert_error(error,G_IO_ERROR,G_IO_ERROR_NOT_CONNECTED);g_clear_error(&error);
    state(s,FALSE);counters(1,FALSE,NULL);g_object_unref(s);g_object_unref(other);
    /* Independently prove callback-driven cleanup without calling a checked
     * operation that could discover the closed connection itself. */
    other=g_dbus_connection_new_for_address_sync(g_getenv("DBUS_SYSTEM_BUS_ADDRESS"),
        G_DBUS_CONNECTION_FLAGS_AUTHENTICATION_CLIENT |
        G_DBUS_CONNECTION_FLAGS_MESSAGE_BUS_CONNECTION,NULL,NULL,&error);
    g_assert_no_error(error);g_assert_nonnull(other);
    s=gsm_bsd3_system_new_for_connection(other,getuid(),&error);
    g_assert_no_error(error);g_assert_nonnull(s);state(s,TRUE);
    gboolean notified=FALSE;
    g_signal_connect(s,"notify::active",G_CALLBACK(activity_notified),&notified);
    g_assert_true(gsm_bsd3_system_inhibit_checked(s,GSM_INHIBITOR_FLAG_IDLE,&error));
    g_assert_no_error(error);counters(1,TRUE,"idle");
    g_assert_true(g_dbus_connection_close_sync(other,NULL,&error));g_assert_no_error(error);
    gint64 end=g_get_monotonic_time()+3000000;
    while(!notified&&g_get_monotonic_time()<end){
        while(g_main_context_iteration(NULL,FALSE)){}
        g_usleep(1000);
    }
    g_assert_true(notified);state(s,FALSE);counters(1,FALSE,NULL);
    g_object_unref(s);g_object_unref(other);
}
int main(int argc,char **argv)
{
    GError *error=NULL;
    g_test_init(&argc,&argv,NULL);
    bus=g_bus_get_sync(G_BUS_TYPE_SYSTEM,NULL,&error);g_assert_no_error(error);
    g_test_add_func("/bsd3/gnome49-native-system",test_native_system);
    int result=g_test_run();g_object_unref(bus);return result;
}
