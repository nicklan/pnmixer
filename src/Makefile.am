## Process this file with automake to produce Makefile.in

AM_CPPFLAGS = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = pnmixer

pnmixer_SOURCES =		\
	alsa.c			\
	audio.c			\
	hotkey.c		\
	hotkeys.c		\
	main.c			\
	notif.c			\
	prefs.c			\
	support-intl.c		\
	support-log.c		\
	support-ui.c		\
	ui-about-dialog.c	\
	ui-hotkey-dialog.c	\
	ui-popup-menu.c		\
	ui-popup-window.c	\
	ui-prefs-dialog.c	\
	ui-tray-icon.c

pnmixer_LDADD = @PACKAGE_LIBS@ $(INTLLIBS)

