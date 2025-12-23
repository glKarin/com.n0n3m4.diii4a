cmake_minimum_required(VERSION 3.12)
project(openmohaa_platform)

### Platform-specific code
if(WIN32)
	set(SOURCES_PLATFORM_SPECIFIC
		"${OPENMOHAA_SOURCE_DIR}/code/sys/con_log.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/sys_autoupdater.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/sys_main.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/new/sys_main_new.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/new/sys_win32_new.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/con_win32.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/sys_win32.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/win_resource.rc"
		# These are still used even they're prefixed 'win'
		"${OPENMOHAA_SOURCE_DIR}/code/sys/win_bounds.cpp"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/win_localization.cpp"
	)
elseif(ANDROID) #k
	set(SOURCES_PLATFORM_SPECIFIC
			"${OPENMOHAA_SOURCE_DIR}/code/sys/con_log.c"
			"${OPENMOHAA_SOURCE_DIR}/code/sys/sys_autoupdater.c"
			"${OPENMOHAA_SOURCE_DIR}/code/android/android_main.c"
			"${OPENMOHAA_SOURCE_DIR}/code/android/new/android_main_new.c"
			"${OPENMOHAA_SOURCE_DIR}/code/android/new/android_unix_new.c"
			"${OPENMOHAA_SOURCE_DIR}/code/sys/con_tty.c"
			"${OPENMOHAA_SOURCE_DIR}/code/sys/sys_unix.c"
			# These are still used even they're prefixed 'win'
			"${OPENMOHAA_SOURCE_DIR}/code/sys/win_bounds.cpp"
			"${OPENMOHAA_SOURCE_DIR}/code/sys/win_localization.cpp"
			)
else()
	set(SOURCES_PLATFORM_SPECIFIC
		"${OPENMOHAA_SOURCE_DIR}/code/sys/con_log.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/sys_autoupdater.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/sys_main.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/new/sys_main_new.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/new/sys_unix_new.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/con_tty.c"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/sys_unix.c"
		# These are still used even they're prefixed 'win'
		"${OPENMOHAA_SOURCE_DIR}/code/sys/win_bounds.cpp"
		"${OPENMOHAA_SOURCE_DIR}/code/sys/win_localization.cpp"
	)
endif()

if (APPLE)
	set(SOURCES_PLATFORM_SPECIFIC ${SOURCES_PLATFORM_SPECIFIC} "${OPENMOHAA_SOURCE_DIR}/code/sys/sys_osx.m")
endif()

set(SOURCES_PLATFORM_COMMON
    "${OPENMOHAA_SOURCE_DIR}/code/sys/sys_curl.c"
    "${OPENMOHAA_SOURCE_DIR}/code/sys/sys_update_checker.cpp"
)

add_library(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE)
target_sources(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE ${SOURCES_PLATFORM_SPECIFIC} ${SOURCES_PLATFORM_COMMON})
target_compile_features(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE cxx_nullptr cxx_std_17)
target_compile_features(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE c_variadic_macros)
target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE ${OPENMOHAA_TARGET_PREFIX}qcommon)
target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE curldefs)

if(WIN32)
	target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE wsock32 ws2_32)
	target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE winmm)
	target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE dbghelp)
elseif(UNIX)
	find_package(Threads)
	target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE ${CMAKE_DL_LIBS} ${CMAKE_THREAD_LIBS_INIT})
	# Check for libc
	if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
		execute_process(COMMAND ${CMAKE_C_COMPILER} -v
						OUTPUT_VARIABLE COMPILER_OUTPUT
						ERROR_VARIABLE COMPILER_OUTPUT
						OUTPUT_STRIP_TRAILING_WHITESPACE)

		if(COMPILER_OUTPUT MATCHES "musl|freebsd|openbsd|netbsd")
			target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE execinfo m)
		else()
			target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE m)
			if (NOT APPLE AND NOT ANDROID) #k
				# For when using GLIBC versions older than 2.34
				target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE rt)
			endif()
		endif()
	endif()
	if(NOT ANDROID) #k use NDK's ANDROID_STL
	target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE stdc++)
	endif()
endif()

if (APPLE)
	find_library(COCOA_LIBRARY Cocoa)
	target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE ${COCOA_LIBRARY})
	target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE "-framework Foundation")
	target_link_libraries(${OPENMOHAA_TARGET_PREFIX}syslib INTERFACE objc)
	set_target_properties(${OPENMOHAA_TARGET_PREFIX}syslib PROPERTIES LINK_FLAGS "-Wl,-F/Library/Frameworks")
endif()