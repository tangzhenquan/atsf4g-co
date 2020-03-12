set (PROJECT_LIB_LINK ${G3LOG_LINK_NAME})

EchoWithColor(COLOR GREEN "-- Configure ${PROJECT_LIB_LINK} on ${CMAKE_CURRENT_LIST_DIR}")

file(GLOB_RECURSE HEADER_FILES *.h *.inl)
set(SOURCE_FILES filesink.cpp
	g3log.cpp
	logcapture.cpp
	loglevels.cpp
	logmessage.cpp
	logworker.cpp
	time.cpp)

if ( MSVC )
  add_definitions("-D_CRT_SECURE_NO_WARNINGS")
  list(APPEND SOURCE_FILES crashhandler_windows.cpp)
  list(APPEND SOURCE_FILES stacktrace_windows.cpp)
else()
  list(APPEND SOURCE_FILES crashhandler_unix.cpp)
endif()


add_library(${PROJECT_LIB_LINK} STATIC ${HEADER_FILES} ${SOURCE_FILES})
set_target_properties(${PROJECT_LIB_LINK} PROPERTIES PREFIX "")
set_target_properties(${PROJECT_LIB_LINK} PROPERTIES SUFFIX "${LIB_STATIC_SUFFIX}")
set_target_properties(${PROJECT_LIB_LINK} PROPERTIES DEBUG_OUTPUT_NAME "${PROJECT_LIB_LINK}_d.lib")
set_target_properties(${PROJECT_LIB_LINK} PROPERTIES RELEASE_OUTPUT_NAME "${PROJECT_LIB_LINK}.lib")
set_target_properties(${PROJECT_LIB_LINK} PROPERTIES RELWITHDEBINFO_OUTPUT_NAME "${PROJECT_LIB_LINK}.lib")
install(TARGETS ${PROJECT_LIB_LINK}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        )
