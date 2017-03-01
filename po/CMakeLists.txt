## languages
file(STRINGS ${CMAKE_CURRENT_SOURCE_DIR}/LINGUAS gettext_languages
	REGEX "^[^#].*")

## 'update_pot' target
add_custom_target(
	update-pot
	COMMAND ${XGETTEXT_EXECUTABLE}
		-o ${PACKAGE}.pot
		-p "${CMAKE_CURRENT_SOURCE_DIR}"
		--default-domain=pnmixer
		--directory="${CMAKE_SOURCE_DIR}"
		--add-comments=TRANSLATORS:
		--from-code=UTF-8
		--keyword=_
		--files-from="${CMAKE_SOURCE_DIR}/po/POTFILES.in"
		--copyright-holder='Copyright (C) 2010-2017 Nick Lanham'
		--package-name="${PACKAGE}"
		--package-version="${PACKAGE_VERSION}"
		--msgid-bugs-address="https://github.com/nicklan/pnmixer/issues"
	DEPENDS
		"${CMAKE_CURRENT_SOURCE_DIR}/${PACKAGE}.pot"
)


## 'update_po' target
set(po_targets)
foreach(lang ${gettext_languages})
	add_custom_target(
		update_po_${lang}
		COMMAND ${GETTEXT_MSGMERGE_EXECUTABLE}
			--lang=${lang}
			"${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po"
			"${CMAKE_CURRENT_SOURCE_DIR}/${PACKAGE}.pot"
			-o "${CMAKE_CURRENT_SOURCE_DIR}/${lang}.new.po"
		COMMAND mv
			-f "${CMAKE_CURRENT_SOURCE_DIR}/${lang}.new.po"
			"${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po"
			"${CMAKE_CURRENT_SOURCE_DIR}/${PACKAGE}.pot"
	)
	LIST(APPEND po_targets update_po_${lang})
endforeach()
add_custom_target(update-po DEPENDS ${po_targets})


## 'pnmixer-translations' target (part of default target)
set(mo_files)
foreach(lang ${gettext_languages})
	add_custom_command(OUTPUT ${lang}.gmo
		COMMAND ${GETTEXT_MSGFMT_EXECUTABLE}
			-c
			--directory="${CMAKE_SOURCE_DIR}"
			-o "${lang}.gmo"
			"${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po"
		DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${lang}.po")
		install(FILES "${CMAKE_CURRENT_BINARY_DIR}/${lang}.gmo" DESTINATION "${CMAKE_INSTALL_FULL_LOCALEDIR}/${lang}/LC_MESSAGES" RENAME "${PACKAGE}.mo")
	LIST(APPEND mo_files ${lang}.gmo)
endforeach()

add_custom_target(pnmixer-translations ALL DEPENDS ${mo_files})

