/* prefs.h
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */


#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "main.h"
#include "notify.h"
#include "support.h"

#ifdef HAVE_LIBN

// code for when we have libnotify


// need to report error in idle moment since we can't report_error before gtk_main is called
static gboolean idle_report_error(gpointer data) {
  report_error("Unable to initialize libnotify.  Notifications will not be sent");
  return FALSE;
}

static NotifyNotification* notification = NULL;

void init_libnotify() {
  if (!notify_is_initted())
    if (!notify_init(PACKAGE))
      g_idle_add(idle_report_error, NULL);
}

void uninit_libnotify() {
  if (notify_is_initted())
    notify_uninit();
}

void do_notify(gint level,gboolean muted) {
  gchar  *summary, *icon;
  GError *error = NULL;

  if (level < 0) level = 0;
  if (level > 100) level = 100;

  if (muted)
    summary = g_strdup("Volume muted");
  else 
    summary = g_strdup_printf("Volume: %d%%\n",level);

  if (muted)
    icon = "audio-volume-muted";
  else if (level < 33) 
    icon = "audio-volume-low";
  else if (level < 66)
    icon = "audio-volume-medium";
  else 
    icon = "audio-volume-high";
  
  if (notification == NULL)
    notification = notify_notification_new(summary,NULL,icon);
  else
    notify_notification_update(notification,summary,NULL,icon);

  notify_notification_set_hint_int32(notification,"value",level);
  notify_notification_set_hint_string(notification,"x-canonical-private-synchronous","");
  
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
void do_notify(gint level,gboolean muted) {}

#endif // HAVE_LIBN
