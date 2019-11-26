include_directories(${PROJECT_SERVER_FRAME_INC_DIR})

# generated config loader protocol
add_custom_command(OUTPUT 
    "${PROJECT_SERVER_FRAME_SRC_DIR}/config/excel/pb_header_v3.pb.h"
    "${PROJECT_SERVER_FRAME_SRC_DIR}/config/excel/pb_header_v3.pb.cc"
    COMMAND ${PROTOBUF_PROTOC_EXECUTABLE}
    --proto_path "./"
    --cpp_out "./"
    #-o "${PROJECT_INSTALL_RES_PBD_DIR}/config.pb"
    "./pb_header_v3.proto"
    WORKING_DIRECTORY "${PROJECT_SERVER_FRAME_SRC_DIR}/config/excel"
    DEPENDS "${PROJECT_SERVER_FRAME_SRC_DIR}/config/excel/pb_header_v3.proto"
    COMMENT "Generate [@${PROJECT_SERVER_FRAME_SRC_DIR}/config/excel] ./pb_header_v3.proto"
)

find_package(Libuuid)
if(Libuuid_FOUND)
    EchoWithColor(COLOR YELLOW "-- Dependency: libuuid found.${Libuuid_INCLUDE_DIRS}")
    include_directories(${Libuuid_INCLUDE_DIRS})
    set (SERVER_FRAME_ENABLE_LIBUUID 1)
else ()
    unset(Libuuid_LIBRARIES)
endif()

configure_file(
    "${CMAKE_CURRENT_LIST_DIR}/config/server_frame_build_feature.h.in"
    "${CMAKE_CURRENT_LIST_DIR}/config/server_frame_build_feature.h"
    ESCAPE_QUOTES @ONLY
)
list(APPEND SRC_LIST "${CMAKE_CURRENT_LIST_DIR}/config/server_frame_build_feature.h")

# build libserver_frame.so
file(GLOB_RECURSE SRC_LIST_ORIGIN
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.h
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.hpp
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.c
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.cpp
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.cc
)


file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/skip-glob-sources.txt" "")
foreach(SRC_FILE_REALPATH IN LISTS SRC_LIST_ORIGIN)
    get_filename_component(SRC_DIRPATH_REALPATH ${SRC_FILE_REALPATH} DIRECTORY)
    if (SOURCE_FILE MATCHES "^${PROJECT_SERVER_FRAME_SRC_DIR}[/\\\\]config[/\\\\]server_frame_build_feature.h")
        file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/skip-glob-sources.txt" "${SRC_FILE_REALPATH}\r\n")
        continue()
    endif ()
    if (SOURCE_FILE MATCHES "^${PROJECT_SERVER_FRAME_SRC_DIR}[/\\\\]protocol[/\\\\].*")
        file(APPEND "${CMAKE_CURRENT_BINARY_DIR}/skip-glob-sources.txt" "${SRC_FILE_REALPATH}\r\n")
        continue()
    endif ()

    list(APPEND SRC_LIST ${SRC_FILE_REALPATH})
endforeach()
unset(SRC_FILE_REALPATH)
unset(SRC_DIRPATH_REALPATH)

# add generated files
include("${PROJECT_SERVER_FRAME_BAS_DIR}/protocol/generate_proto_source.cmake")
list(REMOVE_DUPLICATES SRC_LIST)

source_group_by_dir(SRC_LIST)

# these service will use atgateway protocol
include_directories(${ATFRAMEWORK_SERVICE_GATEWAY_PROTOCOL_DIR})

add_library(${PROJECT_SERVER_FRAME_LIB_LINK} STATIC
    ${SRC_LIST}
    ${PROJECT_3RD_PARTY_SRC_LIST}
)

