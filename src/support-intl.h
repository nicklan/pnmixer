/* support-intl.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file support-intl.h
 * Internationalization support, should be included by every file.
 * @brief Internationalization support.
 */

#ifndef _SUPPORT_INTL_H_
#define _SUPPORT_INTL_H_

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#ifdef ENABLE_NLS

#include <libintl.h>

/* Standard gettext macros  */

#undef _
#define _(String) dgettext (PACKAGE, String)
#define Q_(String) g_strip_context ((String), gettext (String))
#ifdef gettext_noop
#define N_(String) gettext_noop (String)
#else
#define N_(String) (String)
#endif
#else
#define textdomain(String) (String)
#define gettext(String) (String)
#define dgettext(Domain, Message) (Message)
#define dcgettext(Domain, Message, Type) (Message)
#define bindtextdomain(Domain, Directory) (Domain)
#define _(String) (String)
#define Q_(String) g_strip_context ((String), (String))
#define N_(String) (String)

#endif

void intl_init(void);

#endif				// _SUPPORT_INTL_H_

