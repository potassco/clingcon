add_library(libwide-integer INTERFACE)
target_include_directories(libwide-integer INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wide-integer>")
