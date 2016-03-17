/* ui-hotkey-dialog.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file ui-hotkey-dialog.h
 * Header for ui-hotkey-dialog.c.
 * @brief Header for ui-hotkey-dialog.c.
 */

#ifndef _UI_HOTKEY_DIALOG_H_
#define _UI_HOTKEY_DIALOG_H_

#include <gtk/gtk.h>

typedef struct hotkey_dialog HotkeyDialog;

HotkeyDialog *hotkey_dialog_create(GtkWindow *parent, const gchar *hotkey);
void hotkey_dialog_destroy(HotkeyDialog *dialog);
gchar *hotkey_dialog_run(HotkeyDialog *dialog);

#endif				// _UI_HOTKEY_DIALOG_H_
