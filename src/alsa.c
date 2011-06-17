/* alsa.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include "alsa.h"
#include "main.h"
#include "prefs.h"

#include <math.h>
#include <alsa/asoundlib.h>

static int smixer_level = 0;
static struct snd_mixer_selem_regopt smixer_options;
static snd_mixer_elem_t *elem;
static snd_mixer_t *handle;

static GSList* get_channels(gchar* card);

// partly based on get_cards function in alsamixer
static void get_cards() {
  int err, num;
  snd_ctl_card_info_t *info;
  snd_ctl_t *ctl;
  char buf[10];
  struct acard *cur_card, *default_card;
  
  cards = NULL;

  default_card = malloc(sizeof(struct acard));
  default_card->name = "(default)";
  default_card->dev = "default";
  default_card->channels = get_channels("default");

  cards = g_slist_append(cards,default_card);

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
    cur_card = malloc(sizeof(struct acard));
    cur_card->name = strdup(snd_ctl_card_info_get_name(info));
    sprintf(buf,"hw:%d",num);
    cur_card->dev = strdup(buf);
    cur_card->channels = get_channels(buf);
    cards = g_slist_append(cards,cur_card);
  }
}

// TODO: Warn when selected card can't be found
char* selected_card_dev(gchar* selected_card) {
  gchar *ret = NULL;
  struct acard* c;
  if (selected_card) {
    GSList *cur_card = cards;
    while (cur_card) {
      c = cur_card->data;
      if (!strcmp(c->name,selected_card)) {
	ret = c->dev;
	break;
      }
      cur_card = cur_card->next;
    }
  }
  if (!ret) {
    c = cards->data;
    ret = c->dev;
  }
  return ret;
}

static int open_mixer(snd_mixer_t **mixer, char* card, struct snd_mixer_selem_regopt* opts,int level) {
  int err;


  DEBUG_PRINT("Opening mixer for card: %s\n",card);


  if ((err = snd_mixer_open(mixer, 0)) < 0) {
    report_error("Mixer %s open error: %s", card, snd_strerror(err));
    return err;
  }
  if (level == 0 && (err = snd_mixer_attach(*mixer, card)) < 0) {
    report_error("Mixer attach %s error: %s", card, snd_strerror(err));
    snd_mixer_close(*mixer);
    return err;
  }
  if ((err = snd_mixer_selem_register(*mixer, level > 0 ? opts : NULL, NULL)) < 0) {
    report_error("Mixer register error: %s", snd_strerror(err));
    snd_mixer_close(*mixer);
    return err;
  }
  if ((err = snd_mixer_load(*mixer)) < 0) {
    report_error("Mixer %s load error: %s", card, snd_strerror(err));
    snd_mixer_close(*mixer);
    return err;
  }
  return 0;
}

static int alsa_cb(snd_mixer_elem_t *e, unsigned int mask) {
  get_current_levels();
  get_mute_state();
  return 0;
}


static gchar sbuf[256];
static GIOChannelError *serr = NULL;
static gsize sread = 1;
static gboolean poll_cb(GIOChannel *source, GIOCondition condition, gpointer data) {
  snd_mixer_handle_events(handle);

  sread = 1;
  while (sread) {
    /* This handles the case where alsa_cb doesn't read all the data on source.
       If we don't clear it out we'll go into an infinite callback loop since there
       will be data on the channel forever */
    GIOStatus stat = g_io_channel_read_chars(source,sbuf,256,&sread,(GError**)&serr);
    if (stat == G_IO_STATUS_AGAIN) // normal, means alsa_cb cleared out the channel
      continue;
    else if(stat == G_IO_STATUS_NORMAL) // actually bad, alsa failed to clear channel
      report_error("Warning: Connection to sound system failed, you probably need to restart pnmixer\n");
    else if (stat == G_IO_STATUS_ERROR || G_IO_STATUS_EOF)
      report_error("Error: GIO error has occured.  Won't respond to external volume changes anymore\n");
    else
      report_error("Error: Unknown status from g_io_channel_read_chars\n");
  }
  return TRUE;
}

static void set_io_watch(snd_mixer_t *mixer) {
  int pcount;

  pcount = snd_mixer_poll_descriptors_count(mixer);
  if (pcount) {
    int i;
    struct pollfd fds[pcount];
    pcount = snd_mixer_poll_descriptors(mixer,fds,pcount);
    if (pcount <= 0)
      report_error("Warning: Couldn't get any poll descriptors.  Won't respond to external volume changes");
    for (i = 0;i < pcount;i++) {
      GIOChannel *gioc = g_io_channel_unix_new(fds[i].fd);
      g_io_add_watch(gioc,G_IO_IN,poll_cb,NULL);
    }
  }
}

static int close_mixer(snd_mixer_t **mixer) {
  int err;
  if ((err = snd_mixer_close(*mixer)) < 0) 
    report_error("Mixer close error: %s", snd_strerror(err));
  return err;
}


static GSList* get_channels(gchar* card) {
  int ccount,i;
  snd_mixer_t *mixer;
  snd_mixer_elem_t *telem;
  GSList *channels = NULL;

  open_mixer(&mixer,card,NULL,0);

  ccount = snd_mixer_get_count(mixer);
  telem = snd_mixer_first_elem(mixer);

  for(i = 0;i < ccount;i++) {
    if(snd_mixer_selem_has_playback_volume(telem))
      channels = g_slist_append(channels,strdup(snd_mixer_selem_get_name(telem)));
    telem = snd_mixer_elem_next(telem);
  }

  close_mixer(&mixer);

#ifdef DEBUG
  GSList *tmp = channels;
  if (tmp) {
    printf("Channels for card: %s\n",card);
    while (tmp) {
      printf("\t%s\n",(char*)tmp->data);
      tmp = tmp->next;
    }
  } else {
    printf("%s has no playable channels\n",card);
  }
#endif

  return channels;
}

static int alsaset() {
  snd_mixer_selem_id_t *sid;
  gchar *card,*channel;
  char *card_dev;

  get_cards();
  card = get_selected_card();
  card_dev = selected_card_dev(card);
  smixer_options.device = card_dev;

  open_mixer(&handle,card_dev,&smixer_options,smixer_level);

  // set watch for volume changes
  set_io_watch(handle);

  // now set the channel
  snd_mixer_selem_id_alloca(&sid);
  channel = get_selected_channel(card);
  if (channel == NULL)
    elem = snd_mixer_first_elem(handle);
  else {
    snd_mixer_selem_id_set_name(sid, channel);
    elem = snd_mixer_find_selem(handle, sid);
    g_free(channel);
  }
  assert(elem != NULL);
  snd_mixer_elem_set_callback(elem, alsa_cb);

  if (card)
    g_free(card);

  return 0;
}

static int convert_prange(long val, long min, long max) {
  long range = max - min;
  if (range == 0)
    return 0;
  val -= min;
  return rint(val/(double)range * 100);
}

void setvol(int vol) {
  long pmin = 0, pmax = 0, target, current;
  int cur_perc;
  snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);

  snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &current);
  cur_perc = convert_prange(current,pmin,pmax);

  target = ceil((vol) * ((pmax) - (pmin)) * 0.01 + (pmin));

  DEBUG_PRINT("Setting volume.  cur: %li  tar: %li  curp: %i  tp: %i\n",current,target,cur_perc,vol);

  while(target == current) { // deal with channels that have fewer than 100 steps
    if (cur_perc < vol) {
      if (target == pmax) break;
      vol++;
    }
    else {
      if (target == pmin) break;
      vol--;
    }
    target = ceil((vol) * ((pmax) - (pmin)) * 0.01 + (pmin));

    DEBUG_PRINT("In while:  New target: %li  New perc: %i\n",target, vol);
  }
  target = (target < pmin)?pmin:target;
  target = (target > pmax)?pmax:target;

  DEBUG_PRINT("Final target: %li\n",target);

  snd_mixer_selem_set_playback_volume_all(elem, target);
  snd_mixer_selem_set_playback_volume_all(elem, target);
}

void setmute() {
  if (ismuted())
    snd_mixer_selem_set_playback_switch_all(elem, 0);
  else
    snd_mixer_selem_set_playback_switch_all(elem, 1);
}

int ismuted() {
  int muted;
  snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &muted);
  return muted;
}

int getvol() {
  long pmin = 0, pmax = 0;
  snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);

  long val;
  snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &val);
  DEBUG_PRINT("[getvol] From mixer: %li  pmin: %li  pmax: %li\n",val,pmin,pmax);
  return convert_prange(val,pmin,pmax);
}

static gint inited = 0;
void alsa_init() {
  if (inited) { // re-init, need to close down first
    close_mixer(&handle);
  }
  alsaset();
  inited = 1;
}

void alsa_close() {
  snd_mixer_close(handle);
}
