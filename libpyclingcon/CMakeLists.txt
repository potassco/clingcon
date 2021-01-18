# [[[source: .
set(ide_source_group "Source Files")
set(source-group
    "${CMAKE_CURRENT_SOURCE_DIR}/_clingcon.c")
source_group("${ide_source_group}" FILES ${source-group})
set(source
    ${source-group})
# ]]]

if(FALSE)
if (NOT PYCLINGO_INSTALL_DIR AND Python_EXECUTABLE)
    if (PYCLINGO_USER_INSTALL)
        execute_process(COMMAND ${Python_EXECUTABLE} -m site --user-site OUTPUT_VARIABLE PYCLINGO_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    elseif(PYCLINGO_USE_INSTALL_PREFIX)
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGO_SOURCE_DIR}/cmake/python-site.py prefix "${CMAKE_INSTALL_PREFIX}" OUTPUT_VARIABLE PYCLINGO_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGO_SOURCE_DIR}/cmake/python-site.py prefix OUTPUT_VARIABLE PYCLINGO_INSTALL_DIR OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
endif()

if (NOT PYCLINGO_SUFFIX)
    if (Python_EXECUTABLE)
        execute_process(COMMAND ${Python_EXECUTABLE} ${CLINGO_SOURCE_DIR}/cmake/python-site.py suffix OUTPUT_VARIABLE PYCLINGO_SUFFIX OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        if (CYGWIN)
            set(PYCLINGO_SUFFIX ".dll")
        elseif (UNIX)
            set(PYCLINGO_SUFFIX ".so")
        else()
            set(PYCLINGO_SUFFIX ".pyd")
        endif()
    endif()
endif()
else()
message(STATUS "TODO: implement suffix and prefix detection")
set(PYCLINGO_SUFFIX ".so")
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
    SUFFIX ${PYCLINGO_SUFFIX}
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

if(FALSE)
if (PYCLINGO_INSTALL_DIR)
    file(TO_CMAKE_PATH "${PYCLINGO_INSTALL_DIR}" _PYCLINGO_INSTALL_DIR)
    install(TARGETS pyclingcon
        RUNTIME DESTINATION ${_PYCLINGO_INSTALL_DIR}/clingo
        LIBRARY DESTINATION ${_PYCLINGO_INSTALL_DIR}/clingo
        ARCHIVE DESTINATION ${_PYCLINGO_INSTALL_DIR}/clingo)
    install(DIRECTORY ${CMAKE_SOURCE_DIR}/libpyclingo_cffi/clingo/
        DESTINATION ${_PYCLINGO_INSTALL_DIR}/clingo
        FILES_MATCHING
        PATTERN "*.py"
        PATTERN "*.typed"
        PATTERN "tests" EXCLUDE)
    install(FILES ${CMAKE_SOURCE_DIR}/libclingo/clingo.h
        DESTINATION ${_PYCLINGO_INSTALL_DIR}/clingo)
endif()
else()
    message(STATUS "TODO: install")
endif()
