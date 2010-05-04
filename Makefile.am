AUTOMAKE_OPTIONS = foreign

SUBDIRS = src

noinst_HEADERS = src/yaml-module.h

EXTRA_DIST = COPYING ChangeLog AUTHORS README \
	RELEASE-NOTES \
	test/yaml-test.q \
	docs/yaml-module-doc.html \
	qore-yaml-module.spec

ACLOCAL_AMFLAGS=-I m4

libtool: $(LIBTOOL_DEPS)
	$(SHELL) ./config.status --recheck
