# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/_clingcon.c")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

if (NOT PYCLINGCON_INSTALL_DIR AND Python_EXECUTABLE)
    if (PYCLINGCON_USER_INSTALL)
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGCON_SOURCE_DIR}/cmake/python-site.py site OUTPUT_VARIABLE PYCLINGCON_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    elseif(PYCLINGCON_USE_INSTALL_PREFIX)
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGCON_SOURCE_DIR}/cmake/python-site.py prefix "${CMAKE_INSTALL_PREFIX}" OUTPUT_VARIABLE PYCLINGCON_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGCON_SOURCE_DIR}/cmake/python-site.py prefix OUTPUT_VARIABLE PYCLINGCON_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
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

if (CLINGCON_BUILD_SHARED_INTERFACE)
    add_library(pyclingcon SHARED ${source})
else()
    add_library(pyclingcon MODULE ${source})
endif()
target_link_libraries(pyclingcon PRIVATE libclingcon Python::Module)
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
    install(FILES ${CMAKE_SOURCE_DIR}/libclingcon/clingcon.h
        DESTINATION ${_PYCLINGCON_INSTALL_DIR}/clingcon)
endif()
