cmake_minimum_required(VERSION 3.12)

project(omohserver)

add_subdirectory("../fgame" "./fgame")

file(GLOB_RECURSE SOURCES_SERVER "./*.c*")

set(SOURCES_SERVER ${SOURCES_SERVER}
	# Gamespy
	"${OPENMOHAA_SOURCE_DIR}/code/gamespy/sv_gamespy.c"
	"${OPENMOHAA_SOURCE_DIR}/code/gamespy/sv_gqueryreporting.c"
)

add_library(omohserver INTERFACE)
target_sources(omohserver INTERFACE ${SOURCES_SERVER})
target_compile_features(omohserver INTERFACE cxx_nullptr)
target_compile_features(omohserver INTERFACE c_variadic_macros)
target_link_libraries(omohserver INTERFACE ${OPENMOHAA_TARGET_PREFIX}qcommon ${OPENMOHAA_TARGET_PREFIX}qcommon_standalone)
target_link_libraries(omohserver INTERFACE ${OPENMOHAA_TARGET_PREFIX}gcd)
