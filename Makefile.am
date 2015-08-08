## Process this file with automake to produce Makefile.in

SUBDIRS = data po src

EXTRA_DIST = \
	autogen.sh 

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
