/* ui-popup-window.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file ui-popup-window.h
 * Header for ui-popup-window.c.
 * @brief Header for ui-popup-window.c.
 */

#ifndef _UI_POPUP_WINDOW_H_
#define _UI_POPUP_WINDOW_H_

#include "audio.h"

typedef struct popup_window PopupWindow;

PopupWindow *popup_window_create(Audio *audio);
void popup_window_destroy(PopupWindow *window);
void popup_window_reload(PopupWindow *window);
void popup_window_show(PopupWindow *window);
void popup_window_hide(PopupWindow *window);
void popup_window_toggle(PopupWindow *window);

#endif				// _UI_POPUP_WINDOW_H_
