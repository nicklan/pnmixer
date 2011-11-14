/* prefs.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */


#ifndef PREFS_H_
#define PREFS_H_

#include <glib.h>
#include <gtk/gtk.h>

#include "support.h"

GKeyFile* keyFile;
int scroll_step;
GtkIconTheme* icon_theme;

GtkWidget* create_prefs_window (void);
void ensure_prefs_dir(void);
void apply_prefs(gint);
void load_prefs(void);
void get_icon_theme();
gchar* get_vol_command();
gchar* get_selected_card();
gchar* get_selected_channel(gchar*);
void aquire_hotkey(const char*,PrefsData*);

#endif // PREFS_H_
