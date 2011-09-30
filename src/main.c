/* main.c
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

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <gdk/gdkkeysyms.h>
#include "alsa.h"
#include "callbacks.h"
#include "main.h"
#include "support.h"
#include "prefs.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name)			\
  g_object_set_data_full (G_OBJECT (component), name, gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)
#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name)	\
  g_object_set_data (G_OBJECT (component), name, widget)
GtkStatusIcon *tray_icon = NULL;
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

static char err_buf[512];

void report_error(char* err,...) {
  va_list ap;
  va_start(ap, err);
  if (window1) {
    vsnprintf(err_buf,512,err,ap);
    GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(window1),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_CLOSE,
						NULL);
    g_object_set(dialog,"text",err_buf,NULL);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  } else 
    vfprintf(stderr,err,ap);
  va_end(ap);
}

void warn_sound_conn_lost() {
  if (window1) {
    gint resp;
    GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(window1),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_YES_NO,
						"Warning: Connection to sound system failed.");
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dialog),
					      "Do you want to re-initialize the connection to alsa?\n\n"
					      "If you do not, you will either need to restart PNMixer "
					      "or select the 'Reload Alsa' option in the right click "
					      "menu in order for PNMixer to function.");
    resp = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    if (resp == GTK_RESPONSE_YES)
      do_alsa_reinit();
  } else
    fprintf(stderr,"Warning: Connection to sound system failed, you probably need to restart pnmixer\n");
}

void on_mixer(void) {
  pid_t pid;
  int status;
  gchar* cmd = get_vol_command();

  if (cmd) {
    pid = fork();

    if (pid == 0) {
      if (execlp (cmd, cmd, NULL))
	_exit(errno);
      _exit (EXIT_FAILURE);
    }
    else if (pid < 0)
      status = -1;
    else {
      /* this is a bit hacky, but seems the best way to
	 figure out launching the command failed.  this
	 is basically a waitpid with a timeout (.5 seconds)
	 to give time for the command to try and start.
	 if it hasn't failed within 0.5 seconds we assume
	 it started okay. */
      int wr;
      int tl = 5;
      do {
	wr = waitpid (pid, &status, WNOHANG);
	if (wr && wr != pid) {
	  status = -1;
	  break;
	}
	usleep(100000);
	tl--;
      }	while(!wr && tl);
      if (!tl)
	status = 0;
    }

    if (status) {
      if (status == -1)
	report_error("An unknown error occured trying to launch your volume control command");
      else 
	report_error("Unable to launch volume command \"%s\".\n\n%s",cmd,strerror(WEXITSTATUS(status)));
    }

    g_free(cmd);
  } else 
    report_error("\nNo mixer application was found on your system.\n\nPlease open preferences and set the command you want to run for volume control.");

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
	    report_error("Couldn't execute custom command: \"%s\"\n",cc);
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

gint tray_icon_size() {
  if(tray_icon && gtk_status_icon_is_embedded(tray_icon))
    return gtk_status_icon_get_size(tray_icon);
  return 48;
}


static gboolean tray_icon_resized(GtkStatusIcon *status_icon,
				  gint           size,
				  gpointer       user_data) {
  update_status_icons();
  return FALSE;
}

GtkStatusIcon *create_tray_icon() {
  tray_icon = gtk_status_icon_new();

  /* catch scroll-wheel events */
  g_signal_connect ((gpointer) tray_icon, "scroll_event", G_CALLBACK (on_scroll), NULL);
  g_signal_connect ((gpointer) tray_icon, "size-changed", G_CALLBACK (tray_icon_resized), NULL);

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
  g_signal_connect ((gpointer) hscale1, "change-value",G_CALLBACK (vol_scroll_event),NULL);
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

  image = gtk_image_new_from_pixbuf (get_stock_pixbuf("gtk-execute",GTK_ICON_SIZE_MENU));
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

  image = gtk_image_new_from_stock (GTK_STOCK_REFRESH, GTK_ICON_SIZE_MENU);
  item = gtk_image_menu_item_new_with_label(_("Reload Alsa"));
  gtk_image_menu_item_set_image (GTK_IMAGE_MENU_ITEM (item), image);
  gtk_widget_show(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  menuitem_prefs = item;
  g_signal_connect(item, "activate",G_CALLBACK(do_alsa_reinit), NULL);

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

void do_prefs (void) {
  GtkWidget* pref_window = create_prefs_window();
  gtk_widget_show(pref_window);
}

void do_alsa_reinit (void) {
  alsa_init();
  update_status_icons();
  update_vol_text();
  get_mute_state();
}

void create_about (void) {
  GtkWidget *about;
  GtkWidget *vbox1;
  GtkWidget *about_image;
  GtkWidget *label1,*title_label;
  gchar title_buf[128];

  about = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (about), _("About PNMixer"));

  vbox1 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (about), vbox1);

  g_snprintf(title_buf,128,"<span font_size=\"x-large\" font_weight=\"bold\">PNMixer %s</span>",VERSION);
  title_label = gtk_label_new (_(title_buf));

  gtk_widget_show (title_label);
  gtk_box_pack_start (GTK_BOX (vbox1), title_label, FALSE, FALSE, 0);
  gtk_label_set_use_markup (GTK_LABEL (title_label), TRUE);
  gtk_widget_set_size_request (title_label, 250, 70);
  gtk_label_set_justify (GTK_LABEL (title_label), GTK_JUSTIFY_CENTER);

  about_image = create_pixmap (about, "pnmixer-about.png");
  gtk_widget_show (about_image);
  gtk_box_pack_start (GTK_BOX (vbox1), about_image, TRUE, TRUE, 16);

  label1 = gtk_label_new (_("A mixer for the system tray.\nhttp://github.com/nicklan/pnmixer"));

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

static float vol_div_factor;
static guchar* vol_meter_row = NULL;
static void draw_vol_meter(GdkPixbuf *pixbuf, int x, int y, int h) {
  int width, height, rowstride, n_channels,i;
  guchar *pixels, *p;

  n_channels = gdk_pixbuf_get_n_channels (pixbuf);

  g_assert (gdk_pixbuf_get_colorspace (pixbuf) == GDK_COLORSPACE_RGB);
  g_assert (gdk_pixbuf_get_bits_per_sample (pixbuf) == 8);
  g_assert (gdk_pixbuf_get_has_alpha (pixbuf));
  g_assert (n_channels == 4);

  width = gdk_pixbuf_get_width (pixbuf);
  height = gdk_pixbuf_get_height (pixbuf);

  g_assert (x >= 0 && x < width);
  g_assert ((y+h) >= 0 && y < height);

  rowstride = gdk_pixbuf_get_rowstride (pixbuf);
  pixels = gdk_pixbuf_get_pixels (pixbuf);

  y = (height - y);
  for (i = 0;i < h;i++) {
    p = pixels + (y-i) * rowstride + x * n_channels;
    memcpy(p,vol_meter_row,40);
  }
}

static int draw_offset = 0;
static GdkPixbuf *icon_copy = NULL;
int get_mute_state() {
  int muted;
  int tmpvol = getvol();
  char tooltip [60];
  
  muted = ismuted();

  if( muted == 1 ) {
    GdkPixbuf *icon;
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), FALSE);
    if (tmpvol < 33) 
      icon = status_icons[1];
    else if (tmpvol < 66)
      icon = status_icons[2];
    else 
      icon = status_icons[3];
    sprintf(tooltip, "Volume: %d %%", tmpvol);

    if (vol_meter_row) {
      GdkPixbuf* old_icon = icon_copy;
      icon_copy = gdk_pixbuf_copy(icon);
      draw_vol_meter(icon_copy,draw_offset,5,(tmpvol*vol_div_factor));
      if (old_icon) 
	g_object_unref(old_icon);
      gtk_status_icon_set_from_pixbuf(tray_icon, icon_copy);
    } else
      gtk_status_icon_set_from_pixbuf(tray_icon, icon);
  } else {
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (checkbutton1), TRUE);
    gtk_status_icon_set_from_pixbuf(tray_icon, status_icons[0]);
    sprintf(tooltip, "Volume: %d %%\nMuted", tmpvol);
  }
  gtk_status_icon_set_tooltip_text(tray_icon, tooltip);
  return muted;
}


void hide_me() {
  gtk_widget_hide(window1);
}


static guchar vol_meter_red,vol_meter_green,vol_meter_blue;

void set_vol_meter_color(guint16 nr,guint16 ng,guint16 nb) {
  vol_meter_red = nr/257;
  vol_meter_green = ng/257;
  vol_meter_blue = nb/257;
  if (vol_meter_row)
    g_free(vol_meter_row);
  vol_meter_row = NULL;
}

void update_status_icons() {
  int i;
  GdkPixbuf* old_icons[4];
  int size = tray_icon_size();
  for(i=0;i<4;i++)
    old_icons[i] = status_icons[i];
  if (g_key_file_has_key(keyFile,"PNMixer","IconTheme",NULL)) {
    status_icons[0] = get_stock_pixbuf("audio-volume-muted",size);
    status_icons[1] = get_stock_pixbuf("audio-volume-low",size);
    status_icons[2] = get_stock_pixbuf("audio-volume-medium",size);
    status_icons[3] = get_stock_pixbuf("audio-volume-high",size);
  } else {
    status_icons[0] = create_pixbuf("pnmixer-muted.png");
    status_icons[1] = create_pixbuf("pnmixer-low.png");
    status_icons[2] = create_pixbuf("pnmixer-medium.png");
    status_icons[3] = create_pixbuf("pnmixer-high.png");
  }
  vol_div_factor = ((size-10)/100.0);
  if (!vol_meter_row &&  g_key_file_get_boolean(keyFile,"PNMixer","DrawVolMeter",NULL)) {
    vol_meter_row = g_malloc(40*sizeof(guchar));
    for(i=0;i<10;i++) {
      vol_meter_row[i*4]   = vol_meter_red;
      vol_meter_row[i*4+1] = vol_meter_green;
      vol_meter_row[i*4+2] = vol_meter_blue;
      vol_meter_row[i*4+3] = 255;
    }
  } else if (vol_meter_row && !g_key_file_get_boolean(keyFile,"PNMixer","DrawVolMeter",NULL)) {
    free(vol_meter_row);
    vol_meter_row = NULL;
    if (icon_copy)
      g_object_unref(icon_copy);
    icon_copy = NULL;
  }
  draw_offset = g_key_file_get_integer(keyFile,"PNMixer","VolMeterPos",NULL);
  if (tray_icon)
    get_mute_state();
  for(i = 0;i < 4;i++)
    if(old_icons[i]) 
      g_object_unref(old_icons[i]);
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

static gboolean version = FALSE;
static GOptionEntry args[] = 
  {
    { "version", 0, 0, G_OPTION_ARG_NONE, &version, "Show version and exit", NULL },
    { NULL }
  };

int main (int argc, char *argv[]) {
  GtkWidget *menu;
  GError *error = NULL;
  GOptionContext *context;

#ifdef ENABLE_NLS
  bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
  bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
  textdomain (GETTEXT_PACKAGE);
#endif

  DEBUG_PRINT("[Debugging Mode Build]\n");

  gtk_set_locale ();
  context = g_option_context_new (_("- A mixer for the system tray."));
  g_option_context_add_main_entries (context, args, GETTEXT_PACKAGE);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));
  g_option_context_parse (context, &argc, &argv, &error);
  gtk_init (&argc, &argv);

  g_option_context_free(context);


  if (version) {
    printf("%s version: %s\n",PACKAGE,VERSION);
    exit(0);
  }

  window1 = NULL;
  status_icons[0] = status_icons[1] = status_icons[2] = status_icons[3] = NULL;

  add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
  add_pixmap_directory ("./pixmaps");

  ensure_prefs_dir();
  load_prefs();
  cards = NULL; // so we don't try and free on first run
  alsa_init();
  window1 = create_window1 ();
  apply_prefs(0);

  tray_icon = create_tray_icon();        
  menu = create_popupmenu();
  g_signal_connect(G_OBJECT(tray_icon), "popup-menu",G_CALLBACK(popup_callback), menu);
  g_signal_connect(G_OBJECT(tray_icon), "activate", G_CALLBACK(tray_icon_on_click), NULL);
  g_signal_connect(G_OBJECT(tray_icon), "button-release-event", G_CALLBACK(tray_icon_button), NULL);

  gtk_main ();
  alsa_close();
  return 0;
}
