cmake_minimum_required(VERSION 3.1)

file(READ "libclingcon/clingcon.h" main)
string(REGEX MATCH "#define CLINGCON_VERSION \"([^\"]*)\"" clingov ${main})

project(CLINGCON VERSION "${CMAKE_MATCH_1}" LANGUAGES CXX)
if (POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "No build type selected - using 'Release'")
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(clingo_header_group_name "Clingo Header Files")
set(clingo_header_group
    "${CLINGO_INCLUDE_DIR}/clingo.hh")
source_group("${clingo_header_group_name}" FILES ${clingo_header_group})

# Enable folders in IDEs like Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

include(GNUInstallDirs)
include(CMakeDependentOption)

option(CLINGCON_BUILD_WITH_SYSTEM_CLINGO "if true the local clingo copy will not be used" OFF)
option(CLINGCON_BUILD_STATIC "do not build shared libraries" OFF)
option(CLINGCON_MANAGE_RPATH "set rpath if not installed into system directory" ON)
option(CLINGCON_BUILD_TESTS "build tests" OFF)

CMAKE_DEPENDENT_OPTION(CLINGCON_BUILD_SHARED "build clingo-dl library shared" ON "NOT CLINGCON_BUILD_STATIC" OFF)

mark_as_advanced(CLINGCON_BUILD_STATIC)
mark_as_advanced(CLINGCON_BUILD_SHARED)

if (MSVC)
    set(VC_RELEASE_LINK_OPTIONS /LTCG)
    SET(CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE}    ${VC_RELEASE_LINK_OPTIONS}")
    SET(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} ${VC_RELEASE_LINK_OPTIONS}")
    SET(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${VC_RELEASE_LINK_OPTIONS}")
    SET(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} ${VC_RELEASE_LINK_OPTIONS}")
    if (CLINGCON_BUILD_STATIC)
        # force static runtime
        string(REGEX REPLACE "/MD" "/MT" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
    endif()
endif()

if (POLICY CMP0063 AND CLINGCON_BUILD_SHARED)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
endif()

enable_testing()

if (CLINGCON_MANAGE_RPATH)
    set(CMAKE_SKIP_BUILD_RPATH FALSE)
    set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
    list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_FULL_LIBDIR}" isSystemDir)
    if ("${isSystemDir}" STREQUAL "-1")
        set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
    endif()
endif()

if (NOT CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
endif()
if (NOT CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
if (NOT CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
endif()
if (NOT CMAKE_IMPORT_LIBRARY_PREFIX)
    set(CMAKE_IMPORT_LIBRARY_PREFIX import_)
endif()

if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/clingo/CMakeLists.txt OR CLINGCON_BUILD_WITH_SYSTEM_CLINGO)
    find_package(Clingo REQUIRED)
    message(STATUS "Using clingo's system installation")
else()
    message(STATUS "Using local copy of clingo")
    set(CLINGO_MANAGE_RPATH OFF CACHE INTERNAL "" FORCE)
    set(CLINGO_BUILD_APPS OFF CACHE INTERNAL "" FORCE)
    set(CLINGO_BUILD_TESTS OFF CACHE INTERNAL "" FORCE)
    set(CLINGO_BUILD_EXAMPLES OFF CACHE INTERNAL "" FORCE)
    set(CLINGO_BUILD_STATIC ${CLINGCON_BUILD_STATIC} CACHE BOOL "")
    set(CLINGO_BUILD_SHARED ${CLINGCON_BUILD_SHARED} CACHE BOOL "")
    add_subdirectory(clingo)
endif()

add_subdirectory(libclingcon)
add_subdirectory(app)


