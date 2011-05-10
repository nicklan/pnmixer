/* main.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v2. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <math.h>
#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>
#include <alsa/asoundlib.h>
#include "callbacks.h"
#include "main.h"
#include "support.h"
#include "prefs.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name)			\
  g_object_set_data_full (G_OBJECT (component), name, gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)
#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name)	\
  g_object_set_data (G_OBJECT (component), name, widget)
GtkStatusIcon *tray_icon;
GtkWidget *window1;
GtkWidget *vbox1;
GtkWidget *hbox2;
GtkWidget *image1;
GtkWidget *hscale1;
GtkWidget *image2;
GtkWidget *hbox1;
GtkWidget *checkbutton1;
GtkWidget *button1;
static GtkWidget *menuitem_mute = NULL;
static GtkWidget *menuitem_about = NULL;
static GtkWidget *menuitem_vol = NULL;
static GtkWidget *menuitem_prefs = NULL;
GtkAdjustment *vol_adjustment;
GdkPixbuf *icon0;
static GdkPixbuf* status_icons[4];

static int smixer_level = 0;
static struct snd_mixer_selem_regopt smixer_options;
static char card[64] = "default";
static snd_mixer_elem_t *elem;
static snd_mixer_t *handle;



static snd_mixer_elem_t* alsa_get_mixer_elem(snd_mixer_t *mixer, char *name, int index) {
  snd_mixer_selem_id_t *selem_id;
  snd_mixer_elem_t* elem;
  snd_mixer_selem_id_alloca(&selem_id);

  if (index != -1)
    snd_mixer_selem_id_set_index(selem_id, index);
  if (name != NULL)
    snd_mixer_selem_id_set_name(selem_id, name);

  elem = snd_mixer_find_selem(mixer, selem_id);

  return elem;
}

static int alsaset() {
  smixer_options.device = card;
  int level = 1;
  int err;
  snd_mixer_selem_id_t *sid;
  snd_mixer_selem_id_alloca(&sid);

  if ((err = snd_mixer_open(&handle, 0)) < 0) {
    error("Mixer %s open error: %s", card, snd_strerror(err));
    return err;
  }
  if (smixer_level == 0 && (err = snd_mixer_attach(handle, card)) < 0) {
    error("Mixer attach %s error: %s", card, snd_strerror(err));
    snd_mixer_close(handle);
    return err;
  }
  if ((err = snd_mixer_selem_register(handle, smixer_level > 0 ? &smixer_options : NULL, NULL)) < 0) {
    error("Mixer register error: %s", snd_strerror(err));
    snd_mixer_close(handle);
    return err;
  }
  err = snd_mixer_load(handle);
  if (err < 0) {
    error("Mixer %s load error: %s", card, snd_strerror(err));
    snd_mixer_close(handle);
    return err;
  }

  elem = snd_mixer_first_elem(handle);
  snd_mixer_selem_get_id(elem, sid);

  return 0;

}

static int convert_prange(int val, int min, int max) {
  int range = max - min;
  int tmp;

  if (range == 0)
    return 0;
  val -= min;
  tmp = rint((double)val/(double)range * 100);
  return tmp;
}


static int get_percent(int val, int min, int max) {
  static char str[32];
  int p;
	
  p = ceil((val) * ((max) - (min)) * 0.01 + (min));
  return p;
}

int setvol(int vol) {
  long pmin = 0, pmax = 0;
  snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);

  vol = get_percent(vol,pmin,pmax);

  snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT,vol);
  snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT,vol);
}

void setmute() {
  int muted;
  snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &muted);

  if (muted == 1) {
    snd_mixer_selem_set_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT,0);
  } else {
    snd_mixer_selem_set_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT,1);
  }
}

int getvol() {
  long pmin = 0, pmax = 0;
  snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);

  int rr;
  long lr = rr;
  snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &lr);
  rr = lr;
  int vol;
  vol=convert_prange(rr,pmin,pmax);

  return vol;

}

void on_mixer(void) {	
  int no_pavucontrol = system("which pavucontrol | grep /pavucontrol");
  int no_alsamixer = system("which alsamixergui | grep /alsamixergui");

  if (no_pavucontrol) {
    if (no_alsamixer) {
      GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(window1),
						  GTK_DIALOG_DESTROY_WITH_PARENT,
						  GTK_MESSAGE_ERROR,
						  GTK_BUTTONS_CLOSE,
						  "\nNo mixer application was not found on your system.\n\nYou will need to install either pavucontrol or alsamixergui if you wish to use a mixer from the volume control.");
      gtk_dialog_run (GTK_DIALOG (dialog));
      gtk_widget_destroy (dialog);
    } else {
      const char *cmd1 = "alsamixergui&";
      if (system(cmd1)) { printf ("Failed to execute command \"alsamixergui\" \n"); }
    }
  } else {
    const char *cmd = "pavucontrol&";
    if (system(cmd)) { printf ("Failed to execute command \"pavucontrol\" \n"); }
  }

  gtk_widget_hide (window1);
}

void tray_icon_button(GtkStatusIcon *status_icon, GdkEventButton *event, gpointer user_data) {
  if (event->button == 2) {
    gint act = 0;
    if (g_key_file_has_key(keyFile,"PNMixer","MiddleClickAction",NULL)) 
      act = g_key_file_get_integer(keyFile,"PNMixer","MiddleClickAction",NULL);
    switch (act) {
    case 0: // mute/unmute
      setmute();
      get_mute_state();
      break;
    case 1:
      do_prefs();
      break;
    case 2:
      on_mixer();
      break;
    case 3:
      if (g_key_file_has_key(keyFile,"PNMixer","CustomCommand",NULL)) {
	gchar* cc = g_key_file_get_string(keyFile,"PNMixer","CustomCommand",NULL);
	if (cc != NULL) {
	  gchar* cmd = g_strconcat(cc, "&", NULL);
	  if (system(cmd))  
	    fprintf(stderr,"Couldn't execute custom command: %s\n",cc);
	  g_free(cmd);
	  g_free(cc);
	}
      }
      break;
    default: {} // nothing
    }
  }
}

void tray_icon_on_click(GtkStatusIcon *status_icon, gpointer user_data) {
  get_current_levels();
  if (!GTK_WIDGET_VISIBLE(window1)) {
    gtk_widget_grab_focus(hscale1);
    gtk_widget_show(window1);
  } else {
    gtk_widget_hide (window1);
  }
}


GtkStatusIcon *create_tray_icon() {
  tray_icon = gtk_status_icon_new();

  get_mute_state();

  /* catch scroll-wheel events */
  g_signal_connect ((gpointer) tray_icon, "scroll_event", G_CALLBACK (on_scroll),NULL);

  gtk_status_icon_set_visible(tray_icon, TRUE);
  return tray_icon;

}


GtkWidget* create_window1 (void) {
  window1 = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_widget_set_size_request (window1, 285, 82);
  GTK_WIDGET_SET_FLAGS (window1, GTK_CAN_FOCUS);
  gtk_window_set_position (GTK_WINDOW (window1), GTK_WIN_POS_MOUSE);
  gtk_window_set_resizable (GTK_WINDOW (window1), FALSE);
  gtk_window_set_decorated (GTK_WINDOW (window1), FALSE);
  gtk_window_set_skip_taskbar_hint (GTK_WINDOW (window1), TRUE);
  gtk_window_set_skip_pager_hint (GTK_WINDOW (window1), TRUE);
  gtk_window_set_type_hint (GTK_WINDOW (window1), GDK_WINDOW_TYPE_HINT_DIALOG);

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (window1), vbox1);

  hbox2 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox2);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox2, TRUE, TRUE, 0);

  image1 = gtk_image_new_from_stock ("gtk-remove", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (image1);
  gtk_box_pack_start (GTK_BOX (hbox2), image1, FALSE, FALSE, 0);
  gtk_widget_set_size_request (image1, 40, -1);

  vol_adjustment=GTK_ADJUSTMENT (gtk_adjustment_new (1, 0, 100, 1, 10, 0));
  /* get original adjustments */
  get_current_levels();

  hscale1 = gtk_hscale_new (GTK_ADJUSTMENT (vol_adjustment));
  gtk_widget_show (hscale1);
  gtk_box_pack_start (GTK_BOX (hbox2), hscale1, FALSE, FALSE, 0);
  gtk_widget_set_size_request (hscale1, 205, -1);
  gtk_scale_set_digits (GTK_SCALE (hscale1), 0);

  image2 = gtk_image_new_from_stock ("gtk-add", GTK_ICON_SIZE_BUTTON);
  gtk_widget_show (image2);
  gtk_box_pack_start (GTK_BOX (hbox2), image2, FALSE, FALSE, 0);
  gtk_widget_set_size_request (image2, 40, -1);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox1), hbox1, TRUE, TRUE, 0);

  checkbutton1 = gtk_check_button_new_with_mnemonic (_("Mute"));
  gtk_widget_show (checkbutton1);
  gtk_box_pack_start (GTK_BOX (hbox1), checkbutton1, FALSE, FALSE, 0);
  gtk_widget_set_size_request (checkbutton1, 97, 18);
  gtk_container_set_border_width (GTK_CONTAINER (checkbutton1), 8);

  button1 = gtk_button_new_with_mnemonic (_("Volume Control..."));
  gtk_widget_show (button1);
  gtk_box_pack_start (GTK_BOX (hbox1), button1, FALSE, FALSE, 0);
  gtk_widget_set_size_request (button1, 185, 19);
  gtk_container_set_border_width (GTK_CONTAINER (button1), 6);

  g_signal_connect ((gpointer) window1, "focus-out-event",G_CALLBACK (hide_me),NULL);
  g_signal_connect ((gpointer) window1, "button_release_event",G_CALLBACK (hide_me),NULL);
  g_signal_connect ((gpointer) hscale1, "key_press_event",G_CALLBACK (hide_me),NULL);
  g_signal_connect ((gpointer) hscale1, "value-changed",G_CALLBACK (on_hscale1_value_change_event),NULL);
  g_signal_connect ((gpointer) checkbutton1, "pressed",G_CALLBACK (on_checkbutton1_clicked),NULL);
  g_signal_connect ((gpointer) button1, "button_press_event",G_CALLBACK (on_mixer),NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (window1, window1, "window1");
  GLADE_HOOKUP_OBJECT (window1, vbox1, "vbox1");
  GLADE_HOOKUP_OBJECT (window1, hbox2, "hbox2");
  GLADE_HOOKUP_OBJECT (window1, image1, "image1");
  GLADE_HOOKUP_OBJECT (window1, hscale1, "hscale1");
  GLADE_HOOKUP_OBJECT (window1, image2, "image2");
  GLADE_HOOKUP_OBJECT (window1, hbox1, "hbox1");
  GLADE_HOOKUP_OBJECT (window1, checkbutton1, "checkbutton1");
  GLADE_HOOKUP_OBJECT (window1, button1, "button1");

  gtk_widget_grab_focus (hscale1);
  return window1;
}


static void popup_callback(GObject *widget, guint button,
			   guint activate_time, gpointer user_data) {
  GtkWidget *menu = user_data;

  gtk_widget_set_sensitive(menuitem_prefs,TRUE);
  gtk_widget_set_sensitive(menuitem_about,TRUE);
  gtk_widget_set_sensitive(menuitem_vol,TRUE);
  gtk_widget_set_sensitive(menuitem_mute,TRUE);

  gtk_widget_hide (window1);

  gtk_menu_popup(GTK_MENU(menu), NULL, NULL,
		 gtk_status_icon_position_menu,
		 GTK_STATUS_ICON(widget), button, activate_time);
}


static GtkWidget *create_popupmenu(void) {
  GtkWidget *menu;
  GtkWidget *item;
  GtkWidget *image;

  menu = gtk_menu_new();

  image = gtk_image_new_from_pixbuf (get_stock_pixbuf("stock_volume-mute",GTK_ICON_SIZE_MENU));
  item = gtk_image_menu_item_new_with_label(_("Mute/Unmute"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  menuitem_mute = item;
  g_signal_connect(item, "activate",G_CALLBACK(on_checkbutton1_clicked), NULL);

  image = gtk_image_new_from_pixbuf (get_stock_pixbuf("audio-volume-high",GTK_ICON_SIZE_MENU));
  item = gtk_image_menu_item_new_with_label(_("Volume Control"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  menuitem_vol = item;
  g_signal_connect(item, "activate",G_CALLBACK(on_mixer), NULL);

  image = gtk_image_new_from_stock (GTK_STOCK_PREFERENCES, GTK_ICON_SIZE_MENU);
  item = gtk_image_menu_item_new_with_label(_("Preferences"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  menuitem_prefs = item;
  g_signal_connect(item, "activate",G_CALLBACK(do_prefs), NULL);

  image = gtk_image_new_from_stock (GTK_STOCK_ABOUT, GTK_ICON_SIZE_MENU);
  item = gtk_image_menu_item_new_with_label(_("About"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  menuitem_about = item;
  g_signal_connect(item, "activate",G_CALLBACK(create_about), NULL);

  item = gtk_separator_menu_item_new();
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);

  image = gtk_image_new_from_stock (GTK_STOCK_QUIT, GTK_ICON_SIZE_MENU);
  item = gtk_image_menu_item_new_with_label(_("Quit"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  g_signal_connect(item, "activate",G_CALLBACK(gtk_exit), 0);

  return menu;
}

GtkWidget* do_prefs (void) {
  GtkWidget* pref_window = create_prefs_window();
  gtk_widget_show(pref_window);
}

GtkWidget* create_about (void) {
  GtkWidget *about;
  GtkWidget *vbox1;
  GtkWidget *about_image;
  GtkWidget *label1;

  about = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (about), _("About PNMixer"));

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (about), vbox1);

  about_image = create_pixmap (about, "pnmixer-sp.png");
  gtk_widget_show (about_image);
  gtk_box_pack_start (GTK_BOX (vbox1), about_image, TRUE, TRUE, 16);

  label1 = gtk_label_new (_("A mixer for the system tray.  http://github.com/nicklan/pnmixer"));;

  gtk_widget_show (label1);
  gtk_box_pack_start (GTK_BOX (vbox1), label1, FALSE, FALSE, 0);
  gtk_widget_set_size_request (label1, 250, 70);
  gtk_label_set_justify (GTK_LABEL (label1), GTK_JUSTIFY_CENTER);

  g_signal_connect ((gpointer) about, "delete_event",G_CALLBACK (gtk_widget_destroy),NULL);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (about, about, "about");
  GLADE_HOOKUP_OBJECT (about, vbox1, "vbox1");
  GLADE_HOOKUP_OBJECT (about, about_image, "about_image");
  GLADE_HOOKUP_OBJECT (about, label1, "label1");

  gtk_widget_show(about);

}


void get_current_levels() {
  int tmpvol = getvol();
  gtk_adjustment_set_value(GTK_ADJUSTMENT(vol_adjustment), (double) tmpvol);
}


int get_mute_state() {
  int muted;
  int tmpvol = getvol();
  char tooltip [60];
  GdkPixbuf* icon;
	
  snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &muted);

  if( muted == 1 ) {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), FALSE);
    if (tmpvol < 33) 
      icon = status_icons[1];
    else if (tmpvol < 66)
      icon = status_icons[2];
    else 
      icon = status_icons[3];
    sprintf(tooltip, "Volume: %d %%", tmpvol);
  } else {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), TRUE);
    icon = status_icons[0];
    sprintf(tooltip, "Volume: %d %%\nMuted", tmpvol);
  }
	
  gtk_status_icon_set_tooltip(tray_icon, tooltip);
  gtk_status_icon_set_from_pixbuf(tray_icon, icon);
  return muted;
}


void hide_me() {
  gtk_widget_hide(window1);
}

void load_status_icons() {
  status_icons[0] = get_stock_pixbuf("audio-volume-muted",48);
  status_icons[1] = get_stock_pixbuf("audio-volume-low",48);
  status_icons[2] = get_stock_pixbuf("audio-volume-medium",48);
  status_icons[3] = get_stock_pixbuf("audio-volume-high",48);
}

void update_vol_text() {
  gboolean show = TRUE;
  if (g_key_file_has_key(keyFile,"PNMixer","DisplayTextVolume",NULL))
    show = g_key_file_get_boolean(keyFile,"PNMixer","DisplayTextVolume",NULL);
  if (show) {
    GtkPositionType pos = GTK_POS_RIGHT;
    if (g_key_file_has_key(keyFile,"PNMixer","TextVolumePosition",NULL)) {
      gint pi = g_key_file_get_integer(keyFile,"PNMixer","TextVolumePosition",NULL);
      pos = 
	pi==0?GTK_POS_TOP:
	pi==1?GTK_POS_BOTTOM:
	pi==2?GTK_POS_LEFT:
	GTK_POS_RIGHT;
    }
    gtk_scale_set_draw_value (GTK_SCALE (hscale1), TRUE);
    gtk_scale_set_value_pos (GTK_SCALE (hscale1), pos);
  }
  else
    gtk_scale_set_draw_value (GTK_SCALE (hscale1), FALSE);
}

main (int argc, char *argv[]) {
  alsaset();

  GtkWidget *window1;
  GtkWidget *menu;

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  gtk_set_locale ();
  gtk_init (&argc, &argv);

  add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
  window1 = create_window1 ();

  load_prefs();
  apply_prefs();

  gtk_widget_realize(window1);
  gtk_widget_hide(window1);

  tray_icon = create_tray_icon();        
  menu = create_popupmenu();
  g_signal_connect(G_OBJECT(tray_icon), "popup-menu",G_CALLBACK(popup_callback), menu);
  g_signal_connect(G_OBJECT(tray_icon), "activate", G_CALLBACK(tray_icon_on_click), NULL);
  g_signal_connect(G_OBJECT(tray_icon), "button-release-event", G_CALLBACK(tray_icon_button), NULL);

  gtk_main ();
  snd_mixer_close(handle);
  return 0;
}
