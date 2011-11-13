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
#include <gdk/gdkx.h>

#ifdef HAVE_CONFIG_H
#  include <config.h>
#endif

// TODO: Set these from prefs
static int volMuteKey = 121;
static int volDownKey = 122;
static int volUpKey =   123;
 
GdkFilterReturn key_filter(GdkXEvent *gdk_xevent, GdkEvent *event,
			   gpointer data) {
  int type;
  int key, keysym;
  XKeyEvent *xevent;
  gboolean bResult;

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
	setvol(cv-2);
      } 
      else if (key == volUpKey) {
	setvol(cv+2);
      }
      else printf("Unknown hotkey\n");

      if (get_mute_state() == 0) {
	setmute();
	get_mute_state();
      }

      // this will set the slider value
      get_current_levels();
    }
    return GDK_FILTER_CONTINUE;
  }
}

static char xErr;
int errBufSize = 512;
char *errBuf,*printBuf;
static int tryingKey;
static int errhdl(Display *disp, XErrorEvent *ev) {
  int p;
  xErr = 1;
  p = snprintf(printBuf,errBufSize,"%s\n",XKeysymToString(XKeycodeToKeysym(GDK_DISPLAY(), tryingKey, 0)));
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

void grab_keys(GtkWidget* win) {
  Display* disp = gdk_x11_display_get_xdisplay(gdk_screen_get_display(gtk_window_get_screen(GTK_WINDOW(win))));
  gdk_window_add_filter(gdk_window_foreign_new(GDK_ROOT_WINDOW()),
			key_filter,NULL);
  
  xErr = 0;
  errBuf = g_malloc(errBufSize*sizeof(gchar));
  printBuf = errBuf + snprintf(errBuf,errBufSize,"Could not bind the following hotkeys:\n");
  errBufSize -= (printBuf - errBuf);

  XErrorHandler old_hdlr = XSetErrorHandler(errhdl);

  tryingKey = volMuteKey;
  XGrabKey(disp,tryingKey,0,GDK_ROOT_WINDOW(),1,GrabModeAsync,GrabModeAsync);

  tryingKey = volDownKey;
  XGrabKey(disp,tryingKey,0,GDK_ROOT_WINDOW(),1,GrabModeAsync,GrabModeAsync);

  tryingKey = volUpKey;
  XGrabKey(disp,tryingKey,0,GDK_ROOT_WINDOW(),1,GrabModeAsync,GrabModeAsync);

  XFlush(disp);
  XSync(disp, False);
  (void) XSetErrorHandler(old_hdlr);
  
  if (xErr) 
    g_idle_add(idle_report_error, NULL);
  else
    g_free(errBuf);
}
