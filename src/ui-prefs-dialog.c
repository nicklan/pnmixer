/* ui-prefs-dialog.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file ui-prefs-dialog.c
 * This file holds the ui-related code for the preferences window.
 * @brief Preferences dialog ui subsystem.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkx.h>
#ifndef WITH_GTK3
#include <gdk/gdkkeysyms.h>
#endif

#include "audio.h"
#include "prefs.h"
#include "hotkey.h"
#include "hotkeys.h"
#include "support-log.h"
#include "support-intl.h"
#include "support-ui.h"
#include "ui-prefs-dialog.h"
#include "ui-hotkey-dialog.h"

#include "main.h"

#ifdef WITH_GTK3
#define PREFS_UI_FILE "prefs-dialog-gtk3.glade"
#else
#define PREFS_UI_FILE "prefs-dialog-gtk2.glade"
#endif

/* Helpers */

/* Gets one of the hotkey code and mode in the Hotkeys settings
 * from the specified label (parsed as an accelerator name).
 */
static void
get_keycode_for_label(GtkLabel *label, gint *code, GdkModifierType *mods)
{
	const gchar *key_accel;

	key_accel = gtk_label_get_text(label);
	hotkey_accel_to_code(key_accel, code, mods);
}

/* Sets one of the hotkey labels in the Hotkeys settings
 * to the specified keycode (converted to a accelerator name).
 */
static void
set_label_for_keycode(GtkLabel *label, gint code, GdkModifierType mods)
{
	gchar *key_accel;

	if (code < 0)
		return;

	key_accel = hotkey_code_to_accel(code, mods);
	gtk_label_set_text(label, key_accel);
	g_free(key_accel);
}

/**
 * Fills the GtkComboBoxText 'chan_combo' with the currently available channels
 * for a given card.
 * The active channel in the combo box is set to the SELECTED channel found in
 * preferences for this card.
 *
 * @param combo the GtkComboBoxText widget for the channels.
 * @param card_name the card to use to get the channels list.
 */
static void
fill_chan_combo(GtkComboBoxText *combo, const gchar *card_name)
{
	int idx, sidx;
	gchar *selected_channel;
	GSList *channel_list, *item;

	DEBUG("Filling channels ComboBox for card '%s'", card_name);

	selected_channel = prefs_get_channel(card_name);
	channel_list = audio_get_channel_list(card_name);

	/* Empty the combo box */
	gtk_combo_box_text_remove_all(combo);

	/* Fill the combo box with the channels, save the selected channel index */
	for (sidx = idx = 0, item = channel_list; item; idx++, item = item->next) {
		const char *channel_name = item->data;
		gtk_combo_box_text_append_text(combo, channel_name);

		if (!g_strcmp0(channel_name, selected_channel))
			sidx = idx;
	}

	/* Set the combo box active item */
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), sidx);

	/* Cleanup */
	g_slist_free_full(channel_list, g_free);
	g_free(selected_channel);
}

/**
 * Fills the GtkComboBoxText 'card_combo' with the currently available cards.
 * The active card in the combo box is set to the currently ACTIVE card,
 * which may be different from the SELECTED card found in preferences.
 *
 * @param combo the GtkComboBoxText widget for the cards.
 * @param audio an Audio instance.
 */
static void
fill_card_combo(GtkComboBoxText *combo, Audio *audio)
{
	int idx, sidx;
	const gchar *active_card;
	GSList *card_list, *item;

	DEBUG("Filling cards ComboBox");

	active_card = audio_get_card(audio);
	card_list = audio_get_card_list();

	/* Empty the combo box */
	gtk_combo_box_text_remove_all(combo);

	/* Fill the combo box with the cards, save the active card index */
	for (sidx = idx = 0, item = card_list; item; idx++, item = item->next) {
		const char *card_name = item->data;
		gtk_combo_box_text_append_text(combo, card_name);

		if (!g_strcmp0(card_name, active_card))
			sidx = idx;
	}

	/* Set the combo box active item */
	gtk_combo_box_set_active(GTK_COMBO_BOX(combo), sidx);

	/* Cleanup */
	g_slist_free_full(card_list, g_free);
}

/* Public functions & signals handlers */

struct prefs_dialog {
	/* Audio system
	 * We need it to display card/channel lists, and also to be notified
	 * if something interesting happens (card disappearing, for example).
	 */
	Audio *audio;
	/* Hotkeys system
	 * When assigning hotkeys, we must unbind the hotkeys first.
	 * Otherwise the currently assigned keys are intercepted
	 * and can't be used.
	 */
	Hotkeys *hotkeys;
	HotkeyDialog *hotkey_dialog;
	/* Top-level widgets */
	GtkWidget *prefs_dialog;
	GtkWidget *notebook;
	GtkWidget *ok_button;
	GtkWidget *cancel_button;
	/* View panel */
	GtkWidget *vol_orientation_combo;
	GtkWidget *vol_text_check;
	GtkWidget *vol_pos_label;
	GtkWidget *vol_pos_combo;
	GtkWidget *vol_meter_draw_check;
	GtkWidget *vol_meter_pos_label;
	GtkWidget *vol_meter_pos_spin;
	GtkAdjustment *vol_meter_pos_adjustment;
	GtkWidget *vol_meter_color_label;
	GtkWidget *vol_meter_color_button;
	GtkWidget *system_theme;
	/* Device panel */
	GtkWidget *card_combo;
	GtkWidget *chan_combo;
	GtkWidget *normalize_vol_check;
	/* Behavior panel */
	GtkWidget *vol_control_entry;
	GtkWidget *scroll_step_spin;
	GtkWidget *fine_scroll_step_spin;
	GtkWidget *middle_click_combo;
	GtkWidget *custom_label;
	GtkWidget *custom_entry;
	/* Hotkeys panel */
	GtkWidget *hotkeys_enable_check;
	GtkWidget *hotkeys_mute_eventbox;
	GtkWidget *hotkeys_mute_label;
	GtkWidget *hotkeys_up_eventbox;
	GtkWidget *hotkeys_up_label;
	GtkWidget *hotkeys_down_eventbox;
	GtkWidget *hotkeys_down_label;
	/* Notifications panel */
#ifdef HAVE_LIBN
	GtkWidget *noti_vbox_enabled;
	GtkWidget *noti_enable_check;
	GtkWidget *noti_timeout_label;
	GtkWidget *noti_timeout_spin;
	GtkWidget *noti_hotkey_check;
	GtkWidget *noti_mouse_check;
	GtkWidget *noti_popup_check;
	GtkWidget *noti_ext_check;
#else
	GtkWidget *noti_vbox_disabled;
#endif
};

/**
 * Handles the 'toggled' signal on the GtkCheckButton 'vol_text_check'.
 * Updates the preferences dialog.
 *
 * @param button the button which received the signal.
 * @param dialog user data set when the signal handler was connected.
 */
void
on_vol_text_check_toggled(GtkToggleButton *button, PrefsDialog *dialog)
{
	gboolean active = gtk_toggle_button_get_active(button);
	gtk_widget_set_sensitive(dialog->vol_pos_label, active);
	gtk_widget_set_sensitive(dialog->vol_pos_combo, active);
}

/**
 * Handles the 'toggled' signal on the GtkCheckButton 'vol_meter_draw_check'.
 * Updates the preferences dialog.
 *
 * @param button the button which received the signal.
 * @param dialog user data set when the signal handler was connected.
 */
void
on_vol_meter_draw_check_toggled(GtkToggleButton *button, PrefsDialog *dialog)
{
	gboolean active = gtk_toggle_button_get_active(button);
	gtk_widget_set_sensitive(dialog->vol_meter_pos_label, active);
	gtk_widget_set_sensitive(dialog->vol_meter_pos_spin, active);
	gtk_widget_set_sensitive(dialog->vol_meter_color_label, active);
	gtk_widget_set_sensitive(dialog->vol_meter_color_button, active);
}

/**
 * Handles the 'changed' signal on the GtkComboBoxText 'card_combo'.
 * This basically refills the channel list if the card changes.
 *
 * @param box the box which received the signal.
 * @param dialog user data set when the signal handler was connected.
 */
void
on_card_combo_changed(GtkComboBoxText *box, PrefsDialog *dialog)
{
	gchar *card_name;

	card_name = gtk_combo_box_text_get_active_text(box);
	fill_chan_combo(GTK_COMBO_BOX_TEXT(dialog->chan_combo), card_name);
	g_free(card_name);
}

/**
 * Handles the 'changed' signal on the GtkComboBoxText 'middle_click_combo'.
 * Updates the preferences dialog.
 *
 * @param box the combobox which received the signal.
 * @param dialog user data set when the signal handler was connected.
 */
void
on_middle_click_combo_changed(GtkComboBoxText *box, PrefsDialog *dialog)
{
	gboolean cust = gtk_combo_box_get_active(GTK_COMBO_BOX(box)) == 3;
	gtk_widget_set_sensitive(dialog->custom_label, cust);
	gtk_widget_set_sensitive(dialog->custom_entry, cust);
}

/**
 * Handles the 'toggled' signal on the GtkCheckButton 'hotkeys_enable_check'.
 * Updates the preferences dialog.
 *
 * @param button the button which received the signal.
 * @param dialog user data set when the signal handler was connected.
 */
void
on_hotkeys_enable_check_toggled(GtkToggleButton *button, PrefsDialog *dialog)
{
	gboolean active = gtk_toggle_button_get_active(button);
	(void) active;
}

/**
 * Handles 'button-press-event' signal on one of the GtkEventBoxes used to
 * define a hotkey: 'hotkeys_mute/up/down_eventbox'.
 * Runs a dialog dialog where user can define a new hotkey.
 * User should double-click on the event box to define a new hotkey.
 *
 * @param widget the object which received the signal.
 * @param event the GdkEventButton which triggered this signal.
 * @param dialog user data set when the signal handler was connected.
 * @return TRUE to stop other handlers from being invoked for the event.
 * FALSE to propagate the event further.
 */
gboolean
on_hotkey_event_box_button_press_event(GtkWidget *widget, GdkEventButton *event,
                                       PrefsDialog *dialog)
{
	const gchar *hotkey;
	GtkLabel *hotkey_label;
	gchar *key_pressed;

	/* We want a left-click */
	if (event->button != 1)
		return FALSE;

	/* We want it to be double-click */
	if (event->type != GDK_2BUTTON_PRESS)
		return FALSE;

	/* Let's check which eventbox was clicked */
	hotkey = NULL;
	if (widget == dialog->hotkeys_mute_eventbox) {
		hotkey_label = GTK_LABEL(dialog->hotkeys_mute_label);
		hotkey = _("Mute/Unmute");
	} else if (widget == dialog->hotkeys_up_eventbox) {
		hotkey_label = GTK_LABEL(dialog->hotkeys_up_label);
		hotkey = _("Volume Up");
	} else if (widget == dialog->hotkeys_down_eventbox) {
		hotkey_label = GTK_LABEL(dialog->hotkeys_down_label);
		hotkey = _("Volume Down");
	}
	g_assert(hotkey);

	/* Ensure there's no dialog already running */
	if (dialog->hotkey_dialog)
		return FALSE;

	/* Unbind hotkeys */
	hotkeys_unbind(dialog->hotkeys);

	/* Run the hotkey dialog */
	dialog->hotkey_dialog = hotkey_dialog_create
	                        (GTK_WINDOW(dialog->prefs_dialog), hotkey);
	key_pressed = hotkey_dialog_run(dialog->hotkey_dialog);
	hotkey_dialog_destroy(dialog->hotkey_dialog);
	dialog->hotkey_dialog = NULL;

	/* Bind hotkeys */
	hotkeys_bind(dialog->hotkeys);

	/* Check the response */
	if (key_pressed == NULL)
		return FALSE;

	/* <Primary>c is used to disable the hotkey */
	if (!g_ascii_strcasecmp(key_pressed, "<Primary>c")) {
		g_free(key_pressed);
		key_pressed = g_strdup_printf("(%s)", _("None"));
	}

	/* Set */
	gtk_label_set_text(hotkey_label, key_pressed);
	g_free(key_pressed);

	return FALSE;
}

/**
 * Handles the 'toggled' signal on the GtkCheckButton 'noti_enable_check'.
 * Updates the preferences dialog.
 *
 * @param button the button which received the signal.
 * @param dialog user data set when the signal handler was connected.
 */
#ifdef HAVE_LIBN
void
on_noti_enable_check_toggled(GtkToggleButton *button, PrefsDialog *dialog)
{
	gboolean active = gtk_toggle_button_get_active(button);
	gtk_widget_set_sensitive(dialog->noti_timeout_label, active);
	gtk_widget_set_sensitive(dialog->noti_timeout_spin, active);
	gtk_widget_set_sensitive(dialog->noti_hotkey_check, active);
	gtk_widget_set_sensitive(dialog->noti_mouse_check, active);
	gtk_widget_set_sensitive(dialog->noti_popup_check, active);
	gtk_widget_set_sensitive(dialog->noti_ext_check, active);
}
#else
void
on_noti_enable_check_toggled(G_GNUC_UNUSED GtkToggleButton *button,
                             G_GNUC_UNUSED PrefsDialog *dialog)
{
}
#endif

/**
 * Handle signals from the audio subsystem.
 *
 * @param audio the Audio instance that emitted the signal.
 * @param event the AudioEvent containing useful information.
 * @param data user supplied data.
 */
static void
on_audio_changed(Audio *audio, AudioEvent *event, gpointer data)
{
	PrefsDialog *dialog = (PrefsDialog *) data;
	GtkComboBoxText *card_combo = GTK_COMBO_BOX_TEXT(dialog->card_combo);
	GtkComboBoxText *chan_combo = GTK_COMBO_BOX_TEXT(dialog->chan_combo);

	switch (event->signal) {
	case AUDIO_CARD_INITIALIZED:
	case AUDIO_CARD_CLEANED_UP:
		/* A card may have appeared or disappeared */
		fill_card_combo(card_combo, audio);
		fill_chan_combo(chan_combo, audio_get_card(audio));
		break;
	default:
		break;
	}
}

/**
 * Retrieve the preferences dialog values and assign then to preferences.
 * @param dialog struct holding the GtkWidgets of the preferences dialog.
 */
void
prefs_dialog_retrieve(PrefsDialog *dialog)
{
	DEBUG("Retrieving prefs dialog values");

	// volume slider orientation
	GtkWidget *soc = dialog->vol_orientation_combo;
	const gchar *orientation;
#ifdef WITH_GTK3
	orientation = gtk_combo_box_get_active_id(GTK_COMBO_BOX(soc));
#else
	/* Gtk2 ComboBoxes don't have item ids */
	orientation = "vertical";
	if (gtk_combo_box_get_active(GTK_COMBO_BOX(soc)) == 1)
		orientation = "horizontal";
#endif
	prefs_set_string("SliderOrientation", orientation);

	// volume text display
	GtkWidget *vtc = dialog->vol_text_check;
	gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vtc));
	prefs_set_boolean("DisplayTextVolume", active);

	// volume text position
	GtkWidget *vpc = dialog->vol_pos_combo;
	gint idx = gtk_combo_box_get_active(GTK_COMBO_BOX(vpc));
	prefs_set_integer("TextVolumePosition", idx);

	// volume meter display
	GtkWidget *dvc = dialog->vol_meter_draw_check;
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dvc));
	prefs_set_boolean("DrawVolMeter", active);

	// volume meter positon
	GtkWidget *vmps = dialog->vol_meter_pos_spin;
	gint vmpos = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(vmps));
	prefs_set_integer("VolMeterPos", vmpos);

	// volume meter colors
	GtkWidget *vcb = dialog->vol_meter_color_button;
	gdouble colors[3];
#ifdef WITH_GTK3
	GdkRGBA color;
	gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(vcb), &color);
	colors[0] = color.red;
	colors[1] = color.green;
	colors[2] = color.blue;
#else
	GdkColor color;
	gtk_color_button_get_color(GTK_COLOR_BUTTON(vcb), &color);
	colors[0] = (gdouble) color.red / 65536;
	colors[1] = (gdouble) color.green / 65536;
	colors[2] = (gdouble) color.blue / 65536;
#endif
	prefs_set_double_list("VolMeterColor", colors, 3);

	// icon theme
	GtkWidget *system_theme = dialog->system_theme;
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(system_theme));
	prefs_set_boolean("SystemTheme", active);

	// audio card
	GtkWidget *acc = dialog->card_combo;
	gchar *card = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(acc));
	prefs_set_string("AlsaCard", card);

	// audio channel
	GtkWidget *ccc = dialog->chan_combo;
	gchar *chan = gtk_combo_box_text_get_active_text(GTK_COMBO_BOX_TEXT(ccc));
	prefs_set_channel(card, chan);
	g_free(card);
	g_free(chan);

	// normalize volume
	GtkWidget *vnorm = dialog->normalize_vol_check;
	gboolean is_pressed;
	is_pressed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vnorm));
	prefs_set_boolean("NormalizeVolume", is_pressed);

	// volume control command
	GtkWidget *ve = dialog->vol_control_entry;
	const gchar *vc = gtk_entry_get_text(GTK_ENTRY(ve));
	prefs_set_string("VolumeControlCommand", vc);

	// volume scroll steps
	GtkWidget *sss = dialog->scroll_step_spin;
	gdouble step = gtk_spin_button_get_value(GTK_SPIN_BUTTON(sss));
	prefs_set_double("ScrollStep", step);

	GtkWidget *fsss = dialog->fine_scroll_step_spin;
	gdouble fine_step = gtk_spin_button_get_value(GTK_SPIN_BUTTON(fsss));
	prefs_set_double("FineScrollStep", fine_step);

	// middle click
	GtkWidget *mcc = dialog->middle_click_combo;
	idx = gtk_combo_box_get_active(GTK_COMBO_BOX(mcc));
	prefs_set_integer("MiddleClickAction", idx);

	// custom command
	GtkWidget *ce = dialog->custom_entry;
	const gchar *cc = gtk_entry_get_text(GTK_ENTRY(ce));
	prefs_set_string("CustomCommand", cc);

	// hotkeys enabled
	GtkWidget *hkc = dialog->hotkeys_enable_check;
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hkc));
	prefs_set_boolean("EnableHotKeys", active);

	// hotkeys
	GtkWidget *kl;
	gint keycode;
	GdkModifierType mods;

	kl = dialog->hotkeys_mute_label;
	get_keycode_for_label(GTK_LABEL(kl), &keycode, &mods);
	prefs_set_integer("VolMuteKey", keycode);
	prefs_set_integer("VolMuteMods", mods);

	kl = dialog->hotkeys_up_label;
	get_keycode_for_label(GTK_LABEL(kl), &keycode, &mods);
	prefs_set_integer("VolUpKey", keycode);
	prefs_set_integer("VolUpMods", mods);

	kl = dialog->hotkeys_down_label;
	get_keycode_for_label(GTK_LABEL(kl), &keycode, &mods);
	prefs_set_integer("VolDownKey", keycode);
	prefs_set_integer("VolDownMods", mods);

	// notifications
#ifdef HAVE_LIBN
	GtkWidget *nc = dialog->noti_enable_check;
	gint noti_spin;
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nc));
	prefs_set_boolean("EnableNotifications", active);

	nc = dialog->noti_hotkey_check;
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nc));
	prefs_set_boolean("HotkeyNotifications", active);

	nc = dialog->noti_mouse_check;
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nc));
	prefs_set_boolean("MouseNotifications", active);

	nc = dialog->noti_popup_check;
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nc));
	prefs_set_boolean("PopupNotifications", active);

	nc = dialog->noti_ext_check;
	active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nc));
	prefs_set_boolean("ExternalNotifications", active);

	nc = dialog->noti_timeout_spin;
	noti_spin = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(nc));
	prefs_set_integer("NotificationTimeout", noti_spin);
#endif
}

/**
 * Set the preferences dialog values according to current preferences.
 * @param dialog struct holding the GtkWidgets of the preferences dialog.
 */
void
prefs_dialog_populate(PrefsDialog *dialog)
{
	gdouble *vol_meter_clrs;
	gchar *slider_orientation, *vol_cmd, *custcmd;

	DEBUG("Populating prefs dialog values");

	// volume slider orientation
	slider_orientation = prefs_get_string("SliderOrientation", NULL);
	if (slider_orientation) {
		GtkComboBox *combo_box =
		        GTK_COMBO_BOX(dialog->vol_orientation_combo);
#ifndef WITH_GTK3
		/* Gtk2 ComboBoxes don't have item ids */
		if (!strcmp(slider_orientation, "horizontal"))
			gtk_combo_box_set_active(combo_box, 1);
		else
			gtk_combo_box_set_active(combo_box, 0);
#else
		gtk_combo_box_set_active_id(combo_box, slider_orientation);
#endif
		g_free(slider_orientation);
	}

	// volume text display
	gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON(dialog->vol_text_check),
	 prefs_get_boolean("DisplayTextVolume", FALSE));

	on_vol_text_check_toggled
	(GTK_TOGGLE_BUTTON(dialog->vol_text_check), dialog);

	// volume text position
	gtk_combo_box_set_active
	(GTK_COMBO_BOX(dialog->vol_pos_combo),
	 prefs_get_integer("TextVolumePosition", 0));

	// volume meter display
	gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON(dialog->vol_meter_draw_check),
	 prefs_get_boolean("DrawVolMeter", FALSE));

	on_vol_meter_draw_check_toggled
	(GTK_TOGGLE_BUTTON(dialog->vol_meter_draw_check), dialog);

	// volume meter position
	gtk_spin_button_set_value
	(GTK_SPIN_BUTTON(dialog->vol_meter_pos_spin),
	 prefs_get_integer("VolMeterPos", 0));

	// volume meter colors
	vol_meter_clrs = prefs_get_double_list("VolMeterColor", NULL);
#ifdef WITH_GTK3
	GdkRGBA vol_meter_color_button_color;
	vol_meter_color_button_color.red = vol_meter_clrs[0];
	vol_meter_color_button_color.green = vol_meter_clrs[1];
	vol_meter_color_button_color.blue = vol_meter_clrs[2];
	vol_meter_color_button_color.alpha = 1.0;
	gtk_color_chooser_set_rgba
	(GTK_COLOR_CHOOSER(dialog->vol_meter_color_button),
	 &vol_meter_color_button_color);
#else
	GdkColor vol_meter_color_button_color;
	vol_meter_color_button_color.red = (guint32) (vol_meter_clrs[0] * 65536);
	vol_meter_color_button_color.green = (guint32) (vol_meter_clrs[1] * 65536);
	vol_meter_color_button_color.blue = (guint32) (vol_meter_clrs[2] * 65536);
	gtk_color_button_set_color
	(GTK_COLOR_BUTTON(dialog->vol_meter_color_button),
	 &vol_meter_color_button_color);
#endif
	g_free(vol_meter_clrs);

	// icon theme
	gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON(dialog->system_theme),
	 prefs_get_boolean("SystemTheme", FALSE));

	// fill in card & channel combo boxes
	fill_card_combo(GTK_COMBO_BOX_TEXT(dialog->card_combo), dialog->audio);
#ifdef GTK3
	/* On Gtk3, refilling the card combo doesn't emit a 'changed' signal,
	 * therefore we must refill channel combo explicitely.
	 */
	fill_chan_combo(GTK_COMBO_BOX_TEXT(dialog->chan_combo),
	                audio_get_card(dialog->audio));
#endif

	// normalize volume
	gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON(dialog->normalize_vol_check),
	 prefs_get_boolean("NormalizeVolume", FALSE));

	// volume control command
	vol_cmd = prefs_get_string("VolumeControlCommand", NULL);
	if (vol_cmd) {
		gtk_entry_set_text(GTK_ENTRY(dialog->vol_control_entry), vol_cmd);
		g_free(vol_cmd);
	}

	// volume scroll steps
	gtk_spin_button_set_value
	(GTK_SPIN_BUTTON(dialog->scroll_step_spin),
	 prefs_get_double("ScrollStep", 5));

	gtk_spin_button_set_value
	(GTK_SPIN_BUTTON(dialog->fine_scroll_step_spin),
	 prefs_get_double("FineScrollStep", 1));

	//  middle click
	gtk_combo_box_set_active
	(GTK_COMBO_BOX(dialog->middle_click_combo),
	 prefs_get_integer("MiddleClickAction", 0));

	on_middle_click_combo_changed
	(GTK_COMBO_BOX_TEXT(dialog->middle_click_combo), dialog);

	// custom command
	gtk_entry_set_invisible_char(GTK_ENTRY(dialog->custom_entry), 8226);

	custcmd = prefs_get_string("CustomCommand", NULL);
	if (custcmd) {
		gtk_entry_set_text(GTK_ENTRY(dialog->custom_entry), custcmd);
		g_free(custcmd);
	}

	// hotkeys enabled
	gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON(dialog->hotkeys_enable_check),
	 prefs_get_boolean("EnableHotKeys", FALSE));

	// hotkeys
	set_label_for_keycode(GTK_LABEL(dialog->hotkeys_mute_label),
	                      prefs_get_integer("VolMuteKey", -1),
	                      prefs_get_integer("VolMuteMods", 0));

	set_label_for_keycode(GTK_LABEL(dialog->hotkeys_up_label),
	                      prefs_get_integer("VolUpKey", -1),
	                      prefs_get_integer("VolUpMods", 0));

	set_label_for_keycode(GTK_LABEL(dialog->hotkeys_down_label),
	                      prefs_get_integer("VolDownKey", -1),
	                      prefs_get_integer("VolDownMods", 0));

	on_hotkeys_enable_check_toggled
	(GTK_TOGGLE_BUTTON(dialog->hotkeys_enable_check), dialog);

	// notifications
#ifdef HAVE_LIBN
	gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON(dialog->noti_enable_check),
	 prefs_get_boolean("EnableNotifications", FALSE));

	gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON(dialog->noti_hotkey_check),
	 prefs_get_boolean("HotkeyNotifications", TRUE));

	gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON(dialog->noti_mouse_check),
	 prefs_get_boolean("MouseNotifications", TRUE));

	gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON(dialog->noti_popup_check),
	 prefs_get_boolean("PopupNotifications", FALSE));

	gtk_toggle_button_set_active
	(GTK_TOGGLE_BUTTON(dialog->noti_ext_check),
	 prefs_get_boolean("ExternalNotifications", FALSE));

	gtk_spin_button_set_value
	(GTK_SPIN_BUTTON(dialog->noti_timeout_spin),
	 prefs_get_integer("NotificationTimeout", 1500));

	on_noti_enable_check_toggled
	(GTK_TOGGLE_BUTTON(dialog->noti_enable_check), dialog);
#endif
}

/**
 * Runs the preferences dialog.
 *
 * @param dialog a PrefsDialog instance.
 * @return a GtkResponse code.
 */
gint
prefs_dialog_run(PrefsDialog *dialog)
{
	GtkDialog *prefs_dialog = GTK_DIALOG(dialog->prefs_dialog);
	gint resp;

	resp = gtk_dialog_run(prefs_dialog);

	return resp;
}

/**
 * Destroys the preferences dialog, freeing any resources.
 *
 * @param dialog a PrefsDialog instance.
 */
void
prefs_dialog_destroy(PrefsDialog *dialog)
{
	audio_signals_disconnect(dialog->audio, on_audio_changed, dialog);

	if (dialog->hotkey_dialog)
		hotkey_dialog_destroy(dialog->hotkey_dialog);

	gtk_widget_destroy(dialog->prefs_dialog);
	g_free(dialog);
}

/**
 * Creates the preferences dialog.
 *
 * @param parent a GtkWindow to be used as the parent.
 * @param audio pointer to this audio subsystem.
 * @param hotkeys pointer to this hotkey subsystem.
 * @return the newly created PrefsDialog instance.
 */
PrefsDialog *
prefs_dialog_create(GtkWindow *parent, Audio *audio, Hotkeys *hotkeys)
{
	gchar *uifile = NULL;
	GtkBuilder *builder = NULL;
	PrefsDialog *dialog;

	dialog = g_new0(PrefsDialog, 1);

	/* Build UI file */
	uifile = get_ui_file(PREFS_UI_FILE);
	g_assert(uifile);

	DEBUG("Building from ui file '%s'", uifile);
	builder = gtk_builder_new_from_file(uifile);

	/* Append the notification page.
	 * This has to be done manually here, in the C code,
	 * because notifications support is optional at build time.
	 */
	gtk_notebook_append_page
	(GTK_NOTEBOOK(gtk_builder_get_object(builder, "notebook")),
#ifdef HAVE_LIBN
	 GTK_WIDGET(gtk_builder_get_object(builder, "noti_vbox_enabled")),
#else
	 GTK_WIDGET(gtk_builder_get_object(builder, "noti_vbox_disabled")),
#endif
	 gtk_label_new(_("Notifications")));

	/* Save some widgets for later use */
	// Top level widgets
	assign_gtk_widget(builder, dialog, prefs_dialog);
	assign_gtk_widget(builder, dialog, notebook);
	assign_gtk_widget(builder, dialog, ok_button);
	assign_gtk_widget(builder, dialog, cancel_button);
	// View panel
	assign_gtk_widget(builder, dialog, vol_orientation_combo);
	assign_gtk_widget(builder, dialog, vol_text_check);
	assign_gtk_widget(builder, dialog, vol_pos_label);
	assign_gtk_widget(builder, dialog, vol_pos_combo);
	assign_gtk_widget(builder, dialog, vol_meter_draw_check);
	assign_gtk_widget(builder, dialog, vol_meter_pos_label);
	assign_gtk_widget(builder, dialog, vol_meter_pos_spin);
	assign_gtk_adjustment(builder, dialog, vol_meter_pos_adjustment);
	assign_gtk_widget(builder, dialog, vol_meter_color_label);
	assign_gtk_widget(builder, dialog, vol_meter_color_button);
	assign_gtk_widget(builder, dialog, system_theme);
	// Device panel
	assign_gtk_widget(builder, dialog, card_combo);
	assign_gtk_widget(builder, dialog, chan_combo);
	assign_gtk_widget(builder, dialog, normalize_vol_check);
	// Behavior panel
	assign_gtk_widget(builder, dialog, vol_control_entry);
	assign_gtk_widget(builder, dialog, scroll_step_spin);
	assign_gtk_widget(builder, dialog, fine_scroll_step_spin);
	assign_gtk_widget(builder, dialog, middle_click_combo);
	assign_gtk_widget(builder, dialog, custom_label);
	assign_gtk_widget(builder, dialog, custom_entry);
	// Hotkeys panel
	assign_gtk_widget(builder, dialog, hotkeys_enable_check);
	assign_gtk_widget(builder, dialog, hotkeys_mute_eventbox);
	assign_gtk_widget(builder, dialog, hotkeys_mute_label);
	assign_gtk_widget(builder, dialog, hotkeys_up_eventbox);
	assign_gtk_widget(builder, dialog, hotkeys_up_label);
	assign_gtk_widget(builder, dialog, hotkeys_down_eventbox);
	assign_gtk_widget(builder, dialog, hotkeys_down_label);
	// Notifications panel
#ifdef HAVE_LIBN
	assign_gtk_widget(builder, dialog, noti_vbox_enabled);
	assign_gtk_widget(builder, dialog, noti_enable_check);
	assign_gtk_widget(builder, dialog, noti_timeout_spin);
	assign_gtk_widget(builder, dialog, noti_timeout_label);
	assign_gtk_widget(builder, dialog, noti_hotkey_check);
	assign_gtk_widget(builder, dialog, noti_mouse_check);
	assign_gtk_widget(builder, dialog, noti_popup_check);
	assign_gtk_widget(builder, dialog, noti_ext_check);
#else
	assign_gtk_widget(builder, dialog, noti_vbox_disabled);
#endif

	/* Set transient parent */
	gtk_window_set_transient_for(GTK_WINDOW(dialog->prefs_dialog), parent);

	/* Connect ui signal handlers */
	gtk_builder_connect_signals(builder, dialog);

	/* Save some references */
	dialog->hotkeys = hotkeys;
	dialog->audio = audio;

	/* Connect audio signal handlers */
	audio_signals_connect(audio, on_audio_changed, dialog);

	/* Cleanup */
	g_object_unref(G_OBJECT(builder));
	g_free(uifile);

	return dialog;
}
