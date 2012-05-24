modverdir = $(libdir)/$(mymodverdir)

AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

noinst_HEADERS = src/yaml-module.h

USER_MODULES = qlib/YamlRpcClient.qm qlib/YamlRpcHandler.qm

EXTRA_DIST = COPYING ChangeLog AUTHORS README \
	RELEASE-NOTES \
	src/ql_yaml.qpp \
	test/yaml-test.q \
	examples/yaml-server.q \
	$(USER_MODULES) \
	qore-yaml-module.spec

ACLOCAL_AMFLAGS=-I m4

dist_modver_DATA = $(USER_MODULES)

if COND_DOXYGEN
DOX = yaml YamlRpcClient YamlRpcHandler
DOXYGEN_OUTPUT=docs/yaml/html docs/YamlRpcClient/html docs/YamlRpcHandler/html
DOXYFILES = docs/doxyfile docs/mainpage.doxygen
DOXYFILES_SRC = docs/doxyfile.tmpl docs/qlib/Doxyfile.tmpl docs/footer_template.html
QLIB_TMP_DOXYFILES = docs/qlib/YamlRpcClient.qm.dox.h docs/qlib/Doxyfile.YamlRpcClient \
	docs/qlib/YamlRpcHandler.qm.dox.h docs/qlib/Doxyfile.YamlRpcHandler
QDX = qdx
DX_CLEANFILES = ${DOXYFILES} $(QLIB_TMP_DOXYFILES)

docs/qlib/Doxyfile.YamlRpcClient: docs/qlib/Doxyfile.tmpl
	$(QDX) -M=qlib/YamlRpcClient.qm:docs/qlib/YamlRpcClient.qm.dox.h $< $@

docs/qlib/YamlRpcClient.qm.dox.h: qlib/YamlRpcClient.qm
	$(QDX) $< $@

docs/YamlRpcClient/html: docs/qlib/YamlRpcClient.qm.dox.h docs/qlib/Doxyfile.YamlRpcClient
	$(DOXYGEN_CMD) docs/qlib/Doxyfile.YamlRpcClient
	$(QDX) --post docs/YamlRpcClient/html/*html
	$(QDX) --post docs/YamlRpcClient/html/search/*html

docs/qlib/Doxyfile.YamlRpcHandler: docs/qlib/Doxyfile.tmpl
	$(QDX) -M=qlib/YamlRpcHandler.qm:docs/qlib/YamlRpcHandler.qm.dox.h $< $@

docs/qlib/YamlRpcHandler.qm.dox.h: qlib/YamlRpcHandler.qm
	$(QDX) $< $@

docs/YamlRpcHandler/html: docs/qlib/YamlRpcHandler.qm.dox.h docs/qlib/Doxyfile.YamlRpcHandler
	$(DOXYGEN_CMD) docs/qlib/Doxyfile.YamlRpcHandler
	$(QDX) --post docs/YamlRpcHandler/html/*html
	$(QDX) --post docs/YamlRpcHandler/html/search/*html

docs/yaml/html: $(DOXYFILES)
	cd docs; $(DOXYGEN_CMD) doxyfile;
	$(QDX) --post docs/yaml/html/*html
	$(QDX) --post docs/yaml/html/search/*html

doxygen-doc: $(DOXYGEN_OUTPUT)

EXTRA_DIST += $(DOXYGEN_OUTPUT)

html-local: $(DOXYGEN_OUTPUT)

install-html-local:
	for dox in $(DOX); do $(MKDIR_P) $(DESTDIR)$(datadir)/$(PACKAGE)/docs/$$dox/html/search; done
	for dir in $(DOXYGEN_OUTPUT); do \
	   for f in $$dir/*; do if [ -f $$f ]; then $(INSTALL_DATA) $$f $(DESTDIR)$(datadir)/$(PACKAGE)/$$dir; fi; done; \
	   for f in $$dir/search/*; do $(INSTALL_DATA) $$f $(DESTDIR)$(datadir)/$(PACKAGE)/$$dir/search; done; \
	done

clean-local:
	$(RM) -rf $(DOXYGEN_OUTPUT)

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
	$(QDX) -d $< $@

docs/doxyfile: docs/doxyfile.tmpl
	sed s/__VERSION__/${VERSION}/ $< > $@
