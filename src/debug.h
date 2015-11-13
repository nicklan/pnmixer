/* debug.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file debug.h
 * Header providing macros and global variables for debugging purposes.
 * If you need debugging in your .c file, include this header.
 * @brief debugging header
 */

#ifndef DEBUG_H
#define DEBUG_H

#include <glib.h>


/**
 * Global variable to control whether we want debugging.
 * This variable is initialized in main() and depends on the
 * '--debug'/'-d' command line argument.
 */
gboolean want_debug;

/**
 * Macro to print verbose debug info in case we want debugging.
 */
#define DEBUG_PRINT(fmt, ...) \
{ \
	if (want_debug == TRUE) { \
		printf(fmt"\n", ##__VA_ARGS__); \
	} \
}

#endif				// DEBUG_H
