
# =========== 3rdparty protobuf ==================
set (3RD_PARTY_PROTOBUF_BASE_DIR "${CMAKE_CURRENT_LIST_DIR}")
set (3RD_PARTY_PROTOBUF_PKG_DIR "${CMAKE_CURRENT_LIST_DIR}/pkg")

set (3RD_PARTY_PROTOBUF_VERSION "3.8.0")

if( ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
    if ( CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.7.0")
        set (3RD_PARTY_PROTOBUF_VERSION "3.5.2")
    endif()
elseif( ${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang")
    if ( CMAKE_CXX_COMPILER_VERSION VERSION_LESS "3.3")
        set (3RD_PARTY_PROTOBUF_VERSION "3.5.2")
    endif()
elseif( ${CMAKE_CXX_COMPILER_ID} STREQUAL "AppleClang")
    if ( CMAKE_CXX_COMPILER_VERSION VERSION_LESS "5.0")
        set (3RD_PARTY_PROTOBUF_VERSION "3.5.2")
    endif()
endif()

if(PROTOBUF_ROOT)
    set (3RD_PARTY_PROTOBUF_ROOT_DIR "${PROTOBUF_ROOT}")
else()
    set (3RD_PARTY_PROTOBUF_ROOT_DIR "${CMAKE_CURRENT_LIST_DIR}/prebuilt/${PLATFORM_BUILD_PLATFORM_NAME}")
endif()

include(GNUInstallDirs)
list(APPEND CMAKE_INCLUDE_PATH "${3RD_PARTY_PROTOBUF_ROOT_DIR}/include")
if (CMAKE_ANDROID_ARCH_ABI)
    list(APPEND CMAKE_LIBRARY_PATH "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib/${CMAKE_ANDROID_ARCH_ABI}" "${3RD_PARTY_PROTOBUF_ROOT_DIR}/${CMAKE_INSTALL_BINDIR}")
else ()
    list(APPEND CMAKE_LIBRARY_PATH "${3RD_PARTY_PROTOBUF_ROOT_DIR}/${CMAKE_INSTALL_LIBDIR}" "${3RD_PARTY_PROTOBUF_ROOT_DIR}/${CMAKE_INSTALL_BINDIR}")
endif()
if(PROTOBUF_HOST_BIN)
    list(APPEND CMAKE_PROGRAM_PATH ${PROTOBUF_HOST_BIN})
else()
    list(APPEND CMAKE_PROGRAM_PATH "${3RD_PARTY_PROTOBUF_ROOT_DIR}/${CMAKE_INSTALL_BINDIR}")
endif()

project_build_cmake_options(3RD_PARTY_PROTOBUF_FLAG_OPTIONS)

if (NOT EXISTS ${3RD_PARTY_PROTOBUF_PKG_DIR})
    file(MAKE_DIRECTORY ${3RD_PARTY_PROTOBUF_PKG_DIR})
endif()

# MSVC 必须用静态库，而且会被用/MT编译。我们要把默认的/MD改为/MT
# 使用 /MD protobuf容易跨堆管理数据，容易崩溃，/MT依赖较少不容易出问题
# 注意protobuf的 RelWithDebInfo 默认使用 /MT 而本工程默认是 /MTd
# if (MSVC)
#     set (3RD_PARTY_PROTOBUF_BUILD_SHARED_LIBS -DBUILD_SHARED_LIBS=OFF)
#     # add_compiler_define(PROTOBUF_USE_DLLS) # MSVC 使用动态库必须加这个选项
#     foreach(flag_var
#         CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
#         CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
#         if(${flag_var} MATCHES "/MD")
#             string(REGEX REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
#         endif(${flag_var} MATCHES "/MD")
#     endforeach(flag_var)
# #else ()
#     # 其他情况使用默认值即可
#     # set (3RD_PARTY_PROTOBUF_BUILD_SHARED_LIBS OFF)
# endif ()

set (Protobuf_ROOT ${3RD_PARTY_PROTOBUF_ROOT_DIR})
list(APPEND CMAKE_PREFIX_PATH ${3RD_PARTY_PROTOBUF_ROOT_DIR})
list(APPEND CMAKE_FIND_ROOT_PATH ${3RD_PARTY_PROTOBUF_ROOT_DIR})

if (CMAKE_VERSION VERSION_LESS "3.14" AND EXISTS "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib64" AND NOT EXISTS "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib")
    if (CMAKE_HOST_WIN32)
        execute_process(
            COMMAND mklink /D "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib" "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib64"
            WORKING_DIRECTORY ${3RD_PARTY_PROTOBUF_ROOT_DIR}
        )
    else ()
        execute_process(
            COMMAND ln -s "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib64" "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib"
            WORKING_DIRECTORY ${3RD_PARTY_PROTOBUF_ROOT_DIR}
        )
    endif ()
endif()

set(Protobuf_USE_STATIC_LIBS ON)
find_library(3RD_PARTY_PROTOBUF_FIND_LIB NAMES protobuf libprotobuf protobufd libprotobufd
    PATHS "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib" "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib64" NO_DEFAULT_PATH)
if (NOT 3RD_PARTY_PROTOBUF_FIND_LIB)
    if (NOT EXISTS "${3RD_PARTY_PROTOBUF_PKG_DIR}/protobuf-${3RD_PARTY_PROTOBUF_VERSION}")
        if (NOT EXISTS "${3RD_PARTY_PROTOBUF_PKG_DIR}/protobuf-cpp-${3RD_PARTY_PROTOBUF_VERSION}.tar.gz")
            FindConfigurePackageDownloadFile("https://github.com/google/protobuf/releases/download/v${3RD_PARTY_PROTOBUF_VERSION}/protobuf-cpp-${3RD_PARTY_PROTOBUF_VERSION}.tar.gz" "${3RD_PARTY_PROTOBUF_PKG_DIR}/protobuf-cpp-${3RD_PARTY_PROTOBUF_VERSION}.tar.gz")
        endif ()

        execute_process(COMMAND ${CMAKE_COMMAND} -E tar xvf "${3RD_PARTY_PROTOBUF_PKG_DIR}/protobuf-cpp-${3RD_PARTY_PROTOBUF_VERSION}.tar.gz"
            WORKING_DIRECTORY ${3RD_PARTY_PROTOBUF_PKG_DIR}
        )
    endif ()

    if (NOT EXISTS "${3RD_PARTY_PROTOBUF_PKG_DIR}/protobuf-${3RD_PARTY_PROTOBUF_VERSION}")
        EchoWithColor(COLOR RED "-- Dependency: Build protobuf failed")
        message(FATAL_ERROR "Dependency: Protobuf is required")
    endif ()

    unset(3RD_PARTY_PROTOBUF_BUILD_FLAGS)
    list(APPEND 3RD_PARTY_PROTOBUF_BUILD_FLAGS 
        ${CMAKE_COMMAND} "${3RD_PARTY_PROTOBUF_PKG_DIR}/protobuf-${3RD_PARTY_PROTOBUF_VERSION}/cmake"
        "-DCMAKE_INSTALL_PREFIX=${3RD_PARTY_PROTOBUF_ROOT_DIR}"
        ${3RD_PARTY_PROTOBUF_FLAG_OPTIONS} 
        "-Dprotobuf_BUILD_TESTS=OFF" "-Dprotobuf_BUILD_EXAMPLES=OFF" "-DCMAKE_CXX_STANDARD=${CMAKE_CXX_STANDARD}"
        "-Dprotobuf_BUILD_CONFORMANCE=OFF" "-Dprotobuf_BUILD_PROTOC_BINARIES=OFF"
        "-DBUILD_SHARED_LIBS=OFF" "-Dprotobuf_BUILD_SHARED_LIBS=OFF" "-Dprotobuf_MSVC_STATIC_RUNTIME=OFF"
        ${3RD_PARTY_PROTOBUF_BUILD_SHARED_LIBS}
    )

    set (3RD_PARTY_PROTOBUF_BUILD_DIR "${3RD_PARTY_PROTOBUF_PKG_DIR}/protobuf-${3RD_PARTY_PROTOBUF_VERSION}/build-obj-dir")
    if (NOT EXISTS ${3RD_PARTY_PROTOBUF_BUILD_DIR})
        file(MAKE_DIRECTORY ${3RD_PARTY_PROTOBUF_BUILD_DIR})
    endif()
    string(REPLACE ";" " " 3RD_PARTY_PROTOBUF_BUILD_FLAGS_CMD "${3RD_PARTY_PROTOBUF_BUILD_FLAGS}")
    message(STATUS "@${3RD_PARTY_PROTOBUF_BUILD_DIR} Run: ${3RD_PARTY_PROTOBUF_BUILD_FLAGS_CMD}")

    # build
    execute_process(
        COMMAND ${3RD_PARTY_PROTOBUF_BUILD_FLAGS}
        WORKING_DIRECTORY ${3RD_PARTY_PROTOBUF_BUILD_DIR}
    )

    if (MSVC)
        set(3RD_PARTY_PROTOBUF_BUILD_FLAG "--" "/m")
    else ()
        include(ProcessorCount)
        ProcessorCount(CPU_CORE_NUM)
        set(3RD_PARTY_PROTOBUF_BUILD_FLAG "--" "-j${CPU_CORE_NUM}")
    endif ()

    # install
    if (MSVC)
        execute_process(
            COMMAND ${CMAKE_COMMAND} --build . --target install --config Debug ${3RD_PARTY_PROTOBUF_BUILD_FLAG}
            WORKING_DIRECTORY ${3RD_PARTY_PROTOBUF_BUILD_DIR}
        )
    endif ()
    execute_process(
        COMMAND ${CMAKE_COMMAND} --build . --target install --config Release ${3RD_PARTY_PROTOBUF_BUILD_FLAG}
        WORKING_DIRECTORY ${3RD_PARTY_PROTOBUF_BUILD_DIR}
    )
endif ()

find_package(Protobuf)

# try again, cached vars will cause find failed.
if (NOT PROTOBUF_FOUND OR NOT PROTOBUF_PROTOC_EXECUTABLE OR NOT Protobuf_INCLUDE_DIRS OR NOT Protobuf_LIBRARY)
    if (CMAKE_VERSION VERSION_LESS "3.14" AND EXISTS "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib64" AND NOT EXISTS "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib")
        if (CMAKE_HOST_WIN32)
            execute_process(
                COMMAND mklink /D "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib" "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib64"
                WORKING_DIRECTORY ${3RD_PARTY_PROTOBUF_ROOT_DIR}
            )
        else ()
            execute_process(
                COMMAND ln -s "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib64" "${3RD_PARTY_PROTOBUF_ROOT_DIR}/lib"
                WORKING_DIRECTORY ${3RD_PARTY_PROTOBUF_ROOT_DIR}
            )
        endif ()
    endif()
    EchoWithColor(COLOR YELLOW "-- Dependency: Try to find protobuf libraries again")
    unset(Protobuf_LIBRARY)
    unset(Protobuf_PROTOC_LIBRARY)
    unset(Protobuf_INCLUDE_DIR)
    unset(Protobuf_PROTOC_EXECUTABLE)
    unset(Protobuf_LIBRARY_DEBUG)
    unset(Protobuf_PROTOC_LIBRARY_DEBUG)
    unset(Protobuf_LITE_LIBRARY)
    unset(Protobuf_LITE_LIBRARY_DEBUG)
    unset(Protobuf_LIBRARIES)
    unset(Protobuf_PROTOC_LIBRARIES)
    unset(Protobuf_LITE_LIBRARIES)
    unset(Protobuf::protoc)
    find_package(Protobuf)
endif()

if(PROTOBUF_FOUND AND Protobuf_LIBRARY)
    EchoWithColor(COLOR GREEN "-- Dependency: Protobuf found.(${PROTOBUF_PROTOC_EXECUTABLE})")
    EchoWithColor(COLOR GREEN "-- Dependency: Protobuf include.(${Protobuf_INCLUDE_DIRS})")
else()
    EchoWithColor(COLOR RED "-- Dependency: Protobuf is required")
    message(FATAL_ERROR "Protobuf not found")
endif()

if (UNIX)
    execute_process(COMMAND chmod +x "${PROTOBUF_PROTOC_EXECUTABLE}")
endif()

set (3RD_PARTY_PROTOBUF_INC_DIR ${PROTOBUF_INCLUDE_DIRS})
if (${CMAKE_BUILD_TYPE} STREQUAL "Debug" AND Protobuf_LIBRARY_DEBUG)
    get_filename_component(3RD_PARTY_PROTOBUF_LIB_DIR ${Protobuf_LIBRARY_DEBUG} DIRECTORY)
    get_filename_component(3RD_PARTY_PROTOBUF_BIN_DIR ${PROTOBUF_PROTOC_EXECUTABLE} DIRECTORY)
    set (3RD_PARTY_PROTOBUF_LINK_NAME ${Protobuf_LIBRARY_DEBUG})
    set (3RD_PARTY_PROTOBUF_LITE_LINK_NAME ${Protobuf_LITE_LIBRARY_DEBUG})
    EchoWithColor(COLOR GREEN "-- Dependency: Protobuf libraries.(${Protobuf_LIBRARY_DEBUG})")
    EchoWithColor(COLOR GREEN "-- Dependency: Protobuf lite libraries.(${Protobuf_LITE_LIBRARY_DEBUG})")
else()
    get_filename_component(3RD_PARTY_PROTOBUF_LIB_DIR ${Protobuf_LIBRARY} DIRECTORY)
    get_filename_component(3RD_PARTY_PROTOBUF_BIN_DIR ${PROTOBUF_PROTOC_EXECUTABLE} DIRECTORY)
    if (Protobuf_LIBRARY_RELEASE)
        set (3RD_PARTY_PROTOBUF_LINK_NAME ${Protobuf_LIBRARY_RELEASE})
    else ()
        set (3RD_PARTY_PROTOBUF_LINK_NAME ${Protobuf_LIBRARY})
    endif ()
    if (Protobuf_LITE_LIBRARY_RELEASE)
        set (3RD_PARTY_PROTOBUF_LITE_LINK_NAME ${Protobuf_LITE_LIBRARY_RELEASE})
    else ()
        set (3RD_PARTY_PROTOBUF_LITE_LINK_NAME ${Protobuf_LIBRARY})
    endif ()
    EchoWithColor(COLOR GREEN "-- Dependency: Protobuf libraries.(${Protobuf_LIBRARY})")
    EchoWithColor(COLOR GREEN "-- Dependency: Protobuf lite libraries.(${Protobuf_LITE_LIBRARY})")
endif()

set (3RD_PARTY_PROTOBUF_BIN_PROTOC ${PROTOBUF_PROTOC_EXECUTABLE})

include_directories(${3RD_PARTY_PROTOBUF_INC_DIR})

# file(GLOB 3RD_PARTY_PROTOBUF_ALL_LIB_FILES 
#     "${3RD_PARTY_PROTOBUF_LIB_DIR}/libprotobuf*.so"
#     "${3RD_PARTY_PROTOBUF_LIB_DIR}/libprotobuf*.so.*"
#     "${3RD_PARTY_PROTOBUF_LIB_DIR}/libprotobuf*.dll"
#     "${3RD_PARTY_PROTOBUF_BIN_DIR}/libprotobuf*.so"
#     "${3RD_PARTY_PROTOBUF_BIN_DIR}/libprotobuf*.so.*"
#     "${3RD_PARTY_PROTOBUF_BIN_DIR}/libprotobuf*.dll"
# )
# project_copy_shared_lib(${3RD_PARTY_PROTOBUF_ALL_LIB_FILES} ${PROJECT_INSTALL_SHARED_DIR})
