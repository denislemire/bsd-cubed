/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */
/* Original GTK4 lifetime probe; no Epiphany/GTK implementation is copied. */
#include <gtk/gtk.h>
#include <gdk/wayland/gdkwayland.h>
#include <unistd.h>

struct lifetime {
	GtkApplication *application;
	GtkWindow *held;
	guint deadline;
	guint poll;
	unsigned removed;
	unsigned disposed;
	unsigned finalized;
	unsigned stage;
	gboolean removed_before_dispose;
	gboolean empty_on_removal;
	gboolean application_held;
	gboolean done;
	int status;
};

typedef struct {
	GtkApplicationWindow parent;
	struct lifetime *probe;
} ProbeWindow;

typedef struct {
	GtkApplicationWindowClass parent_class;
} ProbeWindowClass;

G_DEFINE_TYPE(ProbeWindow, probe_window, GTK_TYPE_APPLICATION_WINDOW)

static void
probe_window_dispose(GObject *object)
{
	struct lifetime *probe = ((ProbeWindow *)object)->probe;

	if (probe) {
		probe->disposed++;
		g_print("DISPOSE: %u\n", probe->disposed);
	}
	G_OBJECT_CLASS(probe_window_parent_class)->dispose(object);
}

static void
probe_window_finalize(GObject *object)
{
	struct lifetime *probe = ((ProbeWindow *)object)->probe;

	if (probe) {
		probe->finalized++;
		g_print("FINALIZE: %u\n", probe->finalized);
	}
	G_OBJECT_CLASS(probe_window_parent_class)->finalize(object);
}

static void
probe_window_class_init(ProbeWindowClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS(klass);

	object_class->dispose = probe_window_dispose;
	object_class->finalize = probe_window_finalize;
}

static void
probe_window_init(ProbeWindow *window)
{
	window->probe = NULL;
}

static void
finish(struct lifetime *probe, gboolean success, const char *message)
{
	if (probe->done)
		return;
	probe->done = TRUE;
	probe->status = success ? 0 : 1;
	g_print("%s: %s\n", success ? "PASS" : "FAIL", message);
	if (probe->deadline) {
		g_source_remove(probe->deadline);
		probe->deadline = 0;
	}
	if (probe->poll) {
		g_source_remove(probe->poll);
		probe->poll = 0;
	}
	if (probe->held) {
		gtk_window_destroy(probe->held);
		g_clear_object(&probe->held);
	}
	/* This hold represents the asynchronous probe, not an exit shortcut. */
	if (probe->application_held) {
		probe->application_held = FALSE;
		g_application_release(G_APPLICATION(probe->application));
	}
}

static gboolean
expired(gpointer data)
{
	struct lifetime *probe = data;

	probe->deadline = 0;
	finish(probe, FALSE, "30-second window lifecycle deadline exceeded");
	return (G_SOURCE_REMOVE);
}

static void
window_removed(GtkApplication *application, GtkWindow *window, gpointer data)
{
	struct lifetime *probe = data;

	if (window != probe->held)
		return;
	probe->removed++;
	probe->removed_before_dispose = probe->disposed == 0 && probe->finalized == 0;
	probe->empty_on_removal = gtk_application_get_windows(application) == NULL;
	g_print("WINDOW_REMOVED: count=%u, application-empty=%s, dispose=%u, finalize=%u\n",
	    probe->removed, probe->empty_on_removal ? "yes" : "no",
	    probe->disposed, probe->finalized);
}

static gboolean
advance(gpointer data)
{
	struct lifetime *probe = data;

	if (probe->stage == 0) {
		if (!gtk_widget_get_mapped(GTK_WIDGET(probe->held)))
			return (G_SOURCE_CONTINUE);
		gtk_window_destroy(probe->held);
		g_print("AFTER_DESTROY_WITH_HELD_REF: application-empty=%s, dispose=%u, finalize=%u\n",
		    gtk_application_get_windows(probe->application) == NULL ? "yes" : "no",
		    probe->disposed, probe->finalized);
		if (probe->removed != 1 || !probe->removed_before_dispose ||
		    !probe->empty_on_removal || probe->disposed || probe->finalized ||
		    gtk_application_get_windows(probe->application) != NULL) {
			probe->poll = 0;
			finish(probe, FALSE, "logical removal was not independent of disposal/finalization");
			return (G_SOURCE_REMOVE);
		}
		probe->stage = 1;
		g_clear_object(&probe->held);
		g_print("HELD_REFERENCE_RELEASED\n");
	}
	if (probe->disposed && probe->finalized == 1) {
		probe->poll = 0;
		finish(probe, TRUE, "logical GTK window removal precedes disposal/finalization while a reference is held");
		return (G_SOURCE_REMOVE);
	}
	return (G_SOURCE_CONTINUE);
}

static void
activate(GtkApplication *application, gpointer data)
{
	struct lifetime *probe = data;
	ProbeWindow *window;

	if (!GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) {
		g_printerr("FAIL: actual Wayland display required\n");
		return;
	}
	probe->application = application;
	/* Keep the normal application loop alive only until the bounded probe
	 * has seen finalization after releasing its own strong window reference. */
	g_application_hold(G_APPLICATION(application));
	probe->application_held = TRUE;
	g_signal_connect(application, "window-removed", G_CALLBACK(window_removed), probe);
	window = g_object_new(probe_window_get_type(), "application", application, NULL);
	window->probe = probe;
	/* GTK owns the constructor's toplevel reference. Add our explicit one. */
	probe->held = GTK_WINDOW(g_object_ref(window));
	gtk_window_set_title(GTK_WINDOW(window), "BSD3 GTK lifetime probe");
	gtk_window_set_default_size(GTK_WINDOW(window), 480, 200);
	gtk_window_set_child(GTK_WINDOW(window), gtk_label_new("Held reference versus logical window lifetime"));
	probe->deadline = g_timeout_add_seconds(30, expired, probe);
	probe->poll = g_timeout_add(50, advance, probe);
	gtk_window_present(GTK_WINDOW(window));
}

int
main(int argc, char **argv)
{
	g_autoptr(GtkApplication) application = NULL;
	struct lifetime probe = { .status = 1 };
	int status;

	if (argc != 1 || geteuid() == 0) {
		g_printerr("usage (ordinary user on Wayland): native-gtk-window-lifetime\n");
		return (64);
	}
	application = gtk_application_new("org.bsd3.NativeGtkWindowLifetime", G_APPLICATION_NON_UNIQUE);
	g_signal_connect(application, "activate", G_CALLBACK(activate), &probe);
	status = g_application_run(G_APPLICATION(application), argc, argv);
	if (!probe.done)
		g_printerr("FAIL: application exited before lifecycle verification\n");
	return (status ? status : probe.status);
}
