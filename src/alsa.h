/* alsa.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */

#ifndef ALSA_H_
#define ALSA_H_

#include <glib.h>

struct acard {
  char *name;
  char *dev;
  GSList *channels; 
};
GSList* cards;

void setvol(int vol,gboolean notify);
void setmute(gboolean notify);
int getvol();
int ismuted();
void alsa_init();
void alsa_close();

#endif // ALSA_H_
