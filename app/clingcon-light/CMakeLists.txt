cmake_minimum_required(VERSION 3.1)

set(clingo_header_group_name "Clingo Header Files")
set(clingo_header_group
    "${CLINGO_INCLUDE_DIR}/clingo.hh")
source_group("${clingo_header_group_name}" FILES ${clingo_header_group})

set(clingcon_header_group_name "Clingcon Header Files")
set(clingcon_header_group
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
source_group("${clingcon_header_group_name}" FILES ${clingcon_header_group})
set(clingcon_header
    ${clingo_header_group}
    ${clingcon_header_group})

set(clingcon_src_group_name "Clingcon Source Files")
set(clingcon_src_group
    "${CMAKE_CURRENT_SOURCE_DIR}/src/constraint.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/storage.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/domain.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/theoryparser.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/normalizer.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/linearpropagator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/translator.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/clingconpropagator.cpp")

set(clingcon_app_src_group
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
source_group("${clingcon_src_group_name}" FILES ${clingcon_header_group_name} ${clingcon_app_src_group})
set(clingcon_src
    ${clingcon_app_src_group}
    ${clingcon_src_group})


#list(APPEND header "${CLINGO_INCLUDE_DIR}/clingo.hh")

set( CMAKE_VERBOSE_MAKEFILE on )
# Enable folders in IDEs like Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -ggdb3")


add_executable(clingcon ${clingo_header} ${clingcon_header} ${clingcon_src})

find_library(CLINGOLIB
            NAMES libclingo clingo
            HINTS ${CLINGO_LIBRARY_DIR})

target_include_directories(clingcon
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    PRIVATE
    "$<BUILD_INTERFACE:${CLINGO_INCLUDE_DIR}>")

target_link_libraries(clingcon PUBLIC ${CLINGOLIB})
set_target_properties(clingcon PROPERTIES FOLDER exe)

install(TARGETS clingcon RUNTIME DESTINATION bin)


if (CLINGCON_BUILD_TESTS)
    set(test_src_group_name "Test Source Files")
    set(test_src_group
        "${CMAKE_CURRENT_SOURCE_DIR}/test/src/constrainttest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/src/domaintest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/src/linearpropagatortest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/src/translatortest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/src/clingcontest.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/test/src/main.cpp")
    source_group("${test_src_group_name}" FILES ${test_src_group_name})
    set(test_src
        ${test_src_group})
    
    set(test_header_group_name "Test Header Files")
    set(test_header_group
        "${CMAKE_CURRENT_SOURCE_DIR}/test/test/testapp.h")
    source_group("${test_header_group_name}" FILES ${test_header_group})
    set(test_header
        ${test_header_group})

    add_executable(testclingcon ${clingo_header} ${clingcon_header} ${clingcon_src_group} ${test_src} ${test_header})
        
    target_include_directories(testclingcon
        PUBLIC
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
        "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/test>" 
        PRIVATE
        "$<BUILD_INTERFACE:${CLINGO_INCLUDE_DIR}>")
    
    target_link_libraries(testclingcon PUBLIC ${CLINGOLIB})
    set_target_properties(testclingcon PROPERTIES FOLDER exe)
    
    install(TARGETS testclingcon RUNTIME DESTINATION bin)

endif()

