#ifndef ALSA_H_
#define ALSA_H_

int setvol(int vol);
void setmute();
int getvol();
int ismuted();
void alsa_init();
void alsa_close();

#endif // ALSA_H_
