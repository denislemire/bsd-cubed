/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */
/* Bounded test-only ConsoleKit idle/sleep inhibitor. Never changes power policy. */
#include <gio/gio.h>
#include <gio/gunixfdlist.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
static volatile sig_atomic_t stopped;
static void stop(int sig) { (void)sig; stopped=1; }
int main(int argc, char **argv)
{
 char *end;unsigned long seconds;
 GDBusConnection *bus;GUnixFDList *fds=NULL;GVariant *reply;gint handle;
 if(argc!=2)return 64;
 errno=0;seconds=strtoul(argv[1],&end,10);
 if(errno || !*argv[1] || *end || seconds<1 || seconds>7200)return 64;
 if(geteuid()==0){g_unsetenv("DBUS_SYSTEM_BUS_ADDRESS");g_unsetenv("DBUS_SESSION_BUS_ADDRESS");}
 bus=g_bus_get_sync(G_BUS_TYPE_SYSTEM,NULL,NULL);
 if(!bus)return 1;
 reply=g_dbus_connection_call_with_unix_fd_list_sync(bus,"org.freedesktop.ConsoleKit",
  "/org/freedesktop/ConsoleKit/Manager","org.freedesktop.ConsoleKit.Manager","Inhibit",
  g_variant_new("(ssss)","idle:sleep","BSD3 VM tests","Bounded GDM integration","block"),
  G_VARIANT_TYPE("(h)"),G_DBUS_CALL_FLAGS_NONE,3000,NULL,&fds,NULL,NULL);
 if(!reply || !fds || g_unix_fd_list_get_length(fds)!=1)return 1;
 g_variant_get(reply,"(h)",&handle);
 if(handle!=0)return 1;
 int fd=g_unix_fd_list_get(fds,handle,NULL);
 if(fd<0)return 1;
 g_variant_unref(reply);g_object_unref(fds);
 signal(SIGTERM,stop);signal(SIGINT,stop);signal(SIGHUP,stop);
 puts("BOUNDED_INHIBITOR_READY");fflush(stdout);
 for(unsigned long i=0;i<seconds && !stopped;i++)sleep(1);
 close(fd);g_object_unref(bus);return 0;
}
