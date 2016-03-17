/* hotkey.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file hotkey.h
 * Header for hotkey.c.
 * @brief Header for hotkey.c.
 */

#ifndef _HOTKEY_H_
#define _HOTKEY_H_

#include <gdk/gdkx.h>

struct hotkey {
	/* These values should only be accessed for reading,
	 * and shouldn't be modified outside of hotkey.c
	 */
	guint code; /* Key code */
	GdkModifierType mods; /* Key modifier */
	unsigned long int sym; /* X Key Symbol */
	gchar *str; /* Gtk Accelerator string */
};

typedef struct hotkey Hotkey;

Hotkey *hotkey_new(guint code, GdkModifierType mods);
void hotkey_free(Hotkey *key);
gboolean hotkey_matches(Hotkey *hotkey, guint code, GdkModifierType mods);

void hotkey_ungrab(Hotkey *hotkey);
gboolean hotkey_grab(Hotkey *hotkey);

gchar *hotkey_code_to_accel(guint code, GdkModifierType mods);
void hotkey_accel_to_code(const gchar *accel, gint *code, GdkModifierType *mods);

#endif				// _HOTKEY_H
