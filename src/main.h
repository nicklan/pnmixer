/* main.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file main.h
 * Header for main.c
 * @brief Header for main.c
 */

#ifndef _MAIN_H_
#define _MAIN_H_

#include <gtk/gtk.h>

void run_mixer_command(void);
void run_custom_command(void);

void run_about_dialog(void);
void run_error_dialog(const char *, ...);
void run_prefs_dialog(void);

void do_toggle_popup_window(void);
void do_show_popup_menu(GtkMenuPositionFunc func, gpointer data,
                        guint button, guint activate_time);

#endif				// _MAIN_H
