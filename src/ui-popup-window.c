/* ui-popup-window.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file ui-popup-window.c
 * This file holds the ui-related code for the popup window.
 *
 * There are two ways to handle the slider value.
 * - It can reflect the value set by the user.
 * It means that we ignore the volume value reported by the audio system.
 * - It can reflect the value reported by the audio system.
 * It means that we prevent Gtk from updating the slider, and we do it
 * ourselves when the audio system invoke the callback.
 *
 * I tried both ways. The second one is a real pain in the ass. A few
 * unexpected problems arise, it makes things complex, and ends up with
 * dirty workarounds. So, trust me, don't try it.
 *
 * The first way is much simpler, works great.
 *
 * So, here's the current behavior for the slider.
 * When you open it, it queries the audio system, and display the real
 * volume value. Then, when you change the volume, it displays the volume
 * as you ask it to be, not as it is. So if volume is 60, and you move it
 * to 61, then 61 is displayed. But the truth may be that the volume is
 * still 60, because your hardware doesn't have such fine capabilities,
 * and the next real step will be 65.
 *
 * @brief Popup window subsystem.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#ifndef WITH_GTK3
#include <gdk/gdkkeysyms.h>
#endif

#include "audio.h"
#include "prefs.h"
#include "support-intl.h"
#include "support-log.h"
#include "support-ui.h"
#include "ui-popup-window.h"

#include "main.h"

#ifdef WITH_GTK3
#define POPUP_WINDOW_HORIZONTAL_UI_FILE "popup-window-horizontal-gtk3.glade"
#define POPUP_WINDOW_VERTICAL_UI_FILE   "popup-window-vertical-gtk3.glade"
#else
#define POPUP_WINDOW_HORIZONTAL_UI_FILE "popup-window-horizontal-gtk2.glade"
#define POPUP_WINDOW_VERTICAL_UI_FILE   "popup-window-vertical-gtk2.glade"
#endif

/* Helpers */

/* Configure the appearance of the text that is shown around the volume slider,
 * according to the current preferences.
 */
static void
configure_vol_text(GtkScale *vol_scale)
{
	gboolean enabled;
	gint position;
	GtkPositionType gtk_position;

	enabled = prefs_get_boolean("DisplayTextVolume", TRUE);
	position = prefs_get_integer("TextVolumePosition", 0);

	gtk_position =
	        position == 0 ? GTK_POS_TOP :
	        position == 1 ? GTK_POS_BOTTOM :
	        position == 2 ? GTK_POS_LEFT :
	        GTK_POS_RIGHT;

	if (enabled) {
		gtk_scale_set_draw_value(vol_scale, TRUE);
		gtk_scale_set_value_pos(vol_scale, gtk_position);
	} else {
		gtk_scale_set_draw_value(vol_scale, FALSE);
	}
}

/* Configure the page and step increment of the volume slider,
 * according to the current preferences.
 */
static void
configure_vol_increment(GtkAdjustment *vol_scale_adj)
{
	gdouble scroll_step;
	gdouble fine_scroll_step;

	scroll_step = prefs_get_double("ScrollStep", 5);
	fine_scroll_step =  prefs_get_double("FineScrollStep", 1);

	gtk_adjustment_set_page_increment(vol_scale_adj, scroll_step);
	gtk_adjustment_set_step_increment(vol_scale_adj, fine_scroll_step);
}

/* Update the mute checkbox according to the current audio state. */
static void
update_mute_check(GtkToggleButton *mute_check, GCallback handler_func,
                  gpointer handler_data, gboolean has_mute, gboolean muted)
{
	gint n_blocked;

	n_blocked = g_signal_handlers_block_by_func
	            (G_OBJECT(mute_check), DATA_PTR(handler_func), handler_data);
	g_assert(n_blocked == 1);

	if (has_mute == FALSE) {
		gtk_toggle_button_set_active(mute_check, TRUE);
		gtk_widget_set_sensitive(GTK_WIDGET(mute_check), FALSE);
		gtk_widget_set_tooltip_text(GTK_WIDGET(mute_check),
		                            _("Soundcard has no mute switch"));
	} else {
		gtk_toggle_button_set_active(mute_check, muted);
		gtk_widget_set_tooltip_text(GTK_WIDGET(mute_check), NULL);
	}

	g_signal_handlers_unblock_by_func
	(G_OBJECT(mute_check), DATA_PTR(handler_func), handler_data);
}

/* Update the volume slider according to the current audio state. */
static void
update_volume_slider(GtkAdjustment *vol_scale_adj, gdouble volume)
{
	gtk_adjustment_set_value(vol_scale_adj, volume);
}

/* Grab mouse and keyboard */
#ifdef WITH_GTK3
#if GTK_CHECK_VERSION(3,20,0)
static void
grab_devices(GtkWidget *window)
{
	GdkGrabStatus status;
	GdkDevice *device;

	/* Grab the current event device */
	device = gtk_get_current_event_device();
	if (device == NULL) {
		WARN("Couldn't get current device");
		return;
	}

	/* Grab every seat capabilities */
	status = gdk_seat_grab(gdk_device_get_seat(device),
	                       gtk_widget_get_window(window),
	                       GDK_SEAT_CAPABILITY_ALL, TRUE,
	                       NULL, NULL, NULL, NULL);
	if (status != GDK_GRAB_SUCCESS)
		WARN("Could not grab %s", gdk_device_get_name(device));
}
#else
static void
grab_devices(GtkWidget *window)
{
	GdkDevice *pointer_dev;
	GdkDevice *keyboard_dev;
	GdkGrabStatus status;

	/* Grab the mouse */
	pointer_dev = gtk_get_current_event_device();
	if (pointer_dev == NULL) {
		WARN("Couldn't get current device");
		return;
	}

	status = gdk_device_grab(pointer_dev,
	                         gtk_widget_get_window(window),
	                         GDK_OWNERSHIP_NONE,
	                         TRUE,
	                         GDK_BUTTON_PRESS_MASK,
	                         NULL,
	                         GDK_CURRENT_TIME);
	if (status != GDK_GRAB_SUCCESS)
		WARN("Could not grab %s", gdk_device_get_name(pointer_dev));

	/* Grab the keyboard */
	keyboard_dev = gdk_device_get_associated_device(pointer_dev);
	if (keyboard_dev == NULL) {
		WARN("Couldn't get associated device");
		return;
	}

	status = gdk_device_grab(keyboard_dev,
	                         gtk_widget_get_window(window),
	                         GDK_OWNERSHIP_NONE,
	                         TRUE,
	                         GDK_KEY_PRESS_MASK,
	                         NULL, GDK_CURRENT_TIME);
	if (status != GDK_GRAB_SUCCESS)
		WARN("Could not grab %s", gdk_device_get_name(keyboard_dev));
}
#endif /*  GTK_CHECK_VERSION(3,20,0) */
#else
static void
grab_devices(GtkWidget *window)
{
	gdk_pointer_grab(gtk_widget_get_window(window), TRUE,
	                 GDK_BUTTON_PRESS_MASK, NULL, NULL,
	                 GDK_CURRENT_TIME);
	gdk_keyboard_grab(gtk_widget_get_window(window), TRUE,
	                  GDK_CURRENT_TIME);
}
#endif /* WITH_GTK3 */

/* Public functions & signal handlers */

struct popup_window {
	/* Audio system */
	Audio *audio;
	/* Widgets */
	GtkWidget *popup_window;
	GtkWidget *vol_scale;
	GtkAdjustment *vol_scale_adj;
	GtkWidget *mute_check;
};

/**
 * Handles 'button-press-event', 'key-press-event' and 'grab-broken-event' signals,
 * on the GtkWindow. Used to hide the volume popup window.
 *
 * @param widget the object which received the signal.
 * @param event the GdkEvent which triggered this signal.
 * @param window user data set when the signal handler was connected.
 * @return TRUE to stop other handlers from being invoked for the event.
 * FALSE to propagate the event further.
 */
gboolean
on_popup_window_event(G_GNUC_UNUSED GtkWidget *widget, GdkEvent *event,
                      PopupWindow *window)
{
	switch (event->type) {

	/* If a click happens outside of the popup, hide it */
	case GDK_BUTTON_PRESS: {
		gint x, y;
#ifdef WITH_GTK3
		GdkDevice *device = gtk_get_current_event_device();

		if (!gdk_device_get_window_at_position(device, &x, &y))
#else
		if (!gdk_window_at_pointer(&x, &y))
#endif
			popup_window_hide(window);

		break;
	}

	/* If 'Esc' is pressed, hide popup */
	case GDK_KEY_PRESS:
		if (event->key.keyval == GDK_KEY_Escape)
			popup_window_hide(window);
		break;

	/* Broken grab, hide popup */
	case GDK_GRAB_BROKEN:
		popup_window_hide(window);
		break;

	/* Unhandle event, do nothing */
	default:
		break;
	}

	return FALSE;
}

/**
 * Handles the 'value-changed' signal on the GtkRange 'vol_scale',
 * changing the voume accordingly.
 *
 * There are many ways for the user to change the slider value.
 * Think about testing them all if you touch this function.
 * Here's a list of actions you can do to trigger this callback:
 *  - click somewhere on the slider
 *  - click on the slider's knob and drag it
 *  - mouse scroll on the slider
 *  - keyboard (when the slider has focus), here's a list of keys:
 *      Up, Down, Left, Right, Page Up, Page Down, Home, End
 *
 * @param range the GtkRange that received the signal.
 * @param window user data set when the signal handler was connected.
 */
void
on_vol_scale_value_changed(GtkRange *range, PopupWindow *window)
{
	gdouble value;

	value = gtk_range_get_value(range);
	audio_set_volume(window->audio, AUDIO_USER_POPUP, value, 0);
}

/**
 * Handles the 'toggled' signal on the GtkToggleButton 'mute_check',
 * changing the mute status accordingly.
 *
 * @param button the GtkToggleButton that received the signal.
 * @param window user data set when the signal handler was connected.
 */
void
on_mute_check_toggled(G_GNUC_UNUSED GtkToggleButton *button, PopupWindow *window)
{
	audio_toggle_mute(window->audio, AUDIO_USER_POPUP);
}

/**
 * Handles the 'clicked' signal on the GtkButton 'mixer_button',
 * therefore opening the mixer application.
 *
 * @param button the GtkButton that received the signal.
 * @param window user data set when the signal handler was connected.
 */
void
on_mixer_button_clicked(G_GNUC_UNUSED GtkButton *button, PopupWindow *window)
{
	popup_window_hide(window);
	run_mixer_command();
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
	PopupWindow *window = (PopupWindow *) data;
	GtkWidget *popup_window = window->popup_window;

	/* Nothing to do if the window is hidden.
	 * The window will be updated anyway when shown.
	 */
	if (!gtk_widget_get_visible(popup_window))
		return;

	/* Update mute checkbox */
	update_mute_check(GTK_TOGGLE_BUTTON(window->mute_check),
	                  G_CALLBACK(on_mute_check_toggled), window,
	                  event->has_mute, event->muted);

	/* Update volume slider
	 * If the user changes the volume through the popup window,
	 * we MUST NOT update the slider value, it's been done already.
	 * It means that, as long as the popup window is visible,
	 * the slider value reflects the value set by user,
	 * and not the real value reported by the audio system.
	 */
	if (event->user != AUDIO_USER_POPUP)
		update_volume_slider(window->vol_scale_adj, event->volume);
}

/**
 * Shows the popup window, and grab the focus.
 *
 * @param window a PopupWindow instance.
 */
void
popup_window_show(PopupWindow *window)
{
	GtkWidget *popup_window = window->popup_window;
	GtkWidget *vol_scale = window->vol_scale;
	Audio *audio = window->audio;

	/* Update window elements at first */
	update_mute_check(GTK_TOGGLE_BUTTON(window->mute_check),
	                  G_CALLBACK(on_mute_check_toggled), window,
	                  audio_has_mute(audio), audio_is_muted(audio));
	update_volume_slider(window->vol_scale_adj,
	                     audio_get_volume(audio));

	/* Show the window */
	gtk_widget_show_now(popup_window);

	/* Give focus to volume scale */
	gtk_widget_grab_focus(vol_scale);

	/* Grab mouse and keyboard */
	grab_devices(popup_window);
}

/**
 * Hides the popup window.
 *
 * @param window a PopupWindow instance.
 */
void
popup_window_hide(PopupWindow *window)
{
	gtk_widget_hide(window->popup_window);
}

/**
 * Toggle the popup window (aka hide or show).
 *
 * @param window a PopupWindow instance.
 */
void
popup_window_toggle(PopupWindow *window)
{
	GtkWidget *popup_window = window->popup_window;

	if (gtk_widget_get_visible(popup_window))
		popup_window_hide(window);
	else
		popup_window_show(window);
}

/*
 * Cleanup a popup window.
 */
static void
popup_window_cleanup(PopupWindow *window)
{
	DEBUG("Destroying");

	/* Disconnect audio signals */
	audio_signals_disconnect(window->audio, on_audio_changed, window);

	/* Destroy the Gtk window, freeing any resources */
	gtk_widget_destroy(window->popup_window);

	/* Set the struct value to zero since it will be re-used */
	memset(window, 0, sizeof(PopupWindow));
}

/* Initialize a popup window.
 * The struct is supposed to be empty at this point.
 */
static void
popup_window_init(PopupWindow *window, Audio *audio)
{
	gchar *uifile;
	GtkBuilder *builder;

	/* Build UI file depending on slider orientation */
	gchar *orientation;
	orientation = prefs_get_string("SliderOrientation", "vertical");
	if (!g_strcmp0(orientation, "horizontal"))
		uifile = get_ui_file(POPUP_WINDOW_HORIZONTAL_UI_FILE);
	else
		uifile = get_ui_file(POPUP_WINDOW_VERTICAL_UI_FILE);
	g_free(orientation);

	DEBUG("Building from ui file '%s'", uifile);
	builder = gtk_builder_new_from_file(uifile);

	/* Save some widgets for later use */
	assign_gtk_widget(builder, window, popup_window);
	assign_gtk_widget(builder, window, mute_check);
	assign_gtk_widget(builder, window, vol_scale);
	assign_gtk_adjustment(builder, window, vol_scale_adj);

	/* Configure some widgets */
	configure_vol_text(GTK_SCALE(window->vol_scale));
	configure_vol_increment(GTK_ADJUSTMENT(window->vol_scale_adj));

	/* Connect ui signal handlers */
	gtk_builder_connect_signals(builder, window);

	/* Connect audio signal handlers */
	window->audio = audio;
	audio_signals_connect(audio, on_audio_changed, window);

	/* Cleanup */
	g_object_unref(builder);
	g_free(uifile);
}

/**
 * Update the popup window according to the current preferences.
 * This has to be called each time the preferences are modified.
 *
 * @param window a PopupWindow instance.
 */
void
popup_window_reload(PopupWindow *window)
{
	Audio *audio;

	/* Keep track of this pointer before cleaning up, we need it */
	audio = window->audio;

	popup_window_cleanup(window);
	popup_window_init(window, audio);
}

/**
 * Destroys the popup window, freeing any resources.
 *
 * @param window a PopupWindow instance.
 */
void
popup_window_destroy(PopupWindow *window)
{
	popup_window_cleanup(window);
	g_free(window);
}

/**
 * Creates the popup window and connects all the signals.
 *
 * @param audio pointer to this audio subsystem.
 * @return the newly created PopupWindow instance.
 */
PopupWindow *
popup_window_create(Audio *audio)
{
	PopupWindow *window;

	window = g_new0(PopupWindow, 1);
	popup_window_init(window, audio);

	return window;
}
