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

#include "support-intl.h"
#include "ui-about-dialog.h"

#define LICENSE_GPL3 \
	"PNMixer is free software; you can redistribute it and/or modify it "	\
	"under the terms of the GNU General Public License v3 as published " \
	"by the Free Software Foundation.\n" \
	"\n" \
	"PNMixer is distributed in the hope that it will be useful, but " \
	"WITHOUT ANY WARRANTY; without even the implied warranty of " \
	"MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. " \
	"See the GNU General Public License for more details.\n" \
	"\n" \
	"You should have received a copy of the GNU General Public License " \
	"along with PNMixer; if not, write to the Free Software Foundation, "	\
	"Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA."

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
	AboutDialog *dialog;
	const gchar *artists[] = {
		"Paul Davey",
		NULL
	};
	const gchar *authors[] = {
		"Brian Bidulock",
		"El Boulangero <elboulangero@gmail.com>",
		"Julian Ospald <hasufell@posteo.de>",
		"Nick Lanham",
		"Steven Honeyman",
		NULL
	};
	const gchar *translators =
	        "The Translation Project http://translationproject.org\n" \
	        "Mario Blättermann (de)\n" \
	        "Stéphane Aulery (fr)\n" \
	        "Božidar Putanec (hr)\n" \
	        "Mattia Bertoni (it)\n" \
	        "Benno Schulenberg (nl)\n" \
	        "Pavel Roschin (ru)\n" \
	        "Yuri Chornoivan (uk)\n" \
	        "Trần Ngọc Quân (vi)\n" \
	        "Shengyu Zhang (zh_CN)";

	/* Create about dialog */
	dialog = g_new0(AboutDialog, 1);
	dialog->about_dialog = gtk_about_dialog_new();

	/* Fill with the relevant information */
	g_object_set(dialog->about_dialog,
	             "artists",            artists,
	             "authors",            authors,
	             "comments",           _("A mixer for the system tray"),
	             "copyright",          _("Copyright © 2010-2016 Nick Lanham"),
	             "license",            LICENSE_GPL3,
	             "wrap-license",       TRUE,
	             "logo-icon-name",     "pnmixer",
	             "program-name",       "PNMixer",
	             "translator-credits", translators,
	             "version",            PACKAGE_VERSION,
	             "website",            "http://github.com/nicklan/pnmixer",
	             NULL);

	/* More config for window */
	gtk_window_set_skip_taskbar_hint(GTK_WINDOW(dialog->about_dialog), TRUE);
	gtk_window_set_transient_for(GTK_WINDOW(dialog->about_dialog), parent);

	return dialog;
}
