
set (ATSF4G_APP_NAME sample)
EchoWithColor(COLOR GREEN "-- Configure ${ATSF4G_APP_NAME} on ${CMAKE_CURRENT_LIST_DIR}")


find_package(Threads)
if (CMAKE_USE_PTHREADS_INIT)
    add_definitions(-D_POSIX_MT_)
    if(NOT ANDROID)
        list(APPEND COMPILER_OPTION_EXTERN_CXX_LIBS pthread)
    endif()
    if (THREADS_PREFER_PTHREAD_FLAG)
        add_definitions(${THREADS_PREFER_PTHREAD_FLAG})
    endif ()
endif ()


# ============ sample ============

file(GLOB_RECURSE ATSF4G_APP_SRC_LIST
        *.cpp
        *.cc
        *.c
        *.cxx
        *.h
        *.hpp
        )
source_group_by_dir(ATSF4G_APP_SRC_LIST)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_INSTALL_BAS_DIR}/sample/${ATSF4G_APP_NAME}/bin")

if (NOT EXISTS ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
    file(MAKE_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()


add_executable(${ATSF4G_APP_NAME} ${ATSF4G_APP_SRC_LIST} )
target_link_libraries(${ATSF4G_APP_NAME}
        shapp
        ${PROJECT_LIB_LINK}
        ${ATFRAMEWORK_ATBUS_LINK_NAME}
        ${ATFRAMEWORK_ATFRAME_UTILS_LINK_NAME}
        ${3RD_PARTY_LIBUV_LINK_NAME}
        ${COMPILER_OPTION_EXTERN_CXX_LIBS}
        )
