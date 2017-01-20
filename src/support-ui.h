/* support-ui.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file support-ui.h
 * Header for support-ui.c.
 * This should be included in ui files only.
 * @brief Header for support-ui.c.
 */

#ifndef _SUPPORT_UI_H_
#define _SUPPORT_UI_H_

#include <gtk/gtk.h>

#ifndef WITH_GTK3

/*
 * Simple functions lacking in Gtk2
 */

GtkBuilder *gtk_builder_new_from_file(const gchar *filename);
void gtk_combo_box_text_remove_all(GtkComboBoxText *combo_box);

#endif

#if GTK_CHECK_VERSION(3,22,0)
#define GTK_3_22_UNUSED G_GNUC_UNUSED
#else
#define GTK_3_22_UNUSED
#endif


/*
 * Cast a pointer (that may be a function pointer) to a data pointer,
 * suppressing any warnings from the compilator.
 * We need that to suppress pedantic warnings when using
 * g_signal_handlers_block_by_func(). Indeed, we must feed it a
 * function pointer, but the signature expects a data pointer.
 * Aparently Glib doesn't respect ISO C here.
 * For more details, see this discussion:
 * http://compgroups.net/comp.lang.c/cast-function-pointer-to-void/722812
 */

#define DATA_PTR(ptr) (*((void **)(&ptr)))

/*
 * GtkBuilder helpers
 */

#define gtk_builder_get_widget(builder, name)	  \
	GTK_WIDGET(gtk_builder_get_object(builder, name))

#define gtk_builder_get_adjustment(builder, name)	  \
	GTK_ADJUSTMENT(gtk_builder_get_object(builder, name))

/*
 * 'assign' functions are used to retrieve a widget from a GtkBuilder
 * and to assign it into a structure. This is used a lot when we build
 * a window, and need to keep a pointer toward some widgets for later use.
 * This macro is more clever that it seems:
 * - it ensures that the name used in the struct is the same as the name used
 *   in the ui file, therefore enforcing a consistent naming across the code.
 * - it ensures that the widget was found amidst the GtkBuilder objects,
 *   therefore detecting errors that can happen when reworking the ui files.
 */

#define assign_gtk_widget(builder, container, name)	  \
	do { \
		container->name = gtk_builder_get_widget(builder, #name); \
		g_assert(GTK_IS_WIDGET(container->name)); \
	} while (0)

#define assign_gtk_adjustment(builder, container, name)	  \
	do { \
		container->name = gtk_builder_get_adjustment(builder, #name); \
		g_assert(GTK_IS_ADJUSTMENT(container->name)); \
	} while (0)

/*
 * File helpers
 */

gchar *get_ui_file(const char *filename);
gchar *get_pixmap_file(const gchar *filename);

#endif				// _SUPPORT_UI_H_
