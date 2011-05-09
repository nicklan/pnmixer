#ifndef CALLBACKS_H_
#define CALLBACKS_H_

#include <gtk/gtk.h>


void
on_checkbutton1_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_hscale1_value_change_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);

void
on_ok_button_clicked                   (GtkButton       *button,
                                        gpointer         user_data);

void
on_cancel_button_clicked                   (GtkButton       *button,
					    gpointer         user_data);

gboolean on_scroll (GtkWidget *widget, GdkEventScroll *event);

#endif // CALLBACKS_H_
