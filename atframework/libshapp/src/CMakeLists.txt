set (PROJECT_LIB_LINK ${ATFRAMEWORK_LIBSHAPP_LINK_NAME})

EchoWithColor(COLOR GREEN "-- Configure ${PROJECT_LIB_LINK} on ${CMAKE_CURRENT_LIST_DIR}")

file(GLOB PROJECT_LIB_SRC_LIST
        *.cpp
        *.cc
        *.c
        *.cxx
        *.h
        *.hpp
        )


message(${PROJECT_LIB_SRC_LIST})
source_group_by_dir(PROJECT_LIB_SRC_LIST)

# ================ multi thread ================
if ( NOT MSVC )
    add_definitions(-D_POSIX_MT_)
endif()

# ============ libatbus - src ============
add_library(${PROJECT_LIB_LINK} STATIC ${PROJECT_LIB_SRC_LIST})

add_library("${PROJECT_LIB_LINK}_packed" STATIC
        ${PROJECT_LIB_SRC_LIST}
        ${PROJECT_3RD_PARTY_SRC_LIST}
        )

target_link_libraries("${PROJECT_LIB_LINK}_packed"
        ${3RD_PARTY_LIBATBUS_LINK_NAME}
        ${3RD_PARTY_ATFRAME_UTILS_LINK_NAME}
        ${3RD_PARTY_LIBUV_LINK_NAME}
        ${COMPILER_OPTION_EXTERN_CXX_LIBS}
        )

install(TARGETS ${PROJECT_LIB_LINK} "${PROJECT_LIB_LINK}_packed"
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )