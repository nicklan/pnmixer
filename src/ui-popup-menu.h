/* ui-popup-menu.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file ui-popup-menu.h
 * Header for ui-popup-menu.c.
 * @brief Header for ui-popup-menu.c.
 */

#ifndef _UI_POPUP_MENU_H_
#define _UI_POPUP_MENU_H_

#include "audio.h"

typedef struct popup_menu PopupMenu;

PopupMenu *popup_menu_create(Audio *audio);
void popup_menu_destroy(PopupMenu *menu);
void popup_menu_show(PopupMenu *menu, GtkMenuPositionFunc func, gpointer data,
                     guint button, guint activate_time);

#include <gtk/gtk.h>
GtkWindow *popup_menu_get_window(PopupMenu *menu);

#endif				// _UI_POPUP_MENU_H_
