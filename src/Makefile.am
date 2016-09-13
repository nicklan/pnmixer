## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = pnmixer

pnmixer_SOURCES =					\
	alsa.c			alsa.h			\
	audio.c			audio.h			\
	hotkey.c		hotkey.h		\
	hotkeys.c		hotkeys.h		\
	main.c			main.h			\
	notif.c			notif.h			\
	prefs.c			prefs.h			\
	support-intl.c		support-intl.h		\
	support-log.c		support-log.h		\
	support-ui.c		support-ui.h		\
	ui-about-dialog.c	ui-about-dialog.h	\
	ui-hotkey-dialog.c	ui-hotkey-dialog.h	\
	ui-popup-menu.c		ui-popup-menu.h		\
	ui-popup-window.c	ui-popup-window.h	\
	ui-prefs-dialog.c	ui-prefs-dialog.h	\
	ui-tray-icon.c		ui-tray-icon.h

pnmixer_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

