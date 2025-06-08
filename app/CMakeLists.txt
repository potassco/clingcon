# [[[source: src
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/src/main.cc")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

add_executable(clingcon-app)
target_sources(clingcon-app PRIVATE ${source})
clingcon_target_properties(TARGET clingcon-app FOLDER "exe" TYPE default)
target_link_libraries(clingcon-app PRIVATE clingcon)
set_target_properties(clingcon-app PROPERTIES OUTPUT_NAME clingcon)
