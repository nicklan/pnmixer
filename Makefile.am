## Process this file with automake to produce Makefile.in

SUBDIRS = src po

EXTRA_DIST = \
	autogen.sh 

# Install "pnmixer.desktop" file
desktopdir = $(datadir)/applications
dist_desktop_DATA = data/pnmixer.desktop

# Install .xml interface files
uidir = $(datadir)/pnmixer/ui
if WITH_GTK3
ui_DATA = data/about-gtk3.xml data/popup_window-gtk3.xml data/prefs-gtk3.xml
else
ui_DATA = data/about-gtk2.xml data/popup_window-gtk2.xml data/prefs-gtk2.xml
endif

uninstall-local:
	@$(NORMAL_UNINSTALL)
	rm -rf $(DESTDIR)$(pkgdatadir)	

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
	  $(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
	  for pixmap in $(srcdir)/pixmaps/*; do \
	    if test -f $$pixmap; then \
	      $(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
	    fi \
	  done \
	fi

dist-hook:
	if test -d pixmaps; then \
	  mkdir $(distdir)/pixmaps; \
	  for pixmap in pixmaps/*; do \
	    if test -f $$pixmap; then \
	      cp -p $$pixmap $(distdir)/pixmaps; \
	    fi \
	  done \
	fi

