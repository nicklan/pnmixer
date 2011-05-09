#include <gtk/gtk.h>


void
on_checkbutton1_clicked                (GtkButton       *button,
                                        gpointer         user_data);

gboolean
on_hscale1_value_change_event        (GtkWidget       *widget,
                                        GdkEventButton  *event,
                                        gpointer         user_data);
