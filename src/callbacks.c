/* callbacks.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file callbacks.c
 * This file holds callback functions for various signals
 * received by different GtkWidgets, some of them defined
 * in the gtk-builder glade files.
 * @brief gtk callbacks
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <alsa/asoundlib.h>
#include "alsa.h"
#include "callbacks.h"
#include "main.h"
#include "support.h"
#include "prefs.h"

int volume;
extern int volume;

/**
 * Callback function when the mute_check (GtkCheckButton) in the
 * volume popup window received the pressed signal
 * or when the mute_item (GtkImageMenuItem) in the right-click
 * menu received the activate signal.
 *
 * @param button the object that received the signal
 * @param event the GdkEvent which triggered this signal
 * @param user_data user data set when the signal handler was connected
 */
gboolean on_mute_clicked(GtkButton *button,
			 GdkEvent  *event,
			 gpointer  user_data) {

  setmute(popup_noti);
  get_mute_state(FALSE);
  return TRUE;
}

/**
 * Callback function when the vol_scale (GtkScale) in the volume
 * popup window received the change-value signal
 * (either via mouse or keyboard).
 *
 * @param range the GtkRange that received the signal
 * @param scroll the type of scroll action that was performed
 * @param value the new value resulting from the scroll action
 * @param user_data user data set when the signal handler was connected
 * @return TRUE to prevent other handlers from being invoked for the
 * signal, FALSE to propagate the signal further
 */
gboolean vol_scroll_event(GtkRange     *range,
			  GtkScrollType scroll,
			  gdouble       value,
			  gpointer      user_data) {
  GtkAdjustment *gtk_adj;
  int volumeset;

  /* We must ensure that the new value meets the requirement
   * defined by the GtkAdjustment. We have to do that manually,
   * because at this moment the value within GtkAdjustment
   * has not been updated yet, so using gtk_adjustment_get_value()
   * returns a wrong value.
   */
  gtk_adj = gtk_range_get_adjustment(range);
  if (value < gtk_adjustment_get_lower(gtk_adj)) {
    value = gtk_adjustment_get_lower(gtk_adj);
  }
  if (value > gtk_adjustment_get_upper(gtk_adj)) {
    value = gtk_adjustment_get_upper(gtk_adj);
  }

  volumeset = (int) value;

  setvol(volumeset,0,popup_noti);
  if (get_mute_state(TRUE) == 0) {
    setmute(popup_noti);
    get_mute_state(TRUE);
  }
  return FALSE;
}

/**
 * Callback function when the tray_icon receives the scroll-event
 * signal.
 *
 * @param status_icon the object which received the signal
 * @param event the GdkEventScroll which triggered this signal
 * @param user_data user data set when the signal handler was connected
 * @return TRUE to stop other handlers from being invoked for the event.
 * False to propagate the event further
 */
gboolean on_scroll(GtkStatusIcon *status_icon,
		GdkEventScroll *event,
		gpointer user_data) {
  int cv = getvol();
  if (event->direction == GDK_SCROLL_UP) {
    setvol(cv + scroll_step,1,mouse_noti);
  } else if (event->direction == GDK_SCROLL_DOWN) {
    setvol(cv - scroll_step,-1,mouse_noti);
  }

  if (get_mute_state(TRUE) == 0) {
    setmute(mouse_noti);
    get_mute_state(TRUE);
  }

  // this will set the slider value
  get_current_levels();
  return TRUE;
}

/**
 * Callback function when one of the hotkey boxes mute_eventbox,
 * up_eventbox or down_eventbox (GtkEventBox) in the
 * preferences received the button-press-event signal.
 *
 * @param widget the object which received the signal
 * @param event the GdkEventButton which triggered this signal
 * @param data struct holding the GtkWidgets of the preferences windows
 * @return TRUE to stop other handlers from being invoked for the event.
 * False to propagate the event further
 */
gboolean on_hotkey_button_click(GtkWidget *widget,
				GdkEventButton *event,
				PrefsData *data) {

  if (event->button ==1 &&
      event->type==GDK_2BUTTON_PRESS)
    acquire_hotkey(gtk_buildable_get_name(GTK_BUILDABLE(widget)),
		  data);
  return TRUE;
}

/**
 * Callback function when the ok_button (GtkButton) of the
 * preferences window received the clicked signal.
 *
 * @param button the object that received the signal
 * @param data struct holding the GtkWidgets of the preferences windows
 */
void on_ok_button_clicked(GtkButton *button,
			  PrefsData *data) {
  gsize len;
  GError *err = NULL;
  gint alsa_change = 0;

  // pull out various prefs

  // show vol text
  GtkWidget* vtc = data->vol_text_check;
  gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vtc));
  g_key_file_set_boolean(keyFile,"PNMixer","DisplayTextVolume",active);

  // vol pos
  GtkWidget* vpc = data->vol_pos_combo;
  gint idx = gtk_combo_box_get_active(GTK_COMBO_BOX(vpc));
  g_key_file_set_integer(keyFile,"PNMixer","TextVolumePosition",idx);

  // show vol meter
  GtkWidget* dvc = data->draw_vol_check;
  active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(dvc));
  g_key_file_set_boolean(keyFile,"PNMixer","DrawVolMeter",active);

  // vol meter pos
  GtkWidget* vmps = data->vol_meter_pos_spin;
  gint vmpos = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(vmps));
  g_key_file_set_integer(keyFile,"PNMixer","VolMeterPos",vmpos);

  GtkWidget* vcb = data->vol_meter_color_button;
#ifdef WITH_GTK3
  GdkRGBA color;
  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(vcb),&color);
  gdouble colints[3];
#else
  GdkColor color;
  gtk_color_button_get_color(GTK_COLOR_BUTTON(vcb),&color);
  gint colints[3];
#endif
  colints[0] = color.red;
  colints[1] = color.green;
  colints[2] = color.blue;

#ifdef WITH_GTK3
  g_key_file_set_double_list(keyFile,"PNMixer","VolMeterColor",colints,3);
#else
  g_key_file_set_integer_list(keyFile,"PNMixer","VolMeterColor",colints,3);
#endif

  // alsa card
  GtkWidget *acc = data->card_combo;
  gchar *old_card = get_selected_card();
  gchar *card = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(acc));
  if (old_card && strcmp(old_card,card))
      alsa_change = 1;
  g_key_file_set_string(keyFile,"PNMixer","AlsaCard",card);

  // channel
  GtkWidget *ccc = data->chan_combo;
  gchar* old_channel = NULL;
  if (old_card)
    old_channel = get_selected_channel(old_card);
  gchar* chan = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(ccc));
  if (old_channel) {
    if (strcmp(old_channel,chan))
      alsa_change = 1;
    g_free(old_card);
    g_free(old_channel);
  }
  g_key_file_set_string(keyFile,card,"Channel",chan);
  g_free(card);
  g_free(chan);

  // icon theme
  GtkWidget* icon_combo = data->icon_theme_combo;
  idx = gtk_combo_box_get_active (GTK_COMBO_BOX(icon_combo));
  if (idx == 0) { // internal theme
    g_key_file_remove_key(keyFile,"PNMixer","IconTheme",NULL);
  } else {
    gchar* theme_name = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT(icon_combo));
    if (theme_name) {
      g_key_file_set_string(keyFile,"PNMixer","IconTheme",theme_name);
      g_free(theme_name);
    }
  }

  // volume control command
  GtkWidget* ve = data->vol_control_entry;
  const gchar* vc = gtk_entry_get_text (GTK_ENTRY(ve));
  g_key_file_set_string(keyFile,"PNMixer","VolumeControlCommand",vc);

  // scroll step
  GtkWidget* sss = data->scroll_step_spin;
  gint spin = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sss));
  g_key_file_set_integer(keyFile,"PNMixer","MouseScrollStep",spin);

  // middle click
  GtkWidget* mcc = data->middle_click_combo;
  idx = gtk_combo_box_get_active(GTK_COMBO_BOX(mcc));
  g_key_file_set_integer(keyFile,"PNMixer","MiddleClickAction",idx);

  // custom command
  GtkWidget* ce = data->custom_entry;
  const gchar* cc = gtk_entry_get_text (GTK_ENTRY(ce));
  g_key_file_set_string(keyFile,"PNMixer","CustomCommand",cc);

  // normalize volume
  GtkWidget* vnorm = data->normalize_vol_check;
  gboolean is_pressed;
  is_pressed = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vnorm));
  g_key_file_set_boolean(keyFile,"PNMixer","NormalizeVolume",is_pressed);

  // hotkey enable
  GtkWidget* hkc = data->enable_hotkeys_check;
  active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hkc));
  g_key_file_set_boolean(keyFile,"PNMixer","EnableHotKeys",active);

  // scroll step
  GtkWidget* hs = data->hotkey_vol_spin;
  gint hotstep = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(hs));
  g_key_file_set_integer(keyFile,"PNMixer","HotkeyVolumeStep",hotstep);

  // hotkeys
  guint keysym;
  gint keycode;
  GdkModifierType mods;
  GtkWidget *kl = data->mute_hotkey_label;
  gtk_accelerator_parse(gtk_label_get_text(GTK_LABEL(kl)),&keysym,&mods);
  if (keysym != 0)
    keycode = XKeysymToKeycode(gdk_x11_get_default_xdisplay(),keysym);
  else
    keycode = -1;
  g_key_file_set_integer(keyFile,"PNMixer","VolMuteKey",keycode);
  g_key_file_set_integer(keyFile,"PNMixer","VolMuteMods",mods);

  kl = data->up_hotkey_label;
  gtk_accelerator_parse(gtk_label_get_text(GTK_LABEL(kl)),&keysym,&mods);
  if (keysym != 0)
    keycode = XKeysymToKeycode(gdk_x11_get_default_xdisplay(),keysym);
  else
    keycode = -1;
  g_key_file_set_integer(keyFile,"PNMixer","VolUpKey",keycode);
  g_key_file_set_integer(keyFile,"PNMixer","VolUpMods",mods);

  kl = data->down_hotkey_label;
  gtk_accelerator_parse(gtk_label_get_text(GTK_LABEL(kl)),&keysym,&mods);
  if (keysym != 0)
    keycode = XKeysymToKeycode(gdk_x11_get_default_xdisplay(),keysym);
  else
    keycode = -1;
  g_key_file_set_integer(keyFile,"PNMixer","VolDownKey",keycode);
  g_key_file_set_integer(keyFile,"PNMixer","VolDownMods",mods);

#ifdef HAVE_LIBN
  // notification prefs
  GtkWidget* nc = data->enable_noti_check;
  gint noti_spin;
  active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nc));
  g_key_file_set_boolean(keyFile,"PNMixer","EnableNotifications",active);

  nc = data->hotkey_noti_check;
  active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nc));
  g_key_file_set_boolean(keyFile,"PNMixer","HotkeyNotifications",active);

  nc = data->mouse_noti_check;
  active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nc));
  g_key_file_set_boolean(keyFile,"PNMixer","MouseNotifications",active);

  nc = data->popup_noti_check;
  active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nc));
  g_key_file_set_boolean(keyFile,"PNMixer","PopupNotifications",active);

  nc = data->external_noti_check;
  active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(nc));
  g_key_file_set_boolean(keyFile,"PNMixer","ExternalNotifications",active);

  nc = data->noti_timeout_spin;
  noti_spin = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(nc));
  g_key_file_set_integer(keyFile,"PNMixer","NotificationTimeout",noti_spin);
#endif

  gchar* filename = g_strconcat(g_get_user_config_dir(), "/pnmixer/config", NULL);
  gchar* filedata = g_key_file_to_data(keyFile,&len,NULL);
  g_file_set_contents(filename,filedata,len,&err);
  if (err != NULL) {
    report_error(_("Couldn't write preferences file: %s\n"), err->message);
    g_error_free (err);
  } else
    apply_prefs(alsa_change);
  g_free(filename);
  gtk_widget_destroy(data->prefs_window);
  g_slice_free(PrefsData,data);
}

/**
 * Callback function when the cancel_button (GtkButton) of the
 * preferences window received the clicked signal.
 *
 * @param button the object that received the signal
 * @param data struct holding the GtkWidgets of the preferences windows
 */
void on_cancel_button_clicked(GtkButton *button,
			      PrefsData *data) {
  gtk_widget_destroy(data->prefs_window);
  g_slice_free(PrefsData,data);
}

/**
 * Callback function when a key is hit in prefs_window. Currently handles
 * Esc key (calls on_cancel_button_clicked())
 * and
 * Return key (calls on_ok_button_clicked()).
 *
 * @param widget the widget that received the signal
 * @param event the key event that was triggered
 * @param data struct holding the GtkWidgets of the preferences windows
 * @return TRUE to stop other handlers from being invoked for the event.
 * False to propagate the event further
 */
gboolean on_key_press(GtkWidget *widget,
		GdkEventKey *event,
		PrefsData *data) {

	switch(event->keyval) {
		case GDK_KEY_Escape:
			on_cancel_button_clicked(NULL, data);
			break;
		case GDK_KEY_Return:
			on_ok_button_clicked(NULL, data);
			break;
		default:
			return FALSE;
	}
	return TRUE;
}
