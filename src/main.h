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

GtkWidget* create_window1 (void);
GtkWidget* create_menu (void);
GtkWidget* create_about (void);
GtkWidget* do_prefs (void);

void report_error(char*,...);
void get_current_levels();
int update_mute_state();
void hide_me();
void load_status_icons();
void update_vol_text();

#endif // MAIN_H
