/* alsa.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v2. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */

#include <alsa/asoundlib.h>

static int smixer_level = 0;
static struct snd_mixer_selem_regopt smixer_options;
static char card[64] = "default";
static snd_mixer_elem_t *elem;
static snd_mixer_t *handle;


static snd_mixer_elem_t* alsa_get_mixer_elem(snd_mixer_t *mixer, char *name, int index) {
  snd_mixer_selem_id_t *selem_id;
  snd_mixer_elem_t* elem;
  snd_mixer_selem_id_alloca(&selem_id);

  if (index != -1)
    snd_mixer_selem_id_set_index(selem_id, index);
  if (name != NULL)
    snd_mixer_selem_id_set_name(selem_id, name);

  elem = snd_mixer_find_selem(mixer, selem_id);

  return elem;
}

static int alsaset() {
  smixer_options.device = card;
  int level = 1;
  int err;
  snd_mixer_selem_id_t *sid;
  snd_mixer_selem_id_alloca(&sid);

  if ((err = snd_mixer_open(&handle, 0)) < 0) {
    error("Mixer %s open error: %s", card, snd_strerror(err));
    return err;
  }
  if (smixer_level == 0 && (err = snd_mixer_attach(handle, card)) < 0) {
    error("Mixer attach %s error: %s", card, snd_strerror(err));
    snd_mixer_close(handle);
    return err;
  }
  if ((err = snd_mixer_selem_register(handle, smixer_level > 0 ? &smixer_options : NULL, NULL)) < 0) {
    error("Mixer register error: %s", snd_strerror(err));
    snd_mixer_close(handle);
    return err;
  }
  err = snd_mixer_load(handle);
  if (err < 0) {
    error("Mixer %s load error: %s", card, snd_strerror(err));
    snd_mixer_close(handle);
    return err;
  }

  elem = snd_mixer_first_elem(handle);
  snd_mixer_selem_get_id(elem, sid);

  return 0;
}

static int convert_prange(int val, int min, int max) {
  int range = max - min;
  int tmp;

  if (range == 0)
    return 0;
  val -= min;
  tmp = rint((double)val/(double)range * 100);
  return tmp;
}


static int get_percent(int val, int min, int max) {
  static char str[32];
  int p;
	
  p = ceil((val) * ((max) - (min)) * 0.01 + (min));
  return p;
}

int setvol(int vol) {
  long pmin = 0, pmax = 0;
  snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);

  vol = get_percent(vol,pmin,pmax);

  snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_LEFT,vol);
  snd_mixer_selem_set_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT,vol);
}

void setmute() {
  int muted;
  snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &muted);

  if (muted == 1) {
    snd_mixer_selem_set_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT,0);
  } else {
    snd_mixer_selem_set_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT,1);
  }
}

int ismuted() {
  int muted;
  snd_mixer_selem_get_playback_switch(elem, SND_MIXER_SCHN_FRONT_LEFT, &muted);
  return muted;
}

int getvol() {
  long pmin = 0, pmax = 0;
  snd_mixer_selem_get_playback_volume_range(elem, &pmin, &pmax);

  int rr;
  long lr = rr;
  snd_mixer_selem_get_playback_volume(elem, SND_MIXER_SCHN_FRONT_RIGHT, &lr);
  rr = lr;
  int vol;
  vol=convert_prange(rr,pmin,pmax);

  return vol;

}

void alsa_init() {
  alsaset();
}

void alsa_close() {
  snd_mixer_close(handle);
}
