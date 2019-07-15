# =========== server_frame =========== 
set (PROJECT_SERVER_FRAME_BAS_DIR ${CMAKE_CURRENT_LIST_DIR})
set (PROJECT_SERVER_FRAME_INC_DIR ${PROJECT_SERVER_FRAME_BAS_DIR})
set (PROJECT_SERVER_FRAME_SRC_DIR ${PROJECT_SERVER_FRAME_BAS_DIR})
set (PROJECT_SERVER_FRAME_LIB_LINK server_frame)


find_package(Libuuid)
if(NOT Libuuid_FOUND)
    EchoWithColor(COLOR YELLOW "-- Dependency: libuuid not found")
    message(FATAL_ERROR "libuuid-devel or uuid-dev is required")
    set(Libuuid_LIBRARIES "")
endif()
