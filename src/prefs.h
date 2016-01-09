/* prefs.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file prefs.h
 * Header for prefs.c, holding public functions and globals.
 * @brief header for prefs.c
 */

#ifndef PREFS_H_
#define PREFS_H_

#include <glib.h>
#include <gtk/gtk.h>

#include "support.h"

GKeyFile *keyFile;
gint scroll_step, fine_scroll_step;
gboolean enable_noti, hotkey_noti, mouse_noti, popup_noti, external_noti;
gint noti_timeout;
GtkIconTheme *icon_theme;

gboolean prefs_get_boolean(gchar *key, gboolean def);
gint prefs_get_integer(gchar *key, gint def);
gdouble prefs_get_double(gchar *key, gdouble def);
gchar *prefs_get_string(gchar *key, const gchar *def);

gchar *prefs_get_selected_channel(const gchar *);
gchar *prefs_get_vol_command(void);

GtkWidget *create_prefs_window(void);
void ensure_prefs_dir(void);
void apply_prefs(gint);
void load_prefs(void);
void acquire_hotkey(const char *, PrefsData *);
gboolean normalize_vol(void);

#endif				// PREFS_H_
