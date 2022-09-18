# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/assumptions.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/csp.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/fs.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/money.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/parsing.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/solve.hh"
    "${CMAKE_CURRENT_SOURCE_DIR}/util.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_executable(test_clingcon ${source})
target_link_libraries(test_clingcon PRIVATE libclingcon_t Catch2::Catch2WithMain)
target_include_directories(test_clingcon PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)
set_target_properties(test_clingcon PROPERTIES FOLDER test)
if(MSVC)
    target_compile_definitions(test_clingcon PRIVATE _SCL_SECURE_NO_WARNINGS)
endif()

add_test(NAME test_clingcon COMMAND test_clingcon)
