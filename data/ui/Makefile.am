gtk3_ui_files = about-gtk3.xml popup_window-gtk3.xml prefs-gtk3.xml
gtk2_ui_files = about-gtk2.xml popup_window-gtk2.xml prefs-gtk2.xml

uidir = $(pkgdatadir)/ui
if WITH_GTK3
ui_DATA = $(gtk3_ui_files)
else
ui_DATA = $(gtk2_ui_files)
endif

EXTRA_DIST =			\
	$(gtk3_ui_files)	\
	$(gtk2_ui_files)
