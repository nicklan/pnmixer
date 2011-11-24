/* prefs.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#ifndef NOTIFY_H_
#define NOTIFY_H_

#ifdef HAVE_LIBN
// only include if we have libnotify
#include <libnotify/notify.h>
#endif // HAVE_LIBN

void init_libnotify();
void uninit_libnotify();
void do_notify(gint level,gboolean muted);

#endif // PREFS_H_
