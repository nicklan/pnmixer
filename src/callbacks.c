/* callbacks.c
 * OBmixer was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v2. source code is available at 
 * <http://www.jpegserv.com>
 */
#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>
#include <alsa/asoundlib.h>
#include "callbacks.h"
#include "main.h"
#include "support.h"

GtkWidget *window1;
GtkWidget *checkbutton1;
GtkAdjustment *vol_adjustment;
int volume;
extern int volume;

void
on_checkbutton1_clicked                (GtkButton       *button,
                                        gpointer         user_data)
{

	gtk_widget_hide (window1);
	setmute();
	get_mute_state();

}

gboolean
on_hscale1_value_change_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data)
{
	int volumeset;
	volumeset = (int)gtk_adjustment_get_value(vol_adjustment);

	setvol(volumeset);
	if (get_mute_state() == 0) {
		setmute();
		get_mute_state();
	}
	
	return FALSE;
}


