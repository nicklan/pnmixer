if(WITH_GTK3)
	file(GLOB ui_files *-gtk3.glade)
else(WITH_GTK3)
	file(GLOB ui_files *-gtk2.glade)
endif(WITH_GTK3)

install(FILES ${ui_files} DESTINATION "${CMAKE_INSTALL_DATADIR}/${PACKAGE}/ui")


## make running 'src/pnmixer' from the build directory work
## since pnmixer looks up ui files from a relative path
symlink_to_binary_dir("${ui_files}")

