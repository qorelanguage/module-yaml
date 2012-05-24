modverdir = $(libdir)/$(mymodverdir)

AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

noinst_HEADERS = src/yaml-module.h

USER_MODULES = qlib/YamlRpcClient.qm

EXTRA_DIST = COPYING ChangeLog AUTHORS README \
	RELEASE-NOTES \
	src/ql_yaml.qpp \
	test/yaml-test.q \
	examples/HttpServer.qc \
	examples/YamlRpcHandler.qc \
	$(USER_MODULES) \
	qore-yaml-module.spec

ACLOCAL_AMFLAGS=-I m4

dist_modver_DATA = $(USER_MODULES)

if COND_DOXYGEN
DOXYGEN_QORE_API_FILTER = docs/doxygen-api-filter.q
DOXYGEN_OUTPUT=docs/yaml/html docs/YamlRpcClient/html
DOXYFILES = docs/doxyfile docs/mainpage.doxygen
DOXYFILES_SRC = docs/doxyfile.tmpl docs/qlib/Doxyfile.tmpl docs/footer_template.html
QLIB_TMP_DOXYFILES = docs/qlib/YamlRpcClient.qm.dox.h docs/qlib/Doxyfile.YamlRpcClient
QDX = qdx
DX_CLEANFILES = ${DOXYFILES} $(QLIB_TMP_DOXYFILES)

docs/qlib/Doxyfile.YamlRpcClient: docs/qlib/Doxyfile.tmpl
	$(QDX) -M=qlib/YamlRpcClient.qm:docs/qlib/YamlRpcClient.qm.dox.h $< $@

docs/qlib/YamlRpcClient.qm.dox.h: qlib/YamlRpcClient.qm
	$(QDX) $< $@

docs/YamlRpcClient/html: docs/qlib/YamlRpcClient.qm.dox.h docs/qlib/Doxyfile.YamlRpcClient
	$(DOXYGEN_CMD) docs/qlib/Doxyfile.YamlRpcClient
	$(DOXYGEN_QORE_API_FILTER) --post docs/YamlRpcClient/html/*html
	$(DOXYGEN_QORE_API_FILTER) --post docs/YamlRpcClient/html/search/*html

docs/yaml/html: $(DOXYFILES)
	cd docs; $(DOXYGEN_CMD) doxyfile;
	$(DOXYGEN_QORE_API_FILTER) --post docs/yaml/html/*html
	$(DOXYGEN_QORE_API_FILTER) --post docs/yaml/html/search/*html

doxygen-doc: $(DOXYGEN_OUTPUT)

EXTRA_DIST += $(DOXYGEN_OUTPUT)

html-local: $(DOXYGEN_OUTPUT)

install-html-local:
	$(MKDIR_P) $(DESTDIR)$(datadir)/$(PACKAGE)/docs/yaml/html/search
	$(MKDIR_P) $(DESTDIR)$(datadir)/$(PACKAGE)/docs/YamlRpcClient/html/search
	@for dir in $(DOXYGEN_OUTPUT); do \
	   for f in $$dir/*; do if [ -f $$f ]; then $(INSTALL_DATA) $$f $(DESTDIR)$(datadir)/$(PACKAGE)/$$dir; fi; done; \
	   for f in $$dir/search/*; do $(INSTALL_DATA) $$f $(DESTDIR)$(datadir)/$(PACKAGE)/$$dir/search; done; \
	done

uninstall-local:
	$(RM) -rf $(DESTDIR)$(datadir)/$(PACKAGE)/docs

else
doxygen-doc:
	echo no doxygen support
endif

mostlyclean-local:
	rm -rf ${DOXYGEN_OUTPUT} ${DX_CLEANFILES}

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck

docs/mainpage.doxygen: docs/mainpage.doxygen.tmpl
	$(DOXYGEN_QORE_API_FILTER) -l $<

docs/doxyfile: docs/doxyfile.tmpl
	sed s/__VERSION__/${VERSION}/ $< > $@
