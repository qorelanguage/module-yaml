cmake_minimum_required(VERSION 2.8.3)

project(qore-yaml-module)

set (VERSION_MAJOR 0)
set (VERSION_MINOR 7)
set (VERSION_PATCH 2)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )


find_package(Qore 0.9 REQUIRED)
find_package(LibYAML REQUIRED)

include_directories( ${CMAKE_SOURCE_DIR}/src )
include_directories( ${LIBYAML_INCLUDE_DIR} )

set(QPP_SRC
    src/ql_yaml.qpp
)

set(CPP_SRC
    src/QoreYamlEmitter.cpp
    src/QoreYamlParser.cpp
    src/yaml-module.cpp
)

set(QMOD
    qlib/DataStreamClient.qm
    qlib/DataStreamUtil.qm
    qlib/YamlRpcHandler.qm
    qlib/DataStreamRequestHandler.qm
    qlib/YamlRpcClient.qm
)

# Check for C++11.
include(CheckCXXCompilerFlag)
CHECK_CXX_COMPILER_FLAG("-std=c++11" COMPILER_SUPPORTS_CXX11)
if(COMPILER_SUPPORTS_CXX11)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
else()
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} has no C++11 support. Please use a different C++ compiler.")
endif()

qore_wrap_qpp_value(QPP_SOURCES ${QPP_SRC})

SET (module_name "yaml")

set(QORE_DOX_TMPL_SRC
  docs/mainpage.doxygen.tmpl
)

add_library(${module_name} MODULE ${CPP_SRC} ${QPP_SOURCES} ${CPP_SRC})

if (WIN32 AND MINGW AND MSYS)
    target_compile_definitions(${module_name} PUBLIC BUILDING_DLL)
endif (WIN32 AND MINGW AND MSYS)

qore_external_binary_module(${module_name} "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" ${LIBYAML_LIBRARY})
qore_user_modules("${QMOD}")

qore_external_user_module("qlib/DataStreamUtil.qm" "")
qore_external_user_module("qlib/DataStreamClient.qm" "DataStreamUtil")
qore_external_user_module("qlib/DataStreamRequestHandler.qm" "DataStreamUtil")
qore_external_user_module("qlib/YamlRpcClient.qm" "")
qore_external_user_module("qlib/YamlRpcHandler.qm" "")

qore_dist("${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

qore_config_info()

FIND_PACKAGE(Doxygen)
if (DOXYGEN_FOUND)
    qore_wrap_dox(QORE_DOX_SRC ${QORE_DOX_TMPL_SRC})
    add_custom_target(QORE_MOD_DOX_FILES DEPENDS ${QORE_DOX_SRC})
    add_dependencies(docs-module QORE_MOD_DOX_FILES)
endif()

