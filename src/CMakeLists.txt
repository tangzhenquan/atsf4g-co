include_directories(${ATFRAMEWORK_SERVICE_COMPONENT_DIR})

include("${CMAKE_CURRENT_LIST_DIR}/server_frame/server_frame.cmake")

find_package(Threads)
if (CMAKE_USE_PTHREADS_INIT)
    add_definitions(-D_POSIX_MT_)
    list(APPEND COMPILER_OPTION_EXTERN_CXX_LIBS pthread)
endif ()

add_project_recurse(${CMAKE_CURRENT_LIST_DIR})
