/* support-log.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file support-log.h
 * Logging support.
 * Provides various macros to print debug, warning & error messages.
 * Debug messages are enabled with a command-line argument.
 * This should be included by every file.
 * @brief Logging support.
 */

#ifndef _SUPPORT_LOG_H_
#define _SUPPORT_LOG_H_

#include <glib.h>

extern gboolean want_debug;

enum log_level {
	LOG_ERROR,
	LOG_WARN,
	LOG_DEBUG
};

void log_msg_v(enum log_level level, const char *file, const char *format, va_list args);
void log_msg(enum log_level level, const char *file, const char *format, ...);

#define ERROR(...) log_msg(LOG_ERROR, __FILE__, __VA_ARGS__)
#define WARN(...)  log_msg(LOG_WARN,  __FILE__, __VA_ARGS__)
#define DEBUG(...) log_msg(LOG_DEBUG, __FILE__, __VA_ARGS__)

#endif				// _SUPPORT_LOG_H_
