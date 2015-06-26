/* hotkeys.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */

/**
 * @file hotkeys.c
 * This file handles the hotkey subsystem, including
 * communcation with Xlib and intercepting key presses
 * before they can be interpreted by gdk/gtk.
 * @brief hotkey subsystem
 */

#include "support.h"
#include "main.h"
#include "prefs.h"
#include "alsa.h"
#include <gdk/gdkx.h>
#include <X11/XKBlib.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

static int volMuteKey = -1;
static int volDownKey = -1;
static int volUpKey   = -1;
static int volMuteMods = -1;
static int volDownMods = -1;
static int volUpMods   = -1;
static int volStep    = 1;

// `xmodmap -pm`
/**
 * List of key modifiers which will be ignored whenever
 * we check whether the defined hotkeys have been pressed.
 */
static guint keymasks [] = {
    0,					/* No Modkey */
    GDK_MOD2_MASK, 			/* Numlock */
    GDK_LOCK_MASK, 			/* Capslock */
    GDK_MOD2_MASK | GDK_LOCK_MASK 	/* Both */
 };

/**
 * Checks if the keycode we got (minus modifiers like
 * numlock/capslock) matches the keycode we want.
 * Thus numlock + o will match o.
 *
 * @param got the key we got
 * @param want the key we want
 * @return TRUE if there is a match,
 * FALSE otherwise
 */
static gboolean checkModKey(int got, int want) {
  guint i;
  for (i=0; i < G_N_ELEMENTS(keymasks); i++)
    if ((int)(want | keymasks[i]) == got)
		return TRUE;
  return FALSE;
}

/**
 * This function is called before gdk/gtk can respond
 * to any(!) window event and handles pressed hotkeys.
 *
 * @param gdk_xevent the native event to filter
 * @param event the GDK event to which the X event will be translated
 * @param data user data set when the filter was installed
 * @return a GdkFilterReturn value, should be GDK_FILTER_CONTINUE only
 */
static GdkFilterReturn key_filter(GdkXEvent *gdk_xevent,
		GdkEvent *event,
		gpointer data) {
  int type;
  guint key,state;
  XKeyEvent *xevent;
  //gboolean bResult;

  xevent = gdk_xevent;
  type = xevent->type;

  if (type == KeyPress) {
    key = ((XKeyEvent *)xevent)->keycode;
    state = ((XKeyEvent *)xevent)->state;

    if ((int)key == volMuteKey && checkModKey(state, volMuteMods)) {
      setmute(enable_noti&&hotkey_noti);
      get_mute_state(TRUE);
      return GDK_FILTER_CONTINUE;
    } else {
      int cv = getvol();
      if ((int)key == volUpKey && checkModKey(state, volUpMods)) {
	setvol(cv+volStep,1,enable_noti&&hotkey_noti);
      }
      else if ((int)key == volDownKey && checkModKey(state, volDownMods)) {
	setvol(cv-volStep,-1,enable_noti&&hotkey_noti);
      } 
      // just ignore unknown hotkeys

      if (get_mute_state(TRUE) == 0)
	setmute(enable_noti&&hotkey_noti);

      // this will set the slider value
      get_current_levels();
    }
  }
  return GDK_FILTER_CONTINUE;
}

/**
 * Attaches the key_filter() function as a filter
 * to the the root window, so it will intercept window events.
 */
void add_filter() {
  gdk_window_add_filter(
		  gdk_x11_window_foreign_new_for_display(gdk_display_get_default(),
			  GDK_ROOT_WINDOW()),
		  key_filter,NULL);
}

static char xErr;
int errBufSize = 512;
char *errBuf,
	 *printBuf;
static unsigned long muteSerial,
					 downSerial,
					 upSerial;
static gchar *muteSymStr=NULL,
			 *downSymStr=NULL,
			 *upSymStr=NULL;

/**
 * When an Xlib error occurs, this function is called. It is
 * set via XSetErrorHandler().
 *
 * @param disp the display where the error occured
 * @param ev the error event
 * @return it's acceptable to return, but the return value is ignored
 */
static int errhdl(Display *disp, XErrorEvent *ev) {
  int p;
  xErr = 1;
  if (ev->serial == muteSerial) 
    p = snprintf(printBuf,errBufSize," %s\n",muteSymStr);
  else if (ev->serial == downSerial)
    p = snprintf(printBuf,errBufSize," %s\n",downSymStr);
  else if (ev->serial == upSerial)
    p = snprintf(printBuf,errBufSize," %s\n",upSymStr);
  else {
    p = 0;
    g_warning("Unknown serial in X error handler\n");
  }
  errBufSize -= p;
  printBuf = printBuf+p;
  return 0;
}

/**
 * We need to report error in idle moment
 * since we can't report_error before gtk_main is called.
 * This function is attached via g_idle_add() in grab_keys(),
 * whenever there is an Xerror.
 *
 * @param data passed to the function,
 * set when the source was created
 * @return FALSE if the source should be removed,
 * TRUE otherwise
 */
static gboolean idle_report_error(gpointer data) {
  report_error(errBuf);
  g_free(errBuf);
  return FALSE;
}

/**
 * Grabs keys on the Xserver level via XGrabKey(),
 * so they can be intercepted and interpreted by
 * our application, thus having global hotkeys.
 *
 * If mk, uk and dk parameters are -1, then
 * this function will just ungrab everything.
 *
 * @param mk mutekey
 * @param uk volume up key
 * @param dk volume down key
 * @param mm volume mute mod key
 * @param um volume up mod key
 * @param dm volume down mod key
 * @param step hotkey volume step
 */
void grab_keys(int mk, int uk, int dk,
	       int mm, int um, int dm,
	       int step) {
  Display* disp = gdk_x11_get_default_xdisplay();

  // ungrab any previous keys
  XUngrabKey(disp, AnyKey, AnyModifier, GDK_ROOT_WINDOW());

  volMuteKey = mk;
  volUpKey = uk;
  volDownKey = dk;
  volMuteMods = mm;
  volUpMods = um;
  volDownMods = dm;
  volStep = step;

  if (mk < 0 &&
      uk < 0 &&
      dk < 0)
    return;
  
  xErr = 0;
  errBuf = g_malloc(errBufSize*sizeof(gchar));
  printBuf = errBuf + snprintf(errBuf,errBufSize,_("Could not bind the following hotkeys:\n"));
  errBufSize -= (printBuf - errBuf);

  if (muteSymStr) g_free(muteSymStr);
  if (upSymStr)   g_free(upSymStr);
  if (downSymStr) g_free(downSymStr);

  muteSymStr =
	  gtk_accelerator_name(
			  XkbKeycodeToKeysym(gdk_x11_get_default_xdisplay(),
				  volMuteKey, 0, 0),volMuteMods);
  upSymStr =
	  gtk_accelerator_name(
			  XkbKeycodeToKeysym(gdk_x11_get_default_xdisplay(),
				  volUpKey, 0, 0),volUpMods);
  downSymStr =
	  gtk_accelerator_name(
			  XkbKeycodeToKeysym(gdk_x11_get_default_xdisplay(),
				  volDownKey, 0, 0),volDownMods);

  XErrorHandler old_hdlr = XSetErrorHandler(errhdl);
  guint i;
  for (i=0; i<G_N_ELEMENTS(keymasks); i++) {
    if (volMuteKey > 0) {
      muteSerial = NextRequest(disp);
      XGrabKey(disp,volMuteKey,volMuteMods|keymasks[i],GDK_ROOT_WINDOW(),1,GrabModeAsync,GrabModeAsync);
    }

    if (volUpKey > 0) {
      upSerial = NextRequest(disp);
      XGrabKey(disp,volUpKey,volUpMods|keymasks[i],GDK_ROOT_WINDOW(),1,GrabModeAsync,GrabModeAsync);
    }

    if (volDownKey > 0) {
      downSerial = NextRequest(disp);
      XGrabKey(disp,volDownKey,volDownMods|keymasks[i],GDK_ROOT_WINDOW(),1,GrabModeAsync,GrabModeAsync);
    }
  }

  XFlush(disp);
  XSync(disp, False);
  (void) XSetErrorHandler(old_hdlr);
  
  if (xErr) 
    g_idle_add(idle_report_error, NULL);
  else
    g_free(errBuf);
}
