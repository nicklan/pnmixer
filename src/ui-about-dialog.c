/* ui-about-dialog.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file ui-about-dialog.c
 * This file holds the ui-related code for the about dialog.
 * @brief About dialog subsystem.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <gtk/gtk.h>

#include "support-log.h"
#include "support-ui.h"
#include "ui-about-dialog.h"

#ifdef WITH_GTK3
#define ABOUT_UI_FILE "about-dialog-gtk3.glade"
#else
#define ABOUT_UI_FILE "about-dialog-gtk2.glade"
#endif

/* Public functions */

struct about_dialog {
	GtkWidget *about_dialog;
};

/**
 * Runs the about dialog.
 *
 * @param dialog a AboutDialog instance.
 */
void
about_dialog_run(AboutDialog *dialog)
{
	GtkDialog *about_dialog = GTK_DIALOG(dialog->about_dialog);

	gtk_dialog_run(about_dialog);
}

/**
 * Destroys the about dialog, freeing any resources.
 *
 * @param dialog a AboutDialog instance.
 */
void
about_dialog_destroy(AboutDialog *dialog)
{
	gtk_widget_destroy(dialog->about_dialog);
	g_free(dialog);
}

/**
 * Creates the about dialog.
 *
 * @param parent a GtkWindow to be used as the parent.
 * @return the newly created AboutDialog instance.
 */
AboutDialog *
about_dialog_create(GtkWindow *parent)
{
	gchar *uifile;
	GtkBuilder *builder;
	AboutDialog *dialog;

	dialog = g_new0(AboutDialog, 1);

	/* Build UI file */
	uifile = get_ui_file(ABOUT_UI_FILE);
	g_assert(uifile);

	DEBUG("Building from ui file '%s'", uifile);
	builder = gtk_builder_new_from_file(uifile);

	/* Save some widgets for later use */
	assign_gtk_widget(builder, dialog, about_dialog);

	/* Configure some widgets */
	gtk_about_dialog_set_version(GTK_ABOUT_DIALOG(dialog->about_dialog), VERSION);

	/* Set transient parent */
	gtk_window_set_transient_for(GTK_WINDOW(dialog->about_dialog), parent);

	/* Cleanup */
	g_object_unref(builder);
	g_free(uifile);

	return dialog;
}
