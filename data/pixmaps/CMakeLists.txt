file(GLOB png_images *.png)

install(FILES
	${png_images}
	DESTINATION "${CMAKE_INSTALL_DATADIR}/${PACKAGE}/pixmaps"
)


## make running 'src/pnmixer' from the build directory work
## since pnmixer looks up pixmaps files from a relative path
symlink_to_binary_dir("${png_images}")

