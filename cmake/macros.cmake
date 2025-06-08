function(clingcon_target_properties)
    set(options)
    set(single_values TARGET FOLDER TYPE SUBDIR)
    set(multi_values)
    cmake_parse_arguments(clingcon "${options}" "${single_values}" "${multi_values}" ${ARGV})

    set(binary_subdir "bin")
    set(library_subdir "lib")
    if(clingcon_SUBDIR)
        set(binary_subdir "bin/${clingcon_SUBDIR}")
        set(library_subdir "lib/${clingcon_SUBDIR}")
    endif()

    get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
    if(is_multi_config)
        set(binary_subdir "${binary_subdir}/$<CONFIG>")
        set(library_subdir "${library_subdir}/$<CONFIG>")
    endif()

    if(clingcon_FOLDER)
        set_target_properties(${clingcon_TARGET} PROPERTIES
            FOLDER "${clingcon_FOLDER}"
            POSITION_INDEPENDENT_CODE ON
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${binary_subdir}"
            LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${binary_subdir}"
            ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${library_subdir}"
            PDB_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/${binary_subdir}")
    endif()

    if(clingcon_TYPE STREQUAL "extra" AND CLINGCON_INSTALL_EXTRA)
        install(
            TARGETS "${clingcon_TARGET}"
            EXPORT clingcon-targets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    elseif((clingcon_TYPE STREQUAL "default" OR clingcon_TYPE STREQUAL "binary") AND CLINGCON_INSTALL_DEFAULT)
        install(
            TARGETS "${clingcon_TARGET}"
            EXPORT clingcon-targets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
    endif()
endfunction()
