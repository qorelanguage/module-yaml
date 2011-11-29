AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

DOXYGEN_QORE_API_FILTER=docs/doxygen-api-filter.q
DOXYGEN_OUTPUT=docs/xml

DOXYFILES = docs/doxyfile \
	docs/mainpage.doxygen

noinst_HEADERS = src/yaml-module.h

EXTRA_DIST = COPYING ChangeLog AUTHORS README \
	RELEASE-NOTES \
	test/yaml-test.q \
	examples/HttpServer.qc \
	examples/YamlRpcHandler.qc \
	examples/YamlRpcClient.qc \
	docs/yaml-module-doc.html \
	qore-yaml-module.spec

ACLOCAL_AMFLAGS=-I m4

if COND_DOXYGEN
DX_CLEANFILES = ${DOXYFILES}
docs/xml/html: $(DOXYFILES)
	cd docs; $(DOXYGEN_CMD) doxyfile;
	$(DOXYGEN_QORE_API_FILTER) --post $(DOXYGEN_OUTPUT)/html/*html
	$(DOXYGEN_QORE_API_FILTER) --post $(DOXYGEN_OUTPUT)/html/search/*html

doxygen-doc: docs/xml/html

EXTRA_DIST += docs/xml/html

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
