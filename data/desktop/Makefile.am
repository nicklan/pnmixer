desktopdir = $(datadir)/applications
desktop_in_files = pnmixer.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

# Translate with gettext
%.desktop: %.desktop.in
	msgfmt --desktop -d $(top_srcdir)/po --template $< -o $@

EXTRA_DIST = $(desktop_in_files)

CLEANFILES = $(desktop_DATA)
