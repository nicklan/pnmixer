/* audio.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file audio.h
 * Header for audio.c.
 * @brief Header for audio.c.
 */

#ifndef _AUDIO_H_
#define _AUDIO_H_

#include <glib.h>

/* High-level audio functions, no need to have a soundcard ready for that */

GSList *audio_get_card_list(void);
GSList *audio_get_channel_list(const char *card);

/* Soundcard management */

typedef struct audio Audio;

Audio *audio_new(void);
void audio_free(Audio *audio);
void audio_reload(Audio *audio);

/* Audio status: card & channel name, mute & volume handling.
 * Everyone who changes the volume must say who he is.
 */

enum audio_user {
	AUDIO_USER_UNKNOWN,
	AUDIO_USER_POPUP,
	AUDIO_USER_TRAY_ICON,
	AUDIO_USER_HOTKEYS,
};

typedef enum audio_user AudioUser;

const char *audio_get_card(Audio *audio);
const char *audio_get_channel(Audio *audio);
gboolean audio_is_muted(Audio *audio);
void audio_toggle_mute(Audio *audio, AudioUser user);
gdouble audio_get_volume(Audio *audio);
void audio_set_volume(Audio *audio, AudioUser user, gdouble volume, gint direction);
void audio_lower_volume(Audio *audio, AudioUser user);
void audio_raise_volume(Audio *audio, AudioUser user);

/* Signal handling.
 * The audio system sends signals out there when something happens.
 */

enum audio_signal {
	AUDIO_NO_CARD,
	AUDIO_CARD_INITIALIZED,
	AUDIO_CARD_CLEANED_UP,
	AUDIO_CARD_DISCONNECTED,
	AUDIO_CARD_ERROR,
	AUDIO_VALUES_CHANGED,
};

typedef enum audio_signal AudioSignal;

struct audio_event {
	AudioSignal signal;
	AudioUser user;
	const gchar *card;
	const gchar *channel;
	gboolean muted;
	gdouble volume;
};

typedef struct audio_event AudioEvent;

typedef void (*AudioCallback) (Audio *audio, AudioEvent *event, gpointer data);

void audio_signals_connect(Audio *audio, AudioCallback callback, gpointer data);
void audio_signals_disconnect(Audio *audio, AudioCallback callback, gpointer data);

#endif				// _AUDIO_H
