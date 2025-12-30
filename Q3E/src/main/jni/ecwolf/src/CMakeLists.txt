if (NOT LIBRETRO AND NOT _DIII4A)
	add_custom_target(revision_check ALL
		COMMAND updaterevision ${CMAKE_CURRENT_BINARY_DIR}/gitinfo.h
		WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
		DEPENDS updaterevision
	)
endif()

if(LIBRETRO)
        set(ENGINE_TARGET ecwolf_libretro)
elseif(_DIII4A)
		set(ENGINE_TARGET ecwolf)
else()
        set(ENGINE_TARGET engine)
endif()

set(DISABLE_ADLIB OFF)      

# The ability to start with an empty target was added in CMake 3.11 so since
# for Android we use a shared library we need to use a macro to determine which
# function to call.
macro(initial_sources TARGET)
	if(LIBRETRO AND LIBRETRO_STATIC)
    		add_library(${ENGINE_TARGET} STATIC ${ARGV})
	elseif(ANDROID OR LIBRETRO)
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
	dobject.cpp
	dobjgc.cpp
	farchive.cpp
	files.cpp
	g_conversation.cpp
	g_mapinfo.cpp
	gamemap.cpp
	gamemap_planes.cpp
	gamemap_uwmf.cpp
	id_ca.cpp
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
	wl_dir3dspr.cpp
	wl_draw.cpp
	wl_floorceiling.cpp
	wl_iwad.cpp
	wl_loadsave.cpp
	wl_menu.cpp
	wl_parallax.cpp
	wl_state.cpp
	zstrformat.cpp
	zstring.cpp
)
      
if(LIBRETRO)
  target_sources(${ENGINE_TARGET} PRIVATE
    libretro/libretro.cpp
    libretro/libretro-vfs.cpp
    libretro/id_in.cpp
    libretro/id_sd.cpp
    libretro/id_us_1.cpp
    libretro/wl_play.cpp
    libretro/wl_game.cpp
    libretro/wl_inter.cpp
    libretro/wl_main.cpp
    libretro/wl_text.cpp
    libretro/g_conversation.cpp
    libretro/g_intermission.cpp)
  if(PS2)
    set(DISABLE_ADLIB ON)
  endif()
  if(RS90) # TODO: Autodetect when we compile for RS90
    set(DISABLE_ADLIB ON)
    target_compile_definitions(${ENGINE_TARGET} PRIVATE -DRS90=1)
  endif()
  if(NOT DISABLE_ADLIB)
    target_sources(${ENGINE_TARGET} PRIVATE
      libretro/id_sd_adlib.cpp
      libretro/id_sd_n3dmus.cpp)
  else()
    target_compile_definitions(${ENGINE_TARGET} PRIVATE -DDISABLE_ADLIB=1)
  endif()

  if(NOT LIBRETRO_STATIC)
	set(ENGINE_BINARY_NAME "ecwolf_libretro${LIBRETRO_SUFFIX}")
  endif()
  # Set the libretro core name.
  set_target_properties(${ENGINE_TARGET} PROPERTIES PREFIX "")
  if(ANDROID)
    set(ENGINE_BINARY_NAME "ecwolf_libretro_android")
  endif()
  set_target_properties(${ENGINE_TARGET} PROPERTIES LIBRARY_OUTPUT_NAME "${ENGINE_BINARY_NAME}")
  # Output the library to CMake's build directory.
  set_target_properties(${ENGINE_TARGET} PROPERTIES LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}")
if(WIN32)
  target_sources(${ENGINE_TARGET} PRIVATE
    libretro/w32_random.cpp)
endif()
if(NOT LIBRETRO_STATIC)
target_sources(${ENGINE_TARGET} PRIVATE
	libretro/libretro-common/compat/compat_posix_string.c
	libretro/libretro-common/compat/compat_snprintf.c
	libretro/libretro-common/compat/compat_strl.c 
	libretro/libretro-common/compat/fopen_utf8.c 
	libretro/libretro-common/encodings/encoding_utf.c
	libretro/libretro-common/file/file_path.c
	libretro/libretro-common/file/retro_dirent.c
	libretro/libretro-common/streams/file_stream.c
	libretro/libretro-common/string/stdstring.c
	libretro/libretro-common/time/rtime.c
	libretro/libretro-common/vfs/vfs_implementation.c)
endif()

  target_include_directories(${ENGINE_TARGET} PRIVATE libretro/libretro-common/include)
else()
  target_sources(${ENGINE_TARGET} PRIVATE
      c_cvars.cpp
      filesys.cpp
      filesys_steam.cpp
      id_in.cpp
      id_sd.cpp
      id_sd_n3dmus.cpp
      id_us_1.cpp
      wl_debug.cpp
      gitinfo.cpp
      g_intermission.cpp
      wl_play.cpp
      wl_game.cpp
      wl_main.cpp
      sdlvideo.cpp
      wl_text.cpp
      wl_inter.cpp
      wl_net.cpp)
endif()


if(ANDROID AND NOT LIBRETRO AND NOT _DIII4A)
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

if(LIBRETRO)
	target_compile_definitions(${ENGINE_TARGET} PRIVATE -DLIBRETRO=1 -DNO_STORE=1 -DDISABLE_GDTOA=1)
	if(WIN32)
		target_compile_definitions(${ENGINE_TARGET} PRIVATE -DUSE_WINDOWS_DWORD=1)
	endif()
endif()

if(LIBRETRO_STATIC)
	set_target_properties(${ENGINE_TARGET} PROPERTIES SUFFIX "_partial.a")
        if(EMSCRIPTEN)
		set(LIBRETRO_EXTENSION "bc")
		set(ZLIB_NAME "libz")
        else()
		set(LIBRETRO_EXTENSION "a")
		set(ZLIB_NAME "libzlibstatic")
        endif()
        add_custom_command(TARGET ${ENGINE_TARGET}
               POST_BUILD
                   COMMAND ${CMAKE_SOURCE_DIR}/src/libretro/merge_static.sh $(AR) ${CMAKE_BINARY_DIR}/ecwolf_libretro${LIBRETRO_SUFFIX}.${LIBRETRO_EXTENSION} ${CMAKE_BINARY_DIR}/src/ecwolf_partial.a ${CMAKE_BINARY_DIR}/deps/bzip2/libbz2.a ${CMAKE_BINARY_DIR}/deps/lzma/liblzma.a ${CMAKE_BINARY_DIR}/deps/jpeg-6b/libjpeg.a ${CMAKE_BINARY_DIR}/deps/zlib/${ZLIB_NAME}.a)
	target_compile_definitions(${ENGINE_TARGET} PRIVATE -DSeqInStream_Read2=ecwolf_SeqInStream_Read2 -DSeqInStream_Read=ecwolf_SeqInStream_Read -DSeqInStream_ReadByte=ecwolf_SeqInStream_ReadByte -DLookInStream_SeekTo=ecwolf_LookInStream_SeekTo -DLookInStream_LookRead=ecwolf_LookInStream_LookRead -DLookInStream_Read2=ecwolf_LookInStream_Read2 -DLookInStream_Read=ecwolf_LookInStream_Read -DSecToLook_CreateVTable=ecwolf_SecToLook_CreateVTable -DSecToRead_CreateVTable=ecwolf_SecToRead_CreateVTable -DCrcGenerateTable=ecwolf_CrcGenerateTable -DCrcUpdateT1=ecwolf_CrcUpdateT1 -Dg_CrcUpdate=ecwolf_g_CrcUpdate -Dg_CrcTable=ecwolf_g_CrcTable -DCrcUpdate=ecwolf_CrcUpdate -Dk7zSignature=ecwolf_k7zSignature -DSzArEx_GetFileNameUtf16=ecwolf_SzArEx_GetFileNameUtf16 -DSzAr_DecodeFolder=ecwolf_SzAr_DecodeFolder -DSzArEx_Extract=ecwolf_SzArEx_Extract -DBuf_Free=ecwolf_Buf_Free -DBuf_Init=ecwolf_Buf_Init -DBuf_Create=ecwolf_Buf_Create -DCrcCalc=ecwolf_CrcCalc -DSzArEx_Open=ecwolf_SzArEx_Open -DSzAr_GetFolderUnpackSize=ecwolf_SzAr_GetFolderUnpackSize -DSzGetNextFolderItem=ecwolf_SzGetNextFolderItem -DSzArEx_Free=ecwolf_SzArEx_Free -DSzArEx_Init=ecwolf_SzArEx_Init -DSzBitUi64s_Free=ecwolf_SzBitUi64s_Free -DSzBitUi32s_Free=ecwolf_SzBitUi32s_Free -Dg_CrcUpdateT4=ecwolf_g_CrcUpdateT4 -Dg_CrcUpdateT8=ecwolf_g_CrcUpdateT8 -DCrcUpdateT4=ecwolf_CrcUpdateT4 -DCrcUpdateT8=ecwolf_CrcUpdateT8 -DLzmaDec_InitDicAndState=ecwolf_LzmaDec_InitDicAndState -DLzmaDec_Init=ecwolf_LzmaDec_Init -DLzmaDec_DecodeToDic=ecwolf_LzmaDec_DecodeToDic -DLzmaDec_DecodeToBuf=ecwolf_LzmaDec_DecodeToBuf -DLzmaDec_FreeProbs=ecwolf_LzmaDec_FreeProbs -DLzmaDec_Free=ecwolf_LzmaDec_Free -DLzmaProps_Decode=ecwolf_LzmaProps_Decode -DLzmaDec_AllocateProbs=ecwolf_LzmaDec_AllocateProbs -DLzmaDec_Allocate=ecwolf_LzmaDec_Allocate -DLzmaDecode=ecwolf_LzmaDecode -DLzma2Dec_AllocateProbs=ecwolf_Lzma2Dec_AllocateProbs -DLzma2Dec_Init=ecwolf_Lzma2Dec_Init -DLzma2Dec_DecodeToDic=ecwolf_Lzma2Dec_DecodeToDic -DBcj2Dec_Init=ecwolf_Bcj2Dec_Init -DBcj2Dec_Decode=ecwolf_Bcj2Dec_Decode -DDelta_Init=ecwolf_Delta_Init -DDelta_Encode=ecwolf_Delta_Encode -DDelta_Decode=ecwolf_Delta_Decode -DLzma2Dec_Allocate=ecwolf_Lzma2Dec_Allocate -DLzma2Dec_DecodeToBuf=ecwolf_Lzma2Dec_DecodeToBuf -DLzma2Decode=ecwolf_Lzma2Decode -DIA64_Convert=ecwolf_IA64_Convert -DSPARC_Convert=ecwolf_SPARC_Convert -Dx86_Convert=ecwolf_x86_Convert -DPPC_Convert=ecwolf_PPC_Convert -DARMT_Convert=ecwolf_ARMT_Convert -DARM_Convert=ecwolf_ARM_Convert)
endif()
      
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

if(LIBRETRO_STATIC)
	set(STRICMP_EXISTS NO)
	set(STRNICMP_EXISTS NO)
	set(ATOLL_EXISTS YES)
else()
	check_function_exists(stricmp STRICMP_EXISTS)
	check_function_exists(strnicmp STRNICMP_EXISTS)
	check_function_exists(atoll ATOLL_EXISTS)
endif()

check_function_exists(malloc_usable_size MALLOC_USABLE_SIZE)
if(MALLOC_USABLE_SIZE)
	target_compile_definitions(${ENGINE_TARGET} PRIVATE -DHAS_MALLOC_USABLE_SIZE=1)
endif()

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
	if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 7.0 )
		target_compile_options(${ENGINE_TARGET} PRIVATE -Wno-implicit-fallthrough)
	endif()
	if( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_VERSION VERSION_LESS 4.4 )
		target_compile_options(${ENGINE_TARGET} PRIVATE -Wno-unused-result)
	endif()
	target_compile_options(${ENGINE_TARGET} PRIVATE -Wall -Wextra -Wno-unused -Wno-unused-parameter -Wno-missing-field-initializers)
endif()

if(LIBRETRO)
        set(NO_GTK ON)
elseif(WIN32)
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

if(APPLE AND NOT LIBRETRO)
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
elseif(ANDROID AND NOT LIBRETRO)
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

if (LIBRETRO)
	if(NOT LIBRETRO_STATIC)
		target_link_libraries(${ENGINE_TARGET} PRIVATE
			${BZIP2_LIBRARIES}
			${JPEG_LIBRARIES}
			lzma
			ZLIB::ZLIB
			)
	else()
		target_include_directories(${ENGINE_TARGET} PRIVATE
		  ${CMAKE_SOURCE_DIR}/deps/lzma/C
		  ${CMAKE_SOURCE_DIR}/deps/zlib
		  ${CMAKE_BINARY_DIR}/deps/zlib)
		add_dependencies(${ENGINE_TARGET} bz2 jpeg lzma zlibstatic)
	endif()
else()
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
		SDL2::SDL2_mixer
		SDL2::SDL2_net
		SDL2::SDL2
		${BZIP2_LIBRARIES}
		${JPEG_LIBRARIES}
		lzma
		gdtoa
		ZLIB::ZLIB
	)
		endif()
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
)

if ("${CMAKE_OSX_DEPLOYMENT_TARGET}" STREQUAL "" OR "${CMAKE_OSX_DEPLOYMENT_TARGET}" VERSION_GREATER "10.4.16")
set_target_properties(${ENGINE_TARGET} PROPERTIES
	# On Linux look for any libraries we link against in the executable directory.
	INSTALL_RPATH "\$ORIGIN"
	BUILD_WITH_INSTALL_RPATH ON)
endif()
      

# Install
if(NOT ANDROID AND NOT LIBRETRO)
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
