/* ui-prefs-dialog.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file ui-prefs-dialog.h
 * Header for ui-prefs-dialog.c.
 * @brief Header for ui-prefs-dialog.c.
 */

#ifndef _UI_PREFS_DIALOG_H_
#define _UI_PREFS_DIALOG_H_

#include "audio.h"
#include "hotkeys.h"

typedef struct prefs_dialog PrefsDialog;

PrefsDialog *prefs_dialog_create(GtkWindow *parent, Audio *audio, Hotkeys *hotkeys);
void prefs_dialog_destroy(PrefsDialog *dialog);
gint prefs_dialog_run(PrefsDialog *dialog);

void prefs_dialog_populate(PrefsDialog *dialog);
void prefs_dialog_retrieve(PrefsDialog *dialog);

#endif				// _UI_PREFS_DIALOG_H_
