set(PROJECT_LIB_LINK "atproxy_cli")
EchoWithColor(COLOR GREEN "-- Configure ${CMAKE_CURRENT_LIST_DIR}")

# ============ c binding ============

file(GLOB BINDING_SRC_LIST RELATIVE "${CMAKE_CURRENT_LIST_DIR}"
    ${CMAKE_CURRENT_LIST_DIR}/*.cpp
    ${CMAKE_CURRENT_LIST_DIR}/*.cc
    ${CMAKE_CURRENT_LIST_DIR}/*.c
    ${CMAKE_CURRENT_LIST_DIR}/*.cxx
)
include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories(${ATFRAMEWORK_SERVICE_ATPROXY_PROTOCOL_DIR})

# ============ libatbus - src ============
# 保持和MSVC得导出行为一致
if( ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    add_definitions(-fvisibility=hidden)
elseif( ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    add_definitions(-fvisibility=hidden)
endif()




add_library(${PROJECT_LIB_LINK} STATIC ${BINDING_SRC_LIST} )




target_link_libraries(${PROJECT_LIB_LINK}
        shapp
        ${ATFRAMEWORK_SERVICE_COMPONENT_LINK_NAME}
        ${ATFRAMEWORK_ATAPP_LINK_NAME}
        ${ATFRAMEWORK_ATBUS_LINK_NAME}
        ${ATFRAMEWORK_ATFRAME_UTILS_LINK_NAME}
        ${3RD_PARTY_LIBCURL_LINK_NAME} ${3RD_PARTY_LIBCURL_STATIC_LINK_NAMES}
        ${3RD_PARTY_LIBUV_LINK_NAME}
        ${COMPILER_OPTION_EXTERN_CXX_LIBS}
		${G3LOG_LINK_NAME}
)

install(TARGETS ${PROJECT_LIB_LINK}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

#install(FILES ${CMAKE_CURRENT_LIST_DIR}/atframe/libatapp_c.h
#    DESTINATION include/atframe
#)

#install(FILES "${3RD_PARTY_ATFRAME_UTILS_INC_DIR}/config/compile_optimize.h"
#    DESTINATION include/atframe/config
#)
