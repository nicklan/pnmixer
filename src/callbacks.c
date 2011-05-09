/* callbacks.c
 * OBmixer was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v2. source code is available at 
 * <http://www.jpegserv.com>
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <alsa/asoundlib.h>
#include "callbacks.h"
#include "main.h"
#include "support.h"
#include "prefs.h"

GtkWidget *window1;
GtkWidget *checkbutton1;
GtkAdjustment *vol_adjustment;
int volume;
extern int volume;

void
on_checkbutton1_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{

	gtk_widget_hide (window1);
	setmute();
	get_mute_state();

}

gboolean
on_hscale1_value_change_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	int volumeset;
	volumeset = (int)gtk_adjustment_get_value(vol_adjustment);
	
	setvol(volumeset);
	if (get_mute_state() == 0) {
		setmute();
		get_mute_state();
	}
	
	return FALSE;
}

gboolean on_scroll (GtkWidget *widget, GdkEventScroll *event) {
  if (event->direction == GDK_SCROLL_UP)
    setvol(getvol()+scroll_step);
  else
    setvol(getvol()-scroll_step);

  if (get_mute_state() == 0) {
    setmute();
    get_mute_state();
  }

  // this will set the slider value
  get_current_levels();
  return TRUE;
}

void
on_ok_button_clicked                   (GtkButton       *button,
                                        gpointer         user_data)
{
  gsize len;
  GError *err = NULL;

  // pull out various prefs

  // show vol text
  GtkWidget* vtc = lookup_widget(user_data,"vol_text_check");
  gboolean active = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(vtc));
  g_key_file_set_boolean(keyFile,"OBMixer","DisplayTextVolume",active);
  
  // vol pos
  GtkWidget* vpc = lookup_widget(user_data,"vol_pos_combo");
  gint idx = gtk_combo_box_get_active(GTK_COMBO_BOX(vpc));
  g_key_file_set_integer(keyFile,"OBMixer","TextVolumePosition",idx);

  // icon theme
  GtkWidget* icon_combo = lookup_widget(user_data,"icon_theme_combo");
  gchar* theme_name = gtk_combo_box_get_active_text (GTK_COMBO_BOX(icon_combo));
  g_key_file_set_string(keyFile,"OBMixer","IconTheme",theme_name);
  g_free(theme_name);

  // scroll step
  GtkWidget* sss = lookup_widget(user_data,"scroll_step_spin");
  gint spin = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(sss));
  g_key_file_set_integer(keyFile,"OBMixer","MouseScrollStep",spin);

  // middle click
  GtkWidget* mcc = lookup_widget(user_data,"middle_click_combo");
  idx = gtk_combo_box_get_active(GTK_COMBO_BOX(mcc));
  g_key_file_set_integer(keyFile,"OBMixer","MiddleClickAction",idx);

  // custom command
  GtkWidget* ce = lookup_widget(user_data,"custom_entry");
  const gchar* cc = gtk_entry_get_text (GTK_ENTRY(ce));
  g_key_file_set_string(keyFile,"OBMixer","CustomCommand",cc);


  gchar* filename = g_strconcat(g_get_user_config_dir(), "/obmixer", NULL);
  gchar* data = g_key_file_to_data(keyFile,&len,NULL);
  g_file_set_contents(filename,data,len,&err);
  if (err != NULL) {
    fprintf (stderr, "Couldn't write preferences file: %s\n", err->message);
    g_error_free (err);
  } else
    apply_prefs();
  g_free(filename);
  gtk_widget_destroy(user_data);
  get_mute_state();
}

void
on_cancel_button_clicked                   (GtkButton       *button,
					    gpointer         user_data)
{
  gtk_widget_destroy(user_data);
}
