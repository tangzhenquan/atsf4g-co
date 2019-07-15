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

# build libserver_frame.so
file(GLOB_RECURSE SRC_LIST
    RELATIVE ${PROJECT_SERVER_FRAME_SRC_DIR}
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.h
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.hpp
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.c
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.cpp
    ${PROJECT_SERVER_FRAME_SRC_DIR}/*.cc
)

# add generated files
include("${PROJECT_SERVER_FRAME_BAS_DIR}/protocol/generate_proto_source.cmake")
list(REMOVE_DUPLICATES SRC_LIST)

source_group_by_dir(SRC_LIST)
if ( NOT MSVC AND NOT APPLE )
    add_definitions(-shared)
endif()

# these service will use atgateway protocol
include_directories(${ATFRAMEWORK_SERVICE_GATEWAY_PROTOCOL_DIR})

add_library(${PROJECT_SERVER_FRAME_LIB_LINK} STATIC
    ${SRC_LIST}
    ${PROJECT_3RD_PARTY_SRC_LIST}
)

