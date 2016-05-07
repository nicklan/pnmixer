/* hotkeys.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file hotkeys.c
 * This file handles the hotkeys subsystem, including
 * communication with Xlib and intercepting key presses
 * before they can be interpreted by Gtk/Gdk.
 * @brief Hotkeys subsystem.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gdk/gdkx.h>
#include <X11/XKBlib.h>

#include "audio.h"
#include "prefs.h"
#include "support-intl.h"
#include "support-log.h"
#include "hotkey.h"
#include "hotkeys.h"

#include "main.h"

/* Helpers */

/* Removes the previously attached key_filter() function from
 * the root window.
 */
static void
hotkeys_remove_filter(GdkFilterFunc filter, gpointer data)
{
	GdkWindow *window;

	window = gdk_x11_window_foreign_new_for_display
	         (gdk_display_get_default(), GDK_ROOT_WINDOW());

	gdk_window_remove_filter(window, filter, data);
}

/* Ataches the key_filter() function as a filter
 * to the root window, so it will intercept window events.
 */
static void
hotkeys_add_filter(GdkFilterFunc filter, gpointer data)
{
	GdkWindow *window;

	window = gdk_x11_window_foreign_new_for_display
	         (gdk_display_get_default(), GDK_ROOT_WINDOW());

	gdk_window_add_filter(window, filter, data);
}

/* Public functions & callbacks */

struct hotkeys {
	/* Audio system */
	Audio  *audio;
	/* Hotkeys */
	Hotkey *mute_hotkey;
	Hotkey *up_hotkey;
	Hotkey *down_hotkey;
};

/**
 * This function is called before Gtk/Gdk can respond
 * to any(!) window event and handles pressed hotkeys.
 *
 * @param gdk_xevent the native event to filter
 * @param event the GDK event to which the X event will be translated
 * @param data user data set when the filter was installed
 * @return a GdkFilterReturn value, should be GDK_FILTER_CONTINUE only
 */
static GdkFilterReturn
key_filter(GdkXEvent *gdk_xevent, G_GNUC_UNUSED GdkEvent *event, gpointer data)
{
	gint type;
	guint key, state;
	XKeyEvent *xevent = (XKeyEvent *) gdk_xevent;
	Hotkeys *hotkeys = (Hotkeys *) data;
	Audio *audio = hotkeys->audio;

	type = xevent->type;
	key = xevent->keycode;
	state = xevent->state;

	if (type == KeyPress) {
		if (hotkey_matches(hotkeys->mute_hotkey, key, state))
			audio_toggle_mute(audio, AUDIO_USER_HOTKEYS);
		else if (hotkey_matches(hotkeys->up_hotkey, key, state))
			audio_raise_volume(audio, AUDIO_USER_HOTKEYS);
		else if (hotkey_matches(hotkeys->down_hotkey, key, state))
			audio_lower_volume(audio, AUDIO_USER_HOTKEYS);
		// just ignore unknown hotkeys
	}

	return GDK_FILTER_CONTINUE;
}

/**
 * Reload hotkey preferences.
 * This has to be called each time the preferences are modified.
 *
 * @param hotkeys a Hotkeys instance.
 */
void
hotkeys_reload(Hotkeys *hotkeys)
{
	gboolean enabled;
	gint key, mods;
	gboolean mute_err, up_err, down_err;

	/* Free any hotkey that may be currently assigned */
	hotkey_free(hotkeys->mute_hotkey);
	hotkeys->mute_hotkey = NULL;

	hotkey_free(hotkeys->up_hotkey);
	hotkeys->up_hotkey = NULL;

	hotkey_free(hotkeys->down_hotkey);
	hotkeys->down_hotkey = NULL;

	/* Return if hotkeys are disabled */
	enabled = prefs_get_boolean("EnableHotKeys", FALSE);
	if (enabled == FALSE)
		return;

	/* Setup mute hotkey */
	mute_err = FALSE;
	key = prefs_get_integer("VolMuteKey", -1);
	mods = prefs_get_integer("VolMuteMods", 0);
	if (key != -1) {
		hotkeys->mute_hotkey = hotkey_new(key, mods);
		if (hotkeys->mute_hotkey == NULL)
			mute_err = TRUE;
	}

	/* Setup volume up hotkey */
	up_err = FALSE;
	key = prefs_get_integer("VolUpKey", -1);
	mods = prefs_get_integer("VolUpMods", 0);
	if (key != -1) {
		hotkeys->up_hotkey = hotkey_new(key, mods);
		if (hotkeys->up_hotkey == NULL)
			up_err = TRUE;
	}

	/* Setup volume down hotkey */
	down_err = FALSE;
	key = prefs_get_integer("VolDownKey", -1);
	mods = prefs_get_integer("VolDownMods", 0);
	if (key != -1) {
		hotkeys->down_hotkey = hotkey_new(key, mods);
		if (hotkeys->down_hotkey == NULL)
			down_err = TRUE;
	}

	/* Display error message if needed */
	if (mute_err || up_err || down_err) {
		run_error_dialog("%s:\n%s%s%s%s%s%s",
		                 _("Could not grab the following HotKeys"),
		                 mute_err ? _("Mute/Unmute") : "",
		                 mute_err ? "\n" : "",
		                 up_err ? _("Volume Up") : "",
		                 up_err ? "\n" : "",
		                 down_err ? _("Volume Down") : "",
		                 down_err ? "\n" : ""
		                );
	}
}

/**
 * Unbind hotkeys manually. Should be paired with a hotkeys_bind() call.
 *
 * @param hotkeys a Hotkeys instance.
 */
void
hotkeys_unbind(Hotkeys *hotkeys)
{
	hotkeys_remove_filter(key_filter, hotkeys);

	if (hotkeys->mute_hotkey)
		hotkey_ungrab(hotkeys->mute_hotkey);
	if (hotkeys->up_hotkey)
		hotkey_ungrab(hotkeys->up_hotkey);
	if (hotkeys->down_hotkey)
		hotkey_ungrab(hotkeys->down_hotkey);
}

/**
 * Bind hotkeys manually. Should be paired with a hotkeys_unbind() call.
 *
 * @param hotkeys a Hotkeys instance.
 */
void
hotkeys_bind(Hotkeys *hotkeys)
{
	if (hotkeys->mute_hotkey)
		hotkey_grab(hotkeys->mute_hotkey);
	if (hotkeys->up_hotkey)
		hotkey_grab(hotkeys->up_hotkey);
	if (hotkeys->down_hotkey)
		hotkey_grab(hotkeys->down_hotkey);

	hotkeys_add_filter(key_filter, hotkeys);
}

/**
 * Cleanup the hotkey subsystem.
 *
 * @param hotkeys a Hotkeys instance.
 */
void
hotkeys_free(Hotkeys *hotkeys)
{
	if (hotkeys == NULL)
		return;

	/* Disable hotkeys */
	hotkeys_remove_filter(key_filter, hotkeys);

	/* Free anything */
	hotkey_free(hotkeys->mute_hotkey);
	hotkey_free(hotkeys->up_hotkey);
	hotkey_free(hotkeys->down_hotkey);
	g_free(hotkeys);
}

/**
 * Creates the hotkeys subsystem, and bind the hotkeys.
 *
 * @param audio the audio system, needed to control the audio.
 * @return the newly created Hotkeys instance.
 */
Hotkeys *
hotkeys_new(Audio *audio)
{
	Hotkeys *hotkeys;

	DEBUG("Creating hotkeys control");

	hotkeys = g_new0(Hotkeys, 1);

	/* Save audio pointer */
	hotkeys->audio = audio;

	/* Load preferences */
	hotkeys_reload(hotkeys);

	/* Bind hotkeys */
	hotkeys_add_filter(key_filter, hotkeys);

	return hotkeys;
}
