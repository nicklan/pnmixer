/* hotkeys.c
 * PNmixer is written by Nick Lanham, a fork of OBmixer
 * which was programmed by Lee Ferrett, derived 
 * from the program "AbsVolume" by Paul Sherman
 * This program is free software; you can redistribute 
 * it and/or modify it under the terms of the GNU General 
 * Public License v3. source code is available at 
 * <http://github.com/nicklan/pnmixer>
 */

#include "main.h"
#include "alsa.h"
#include <gdk/gdkx.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

static int volMuteKey = -1;
static int volDownKey = -1;
static int volUpKey   = -1;
static int volStep    = 1;
 
static
GdkFilterReturn key_filter(GdkXEvent *gdk_xevent, GdkEvent *event,
			   gpointer data) {
  int type;
  int key;
  XKeyEvent *xevent;
  //gboolean bResult;

  xevent = gdk_xevent;
  type = xevent->type;

  if (type == KeyPress) {
    key = ((XKeyEvent *)xevent)->keycode;

    if (key == volMuteKey) {
      setmute();
      get_mute_state();
      return GDK_FILTER_CONTINUE;
    } else {
      int cv = getvol();
      if (key == volDownKey) {
	setvol(cv-volStep);
      } 
      else if (key == volUpKey) {
	setvol(cv+volStep);
      }
      // just ignore unknown hotkeys

      if (get_mute_state() == 0) {
	setmute();
	get_mute_state();
      }

      // this will set the slider value
      get_current_levels();
    }
  }
  return GDK_FILTER_CONTINUE;
}

void add_filter() {
  gdk_window_add_filter(gdk_window_foreign_new(GDK_ROOT_WINDOW()),
			key_filter,NULL);
}

static char xErr;
int errBufSize = 512;
char *errBuf,*printBuf;
static unsigned long muteSerial,downSerial,upSerial;
static const char *muteSymStr,*downSymStr,*upSymStr;
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

void grab_keys(int mk, int uk, int dk,int step) {
  Display* disp = GDK_DISPLAY();

  // ungrab any previous keys
  XUngrabKey(disp, AnyKey, AnyModifier, GDK_ROOT_WINDOW());

  volMuteKey = mk;
  volUpKey = uk;
  volDownKey = dk;
  volStep = step;

  if (mk < 0 &&
      uk < 0 &&
      dk < 0)
    return;
  
  xErr = 0;
  errBuf = g_malloc(errBufSize*sizeof(gchar));
  printBuf = errBuf + snprintf(errBuf,errBufSize,"Could not bind the following hotkeys:\n");
  errBufSize -= (printBuf - errBuf);

  muteSymStr = XKeysymToString(XKeycodeToKeysym(GDK_DISPLAY(), volMuteKey, 0));
  downSymStr = XKeysymToString(XKeycodeToKeysym(GDK_DISPLAY(), volDownKey, 0));
  upSymStr = XKeysymToString(XKeycodeToKeysym(GDK_DISPLAY(), volUpKey, 0));

  XErrorHandler old_hdlr = XSetErrorHandler(errhdl);
  if (volMuteKey > 0) {
    muteSerial = NextRequest(disp);
    XGrabKey(disp,volMuteKey,0,GDK_ROOT_WINDOW(),1,GrabModeAsync,GrabModeAsync);
  }

  if (volUpKey > 0) {
    upSerial = NextRequest(disp);
    XGrabKey(disp,volUpKey,0,GDK_ROOT_WINDOW(),1,GrabModeAsync,GrabModeAsync);
  }

  if (volDownKey > 0) {
    downSerial = NextRequest(disp);
    XGrabKey(disp,volDownKey,0,GDK_ROOT_WINDOW(),1,GrabModeAsync,GrabModeAsync);
  }

  XFlush(disp);
  XSync(disp, False);
  (void) XSetErrorHandler(old_hdlr);
  
  if (xErr) 
    g_idle_add(idle_report_error, NULL);
  else
    g_free(errBuf);
}
