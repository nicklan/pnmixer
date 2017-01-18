/* ui-popup-menu.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file ui-popup-menu.c
 * This file holds the ui-related code for the popup menu.
 * @brief Popup menu subsystem.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "audio.h"
#include "support-log.h"
#include "support-intl.h"
#include "support-ui.h"
#include "ui-popup-menu.h"
#include "ui-about-dialog.h"

#include "main.h"

#ifdef WITH_GTK3
#define POPUP_MENU_UI_FILE "popup-menu-gtk3.glade"
#else
#define POPUP_MENU_UI_FILE "popup-menu-gtk2.glade"
#endif

/* Helpers */

#ifdef WITH_GTK3

/* Updates the mute checkbox according to the current audio state. */
static void
update_mute_check(GtkToggleButton *mute_check, gboolean muted)
{
	/* On Gtk3 version, we listen for the signal sent by the GtkMenuItem.
	 * So, when we change the value of the GtkToggleButton, we don't have
	 * to block the signal handlers, since there's nobody listening to the
	 * GtkToggleButton anyway.
	 */
	gtk_toggle_button_set_active(mute_check, muted);
}

#else

/* Updates the mute item according to the current audio state. */
static void
update_mute_item(GtkCheckMenuItem *mute_item, GCallback handler_func,
                 gpointer handler_data, gboolean muted)
{
	/* On Gtk2 version, we must block the signals sent by the GtkCheckMenuItem
	 * before we update it manually.
	 */
	gint n_blocked;

	n_blocked = g_signal_handlers_block_by_func
	            (G_OBJECT(mute_item), DATA_PTR(handler_func), handler_data);
	g_assert(n_blocked == 1);

	gtk_check_menu_item_set_active(mute_item, muted);

	g_signal_handlers_unblock_by_func
	(G_OBJECT(mute_item),  DATA_PTR(handler_func), handler_data);
}

#endif

/* Public functions & signal handlers */

struct popup_menu {
	/* Audio system */
	Audio *audio;
	/* Widgets */
	GtkWidget *menu_window;
	GtkWidget *menu;
#ifdef WITH_GTK3
	GtkWidget *mute_check;
#else
	GtkWidget *mute_item;
#endif
};

/**
 * Handles a click on 'mute_item', toggling the mute audio state.
 *
 * @param item the object which received the signal.
 * @param menu PopupMenu instance set when the signal handler was connected.
 */
void
#ifdef WITH_GTK3
on_mute_item_activate(G_GNUC_UNUSED GtkMenuItem *item,
#else
on_mute_item_activate(G_GNUC_UNUSED GtkCheckMenuItem *item,
#endif
                      PopupMenu *menu)
{
	audio_toggle_mute(menu->audio, AUDIO_USER_POPUP);
}

/**
 * Handles a click on 'mixer_item', opening the specified mixer application.
 *
 * @param item the object which received the signal.
 * @param menu PopupMenu instance set when the signal handler was connected.
 */
void
on_mixer_item_activate(G_GNUC_UNUSED GtkMenuItem *item,
                       G_GNUC_UNUSED PopupMenu *menu)
{
	run_mixer_command();
}

/**
 * Handles a click on 'prefs_item', opening the preferences window.
 *
 * @param item the object which received the signal.
 * @param menu PopupMenu instance set when the signal handler was connected.
 */
void
on_prefs_item_activate(G_GNUC_UNUSED GtkMenuItem *item,
                       G_GNUC_UNUSED PopupMenu *menu)
{
	run_prefs_dialog();
}

/**
 * Handles a click on 'reload_item', re-initializing the audio subsystem.
 *
 * @param item the object which received the signal.
 * @param menu PopupMenu instance set when the signal handler was connected.
 */
void
on_reload_item_activate(G_GNUC_UNUSED GtkMenuItem *item,
                        PopupMenu *menu)
{
	audio_reload(menu->audio);
}

/**
 * Handles a click on 'about_item', opening the About dialog.
 *
 * @param item the object which received the signal.
 * @param menu PopupMenu instance set when the signal handler was connected.
 */
void
on_about_item_activate(G_GNUC_UNUSED GtkMenuItem *item,
                       G_GNUC_UNUSED PopupMenu *menu)
{
	run_about_dialog();
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
	PopupMenu *menu = (PopupMenu *) data;

#ifdef WITH_GTK3
	update_mute_check(GTK_TOGGLE_BUTTON(menu->mute_check), event->muted);
#else
	update_mute_item(GTK_CHECK_MENU_ITEM(menu->mute_item),
	                 G_CALLBACK(on_mute_item_activate),
	                 menu, event->muted);
#endif
}

/**
 * Return a pointer toward the internal GtkWindow instance.
 *
 * @param menu a PopupMenu instance.
 */
GtkWindow *
popup_menu_get_window(PopupMenu *menu)
{
	return GTK_WINDOW(menu->menu_window);
}

/**
 * Shows the popup menu.
 * The weird prototype of this function comes from the underlying
 * gtk_menu_popup() that is used to display the popup menu.
 *
 * @param menu a PopupMenu instance.
 * @param func a user supplied function used to position the menu, or NULL.
 * @param data user supplied data to be passed to func.
 * @param button the mouse button which was pressed to initiate the event.
 * @param activate_time the time at which the activation event occurred.
 */
void
popup_menu_show(PopupMenu *menu, GtkMenuPositionFunc func, gpointer data,
                guint button, guint activate_time)
{
#if GTK_CHECK_VERSION(3,22,0)
	(void) func;
	(void) data;
	(void) button;
	(void) activate_time;
	gtk_menu_popup_at_pointer(GTK_MENU(menu->menu), NULL);
#else
	gtk_menu_popup(GTK_MENU(menu->menu), NULL, NULL,
	               func, data, button, activate_time);
#endif
}

/**
 * Destroys the popup menu, freeing any resources.
 *
 * @param menu a PopupMenu instance.
 */
void
popup_menu_destroy(PopupMenu *menu)
{
	DEBUG("Destroying");

	audio_signals_disconnect(menu->audio, on_audio_changed, menu);
	gtk_widget_destroy(menu->menu_window);
	g_free(menu);
}

/**
 * Creates the popup menu and connects all the signals.
 *
 * @param audio pointer to this audio subsystem.
 * @return the newly created PopupMenu instance.
 */
PopupMenu *
popup_menu_create(Audio *audio)
{
	gchar *uifile;
	GtkBuilder *builder;
	PopupMenu *menu;

	menu = g_new0(PopupMenu, 1);

	/* Build UI file */
	uifile = get_ui_file(POPUP_MENU_UI_FILE);
	g_assert(uifile);

	DEBUG("Building from ui file '%s'", uifile);
	builder = gtk_builder_new_from_file(uifile);

	/* Save some widgets for later use */
	assign_gtk_widget(builder, menu, menu_window);
	assign_gtk_widget(builder, menu, menu);
#ifdef WITH_GTK3
	assign_gtk_widget(builder, menu, mute_check);
#else
	assign_gtk_widget(builder, menu, mute_item);
#endif

#ifdef WITH_GTK3
	/* Gtk3 doesn't seem to scale images automatically like Gtk2 did.
	 * If we have a 'broken' icon set that only provides one size of icon
	 * (let's say 128x128), Gtk2 would scale it appropriately, but not Gtk3.
	 * This will result in huge icons in the menu.
	 * If we follow the Gtk3 logic, then we shouldn't do anything to handle that,
	 * and when users report such problem, we tell them to fix the icon theme.
	 * If we want PNMixer to work in as many cases as possible, then we must
	 * handle the broken icon theme and resize the icons by ourself.
	 * We choose the second option here.
	 */
	GtkRequisition label_req;
	GtkRequisition image_req;
	GtkWidget *mute_accellabel;
	GtkWidget *mixer_image;
	GtkWidget *prefs_image;
	GtkWidget *reload_image;
	GtkWidget *about_image;
	GtkWidget *quit_image;

	mute_accellabel = gtk_builder_get_widget(builder, "mute_accellabel");
	mixer_image = gtk_builder_get_widget(builder, "mixer_image");
	prefs_image = gtk_builder_get_widget(builder, "prefs_image");
	reload_image = gtk_builder_get_widget(builder, "reload_image");
	about_image = gtk_builder_get_widget(builder, "about_image");
	quit_image = gtk_builder_get_widget(builder, "quit_image");

	gtk_widget_get_preferred_size(mute_accellabel, &label_req, NULL);
	gtk_widget_get_preferred_size(mixer_image, &image_req, NULL);

	/* We only care about height. We want the image to stick to the text height. */
	if (image_req.height > (label_req.height + 1)) {
		gint new_height = label_req.height;

		if (new_height % 2)
			new_height++; // make it even

		DEBUG("Gtk3 workaround: resizing images from %dpx to %dpx",
		      image_req.height, new_height);
		gtk_image_set_pixel_size(GTK_IMAGE(mixer_image), new_height);
		gtk_image_set_pixel_size(GTK_IMAGE(prefs_image), new_height);
		gtk_image_set_pixel_size(GTK_IMAGE(reload_image), new_height);
		gtk_image_set_pixel_size(GTK_IMAGE(about_image), new_height);
		gtk_image_set_pixel_size(GTK_IMAGE(quit_image), new_height);
	}
#endif

	/* Connect ui signal handlers */
	gtk_builder_connect_signals(builder, menu);

	/* Connect audio signal handlers */
	menu->audio = audio;
	audio_signals_connect(audio, on_audio_changed, menu);

	/* Cleanup */
	g_object_unref(builder);
	g_free(uifile);

	return menu;
}
