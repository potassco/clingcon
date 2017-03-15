# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/src/clingcon_app.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/clingcon_app.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/version.h")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

list(APPEND header "${CLASP_SOURCE_DIR}/app/clasp_app.h")
list(APPEND source "${CLASP_SOURCE_DIR}/app/clasp_app.cpp")

add_executable(clingcon ${header} ${source})

target_include_directories(clingcon
    PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>"
    PRIVATE
    "$<BUILD_INTERFACE:${CLASP_SOURCE_DIR}/app>"
    "$<BUILD_INTERFACE:${CLINGO_SOURCE_DIR}/libgringo>")

target_link_libraries(clingcon PUBLIC libclingo libgringo libpyclingo libluaclingo libclingcon)
set_target_properties(clingcon PROPERTIES FOLDER exe)

install(TARGETS clingcon RUNTIME DESTINATION bin)

