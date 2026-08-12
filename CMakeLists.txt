cmake_minimum_required(VERSION 3.5)

project(qore-yaml-module)

set (VERSION_MAJOR 1)
set (VERSION_MINOR 2)
set (VERSION_PATCH 0)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )


find_package(Qore 0.9 REQUIRED)
find_package(LibYAML REQUIRED)

include_directories( ${CMAKE_SOURCE_DIR}/src )
include_directories( ${LIBYAML_INCLUDE_DIR} )

set(QPP_SRC
    src/ql_yaml.qpp
    src/QC_YamlSaxParser.qpp
    src/QC_YamlDocumentIterator.qpp
    src/QC_YamlStreamWriter.qpp
)

set(CPP_SRC
    src/QoreYamlEmitter.cpp
    src/QoreYamlParser.cpp
    src/QoreYamlSaxParser.cpp
    src/QoreYamlDocumentIterator.cpp
    src/QoreYamlStreamWriter.cpp
    src/yaml-module.cpp
    src/yaml-scalar-util.cpp
    src/YamlStreamReadHandler.cpp
)

set(QMOD
    qlib/DataStreamClient.qm
    qlib/DataStreamClientIo.qm
    qlib/DataStreamUtil.qm
    qlib/YamlRpcHandler.qm
    qlib/DataStreamRequestHandler.qm
    qlib/YamlRpcClient.qm
    qlib/YamlTagHandler.qm
    qlib/YamlSchema.qm
    qlib/YamlRpcClientIo
)

# Check for C++11.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

qore_wrap_qpp_value(QPP_SOURCES METALIST QPP_META_SOURCES ${QPP_SRC})

SET (module_name "yaml")

set(QORE_DOX_TMPL_SRC
  docs/mainpage.doxygen.tmpl
)

add_library(${module_name} MODULE ${CPP_SRC} ${QPP_SOURCES} ${CPP_SRC})

if (WIN32 AND MINGW AND MSYS)
    target_compile_definitions(${module_name} PUBLIC BUILDING_DLL)
endif (WIN32 AND MINGW AND MSYS)

if (DEFINED ENV{DOXYGEN_EXECUTABLE})
    set(DOXYGEN_EXECUTABLE $ENV{DOXYGEN_EXECUTABLE})
endif()

qore_external_binary_module(${module_name} "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" ${LIBYAML_LIBRARY})
qore_user_modules("${QMOD}")

qore_external_user_module("qlib/DataStreamUtil.qm" "")
qore_external_user_module("qlib/DataStreamClient.qm" "DataStreamUtil")
qore_external_user_module("qlib/DataStreamClientIo.qm" "DataStreamUtil")
qore_external_user_module("qlib/DataStreamRequestHandler.qm" "DataStreamUtil")
qore_external_user_module("qlib/YamlRpcClient.qm" "YamlRpcClientIo")
qore_external_user_module("qlib/YamlRpcHandler.qm" "")
qore_external_user_module("qlib/YamlDocumentDataProvider" "")
qore_external_user_module("qlib/YamlTagHandler.qm" "")
qore_external_user_module("qlib/YamlSchema.qm" "")
qore_external_user_module("qlib/Connect" "")
qore_external_user_module("qlib/ConnectDataProvider" "")
qore_external_user_module("qlib/YamlRpcClientIo" "")

# Reconcile the installed i18n catalog root against the fragments these modules
# currently own.  Installing catalogs is otherwise additive: a fragment installed by
# an earlier generation of these sources survives every later install, is still
# discovered, and makes catalog composition fail as soon as the current generation
# describes the same app differently.
#
# This must come after every call above that installs catalogs, both because those
# calls record the fragment set and because install() rules run in declaration order,
# which is what publishes the manifest after the fragments it describes.  Removals are
# scoped to this project, so fragments Qore and other products install into the same
# shared catalog root are untouched.  Guarded because the helper arrived in Qore 2.3.
if (COMMAND qore_finalize_catalog_install)
    qore_finalize_catalog_install()
endif()

# install qpp metadata files
if (COMMAND qore_install_qpp_metadata)
    qore_install_qpp_metadata(${module_name} ${QPP_META_SOURCES})
else()
    install(FILES ${QPP_META_SOURCES}
            DESTINATION ${CMAKE_INSTALL_DATADIR}/qore/metadata)
endif()

qore_dist("${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

qore_config_info()

if (DOXYGEN_FOUND)
    qore_wrap_dox(QORE_DOX_SRC ${QORE_DOX_TMPL_SRC})
    add_custom_target(QORE_MOD_DOX_FILES DEPENDS ${QORE_DOX_SRC})
    add_dependencies(docs-module QORE_MOD_DOX_FILES)
endif()

