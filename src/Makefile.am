## Process this file with automake to produce Makefile.in

AM_CFLAGS = @AM_CFLAGS@

INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	@LIBNOTIFY_CFLAGS@ \
	@PACKAGE_CFLAGS@

bin_PROGRAMS = pnmixer

pnmixer_SOURCES = \
	support.c support.h \
	main.c main.h \
	hotkeys.c hotkeys.h \
	notify.c notify.h \
	alsa.c alsa.h \
	callbacks.c callbacks.h \
	prefs.c prefs.h

pnmixer_LDADD = @PACKAGE_LIBS@ @LIBNOTIFY_LIBS@ $(INTLLIBS)

