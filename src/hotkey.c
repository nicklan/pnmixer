/* hotkey.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file hotkey.c
 * This file define what's a hotkey.
 * Deals with the low-level XKBlib and Gtk/Gdk.
 * @brief Hotkey subsystem.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>
#include <gdk/gdkx.h>
#include <X11/XKBlib.h>

#include "support-log.h"
#include "hotkey.h"

// `xmodmap -pm`
/* List of key modifiers which will be ignored whenever
 * we check whether the defined hotkeys have been pressed.
 */
static guint keymasks[] = {
	0,			/* No Modkey */
	GDK_MOD2_MASK,		/* Numlock */
	GDK_LOCK_MASK,		/* Capslock */
	GDK_MOD2_MASK | GDK_LOCK_MASK	/* Both */
};

/* Error that may be set if grabbing hotkey fails. */
static char grab_error;

/* Helpers */

/* When an Xlib error occurs when grabing the hotkey, this function is called.
 * The error handler should not call any functions (directly or indirectly)
 * on the display that will generate protocol requests or that will look for
 * input events.
 * Return value is ignored.
 */
static int
grab_error_handler(G_GNUC_UNUSED Display *disp, G_GNUC_UNUSED XErrorEvent *ev)
{
	WARN("Error while grabing hotkey");
	grab_error = 1;
	return 0;
}

/* Public functions */

/**
 * Ungrab a key manually. Should be paired with a hotkey_grab() call.
 *
 * @param hotkey a Hotkey instance.
 * @return TRUE on success, FALSE on error.
 */
void
hotkey_ungrab(Hotkey *hotkey)
{
	Display *disp;
	guint i;

	DEBUG("Ungrabing hotkey '%s'", hotkey->str);

	disp = gdk_x11_get_default_xdisplay();

	/* Ungrab the key */
	for (i = 0; i < G_N_ELEMENTS(keymasks); i++)
		XUngrabKey(disp, hotkey->code, hotkey->mods | keymasks[i],
		           GDK_ROOT_WINDOW());
}

/**
 * Grab a key manually. Should be paired with a hotkey_ungrab() call.
 *
 * @param hotkey a Hotkey instance.
 * @return TRUE on success, FALSE on error.
 */
gboolean
hotkey_grab(Hotkey *hotkey)
{
	Display *disp;
	XErrorHandler old_hdlr;
	guint i;

	DEBUG("Grabing hotkey '%s'", hotkey->str);

	disp = gdk_x11_get_default_xdisplay();

	/* Init error handling */
	grab_error = 0;
	old_hdlr = XSetErrorHandler(grab_error_handler);

	/* Grab the key */
	for (i = 0; i < G_N_ELEMENTS(keymasks); i++)
		XGrabKey(disp, hotkey->code, hotkey->mods | keymasks[i],
		         GDK_ROOT_WINDOW(), 1, GrabModeAsync, GrabModeAsync);

	/* Synchronize X */
	XFlush(disp);
	XSync(disp, False);

	/* Restore error handler */
	(void) XSetErrorHandler(old_hdlr);

	/* Check for error */
	if (grab_error)
		return FALSE;

	return TRUE;
}

/**
 * Checks if the keycode we got (minus modifiers like
 * numlock/capslock) matches the hotkey.
 * Thus numlock + o will match o.
 *
 * @param hotkey a Hotkey instance.
 * @param code the key code to compare against.
 * @param mods the key modifiers to compare against.
 * @return TRUE if there is a match, FALSE otherwise.
 */
gboolean
hotkey_matches(Hotkey *hotkey, guint code, GdkModifierType mods)
{
	guint i;

	if (code != hotkey->code)
		return FALSE;

	for (i = 0; i < G_N_ELEMENTS(keymasks); i++)
		if ((hotkey->mods | keymasks[i]) == mods)
			return TRUE;

	return FALSE;
}

/**
 * Ungrab a key and free any resources.
 *
 * @param hotkey a Hotkey instance.
 */
void
hotkey_free(Hotkey *hotkey)
{
	if (hotkey == NULL)
		return;

	hotkey_ungrab(hotkey);

	g_free(hotkey->str);
	g_free(hotkey);
}

/**
 * Creates a new hotkey and grab it.
 *
 * @param code the key's code.
 * @param mods the key's modifiers.
 * @return the newly created Hotkey instance.
 */
Hotkey *
hotkey_new(guint code, GdkModifierType mods)
{
	Hotkey *hotkey;
	Display *disp;

	hotkey = g_new0(Hotkey, 1);
	disp = gdk_x11_get_default_xdisplay();

	hotkey->code = code;
	hotkey->mods = mods;
	hotkey->sym = XkbKeycodeToKeysym(disp, hotkey->code, 0, 0);
	hotkey->str = gtk_accelerator_name(hotkey->sym, hotkey->mods);

	if (hotkey_grab(hotkey) == FALSE) {
		hotkey_free(hotkey);
		hotkey = NULL;
	}

	return hotkey;
}

/**
 * Translate a key into a Gtk Accelerator string.
 *
 * @param code the key code to process.
 * @param mods the key modifiers to process.
 * @return the accelerator string, must be freed.
 */
gchar *
hotkey_code_to_accel(guint code, GdkModifierType mods)
{
	Display *disp;
	guint sym;
	gchar *accel;

	disp = gdk_x11_get_default_xdisplay();

	sym = XkbKeycodeToKeysym(disp, code, 0, 0);
	accel = gtk_accelerator_name(sym, mods);

	return accel;
}

/**
 * Translate a Gtk Accelerator string to a key code and mods.
 *
 * @param accel the accelerator string to parse.
 * @param code the key code returned after parsing.
 * @param mods the key modifiers after parsing.
 */
void
hotkey_accel_to_code(const gchar *accel, gint *code, GdkModifierType *mods)
{
	Display *disp;
	guint sym;

	disp = gdk_x11_get_default_xdisplay();

	gtk_accelerator_parse(accel, &sym, mods);
	if (sym != 0)
		*code = XKeysymToKeycode(disp, sym);
	else
		*code = -1;
}
