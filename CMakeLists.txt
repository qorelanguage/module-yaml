cmake_minimum_required(VERSION 2.8.3)

project(qore-yaml-module)

set (VERSION_MAJOR 0)
set (VERSION_MINOR 5)
set (VERSION_PATCH 0)

# where to look first for cmake modules, before ${CMAKE_ROOT}/Modules/ is checked
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )


find_package(Qore REQUIRED)
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

qore_wrap_qpp_value(QPP_SOURCES ${QPP_SRC})

SET (module_name "yaml")

add_library(${module_name} SHARED ${CPP_SRC} ${QPP_SOURCES} ${CPP_SRC})

if (WIN32 AND MINGW AND MSYS)
    target_compile_definitions(${module_name} PUBLIC BUILDING_DLL)
endif (WIN32 AND MINGW AND MSYS)

qore_binary_module(${module_name} "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}" ${LIBYAML_LIBRARY})
qore_user_modules("${QMOD}")

qore_dist("${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

qore_config_info()


