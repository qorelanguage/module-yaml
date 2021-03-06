moddir = $(userlibdir)

AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

noinst_HEADERS = src/yaml-module.h

USER_MODULES = qlib/YamlRpcClient.qm qlib/YamlRpcHandler.qm qlib/DataStreamUtil.qm qlib/DataStreamClient.qm qlib/DataStreamRequestHandler.qm

EXTRA_DIST = COPYING.LGPL COPYING.MIT AUTHORS README \
	RELEASE-NOTES \
	src/ql_yaml.qpp \
	test/yaml.qtest \
	test/YamlRpcClient.qtest \
	test/YamlRpcHandler.qtest \
	test/DataStreamUtil.qtest \
	test/DataStreamClient.qtest \
	test/DataStreamHandler.qtest \
	examples/yaml-server.q \
	$(USER_MODULES) \
	qore-yaml-module.spec

ACLOCAL_AMFLAGS=-I m4

dist_mod_DATA = $(USER_MODULES)

if COND_DOXYGEN
DOX = yaml YamlRpcClient YamlRpcHandler DataStreamUtil DataStreamClient DataStreamRequestHandler
DOXYGEN_OUTPUT=docs/yaml/html docs/YamlRpcClient/html docs/YamlRpcHandler/html docs/DataStreamUtil/html docs/DataStreamClient/html docs/DataStreamRequestHandler/html
DOXYFILES = docs/doxyfile docs/mainpage.doxygen
DOXYFILES_SRC = docs/doxyfile.tmpl docs/qlib/Doxyfile.tmpl docs/footer_template.html
QLIB_TMP_DOXYFILES = docs/qlib/YamlRpcClient.qm.dox.h docs/qlib/Doxyfile.YamlRpcClient \
	docs/qlib/YamlRpcHandler.qm.dox.h docs/qlib/Doxyfile.YamlRpcHandler \
	docs/qlib/DataStreamUtil.qm.dox.h docs/qlib/Doxyfile.DataStreamUtil \
	docs/qlib/DataStreamClient.qm.dox.h docs/qlib/Doxyfile.DataStreamClient \
	docs/qlib/DataStreamRequestHandler.qm.dox.h docs/qlib/Doxyfile.DataStreamRequestHandler
QDX = qdx
DX_CLEANFILES = ${DOXYFILES} $(QLIB_TMP_DOXYFILES)

docs/qlib/Doxyfile.tmp: docs/qlib/Doxyfile.tmpl
	sed s:__USERMODDIR__:${usermoddir}: $< > $@

docs/qlib/Doxyfile.YamlRpcClient: docs/qlib/Doxyfile.tmp
	$(QDX) -M=qlib/YamlRpcClient.qm:qlib/YamlRpcClient.qm.dox.h $< $@

docs/qlib/YamlRpcClient.qm.dox.h: qlib/YamlRpcClient.qm
	$(QDX) $< $@

docs/YamlRpcClient/html: docs/qlib/YamlRpcClient.qm.dox.h docs/qlib/Doxyfile.YamlRpcClient
	cd docs; $(DOXYGEN_CMD) qlib/Doxyfile.YamlRpcClient
	$(QDX) --post docs/YamlRpcClient/html docs/YamlRpcClient/html/search

docs/qlib/Doxyfile.YamlRpcHandler: docs/qlib/Doxyfile.tmp
	$(QDX) -M=qlib/YamlRpcHandler.qm:qlib/YamlRpcHandler.qm.dox.h $< $@

docs/qlib/YamlRpcHandler.qm.dox.h: qlib/YamlRpcHandler.qm
	$(QDX) $< $@

docs/YamlRpcHandler/html: docs/qlib/YamlRpcHandler.qm.dox.h docs/qlib/Doxyfile.YamlRpcHandler
	cd docs; $(DOXYGEN_CMD) qlib/Doxyfile.YamlRpcHandler
	$(QDX) --post docs/YamlRpcHandler/html docs/YamlRpcHandler/html/search/*html

docs/qlib/Doxyfile.DataStreamUtil: docs/qlib/Doxyfile.tmp
	$(QDX) -M=qlib/DataStreamUtil.qm:qlib/DataStreamUtil.qm.dox.h $< $@

docs/qlib/DataStreamUtil.qm.dox.h: qlib/DataStreamUtil.qm
	$(QDX) $< $@

docs/DataStreamUtil/html: docs/qlib/DataStreamUtil.qm.dox.h docs/qlib/Doxyfile.DataStreamUtil
	cd docs; $(DOXYGEN_CMD) qlib/Doxyfile.DataStreamUtil
	$(QDX) --post docs/DataStreamUtil/html docs/DataStreamUtil/html/search

docs/qlib/Doxyfile.DataStreamClient: docs/qlib/Doxyfile.tmp
	$(QDX) -M=qlib/DataStreamClient.qm:qlib/DataStreamClient.qm.dox.h -tDataStreamUtil.tag=../../DataStreamUtil/html $< $@

docs/qlib/DataStreamClient.qm.dox.h: qlib/DataStreamClient.qm
	$(QDX) $< $@

docs/DataStreamClient/html: docs/qlib/DataStreamClient.qm.dox.h docs/qlib/Doxyfile.DataStreamClient
	cd docs; $(DOXYGEN_CMD) qlib/Doxyfile.DataStreamClient
	$(QDX) --post docs/DataStreamClient/html docs/DataStreamClient/html/search

docs/qlib/Doxyfile.DataStreamRequestHandler: docs/qlib/Doxyfile.tmp
	$(QDX) -M=qlib/DataStreamRequestHandler.qm:qlib/DataStreamRequestHandler.qm.dox.h -tDataStreamUtil.tag=../../DataStreamUtil/html $< $@

docs/qlib/DataStreamRequestHandler.qm.dox.h: qlib/DataStreamRequestHandler.qm
	$(QDX) $< $@

docs/DataStreamRequestHandler/html: docs/qlib/DataStreamRequestHandler.qm.dox.h docs/qlib/Doxyfile.DataStreamRequestHandler
	cd docs; $(DOXYGEN_CMD) qlib/Doxyfile.DataStreamRequestHandler
	$(QDX) --post docs/DataStreamRequestHandler/html docs/DataStreamRequestHandler/html/search

docs/yaml/html: $(DOXYFILES)
	cd docs; $(DOXYGEN_CMD) doxyfile;
	$(QDX) --post docs/yaml/html docs/yaml/html/search

docs/mainpage.doxygen: docs/mainpage.doxygen.tmpl
	$(QDX) -d $< $@

docs/doxyfile: docs/doxyfile.tmpl
	sed -e s/__VERSION__/${VERSION}/ -e s:__USERMODDIR__:${usermoddir}: $< > $@

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
