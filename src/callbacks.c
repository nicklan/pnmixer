/* callbacks.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <alsa/asoundlib.h>
#include "alsa.h"
#include "callbacks.h"
#include "main.h"
#include "support.h"
#include "prefs.h"

GtkWidget *window1;
GtkWidget *checkbutton1;
GtkAdjustment *vol_adjustment;
int volume;
extern int volume;

void on_checkbutton1_clicked(GtkButton *button,
			     gpointer  user_data) {
  gtk_widget_hide (window1);
  setmute();
  get_mute_state();
}


gboolean vol_scroll_event(GtkRange     *range,
			  GtkScrollType scroll,
			  gdouble       value,
			  gpointer      user_data) {
  int volumeset;
  volumeset = (int)gtk_adjustment_get_value(vol_adjustment);
  
  setvol(volumeset);
  if (get_mute_state() == 0) {
    setmute();
    get_mute_state();
  }
  return FALSE;
}

gboolean on_scroll(GtkWidget *widget, GdkEventScroll *event) {
  int cv = getvol();
  if (event->direction == GDK_SCROLL_UP)
    setvol(cv+scroll_step);
  else 
    setvol(cv-scroll_step);

  if (get_mute_state() == 0) {
    setmute();
    get_mute_state();
  }

  // this will set the slider value
  get_current_levels();
  return TRUE;
}

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
  GdkColor color;
  gtk_color_button_get_color(GTK_COLOR_BUTTON(vcb),&color);
  gint colints[3];
  colints[0] = color.red;
  colints[1] = color.green;
  colints[2] = color.blue;
  g_key_file_set_integer_list(keyFile,"PNMixer","VolMeterColor",colints,3);

  // alsa card
  GtkWidget *acc = data->card_combo;
  gchar *old_card = get_selected_card();
  gchar *card = gtk_combo_box_get_active_text (GTK_COMBO_BOX(acc));
  if (old_card && strcmp(old_card,card))
      alsa_change = 1;
  g_key_file_set_string(keyFile,"PNMixer","AlsaCard",card);

  // channel
  GtkWidget *ccc = data->chan_combo;
  gchar* old_channel = NULL;
  if (old_card)
    old_channel = get_selected_channel(old_card);
  gchar* chan = gtk_combo_box_get_active_text (GTK_COMBO_BOX(ccc));
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
    gchar* theme_name = gtk_combo_box_get_active_text (GTK_COMBO_BOX(icon_combo));
    g_key_file_set_string(keyFile,"PNMixer","IconTheme",theme_name);
    g_free(theme_name);
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


  gchar* filename = g_strconcat(g_get_user_config_dir(), "/pnmixer/config", NULL);
  gchar* filedata = g_key_file_to_data(keyFile,&len,NULL);
  g_file_set_contents(filename,filedata,len,&err);
  if (err != NULL) {
    report_error("Couldn't write preferences file: %s\n", err->message);
    g_error_free (err);
  } else 
    apply_prefs(alsa_change);
  g_free(filename);
  gtk_widget_destroy(data->prefs_window);
  g_slice_free(PrefsData,data);
}

void on_cancel_button_clicked(GtkButton *button,
			      PrefsData *data) {
  gtk_widget_destroy(data->prefs_window);
  g_slice_free(PrefsData,data);
}
