# symlink the given files to the corresponding binary directory in case
# we do out-of-source build
macro(symlink_to_binary_dir _files)
	foreach(_file ${_files})
		get_filename_component(_filename ${_file} NAME)
		file(RELATIVE_PATH _rel_syml "${CMAKE_CURRENT_BINARY_DIR}" "${_file}")
		if (NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}/${_filename}")
			execute_process(
				COMMAND ln -s
					"${_rel_syml}"
					"${CMAKE_CURRENT_BINARY_DIR}/${_filename}"
				WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
				)
		endif()
	endforeach()
endmacro(symlink_to_binary_dir)

