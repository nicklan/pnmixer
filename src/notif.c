/* notif.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute
 * it and/or modify it under the terms of the GNU General
 * Public License v3. source code is available at
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file notif.c
 * This file handles the notification subsystem
 * via libnotify and mostly reacts to volume changes.
 * @brief Notification subsystem.
 */

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <glib.h>

#ifdef HAVE_LIBN
#include <libnotify/notify.h>
#endif

#include "audio.h"
#include "prefs.h"
#include "support-intl.h"
#include "support-log.h"
#include "notif.h"

#include "main.h"

#ifdef HAVE_LIBN

#if NOTIFY_CHECK_VERSION (0, 7, 0)
#define NOTIFICATION_NEW(summary, body, icon)	\
  notify_notification_new(summary, body, icon)
#define NOTIFICATION_SET_HINT_STRING(notification, key, value)	  \
  notify_notification_set_hint(notification, key, g_variant_new_string(value))
#define NOTIFICATION_SET_HINT_INT32(notification, key, value)	  \
  notify_notification_set_hint(notification, key, g_variant_new_int32(value))
#else
#define NOTIFICATION_NEW(summary, body, icon)	\
  notify_notification_new(summary, body, icon, NULL)
#define NOTIFICATION_SET_HINT_STRING(notification, key, value)	\
  notify_notification_set_hint_string(notification, key, value)
#define NOTIFICATION_SET_HINT_INT32(notification, key, value)	\
  notify_notification_set_hint_int32(notification, key, value)
#endif

/* Helpers */

static void
show_volume_notif(NotifyNotification *notification,
                  const gchar *card, const gchar *channel,
                  gboolean muted, gdouble volume)
{
	GError *error = NULL;
	gchar *icon, *summary;

	if (muted)
		icon = "audio-volume-muted";
	else if (volume == 0)
		icon = "audio-volume-off";
	else if (volume < 33)
		icon = "audio-volume-low";
	else if (volume < 66)
		icon = "audio-volume-medium";
	else
		icon = "audio-volume-high";

	if (muted)
		summary = g_strdup("Volume muted");
	else
		summary = g_strdup_printf("%s (%s)\nVolume: %ld%%\n",
		                          card, channel, lround(volume));

	notify_notification_update(notification, summary, NULL, icon);
	NOTIFICATION_SET_HINT_INT32(notification, "value", lround(volume));

	if (!notify_notification_show(notification, &error)) {
		ERROR("Could not send notification: %s", error->message);
		g_error_free(error);
	}

	g_free(summary);
}

static void
show_text_notif(NotifyNotification *notification,
                const gchar *summary, const gchar *body)
{
	GError *error = NULL;

	notify_notification_update(notification, summary, body, NULL);

	if (!notify_notification_show(notification, &error)) {
		ERROR("Could not send notification: %s", error->message);
		g_error_free(error);
	}
}

/* Public functions & signal handlers */

struct notif {
	/* Audio system */
	Audio *audio;
	/* Preferences */
	gboolean enabled;
	gboolean popup;
	gboolean tray;
	gboolean hotkey;
	gboolean external;
	/* Notifications */
	NotifyNotification *volume_notif;
	NotifyNotification *text_notif;

};

/* Handle signals coming from the audio subsystem. */
static void
on_audio_changed(G_GNUC_UNUSED Audio *audio, AudioEvent *event, gpointer data)
{
	Notif *notif = (Notif *) data;

	switch (event->signal) {
	case AUDIO_NO_CARD:
		show_text_notif(notif->text_notif,
		                _("No sound card"),
		                _("No playable soundcard found"));
		break;

	case AUDIO_CARD_DISCONNECTED:
		show_text_notif(notif->text_notif,
		                _("Soundcard disconnected"),
		                _("Soundcard has been disconnected, reloading sound system..."));
		break;

	case AUDIO_VALUES_CHANGED:
		if (!notif->enabled)
			return;

		switch (event->user) {
		case AUDIO_USER_UNKNOWN:
			if (!notif->external)
				return;
			break;
		case AUDIO_USER_POPUP:
			if (!notif->popup)
				return;
			break;
		case AUDIO_USER_TRAY_ICON:
			if (!notif->tray)
				return;
			break;
		case AUDIO_USER_HOTKEYS:
			if (!notif->hotkey)
				return;
			break;
		default:
			WARN("Unhandled audio user");
			return;
		}

		show_volume_notif(notif->volume_notif,
		                  event->card, event->channel,
		                  event->muted, event->volume);
		break;

	default:
		break;
	}
}

/**
 * Reload notif preferences.
 * This has to be called each time the preferences are modified.
 *
 * @param notif a Notif instance.
 */
void
notif_reload(Notif *notif)
{
	guint timeout;
	NotifyNotification *notification;

	/* Get preferences */
	notif->enabled = prefs_get_boolean("EnableNotifications", FALSE);
	notif->popup = prefs_get_boolean("PopupNotifications", FALSE);
	notif->tray = prefs_get_boolean("MouseNotifications", TRUE);
	notif->hotkey = prefs_get_boolean("HotkeyNotifications", TRUE);
	notif->external = prefs_get_boolean("ExternalNotifications", FALSE);
	timeout = prefs_get_integer("NotificationTimeout", 1500);

	/* Create volume notification */
	notification = NOTIFICATION_NEW("", NULL, NULL);
	notify_notification_set_timeout(notification, timeout);
	NOTIFICATION_SET_HINT_STRING(notification, "x-canonical-private-synchronous", "");
	if (notif->volume_notif)
		g_object_unref(notif->volume_notif);
	notif->volume_notif = notification;

	/* Create text notification */
	notification = NOTIFICATION_NEW("", NULL, NULL);
	notify_notification_set_timeout(notification, timeout * 2);
	NOTIFICATION_SET_HINT_STRING(notification, "x-canonical-private-synchronous", "");
	if (notif->text_notif)
		g_object_unref(notif->text_notif);
	notif->text_notif = notification;
}

/**
 * Uninitializes libnotify.
 * This should be called only once at cleanup.
 *
 * @param notif a Notif instance.
 */
void
notif_free(Notif *notif)
{
	if (notif == NULL)
		return;

	/* Unref any existing notification */
	if (notif->volume_notif)
		g_object_unref(notif->volume_notif);
	if (notif->text_notif)
		g_object_unref(notif->text_notif);

	/* Disconnect audio signal handlers */
	audio_signals_disconnect(notif->audio, on_audio_changed, notif);

	/* Uninit libnotify. This should be done only once */
	g_assert(notify_is_initted() == TRUE);
	notify_uninit();

	g_free(notif);
}

/**
 * Initializes libnotify.
 * This should be called only once at startup.
 *
 * @param audio An Audio instance.
 * @return the newly created Notif instance.
 */
Notif *
notif_new(Audio *audio)
{
	Notif *notif;

	notif = g_new0(Notif, 1);

	/* Init libnotify. This should be done only once */
	g_assert(notify_is_initted() == FALSE);
	if (!notify_init(PACKAGE))
		run_error_dialog("Unable to initialize libnotify. "
		                 "Notifications won't be sent.");

	/* Connect audio signals handlers */
	notif->audio = audio;
	audio_signals_connect(audio, on_audio_changed, notif);

	/* Load preferences */
	notif_reload(notif);

	return notif;
}

#else

void
notif_free(G_GNUC_UNUSED Notif *notif)
{
}

Notif *
notif_new(G_GNUC_UNUSED Audio *audio)
{
	return NULL;
}

void
notif_reload(G_GNUC_UNUSED Notif *notif)
{
}

#endif				// HAVE_LIBN
