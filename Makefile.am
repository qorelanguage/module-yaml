moddir = $(userlibdir)

AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

noinst_HEADERS = src/yaml-module.h

USER_MODULES = qlib/YamlRpcClient.qm qlib/YamlRpcHandler.qm qlib/DataStreamUtil.qm qlib/DataStreamClient.qm qlib/DataStreamRequestHandler.qm qlib/YamlSchema.qm qlib/YamlTagHandler.qm

EXTRA_DIST = COPYING.LGPL COPYING.MIT AUTHORS README \
	RELEASE-NOTES \
	src/ql_yaml.qpp \
	src/QC_YamlSaxParser.qpp \
	src/QC_YamlDocumentIterator.qpp \
	src/QC_YamlStreamWriter.qpp \
	test/yaml.qtest \
	test/Yaml12.qtest \
	test/YamlRpcClient.qtest \
	test/YamlRpcHandler.qtest \
	test/DataStreamUtil.qtest \
	test/DataStreamClient.qtest \
	test/DataStreamHandler.qtest \
	test/YamlSaxParser.qtest \
	test/YamlDocumentIterator.qtest \
	test/YamlDocumentDataProvider.qtest \
	test/YamlStreamWriter.qtest \
	test/YamlSchema.qtest \
	test/YamlTagHandler.qtest \
	test/YamlInterrupt.qtest \
	test/Connect.qtest \
	test/ConnectGrpc.qtest \
	test/ConnectStreaming.qtest \
	test/DataStreamDualProtocol.qtest \
	qlib/YamlDocumentDataProvider/YamlDocumentDataProvider.qm \
	qlib/YamlDocumentDataProvider/YamlDocumentDataProviderFactory.qc \
	qlib/YamlDocumentDataProvider/YamlDocumentReadDataProvider.qc \
	qlib/Connect/Connect.qm \
	qlib/Connect/ConnectCodec.qc \
	qlib/Connect/YamlConnectCodec.qc \
	qlib/Connect/ConnectHelper.qc \
	qlib/Connect/ConnectError.qc \
	qlib/Connect/ConnectClient.qc \
	qlib/Connect/ConnectClientStream.qc \
	qlib/Connect/ConnectHandler.qc \
	qlib/Connect/ConnectServerStream.qc \
	qlib/Connect/JsonConnectCodec.qc \
	qlib/Connect/CborConnectCodec.qc \
	qlib/Connect/ProtobufConnectCodec.qc \
	qlib/Connect/ConnectCompression.qc \
	qlib/Connect/GrpcProtocolAdapter.qc \
	qlib/ConnectDataProvider/ConnectDataProvider.qm \
	qlib/ConnectDataProvider/ConnectDataProviderBase.qc \
	qlib/ConnectDataProvider/ConnectServiceDataProvider.qc \
	qlib/ConnectDataProvider/ConnectMethodDataProvider.qc \
	qlib/ConnectDataProvider/ConnectDataProviderFactory.qc \
	qlib/ConnectDataProvider/ConnectConnection.qc \
	test/ConnectDataProvider.qtest \
	examples/yaml-server.q \
	$(USER_MODULES) \
	qore-yaml-module.spec

ACLOCAL_AMFLAGS=-I m4

dist_mod_DATA = $(USER_MODULES)

if COND_DOXYGEN
DOX = yaml YamlRpcClient YamlRpcHandler DataStreamUtil DataStreamClient DataStreamRequestHandler YamlSchema YamlTagHandler YamlDocumentDataProvider Connect ConnectDataProvider
DOXYGEN_OUTPUT=docs/yaml/html docs/YamlRpcClient/html docs/YamlRpcHandler/html docs/DataStreamUtil/html docs/DataStreamClient/html docs/DataStreamRequestHandler/html docs/YamlSchema/html docs/YamlTagHandler/html docs/YamlDocumentDataProvider/html docs/Connect/html docs/ConnectDataProvider/html
DOXYFILES = docs/doxyfile docs/mainpage.doxygen
DOXYFILES_SRC = docs/doxyfile.tmpl docs/qlib/Doxyfile.tmpl docs/footer_template.html
QLIB_TMP_DOXYFILES = docs/qlib/YamlRpcClient.qm.dox.h docs/qlib/Doxyfile.YamlRpcClient \
	docs/qlib/YamlRpcHandler.qm.dox.h docs/qlib/Doxyfile.YamlRpcHandler \
	docs/qlib/DataStreamUtil.qm.dox.h docs/qlib/Doxyfile.DataStreamUtil \
	docs/qlib/DataStreamClient.qm.dox.h docs/qlib/Doxyfile.DataStreamClient \
	docs/qlib/DataStreamRequestHandler.qm.dox.h docs/qlib/Doxyfile.DataStreamRequestHandler \
	docs/qlib/YamlSchema.qm.dox.h docs/qlib/Doxyfile.YamlSchema \
	docs/qlib/YamlTagHandler.qm.dox.h docs/qlib/Doxyfile.YamlTagHandler \
	docs/qlib/YamlDocumentDataProvider.qm.dox.h docs/qlib/Doxyfile.YamlDocumentDataProvider \
	docs/qlib/Connect.qm.dox.h docs/qlib/Doxyfile.Connect \
	docs/qlib/ConnectDataProvider.qm.dox.h docs/qlib/Doxyfile.ConnectDataProvider
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

docs/qlib/Doxyfile.YamlSchema: docs/qlib/Doxyfile.tmp
	$(QDX) -M=qlib/YamlSchema.qm:qlib/YamlSchema.qm.dox.h $< $@

docs/qlib/YamlSchema.qm.dox.h: qlib/YamlSchema.qm
	$(QDX) $< $@

docs/YamlSchema/html: docs/qlib/YamlSchema.qm.dox.h docs/qlib/Doxyfile.YamlSchema
	cd docs; $(DOXYGEN_CMD) qlib/Doxyfile.YamlSchema
	$(QDX) --post docs/YamlSchema/html docs/YamlSchema/html/search

docs/qlib/Doxyfile.YamlTagHandler: docs/qlib/Doxyfile.tmp
	$(QDX) -M=qlib/YamlTagHandler.qm:qlib/YamlTagHandler.qm.dox.h $< $@

docs/qlib/YamlTagHandler.qm.dox.h: qlib/YamlTagHandler.qm
	$(QDX) $< $@

docs/YamlTagHandler/html: docs/qlib/YamlTagHandler.qm.dox.h docs/qlib/Doxyfile.YamlTagHandler
	cd docs; $(DOXYGEN_CMD) qlib/Doxyfile.YamlTagHandler
	$(QDX) --post docs/YamlTagHandler/html docs/YamlTagHandler/html/search

docs/qlib/Doxyfile.YamlDocumentDataProvider: docs/qlib/Doxyfile.tmp
	$(QDX) -M=qlib/YamlDocumentDataProvider/YamlDocumentDataProvider.qm:qlib/YamlDocumentDataProvider.qm.dox.h $< $@

docs/qlib/YamlDocumentDataProvider.qm.dox.h: qlib/YamlDocumentDataProvider/YamlDocumentDataProvider.qm
	$(QDX) $< $@

docs/YamlDocumentDataProvider/html: docs/qlib/YamlDocumentDataProvider.qm.dox.h docs/qlib/Doxyfile.YamlDocumentDataProvider
	cd docs; $(DOXYGEN_CMD) qlib/Doxyfile.YamlDocumentDataProvider
	$(QDX) --post docs/YamlDocumentDataProvider/html docs/YamlDocumentDataProvider/html/search

docs/qlib/Doxyfile.Connect: docs/qlib/Doxyfile.tmp
	$(QDX) -M=qlib/Connect/Connect.qm:qlib/Connect.qm.dox.h $< $@

docs/qlib/Connect.qm.dox.h: qlib/Connect/Connect.qm
	$(QDX) $< $@

docs/Connect/html: docs/qlib/Connect.qm.dox.h docs/qlib/Doxyfile.Connect
	cd docs; $(DOXYGEN_CMD) qlib/Doxyfile.Connect
	$(QDX) --post docs/Connect/html docs/Connect/html/search

docs/qlib/Doxyfile.ConnectDataProvider: docs/qlib/Doxyfile.tmp
	$(QDX) -M=qlib/ConnectDataProvider/ConnectDataProvider.qm:qlib/ConnectDataProvider.qm.dox.h $< $@

docs/qlib/ConnectDataProvider.qm.dox.h: qlib/ConnectDataProvider/ConnectDataProvider.qm
	$(QDX) $< $@

docs/ConnectDataProvider/html: docs/qlib/ConnectDataProvider.qm.dox.h docs/qlib/Doxyfile.ConnectDataProvider
	cd docs; $(DOXYGEN_CMD) qlib/Doxyfile.ConnectDataProvider
	$(QDX) --post docs/ConnectDataProvider/html docs/ConnectDataProvider/html/search

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
