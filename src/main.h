/* main.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */

#ifndef MAIN_H_
#define MAIN_H_

#include <gtk/gtk.h>

#ifdef DEBUG
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
#else
#define DEBUG_PRINT(...)
#endif

GtkWidget* create_window1 (void);
GtkWidget* create_menu (void);
void create_about (void);
void do_prefs (void);

void report_error(char*,...);
void get_current_levels();
int get_mute_state();
int update_mute_state();
void hide_me();
gint tray_icon_size();
void set_vol_meter_color(guint16 nr,guint16 ng,guint16 nb);
void update_status_icons();
void update_vol_text();

#endif // MAIN_H
