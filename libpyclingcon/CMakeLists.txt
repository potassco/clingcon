# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/_clingcon.c")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

if (NOT PYCLINGCON_INSTALL_DIR AND Python_EXECUTABLE)
    if (PYCLINGCON_INSTALL STREQUAL "user")
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGCON_SOURCE_DIR}/cmake/python-site.py target --user OUTPUT_VARIABLE PYCLINGCON_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    elseif(PYCLINGCON_INSTALL STREQUAL "prefix")
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGCON_SOURCE_DIR}/cmake/python-site.py target --prefix "${CMAKE_INSTALL_PREFIX}" OUTPUT_VARIABLE PYCLINGCON_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGCON_SOURCE_DIR}/cmake/python-site.py target OUTPUT_VARIABLE PYCLINGCON_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
endif()

if (NOT PYCLINGCON_SUFFIX)
    if (Python_EXECUTABLE)
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGCON_SOURCE_DIR}/cmake/python-site.py suffix OUTPUT_VARIABLE PYCLINGCON_SUFFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        if (CYGWIN)
            set(PYCLINGCON_SUFFIX ".dll")
        elseif (UNIX)
            set(PYCLINGCON_SUFFIX ".so")
        else()
            set(PYCLINGCON_SUFFIX ".pyd")
        endif()
    endif()
endif()

if (PYCLINGCON_ENABLE STREQUAL "pip")
    add_library(pyclingcon SHARED ${source})
else()
    add_library(pyclingcon MODULE ${source})
endif()
target_link_libraries(pyclingcon PRIVATE libclingcon Python::Module)
target_compile_definitions(pyclingcon PRIVATE _CFFI_NO_LIMITED_API)
set_target_properties(pyclingcon PROPERTIES
    FOLDER lib
    OUTPUT_NAME _clingcon
    SUFFIX ${PYCLINGCON_SUFFIX}
    PREFIX "")
if (CMAKE_RUNTIME_OUTPUT_DIRECTORY)
    set_target_properties(pyclingcon PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/python)
endif()
if (CMAKE_LIBRARY_OUTPUT_DIRECTORY)
    set_target_properties(pyclingcon PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_LIBRARY_OUTPUT_DIRECTORY}/python)
endif()
if (CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
    set_target_properties(pyclingcon PROPERTIES ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_ARCHIVE_OUTPUT_DIRECTORY}/python)
endif()

if (PYCLINGCON_INSTALL_DIR)
    file(TO_CMAKE_PATH "${PYCLINGCON_INSTALL_DIR}" _PYCLINGCON_INSTALL_DIR)
    install(TARGETS pyclingcon
        RUNTIME DESTINATION ${_PYCLINGCON_INSTALL_DIR}/clingcon
        LIBRARY DESTINATION ${_PYCLINGCON_INSTALL_DIR}/clingcon
        ARCHIVE DESTINATION ${_PYCLINGCON_INSTALL_DIR}/clingcon)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/libpyclingcon/clingcon/
        DESTINATION ${_PYCLINGCON_INSTALL_DIR}/clingcon
        FILES_MATCHING
        PATTERN "*.py"
        PATTERN "*.typed"
        PATTERN "tests" EXCLUDE)
    if (NOT PYCLINGCON_ENABLE STREQUAL "pip")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/INSTALLER "pip\n")
        file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/METADATA "Metadata-Version: 1.1\nName: clingcon\nVersion: ${CLINGCON_VERSION}\n")
        install(FILES
            ${CMAKE_CURRENT_BINARY_DIR}/INSTALLER
            ${CMAKE_CURRENT_BINARY_DIR}/METADATA
            DESTINATION ${_PYCLINGCON_INSTALL_DIR}/clingcon-${CLINGCON_VERSION}.dist-info)
    else()
        install(FILES ${CMAKE_SOURCE_DIR}/libclingcon/clingcon.h
            DESTINATION ${_PYCLINGCON_INSTALL_DIR}/clingcon)
    endif()
endif()
