## Process this file with automake to produce Makefile.in

SUBDIRS = data po src

EXTRA_DIST = \
	autogen.sh README.md

dist-hook:
	$(MAKE) -C "$(srcdir)/po" "$(GETTEXT_PACKAGE).pot"
	cp "$(srcdir)/po/$(GETTEXT_PACKAGE).pot" "$(distdir)/po/"

uninstall-local:
	@$(NORMAL_UNINSTALL)
	rm -rf $(DESTDIR)$(pkgdatadir)

doc:
	cd "$(top_srcdir)/src"; \
	doxygen; \
	cd "$(top_srcdir)"

install-doc: doc
	$(MKDIR_P) "$(DESTDIR)$(htmldir)"
	cp -pR "$(top_srcdir)"/src/html/* "$(DESTDIR)$(htmldir)"/

indent-code:
	@which astyle >/dev/null || { echo "'astyle' is required, please install it"; exit 1; }
	sed -i 's/[ \t]*$//' src/*.[ch]
	astyle  --style=linux		\
		--indent=tab=8		\
		--align-pointer=name	\
		--suffix=none		\
		src/*.[ch]

indent-xml:
	@which xmllint >/dev/null || { echo "'xmllint' is required, please install it"; exit 1; }
	for f in data/ui/*.glade; do			\
		mv "$$f" "$$f.bck";			\
		xmllint --format "$$f.bck" > "$$f";	\
		rm "$$f.bck";				\
	done

clean-local:
	rm -rf src/html src/latex
