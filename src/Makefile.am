if COND_SINGLE_COMPILATION_UNIT
YAML_SOURCES = single-compilation-unit.cpp
else
YAML_SOURCES = yaml-module.cpp QoreYamlEmitter.cpp QoreYamlParser.cpp
endif

lib_LTLIBRARIES = yaml.la
yaml_la_SOURCES = $(YAML_SOURCES)
yaml_la_LDFLAGS = -module -avoid-version ${YAML_LIBS}

INCLUDES = -I$(top_srcdir)/include $(YAML_INCLUDES)

AM_CPPFLAGS=$(QORE_CPPFLAGS)

if COND_PROFILE
AM_CXXFLAGS=-g -pg
else
if COND_DEBUG
AM_CXXFLAGS=-g
endif
endif

install-exec-hook:
	mv $(DESTDIR)$(libdir)/yaml.$(MODULE_SUFFIX) $(DESTDIR)$(libdir)/yaml-api-$(MODULE_API).qmod
	rm -f $(DESTDIR)$(libdir)/yaml.la
