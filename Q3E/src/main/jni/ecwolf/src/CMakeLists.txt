if (NOT _DIII4A)
add_custom_target(revision_check ALL
	COMMAND "${CMAKE_COMMAND}" -P "${CMAKE_SOURCE_DIR}/tools/updaterevision/UpdateRevision.cmake" "${CMAKE_CURRENT_BINARY_DIR}/gitinfo.h"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
endif()

if(_DIII4A)
		set(ENGINE_TARGET ecwolf)
else()
        set(ENGINE_TARGET engine)
endif()

set(DISABLE_ADLIB OFF)  

# The ability to start with an empty target was added in CMake 3.11 so since
# for Android we use a shared library we need to use a macro to determine which
# function to call.
macro(initial_sources TARGET)
	if(ANDROID)
		add_library(${ENGINE_TARGET} SHARED ${ARGV})
	else()
		add_executable(${ENGINE_TARGET} WIN32 ${ARGV})
	endif()
endmacro()

initial_sources(
	g_blake/a_smartanim.cpp
	g_blake/blake_sbar.cpp
	g_shared/a_deathcam.cpp
	g_shared/a_inventory.cpp
	g_shared/a_keys.cpp
	g_shared/a_patrolpoint.cpp
	g_shared/a_playerpawn.cpp
	g_shared/a_randomspawner.cpp
	g_wolf/a_spearofdestiny.cpp
	g_wolf/wolf_sbar.cpp
	r_2d/r_draw.cpp
	r_2d/r_drawt.cpp
	r_2d/r_main.cpp
	r_2d/r_things.cpp
	r_data/colormaps.cpp
	r_data/r_translate.cpp
	r_data/renderstyle.cpp
	resourcefiles/resourcefile.cpp
	resourcefiles/ancientzip.cpp
	resourcefiles/file_7z.cpp
	resourcefiles/file_audiot.cpp
	resourcefiles/file_directory.cpp
	resourcefiles/file_gamemaps.cpp
	resourcefiles/file_grp.cpp
	resourcefiles/file_pak.cpp
	resourcefiles/file_rff.cpp
	resourcefiles/file_rtl.cpp
	resourcefiles/file_lump.cpp
	resourcefiles/file_macbin.cpp
	resourcefiles/file_vgagraph.cpp
	resourcefiles/file_vswap.cpp
	resourcefiles/file_wad.cpp
	resourcefiles/file_zip.cpp
	resourcefiles/wolfmapcommon.cpp
	sfmt/SFMT.cpp
	textures/anim_switches.cpp
	textures/animations.cpp
	textures/automaptexture.cpp
	textures/bitmap.cpp
	textures/ddstexture.cpp
	textures/emptytexture.cpp
	textures/flattexture.cpp
	textures/imgztexture.cpp
	textures/jpegtexture.cpp
	textures/machudtexture.cpp
	textures/multipatchtexture.cpp
	textures/patchtexture.cpp
	textures/picttexture.cpp
	textures/pcxtexture.cpp
	textures/pngtexture.cpp
	textures/rawpagetexture.cpp
	textures/rottflattexture.cpp
	textures/solidtexture.cpp
	textures/texture.cpp
	textures/texturemanager.cpp
	textures/tgatexture.cpp
	textures/warptexture.cpp
	textures/wolfrawtexture.cpp
	textures/wolfshapetexture.cpp
	thingdef/thingdef.cpp
	thingdef/thingdef_codeptr.cpp
	thingdef/thingdef_expression.cpp
	thingdef/thingdef_parse.cpp
	thingdef/thingdef_properties.cpp
	thingdef/thingdef_type.cpp
	actor.cpp
	am_map.cpp
	colormatcher.cpp
	config.cpp
	c_cvars.cpp
	dobject.cpp
	dobjgc.cpp
	farchive.cpp
	files.cpp
	filesys.cpp
	filesys_steam.cpp
	g_conversation.cpp
	g_intermission.cpp
	g_mapinfo.cpp
	gamemap.cpp
	gamemap_planes.cpp
	gamemap_uwmf.cpp
	gitinfo.cpp
	id_ca.cpp
	id_in.cpp
	id_sd.cpp
	id_sd_n3dmus.cpp
	id_us_1.cpp
	id_vh.cpp
	id_vl.cpp
	language.cpp
	lnspec.cpp
	lumpremap.cpp
	m_alloc.cpp
	m_classes.cpp
	m_random.cpp
	m_png.cpp
	name.cpp
	p_switch.cpp
	r_sprites.cpp
	scanner.cpp
	sdlvideo.cpp
	sndinfo.cpp
	sndseq.cpp
	thinker.cpp
	v_draw.cpp
	v_font.cpp
	v_palette.cpp
	v_pfx.cpp
	v_text.cpp
	v_video.cpp
	w_wad.cpp
	weaponslots.cpp
	wl_act2.cpp
	wl_agent.cpp
	wl_atmos.cpp
	wl_cloudsky.cpp
	wl_debug.cpp
	wl_dir3dspr.cpp
	wl_draw.cpp
	wl_floorceiling.cpp
	wl_game.cpp
	wl_inter.cpp
	wl_iwad.cpp
	wl_loadsave.cpp
	wl_main.cpp
	wl_menu.cpp
	wl_net.cpp
	wl_parallax.cpp
	wl_play.cpp
	wl_state.cpp
	wl_text.cpp
	zstrformat.cpp
	zstring.cpp
)
      
if(ANDROID AND NOT _DIII4A)
	# On Android we build a shared object so create a dummy program so we can
	# get link errors.
	add_executable(link-test android/link-test.cpp)
	# Link SDL2::SDL2 explicitly here since Aarch64 linker seems to be setup
	# improperly.
	target_link_libraries(link-test PRIVATE ${ENGINE_TARGET} SDL2::SDL2)
endif()

include(CheckCXXSourceCompiles)
include(CheckFunctionExists)
include(FindPkgConfig)

if(GPL)
	target_compile_definitions(${ENGINE_TARGET} PRIVATE -DUSE_GPL)
	if(NOT DISABLE_ADLIB)
		target_sources(${ENGINE_TARGET} PRIVATE dosbox/dbopl.cpp)
	endif()

	if(USE_LIBTEXTSCREEN)
		target_compile_definitions(${ENGINE_TARGET} PRIVATE -DUSE_TEXTSCREEN)
		target_link_directories(${ENGINE_TARGET} PRIVATE textscreen)
	endif()
else()
	target_sources(${ENGINE_TARGET} PRIVATE mame/fmopl.cpp)
endif()

check_function_exists(stricmp STRICMP_EXISTS)
check_function_exists(strnicmp STRNICMP_EXISTS)
check_function_exists(atoll ATOLL_EXISTS)
if(NOT STRICMP_EXISTS)
	target_compile_definitions(${ENGINE_TARGET} PRIVATE -Dstricmp=strcasecmp)
endif()
if(NOT STRNICMP_EXISTS)
	target_compile_definitions(${ENGINE_TARGET} PRIVATE -Dstrnicmp=strncasecmp)
endif()
if(NOT ATOLL_EXISTS)
	target_compile_definitions(${ENGINE_TARGET} PRIVATE -Datoll=_atoi64)
endif()

check_cxx_source_compiles(
	"#include <stdarg.h>
	int main() { va_list list1, list2; va_copy(list1, list2); return 0; }"
	HAS_VA_COPY )
if( NOT HAS_VA_COPY )
	check_cxx_source_compiles(
		"#include <stdarg.h>
		int main() { va_list list1, list2; __va_copy(list1, list2); return 0; }"
		HAS___VA_COPY )
	if( HAS___VA_COPY )
		target_compile_definitions(${ENGINE_TARGET} PRIVATE -Dva_copy=__va_copy)
	else()
		target_compile_definitions(${ENGINE_TARGET} PRIVATE -DNO_VA_COPY)
	endif()
endif()

if( ZD_CMAKE_COMPILER_IS_GNUCXX_COMPATIBLE )
	if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 8.0 )
		# Remove after switching to C++11
		target_compile_options(${ENGINE_TARGET} PRIVATE -Wno-class-memaccess)
	endif()
	if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0 )
		target_compile_options(${ENGINE_TARGET} PRIVATE -Wno-implicit-fallthrough)
	endif()
	if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4 )
		target_compile_options(${ENGINE_TARGET} PRIVATE -Wno-unused-result)
	endif()
	target_compile_options(${ENGINE_TARGET} PRIVATE -Wall -Wextra -Wno-unused -Wno-unused-parameter -Wno-missing-field-initializers)
endif()

if(WIN32)
	set(NO_GTK ON)
	target_link_libraries(${ENGINE_TARGET} PRIVATE comctl32)
	target_compile_definitions(${ENGINE_TARGET} PRIVATE -DNO_STDIO_REDIRECT)

	# RC file
	target_sources(${ENGINE_TARGET} PRIVATE win32/windows.rc)

	# Fix stat in v140_xp (broken in RTM and Update 1 so far)
	if( MSVC AND MSVC_VERSION EQUAL 1900 AND CMAKE_GENERATOR_TOOLSET STREQUAL "v140_xp" )
		target_compile_definitions(${ENGINE_TARGET} PRIVATE -D_stat64i32=VS14Stat)
		target_sources(${ENGINE_TARGET} PRIVATE win32/vs2015hack.cpp)
	endif()

	target_sources(${ENGINE_TARGET} PRIVATE
		win32/i_crash.cpp
		win32/i_main.cpp
		win32/wl_iwad_win32.cpp
	)
elseif(APPLE OR ANDROID)
	set(NO_GTK ON)
	if(_DIII4A)
		target_sources(${ENGINE_TARGET} PRIVATE posix/i_main.cpp)
	endif()
else()
	option( NO_GTK "Disable GTK+ dialogs (Not applicable to Windows)" )

	# Use GTK+ for the IWAD picker, if available.
	if( NOT NO_GTK )
		pkg_check_modules( GTK3 gtk+-3.0 )
		if( GTK3_FOUND )
			target_link_libraries(${ENGINE_TARGET} PRIVATE ${GTK3_LIBRARIES})
			target_include_directories(${ENGINE_TARGET} PRIVATE ${GTK3_INCLUDE_DIRS})
		else()
			set( NO_GTK ON )
		endif()
	endif()

	target_sources(${ENGINE_TARGET} PRIVATE posix/i_main.cpp)
endif()

if(NO_GTK)
	target_compile_definitions(${ENGINE_TARGET} PRIVATE -DNO_GTK=1)
endif()

if(APPLE)
	target_sources(${ENGINE_TARGET} PRIVATE
		macosx/filesys.mm
		macosx/i_main.mm
		macosx/wl_iwad_picker_cocoa.mm
	)

	include(${CMAKE_CURRENT_SOURCE_DIR}/macosx/install.cmake)

	option(BUILD_BUNDLE "Build a app bundle on Mac OS X" ON)
	if(BUILD_BUNDLE)
		set_target_properties(${ENGINE_TARGET} PROPERTIES
			MACOSX_BUNDLE ON
			MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macosx/Info.plist.in"
			MACOSX_BUNDLE_BUNDLE_NAME "${PRODUCT_NAME}"
			MACOSX_BUNDLE_GUI_IDENTIFIER "${PRODUCT_IDENTIFIER}"
			MACOSX_BUNDLE_BUNDLE_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
			MACOSX_BUNDLE_LONG_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
			MACOSX_BUNDLE_SHORT_VERSION_STRING "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}"
		)
	endif(BUILD_BUNDLE)
elseif(_DIII4A)
	target_sources(${ENGINE_TARGET} PRIVATE
			diii4a/in_android.cpp
			diii4a/sys_android.cpp
			)

	# Aarch64 linker seems to be setup improperly and requires these to be
	# public for link-test
	target_link_libraries(${ENGINE_TARGET} PUBLIC android log )
elseif(ANDROID)
	target_sources(${ENGINE_TARGET} PRIVATE
		android/android-jni.cpp
		android/in_android.cpp
		android/wl_iwad_picker_android.cpp
	)

	# Aarch64 linker seems to be setup improperly and requires these to be
 	# public for link-test
	target_link_libraries(${ENGINE_TARGET} PUBLIC
		touchcontrols
		GLESv1_CM
		log
	)
endif()

# Derive version info and generate header
string(TOUPPER "${PRODUCT_NAME}" PRODUCT_SIGNATURE)
string(TOLOWER "${PRODUCT_NAME}" PRODUCT_BINARY)
math(EXPR VERSION_INTEGER "(${VERSION_MAJOR}<<20) | (${VERSION_MINOR}<<8) | ${VERSION_PATCH}")
configure_file(versiondefs.h.in versiondefs.h)

# Glob for the header files so project generation can include them
file(GLOB HEADER_FILES
	g_shared/*.h
	g_wolf/*.h
	r_data/*.h
	resourcefiles/*.h
	sfmt/*.h
	textures/*.h
	thingdef/*.h
	*.h
)
list(APPEND HEADER_FILES ${CMAKE_CURRENT_BINARY_DIR}/versiondefs.h)
target_sources(${ENGINE_TARGET} PRIVATE ${HEADER_FILES})

if(_DIII4A)
		target_link_libraries(${ENGINE_TARGET} PRIVATE
				SDL2_mixer
				SDL2_net
				SDL2
				${BZIP2_LIBRARIES}
				${JPEG_LIBRARIES}
				${ECWOLF_PREFIX}lzma
				${ECWOLF_PREFIX}gdtoa
				${ECWOLF_PREFIX}ZLIB::ZLIB
				)
else()
	add_dependencies(${ENGINE_TARGET} revision_check)
	target_link_libraries(${ENGINE_TARGET} PRIVATE
	SDL2_mixer::SDL2_mixer
	SDL2_net::SDL2_net
	SDL2::SDL2
	${BZIP2_LIBRARIES}
	${JPEG_LIBRARIES}
	lzma
	gdtoa
	ZLIB::ZLIB
)
endif()
target_include_directories(${ENGINE_TARGET} PRIVATE
	${BZIP2_INCLUDE_DIR}
	${JPEG_INCLUDE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_SOURCE_DIR}/g_shared
)

set_target_properties(${ENGINE_TARGET} PROPERTIES
	CXX_STANDARD 98
	OUTPUT_NAME "${ENGINE_BINARY_NAME}"
	RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
	# On Linux look for any libraries we link against in the executable directory.
	INSTALL_RPATH "\$ORIGIN"
	BUILD_WITH_INSTALL_RPATH ON
)

# Windows console wrapper
if(WIN32)
	add_executable(engine-console win32/console-wrapper.c)
	set_target_properties(engine-console PROPERTIES
		OUTPUT_NAME "${ENGINE_BINARY_NAME}-console"
		RUNTIME_OUTPUT_DIRECTORY "${OUTPUT_DIR}"
	)
	target_link_libraries(engine-console PRIVATE shlwapi)

	if(MSVC)
		# This program doesn't actually need the C runtime
		if(CMAKE_VERSION VERSION_GREATER_EQUAL 4.0)
			target_compile_options(engine-console PRIVATE /GS-)
			target_link_options(engine-console PRIVATE /NODEFAULTLIB /ENTRY:wmain)
			set_target_properties(engine-console PROPERTIES MSVC_RUNTIME_CHECKS "")
		else()
			target_compile_options(engine-console PRIVATE $<$<NOT:$<CONFIG:Debug>>:/GS->)
			if(CMAKE_VERSION VERSION_GREATER_EQUAL 3.13)
				target_link_options(engine-console PRIVATE $<$<NOT:$<CONFIG:Debug>>:/NODEFAULTLIB> $<$<NOT:$<CONFIG:Debug>>:/ENTRY:wmain>)
			else()
				set_target_properties(engine-console PROPERTIES
					LINK_FLAGS_RELEASE "/NODEFAULTLIB /ENTRY:wmain"
					LINK_FLAGS_RELWITHDEBINFO "/NODEFAULTLIB /ENTRY:wmain"
					LINK_FLAGS_MINSIZEREL "/NODEFAULTLIB /ENTRY:wmain"
				)
			endif()
		endif()

		# Legacy Visual C++ has terrible C support
		if(MSVC_VERSION LESS 1900)
			set_source_files_properties(win32/console-wrapper.c PROPERTIES LANGUAGE CXX)
		endif()
	else()
		target_link_options(engine-console PRIVATE -municode)
		# Workarounds for static linking MinGW runtimes interfere with our ability to trim down this binary
		if(NOT CMAKE_EXE_LINKER_FLAGS MATCHES "--whole-archive")
			target_link_options(engine-console PRIVATE -nodefaultlibs -nostartfiles --entry=_wmain)
		endif()
endif()
      
	install(TARGETS engine-console BUNDLE DESTINATION ${OUTPUT_DIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime)
endif()

# Install
if(NOT ANDROID)
	install(TARGETS ${ENGINE_TARGET} BUNDLE DESTINATION ${OUTPUT_DIR} RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} COMPONENT Runtime)

	if(NOT WIN32 AND NOT APPLE)
		configure_file(posix/engine.desktop.in posix/engine.desktop)
		install(FILES "${CMAKE_CURRENT_BINARY_DIR}/posix/engine.desktop" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications" RENAME "${ENGINE_BINARY_NAME}.desktop")
		install(FILES "posix/icon.svg" DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/icons/hicolor/scalable/apps" RENAME "${ENGINE_BINARY_NAME}.svg")
	endif()
endif()

# Project file source groupings
source_group("Sources" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/.+")
source_group("Game Objects\\Decorate" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/thingdef/.+")
source_group("Game Objects\\Shared" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/g_shared/.+")
source_group("Game Objects\\Wolf3D" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/g_wolf/.+")
source_group("Game Objects" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/(actor|dobj|thinker).+")
source_group("OPL Emulator\\DOSBox" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/dosbox/.+")
source_group("OPL Emulator\\MAME" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/mame/.+")
source_group("Renderer" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/(r_|wl_draw|wl_floorceiling|wl_parallax|wl_atmos|wl_cloudsky).+")
source_group("Renderer\\2D Drawing" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/r_2d/.+")
source_group("Renderer\\Data" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/r_data/.+")
source_group("Resource Files" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/resourcefiles/.+")
source_group("SFMT" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/sfmt/.+")
source_group("Textures" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/textures/.+")
source_group("Win32" REGULAR_EXPRESSION "^${CMAKE_CURRENT_SOURCE_DIR}/win32/.+")
