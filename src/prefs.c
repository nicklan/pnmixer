#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include <gdk/gdkkeysyms.h>
#include <gtk/gtk.h>

#include "callbacks.h"
#include "prefs.h"
#include "support.h"
#include "main.h"

#define GLADE_HOOKUP_OBJECT(component,widget,name) \
  g_object_set_data_full (G_OBJECT (component), name, \
    gtk_widget_ref (widget), (GDestroyNotify) gtk_widget_unref)

#define GLADE_HOOKUP_OBJECT_NO_REF(component,widget,name) \
  g_object_set_data (G_OBJECT (component), name, widget)

#define DEFAULT_PREFS "[PNMixer]\n\
DisplayTextVolume=true\n\
TextVolumePosition=3\n\
MouseScrollStep=1\n\
MiddleClickAction=0\n\
CustomCommand="

/* Get available icon themes.
   This code is based on code from xfce4-appearance-settings */
static void
load_icon_themes(GtkWidget* icon_theme_combo) {
  GDir          *dir;
  GKeyFile      *index_file;
  const gchar   *file;
  gchar         *index_filename;
  const gchar   *theme_name;
  gchar   *active_theme_name;
  gint          i,j,n,act;
  gboolean      is_dup;
  GtkIconTheme* theme;
  gchar         **path;
  GtkSettings*  settings;


  theme = gtk_icon_theme_get_default();
  index_file = g_key_file_new();

  if (g_key_file_has_key(keyFile,"PNMixer","IconTheme",NULL)) {
    active_theme_name = g_key_file_get_string(keyFile,"PNMixer","IconTheme",NULL);
  } else {
    settings = gtk_settings_get_default();
    g_object_get(settings,"gtk-icon-theme-name",&active_theme_name,NULL);
  }
  act = 1;
  

  gtk_icon_theme_get_search_path(theme, &path, &n);
  for (i = 0; i < n; i++) {
    /* Make sure we don't double search */
    is_dup = FALSE;
    for (j = 0; j < n; j++) {
      if (j >= i) break;
      if (g_strcmp0(path[i],path[j]) == 0) {
	is_dup = TRUE;
	break;
      }
    }
    if (is_dup)
      continue;
    

    /* Open directory handle */
    dir = g_dir_open(path[i], 0, NULL);

    /* Try next base directory if this one cannot be read */
    if (G_UNLIKELY (dir == NULL))
      continue;

    /* Iterate over filenames in the directory */
    while ((file = g_dir_read_name (dir)) != NULL) {
      /* Build filename for the index.theme of the current icon theme directory */
      index_filename = g_build_filename (path[i], file, "index.theme", NULL);
      
      /* Try to open the theme index file */
      if (g_key_file_load_from_file(index_file,index_filename,0,NULL)) {
	if (g_key_file_has_key(index_file,"Icon Theme","Directories",NULL) &&
	    !g_key_file_get_boolean(index_file,"Icon Theme","Hidden",NULL)) {
	  theme_name = g_key_file_get_string (index_file, "Icon Theme","Name",NULL);
	  gtk_combo_box_append_text (GTK_COMBO_BOX (icon_theme_combo), _(theme_name));
	  if (g_strcmp0(theme_name,active_theme_name) == 0) 
	    gtk_combo_box_set_active (GTK_COMBO_BOX (icon_theme_combo), act);
	  else
	    act++;
	}
      }
      g_free(index_filename);
    }
  }
  g_key_file_free(index_file);
  g_free(active_theme_name);
}

void load_prefs(void) {
  GError* err = NULL;
  gchar* filename = g_strconcat(g_get_user_config_dir(), "/pnmixer", NULL);

  if (keyFile != NULL)
    g_key_file_free(keyFile);
  keyFile = g_key_file_new();
  if (g_file_test(filename,G_FILE_TEST_EXISTS)) {
    if (!g_key_file_load_from_file(keyFile,filename,0,&err)) {
      fprintf (stderr, "Couldn't load preferences file: %s\n", err->message);
      g_error_free(err);
      g_key_file_free(keyFile);
      keyFile = NULL;
    }
  }
  else {
    if (!g_key_file_load_from_data(keyFile,DEFAULT_PREFS,strlen(DEFAULT_PREFS),0,&err)) {
      fprintf (stderr, "Couldn't load default preferences: %s\n", err->message);
      g_error_free(err);
      g_key_file_free(keyFile);
      keyFile = NULL;
    }
  }
  g_free(filename);
}

void apply_prefs() {
  scroll_step = 1;
  if (g_key_file_has_key(keyFile,"PNMixer","MouseScrollStep",NULL))
    scroll_step = g_key_file_get_integer(keyFile,"PNMixer","MouseScrollStep",NULL);
  get_icon_theme();
  load_status_icons();
  update_vol_text();
}

void get_icon_theme() {
  if (g_key_file_has_key(keyFile,"PNMixer","IconTheme",NULL)) {
    gchar* theme_name = g_key_file_get_string(keyFile,"PNMixer","IconTheme",NULL);
    if (icon_theme == NULL)
      icon_theme = gtk_icon_theme_new();
    gtk_icon_theme_set_custom_theme(icon_theme,theme_name);
    g_free(theme_name);
  }
  else 
    icon_theme = gtk_icon_theme_get_default();
}

void on_vol_text_toggle(GtkToggleButton* button, gpointer user_data) {
  GtkWidget* label = lookup_widget(user_data,"vol_pos_label");
  GtkWidget* combo = lookup_widget(user_data,"vol_pos_combo");
  gboolean active  = gtk_toggle_button_get_active (button);
  gtk_widget_set_sensitive(label,active);
  gtk_widget_set_sensitive(combo,active);
}

void on_middle_changed(GtkComboBox* box, gpointer user_data) {
  GtkWidget* label = lookup_widget(user_data,"label7");
  GtkWidget* entry = lookup_widget(user_data,"custom_entry");
  gboolean cust = gtk_combo_box_get_active (GTK_COMBO_BOX(box)) == 3;
  gtk_widget_set_sensitive(label,cust);
  gtk_widget_set_sensitive(entry,cust);
}

GtkWidget*
create_prefs_window (void)
{
  GtkWidget *prefs_window;
  GtkWidget *vbox1;
  GtkWidget *vol_frame;
  GtkWidget *alignment1;
  GtkWidget *vbox2;
  GtkWidget *vol_text_check;
  GtkWidget *hbox1;
  GtkWidget *vol_pos_label;
  GtkWidget *vol_pos_combo;
  GtkWidget *vol_frame_label;
  GtkWidget *frame2;
  GtkWidget *alignment2;
  GtkWidget *icon_theme_combo;
  GtkWidget *label3;
  GtkWidget *frame3;
  GtkWidget *alignment3;
  GtkWidget *table1;
  GtkWidget *label5;
  GtkWidget *label6;
  GtkObject *scroll_step_spin_adj;
  GtkWidget *scroll_step_spin;
  GtkWidget *middle_click_combo;
  GtkWidget *label7;
  GtkWidget *custom_entry;
  GtkWidget *label4;
  GtkWidget *okay_cancel_button_box;
  GtkWidget *cancel_button;
  GtkWidget *ok_button;

  load_prefs();

  prefs_window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title (GTK_WINDOW (prefs_window), _("OB Mixer Preferences"));

  vbox1 = gtk_vbox_new (FALSE, 10);
  gtk_widget_show (vbox1);
  gtk_container_add (GTK_CONTAINER (prefs_window), vbox1);
  gtk_container_set_border_width (GTK_CONTAINER (vbox1), 5);

  vol_frame = gtk_frame_new (NULL);
  gtk_widget_show (vol_frame);
  gtk_box_pack_start (GTK_BOX (vbox1), vol_frame, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (vol_frame), GTK_SHADOW_IN);

  alignment1 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_show (alignment1);
  gtk_container_add (GTK_CONTAINER (vol_frame), alignment1);
  gtk_container_set_border_width (GTK_CONTAINER (alignment1), 3);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment1), 0, 0, 12, 0);

  vbox2 = gtk_vbox_new (FALSE, 0);
  gtk_widget_show (vbox2);
  gtk_container_add (GTK_CONTAINER (alignment1), vbox2);

  vol_text_check = gtk_check_button_new_with_mnemonic (_("Display Text Volume"));
  gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(vol_text_check),g_key_file_get_boolean(keyFile,"PNMixer","DisplayTextVolume",NULL));
  gtk_widget_show (vol_text_check);
  gtk_box_pack_start (GTK_BOX (vbox2), vol_text_check, FALSE, FALSE, 0);
  g_signal_connect(G_OBJECT(vol_text_check), "toggled",G_CALLBACK(on_vol_text_toggle), prefs_window);

  hbox1 = gtk_hbox_new (FALSE, 0);
  gtk_widget_show (hbox1);
  gtk_box_pack_start (GTK_BOX (vbox2), hbox1, TRUE, TRUE, 0);

  vol_pos_label = gtk_label_new (_("Volume Text Position:"));
  gtk_widget_show (vol_pos_label);
  gtk_box_pack_start (GTK_BOX (hbox1), vol_pos_label, FALSE, FALSE, 0);
  gtk_misc_set_padding (GTK_MISC (vol_pos_label), 10, 0);

  vol_pos_combo = gtk_combo_box_new_text ();
  gtk_widget_show (vol_pos_combo);
  gtk_box_pack_start (GTK_BOX (hbox1), vol_pos_combo, TRUE, TRUE, 0);
  gtk_combo_box_append_text (GTK_COMBO_BOX (vol_pos_combo), _("Top"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (vol_pos_combo), _("Bottom"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (vol_pos_combo), _("Left"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (vol_pos_combo), _("Right"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (vol_pos_combo),g_key_file_get_integer(keyFile,"PNMixer","TextVolumePosition",NULL));

  vol_frame_label = gtk_label_new (_("<b>Volume Display</b>"));
  gtk_widget_show (vol_frame_label);
  gtk_frame_set_label_widget (GTK_FRAME (vol_frame), vol_frame_label);
  gtk_label_set_use_markup (GTK_LABEL (vol_frame_label), TRUE);

  if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON(vol_text_check))) {
    gtk_widget_set_sensitive(vol_pos_label,TRUE);
    gtk_widget_set_sensitive(vol_pos_combo,TRUE);
  } else {
    gtk_widget_set_sensitive(vol_pos_label,FALSE);
    gtk_widget_set_sensitive(vol_pos_combo,FALSE);
  }

  frame2 = gtk_frame_new (NULL);
  gtk_widget_show (frame2);
  gtk_box_pack_start (GTK_BOX (vbox1), frame2, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame2), GTK_SHADOW_IN);

  alignment2 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_show (alignment2);
  gtk_container_add (GTK_CONTAINER (frame2), alignment2);
  gtk_container_set_border_width (GTK_CONTAINER (alignment2), 3);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment2), 0, 0, 12, 0);

  icon_theme_combo = gtk_combo_box_new_text ();
  gtk_widget_show (icon_theme_combo);
  gtk_container_add (GTK_CONTAINER (alignment2), icon_theme_combo);
  gtk_combo_box_append_text (GTK_COMBO_BOX (icon_theme_combo), _("PNMixer Icons"));
  load_icon_themes(icon_theme_combo);

  label3 = gtk_label_new (_("<b>Icon Theme</b>"));
  gtk_widget_show (label3);
  gtk_frame_set_label_widget (GTK_FRAME (frame2), label3);
  gtk_label_set_use_markup (GTK_LABEL (label3), TRUE);

  frame3 = gtk_frame_new (NULL);
  gtk_widget_show (frame3);
  gtk_box_pack_start (GTK_BOX (vbox1), frame3, TRUE, TRUE, 0);
  gtk_frame_set_shadow_type (GTK_FRAME (frame3), GTK_SHADOW_IN);

  alignment3 = gtk_alignment_new (0.5, 0.5, 1, 1);
  gtk_widget_show (alignment3);
  gtk_container_add (GTK_CONTAINER (frame3), alignment3);
  gtk_container_set_border_width (GTK_CONTAINER (alignment3), 3);
  gtk_alignment_set_padding (GTK_ALIGNMENT (alignment3), 0, 0, 12, 0);

  table1 = gtk_table_new (3, 2, FALSE);
  gtk_widget_show (table1);
  gtk_container_add (GTK_CONTAINER (alignment3), table1);
  gtk_table_set_row_spacings (GTK_TABLE (table1), 5);

  label5 = gtk_label_new (_("Mouse Scroll Step:"));
  gtk_widget_show (label5);
  gtk_table_attach (GTK_TABLE (table1), label5, 0, 1, 0, 1,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label5), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label5), 5, 0);

  label6 = gtk_label_new (_("Middle Click Action:"));
  gtk_widget_show (label6);
  gtk_table_attach (GTK_TABLE (table1), label6, 0, 1, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label6), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label6), 5, 0);

  scroll_step_spin_adj = gtk_adjustment_new (1, 0, 100, 1, 10, 0);
  scroll_step_spin = gtk_spin_button_new (GTK_ADJUSTMENT (scroll_step_spin_adj), 1, 0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(scroll_step_spin),g_key_file_get_integer(keyFile,"PNMixer","MouseScrollStep",NULL));
  gtk_widget_show (scroll_step_spin);
  gtk_table_attach (GTK_TABLE (table1), scroll_step_spin, 1, 2, 0, 1,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);

  middle_click_combo = gtk_combo_box_new_text ();
  gtk_widget_show (middle_click_combo);
  gtk_table_attach (GTK_TABLE (table1), middle_click_combo, 1, 2, 1, 2,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (GTK_FILL), 0, 0);
  gtk_combo_box_append_text (GTK_COMBO_BOX (middle_click_combo), _("Mute/Unmute"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (middle_click_combo), _("Show Preferences"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (middle_click_combo), _("Volume Control"));
  gtk_combo_box_append_text (GTK_COMBO_BOX (middle_click_combo), _("Custom (set below)"));
  gtk_combo_box_set_active (GTK_COMBO_BOX (middle_click_combo),g_key_file_get_integer(keyFile,"PNMixer","MiddleClickAction",NULL));
  g_signal_connect(G_OBJECT(middle_click_combo), "changed",G_CALLBACK(on_middle_changed), prefs_window);

  label7 = gtk_label_new (_("Custom Command:"));
  gtk_widget_show (label7);
  gtk_table_attach (GTK_TABLE (table1), label7, 0, 1, 2, 3,
                    (GtkAttachOptions) (GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_misc_set_alignment (GTK_MISC (label7), 0, 0.5);
  gtk_misc_set_padding (GTK_MISC (label7), 5, 0);

  custom_entry = gtk_entry_new ();
  gtk_widget_show (custom_entry);
  gtk_table_attach (GTK_TABLE (table1), custom_entry, 1, 2, 2, 3,
                    (GtkAttachOptions) (GTK_EXPAND | GTK_FILL),
                    (GtkAttachOptions) (0), 0, 0);
  gtk_entry_set_invisible_char (GTK_ENTRY (custom_entry), 8226);
  gtk_entry_set_text (GTK_ENTRY (custom_entry),g_key_file_get_string(keyFile,"PNMixer","CustomCommand",NULL));

  if (gtk_combo_box_get_active (GTK_COMBO_BOX(middle_click_combo)) == 3) {
    gtk_widget_set_sensitive(label7,TRUE);
    gtk_widget_set_sensitive(custom_entry,TRUE);
  } else {
    gtk_widget_set_sensitive(label7,FALSE);
    gtk_widget_set_sensitive(custom_entry,FALSE);
  }

  label4 = gtk_label_new (_("<b>Mouse</b>"));
  gtk_widget_show (label4);
  gtk_frame_set_label_widget (GTK_FRAME (frame3), label4);
  gtk_label_set_use_markup (GTK_LABEL (label4), TRUE);

  okay_cancel_button_box = gtk_hbutton_box_new ();
  gtk_widget_show (okay_cancel_button_box);
  gtk_box_pack_start (GTK_BOX (vbox1), okay_cancel_button_box, TRUE, TRUE, 0);
  gtk_button_box_set_layout (GTK_BUTTON_BOX (okay_cancel_button_box), GTK_BUTTONBOX_END);

  cancel_button = gtk_button_new_from_stock ("gtk-cancel");
  gtk_widget_show (cancel_button);
  gtk_container_add (GTK_CONTAINER (okay_cancel_button_box), cancel_button);
  GTK_WIDGET_SET_FLAGS (cancel_button, GTK_CAN_DEFAULT);

  ok_button = gtk_button_new_from_stock ("gtk-ok");
  gtk_widget_show (ok_button);
  gtk_container_add (GTK_CONTAINER (okay_cancel_button_box), ok_button);
  GTK_WIDGET_SET_FLAGS (ok_button, GTK_CAN_DEFAULT);

  g_signal_connect ((gpointer) ok_button, "clicked",
                    G_CALLBACK (on_ok_button_clicked),
                    prefs_window);

  g_signal_connect ((gpointer) cancel_button, "clicked",
                    G_CALLBACK (on_cancel_button_clicked),
                    prefs_window);

  /* Store pointers to all widgets, for use by lookup_widget(). */
  GLADE_HOOKUP_OBJECT_NO_REF (prefs_window, prefs_window, "prefs_window");
  GLADE_HOOKUP_OBJECT (prefs_window, vbox1, "vbox1");
  GLADE_HOOKUP_OBJECT (prefs_window, vol_frame, "vol_frame");
  GLADE_HOOKUP_OBJECT (prefs_window, alignment1, "alignment1");
  GLADE_HOOKUP_OBJECT (prefs_window, vbox2, "vbox2");
  GLADE_HOOKUP_OBJECT (prefs_window, vol_text_check, "vol_text_check");
  GLADE_HOOKUP_OBJECT (prefs_window, hbox1, "hbox1");
  GLADE_HOOKUP_OBJECT (prefs_window, vol_pos_label, "vol_pos_label");
  GLADE_HOOKUP_OBJECT (prefs_window, vol_pos_combo, "vol_pos_combo");
  GLADE_HOOKUP_OBJECT (prefs_window, vol_frame_label, "vol_frame_label");
  GLADE_HOOKUP_OBJECT (prefs_window, frame2, "frame2");
  GLADE_HOOKUP_OBJECT (prefs_window, alignment2, "alignment2");
  GLADE_HOOKUP_OBJECT (prefs_window, icon_theme_combo, "icon_theme_combo");
  GLADE_HOOKUP_OBJECT (prefs_window, label3, "label3");
  GLADE_HOOKUP_OBJECT (prefs_window, frame3, "frame3");
  GLADE_HOOKUP_OBJECT (prefs_window, alignment3, "alignment3");
  GLADE_HOOKUP_OBJECT (prefs_window, table1, "table1");
  GLADE_HOOKUP_OBJECT (prefs_window, label5, "label5");
  GLADE_HOOKUP_OBJECT (prefs_window, label6, "label6");
  GLADE_HOOKUP_OBJECT (prefs_window, scroll_step_spin, "scroll_step_spin");
  GLADE_HOOKUP_OBJECT (prefs_window, middle_click_combo, "middle_click_combo");
  GLADE_HOOKUP_OBJECT (prefs_window, label7, "label7");
  GLADE_HOOKUP_OBJECT (prefs_window, custom_entry, "custom_entry");
  GLADE_HOOKUP_OBJECT (prefs_window, label4, "label4");
  GLADE_HOOKUP_OBJECT (prefs_window, okay_cancel_button_box, "okay_cancel_button_box");
  GLADE_HOOKUP_OBJECT (prefs_window, cancel_button, "cancel_button");
  GLADE_HOOKUP_OBJECT (prefs_window, ok_button, "ok_button");

  return prefs_window;
}

