# [[[header: include
set(ide_header_group "Header Files")
set(header-group-include-clingcon
    "${CMAKE_CURRENT_SOURCE_DIR}/include/clingcon/base.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/clingcon/constraints.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/clingcon/parsing.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/clingcon/propagator.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/clingcon/solver.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/include/clingcon/util.hh")
source_group("${ide_header_group}\\include\\clingcon" FILES ${header-group-include-clingcon})
set(header
    ${header-group-include-clingcon})
# ]]]
# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/src/constraints.cc"
    # "${CMAKE_CURRENT_SOURCE_DIR}/src/parsing.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/propagator.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/solver.cc"
    )
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]
# [[[test: tests
set(ide_test_group "Test Files")
set(test-group-tests
    #"${CMAKE_CURRENT_SOURCE_DIR}/tests/assumptions.cc"
    #"${CMAKE_CURRENT_SOURCE_DIR}/tests/csp.cc"
    #"${CMAKE_CURRENT_SOURCE_DIR}/tests/fs.cc"
    #"${CMAKE_CURRENT_SOURCE_DIR}/tests/money.cc"
    #"${CMAKE_CURRENT_SOURCE_DIR}/tests/parsing.cc"
    #"${CMAKE_CURRENT_SOURCE_DIR}/tests/solve.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/tests/util.cc")
source_group("${ide_test_group}\\tests" FILES ${test-group-tests})
set(test
    ${test-group-tests})
# ]]]

add_library(clingcon-solver STATIC)
clingcon_target_properties(TARGET clingcon-solver FOLDER "lib" TYPE extra)
target_sources(clingcon-solver PRIVATE ${header} ${source})
target_include_directories(clingcon-solver PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
target_link_libraries(clingcon-solver PUBLIC clingo::clingo libwide-integer)
if(CLINGCON_PROFILE)
    target_compile_definitions(clingcon-solver PUBLIC CLINGCON_PROFILE)
    target_link_libraries(clingcon-solver PUBLIC Gperftools::Profiler)
endif()

if(CLINGCON_BUILD_TESTS)
    add_library(clingcon-solver_t STATIC)
    clingcon_target_properties(TARGET clingcon-solver_t FOLDER "lib")
    target_sources(clingcon-solver_t PRIVATE ${header} ${source})
    target_link_libraries(clingcon-solver_t PUBLIC clingo::clingo libwide-integer)
    target_include_directories(clingcon-solver_t PUBLIC "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>")
    target_compile_definitions(clingcon-solver_t PUBLIC CLINGCON_CROSSCHECK)

    add_executable(test_clingcon-solver)
    clingcon_target_properties(TARGET test_clingcon-solver FOLDER "test" SUBDIR "tests")
    target_sources(test_clingcon-solver PRIVATE ${test})
    target_link_libraries(test_clingcon-solver PRIVATE clingcon-solver_t Catch2::Catch2WithMain)
    catch_discover_tests(test_clingcon-solver DL_PATH "$<TARGET_FILE_DIR:clingo::clingo>")
endif()
