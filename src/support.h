/* support.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */


#ifndef SUPPORT_H_
#define SUPPORT_H_

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

#include <gtk/gtk.h>

/*
 * Standard gettext macros.
 */
#ifdef ENABLE_NLS
#  include <libintl.h>
#  undef _
#  define _(String) dgettext (PACKAGE, String)
#  define Q_(String) g_strip_context ((String), gettext (String))
#  ifdef gettext_noop
#    define N_(String) gettext_noop (String)
#  else
#    define N_(String) (String)
#  endif
#else
#  define textdomain(String) (String)
#  define gettext(String) (String)
#  define dgettext(Domain,Message) (Message)
#  define dcgettext(Domain,Message,Type) (Message)
#  define bindtextdomain(Domain,Directory) (Domain)
#  define _(String) (String)
#  define Q_(String) g_strip_context ((String), (String))
#  define N_(String) (String)
#endif

typedef struct {
  GtkWidget* prefs_window;
  GtkWidget* card_combo;
  GtkWidget* chan_combo;
  GtkWidget* vol_pos_label;
  GtkWidget* vol_pos_combo;
  GtkWidget* vol_meter_pos_label;
  GtkWidget* vol_meter_pos_spin;
  GtkWidget* vol_meter_color_label;
  GtkWidget* vol_meter_color_button;
  GtkWidget* custom_label;
  GtkWidget* custom_entry;
  GtkWidget* vol_text_check;
  GtkWidget* draw_vol_check;
  GtkWidget* icon_theme_combo;
  GtkWidget* vol_control_entry;
  GtkWidget* scroll_step_spin;
  GtkWidget* middle_click_combo;
} PrefsData;


/*
 * Public Functions.
 */

/* Use this function to set the directory containing installed pixmaps. */
void        add_pixmap_directory       (const gchar     *directory);


/*
 * Private Functions.
 */

/* This is used to create the pixmaps used in the interface. */
GtkWidget*  create_pixmap              (GtkWidget       *widget,
                                        const gchar     *filename);

/* This is used to create the pixbufs used in the interface. */
GdkPixbuf*  create_pixbuf              (const gchar     *filename);

/* This is used to set ATK action descriptions. */
void        glade_set_atk_action_description (AtkAction       *action,
                                              const gchar     *action_name,
                                              const gchar     *description);


GdkPixbuf* get_stock_pixbuf(const char* filename, gint size);

#endif // SUPPORT_H_
