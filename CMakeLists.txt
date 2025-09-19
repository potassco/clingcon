cmake_minimum_required(VERSION 3.22.1)

file(STRINGS "lib/c-api/include/clingcon.h" CLINGCON_VERSION_LINE REGEX "#define CLINGCON_VERSION ")
string(REGEX REPLACE "#define CLINGCON_VERSION[ \t]+\"([^\"]+)\"" "\\1" CLINGCON_VERSION "${CLINGCON_VERSION_LINE}")

project(CLINGCON VERSION "${CLINGCON_VERSION}" LANGUAGES C CXX)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_EXTENSIONS OFF)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(CMakeDependentOption)
include(macros)

option(CLINGCON_CROSSCHECK "enable additional state checking" OFF)
cmake_dependent_option(CLINGCON_BUILD_SHARED "build shared library" ON "NOT DEFINED SKBUILD" ON)
cmake_dependent_option(CLINGCON_MANAGE_RPATH "manage rpath when installing" ON "NOT DEFINED SKBUILD" OFF)
cmake_dependent_option(CLINGCON_USE_VISIBILITY "enable visibility attributes" ON "CLINGCON_BUILD_SHARED" OFF)
cmake_dependent_option(CLINGCON_PROFILE "profile the system" OFF "NOT DEFINED SKBUILD" OFF)
cmake_dependent_option(CLINGCON_BUILD_PYTHON "build python library" ON "NOT DEFINED SKBUILD" ON)
cmake_dependent_option(CLINGCON_BUILD_APP "build app" ON "NOT DEFINED SKBUILD" OFF)
cmake_dependent_option(CLINGCON_BUILD_TESTS "build tests" OFF "NOT DEFINED SKBUILD" OFF)
cmake_dependent_option(CLINGCON_USE_LOCAL_CATCH "use bundled catch." ON "CLINGCON_BUILD_TESTS" OFF)

set(PYCLINGCON_INSTALL "prefix" CACHE STRING "where to install python module: user, system, prefix")
set(PYCLINGCON_INSTALL_DIR "" CACHE STRING "path where to install python module")

if(MSVC)
    set(VC_RELEASE_LINK_OPTIONS /LTCG)
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE    "${CMAKE_EXE_LINKER_FLAGS_RELEASE}    ${VC_RELEASE_LINK_OPTIONS}")
    set(CMAKE_MODULE_LINKER_FLAGS_RELEASE "${CMAKE_MODULE_LINKER_FLAGS_RELEASE} ${VC_RELEASE_LINK_OPTIONS}")
    set(CMAKE_SHARED_LINKER_FLAGS_RELEASE "${CMAKE_SHARED_LINKER_FLAGS_RELEASE} ${VC_RELEASE_LINK_OPTIONS}")
    set(CMAKE_STATIC_LINKER_FLAGS_RELEASE "${CMAKE_STATIC_LINKER_FLAGS_RELEASE} ${VC_RELEASE_LINK_OPTIONS}")
endif()

if(${CMAKE_VERSION} VERSION_LESS "3.26")
    set_property(GLOBAL PROPERTY USE_FOLDERS ON)
endif()

if(CLINGCON_USE_VISIBILITY)
    set(CMAKE_C_VISIBILITY_PRESET hidden)
    set(CMAKE_CXX_VISIBILITY_PRESET hidden)
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)
endif()

set(PYBIND11_NEWPYTHON ON)
set(PYBIND11_FINDPYTHON ON)
set(CLINGCON_INSTALL_DEFAULT OFF)
set(CLINGCON_INSTALL_EXTRA OFF)
set(CLINGCON_INSTALL_WHEEL OFF)

if(DEFINED SKBUILD)
    message(STATUS "Configuring for skbuild")
    set(CLINGCON_INSTALL_WHEEL ON)
    find_package(pybind11 CONFIG REQUIRED)
    execute_process(
        COMMAND "${Python_EXECUTABLE}" -c "import clingo, os; print(os.path.dirname(clingo.__file__))"
        OUTPUT_VARIABLE sp_dir
        OUTPUT_STRIP_TRAILING_WHITESPACE)
    add_library(clingo-interface INTERFACE)
    target_include_directories(clingo-interface INTERFACE ${sp_dir}/clingo-include)
    target_link_directories(clingo-interface INTERFACE ${sp_dir})
    target_link_libraries(clingo-interface INTERFACE clingo)
    add_library(clingo::clingo ALIAS clingo-interface)
else()
    if(CLINGCON_MANAGE_RPATH)
        list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_FULL_LIBDIR}" isSystemDir)
        if(isSystemDir EQUAL -1)
            set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
        endif()
    endif()
    set(CLINGCON_INSTALL_DEFAULT ON)
    if(NOT CLINGCON_BUILD_SHARED)
        set(CLINGCON_INSTALL_EXTRA ON)
    endif()
    if(CLINGCON_BUILD_PYTHON)
        find_package(Python COMPONENTS Interpreter Development Development.Module REQUIRED)
    endif()
    find_package(clingo 6.0.0 REQUIRED)
endif()

if(CLINGCON_BUILD_TESTS)
    enable_testing()
endif()

if(CLINGCON_PROFILE)
    find_package(Gperftools REQUIRED)
endif()

add_subdirectory(third_party)
add_subdirectory(lib libclingcon)
if(CLINGCON_BUILD_APP)
    add_subdirectory(app)
endif()

if(CLINGCON_INSTALL_DEFAULT)
    configure_package_config_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/cmake/clingcon-config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/clingcon-config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/clingcon")
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/clingcon-config-version.cmake"
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion)
    install(
        FILES
            "${CMAKE_CURRENT_BINARY_DIR}/clingcon-config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/clingcon-config-version.cmake"
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/clingcon")
    install(EXPORT clingcon-targets
        FILE clingcon-targets.cmake
        NAMESPACE clingcon::
        DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/clingcon")
endif()
