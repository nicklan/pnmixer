/* ui-tray-icon.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file ui-tray-icon.c
 * This file holds the ui-related code for the system tray icon.
 * @brief Tray icon subsystem.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "audio.h"
#include "prefs.h"
#include "support-intl.h"
#include "support-log.h"
#include "support-ui.h"
#include "ui-tray-icon.h"

#include "main.h"

#define ICON_MIN_SIZE 16

enum {
	VOLUME_MUTED,
	VOLUME_OFF,
	VOLUME_LOW,
	VOLUME_MEDIUM,
	VOLUME_HIGH,
	N_VOLUME_PIXBUFS
};

/*
 * Pixbuf handling
 */

/**
 * This is an internally used function to create GdkPixbufs.
 *
 * @param filename filename to create the GtkPixbuf from
 * @return the new GdkPixbuf, NULL on failure
 */
static GdkPixbuf *
pixbuf_new_from_file(const gchar *filename)
{
	GError *error = NULL;
	GdkPixbuf *pixbuf;
	gchar *pathname = NULL;

	if (!filename || !filename[0])
		return NULL;

	pathname = get_pixmap_file(filename);

	if (!pathname) {
		WARN("Couldn't find pixmap file '%s'", filename);
		return NULL;
	}

	DEBUG("Loading PNMixer icon '%s' from '%s'", filename, pathname);

	pixbuf = gdk_pixbuf_new_from_file(pathname, &error);
	if (!pixbuf) {
		WARN("Could not create pixbuf from file '%s': %s",
		     pathname, error->message);
		g_error_free(error);
	}

	g_free(pathname);
	return pixbuf;
}

/**
 * Looks up icons based on the currently selected theme.
 *
 * @param icon_name icon name to look up
 * @param size size of the icon
 * @return the corresponding theme icon, NULL on failure,
 * use g_object_unref() to release the reference to the icon
 */
static GdkPixbuf *
pixbuf_new_from_stock(const gchar *icon_name, gint size)
{
	static GtkIconTheme *icon_theme = NULL;
	GError *err = NULL;
	GtkIconInfo *info = NULL;
	GdkPixbuf *pixbuf = NULL;

	if (icon_theme == NULL)
		icon_theme = gtk_icon_theme_get_default();

	info = gtk_icon_theme_lookup_icon(icon_theme, icon_name, size, 0);
	if (info == NULL) {
		WARN("Unable to lookup icon '%s'", icon_name);
		return NULL;
	}

	DEBUG("Loading stock icon '%s' from '%s'", icon_name,
	      gtk_icon_info_get_filename(info));

	pixbuf = gtk_icon_info_load_icon(info, &err);
	if (pixbuf == NULL) {
		WARN("Unable to load icon '%s': %s", icon_name, err->message);
		g_error_free(err);
	}

	g_object_unref(info);

	return pixbuf;
}

/* Frees a pixbuf array. */
static void
pixbuf_array_free(GdkPixbuf **pixbufs)
{
	gsize i;

	if (!pixbufs)
		return;

	for (i = 0; i < N_VOLUME_PIXBUFS; i++)
		g_object_unref(pixbufs[i]);

	g_free(pixbufs);
}

/* Creates a new pixbuf array, containing the icon set that must be used. */
static GdkPixbuf **
pixbuf_array_new(int size)
{
	GdkPixbuf *pixbufs[N_VOLUME_PIXBUFS];
	gboolean system_theme;

	DEBUG("Building pixbuf array for size %d", size);

	system_theme = prefs_get_boolean("SystemTheme", FALSE);

	if (system_theme) {
		pixbufs[VOLUME_MUTED] = pixbuf_new_from_stock("audio-volume-muted", size);
		pixbufs[VOLUME_OFF] = pixbuf_new_from_stock("audio-volume-off", size);
		pixbufs[VOLUME_LOW] = pixbuf_new_from_stock("audio-volume-low", size);
		pixbufs[VOLUME_MEDIUM] = pixbuf_new_from_stock("audio-volume-medium", size);
		pixbufs[VOLUME_HIGH] = pixbuf_new_from_stock("audio-volume-high", size);
		/* 'audio-volume-off' is not available in every icon set.
		 * Check freedesktop standard for more info:
		 *   http://standards.freedesktop.org/icon-naming-spec/
		 *   icon-naming-spec-latest.html
		 */
		if (pixbufs[VOLUME_OFF] == NULL)
			pixbufs[VOLUME_OFF] = pixbuf_new_from_stock("audio-volume-low", size);
	} else {
		pixbufs[VOLUME_MUTED] = pixbuf_new_from_file("pnmixer-muted.png");
		pixbufs[VOLUME_OFF] = pixbuf_new_from_file("pnmixer-off.png");
		pixbufs[VOLUME_LOW] = pixbuf_new_from_file("pnmixer-low.png");
		pixbufs[VOLUME_MEDIUM] = pixbuf_new_from_file("pnmixer-medium.png");
		pixbufs[VOLUME_HIGH] = pixbuf_new_from_file("pnmixer-high.png");
	}

	return g_memdup(pixbufs, sizeof pixbufs);
}

/* Tray icon volume meter */

struct vol_meter {
	/* Configuration */
	guchar red;
	guchar green;
	guchar blue;
	gint x_offset_pct;
	gint y_offset_pct;
	/* Dynamic stuff */
	GdkPixbuf *pixbuf;
	gint width;
	guchar *row;
};

typedef struct vol_meter VolMeter;

/* Frees a VolMeter instance. */
static void
vol_meter_free(VolMeter *vol_meter)
{
	if (!vol_meter)
		return;

	if (vol_meter->pixbuf)
		g_object_unref(vol_meter->pixbuf);

	g_free(vol_meter->row);
	g_free(vol_meter);
}

/* Returns a new VolMeter instance. Returns NULL if VolMeter is disabled. */
static VolMeter *
vol_meter_new(void)
{
	VolMeter *vol_meter;
	gboolean vol_meter_enabled;
	gdouble *vol_meter_clrs;

	vol_meter_enabled = prefs_get_boolean("DrawVolMeter", FALSE);
	if (vol_meter_enabled == FALSE)
		return NULL;

	vol_meter = g_new0(VolMeter, 1);

	vol_meter->x_offset_pct = prefs_get_integer("VolMeterPos", 0);
	vol_meter->y_offset_pct = 10;

	vol_meter_clrs = prefs_get_double_list("VolMeterColor", NULL);
	vol_meter->red = vol_meter_clrs[0] * 255;
	vol_meter->green = vol_meter_clrs[1] * 255;
	vol_meter->blue = vol_meter_clrs[2] * 255;
	g_free(vol_meter_clrs);

	return vol_meter;
}

/* Draws the volume meter on top of the icon. It doesn't modify the pixbuf passed
 * in parameter. Instead, it makes a copy internally, and return a pointer toward
 * the modified copy. There's no need to unref it.
 */
static GdkPixbuf *
vol_meter_draw(VolMeter *vol_meter, GdkPixbuf *pixbuf, int volume)
{
	int icon_width, icon_height;
	int vm_width, vm_height;
	int x, y;
	int rowstride, i;
	guchar *pixels;

	/* Ensure the pixbuf is as expected */
	g_assert(gdk_pixbuf_get_colorspace(pixbuf) == GDK_COLORSPACE_RGB);
	g_assert(gdk_pixbuf_get_bits_per_sample(pixbuf) == 8);
	g_assert(gdk_pixbuf_get_has_alpha(pixbuf));
	g_assert(gdk_pixbuf_get_n_channels(pixbuf) == 4);

	icon_width = gdk_pixbuf_get_width(pixbuf);
	icon_height = gdk_pixbuf_get_height(pixbuf);

	/* Cache the pixbuf passed in parameter */
	if (vol_meter->pixbuf)
		g_object_unref(vol_meter->pixbuf);
	vol_meter->pixbuf = pixbuf = gdk_pixbuf_copy(pixbuf);

	/* Volume meter coordinates */
	vm_width = icon_width / 6;
	x = vol_meter->x_offset_pct * (icon_width - vm_width) / 100;
	g_assert(x >= 0 && x + vm_width <= icon_width);

	y = vol_meter->y_offset_pct * icon_height / 100;
	vm_height = (icon_height - (y * 2)) * (volume / 100.0);
	g_assert(y >= 0 && y + vm_height <= icon_height);

	/* Let's check if the icon width changed, in which case we
	 * must reinit our internal row of pixels.
	 */
	if (vm_width != vol_meter->width) {
		vol_meter->width = vm_width;
		g_free(vol_meter->row);
		vol_meter->row = NULL;
	}

	if (vol_meter->row == NULL) {
		DEBUG("Allocating vol meter row (width %d)", vm_width);
		vol_meter->row = g_malloc(vm_width * sizeof(guchar) * 4);
		for (i = 0; i < vm_width; i++) {
			vol_meter->row[i * 4 + 0] = vol_meter->red;
			vol_meter->row[i * 4 + 1] = vol_meter->green;
			vol_meter->row[i * 4 + 2] = vol_meter->blue;
			vol_meter->row[i * 4 + 3] = 255;
		}
	}

	/* Draw the volume meter.
	 * Rows in the image are stored top to bottom.
	 */
	y = icon_height - y;
	rowstride = gdk_pixbuf_get_rowstride(pixbuf);
	pixels = gdk_pixbuf_get_pixels(pixbuf);

	for (i = 0; i < vm_height; i++) {
		guchar *p;
		guint row_offset, col_offset;

		row_offset = y - i;
		col_offset = x * 4;
		p = pixels + (row_offset * rowstride) + col_offset;

		memcpy(p, vol_meter->row, vm_width * 4);
	}

	return pixbuf;
}

/* Helpers */

/* Update the tray icon pixbuf according to the current audio state. */
static void
update_status_icon_pixbuf(GtkStatusIcon *status_icon,
                          GdkPixbuf **pixbufs, VolMeter *vol_meter,
                          gdouble volume, gboolean muted)
{
	GdkPixbuf *pixbuf;

	if (!muted) {
		if (volume == 0)
			pixbuf = pixbufs[VOLUME_OFF];
		else if (volume < 33)
			pixbuf = pixbufs[VOLUME_LOW];
		else if (volume < 66)
			pixbuf = pixbufs[VOLUME_MEDIUM];
		else
			pixbuf = pixbufs[VOLUME_HIGH];
	} else {
		pixbuf = pixbufs[VOLUME_MUTED];
	}

	if (vol_meter && muted == FALSE)
		pixbuf = vol_meter_draw(vol_meter, pixbuf, volume);

	gtk_status_icon_set_from_pixbuf(status_icon, pixbuf);
}

/* Update the tray icon tooltip according to the current audio state. */
static void
update_status_icon_tooltip(GtkStatusIcon *status_icon,
                           const gchar *card, const gchar *channel,
                           gdouble volume, gboolean muted)
{
	char tooltip[64];

	if (!muted)
		snprintf(tooltip, sizeof tooltip, "%s (%s)\n%s: %ld %%",
		         card, channel, _("Volume"), lround(volume));
	else
		snprintf(tooltip, sizeof tooltip, "%s (%s)\n%s: %ld %%\n%s",
		         card, channel, _("Volume"), lround(volume), _("Muted"));

	gtk_status_icon_set_tooltip_text(status_icon, tooltip);
}

/* Public functions & signal handlers */

struct tray_icon {
	Audio *audio;
	VolMeter *vol_meter;
	GdkPixbuf **pixbufs;
	GtkStatusIcon *status_icon;
	gint status_icon_size;
};

/**
 * Handles the 'activate' signal on the GtkStatusIcon, bringing up or hiding
 * the volume popup window. Usually triggered by left-click.
 *
 * @param status_icon the object which received the signal.
 * @param icon TrayIcon instance set when the signal handler was connected.
 */
static void
on_activate(G_GNUC_UNUSED GtkStatusIcon *status_icon,
            G_GNUC_UNUSED TrayIcon *icon)
{
	do_toggle_popup_window();
}

/**
 * Handles the 'popup-menu' signal on the GtkStatusIcon, bringing up
 * the context popup menu. Usually triggered by right-click.
 *
 * @param status_icon the object which received the signal.
 * @param button the button that was pressed, or 0 if the signal
 * is not emitted in response to a button press event.
 * @param activate_time the timestamp of the event that triggered
 * the signal emission.
 * @param icon TrayIcon instance set when the signal handler was connected.
 */
static void
on_popup_menu(GtkStatusIcon *status_icon, guint button,
              guint activate_time, G_GNUC_UNUSED TrayIcon *icon)
{
	do_show_popup_menu(gtk_status_icon_position_menu, status_icon, button, activate_time);
}

/**
 * Handles 'button-release-event' signal on the GtkStatusIcon.
 * Only used for middle-click, which runs the middle click action.
 *
 * @param status_icon the object which received the signal.
 * @param event the GdkEventButton which triggered this signal.
 * @param icon TrayIcon instance set when the signal handler was connected.
 * @return TRUE to stop other handlers from being invoked for the event.
 * FALSE to propagate the event further.
 */
static gboolean
on_button_release_event(G_GNUC_UNUSED GtkStatusIcon *status_icon,
                        GdkEventButton *event, G_GNUC_UNUSED TrayIcon *icon)
{
	int middle_click_action;

	if (event->button != 2)
		return FALSE;

	middle_click_action = prefs_get_integer("MiddleClickAction", 0);

	switch (middle_click_action) {
	case 0:
		audio_toggle_mute(icon->audio, AUDIO_USER_TRAY_ICON);
		break;
	case 1:
		run_prefs_dialog();
		break;
	case 2:
		run_mixer_command();
		break;
	case 3:
		run_custom_command();
		break;
	default: {
	}	// nothing
	}

	return FALSE;
}

/**
 * Handles 'scroll-event' signal on the GtkStatusIcon, changing the volume
 * accordingly.
 *
 * @param status_icon the object which received the signal.
 * @param event the GdkEventScroll which triggered this signal.
 * @param icon TrayIcon instance set when the signal handler was connected.
 * @return TRUE to stop other handlers from being invoked for the event.
 * FALSE to propagate the event further
 */
static gboolean
on_scroll_event(G_GNUC_UNUSED GtkStatusIcon *status_icon, GdkEventScroll *event,
                TrayIcon *icon)
{
	if (event->direction == GDK_SCROLL_UP)
		audio_raise_volume(icon->audio, AUDIO_USER_TRAY_ICON);
	else if (event->direction == GDK_SCROLL_DOWN)
		audio_lower_volume(icon->audio, AUDIO_USER_TRAY_ICON);

	return FALSE;
}

/**
 * Handles the 'size-changed' signal on the GtkStatusIcon.
 * Happens when the panel holding the tray icon is resized.
 * Also happens at startup during the tray icon creation.
 *
 * @param status_icon the object which received the signal.
 * @param size the new size.
 * @param icon TrayIcon instance set when the signal handler was connected.
 * @return FALSE, so that Gtk+ scales the icon as necessary.
 */
static gboolean
on_size_changed(G_GNUC_UNUSED GtkStatusIcon *status_icon, gint size, TrayIcon *icon)
{
	DEBUG("Tray icon size is now %d", size);

	/* Ensure a minimum size. This is needed for Gtk2.
	 * With Gtk2, this handler is invoked with a zero size at startup,
	 * which screws up things here and there.
	 */
	if (size < ICON_MIN_SIZE) {
		size = ICON_MIN_SIZE;
		DEBUG("Forcing size to the minimum value %d", size);
	}

	/* Save new size */
	icon->status_icon_size = size;

	/* Rebuild everything */
	tray_icon_reload(icon);

	return FALSE;
}

/**
 * Handle signals from the audio subsystem.
 *
 * @param audio the Audio instance that emitted the signal.
 * @param event the AudioEvent containing useful information.
 * @param data user supplied data.
 */
static void
on_audio_changed(G_GNUC_UNUSED Audio *audio, AudioEvent *event, gpointer data)
{
	TrayIcon *icon = (TrayIcon *) data;
	update_status_icon_pixbuf(icon->status_icon, icon->pixbufs, icon->vol_meter,
	                          event->volume, event->muted);
	update_status_icon_tooltip(icon->status_icon, event->card, event->channel,
	                           event->volume, event->muted);
}

/**
 * Update the tray icon according to the current preferences
 * and the current audio status. Both can't be separated.
 * This has to be called each time the preferences are modified.
 *
 * @param icon a TrayIcon instance.
 */
void
tray_icon_reload(TrayIcon *icon)
{
	const gchar *card, *channel;
	gdouble volume;
	gboolean muted;

	pixbuf_array_free(icon->pixbufs);
	icon->pixbufs = pixbuf_array_new(icon->status_icon_size);

	vol_meter_free(icon->vol_meter);
	icon->vol_meter = vol_meter_new();

	card = audio_get_card(icon->audio);
	channel = audio_get_channel(icon->audio);
	volume = audio_get_volume(icon->audio);
	muted = audio_is_muted(icon->audio);
	update_status_icon_pixbuf(icon->status_icon, icon->pixbufs, icon->vol_meter,
	                          volume, muted);
	update_status_icon_tooltip(icon->status_icon, card, channel, volume, muted);
}

/**
 * Destroys the tray icon, freeing any resources.
 *
 * @param icon a TrayIcon instance.
 */
void
tray_icon_destroy(TrayIcon *icon)
{
	DEBUG("Destroying");

	audio_signals_disconnect(icon->audio, on_audio_changed, icon);
	g_object_unref(icon->status_icon);
	pixbuf_array_free(icon->pixbufs);
	vol_meter_free(icon->vol_meter);
	g_free(icon);
}

/**
 * Creates the tray icon and connects all the signals.
 *
 * @return the newly created TrayIcon instance.
 */
TrayIcon *
tray_icon_create(Audio *audio)
{
	TrayIcon *icon;

	DEBUG("Creating tray icon");

	icon = g_new0(TrayIcon, 1);

	/* Create everything */
	icon->vol_meter = vol_meter_new();
	icon->status_icon = gtk_status_icon_new();
	icon->status_icon_size = ICON_MIN_SIZE;

	/* Connect ui signal handlers */

	// Left-click
	g_signal_connect(icon->status_icon, "activate",
	                 G_CALLBACK(on_activate), icon);
	// Right-click
	g_signal_connect(icon->status_icon, "popup-menu",
	                 G_CALLBACK(on_popup_menu), icon);
	// Middle-click
	g_signal_connect(icon->status_icon, "button-release-event",
	                 G_CALLBACK(on_button_release_event), icon);
	// Mouse scrolling on the icon
	g_signal_connect(icon->status_icon, "scroll_event",
	                 G_CALLBACK(on_scroll_event), icon);
	// Change of size
	g_signal_connect(icon->status_icon, "size-changed",
	                 G_CALLBACK(on_size_changed), icon);

	/* Connect audio signals handlers */
	icon->audio = audio;
	audio_signals_connect(audio, on_audio_changed, icon);

	/* Display icon */
	gtk_status_icon_set_visible(icon->status_icon, TRUE);

	/* Load preferences */
	tray_icon_reload(icon);

	return icon;
}
