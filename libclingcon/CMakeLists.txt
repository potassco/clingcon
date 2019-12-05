# [[[header: .
set(ide_header_group "Header Files")
set(header-group
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon.h")
source_group("${ide_header_group}" FILES ${header-group})
set(header
    ${header-group})
# ]]]
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/src/propagator.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

if (CLINGCON_BUILD_SHARED)
    set(clingcon_lib_type SHARED)
else()
    set(clingcon_lib_type STATIC)
endif()

add_library(libclingcon ${clingcon_lib_type} ${header} ${source})
target_include_directories(libclingcon PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
target_compile_definitions(libclingcon PRIVATE CLINGCON_BUILD_LIBRARY)
if (NOT CLINGCON_BUILD_STATIC AND CLINGCON_BUILD_SHARED)
    set_target_properties(libclingcon PROPERTIES POSITION_INDEPENDENT_CODE ON)
endif()
target_link_libraries(libclingcon PRIVATE libclingo)
if (NOT CLINGCON_BUILD_SHARED)
    target_compile_definitions(libclingcon PUBLIC CLINGCON_NO_VISIBILITY)
else()
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
set_target_properties(libclingcon PROPERTIES
    OUTPUT_NAME clingcon
    LIBRARY_OUTPUT_NAME clingcon
    FOLDER lib)

if (CLINGCON_BUILD_TESTS)
    add_subdirectory(tests)
endif()
