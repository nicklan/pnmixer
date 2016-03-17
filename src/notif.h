/* notif.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file notif.h
 * Header for notif.c.
 * @brief Header for notif.c.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef _NOTIF_H_
#define _NOTIF_H_

typedef struct notif Notif;

Notif *notif_new(Audio *audio);
void notif_free(Notif *notif);
void notif_reload(Notif *notif);

#endif				// _NOTIF_H_
