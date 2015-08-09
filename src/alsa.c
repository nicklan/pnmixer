/* alsa.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/*
 * ALSA volume normalization code adapted from original alsa source:
 *
 *    volume_mapping.c
 *
 * Copyright (c) 2010 Clemens Ladisch <clemens@ladisch.de>
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 */

/**
 * @file alsa.c
 * This file holds the communication of pnmixer
 * with alsa, such as getting available cards as well as
 * setting callback functions for events, and so on.
 * @brief alsa subsystem
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#define _GNU_SOURCE
#include "alsa.h"
#include "main.h"
#include "notify.h"
#include "prefs.h"

#include <math.h>
#include <alsa/asoundlib.h>
#include <string.h>

#define MAX_LINEAR_DB_SCALE	24

static inline gboolean use_linear_dB_scale(long dBmin, long dBmax){
	return dBmax - dBmin <= MAX_LINEAR_DB_SCALE * 100;
}
static int smixer_level = 0;
static struct snd_mixer_selem_regopt smixer_options;
static snd_mixer_elem_t *elem;
static snd_mixer_t *handle;
struct acard *active_card;

static GSList* get_channels(const char* card);

static long lrint_dir(double x, int dir)
{
	if (dir > 0)
		return lrint(ceil(x));
	else if (dir < 0)
		return lrint(floor(x));
	else
		return lrint(x);
}

/**
 * Callback function which is called on an element
 * of the alsa cards GSList, e.g. via g_slist_free_full.
 *
 * @param data the current alsa card
 */
static void card_free(gpointer data) {
  struct acard* c = (struct acard*)data;
  g_free(c->name);
  g_free(c->dev);
  g_slist_free_full(c->channels,g_free);
  g_free(data);
}

/**
 * Partly based on get_cards function in alsamixer.
 * This gets all alsa cards and fills the global
 * GSList 'cards'.
 * The list always starts with the 'default' card.
 */
static void get_cards() {
  int err, num;
  snd_ctl_card_info_t *info;
  snd_ctl_t *ctl;
  char buf[10];
  struct acard *cur_card, *default_card;

  if (cards != NULL)
    g_slist_free_full(cards,card_free);

  cards = NULL;

  default_card = g_malloc(sizeof(struct acard));
  default_card->name = g_strdup("(default)");
  default_card->dev = g_strdup("default");
  default_card->channels = get_channels("default");

  cards = g_slist_append(cards,default_card);

  // don't need to free this as it's alloca'd
  snd_ctl_card_info_alloca(&info);
  num = -1;
  for (;;) {
    err = snd_card_next(&num);
    if (err < 0) {
      report_error("Can't get sounds cards: %s\n",snd_strerror(err));
      return;
    }
    if (num < 0)
      break;
    sprintf(buf, "hw:%d", num);
    if (snd_ctl_open(&ctl,buf, 0) < 0)
      continue;
    err = snd_ctl_card_info(ctl,info);
    snd_ctl_close(ctl);
    if (err < 0)
      continue;
    cur_card = g_malloc(sizeof(struct acard));
    cur_card->name = g_strdup(snd_ctl_card_info_get_name(info));
    sprintf(buf,"hw:%d",num);
    cur_card->dev = g_strdup(buf);
    cur_card->channels = get_channels(buf);
    cards = g_slist_append(cards,cur_card);
  }
#ifdef DEBUG
  GSList *tmp = cards;
  if (tmp) {
    printf("------ Card list ------\n");
    while (tmp) {
      struct acard *c = tmp->data;
      printf("\t%s\t%s\t%s\n", c->dev, c->name, c->channels ? "" : "No chann");
      tmp = tmp->next;
    }
    printf("-----------------------\n");
  }
#endif
}

/**
 * Get the acard struct corresponding to a card name
 * by searching the global GSList 'cards'.
 *
 * @param card_name the card name to get the device string of
 * @return a pointer toward the corresponding acard struct or NULL on failure
 */
struct acard *find_card(const char* card_name) {
  GSList *item;
  
  if (!card_name)
    return NULL;

  for (item = cards; item; item = item->next) {
    struct acard *c = item->data;
    if (!strcmp(c->name, card_name))
      return c;
  }

  return NULL;
}

/**
 * Opens the mixer, attaches the alsa card to it,
 * registers the mixer simple element class and
 * loads the mixer elements.
 *
 * @param card HCTL name
 * @param opts Options container
 * @param level mixer level
 * @return the mixer handle, or NULL on failure
 */
static snd_mixer_t *open_mixer(const char *card,
		struct snd_mixer_selem_regopt* opts,
		int level) {
  int err;
  snd_mixer_t *mixer = NULL;

  DEBUG_PRINT("Card %s: opening mixer",card);

  if ((err = snd_mixer_open(&mixer, 0)) < 0) {
    DEBUG_PRINT("Card %s: mixer open error: %s", card, snd_strerror(err));
    return NULL;
  }
  if (level == 0 && (err = snd_mixer_attach(mixer, card)) < 0) {
    DEBUG_PRINT("Card %s: mixer attach error: %s", card, snd_strerror(err));
    snd_mixer_close(mixer);
    return NULL;
  }
  if ((err = snd_mixer_selem_register(mixer, level > 0 ? opts : NULL, NULL)) < 0) {
    DEBUG_PRINT("Card %s: mixer register error: %s", card, snd_strerror(err));
    snd_mixer_close(mixer);
    return NULL;
  }
  if ((err = snd_mixer_load(mixer)) < 0) {
    DEBUG_PRINT("Card %s: mixer load error: %s", card, snd_strerror(err));
    snd_mixer_close(mixer);
    return NULL;
  }

  return mixer;
}

/**
 * Callback function for the mixer element which is
 * set in alsaset().
 *
 * @param e mixer element
 * @param mask event mask
 * @return 0 on success otherwise a negative error code
 */
static int alsa_cb(snd_mixer_elem_t *e, unsigned int mask) {
  /* Test MASK_REMOVE first, according to Alsa documentation */
  if (mask == SND_CTL_EVENT_MASK_REMOVE) {
    return 0;
  }

  /* Then check if mixer value changed */
  if (mask & SND_CTL_EVENT_MASK_VALUE) {
    int muted;
    get_current_levels();
    muted = get_mute_state(TRUE);
    if (enable_noti && external_noti) {
      int vol = getvol();
      if (muted)
	do_notify_volume(vol,FALSE);
      else
	do_notify_volume(vol,TRUE);
    }
  }

  return 0;
}

/**
 * We need to re-init alsa in an idle moment, it doesn't seem
 * very safe to do that while handling data in poll_cb().
 * This function is attached via g_idle_add() in poll_cb().
 *
 * @param data passed to the function,
 * set when the source was created
 * @return FALSE if the source should be removed,
 * TRUE otherwise
 */
static gboolean idle_alsa_reinit(gpointer data) {
  do_alsa_reinit();
  return FALSE;
}

static gchar sbuf[256];
static GIOChannelError *serr = NULL;
static gsize sread = 1;

/**
 * Callback function for external volume changes,
 * set in set_io_watch().
 *
 * @param source the GIOChannel event source
 * @param condition the condition which has been satisfied
 * @param data user data set inb g_io_add_watch() or g_io_add_watch_full()
 * @return FALSE if the event source should be removed
 */
static gboolean poll_cb(GIOChannel *source,
		GIOCondition condition,
		gpointer data) {
  snd_mixer_handle_events(handle);

  if (condition == G_IO_ERR) {
    /* This happens when the file descriptor we're watching disappeared.
     * For example, if the USB soundcard has been unplugged.
     * In this case, reloading alsa is the nice thing to do, it will
     * cause PNMixer to select the first card available.
     */
    do_notify_text(_("Soundcard disconnected"),
      _("Soundcard has been disconnected, reloading Alsa..."));
    g_idle_add(idle_alsa_reinit, NULL);
    return FALSE;
  }
  sread = 1;
  while (sread) {
    /* This handles the case where alsa_cb doesn't read all the data on source.
       If we don't clear it out we'll go into an infinite callback loop since there
       will be data on the channel forever */
    GIOStatus stat = g_io_channel_read_chars(source,sbuf,256,&sread,(GError**)&serr);
    if (serr) {
      g_error_free((GError*)serr);
      serr = NULL;
    }
    if (stat == G_IO_STATUS_AGAIN) // normal, means alsa_cb cleared out the channel
      continue;
    else if(stat == G_IO_STATUS_NORMAL) // actually bad, alsa failed to clear channel
      warn_sound_conn_lost();
    else if (stat == G_IO_STATUS_ERROR ||
			stat == G_IO_STATUS_EOF)
      report_error("Error: GIO error has occured.  Won't respond to external volume changes anymore\n");
    else
      report_error("Error: Unknown status from g_io_channel_read_chars\n");
    return TRUE;
  }
  return TRUE;
}

// PCOUNT_MAX is a very arbitrary value.
// The number of poll descriptors I witnessed has always been one.
#define PCOUNT_MAX 8
guint gio_watch_ids[PCOUNT_MAX] = { 0 };

/**
 * Sets the io watch for external volume changes
 * and registers the poll_cb() callback function.
 *
 * @param mixer mixer handle
 */
static void set_io_watch(snd_mixer_t *mixer) {
  int i, pcount;
  struct pollfd fds[PCOUNT_MAX];
  
  pcount = snd_mixer_poll_descriptors_count(mixer);
  assert(pcount <= PCOUNT_MAX);
  pcount = snd_mixer_poll_descriptors(mixer, fds, pcount);

  if (pcount <= 0) {
    report_error("Warning: Couldn't get any poll descriptors. "
      "Won't respond to external volume changes");
    return;
  }

  for (i = 0; i < pcount; i++) {
    GIOChannel *gioc = g_io_channel_unix_new(fds[i].fd);
    gio_watch_ids[i] = g_io_add_watch(gioc, G_IO_IN|G_IO_ERR, poll_cb, NULL);
    g_io_channel_unref(gioc);
  }
}

/**
 * Unsets the io watch.
 */
static void unset_io_watch(void) {
  int i;
  for (i = 0; i < PCOUNT_MAX; i++) {
    if (gio_watch_ids[i] == 0)
      break;
    g_source_remove(gio_watch_ids[i]);
    gio_watch_ids[i] = 0;
  }
}

/**
 * Detaches a mixer from the specified card and closes the
 * mixer.
 *
 * @param mixer mixer handle
 * @param card HCTL name of the alsa card
 * @return 0 on success otherwise negative error code
 */
static int close_mixer(snd_mixer_t *mixer, const char* card) {
  int err;

  DEBUG_PRINT("Card %s: closing mixer",card);

  if ((err = snd_mixer_detach(mixer,card)) < 0)
    report_error("Card %s: mixer detach error: %s", card, snd_strerror(err));
  snd_mixer_free(mixer);
  if ((err = snd_mixer_close(mixer)) < 0)
    report_error("Card %s: mixer close error: %s", card, snd_strerror(err));
  return err;
}

/**
 * Get all playable channels for a single alsa card and
 * return them as a GSList.
 *
 * @param card HCTL name of the alsa card
 * @return the GSList of channels
 */
static GSList* get_channels(const char* card) {
  int ccount,i;
  snd_mixer_t *mixer;
  snd_mixer_elem_t *telem;
  GSList *channels = NULL;

  mixer = open_mixer(card,NULL,0);
  if (mixer == NULL)
    return NULL;

  ccount = snd_mixer_get_count(mixer);
  telem = snd_mixer_first_elem(mixer);

  for(i = 0;i < ccount;i++) {
    if(snd_mixer_selem_has_playback_volume(telem))
      channels = g_slist_append(channels,strdup(snd_mixer_selem_get_name(telem)));
    telem = snd_mixer_elem_next(telem);
  }

#ifdef DEBUG
  GSList *tmp = channels;
  if (tmp) {
    printf("Card %s: available channels\n",card);
    while (tmp) {
      printf("\t%s\n",(char*)tmp->data);
      tmp = tmp->next;
    }
  } else {
    printf("Card %s: no playable channels\n",card);
  }
#endif

  close_mixer(mixer,card);
  
  return channels;
}

/**
 * Initializes the alsa system by getting the cards
 * and channels and setting the io watch for external
 * volume changes.
 *
 * @return 0 on success otherwise negative error code
 */
static int alsaset() {
  char *card_name;
  char *channel;

  // update list of available cards
  DEBUG_PRINT("Getting available cards...");
  get_cards();
  assert(cards != NULL);

  // get selected card
  assert(active_card == NULL);
  card_name = get_selected_card();
  DEBUG_PRINT("Selected card: %s", card_name);
  if (card_name) {
    active_card = find_card(card_name);
    g_free(card_name);
  }

  // if not available, use the default card
  if (!active_card) {
    DEBUG_PRINT("Using default soundcard");
    active_card = cards->data;
  }

  // If no playable channels, iterate on card list until a valid card is found.
  // In most situations, the first card of the list (which is the
  // Alsa default card) can be opened.
  // However, in some situations the default card may be unavailable.
  // For example, when it's an USB DAC, and it's disconnected.
  if (!active_card->channels) {
    GSList *item;
    DEBUG_PRINT("Card '%s' has no playable channels, iterating on card list",
      active_card->dev);
    for (item = cards; item; item = item->next) {
      active_card = item->data;
      if (active_card->channels)
	break;
    }
    assert(item != NULL);
  }
  
  // open card
  DEBUG_PRINT("Opening card '%s'...", active_card->dev);
  smixer_options.device = active_card->dev;
  handle = open_mixer(active_card->dev, &smixer_options, smixer_level);
  assert(handle != NULL);

  // Set the channel
  // We have to handle the fact that the channel name defined
  // in PNMixer configuration is not necessarily valid.
  // For example, this may happen when the default alsa soundcard
  // is modified. The channel names of the new default card may
  // not match the channel names of the previous default card.
  assert(elem == NULL);
  channel = get_selected_channel(active_card->name);
  if (channel) {
    snd_mixer_selem_id_t *sid;
    snd_mixer_selem_id_alloca(&sid);
    snd_mixer_selem_id_set_name(sid, channel);
    elem = snd_mixer_find_selem(handle, sid);
    g_free(channel);
  }
  if (elem == NULL) {
    elem = snd_mixer_first_elem(handle);
  }
  assert(elem != NULL);

  // Set callback
  DEBUG_PRINT("Using channel '%s'", snd_mixer_selem_get_name(elem));
  snd_mixer_elem_set_callback(elem, alsa_cb);

  // set watch for volume changes
  set_io_watch(handle);

  return 0;
}

/**
 * Deinitializes the alsa system by
 * closing the mixer.
 */
static void alsaunset() {
  if (active_card == NULL)
    return;
    
  unset_io_watch();

  // 'elem' must be set to NULL at last, because alsa_cb()
  // is invoked when closing mixer, and elem is needed.
  close_mixer(handle, active_card->dev);
  handle = NULL;
  elem = NULL;
  active_card = NULL;
}

/**
 * Get the normalized current volume.
 *
 * @param elem current mixer element
 * @param channel current channel
 * @return normalized volume
 */
static double get_normalized_volume(snd_mixer_elem_t *elem,
		snd_mixer_selem_channel_id_t channel) {
    long min, max, value;
    double normalized, min_norm;
    int err;

    err = snd_mixer_selem_get_playback_dB_range(elem, &min, &max);
    if (err < 0 || min >= max) {
        err = snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
        if (err < 0 || min == max)
            return 0;

        err = snd_mixer_selem_get_playback_volume(elem, channel, &value);
        if (err < 0)
            return 0;

        return (value - min) / (double)(max - min);
    }

    err = snd_mixer_selem_get_playback_dB(elem, channel, &value);
    if (err < 0)
        return 0;

    if (use_linear_dB_scale(min, max))
        return (value - min) / (double)(max - min);

    normalized = exp10((value - max) / 6000.0);
    if (min != SND_CTL_TLV_DB_GAIN_MUTE) {
        min_norm = exp10((min - max) / 6000.0);
        normalized = (normalized - min_norm) / (1 - min_norm);
    }

    return normalized;
}

/**
 * Converts the current volume in the real volume range
 * reported by snd_mixer_selem_get_playback_volume_range()
 * into the 0-100 range.
 *
 * @param val current volume value
 * @param min current minimum volume
 * @param max current maximum volume
 * @return volume converted into 0-100 range
 */
static int convert_prange(long val, long min, long max) {
  long range = max - min;
  if (range == 0)
    return 0;
  val -= min;
  return rint(val/(double)range * 100);
}

/**
 * Adjusts the current volume and sends a notification (if enabled).
 *
 * @param vol new volume value
 * @param dir select direction (-1 = accurate or first bellow, 0 = accurate,
 * 1 = accurate or first above)
 * @param notify whether to send notification
 * @return 0 on success otherwise negative error code
 */
int setvol(int vol, int dir, gboolean notify) {
  long min = 0, max = 0, value;
  int cur_perc = getvol();
  double dvol = 0.01 * vol;

  int err = snd_mixer_selem_get_playback_dB_range(elem, &min, &max);
  if (err < 0 || min >= max || !normalize_vol()) {
    err = snd_mixer_selem_get_playback_volume_range(elem, &min, &max);
    value = lrint_dir(dvol * (max - min), dir) + min;
    snd_mixer_selem_set_playback_volume_all(elem, value);
    if (enable_noti && notify && cur_perc != getvol())
      do_notify_volume(getvol(),FALSE);
    return snd_mixer_selem_set_playback_volume_all(elem, value); // intentionally set twice
  }

  if (use_linear_dB_scale(min, max)) {
    value = lrint_dir(dvol * (max - min), dir) + min;
    return snd_mixer_selem_set_playback_dB_all(elem, value, dir);
  }

  if (min != SND_CTL_TLV_DB_GAIN_MUTE) {
    double min_norm = exp10((min - max) / 6000.0);
    dvol = dvol * (1 - min_norm) + min_norm;
  }

  value = lrint_dir(6000.0 * log10(dvol), dir) + max;
  snd_mixer_selem_set_playback_dB_all(elem, value, dir);
  if (enable_noti && notify && cur_perc != getvol())
    do_notify_volume(getvol(),FALSE);
  return snd_mixer_selem_set_playback_dB_all(elem, value, dir); // intentionally set twice
}

/**
 * Mutes or unmutes playback and sends a notification (if enabled).
 *
 * @param notify whether to send notification
 */
void setmute(gboolean notify) {
  if (!snd_mixer_selem_has_playback_switch(elem))
    return;
  if (ismuted()) {
    snd_mixer_selem_set_playback_switch_all(elem, 0);
    if (enable_noti && notify)
      do_notify_volume(getvol(),TRUE);
  }
  else {
    snd_mixer_selem_set_playback_switch_all(elem, 1);
    if (enable_noti && notify)
      do_notify_volume(getvol(),FALSE);
  }
}

/**
 * Check whether sound is currently muted.
 *
 * @return 0 if mixer is muted, 1 otherwise
 */
int ismuted() {
  int muted = 1;
  if (snd_mixer_selem_has_playback_switch(elem))
    snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &muted);
  return muted;
}

/**
 * Gets the current volume in the range from 0 - 100.
 *
 * @return current volume
 */
int getvol() {
  if (normalize_vol()) {
      return lrint(get_normalized_volume(elem,SND_MIXER_SCHN_FRONT_RIGHT)*100);
  } else {
      long val, pmin = 0, pmax = 0;
      snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);
      snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &val);
      DEBUG_PRINT("[getvol] From mixer: %li  pmin: %li  pmax: %li",val,pmin,pmax);
      return convert_prange(val,pmin,pmax);
  }
}

/**
 * Initializes the alsa system. Deinitializes first
 * if we want to re-initialize.
 */
void alsa_init() {
  if (active_card) // re-init, need to close down first
    alsaunset();
  alsaset();
}

/**
 * Closes the alsa mixer handle.
 */
void alsa_close() {
  snd_mixer_close(handle);
}

/**
 * Get the card in use.
 *
 * @return a pointer toward the active card
 */
struct acard *alsa_get_active_card() {
  return active_card;
}

/**
 * Get the channel name in use.
 *
 * @return the channel name
 */
const char *alsa_get_active_channel() {
  return elem ? snd_mixer_selem_get_name(elem) : NULL;
}
