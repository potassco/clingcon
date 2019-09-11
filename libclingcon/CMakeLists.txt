# [[[header: .
set(ide_header_group "Header Files")
set(header-group
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon.h")
source_group("${ide_header_group}" FILES ${header-group})
set(header-group-clingcon
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/constraint.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/helper.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/normalizer.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/solver.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/theoryparser.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/variable.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/config.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/domain.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/linearpropagator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/platform.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/storage.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/translator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/clingconpropagator.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/types.h")
source_group("${ide_header_group}\\clingcon" FILES ${header-group-clingcon})
set(header
    ${header-group}
    ${header-group-clingcon})
set(header-group-test
    "${CMAKE_CURRENT_SOURCE_DIR}/test/test/testapp.h")
set(header-test
    ${header-grouptest})
# ]]]
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/src/constraint.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/storage.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/domain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/theoryparser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/normalizer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/linearpropagator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/translator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/clingconpropagator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/clingcon.cpp")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
set(source-group-test
    "${CMAKE_CURRENT_SOURCE_DIR}/test/src/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test/src/constrainttest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test/src/domaintest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test/src/linearpropagatortest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test/src/translatortest.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test/src/clingcontest.cpp")
set(source-test
    ${source-group-test})
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
    add_executable(testclingcon ${source} ${header} ${source-test}
    ${header-test})
    target_include_directories(testclingcon PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>")
    target_include_directories(testclingcon PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test>")
    target_link_libraries(testclingcon PRIVATE libclingo)
    set_target_properties(testclingcon PROPERTIES
        OUTPUT_NAME testclingcon
        FOLDER exe)
    install(TARGETS testclingcon RUNTIME DESTINATION bin)
endif()
