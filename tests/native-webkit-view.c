/* SPDX-License-Identifier: BSD-2-Clause */
/* Copyright (c) 2026 Denis Lemire */
/* Original local-content GTK4/WebKit6 probe. No external network requests. */
#include <gtk/gtk.h>
#include <gdk/wayland/gdkwayland.h>
#include <webkit/webkit.h>
#include <glib/gstdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

struct probe {
	GtkWindow *window;
	WebKitWebView *view;
	GCancellable *cancel;
	char *output;
	char *title;
	char *token;
	guint timeout;
	guint tick;
	unsigned frames;
	gboolean evaluating;
	gboolean done;
	int status;
};

static void
finish(struct probe *probe, gboolean success, const char *message)
{
	GtkWindow *window;

	if (probe->done)
		return;
	probe->done = TRUE;
	probe->status = success ? 0 : 1;
	g_print("%s: %s\n", success ? "PASS" : "FAIL", message);
	if (probe->timeout) {
		g_source_remove(probe->timeout);
		probe->timeout = 0;
	}
	if (probe->tick && probe->view) {
		gtk_widget_remove_tick_callback(GTK_WIDGET(probe->view), probe->tick);
		probe->tick = 0;
	}
	g_cancellable_cancel(probe->cancel);
	window = probe->window;
	probe->window = NULL;
	probe->view = NULL;
	/* Removing the application's final window ends its normal lifecycle. */
	if (window)
		gtk_window_destroy(window);
}

static gboolean
expired(gpointer data)
{
	struct probe *probe = data;

	probe->timeout = 0;
	finish(probe, FALSE, "30-second DOM/rendering deadline exceeded");
	return (G_SOURCE_REMOVE);
}

static void
snapshot_ready(GObject *object, GAsyncResult *result, gpointer data)
{
	struct probe *probe = data;
	g_autoptr(GError) error = NULL;
	g_autoptr(GdkTexture) texture = NULL;
	g_autofree guchar *pixels = NULL;
	gsize stride, total, changed = 0, top, bottom;
	int width, height;

	texture = webkit_web_view_get_snapshot_finish(WEBKIT_WEB_VIEW(object),
	    result, &error);
	if (probe->done)
		return;
	if (!texture) {
		g_printerr("SNAPSHOT_ERROR: %s\n", error ? error->message : "no texture");
		finish(probe, FALSE, "WebKit visible snapshot failed");
		return;
	}
	width = gdk_texture_get_width(texture);
	height = gdk_texture_get_height(texture);
	if (width < 8 || height < 8 || width > 4096 || height > 4096) {
		finish(probe, FALSE, "unexpected snapshot dimensions");
		return;
	}
	stride = (gsize)width * 4;
	total = (gsize)width * height;
	pixels = g_try_malloc(stride * (gsize)height);
	if (!pixels) {
		finish(probe, FALSE, "snapshot allocation failed");
		return;
	}
	/* Downloaded pixels are native-endian Cairo ARGB32. Compare complete
	 * pixels rather than assuming channel byte order or a particular GPU. */
	gdk_texture_download(texture, pixels, stride);
	for (gsize i = 0; i < total; i++) {
		if (memcmp(pixels, pixels + i * 4, 4) != 0)
			changed++;
	}
	top = (gsize)(height / 4) * stride + (gsize)(width / 2) * 4;
	bottom = (gsize)(height * 3 / 4) * stride + (gsize)(width / 2) * 4;
	g_print("SNAPSHOT: %dx%d, nonuniform=%" G_GSIZE_FORMAT
	    "/%" G_GSIZE_FORMAT ", bands-differ=%s\n", width, height,
	    changed, total, memcmp(pixels + top, pixels + bottom, 4) ? "yes" : "no");
	/* A text glyph or cursor alone cannot satisfy two substantial CSS bands. */
	if (changed <= total / 4 || memcmp(pixels + top, pixels + bottom, 4) == 0) {
		finish(probe, FALSE, "visible pixels do not show the two CSS bands");
		return;
	}
	if (!gdk_texture_save_to_png(texture, probe->output) ||
	    g_chmod(probe->output, S_IRUSR | S_IWUSR) != 0) {
		finish(probe, FALSE, "saving private snapshot PNG failed");
		return;
	}
	g_print("SNAPSHOT_PNG: %s\n", probe->output);
	finish(probe, TRUE, "local DOM token and nonuniform visible WebKit rendering verified");
}

static gboolean
painted(GtkWidget *widget, GdkFrameClock *clock, gpointer data)
{
	struct probe *probe = data;

	(void)clock;
	if (!gtk_widget_get_mapped(widget))
		return (G_SOURCE_CONTINUE);
	if (++probe->frames < 3)
		return (G_SOURCE_CONTINUE);
	probe->tick = 0;
	webkit_web_view_get_snapshot(probe->view, WEBKIT_SNAPSHOT_REGION_VISIBLE,
	    WEBKIT_SNAPSHOT_OPTIONS_NONE, probe->cancel, snapshot_ready, probe);
	return (G_SOURCE_REMOVE);
}

static void
dom_ready(GObject *object, GAsyncResult *result, gpointer data)
{
	struct probe *probe = data;
	g_autoptr(GError) error = NULL;
	g_autoptr(JSCValue) value = NULL;
	g_autofree char *text = NULL;
	g_autofree char *expected = NULL;

	value = webkit_web_view_evaluate_javascript_finish(WEBKIT_WEB_VIEW(object),
	    result, &error);
	if (probe->done)
		return;
	if (!value || !jsc_value_is_string(value)) {
		g_printerr("DOM_ERROR: %s\n", error ? error->message : "not a string");
		finish(probe, FALSE, "JavaScript DOM evaluation failed");
		return;
	}
	text = jsc_value_to_string(value);
	expected = g_strdup_printf("%s\n%s", probe->title, probe->token);
	if (g_strcmp0(text, expected) != 0) {
		finish(probe, FALSE, "DOM title/body token differs from this run's inline document");
		return;
	}
	g_print("DOM_TITLE: %s\nDOM_BODY_TOKEN: %s\n", probe->title, probe->token);
	probe->tick = gtk_widget_add_tick_callback(GTK_WIDGET(probe->view),
	    painted, probe, NULL);
}

static void
loaded(WebKitWebView *view, WebKitLoadEvent event, gpointer data)
{
	struct probe *probe = data;

	g_print("LOAD_EVENT: %d\n", (int)event);
	if (probe->done || probe->evaluating || event != WEBKIT_LOAD_FINISHED)
		return;
	probe->evaluating = TRUE;
	webkit_web_view_evaluate_javascript(view,
	    "document.title + '\\n' + document.body.textContent.trim()", -1,
	    NULL, NULL, probe->cancel, dom_ready, probe);
}

static gboolean
load_failed(WebKitWebView *view, WebKitLoadEvent event, const char *uri,
    GError *error, gpointer data)
{
	(void)view;
	(void)event;
	(void)uri;
	if (((struct probe *)data)->done)
		return (TRUE);
	g_printerr("LOAD_ERROR: %s\n", error->message);
	finish(data, FALSE, "inline document load failed");
	return (TRUE);
}

static void
process_terminated(WebKitWebView *view, WebKitWebProcessTerminationReason reason,
    gpointer data)
{
	(void)view;
	if (((struct probe *)data)->done)
		return;
	g_printerr("WEB_PROCESS_TERMINATED: %d\n", (int)reason);
	finish(data, FALSE, "WebKit web process terminated");
}

static void
activate(GtkApplication *application, gpointer data)
{
	struct probe *probe = data;
	g_autofree char *html = NULL;
	const char *mode = g_getenv("WEBKIT_DISABLE_COMPOSITING_MODE");

	if (!GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default())) {
		g_printerr("FAIL: actual Wayland GDK display required\n");
		return;
	}
	g_print("WEBKIT_DISABLE_COMPOSITING_MODE: %s\n", mode ? mode : "unset");
	probe->timeout = g_timeout_add_seconds(30, expired, probe);
	probe->window = GTK_WINDOW(gtk_application_window_new(application));
	gtk_window_set_title(probe->window, "BSD3 local WebKit rendering probe");
	gtk_window_set_default_size(probe->window, 800, 600);
	probe->view = WEBKIT_WEB_VIEW(webkit_web_view_new());
	gtk_window_set_child(probe->window, GTK_WIDGET(probe->view));
	g_signal_connect(probe->view, "load-changed", G_CALLBACK(loaded), probe);
	g_signal_connect(probe->view, "load-failed", G_CALLBACK(load_failed), probe);
	g_signal_connect(probe->view, "web-process-terminated",
	    G_CALLBACK(process_terminated), probe);
	html = g_strdup_printf("<!doctype html><html><head><meta charset='utf-8'>"
	    "<title>%s</title><style>html,body{margin:0;width:100%%;height:100%%;overflow:hidden}"
	    "#upper,#lower{width:100%%;height:50%%}"
	    "#upper{background:#d02020;color:white;font:20px sans-serif}"
	    "#lower{background:#2050d0}</style></head><body>"
	    "<div id='upper'>%s</div><div id='lower'></div></body></html>",
	    probe->title, probe->token);
	gtk_window_present(probe->window);
	webkit_web_view_load_html(probe->view, html, "about:blank");
}

int
main(int argc, char **argv)
{
	g_autoptr(GtkApplication) application = NULL;
	g_autofree char *uuid = NULL;
	struct probe probe = { .status = 1 };
	int status;
	char *application_argv[] = { argv[0], NULL };

	if (argc != 2 || geteuid() == 0) {
		g_printerr("usage (ordinary user): native-webkit-view PRIVATE_OUTPUT.png\n");
		return (64);
	}
	umask(077);
	probe.output = g_canonicalize_filename(argv[1], NULL);
	uuid = g_uuid_string_random();
	probe.title = g_strdup_printf("BSD3-WebKit-title-%s", uuid);
	probe.token = g_strdup_printf("BSD3-WebKit-body-%s", uuid);
	probe.cancel = g_cancellable_new();
	application = gtk_application_new("org.bsd3.NativeWebKitView", G_APPLICATION_NON_UNIQUE);
	g_signal_connect(application, "activate", G_CALLBACK(activate), &probe);
	status = g_application_run(G_APPLICATION(application), 1, application_argv);
	if (!probe.done)
		g_printerr("FAIL: application closed before DOM/snapshot completion\n");
	if (probe.timeout)
		g_source_remove(probe.timeout);
	g_clear_object(&probe.cancel);
	g_free(probe.output);
	g_free(probe.title);
	g_free(probe.token);
	return (status ? status : probe.status);
}
