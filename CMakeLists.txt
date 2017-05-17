cmake_minimum_required(VERSION 3.1)

file(READ "app/clingcon/src/version.h" clingconh)
string(REGEX MATCH "#define CLINGCON_VERSION \"([^\"]*)\"" clingconv ${clingconh})

project(CLINGCON VERSION "${CMAKE_MATCH_1}" LANGUAGES C CXX)
if (POLICY CMP0063)
    cmake_policy(SET CMP0063 NEW)
endif()
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
	message(STATUS "No build type selected - using 'Release'")
	set(CMAKE_BUILD_TYPE "Release")
endif()

include(CMakeDependentOption)
#list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/clingo/cmake")

# Enable folders in IDEs like Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)



set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


option(CLINGCON_BUILD_TESTS       "build tests"                   OFF)

option(CLINGCON_BUILD_APPS        "build applications"             ON)

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

if (POLICY CMP0063 AND (CLINGO_BUILD_SHARED OR PYTHONLIBS_FOUND OR LUA_FOUND))
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
endif()

enable_testing()

# NOTE: assumes that submodule has been initialized
set(clingcon_library_targets libclingo libgringo libpotassco libclasp libreify libopts liblp liborder libclingcon)

set(CLINGO_BUILD_STATIC ON CACHE BOOL "need to build static")
set(CLINGO_BUILD_APPS OFF CACHE BOOL "no other apps")
set(CLASP_BUILD_TEST OFF CACHE BOOL "no clasp tests")
add_subdirectory(clingo)
add_subdirectory(libcsp)

if (CLINGCON_BUILD_APPS)
    add_subdirectory(app/clingcon)
endif()

#if (NOT CLINGO_BUILD_STATIC AND (CLINGO_BUILD_SHARED OR PYTHONLIBS_FOUND OR LUA_FOUND))
#    foreach(target ${clingo_library_targets})
#        set_target_properties(${target} PROPERTIES POSITION_INDEPENDENT_CODE ON)
#    endforeach()
#endif()

