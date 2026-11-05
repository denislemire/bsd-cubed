/* SPDX-License-Identifier: BSD-2-Clause */
#include <gtk/gtk.h>
#include <gdk/gdkwayland.h>

static gboolean
frame(GtkWidget *widget, GdkFrameClock *clock, gpointer data)
{
	unsigned *frames = data;
	(void)clock;
	if (!gtk_widget_get_mapped(widget))
		return (G_SOURCE_CONTINUE);
	if (++*frames == 5) {
		g_print("PASS: mapped GTK window on Wayland received five frame ticks\n");
		gtk_main_quit();
		return (G_SOURCE_REMOVE);
	}
	return (G_SOURCE_CONTINUE);
}

int
main(int argc, char **argv)
{
	GtkWidget *window, *label;
	unsigned frames = 0;
	gtk_init(&argc, &argv);
	g_assert_true(GDK_IS_WAYLAND_DISPLAY(gdk_display_get_default()));
	window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(window), "BSD3 native Wayland acceptance");
	gtk_window_set_default_size(GTK_WINDOW(window), 480, 240);
	label = gtk_label_new("BSD3: real Wayland application window");
	gtk_container_add(GTK_CONTAINER(window), label);
	gtk_widget_add_tick_callback(window, frame, &frames, NULL);
	gtk_widget_show_all(window);
	gtk_main();
	gtk_widget_destroy(window);
	return (frames >= 5 ? 0 : 1);
}
