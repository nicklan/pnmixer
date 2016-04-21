/* alsa.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file alsa.h
 * Header for alsa.c.
 * @brief Header for alsa.c.
 */

#ifndef _ALSA_H_
#define _ALSA_H_

#include <glib.h>

GSList *alsa_list_cards(void);
GSList *alsa_list_channels(const char *card_name);

typedef struct alsa_card AlsaCard;

AlsaCard *alsa_card_new(const char *card, const char *channel, gboolean normalize);
void alsa_card_free(AlsaCard *card);

enum alsa_event {
	ALSA_CARD_ERROR,
	ALSA_CARD_DISCONNECTED,
	ALSA_CARD_VALUES_CHANGED
};

typedef void (*AlsaCb) (enum alsa_event event, gpointer data);
void alsa_card_install_callback(AlsaCard *card, AlsaCb callback, gpointer data);

const char *alsa_card_get_name(AlsaCard *card);
const char *alsa_card_get_channel(AlsaCard *card);
gboolean alsa_card_is_muted(AlsaCard *card);
void alsa_card_toggle_mute(AlsaCard *card);
gdouble alsa_card_get_volume(AlsaCard *card);
void alsa_card_set_volume(AlsaCard *card, gdouble value, int dir);

#endif				// _ALSA_H_
