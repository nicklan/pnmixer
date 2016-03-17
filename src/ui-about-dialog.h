/* ui-about-dialog.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file ui-about-dialog.h
 * Header for ui-about-dialog.c.
 * @brief header for ui-about-dialog.c.
 */

#ifndef _UI_ABOUT_DIALOG_H_
#define _UI_ABOUT_DIALOG_H_

#include <gtk/gtk.h>

typedef struct about_dialog AboutDialog;

AboutDialog *about_dialog_create(GtkWindow *parent);
void about_dialog_destroy(AboutDialog *dialog);
void about_dialog_run(AboutDialog *dialog);

#endif				// _UI_ABOUT_DIALOG_H_
