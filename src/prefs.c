/* prefs.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file prefs.c
 * This file holds the preferences subsystem,
 * managing the user config file.
 * @brief Preferences subsystem.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>

#include <glib.h>
#include <glib/gstdio.h>

#include "prefs.h"
#include "support-log.h"
#include "support-intl.h"

#include "main.h"

#define DEFAULT_PREFS "[PNMixer]\n\
SliderOrientation=vertical\n\
DisplayTextVolume=true\n\
TextVolumePosition=0\n\
ScrollStep=5\n\
FineScrollStep=1\n\
MiddleClickAction=0\n\
CustomCommand=\n\
VolMuteKey=-1\n\
VolUpKey=-1\n\
VolDownKey=-1\n\
AlsaCard=(default)\n\
NormalizeVolume=true\n\
SystemTheme=false"

static GKeyFile *keyFile;

/*
 * Default volume commands.
 */
static const gchar *vol_control_commands[] = {
	"pavucontrol",
	"gnome-alsamixer",
	"xfce4-mixer",
	"alsamixergui",
	NULL
};

/*
 * Look for an installed volume command.
 */
static const gchar *
find_vol_control_command(void)
{
	gchar buf[256];
	const char **cmd;

	DEBUG("Looking for a volume control command...");

	cmd = vol_control_commands;
	while (*cmd) {
		snprintf(buf, 256, "which %s >/dev/null", *cmd);
		if (system(buf) == 0) {
			DEBUG("'%s' selected as the volume control command", *cmd);
			return *cmd;
		}
		cmd++;
	}

	return NULL;
}

/**
 * Gets a boolean value from preferences.
 * On error, returns def as default value.
 *
 * @param key the specific settings key
 * @param def the default value to return on error
 * @return the preference value or def on error
 */
gboolean
prefs_get_boolean(const gchar *key, gboolean def)
{
	gboolean ret;
	GError *error = NULL;

	ret = g_key_file_get_boolean(keyFile, "PNMixer", key, &error);
	if (error) {
		g_error_free(error);
		return def;
	}

	return ret;
}

/**
 * Gets an int value from a preferences.
 * On error, returns def as default value.
 *
 * @param key the specific settings key
 * @param def the default value to return on error
 * @return the preference value or def on error
 */
gint
prefs_get_integer(const gchar *key, gint def)
{
	gint ret;
	GError *error = NULL;

	ret = g_key_file_get_integer(keyFile, "PNMixer", key, &error);
	if (error) {
		g_error_free(error);
		return def;
	}

	return ret;
}

/**
 * Gets a double value from preferences.
 * On error, returns def as default value.
 *
 * @param key the specific settings key
 * @param def the default value to return on error
 * @return the preference value or def on error
 */
gdouble
prefs_get_double(const gchar *key, gdouble def)
{
	gdouble ret;
	GError *error = NULL;

	ret = g_key_file_get_double(keyFile, "PNMixer", key, &error);
	if (error) {
		g_error_free(error);
		return def;
	}

	return ret;
}

/**
 * Gets a string value from preferences.
 * On error, returns def as default value.
 *
 * @param key the specific settings key
 * @param def the default value to return on error
 * @return the preference value or def on error. Must be freed.
 */
gchar *
prefs_get_string(const gchar *key, const gchar *def)
{
	gchar *ret;
	GError *error = NULL;

	ret = g_key_file_get_string(keyFile, "PNMixer", key, &error);

	/* Return value if found */
	if (error == NULL)
		return ret;
	else
		g_error_free(error);

	/* If the volume control command is not defined,
	 * be clever and try to find a command installed.
	 */
	if (!g_strcmp0(key, "VolumeControlCommand")) {
		const gchar *cmd = find_vol_control_command();
		if (cmd)
			return g_strdup(cmd);
	}

	/* At last, return default value */
	return g_strdup(def);
}

/**
 * Gets a list of doubles from preferences.
 * On error, returns NULL.
 *
 * @param key the specific settings key
 * @param n integer pointer that will contain the returned array size
 * @return the preference value or NULL on error
 */
gdouble *
prefs_get_double_list(const gchar *key, gsize *n)
{
	gsize numcols = 0;
	gdouble *ret = NULL;
	GError *error = NULL;

	ret = g_key_file_get_double_list(keyFile, "PNMixer", key, &numcols, &error);
	if (error) {
		g_error_free(error);
		ret = NULL;
	}

	/* For the volume meter color, we need a little bit of care.
	 * Ensure the list is valid, and provide default values if needed.
	 */
	if (!g_strcmp0(key, "VolMeterColor")) {
		gsize i;

		if (ret && numcols != 3) {
			g_free(ret);
			ret = NULL;
		}

		if (!ret) {
			ret = g_malloc(3 * sizeof(gdouble));
			ret[0] = 0.909803921569;
			ret[1] = 0.43137254902;
			ret[2] = 0.43137254902;
		}

		for (i = 0; i < 3; i++) {
			if (ret[i] < 0)
				ret[i] = 0;
			if (ret[i] > 1)
				ret[i] = 1;
		}
	}

	if (n)
		*n = numcols;

	return ret;
}

/**
 * Gets the currently selected channel of the specified Alsa Card
 * from the global keyFile and returns the result.
 *
 * @param card the Alsa Card to get the currently selected channel of
 * @return the currently selected channel as newly allocated string,
 * NULL on failure
 */
gchar *
prefs_get_channel(const gchar *card)
{
	if (!card)
		return NULL;
	return g_key_file_get_string(keyFile, card, "Channel", NULL);
}

/**
 * Sets a boolean value to preferences.
 *
 * @param key the specific settings key
 * @param value the value to set
 */
void
prefs_set_boolean(const gchar *key, gboolean value)
{
	g_key_file_set_boolean(keyFile, "PNMixer", key, value);
}

/**
 * Sets a integer value to preferences.
 *
 * @param key the specific settings key
 * @param value the value to set
 */
void
prefs_set_integer(const gchar *key, gint value)
{
	g_key_file_set_integer(keyFile, "PNMixer", key, value);
}

/**
 * Sets a double value to preferences.
 *
 * @param key the specific settings key
 * @param value the value to set
 */
void
prefs_set_double(const gchar *key, gdouble value)
{
	g_key_file_set_double(keyFile, "PNMixer", key, value);
}

/**
 * Sets a string value to preferences.
 *
 * @param key the specific settings key
 * @param value the value to set
 */
void
prefs_set_string(const gchar *key, const gchar *value)
{
	g_key_file_set_string(keyFile, "PNMixer", key, value);
}

/**
 * Sets a list of doubles to preferences.
 *
 * @param key the specific settings key
 * @param list the array of double values to set
 * @param n the array length
 */
void
prefs_set_double_list(const gchar *key, gdouble *list, gsize n)
{
	g_key_file_set_double_list(keyFile, "PNMixer", key, list, n);
}

/**
 * Sets the channel for a given card in preferences.
 *
 * @param card the Alsa Card associated with the channel
 * @param channel the channel to save in the preferences.
 */
void
prefs_set_channel(const gchar *card, const gchar *channel)
{
	g_key_file_set_string(keyFile, card, "Channel", channel);
}

/**
 * Loads the preferences from the config file to the keyFile object (GKeyFile type).
 * Creates the keyFile object if it doesn't exist.
 */
void
prefs_load(void)
{
	GError *err = NULL;
	gchar *filename = g_build_filename(g_get_user_config_dir(),
	                                   "pnmixer", "config", NULL);

	if (keyFile != NULL)
		g_key_file_free(keyFile);

	keyFile = g_key_file_new();

	if (g_file_test(filename, G_FILE_TEST_EXISTS)) {
		if (!g_key_file_load_from_file(keyFile, filename, 0, &err)) {
			run_error_dialog(_("Couldn't load preferences file: %s"),
			                 err->message);
			g_error_free(err);
			g_key_file_free(keyFile);
			keyFile = NULL;
		}
	} else {
		if (!g_key_file_load_from_data
		    (keyFile, DEFAULT_PREFS, strlen(DEFAULT_PREFS), 0, &err)) {
			run_error_dialog(_("Couldn't load default preferences: %s"),
			                 err->message);
			g_error_free(err);
			g_key_file_free(keyFile);
			keyFile = NULL;
		}
	}

	g_free(filename);
}

/**
 * Save the preferences from the keyFile object to the config file.
 */
void
prefs_save(void)
{
	gsize len;
	GError *err = NULL;
	gchar *filename = g_build_filename(g_get_user_config_dir(),
	                                   "pnmixer", "config", NULL);
	gchar *filedata = g_key_file_to_data(keyFile, &len, NULL);

	g_file_set_contents(filename, filedata, len, &err);

	if (err != NULL) {
		run_error_dialog(_("Couldn't write preferences file: %s"), err->message);
		g_error_free(err);
	}

	g_free(filename);
	g_free(filedata);
}

/**
 * Checks if the preferences dir for saving is present and accessible.
 * Creates it if doesn't exist. Reports errors via run_error_dialog().
 */
void
prefs_ensure_save_dir(void)
{
	gchar *prefs_dir = g_build_filename(g_get_user_config_dir(),
	                                    "pnmixer", NULL);

	if (!g_file_test(prefs_dir, G_FILE_TEST_IS_DIR)) {
		if (g_file_test(prefs_dir, G_FILE_TEST_EXISTS))
			run_error_dialog(_("'%s' exists but is not a directory, "
			                   "won't be able to save preferences."),
			                 prefs_dir);
		else if (g_mkdir(prefs_dir, S_IRWXU))
			run_error_dialog(_("Couldn't make preferences directory: %s"),
			                 strerror(errno));
	}

	g_free(prefs_dir);
}
