MNAME=yaml

if COND_SINGLE_COMPILATION_UNIT
YAML_SOURCES = single-compilation-unit.cpp
else
YAML_SOURCES = yaml-module.cpp QoreYamlEmitter.cpp QoreYamlParser.cpp
endif

lib_LTLIBRARIES = yaml.la
yaml_la_SOURCES = $(YAML_SOURCES)
yaml_la_LDFLAGS = -module -avoid-version ${YAML_LIBS} ${MODULE_LDFLAGS}

INCLUDES = -I$(top_srcdir)/include $(YAML_CPPFLAGS)

AM_CPPFLAGS=$(QORE_CPPFLAGS)

if COND_PROFILE
AM_CXXFLAGS=-g -pg
else
if COND_DEBUG
AM_CXXFLAGS=-g
endif
endif

if COND_MINGW
RM_OTHER=$(DESTDIR)$(libdir)/$(MNAME).dll.a
endif

install-exec-hook:
	mv $(DESTDIR)$(libdir)/$(MNAME).$(MODULE_SUFFIX) $(DESTDIR)$(libdir)/$(MNAME)-api-$(MODULE_API).qmod
	$(RM) $(DESTDIR)$(libdir)/$(MNAME).la $(RM_OTHER)

