# [[[header: include
set(ide_header_group "Header Files")
set(header-group-include
    "${CMAKE_CURRENT_SOURCE_DIR}/include/clingcon.h")
source_group("${ide_header_group}\\include" FILES ${header-group-include})
set(header
    ${header-group-include})
# ]]]
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/src/clingcon.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]
# [[[test: tests
set(ide_test_group "Test Files")
set(test-group-tests
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/solve.cc")
source_group("${ide_test_group}\\tests" FILES ${test-group-tests})
set(test
    ${test-group-tests})
# ]]]

if(CLINGCON_BUILD_SHARED)
    set(lib_type SHARED)
else()
    set(lib_type STATIC)
endif()

add_library(clingcon ${lib_type})
clingcon_target_properties(TARGET clingcon FOLDER "lib" TYPE default)
target_sources(clingcon PRIVATE ${header} ${source})
target_include_directories(clingcon PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>")
target_link_libraries(clingcon PUBLIC clingo::clingo PRIVATE clingcon-solver)
if(CLINGCON_USE_VISIBILITY)
    target_compile_definitions(clingcon PRIVATE "$<BUILD_INTERFACE:CLINGCON_BUILD_LIBRARY>")
else()
    target_compile_definitions(clingcon PUBLIC CLINGCON_NO_VISIBILITY)
endif()

if(CLINGCON_INSTALL_DEFAULT)
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} FILES_MATCHING PATTERN "*.h")
elseif(CLINGCON_INSTALL_WHEEL AND CLINGCON_BUILD_SHARED)
    install(
        TARGETS clingcon
        RUNTIME DESTINATION "."
        LIBRARY DESTINATION ".")
    if(CLINGCON_BUILD_SHARED)
        if(APPLE)
            set_target_properties(clingcon PROPERTIES
                INSTALL_NAME_DIR "@loader_path")
        elseif(UNIX)
            set_target_properties(clingcon
                PROPERTIES INSTALL_RPATH "$ORIGIN"
                BUILD_WITH_INSTALL_RPATH TRUE)
        endif()
    endif()
endif()

if(CLINGCON_BUILD_TESTS)
    if(WIN32)
        # NOTE: needed to get the test explorer working
        set(test_subdir "${CMAKE_BINARY_DIR}/bin/tests/")
        set(python_subdir "${CMAKE_BINARY_DIR}/bin/python/")
        get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
        if(is_multi_config)
            set(test_subdir "${test_subdir}$<CONFIG>/")
            set(python_subdir "${python_subdir}$<CONFIG>/")
        endif()
        if(CLINGCON_BUILD_SHARED)
            add_custom_command(
                TARGET clingcon POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "${python_subdir}"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:clingcon>" "${python_subdir}"
            )
        endif()
        add_custom_command(
            TARGET clingcon POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "${test_subdir}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${python_subdir}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:clingo::clingo>" "${test_subdir}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:clingo::clingo>" "${python_subdir}"
        )
    endif()

    add_library(clingcon_t STATIC)
    clingcon_target_properties(TARGET clingcon_t FOLDER "lib")
    target_sources(clingcon_t PRIVATE ${header} ${source})
    target_link_libraries(clingcon_t PUBLIC clingcon-solver_t)
    target_include_directories(clingcon_t PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
    target_compile_definitions(clingcon_t PUBLIC CLINGCON_NO_VISIBILITY)

    add_executable(test_clingcon)
    clingcon_target_properties(TARGET test_clingcon FOLDER "test" SUBDIR "tests")
    target_sources(test_clingcon PRIVATE ${test})
    target_link_libraries(test_clingcon PRIVATE clingcon_t Catch2::Catch2WithMain)
    catch_discover_tests(test_clingcon DL_PATH "$<TARGET_FILE_DIR:clingo::clingo>")
endif()
