# [[[header: .
set(ide_header_group "Header Files")
set(header-group
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon.h")
source_group("${ide_header_group}" FILES ${header-group})
set(header-group-clingcon
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/base.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/constraints.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/parsing.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/propagator.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/solver.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/util.hh")
source_group("${ide_header_group}\\clingcon" FILES ${header-group-clingcon})
set(header
    ${header-group}
    ${header-group-clingcon})
# ]]]
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/src/clingcon.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/constraints.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/parsing.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/propagator.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/solver.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

if (PYCLINGCON_ENABLE STREQUAL "pip")
    set(clingcon_build_interface ON)
else()
    set(clingcon_build_interface OFF)
endif()

set(clingcon_public_scope_ PUBLIC)
set(clingcon_private_scope_ PRIVATE)
if (CLINGCON_BUILD_SHARED)
    set(clingcon_lib_type SHARED)
    if (clingcon_build_interface)
        set(clingcon_lib_type INTERFACE)
        set(clingcon_public_scope_ INTERFACE)
        set(clingcon_private_scope_ INTERFACE)
    endif()
else()
    set(clingcon_lib_type STATIC)
endif()

add_library(libclingcon ${clingcon_lib_type})
target_sources(libclingcon ${clingcon_private_scope_} ${header} ${source})
target_include_directories(libclingcon ${clingcon_public_scope_} "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_compile_definitions(libclingcon ${clingcon_private_scope_} CLINGCON_BUILD_LIBRARY)

if (CLINGCON_BUILD_SHARED AND NOT clingcon_build_interface)
    set_target_properties(libclingcon PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_link_libraries(libclingcon ${clingcon_public_scope_} libclingo)
target_link_libraries(libclingcon ${clingcon_private_scope_} libwide-integer)
if (NOT CLINGCON_BUILD_SHARED)
    target_compile_definitions(libclingcon ${clingcon_public_scope_} CLINGCON_NO_VISIBILITY)
elseif(NOT clingcon_build_interface)
    set_target_properties(libclingcon PROPERTIES PUBLIC_HEADER clingcon.h)
    install(TARGETS libclingcon
        EXPORT ClingconTargets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

    install(EXPORT ClingconTargets
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Clingcon)

    include(CMakePackageConfigHelpers)
    configure_package_config_file(
        ${PROJECT_SOURCE_DIR}/cmake/ClingconConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/ClingconConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Clingcon)
    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/ClingconConfigVersion.cmake
        COMPATIBILITY SameMajorVersion)
    install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/ClingconConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/ClingconConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Clingcon)
endif()
if (NOT clingcon_build_interface)
    set_target_properties(libclingcon PROPERTIES
        OUTPUT_NAME clingcon
        LIBRARY_OUTPUT_NAME clingcon
        FOLDER lib)
endif()

if (CLINGCON_PROFILE)
    find_package(Gperftools REQUIRED)
    target_compile_definitions(libclingcon PUBLIC CLINGCON_PROFILE)
    target_link_libraries(libclingcon PUBLIC Gperftools::Profiler)
endif()

if (CLINGCON_BUILD_TESTS)
    # NOTE: we build statically without hiding symbols
    add_library(libclingcon_t STATIC ${header} ${source})
    target_include_directories(libclingcon_t PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
    target_compile_definitions(libclingcon_t PUBLIC CLINGCON_NO_VISIBILITY)
    target_link_libraries(libclingcon_t PUBLIC libwide-integer libclingo)
    set_target_properties(libclingcon_t PROPERTIES
        OUTPUT_NAME clingcon_t
        LIBRARY_OUTPUT_NAME clingcon_t
        FOLDER lib)
    add_subdirectory(tests)
endif()
