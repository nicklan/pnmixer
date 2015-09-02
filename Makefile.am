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

clean-local:
	rm -rf src/html src/latex
