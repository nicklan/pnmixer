if(ENABLE_NLS)
	add_custom_command(OUTPUT pnmixer.desktop
		COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
			--desktop
			-d "${CMAKE_SOURCE_DIR}/po"
			--template ${CMAKE_CURRENT_SOURCE_DIR}/pnmixer.desktop.in
			-o ${CMAKE_CURRENT_BINARY_DIR}/pnmixer.desktop
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/pnmixer.desktop.in")
else(ENABLE_NLS)
	add_custom_command(OUTPUT pnmixer.desktop
		COMMAND cp ${CMAKE_CURRENT_SOURCE_DIR}/pnmixer.desktop.in
			${CMAKE_CURRENT_BINARY_DIR}/pnmixer.desktop
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/pnmixer.desktop.in")
endif(ENABLE_NLS)

add_custom_target(desktop-files ALL DEPENDS pnmixer.desktop)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/pnmixer.desktop" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications")

