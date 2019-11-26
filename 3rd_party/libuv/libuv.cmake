# =========== 3rdparty libuv ==================
if (NOT 3RD_PARTY_LIBUV_BASE_DIR)
    set (3RD_PARTY_LIBUV_BASE_DIR ${CMAKE_CURRENT_LIST_DIR})
endif()

set (3RD_PARTY_LIBUV_PKG_DIR "${3RD_PARTY_LIBUV_BASE_DIR}/pkg")

set (3RD_PARTY_LIBUV_DEFAULT_VERSION "1.33.1")
set (3RD_PARTY_LIBUV_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/prebuilt/${PLATFORM_BUILD_PLATFORM_NAME}")

if(NOT EXISTS ${3RD_PARTY_LIBUV_PKG_DIR})
    file(MAKE_DIRECTORY ${3RD_PARTY_LIBUV_PKG_DIR})
endif()

# force to use prebuilt when using mingw
# if (MINGW)
#     set(LIBUV_ROOT ${3RD_PARTY_LIBUV_ROOT_DIR})
# endif()

FindConfigurePackage(
    PACKAGE Libuv
    BUILD_WITH_CMAKE
    CMAKE_FLAGS "-DCMAKE_POSITION_INDEPENDENT_CODE=YES" "-DBUILD_SHARED_LIBS=OFF" "-DBUILD_TESTING=OFF"
    MAKE_FLAGS "-j8"
    WORKING_DIRECTORY "${3RD_PARTY_LIBUV_PKG_DIR}"
    BUILD_DIRECTORY "${3RD_PARTY_LIBUV_PKG_DIR}/libuv-v${3RD_PARTY_LIBUV_DEFAULT_VERSION}/build_jobs_${PLATFORM_BUILD_PLATFORM_NAME}"
    PREFIX_DIRECTORY "${3RD_PARTY_LIBUV_ROOT_DIR}"
    SRC_DIRECTORY_NAME "libuv-v${3RD_PARTY_LIBUV_DEFAULT_VERSION}"
    TAR_URL "http://dist.libuv.org/dist/v${3RD_PARTY_LIBUV_DEFAULT_VERSION}/libuv-v${3RD_PARTY_LIBUV_DEFAULT_VERSION}.tar.gz"
)

if(Libuv_FOUND)
    EchoWithColor(COLOR GREEN "-- Dependency: Libuv found.(${Libuv_LIBRARIES})")
else()
    EchoWithColor(COLOR RED "-- Dependency: Libuv is required")
    message(FATAL_ERROR "Libuv not found")
endif()

if (NOT Libuv_FOUND)
    EchoWithColor(COLOR RED "-- Dependency: Libuv is required, we can not find prebuilt for libuv and can not find git to clone the sources")
    message(FATAL_ERROR "Libuv not found")
endif()

set (3RD_PARTY_LIBUV_INC_DIR ${Libuv_INCLUDE_DIRS})
set (3RD_PARTY_LIBUV_LINK_NAME ${Libuv_LIBRARIES})

include_directories(${3RD_PARTY_LIBUV_INC_DIR})

# mingw
if (MINGW)
    EchoWithColor(COLOR GREEN "-- MinGW: custom add lib ws2_32,psapi,userenv,iphlpapi ")
    list(APPEND 3RD_PARTY_LIBUV_LINK_NAME ws2_32 psapi userenv iphlpapi)
elseif (WIN32)
    EchoWithColor(COLOR GREEN "-- Win32: custom add lib psapi,userenv,iphlpapi ")
    list(APPEND 3RD_PARTY_LIBUV_LINK_NAME psapi userenv iphlpapi)
endif()

