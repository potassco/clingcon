cmake_minimum_required(VERSION 3.1)

# [[[header: .
set(ide_header_group "Header Files")
#set(header-group
#    "${CLINGO_INCLUDE_DIR}/clingo.h"
#    "$${CLINGO_INCLUDE_DIR}/clingo.hh")
#source_group("${ide_header_group}" FILES ${header-group})
set(header-group-clingcon
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/clingconorderpropagator.h"
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
    "${CMAKE_CURRENT_SOURCE_DIR}/clingcon/types.h")
source_group("${ide_header_group}\\clingcon" FILES ${header-group-clingcon})
set(header
    ${header-group}
    ${header-group-clingcon})
# ]]]

# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/src/constraint.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

list(APPEND header "${CLINGO_INCLUDE_DIR}/clingo.hh")

set( CMAKE_VERBOSE_MAKEFILE on )
# Enable folders in IDEs like Visual Studio
set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)


add_executable(clingcon ${header} ${source})

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

