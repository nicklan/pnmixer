/* callbacks.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */

#ifndef CALLBACKS_H_
#define CALLBACKS_H_

#include "support.h"
#include <gtk/gtk.h>


void
on_mute_clicked                (GtkButton       *button,
				gpointer         user_data);


gboolean 
vol_scroll_event(GtkRange     *range,
		 GtkScrollType scroll,
		 gdouble       value,
		 gpointer      user_data);

void
on_ok_button_clicked                   (GtkButton       *button,
                                        PrefsData       *data);

void
on_cancel_button_clicked                   (GtkButton       *button,
					    PrefsData       *data);

gboolean on_scroll (GtkWidget *widget, GdkEventScroll *event);

#endif // CALLBACKS_H_
