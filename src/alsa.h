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
 * Header for alsa.c. Holds the acard struct and public
 * functions.
 * @brief header for alsa.c
 */

#ifndef ALSA_H_
#define ALSA_H_

#include <glib.h>

/**
 * Struct representing an alsa card.
 */
struct acard {
  /**
   * Real card name like 'HDA Intel PCH'.
   */
  char *name;
  /**
   * HTCL device name, like 'hw:0'.
   */
  char *dev;
  /**
   * All playable channels in a list.
   */
  GSList *channels;
};

/**
 * The list of cards detected. Not all of them are
 * playable (ie, the channels field may be NULL).
 */
GSList *cards;



struct acard *find_card(const gchar *card);
int setvol(int vol, int dir, gboolean notify);
void setmute(gboolean notify);
int getvol();
int ismuted();
void alsa_init();
void alsa_close();
struct acard *alsa_get_active_card();
const char *alsa_get_active_channel();

#endif // ALSA_H_
