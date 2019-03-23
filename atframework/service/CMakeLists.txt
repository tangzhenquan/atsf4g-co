# include directory for all service
include_directories(${ATFRAMEWORK_SERVICE_COMPONENT_DIR})

find_package(Threads)
if (CMAKE_USE_PTHREADS_INIT)
    add_definitions(-D_POSIX_MT_)
    list(APPEND COMPILER_OPTION_EXTERN_CXX_LIBS pthread)
endif ()

if (COMPILER_STRICT_EXTRA_CFLAGS)
    add_definitions(${COMPILER_STRICT_EXTRA_CFLAGS})
endif ()

if (COMPILER_STRICT_CFLAGS)
    add_definitions(${COMPILER_STRICT_CFLAGS})
endif ()

# base services
add_subdirectory(${ATFRAMEWORK_SERVICE_COMPONENT_DIR})
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/atproxy")
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/atgateway")