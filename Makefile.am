## Process this file with automake to produce Makefile.in

SUBDIRS = src po

EXTRA_DIST = \
	autogen.sh 

# Install "pnmixer.desktop" file
desktopdir = $(datadir)/applications
dist_desktop_DATA = data/pnmixer.desktop

# Install .glade interface files
uidir = $(datadir)/pnmixer/ui
if WITH_GTK3
dist_ui_DATA = data/about-gtk3.glade data/popup_window-gtk3.glade data/prefs-gtk3.glade
else
dist_ui_DATA = data/about-gtk2.glade data/popup_window-gtk2.glade data/prefs-gtk2.glade
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

doc:
	cd "$(top_srcdir)/src"; \
	doxygen; \
	cd "$(top_srcdir)"

install-doc: doc
	$(MKDIR_P) "$(DESTDIR)$(htmldir)"
	cp -pR "$(top_srcdir)"/src/html/* "$(DESTDIR)$(htmldir)"/

clean-local:
	rm -rf src/html src/latex
