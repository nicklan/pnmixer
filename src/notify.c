/* notify.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file notify.c
 * This file handles the notification subsystem
 * via libnotify and mostly reacts to volume changes.
 * @brief libnotify subsystem
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "alsa.h"
#include "main.h"
#include "notify.h"
#include "prefs.h"
#include "support.h"

#ifdef HAVE_LIBN

// code for when we have libnotify

#if NOTIFY_CHECK_VERSION (0, 7, 0)
#define NOTIFICATION_NEW(summary, body, icon)	\
  notify_notification_new(summary, body, icon)
#define NOTIFICATION_SET_HINT_STRING(notification, key, value)		\
  notify_notification_set_hint(notification, key, g_variant_new_string(value))
#define NOTIFICATION_SET_HINT_INT32(notification, key, value)		\
  notify_notification_set_hint(notification, key, g_variant_new_int32(value))
#else
#define NOTIFICATION_NEW(summary, body, icon)	\
  notify_notification_new(summary, body, icon, NULL)
#define NOTIFICATION_SET_HINT_STRING(notification, key, value)	\
  notify_notification_set_hint_string(notification, key, value)
#define NOTIFICATION_SET_HINT_INT32(notification, key, value)	\
  notify_notification_set_hint_int32(notification, key, value)
#endif

/**
 * We need to report error in idle moment
 * since we can't report_error before gtk_main is called.
 * This function is attached via g_idle_add() in init_libnotify().
 *
 * @param data passed to the function,
 * set when the source was created
 * @return FALSE if the source should be removed,
 * TRUE otherwise
 */
static gboolean idle_report_error(gpointer data) {
  report_error("Unable to initialize libnotify.  Notifications will not be sent");
  return FALSE;
}

/**
 * Initializes libnotify if it's not already
 * initialized.
 */
void init_libnotify() {
  if (!notify_is_initted())
    if (!notify_init(PACKAGE))
      g_idle_add(idle_report_error, NULL);
}

/**
 * Uninitializes libnotify if it is initialized.
 */
void uninit_libnotify() {
  if (notify_is_initted())
    notify_uninit();
}

/**
 * Send a volume notification. This is mainly called
 * from the alsa subsystem whenever we have volume
 * changes.
 *
 * @param level the playback volume level
 * @param muted whether the playback is muted
 */
void do_notify_volume(gint level, gboolean muted) {
  static NotifyNotification *notification = NULL;
  gchar  *summary, *icon, *active_card_name;
  const char *active_channel;
  GError *error = NULL;

  active_card_name = (alsa_get_active_card())->name;
  active_channel = alsa_get_active_channel();

  if (notification == NULL) {
    notification = NOTIFICATION_NEW("", NULL, NULL);
    notify_notification_set_timeout(notification, noti_timeout);
    NOTIFICATION_SET_HINT_STRING(notification,"x-canonical-private-synchronous","");
  }
  
  if (level < 0) level = 0;
  if (level > 100) level = 100;

  if (muted)
    summary = g_strdup("Volume muted");
  else 
    summary = g_strdup_printf("%s (%s)\nVolume: %d%%\n", active_card_name, active_channel, level);

  if (muted)
    icon = "audio-volume-muted";
  else if (level == 0)
    icon = "audio-volume-off";
  else if (level < 33) 
    icon = "audio-volume-low";
  else if (level < 66)
    icon = "audio-volume-medium";
  else 
    icon = "audio-volume-high";
  
  notify_notification_update(notification,summary,NULL,icon);
  NOTIFICATION_SET_HINT_INT32(notification,"value",level);
  
  if (!notify_notification_show(notification,&error)) {
    g_warning("Could not send notification: %s",error->message);
    report_error(_("Could not send notification: %s\n"),error->message);
    g_error_free(error);
  }

  g_free(summary);
}

/**
 * Send a text notification. 
 *
 * @param summary the notification summary
 * @param _body the notification body
 */
void do_notify_text(const gchar *summary, const gchar *body) {
  static NotifyNotification *notification = NULL;
  GError *error = NULL;

  if (notification == NULL) {
    notification = NOTIFICATION_NEW("", NULL, NULL);
    notify_notification_set_timeout(notification, noti_timeout * 2);
    NOTIFICATION_SET_HINT_STRING(notification,"x-canonical-private-synchronous","");
  }

  notify_notification_update(notification,summary,body,NULL);

  if (!notify_notification_show(notification,&error)) {
    g_warning("Could not send notification: %s",error->message);
    report_error(_("Could not send notification: %s\n"),error->message);
    g_error_free(error);
  }
}

#else

// without libnotify everything is a no-op
void init_libnotify() {}
void uninit_libnotify() {}
void do_notify_volume(gint level,gboolean muted) {}
void do_notify_text(gchar *text) {}

#endif // HAVE_LIBN
