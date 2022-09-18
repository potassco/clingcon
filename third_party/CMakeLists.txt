set(CMAKE_CXX_CLANG_TIDY "")

if (CLINGCON_BUILD_TESTS AND NOT TARGET Catch2::Catch2WithMain)
    if (CLINGCON_USE_LOCAL_CATCH)
        set(CATCH_BUILD_STATIC_LIBRARY ON CACHE INTERNAL "" FORCE)
        add_subdirectory(catch) 
    else()
        find_package(Catch2 3.1 REQUIRED)
    endif()
endif()

add_library(libwide-integer INTERFACE)
target_include_directories(libwide-integer INTERFACE
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/wide-integer>")
