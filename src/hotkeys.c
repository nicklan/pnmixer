/* hotkeys.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
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
 
static
GdkFilterReturn key_filter(GdkXEvent *gdk_xevent, GdkEvent *event,
			   gpointer data) {
  int type;
  unsigned int key,state;
  XKeyEvent *xevent;
  //gboolean bResult;

  xevent = gdk_xevent;
  type = xevent->type;

  if (type == KeyPress) {
    key = ((XKeyEvent *)xevent)->keycode;
    state = ((XKeyEvent *)xevent)->state;

    if ((int)key == volMuteKey && (int)state == volMuteMods) {
      setmute(enable_noti&&hotkey_noti);
      get_mute_state(TRUE);
      return GDK_FILTER_CONTINUE;
    } else {
      int cv = getvol();
      if ((int)key == volUpKey && (int)state == volUpMods) {
	setvol(cv+volStep,enable_noti&&hotkey_noti);
      }
      else if ((int)key == volDownKey && (int)state == volDownMods) {
	setvol(cv-volStep,enable_noti&&hotkey_noti);
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

void add_filter() {
  gdk_window_add_filter(
		  gdk_x11_window_foreign_new_for_display(gdk_display_get_default(),
			  GDK_ROOT_WINDOW()),
		  key_filter,NULL);
}

static char xErr;
int errBufSize = 512;
char *errBuf,*printBuf;
static unsigned long muteSerial,downSerial,upSerial;
static gchar *muteSymStr=NULL,*downSymStr=NULL,*upSymStr=NULL;
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

// need to report error in idle moment since we can't report_error before gtk_main is called
static gboolean idle_report_error(gpointer data) {
  report_error(errBuf);
  g_free(errBuf);
  return FALSE;
}

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
  if (volMuteKey > 0) {
    muteSerial = NextRequest(disp);
    XGrabKey(disp,volMuteKey,volMuteMods,GDK_ROOT_WINDOW(),1,GrabModeAsync,GrabModeAsync);
  }

  if (volUpKey > 0) {
    upSerial = NextRequest(disp);
    XGrabKey(disp,volUpKey,volUpMods,GDK_ROOT_WINDOW(),1,GrabModeAsync,GrabModeAsync);
  }

  if (volDownKey > 0) {
    downSerial = NextRequest(disp);
    XGrabKey(disp,volDownKey,volDownMods,GDK_ROOT_WINDOW(),1,GrabModeAsync,GrabModeAsync);
  }

  XFlush(disp);
  XSync(disp, False);
  (void) XSetErrorHandler(old_hdlr);
  
  if (xErr) 
    g_idle_add(idle_report_error, NULL);
  else
    g_free(errBuf);
}
