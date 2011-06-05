#ifndef ALSA_H_
#define ALSA_H_

#include <glib.h>

struct acard {
  char *name;
  char *dev;
  GSList *channels; 
};
GSList* cards;

int setvol(int vol);
void setmute();
int getvol();
int ismuted();
void alsa_init();
void alsa_close();

#endif // ALSA_H_
