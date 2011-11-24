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
#include "alsa.h"
#include "callbacks.h"
#include "main.h"
#include "notify.h"
#include "support.h"
#include "hotkeys.h"
#include "prefs.h"

static GtkStatusIcon *tray_icon = NULL;
static GtkWidget *popup_menu;
static GdkPixbuf* status_icons[4];

static char err_buf[512];

void report_error(char* err,...) {
  va_list ap;
  va_start(ap, err);
  if (popup_window) {
    vsnprintf(err_buf,512,err,ap);
    GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(popup_window),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_CLOSE,
						NULL);
    gtk_window_set_title(GTK_WINDOW(dialog),_("PNMixer Error"));
    g_object_set(dialog,"text",err_buf,NULL);
    gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
  } else 
    vfprintf(stderr,err,ap);
  va_end(ap);
}

void warn_sound_conn_lost() {
  if (popup_window) {
    gint resp;
    GtkWidget *dialog = gtk_message_dialog_new (GTK_WINDOW(popup_window),
						GTK_DIALOG_DESTROY_WITH_PARENT,
						GTK_MESSAGE_ERROR,
						GTK_BUTTONS_YES_NO,
						_("Warning: Connection to sound system failed."));
    gtk_message_dialog_format_secondary_text (GTK_MESSAGE_DIALOG(dialog),
					      _("Do you want to re-initialize the connection to alsa?\n\n"
						"If you do not, you will either need to restart PNMixer "
						"or select the 'Reload Alsa' option in the right click "
						"menu in order for PNMixer to function."));
    gtk_window_set_title(GTK_WINDOW(dialog),_("PNMixer Error"));
    resp = gtk_dialog_run (GTK_DIALOG (dialog));
    gtk_widget_destroy (dialog);
    if (resp == GTK_RESPONSE_YES)
      do_alsa_reinit();
  } else
    fprintf(stderr,_("Warning: Connection to sound system failed, you probably need to restart pnmixer\n"));
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
	report_error(_("An unknown error occured trying to launch your volume control command"));
      else 
	report_error(_("Unable to launch volume command \"%s\".\n\n%s"),cmd,strerror(WEXITSTATUS(status)));
    }

    g_free(cmd);
  } else 
    report_error(_("\nNo mixer application was found on your system.\n\nPlease open preferences and set the command you want to run for volume control."));

  gtk_widget_hide (popup_window);
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
	    report_error(_("Couldn't execute custom command: \"%s\"\n"),cc);
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
  if (!GTK_WIDGET_VISIBLE(popup_window)) {
    gtk_widget_grab_focus(vol_scale);
    gtk_widget_show(popup_window);
  } else {
    gtk_widget_hide (popup_window);
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


void create_popups (void) {
  GtkBuilder *builder;
  GError     *error = NULL;
  gchar      *uifile;
  builder = gtk_builder_new();
  uifile = get_ui_file("popup_window.xml");
  if (!uifile) {
    report_error(_("Can't find main user interface file.  Please insure PNMixer is installed correctly.  Exiting\n"));
    gtk_exit(1);
  }
  if (!gtk_builder_add_from_file( builder, uifile, &error )) {
    g_warning("%s",error->message);
    report_error(error->message);
    gtk_exit(1);
  }

  g_free(uifile);

  vol_adjustment = GTK_ADJUSTMENT(gtk_builder_get_object(builder,"vol_scale_adjustment"));
  /* get original adjustments */
  get_current_levels();

  vol_scale = GTK_WIDGET(gtk_builder_get_object(builder,"vol_scale"));
  mute_check = GTK_WIDGET(gtk_builder_get_object(builder,"mute_check"));
  popup_window = GTK_WIDGET(gtk_builder_get_object(builder,"popup_window"));
  popup_menu = GTK_WIDGET(gtk_builder_get_object(builder,"popup_menu"));

  gtk_builder_connect_signals(builder, NULL);
  g_object_unref (G_OBJECT (builder));

  gtk_widget_grab_focus (vol_scale);
}


static void popup_callback(GObject *widget, guint button,
			   guint activate_time, GtkMenu* menu) {
  gtk_widget_hide (popup_window);
  gtk_menu_popup(menu, NULL, NULL,
		 gtk_status_icon_position_menu,
		 GTK_STATUS_ICON(widget), button, activate_time);
}

void do_prefs (void) {
  GtkWidget* pref_window = create_prefs_window();
  if (pref_window)
    gtk_widget_show(pref_window);
}

void do_alsa_reinit (void) {
  alsa_init();
  update_status_icons();
  update_vol_text();
  get_mute_state();
}

void create_about (void) {
  GtkBuilder *builder;
  GError     *error = NULL;
  GtkWidget  *about;
  gchar      *uifile;

  uifile = get_ui_file("about.xml");
  if (!uifile) {
    report_error(_("Can't find about interface file.  Please insure PNMixer is installed correctly."));
    return;
  }
  builder = gtk_builder_new();
  if (!gtk_builder_add_from_file( builder, uifile, &error)) {
    g_warning("%s",error->message);
    report_error(error->message);
    g_error_free(error);
    g_free(uifile);
    g_object_unref (G_OBJECT (builder));
    return;
  }
  g_free(uifile);
  gtk_builder_connect_signals(builder, NULL);
  about = GTK_WIDGET(gtk_builder_get_object(builder,"about_dialog"));
  gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(about),VERSION);
  g_object_unref (G_OBJECT (builder));

  gtk_dialog_run (GTK_DIALOG (about));
  gtk_widget_destroy (about);
}


void get_current_levels() {
  int tmpvol = getvol();
  gtk_adjustment_set_value(GTK_ADJUSTMENT(vol_adjustment), (double) tmpvol);
}

static float vol_div_factor;
static int vol_meter_width;
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
    memcpy(p,vol_meter_row,vol_meter_width);
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
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mute_check), FALSE);
    if (tmpvol < 33) 
      icon = status_icons[1];
    else if (tmpvol < 66)
      icon = status_icons[2];
    else 
      icon = status_icons[3];
    sprintf(tooltip, _("Volume: %d %%"), tmpvol);

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
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (mute_check), TRUE);
    gtk_status_icon_set_from_pixbuf(tray_icon, status_icons[0]);
    sprintf(tooltip, _("Volume: %d %%\nMuted"), tmpvol);
  }
  gtk_status_icon_set_tooltip_text(tray_icon, tooltip);
  return muted;
}


void hide_me() {
  gtk_widget_hide(popup_window);
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
  int i,icon_width;
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
  icon_width = gdk_pixbuf_get_height(status_icons[0]);
  vol_div_factor = ((gdk_pixbuf_get_height(status_icons[0])-10)/100.0);
  vol_meter_width = 1.25*icon_width;
  if (vol_meter_width%4 != 0)
    vol_meter_width -= (vol_meter_width%4);
  if (!vol_meter_row &&  g_key_file_get_boolean(keyFile,"PNMixer","DrawVolMeter",NULL)) {
    int lim = vol_meter_width/4;
    vol_meter_row = g_malloc(vol_meter_width*sizeof(guchar));
    for(i=0;i<lim;i++) {
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
    gtk_scale_set_draw_value (GTK_SCALE (vol_scale), TRUE);
    gtk_scale_set_value_pos (GTK_SCALE (vol_scale), pos);
  }
  else
    gtk_scale_set_draw_value (GTK_SCALE (vol_scale), FALSE);
}

static gboolean version = FALSE;
static GOptionEntry args[] = 
  {
    { "version", 0, 0, G_OPTION_ARG_NONE, &version, "Show version and exit", NULL },
    { NULL }
  };

int main (int argc, char *argv[]) {
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
    printf(_("%s version: %s\n"),PACKAGE,VERSION);
    exit(0);
  }

  popup_window = NULL;
  status_icons[0] = status_icons[1] = status_icons[2] = status_icons[3] = NULL;

  add_pixmap_directory (PACKAGE_DATA_DIR "/" PACKAGE "/pixmaps");
  add_pixmap_directory ("./pixmaps");

  ensure_prefs_dir();
  load_prefs();
  cards = NULL; // so we don't try and free on first run
  alsa_init();
  init_libnotify();
  create_popups();
  add_filter();
  apply_prefs(0);

  tray_icon = create_tray_icon();

  g_signal_connect(G_OBJECT(tray_icon), "popup-menu",G_CALLBACK(popup_callback), popup_menu);
  g_signal_connect(G_OBJECT(tray_icon), "activate", G_CALLBACK(tray_icon_on_click), NULL);
  g_signal_connect(G_OBJECT(tray_icon), "button-release-event", G_CALLBACK(tray_icon_button), NULL);

  gtk_main ();
  alsa_close();
  uninit_libnotify();
  return 0;
}
