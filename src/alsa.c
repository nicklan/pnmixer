/* alsa.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file alsa.c
 * Alsa audio subsystem.
 * All the alsa-related code is enclosed in here, and this is the only
 * file that uses the alsa library.
 * This is the lowest-level part of PNMixer, that's why it doesn't include
 * any other local headers. If you do so, it probably means that you're
 * starting messing up the code, so think twice.
 * @brief Alsa audio subsystem.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#define _GNU_SOURCE /* exp10() */
#include <math.h>
#include <glib.h>
#include <alsa/asoundlib.h>

#include "support-log.h"
#include "alsa.h"

#define ALSA_DEFAULT_CARD "(default)"
#define ALSA_DEFAULT_HCTL "default"

/*
 * Alsa log and debug macros.
 */

static void
alsa_log_msg(enum log_level level, const char *card,
             const char *strerr, const char *format, ...)
{
	va_list args;
	char buf[1024];

	if (!card && !strerr)
		snprintf(buf, sizeof buf, "%s", format);
	else if (card && !strerr)
		snprintf(buf, sizeof buf, "'%s': %s", card, format);
	else if (!card && strerr)
		snprintf(buf, sizeof buf, "%s: %s", format, strerr);
	else // card && strerr
		snprintf(buf, sizeof buf, "'%s': %s: %s", card, format, strerr);

	va_start(args, format);
	log_msg_v(level, __FILE__, buf, args);
	va_end(args);
}

#define ALSA_ERR(err, ...)	  \
	alsa_log_msg(LOG_ERROR, NULL, snd_strerror(err), __VA_ARGS__)

#define ALSA_CARD_DEBUG(card, ...)	\
	alsa_log_msg(LOG_DEBUG, card, NULL, __VA_ARGS__)

#define ALSA_CARD_WARN(card, ...)	\
	alsa_log_msg(LOG_WARN, card, NULL, __VA_ARGS__)

#define ALSA_CARD_ERR(card, err, ...)	\
	alsa_log_msg(LOG_ERROR, card, snd_strerror(err), __VA_ARGS__)

/*
 * Alsa mixer elem handling (deals with 'snd_mixer_elem_t').
 * Parts of this code were taken and adapted from the original
 * `alsa-utils` package, `alsamixer` program, `volume_mapping.c` file.
 *
 * Copyright (c) 2010 Clemens Ladisch <clemens@ladisch.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 */

#define MAX_LINEAR_DB_SCALE 24

static inline gboolean
use_linear_dB_scale(long db_min, long db_max)
{
	return db_max - db_min <= MAX_LINEAR_DB_SCALE * 100;
}

static long
lrint_dir(double x, int dir)
{
	if (dir > 0)
		return lrint(ceil(x));
	else if (dir < 0)
		return lrint(floor(x));
	else
		return lrint(x);
}

/* Return the name of a mixer element */
static const char *
elem_get_name(snd_mixer_elem_t *elem)
{
	return snd_mixer_selem_get_name(elem);
}

/* Get volume, return a value between 0 and 1 */
static double
elem_get_volume(const char *hctl, snd_mixer_elem_t *elem, double *volume)
{
	snd_mixer_selem_channel_id_t channel = SND_MIXER_SCHN_FRONT_RIGHT;
	int err;
	long min, max, value;

	*volume = 0;

	err = snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Can't get playback volume range");
		return FALSE;
	}

	if (min >= max) {
		ALSA_CARD_WARN(hctl, "Invalid playback volume range [%ld - %ld]", min, max);
		return FALSE;
	}

	err = snd_mixer_selem_get_playback_volume(elem, channel, &value);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Can't get playback volume");
		return FALSE;
	}

	*volume = (value - min) / (double) (max - min);

	return TRUE;
}

/* Set volume, input value between 0 and 1 */
static gboolean
elem_set_volume(const char *hctl, snd_mixer_elem_t *elem, double volume, int dir)
{
	int err;
	long min, max, value;

	err = snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Can't get playback volume range");
		return FALSE;
	}

	if (min >= max) {
		ALSA_CARD_WARN(hctl, "Invalid playback volume range [%ld - %ld]", min, max);
		return FALSE;
	}

	value = lrint_dir(volume * (max - min), dir) + min;

	err = snd_mixer_selem_set_playback_volume_all(elem, value);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Can't set playback volume to %ld", value);
		return FALSE;
	}

	return TRUE;
}

/* Get normalized volume, return a value between 0 and 1 */
static gboolean
elem_get_volume_normalized(const char *hctl, snd_mixer_elem_t *elem, double *volume)
{
	snd_mixer_selem_channel_id_t channel = SND_MIXER_SCHN_FRONT_RIGHT;
	int err;
	long min, max, value;
	double normalized, min_norm;

	*volume = 0;

	err = snd_mixer_selem_get_playback_dB_range(elem, &min, &max);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Can't get playback dB range");
		return FALSE;
	}

	if (min >= max) {
		ALSA_CARD_WARN(hctl, "Invalid playback dB range [%ld - %ld]", min, max);
		return FALSE;
	}

	err = snd_mixer_selem_get_playback_dB(elem, channel, &value);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Can't get playback dB");
		return FALSE;
	}

	if (use_linear_dB_scale(min, max)) {
		normalized = (value - min) / (double) (max - min);
	} else {
		normalized = exp10((value - max) / 6000.0);
		if (min != SND_CTL_TLV_DB_GAIN_MUTE) {
			min_norm = exp10((min - max) / 6000.0);
			normalized = (normalized - min_norm) / (1 - min_norm);
		}
	}

	*volume = normalized;

	// ALSA_CARD_DEBUG(hctl, "Getting normalized volume: %lf", *volume);

	return TRUE;
}

/* Set normalized volume, input value between 0 and 1 */
static gboolean
elem_set_volume_normalized(const char *hctl, snd_mixer_elem_t *elem, double volume, int dir)
{
	int err;
	long min, max, value;

	err = snd_mixer_selem_get_playback_dB_range(elem, &min, &max);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Can't get playback dB range");
		return FALSE;
	}

	if (min >= max) {
		ALSA_CARD_WARN(hctl, "Invalid playback dB range [%ld - %ld]", min, max);
		return FALSE;
	}

	if (use_linear_dB_scale(min, max)) {
		value = lrint_dir(volume * (max - min), dir) + min;
	} else {
		if (min != SND_CTL_TLV_DB_GAIN_MUTE) {
			double min_norm = exp10((min - max) / 6000.0);
			volume = volume * (1 - min_norm) + min_norm;
		}
		value = lrint_dir(6000.0 * log10(volume), dir) + max;
	}

	err = snd_mixer_selem_set_playback_dB_all(elem, value, dir);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Can't set playback dB to %ld", value);
		return FALSE;
	}

	// ALSA_CARD_DEBUG(hctl, "Normalized volume set: %lf (%d)", volume, dir);

	return TRUE;
}

/* Whether the card can be muted */
static gboolean
elem_has_mute(G_GNUC_UNUSED const char *hctl, snd_mixer_elem_t *elem)
{
	return snd_mixer_selem_has_playback_switch(elem) ? TRUE : FALSE;
}

/* Get the mute state, either TRUE or FALSE */
static gboolean
elem_get_mute(const char *hctl, snd_mixer_elem_t *elem, gboolean *muted)
{
	snd_mixer_selem_channel_id_t channel = SND_MIXER_SCHN_FRONT_RIGHT;
	int err;
	int value;

	*muted = FALSE;

	if (snd_mixer_selem_has_playback_switch(elem)) {
		err = snd_mixer_selem_get_playback_switch(elem, channel, &value);
		if (err < 0) {
			ALSA_CARD_ERR(hctl, err, "Can't get playback switch");
			return FALSE;
		}
	} else {
		/* If there's no playback switch, assume not muted */
		value = 1;
	}

	/* Value returned: 0 = muted, 1 = not muted */
	*muted = value == 0 ? TRUE : FALSE;

	// ALSA_CARD_DEBUG(hctl, "Getting mute: %d", *muted);

	return TRUE;
}

/* Set the mute state, TRUE or FALSE */
static gboolean
elem_set_mute(const char *hctl, snd_mixer_elem_t *elem, gboolean mute)
{
	int err;
	int value;

	/* Value to set: 0 = muted, 1 = not muted */
	value = mute ? 0 : 1;

	if (snd_mixer_selem_has_playback_switch(elem)) {
		err = snd_mixer_selem_set_playback_switch_all(elem, value);
		if (err < 0) {
			ALSA_CARD_ERR(hctl, err, "Can't set playback switch");
			return FALSE;
		}
	} else {
		/* If there's no playback switch, do nothing */
	}

	// ALSA_CARD_DEBUG(hctl, "Mute set: %d", mute);

	return TRUE;
}

/*
 * Alsa mixer handling (deals with 'snd_mixer_t').
 */

/* Get poll descriptors in a dynamic array (must be freed).
 * The array is terminated by a fd set to -1.
 */
static struct pollfd *
mixer_get_poll_descriptors(const char *hctl, snd_mixer_t *mixer)
{
	int err, count;
	struct pollfd *fds;

	/* Get the number of poll descriptors. Afaik, it's always one. */
	count = snd_mixer_poll_descriptors_count(mixer);

	/* Get the poll descriptors in a dynamic array */
	fds = g_new0(struct pollfd, count + 1);
	err = snd_mixer_poll_descriptors(mixer, fds, count);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Couldn't get poll descriptors");
		g_free(fds);
		return NULL;
	}

	/* Terminate the array with a fd set to -1 */
	fds[count].fd = -1;

	return fds;
}

/* Get the list of playable channels */
static GSList *
mixer_list_playable(G_GNUC_UNUSED const char *hctl, snd_mixer_t *mixer)
{
	GSList *list = NULL;
	snd_mixer_elem_t *elem = NULL;

	elem = snd_mixer_first_elem(mixer);
	while (elem) {
		if (snd_mixer_selem_has_playback_volume(elem)) {
			const char *chan_name = snd_mixer_selem_get_name(elem);
			list = g_slist_append(list, g_strdup(chan_name));
		}
		elem = snd_mixer_elem_next(elem);
	}

	return list;
}

/* Return TRUE if the mixer has at least one playable channel, FALSE otherwise */
static gboolean
mixer_is_playable(const char *hctl, snd_mixer_t *mixer)
{
	GSList *playable_chans;

	playable_chans = mixer_list_playable(hctl, mixer);
	if (playable_chans) {
		g_slist_free_full(playable_chans, g_free);
		return TRUE;
	}

	return FALSE;
}

/* Get a playable mixer element by name */
static snd_mixer_elem_t *
mixer_get_playable_elem(const char *hctl, snd_mixer_t *mixer, const char *channel)
{
	snd_mixer_elem_t *elem;
	snd_mixer_selem_id_t *sid;

	if (!channel)
		return NULL;

	ALSA_CARD_DEBUG(hctl, "Looking for playable mixer element '%s'", channel);

	/* Find the mixer element */
	snd_mixer_selem_id_alloca(&sid);
	snd_mixer_selem_id_set_name(sid, channel);
	elem = snd_mixer_find_selem(mixer, sid);
	if (elem == NULL) {
		ALSA_CARD_WARN(hctl, "Can't find mixer element '%s'", channel);
		return NULL;
	}

	/* Check that it's playable */
	if (!snd_mixer_selem_has_playback_volume(elem)) {
		ALSA_CARD_WARN(hctl, "Mixer element '%s' is not playable", channel);
		return NULL;
	}

	return elem;
}

/* Get the first playable mixer element */
static snd_mixer_elem_t *
mixer_get_first_playable_elem(const char *hctl, snd_mixer_t *mixer)
{
	snd_mixer_elem_t *elem;

	ALSA_CARD_DEBUG(hctl, "Looking for the first playable mixer element...");

	/* Iterate on mixer elements, get the first playable */
	elem = snd_mixer_first_elem(mixer);
	while (elem) {
		if (snd_mixer_selem_has_playback_volume(elem))
			break;
		elem = snd_mixer_elem_next(elem);
	}

	if (elem == NULL) {
		ALSA_CARD_DEBUG(hctl, "No playable mixer element found");
		return NULL;
	}

	return elem;
}

/* Close a mixer */
static void
mixer_close(const char *hctl, snd_mixer_t *mixer)
{
	int err;

	ALSA_CARD_DEBUG(hctl, "Closing mixer");

	snd_mixer_free(mixer);

	err = snd_mixer_detach(mixer, hctl);
	if (err < 0)
		ALSA_CARD_ERR(hctl, err, "Can't detach mixer");

	err = snd_mixer_close(mixer);
	if (err < 0)
		ALSA_CARD_ERR(hctl, err, "Can't close mixer");
}

/* Open a mixer */
static snd_mixer_t *
mixer_open(const char *hctl)
{
	int err;
	snd_mixer_t *mixer = NULL;

	ALSA_CARD_DEBUG(hctl, "Opening mixer");

	err = snd_mixer_open(&mixer, 0);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Can't open mixer");
		return NULL;
	}

	err = snd_mixer_attach(mixer, hctl);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Can't attach card to mixer");
		goto failure;
	}

	err = snd_mixer_selem_register(mixer, NULL, NULL);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Can't register mixer simple element");
		goto failure;
	}

	err = snd_mixer_load(mixer);
	if (err < 0) {
		ALSA_CARD_ERR(hctl, err, "Can't load mixer elements");
		goto failure;
	}

	return mixer;

failure:
	snd_mixer_close(mixer);
	return NULL;
}

/*
 * Alsa card iterator.
 * The Alsa API is really awkward when it comes to deal with cards
 * and to get various informations from it.
 * This iterator is here to make it less painful.
 */

struct alsa_card_iter {
	int number;
	char *name;
	char *hctl;
};

typedef struct alsa_card_iter AlsaCardIter;

/* Free an iterator */
static void
alsa_card_iter_free(AlsaCardIter *iter)
{
	if (iter == NULL)
		return;

	g_free(iter->name);
	g_free(iter->hctl);
	g_free(iter);
}

/* Create a new iterator */
static AlsaCardIter *
alsa_card_iter_new(void)
{
	AlsaCardIter *iter;

	iter = g_new0(AlsaCardIter, 1);
	iter->number = -2;

	return iter;
}

/* Iterate over alsa cards. Return TRUE as long as there is a card,
 * and FALSE when there's no more card.
 * After it returned FALSE, the iterator shouldn't be used anymore and
 * should be freed.
 */
static gboolean
alsa_card_iter_loop(AlsaCardIter *iter)
{
	int err;

	/* Free iter data at first */
	g_free(iter->name);
	iter->name = NULL;
	g_free(iter->hctl);
	iter->hctl = NULL;

	/* First elem is the default alsa soundcard.
	 * It's not really reachable as it with the ALSA API,
	 * so we must add it manually here.
	 */
	if (iter->number == -2) {
		iter->number = -1;
		iter->name = g_strdup(ALSA_DEFAULT_CARD);
		iter->hctl = g_strdup(ALSA_DEFAULT_HCTL);
		return TRUE;
	}

	/* Get next alsa soundcard */
	err = snd_card_next(&iter->number);
	if (err < 0) {
		ALSA_ERR(err, "Can't enumerate sound cards");
		return FALSE;
	}

	/* No more soundcards ? */
	if (iter->number < 0)
		return FALSE;

	/* Get card name */
	err = snd_card_get_name(iter->number, &(iter->name));
	if (err < 0) {
		ALSA_ERR(err, "Can't get card name");
		return FALSE;
	}

	/* Get HCTL name */
	iter->hctl = g_strdup_printf("hw:%d", iter->number);

	return TRUE;
}

/*
 * Alsa poll descriptors handling with GIO.
 */

/* Start watching the poll descriptors provided in the input array */
static guint *
watch_poll_descriptors(const char *hctl, struct pollfd *pollfds,
                       GIOFunc func, gpointer data)
{
	int nfds, i;
	guint *watch_ids;

	/* Count the number of poll file descriptors */
	nfds = i = 0;
	while (pollfds[i++].fd != -1)
		nfds++;

	/* Allocate a zero-termintated array to hold the watch ids */
	watch_ids = g_new0(guint, nfds + 1);

	/* Watch every poll fd */
	for (i = 0; i < nfds; i++) {
		GIOChannel *gioc;

		gioc = g_io_channel_unix_new(pollfds[i].fd);
		watch_ids[i] = g_io_add_watch(gioc, G_IO_IN | G_IO_ERR, func, data);
		g_io_channel_unref(gioc);
	}

	ALSA_CARD_DEBUG(hctl, "%d poll descriptors are now watched", nfds);

	return watch_ids;
}

/* Stop watching poll descriptors */
static void
unwatch_poll_descriptors(guint *watch_ids)
{
	int i;

	for (i = 0; watch_ids[i] != 0; i++) {
		g_source_remove(watch_ids[i]);
		watch_ids[i] = 0;
	}
}

/*
 * Public functions & signal handling
 */

struct alsa_card {
	gboolean normalize; /* Whether we work with normalized volume */
	/* Card names */
	char *name; /* Real card name like 'HDA Intel PCH' */
	char *hctl; /* HTCL device name, like 'hw:0' */
	/* Alsa data pointers */
	snd_mixer_t *mixer; /* Alsa mixer */
	snd_mixer_elem_t *mixer_elem; /* Alsa mixer elem */
	/* Gio watch ids */
	guint *watch_ids;
	/* User callback, to notify when something happens */
	AlsaCb cb_func;
	gpointer cb_data;
};

/**
 * Callback function for volume changes.
 * We forward changes to higher level, through a callback mechanism again.
 *
 * @param source the GIOChannel event source.
 * @param condition the condition which has been satisfied.
 * @param card data set in g_io_add_watch().
 * @return FALSE if the event source should be removed.
 */
static gboolean
poll_watch_cb(GIOChannel *source, GIOCondition condition, AlsaCard *card)
{
	gchar sbuf[256];
	gsize sread = 1;
	AlsaCb callback = card->cb_func;
	gpointer data = card->cb_data;

	// DEBUG("Entering %s()", __func__);

	/* Handle pending mixer events.
	 * Everything is broken if we don't do that !
	 */
	snd_mixer_handle_events(card->mixer);

	/* Check if the soundcard has been unplugged. In such case,
	 * the file descriptor we're watching disappeared, causing a G_IO_ERR.
	 */
	if (condition == G_IO_ERR) {
		if (callback)
			callback(ALSA_CARD_DISCONNECTED, data);
		return FALSE;
	}

	/* Now read data from channel */
	sread = 1;
	while (sread) {
		GIOStatus stat;

		/* This handles the case where mixer_elem_cb() doesn't read all
		 * the data on source. If we don't clear it out we'll go into an infinite
		 * callback loop since there will be data on the channel forever.
		 */
		stat = g_io_channel_read_chars(source, sbuf, 256, &sread, NULL);

		switch (stat) {
		case G_IO_STATUS_AGAIN:
			/* Normal, means alsa_cb cleared out the channel */
			continue;

		case G_IO_STATUS_NORMAL:
			/* Actually bad, alsa failed to clear channel */
			WARN("Alsa failed to clear the channel");
			if (callback)
				callback(ALSA_CARD_RELOAD, data);
			break;

		case G_IO_STATUS_ERROR:
		case G_IO_STATUS_EOF:
			ERROR("GIO error has occurred");
			if (callback)
				callback(ALSA_CARD_ERROR, data);
			break;

		default:
			WARN("Unknown status from g_io_channel_read_chars()");
		}

		return TRUE;
	}

	/* Arriving here, no errors happened.
	 * We can safely notify that values changed.
	 */
	if (callback)
		callback(ALSA_CARD_VALUES_CHANGED, data);

	return TRUE;
}

/**
 * Get the name of the card.
 * This is an internal string that shouldn't be modified.
 *
 * @param card a Card instance.
 * @return the name of the card.
 */
const char *
alsa_card_get_name(AlsaCard *card)
{
	return card->name;
}

/**
 * Get the name of the channel.
 * This is an internal string that shouldn't be modified.
 *
 * @param card a Card instance.
 * @return the name of the channel.
 */
const char *
alsa_card_get_channel(AlsaCard *card)
{
	return elem_get_name(card->mixer_elem);
}

/**
 * Whether the card has mute capabilities.
 *
 * @param card a Card instance.
 * @return TRUE if the card can be muted, FALSE otherwise.
 */
gboolean
alsa_card_has_mute(AlsaCard *card)
{
	return elem_has_mute(card->hctl, card->mixer_elem);
}

/**
 * Get the mute state, either TRUE or FALSE.
 *
 * @param card a Card instance.
 * @return TRUE if the card is muted, FALSE otherwise.
 */
gboolean
alsa_card_is_muted(AlsaCard *card)
{
	gboolean muted;

	elem_get_mute(card->hctl, card->mixer_elem, &muted);
	return muted;
}

/**
 * Toggle the mute state.
 *
 * @param card a Card instance.
 */
void
alsa_card_toggle_mute(AlsaCard *card)
{
	gboolean muted;

	/* Set mute */
	muted = alsa_card_is_muted(card);
	elem_set_mute(card->hctl, card->mixer_elem, !muted);
}

/**
 * Get the volume in percent (value between 0 and 100).
 *
 * @param card a Card instance.
 * @return the volume in percent.
 */
gdouble
alsa_card_get_volume(AlsaCard *card)
{
	gdouble volume = 0;
	gboolean gotten = FALSE;

	if (card->normalize)
		gotten = elem_get_volume_normalized(card->hctl, card->mixer_elem, &volume);

	if (!gotten)
		elem_get_volume(card->hctl, card->mixer_elem, &volume);

	return volume * 100;
}

/**
 * Set the volume in percent (value between 0 and 100).
 *
 * @param card a Card instance.
 * @param value the volume in percent.
 * @param dir the direction of the volume change
 *        (-1: lowering, +1: raising, 0: setting).
 */
void
alsa_card_set_volume(AlsaCard *card, gdouble value, int dir)
{
	gdouble volume;
	gboolean set = FALSE;

	volume = value / 100.0;

	/* Set volume */
	if (card->normalize)
		set = elem_set_volume_normalized(card->hctl, card->mixer_elem, volume, dir);

	if (!set)
		elem_set_volume(card->hctl, card->mixer_elem, volume, dir);
}

/**
 * Set a callback invoked on volume/mute changes.
 *
 * @param card a Card instance.
 * @param callback the callback to be invoked.
 * @param user_data the user data passed to the callback.
 */
void
alsa_card_install_callback(AlsaCard *card, AlsaCb callback, gpointer user_data)
{
	card->cb_func = callback;
	card->cb_data = user_data;
}

/**
 * Free a card instance, therefore closing mixer and freeing any allocated ressources.
 *
 * @param card a Card instance.
 */
void
alsa_card_free(AlsaCard *card)
{
	if (card == NULL)
		return;

	if (card->watch_ids)
		unwatch_poll_descriptors(card->watch_ids);

	if (card->mixer)
		mixer_close(card->hctl, card->mixer);

	g_free(card->hctl);
	g_free(card->name);
	g_free(card);
}

/**
 * Create a new Card instance.
 * Look for the selected card among the available cards.
 * If found, open it, and look for the selected channel.
 * If found, all is well, the card is ready to be used.
 * Otherwise, NULL is returned.
 *
 * @param card_name the name of the card, or NULL to use the default card.
 * @param channel the name of the channel, or NULL to use the first playable channel.
 * @param normalize whether we use normalized volume or not.
 * @return a newly allocated Card instance, or NULL on failure.
 */
AlsaCard *
alsa_card_new(const char *card_name, const char *channel, gboolean normalize)
{
	AlsaCard *card;
	AlsaCardIter *iter;

	card = g_new0(AlsaCard, 1);

	/* Save normalize parameter */
	card->normalize = normalize;

	/* Save card name */
	if (!card_name)
		card_name = ALSA_DEFAULT_CARD;
	card->name = g_strdup(card_name);

	/* Get corresponding HCTL name */
	iter = alsa_card_iter_new();
	while (alsa_card_iter_loop(iter)) {
		if (!g_strcmp0(iter->name, card_name)) {
			card->hctl = g_strdup(iter->hctl);
			break;
		}
	}
	alsa_card_iter_free(iter);

	if (card->hctl == NULL)
		goto failure;

	/* Open mixer */
	card->mixer = mixer_open(card->hctl);
	if (card->mixer == NULL)
		goto failure;

	/* Get mixer element */
	card->mixer_elem = mixer_get_playable_elem(card->hctl, card->mixer, channel);
	if (card->mixer_elem == NULL)
		card->mixer_elem = mixer_get_first_playable_elem(card->hctl, card->mixer);
	if (card->mixer_elem == NULL)
		goto failure;

	/* Get mixer poll descriptors and watch them using gio.
	 * That's how we get notified from every volume/mute changes,
	 * may it be external or due to PNMixer.
	 */
	struct pollfd *pollfds;
	pollfds = mixer_get_poll_descriptors(card->hctl, card->mixer);
	if (pollfds == NULL)
		goto failure;

	card->watch_ids = watch_poll_descriptors(card->hctl, pollfds,
	                  (GIOFunc) poll_watch_cb, card);
	g_free(pollfds);

	/* Sum up the situation */
	DEBUG("'%s': Card '%s' with channel '%s' initialized !",
	      card->hctl, card->name, elem_get_name(card->mixer_elem));

	return card;

failure:
	alsa_card_free(card);
	return NULL;
}

/*
 * Alsa listing functions
 */

/**
 * Return the list of playable cards as a GSList.
 * Must be freed using g_slist_free_full() and g_free().
 *
 * @return a list of playable cards.
 */
GSList *
alsa_list_cards(void)
{
	AlsaCardIter *iter;
	GSList *list = NULL;

	iter = alsa_card_iter_new();

	while (alsa_card_iter_loop(iter)) {
		snd_mixer_t *mixer;

		/* Open mixer */
		mixer = mixer_open(iter->hctl);
		if (mixer == NULL)
			continue;

		/* Only keep cards with playable channels */
		if (mixer_is_playable(iter->hctl, mixer))
			list = g_slist_append(list, g_strdup(iter->name));

		/* Close mixer */
		mixer_close(iter->hctl, mixer);
	}

	alsa_card_iter_free(iter);

	return list;
}

/**
 * For a given card name, return the list of playable channels as a GSList.
 * Must be freed using g_slist_free_full() and g_free().
 *
 * @param card_name the name of the card for which we list the channels
 * @return a list of playable channels.
 */
GSList *
alsa_list_channels(const char *card_name)
{
	AlsaCardIter *iter;
	char *hctl = NULL;
	snd_mixer_t *mixer = NULL;
	GSList *list = NULL;

	/* Iterate over cards to find the one provided in argument */
	iter = alsa_card_iter_new();
	while (alsa_card_iter_loop(iter)) {
		if (!g_strcmp0(iter->name, card_name)) {
			hctl = g_strdup(iter->hctl);
			break;
		}
	}
	alsa_card_iter_free(iter);

	if (hctl == NULL)
		goto exit;

	/* Open the mixer */
	mixer = mixer_open(hctl);
	if (mixer == NULL)
		goto exit;

	/* Get a list of playable channels */
	list = mixer_list_playable(hctl, mixer);

exit:
	/* Cleanup */
	if (mixer)
		mixer_close(hctl, mixer);
	if (hctl)
		g_free(hctl);

	return list;
}
