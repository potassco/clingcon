add_subdirectory(solver)
add_subdirectory(c-api)
if(CLINGCON_BUILD_PYTHON)
    # TODO: remove this when the c-api is ready
    # add_subdirectory(python-api)
endif()
