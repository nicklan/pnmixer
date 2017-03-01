## sources
CONFIGURE_FILE(${CMAKE_CURRENT_SOURCE_DIR}/config.h.in
   ${CMAKE_CURRENT_BINARY_DIR}/config.h)
set(PNMixer_sources
	alsa.c
	audio.c
	hotkey.c
	hotkeys.c
	main.c
	notif.c
	prefs.c
	support-intl.c
	support-log.c
	support-ui.c
	ui-about-dialog.c
	ui-hotkey-dialog.c
	ui-popup-menu.c
	ui-popup-window.c
	ui-prefs-dialog.c
	ui-tray-icon.c
)


## includes
include_directories(
	"${CMAKE_CURRENT_BINARY_DIR}"
)


## libraries
set(default_deps
   	alsa
	glib-2.0
	x11
)

if(WITH_GTK3)
	LIST(APPEND default_deps "gtk+-3.0>=3.12.0")
else(WITH_GTK3)
	LIST(APPEND default_deps "gtk+-2.0>=2.24.0")
endif(WITH_GTK3)

if(WITH_LIBNOTIFY)
	LIST(APPEND default_deps "libnotify")
endif(WITH_LIBNOTIFY)

pkg_check_modules(PNMixer_DEPS REQUIRED
	${default_deps}
)


## main pnmixer target
add_executable(pnmixer ${PNMixer_sources})
# and its flags
target_link_libraries(pnmixer "${PNMixer_DEPS_LDFLAGS}")
target_link_libraries(pnmixer m)
target_compile_options(pnmixer PUBLIC "${PNMixer_DEPS_CFLAGS}")
target_compile_definitions(pnmixer PUBLIC -DHAVE_CONFIG_H)

install(TARGETS pnmixer DESTINATION "${CMAKE_INSTALL_BINDIR}")


## doc target
if(BUILD_DOCUMENTATION)
	find_package(Doxygen)
	if(NOT DOXYGEN_FOUND)
		message(FATAL_ERROR "Doxygen is needed to build the documentation. Please install it correctly.")
	endif()

	CONFIGURE_FILE("${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in"
		"${CMAKE_CURRENT_BINARY_DIR}/Doxyfile")

	# always built
	add_custom_target(doc ALL
		${DOXYGEN_EXECUTABLE} "${CMAKE_CURRENT_BINARY_DIR}/Doxyfile"
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
	install(DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/html"
		DESTINATION "${CMAKE_INSTALL_DOCDIR}")
endif(BUILD_DOCUMENTATION)

