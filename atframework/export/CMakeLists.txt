if (COMPILER_STRICT_EXTRA_CFLAGS)
    add_definitions(${COMPILER_STRICT_EXTRA_CFLAGS})
endif ()

if (COMPILER_STRICT_CFLAGS)
    add_definitions(${COMPILER_STRICT_CFLAGS})
endif ()

# base services
add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/atgw_inner_v1_c")