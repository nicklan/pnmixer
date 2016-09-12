/* ui-hotkey-dialog.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file ui-hotkey-dialog.c
 * This file holds the ui-related code for the hotkey dialog,
 * usually run from the preferences window.
 * @brief Hotkey dialog subsystem.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <glib.h>
#include <gtk/gtk.h>

#include "support-log.h"
#include "support-intl.h"
#include "support-ui.h"
#include "ui-hotkey-dialog.h"

#include "main.h"

#ifdef WITH_GTK3
#define HOTKEY_DIALOG_UI_FILE "hotkey-dialog-gtk3.glade"
#else
#define HOTKEY_DIALOG_UI_FILE "hotkey-dialog-gtk2.glade"
#endif

/* Helpers */

/* Configure the appearance of the hotkey dialog window. */
static void
configure_hotkey_dialog(GtkWindow *window, GtkLabel *instruction_label,
                        const gchar *hotkey)
{
	gchar *title;
	gchar *instruction;

	title = g_strdup_printf(_("Set %s HotKey"), hotkey);
	gtk_window_set_title(window, title);
	g_free(title);

	instruction = g_strdup_printf(_("Press new HotKey for <b>%s</b>"), hotkey);
	gtk_label_set_markup(instruction_label, instruction);
	g_free(instruction);
}

/* Public functions & signal handlers */

struct hotkey_dialog {
	GtkWidget *hotkey_dialog;
	GtkWidget *instruction_label;
	GtkWidget *key_pressed_label;
};

/**
 * Handles the 'key-press-event' signal on the GtkDialog 'hotkey_dialog'.
 * Update the text displayed in the dialog.
 *
 * @param widget the object which received the signal.
 * @param event the GdkEventKey which triggered the signal.
 * @param dialog user data set when the signal handler was connected.
 * @return TRUE to stop other handlers from being invoked for the event.
 * FALSE to propagate the event further.
 */
gboolean
on_hotkey_dialog_key_press_event(G_GNUC_UNUSED GtkWidget *widget,
                                 GdkEventKey *event, HotkeyDialog *dialog)
{
	GdkModifierType state, consumed;
	GtkLabel *key_pressed_label;
	gchar *key_text;
	guint key_val;

	key_pressed_label = GTK_LABEL(dialog->key_pressed_label);

	state = event->state;
	gdk_keymap_translate_keyboard_state(gdk_keymap_get_default(),
	                                    event->hardware_keycode,
	                                    state, event->group, &key_val,
	                                    NULL, NULL, &consumed);

	state &= ~consumed;
	state &= gtk_accelerator_get_default_mod_mask();

	key_text = gtk_accelerator_name(key_val, state);
	gtk_label_set_text(key_pressed_label, key_text);
	g_free(key_text);

	return FALSE;
}

/**
 * Handles the signal 'key-release-event' on the GtkDialog 'hotkey_dialog'.
 * Closes the dialog.
 *
 * @param widget the object which received the signal.
 * @param event the GdkEventKey which triggered the signal.
 * @param dialog user data set when the signal handler was connected.
 * @return TRUE to stop other handlers from being invoked for the event.
 * FALSE to propagate the event further.
 */
gboolean
on_hotkey_dialog_key_release_event(GtkWidget *widget,
                                   G_GNUC_UNUSED GdkEventKey *event,
                                   G_GNUC_UNUSED HotkeyDialog *dialog)
{
	gtk_dialog_response(GTK_DIALOG(widget), GTK_RESPONSE_OK);

	return FALSE;
}

/**
 * Runs the hotkey dialog, and returns a string representing the hotkey
 * that has been pressed. String must be freed.
 *
 * @param dialog a HotkeyDialog instance.
 * @return the hotkey pressed, must be freed.
 */
gchar *
hotkey_dialog_run(HotkeyDialog *dialog)
{
	GtkDialog *hotkey_dialog = GTK_DIALOG(dialog->hotkey_dialog);
	GtkLabel *key_pressed_label = GTK_LABEL(dialog->key_pressed_label);
	GdkWindow *window;
	GdkGrabStatus grab_status;
	gint resp;

	/* Show widget at first, otherwise the following calls fail */
	gtk_widget_show_now(GTK_WIDGET(hotkey_dialog));

	/* Grab keyboard */
#ifdef WITH_GTK3
	GdkDevice *device;

	device = gtk_get_current_event_device();
	if (device == NULL) {
		WARN("Couldn't get current device");
                return NULL;
	}

	window = gtk_widget_get_window(GTK_WIDGET(hotkey_dialog));

#if GTK_CHECK_VERSION(3,20,0)
	grab_status = gdk_seat_grab
		(gdk_device_get_seat(device),
		 window,
		 GDK_SEAT_CAPABILITY_KEYBOARD, TRUE,
		 NULL, NULL, NULL, NULL);
#else
	grab_status = gdk_device_grab
		(device,
		 window,
		 GDK_OWNERSHIP_APPLICATION, TRUE,
		 GDK_KEY_PRESS_MASK, NULL, GDK_CURRENT_TIME);
#endif /*  GTK_CHECK_VERSION(3,20,0) */
#else
	window = gtk_widget_get_window(GTK_WIDGET(hotkey_dialog));

	grab_status = gdk_keyboard_grab
		(window, TRUE, GDK_CURRENT_TIME);
#endif /* WITH_GTK3 */

	if (grab_status != GDK_GRAB_SUCCESS) {
		run_error_dialog(_("Could not grab the keyboard."));
		return NULL;
	}

	/* Run the dialog */
	resp = gtk_dialog_run(hotkey_dialog);

	/* Ungrab keyboard */
#ifdef WITH_GTK3
#if GTK_CHECK_VERSION(3,20,0)
	gdk_seat_ungrab(gdk_device_get_seat(device));
#else
	gdk_device_ungrab(device, GDK_CURRENT_TIME);
#endif /* GTK_CHECK_VERSION(3,20,0) */
#else
	gdk_keyboard_ungrab(GDK_CURRENT_TIME);
#endif /* WITH_GTK3 */

	/* Handle response */
	if (resp != GTK_RESPONSE_OK)
		return NULL;

	/* Return the key entered */
	return g_strdup(gtk_label_get_text(key_pressed_label));
}

/**
 * Destroys a hotkey dialog, freeing any resources.
 *
 * @param dialog a HotkeyDialog instance.
 */
void
hotkey_dialog_destroy(HotkeyDialog *dialog)
{
	DEBUG("Destroying");

	gtk_widget_destroy(dialog->hotkey_dialog);
	g_free(dialog);
}

/**
 * Creates a new hotkey dialog.
 *
 * @param parent a GtkWindow to be used as the parent.
 * @param hotkey the hotkey concerned.
 * @return the newly created HotkeyDialog instance.
 */
HotkeyDialog *
hotkey_dialog_create(GtkWindow *parent, const gchar *hotkey)
{
	gchar *uifile;
	GtkBuilder *builder;
	HotkeyDialog *dialog;

	dialog = g_new0(HotkeyDialog, 1);

	/* Build UI file */
	uifile = get_ui_file(HOTKEY_DIALOG_UI_FILE);
	g_assert(uifile);

	DEBUG("Building from ui file '%s'", uifile);
	builder = gtk_builder_new_from_file(uifile);

	/* Save some widgets for later use */
	assign_gtk_widget(builder, dialog, hotkey_dialog);
	assign_gtk_widget(builder, dialog, instruction_label);
	assign_gtk_widget(builder, dialog, key_pressed_label);

	/* Configure some widgets */
	configure_hotkey_dialog(GTK_WINDOW(dialog->hotkey_dialog),
	                        GTK_LABEL(dialog->instruction_label),
	                        hotkey);

	/* Set transient parent */
	gtk_window_set_transient_for(GTK_WINDOW(dialog->hotkey_dialog), parent);

	/* Connect ui signal handlers */
	gtk_builder_connect_signals(builder, dialog);

	/* Cleanup */
	g_object_unref(builder);
	g_free(uifile);

	return dialog;
}
