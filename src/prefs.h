#ifndef PREFS_H_
#define PREFS_H_

#include <glib.h>
#include <gtk/gtk.h>

GKeyFile* keyFile;
int scroll_step;
GtkIconTheme* icon_theme;

GtkWidget* create_prefs_window (void);
void ensure_prefs_dir(void);
void apply_prefs(gint);
void load_prefs(void);
void get_icon_theme();
gchar* get_selected_card();
gchar* get_selected_channel(gchar*);

#endif // PREFS_H_
