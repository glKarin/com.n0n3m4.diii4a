cmake_minimum_required(VERSION 3.12)

project(openmohaa_qcommon)

if(${CMAKE_VERSION} VERSION_GREATER "3.12")
	cmake_policy(SET CMP0076 NEW)
endif()

include("./q_version.cmake")

# Shared source files for modules
set(SOURCES_SHARED_UBER
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/class.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/con_set.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/con_timer.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/delegate.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/lightclass.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/listener.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/lz77.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/mem_blockalloc.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/mem_tempalloc.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/script.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/str.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/script/scriptexception.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/script/scriptvariable.cpp"
)

add_library(${OPENMOHAA_TARGET_PREFIX}qcommon_uber INTERFACE)
target_sources(${OPENMOHAA_TARGET_PREFIX}qcommon_uber INTERFACE ${SOURCES_SHARED_UBER})
target_compile_features(${OPENMOHAA_TARGET_PREFIX}qcommon_uber INTERFACE cxx_nullptr)
target_include_directories(${OPENMOHAA_TARGET_PREFIX}qcommon_uber INTERFACE "../qcommon" "../script")

if(DEBUG_MEM_BLOCK)
	target_compile_definitions(${OPENMOHAA_TARGET_PREFIX}qcommon_uber INTERFACE _DEBUG_MEMBLOCK=1)
endif()

set(SOURCES_SHARED
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/q_math.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/q_shared.c"
)

add_library(${OPENMOHAA_TARGET_PREFIX}qcommon_shared INTERFACE)
target_sources(${OPENMOHAA_TARGET_PREFIX}qcommon_shared INTERFACE ${SOURCES_SHARED})
target_compile_features(${OPENMOHAA_TARGET_PREFIX}qcommon_shared INTERFACE c_variadic_macros)
target_include_directories(${OPENMOHAA_TARGET_PREFIX}qcommon_shared INTERFACE "../qcommon")
target_link_libraries(${OPENMOHAA_TARGET_PREFIX}qcommon_shared INTERFACE ${OPENMOHAA_TARGET_PREFIX}qcommon_version)

add_library(${OPENMOHAA_TARGET_PREFIX}qcommon INTERFACE)
target_link_libraries(${OPENMOHAA_TARGET_PREFIX}qcommon INTERFACE ${OPENMOHAA_TARGET_PREFIX}qcommon_shared ${OPENMOHAA_TARGET_PREFIX}qcommon_uber)

# Source files for standalone executable
set(SOURCES_COMMON
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/alias.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/bg_compat.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/cm_fencemask.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/cm_load.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/cm_patch.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/cm_polylib.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/cm_terrain.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/cm_test.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/cm_trace.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/cm_trace_lbd.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/cm_trace_obfuscation.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/cmd.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/common.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/crc.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/cvar.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/files.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/ioapi.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/huffman.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/md4.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/md5.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/memory.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/msg.cpp"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/net_chan.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/net_ip.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/q_math.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/q_shared.c"
	"${OPENMOHAA_SOURCE_DIR}/code/qcommon/unzip.c"
	# Gamespy
	"${OPENMOHAA_SOURCE_DIR}/code/gamespy/q_gamespy.c"
)

add_subdirectory("../skeletor" "./skeletor")
add_subdirectory("../tiki" "./tiki")
add_subdirectory("../curl" "./curl")

add_library(${OPENMOHAA_TARGET_PREFIX}qcommon_standalone INTERFACE)
target_sources(${OPENMOHAA_TARGET_PREFIX}qcommon_standalone INTERFACE ${SOURCES_COMMON})
target_compile_definitions(${OPENMOHAA_TARGET_PREFIX}qcommon_standalone INTERFACE APP_MODULE)

if(DEBUG_DROP_ASSERT)
    target_compile_definitions(${OPENMOHAA_TARGET_PREFIX}qcommon_standalone INTERFACE COM_ERROR_DROP_ASSERT)
endif()

target_compile_features(${OPENMOHAA_TARGET_PREFIX}qcommon_standalone INTERFACE cxx_nullptr)
target_compile_features(${OPENMOHAA_TARGET_PREFIX}qcommon_standalone INTERFACE c_variadic_macros)
target_link_libraries(${OPENMOHAA_TARGET_PREFIX}qcommon_standalone INTERFACE omohtiki omohskeletor)

if(USE_INTERNAL_ZLIB)
    set(SOURCES_ZLIB_DIR "../thirdparty/zlib-1.3.1")
	add_library(${OPENMOHAA_TARGET_PREFIX}zlib INTERFACE)

	target_sources(${OPENMOHAA_TARGET_PREFIX}zlib INTERFACE
		# ${OPENMOHAA_TARGET_PREFIX}zlib
		"${SOURCES_ZLIB_DIR}/adler32.c"
		"${SOURCES_ZLIB_DIR}/crc32.c"
		"${SOURCES_ZLIB_DIR}/inffast.c"
		"${SOURCES_ZLIB_DIR}/inflate.c"
		"${SOURCES_ZLIB_DIR}/inftrees.c"
		"${SOURCES_ZLIB_DIR}/zutil.c"
	)

	target_include_directories(${OPENMOHAA_TARGET_PREFIX}zlib INTERFACE "${SOURCES_ZLIB_DIR}")
	target_link_libraries(${OPENMOHAA_TARGET_PREFIX}qcommon_standalone INTERFACE ${OPENMOHAA_TARGET_PREFIX}zlib)
else()
	find_package(ZLIB REQUIRED)
	target_include_directories(${OPENMOHAA_TARGET_PREFIX}qcommon INTERFACE ${ZLIB_INCLUDE_DIRS})
	target_link_libraries(${OPENMOHAA_TARGET_PREFIX}qcommon_standalone INTERFACE ${ZLIB_LIBRARIES})
endif()

if(NOT ANDROID) #k
#
# Unit tests
#

enable_testing()

add_executable(test_lz77 tests/test_lz77.cpp lz77.cpp q_shared.c common_light.c)
target_link_libraries(test_lz77 INTERFACE testing)
add_test(NAME test_lz77 COMMAND test_lz77)
set_tests_properties(test_lz77 PROPERTIES TIMEOUT 15)
endif()
