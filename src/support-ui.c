/* support-ui.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file support-ui.c
 * Various ui-related functions.
 * @brief Various ui-related functions.
 */

#include <glib.h>
#include <gtk/gtk.h>

#include "support-log.h"
#include "support-intl.h"

/**
 * Gets the path to a data file.
 * May first look in ./data/[path], and then in
 * PACKAGE_DATA_DIR/PACKAGE/[path].
 *
 * @param path of the file
 * @return path to the file or NULL on failure. Must be freed.
 */
static gchar *
get_data_file(const char *pathname)
{
	gchar *path;

#ifdef DATA_IN_CWD
	path = g_build_filename(".", "data", pathname, NULL);
	if (g_file_test(path, G_FILE_TEST_EXISTS))
		return path;
	g_free(path);
#endif

	path = g_build_filename(PACKAGE_DATA_DIR, PACKAGE, pathname, NULL);
	if (g_file_test(path, G_FILE_TEST_EXISTS))
		return path;
	g_free(path);

	WARN("Could not find data file '%s'", pathname);

	return NULL;
}


#ifndef WITH_GTK3
GtkBuilder *
gtk_builder_new_from_file (const gchar *filename)
{
	GError *error = NULL;
	GtkBuilder *builder;

	builder = gtk_builder_new();
	if (!gtk_builder_add_from_file(builder, filename, &error))
		g_error("failed to add UI: %s", error->message);

	return builder;
}

void
gtk_combo_box_text_remove_all(GtkComboBoxText *combo_box)
{
	GtkListStore *store;

	g_return_if_fail(GTK_IS_COMBO_BOX_TEXT(combo_box));

	store = GTK_LIST_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(combo_box)));
	gtk_list_store_clear(store);
}
#endif


/**
 * Gets the path to an ui file.
 * May first look in ./data/ui/[file], and then in
 * PACKAGE_DATA_DIR/PACKAGE/ui/[file].
 *
 * @param filename the name of the ui file
 * @return path to the ui file or NULL on failure. Must be freed.
 */
gchar *
get_ui_file(const char *uifilename)
{
	gchar *filename = g_build_filename("ui", uifilename, NULL);
	gchar *filepath = get_data_file(filename);

	g_free(filename);
	return filepath;
}

/**
 * Gets the path to a pixmap file.
 * May first look in ./data/pixmaps/[file], and then in
 * PACKAGE_DATA_DIR/PACKAGE/ui/[file].
 *
 * @param filename the pixmap file to find
 * @return path to the ui file or NULL on failure. Must be freed.
 */
gchar *
get_pixmap_file(const gchar *pixfilename)
{
	gchar *filename = g_build_filename("pixmaps", pixfilename, NULL);
	gchar *filepath = get_data_file(filename);

	g_free(filename);
	return filepath;
}
