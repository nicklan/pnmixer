/* support-log.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file support-log.c
 * Logging support.
 * @brief Logging support.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h>
#include <glib.h>

#include "support-log.h"

#define VT_ESC    "\033"
#define VT_RESET  "[0m"
#define VT_RED    "[0;31m"
#define VT_GREY   "[0;37m"
#define VT_YELLOW "[1;33m"

/**
 * Global variable to control whether we want debugging.
 * This variable is initialized in main() and depends on the
 * '--debug'/'-d' command line argument.
 */

gboolean want_debug = FALSE;

/**
 * Log a message.
 *
 * @param level the log level.
 * @param file the file name.
 * @param format the message format. See the printf() documentation.
 * @param args the parameters to insert into the format string.
 */
void
log_msg_v(enum log_level level, const char *file, const char *format, va_list args)
{
	char buf[1024];
	const char *pfx;

	if (level == LOG_DEBUG && want_debug == FALSE)
		return;

	switch (level) {
	case LOG_ERROR:
		pfx = VT_ESC VT_RED "error" VT_ESC VT_RESET;
		break;
	case LOG_WARN:
		pfx = VT_ESC VT_YELLOW "warning" VT_ESC VT_RESET;
		break;
	case LOG_DEBUG:
		pfx = VT_ESC VT_GREY "debug" VT_ESC VT_RESET;
		break;
	default:
		pfx = "unknown";
	}

	snprintf(buf, sizeof buf, "%s: %s: %s\n", pfx, file, format);
	vfprintf(stderr, buf, args);
}

/**
 * Log a message.
 *
 * @param level the log level.
 * @param file the file name.
 * @param format the message format. See the printf() documentation.
 * @param ... the parameters to insert into the format string.
 */
void
log_msg(enum log_level level, const char *file, const char *format, ...)
{
	va_list args;

	va_start(args, format);
	log_msg_v(level, file, format, args);
	va_end(args);
}
