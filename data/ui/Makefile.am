gtk3_ui_files = about-gtk3.glade popup-menu-gtk3.glade popup-window-horizontal-gtk3.glade popup-window-vertical-gtk3.glade prefs-gtk3.glade
gtk2_ui_files = about-gtk2.glade popup-menu-gtk2.glade popup-window-horizontal-gtk2.glade popup-window-vertical-gtk2.glade prefs-gtk2.glade

uidir = $(pkgdatadir)/ui
if WITH_GTK3
ui_DATA = $(gtk3_ui_files)
else
ui_DATA = $(gtk2_ui_files)
endif

EXTRA_DIST =			\
	$(gtk3_ui_files)	\
	$(gtk2_ui_files)
