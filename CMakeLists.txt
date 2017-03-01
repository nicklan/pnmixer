cmake_minimum_required(VERSION 3.0)
project(PNMixer)


## cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake/Modules/")
include(FindPkgConfig)
include(GNUInstallDirs)
include(PNMixer_macros)


## version and global project vars
set(PNMixer_VERSION_MAJOR 0)
set(PNMixer_VERSION_MINOR 7)
# set (Pnmixer_VERSION_MICRO 0)
set(PACKAGE_VERSION "${PNMixer_VERSION_MAJOR}.${PNMixer_VERSION_MINOR}")
set(PACKAGE "pnmixer")


## options
option(WITH_GTK3 "Use Gtk3 as toolkit" ON)
option(WITH_LIBNOTIFY "Enable sending of notifications" ON)
option(ENABLE_NLS "Enable building of translations" ON)
option(BUILD_DOCUMENTATION "Use Doxygen to create the HTML based API documentation" OFF)


## packages required across multiple subdirectories
if(ENABLE_NLS)
	# find gettext programs
	find_package(Gettext)
	FIND_PROGRAM(XGETTEXT_EXECUTABLE xgettext)
	IF(XGETTEXT_EXECUTABLE)
		MESSAGE(STATUS "Found xgettext: ${XGETTEXT_EXECUTABLE}")
	ELSE(XGETTEXT_EXECUTABLE)
		MESSAGE(FATAL_ERROR "xgettext not found")
	ENDIF(XGETTEXT_EXECUTABLE)
endif(ENABLE_NLS)


## subdirectories
add_subdirectory(data)
if(ENABLE_NLS)
	add_subdirectory(po)
endif(ENABLE_NLS)
add_subdirectory(src)


## additional global targets

# indent-code
add_custom_target(indent-code
	COMMAND "${CMAKE_SOURCE_DIR}/cmake/scripts/astyle.sh"
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# indent-xml
add_custom_target(indent-xml
	COMMAND "${CMAKE_SOURCE_DIR}/cmake/scripts/xmllint.sh"
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")


## install git hook
execute_process(
	COMMAND "${CMAKE_SOURCE_DIR}/cmake/scripts/install-git-hook.sh"
	WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

