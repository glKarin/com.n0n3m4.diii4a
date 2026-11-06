cmake_minimum_required(VERSION 3.14)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

get_property(is_multi_config GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT is_multi_config AND NOT CMAKE_BUILD_TYPE)
	# sometimes people build without specifying build type, then get surprised how slow it is...
	message(FATAL_ERROR "Set CMAKE_BUILD_TYPE to either Release or Debug")
endif()

if (ANDROID)
	# we want debug symbols even with Release builds; enable the "good parts" of -ffast-math (which is not to be trusted)
	string(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT "-g -fno-math-errno -fno-trapping-math -ffinite-math-only") #[[ -fsigned-char]]
	string(APPEND CMAKE_C_FLAGS_RELEASE_INIT "-g -fno-math-errno -fno-trapping-math -ffinite-math-only") #[[ -fsigned-char]]
elseif (UNIX)
	# we want debug symbols even with Release builds; enable the "good parts" of -ffast-math (which is not to be trusted)
	string(APPEND CMAKE_CXX_FLAGS_RELEASE_INIT "-g -fno-math-errno -fno-trapping-math -ffinite-math-only")
	string(APPEND CMAKE_C_FLAGS_RELEASE_INIT "-g -fno-math-errno -fno-trapping-math -ffinite-math-only")

	option(FORCE_COLORED_OUTPUT "Always produce ANSI-colored compiler warnings/errors (GCC/Clang only; esp. useful with ninja)." OFF)
	option(ASAN		"Enable GCC/Clang Adress Sanitizer (ASan)" OFF) # TODO: MSVC also supports this, somehow?
elseif(MSVC)
	set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE)
endif()

option(TDM_THIRDPARTY_ARTEFACTS "Use prebuilt artefacts at ThirdParty/artefacts?" OFF) # ON

# find ordinary cmake includes
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/sys/cmake")
# find third-party package modules
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/ThirdParty/cmake_find_package")

include(CMakeDependentOption)
include(sys/cmake/ucm.cmake)
include(sys/cmake/PrecompiledHeader.cmake)
include(sys/cmake/SvnVersion.cmake)
include(CheckCXXCompilerFlag)
include(CheckPIESupported)

project(TheDarkMod)
# note: must be after project: https://gitlab.kitware.com/cmake/cmake/-/issues/14983
check_pie_supported()
if(TDM_THIRDPARTY_ARTEFACTS)
	include(ThirdParty/use_artefacts.cmake)
endif()

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

cmake_dependent_option(WITH_TOOLS "Compile with Tools" ON "MSVC" OFF)
set(GAME_DIR "${PROJECT_SOURCE_DIR}/../darkmod" CACHE STRING "The directory where the game assets are stored")
option(COPY_EXE "Copy executable to GAME_DIR after build" OFF) # ON
option(ENABLE_TRACY "Enable tracy profiling support. Needs to be activated ingame by enabling the cvar com_enableTracing." OFF) #k ON

if (UNIX)
	# configure sensible warnings
	add_compile_options(
			-Wno-unknown-pragmas            # MSVC specific pragmas
			-Wno-unused-variable            # too many
			-Wno-unused-but-set-variable
			-Wno-sign-compare               # in particular, int/size_t conversions
			-Wno-format-security            # format string is often composed dynamically
			-Woverloaded-virtual            # almost-overriding virtual functions is dangerous for us
			-Wsuggest-override              # recommend using "override" keyword
			-fPIC
	)
	CHECK_CXX_COMPILER_FLAG("-Wdangling-reference" cxx_has_Wdangling_reference)
	if(cxx_has_Wdangling_reference)
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Werror=dangling-reference")
	endif()

	# configure other compile options
	add_compile_options(
			-pipe
			-fno-strict-aliasing
			-fno-omit-frame-pointer         # crashdumps!
			-fmessage-length=0
			-fvisibility=hidden
			#-std=c++14
			-fno-unsafe-math-optimizations
	)

    if(NOT ANDROID)
	# enable SSE2 if supported (mainly 32-bit x86)
	check_cxx_compiler_flag(-msse2 MSSE2_SUPPORTED)
	if (MSSE2_SUPPORTED)
		add_compile_options(-msse2)
    endif()
    endif()

	# don't mess up math code with useless FMA "optimizations"
	# (https://gcc.gnu.org/bugzilla/show_bug.cgi?id=100839)
	# see also https://github.com/dhewm/dhewm3/commit/2521c3dfdb87c9261f69615ab7ecc241c1046bb4
	# and https://github.com/dhewm/dhewm3/issues/147
	# and https://github.com/RobertBeckebans/RBDOOM-3-BFG/issues/436#issuecomment-850930254 (and following)
	CHECK_CXX_COMPILER_FLAG("-ffp-contract=off" cxx_has_fp-contract)
	if(cxx_has_fp-contract)
		add_compile_options(-ffp-contract=off)
	endif()

	if(FORCE_COLORED_OUTPUT)
		if(CMAKE_COMPILER_IS_GNUCC)
		   add_compile_options (-fdiagnostics-color=always)
		else() # clang, probably
		   add_compile_options (-fcolor-diagnostics)
		endif ()
	endif ()

	if(ASAN)
		# if this doesn't work, ASan might not be available on your platform, don't set ASAN then..
		add_compile_options(-fsanitize=address)
	endif()

	# TODO: do we really want to keep using this
	# use old ABI for std::string and std::list to allow TDM to run on distros with old
	# glibcxx (e.g. Ubuntu 14.04). the old ABI is not fully compliant with C++11 standard.
	add_compile_definitions("_GLIBCXX_USE_CXX11_ABI=0")
	# ensure threading support in X
	add_compile_definitions(XTHREADS)
elseif (MSVC)
	add_compile_definitions("WIN32" _WINDOWS
			_CRT_SECURE_NO_DEPRECATE
			_CRT_NONSTDC_NO_DEPRECATE
			_ALLOW_KEYWORD_MACROS
			NO_WARN_MBCS_MFC_DEPRECATION
			_MBCS
	)
	# disable certain warnings
	add_compile_options(/wd4127 /wd4458 /wd4267)
	add_compile_options(/MP /Gm-)
	ucm_add_flags(/GS- /Gy /Zi /GF /FC /Oi /Oy /Ot CONFIG Release)
	ucm_add_linker_flags(EXE /LARGEADDRESSAWARE /NXCOMPAT /SAFESEH:NO /STACK:16777216,16777216)
	set(MSSE2_SUPPORTED 1)
	if (CMAKE_SIZEOF_VOID_P EQUAL 4)
		add_compile_options(/arch:SSE2)
	endif()
	ucm_add_linker_flags(EXE "/DEBUG" CONFIG Release)
endif ()


if(ANDROID)
    add_library(TheDarkMod SHARED)
else()
add_executable(TheDarkMod)
endif()

get_svn_revision(SVNVERSION)
configure_file("idlib/svnversion.cmake.h" "${PROJECT_SOURCE_DIR}/idlib/svnversion.h" @ONLY)

# gather list of source files
ucm_add_dirs(
		"cm"
		"framework"
		"game"
		"idlib"
		"renderer"
		"sound"
		"tests"
		"TypeInfo"
		"ui"
		"tools/compilers"
		TO TDM_SOURCE_FILES RECURSIVE
)
ucm_add_dirs(
	"sys/renderdoc"
	TO TDM_SOURCE_FILES RECURSIVE
)
ucm_add_files(
	"sys/sys_local.cpp"
	"sys/sys_local.h"
	"sys/sys_public.h"
	"sys/sys_padinput.cpp"
	"sys/sys_padinput.h"
	"sys/sys_cpu.cpp"
	TO TDM_SOURCE_FILES
)

if(ANDROID)
    ucm_remove_files(
            "ThirdParty/artefacts/tracy/src/TracyClient.cpp"
            FROM TDM_SOURCE_FILES
    )
    add_definitions(-D_NO_TRACY)
    add_definitions(-D_GLES)
endif()

if (WITH_TOOLS)
	ucm_add_dirs("tools" TO TDM_SOURCE_FILES RECURSIVE)
	ucm_remove_files(
			"tools/edit_stub.cpp"
			"tools/decl/DialogEntityDefEditor.cpp"
			"tools/common/RenderBumpFlatDialog.cpp"
			"tools/guied/GEWindowWrapper_stub.cpp"
			FROM TDM_SOURCE_FILES
	)
else ()
	ucm_add_files("tools/guied/GEWindowWrapper_stub.cpp" TO TDM_SOURCE_FILES)
	add_compile_definitions(NO_MFC)
endif ()

if(ANDROID)
    ucm_add_dirs("sys/android" "sys/posix" TO TDM_SOURCE_FILES RECURSIVE)
    ucm_remove_directories("idlib/sys/win32" FROM TDM_SOURCE_FILES)
    ucm_remove_files(
            "renderer/backend/qgl/glad_wgl.c"
            "renderer/backend/qgl/glad_glx.c"
            "sys/android/dedicated.cpp"
            "sys/android/test_scheduler.c"
            "sys/posix/platform_osx.cpp"
            "sys/android/sys_android.cpp"
            FROM TDM_SOURCE_FILES
    )
    ucm_remove_directories("idlib/sys/linux" FROM TDM_SOURCE_FILES)
elseif (UNIX)
	ucm_add_dirs("sys/linux" "sys/posix" TO TDM_SOURCE_FILES RECURSIVE)
	ucm_remove_directories("idlib/sys/win32" FROM TDM_SOURCE_FILES)
	ucm_remove_files(
			"renderer/backend/qgl/glad_wgl.c"
			"sys/linux/dedicated.cpp"
			"sys/linux/test_scheduler.c"
			"sys/posix/platform_osx.cpp"
			FROM TDM_SOURCE_FILES
	)
elseif (WIN32)
	ucm_add_dirs("sys/win32" TO TDM_SOURCE_FILES RECURSIVE)
	ucm_add_files("sys/win32/rc/doom.rc" TO TDM_SOURCE_FILES)
	ucm_remove_directories("idlib/sys/posix" FROM TDM_SOURCE_FILES)
	ucm_remove_files("renderer/backend/qgl/glad_glx.c" FROM TDM_SOURCE_FILES)
endif ()

# blacklist dead code files that unfortunately still hang around the repository
ucm_remove_directories("framework/async" FROM TDM_SOURCE_FILES)
ucm_remove_files(
		"TypeInfo/main.cpp"
		"game/Game_network.cpp"
		"game/MultiplayerGame.cpp"
		"game/gamesys/Callbacks.cpp"
		"idlib/math/Simd_AltiVec.cpp"
		"sys/win32/win_gamma.cpp"
		FROM TDM_SOURCE_FILES
)

OPTION(TDM_USE_INTERNAL_MINIZIP OFF)
if(TDM_USE_INTERNAL_MINIZIP)
    ucm_add_files(
            "ThirdParty/artefacts/minizip/src/ioapi.c"
            "ThirdParty/artefacts/minizip/src/miniunz.c"
            "ThirdParty/artefacts/minizip/src/minizip.c"
            "ThirdParty/artefacts/minizip/src/unzip.c"
            "ThirdParty/artefacts/minizip/src/zip.c"
            TO TDM_SOURCE_FILES)
endif()
target_sources(TheDarkMod PRIVATE ${TDM_SOURCE_FILES})

if (MSVC)
	ucm_add_dirs("sys/msvc/natvis" TO TDM_NATVIS_FILES RECURSIVE ADDITIONAL_EXT "natvis")
	target_sources(TheDarkMod INTERFACE ${TDM_NATVIS_FILES})
endif ()

# configure precompiled header
# note TypeInfo.cpp must be excluded from precompiled header because it uses some dirty hacks
add_precompiled_header(TheDarkMod
		"idlib/precompiled.h" "idlib/precompiled.cpp" "precompiled.h"
		EXCLUDE
		"game/gamesys/TypeInfo.cpp"
		"idlib/Token.cpp"
		"sys/win32/win_shared.cpp"
		"sys/win32/rc/CreateResourceIDs.cpp"
		"framework/TracingEmbeddedSourceCode.cpp"
		"ThirdParty/artefacts/tracy/src/TracyClient.cpp"
)

add_compile_definitions(
		"ExtLibs="
		IL_STATIC_LIB
		__DOOM__
		CURL_STATICLIB
		LIBJPEG_STATIC
		AL_LIBTYPE_STATIC
)
if (ENABLE_TRACY)
	add_compile_definitions(
			"$<$<CONFIG:RELEASE>:TRACY_ENABLE>"
			TRACY_DELAYED_INIT
			TRACY_MANUAL_LIFETIME
			TRACY_ON_DEMAND
			TRACY_NO_BROADCAST
			TRACY_ONLY_LOCALHOST
	)
endif()

if (ANDROID)
    set_target_properties(TheDarkMod PROPERTIES OUTPUT_NAME "TheDarkMod")
elseif (UNIX)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set_target_properties(TheDarkMod PROPERTIES OUTPUT_NAME "thedarkmod.x64")
	else ()
		set_target_properties(TheDarkMod PROPERTIES OUTPUT_NAME "thedarkmod.x86")
	endif ()
elseif (MSVC)
	if (CMAKE_SIZEOF_VOID_P EQUAL 8)
		set_target_properties(TheDarkMod PROPERTIES OUTPUT_NAME "TheDarkModx64")
	else ()
		set_target_properties(TheDarkMod PROPERTIES OUTPUT_NAME "TheDarkMod")
	endif ()
endif ()
# ffmpeg dep was compiled without -fPIC, so need to disable PIE on 64bit builds
set_target_properties(TheDarkMod PROPERTIES POSITION_INDEPENDENT_CODE OFF)	# see also CMP0083
ucm_set_runtime(STATIC)


set(DEPLIBS_PATH "${PROJECT_SOURCE_DIR}/../deplibs")
set(DEPINCS_PATH "${PROJECT_SOURCE_DIR}/ThirdParty/include")
# ogg headers
set(OGG_CONFIG_TYPES_HEADER_PATH ${DEPINCS_PATH})
include(${DEPLIBS_PATH}/libogg/cmake/GenerateConfigTypesHeader.cmake)
# png headers
configure_file(${DEPLIBS_PATH}/libpng/scripts/pnglibconf.h.prebuilt ${DEPINCS_PATH}/pnglibconf.h)
include_directories(
		"${PROJECT_SOURCE_DIR}"
		"${PROJECT_SOURCE_DIR}/idlib"
        "${PROJECT_SOURCE_DIR}/ThirdParty/artefacts/doctest/include"
        "${DIII4A_DEPEND_INCLUDE_PATH}/openal"
        "${DEPLIBS_PATH}"
        "${DEPLIBS_PATH}/curl/include"
        "${DEPLIBS_PATH}/libjpeg/libjpeg"
        "${DEPLIBS_PATH}/libpng"
        "${DEPLIBS_PATH}/pugixml/src"
        "${DEPLIBS_PATH}/libogg/include"
        "${DEPLIBS_PATH}/libvorbis/include"
        "${DEPINCS_PATH}"
)

#[[
find_package(ZLIB REQUIRED CONFIG)
find_package(minizip REQUIRED CONFIG)
find_package(CURL REQUIRED CONFIG)
find_package(OpenAL REQUIRED CONFIG)
find_package(Ogg REQUIRED CONFIG)
find_package(Vorbis REQUIRED CONFIG)
find_package(JPEG REQUIRED CONFIG)
find_package(PNG REQUIRED CONFIG)
find_package(ffmpeg REQUIRED CONFIG)
find_package(pugixml REQUIRED CONFIG)
find_package(MbedTLS REQUIRED CONFIG)
find_package(glfw3 REQUIRED CONFIG)
find_package(doctest REQUIRED CONFIG)
find_package(tracy REQUIRED CONFIG)

target_link_libraries(TheDarkMod
		ZLIB::ZLIB
		minizip::minizip
		CURL::libcurl
		OpenAL::OpenAL
		vorbis::vorbis
		Ogg::ogg
		JPEG::JPEG
		PNG::PNG
		ffmpeg::ffmpeg
		glfw
		pugixml::pugixml
		doctest::doctest
		tracy::tracy
)
]]


if(TDM_USE_INTERNAL_MINIZIP)
    include_directories(
            "${PROJECT_SOURCE_DIR}/ThirdParty/artefacts/minizip/include"
            "${PROJECT_SOURCE_DIR}/ThirdParty/artefacts/minizip/include/minizip"
    )
else()
    include_directories(
            "${DEPLIBS_PATH}/minizip"
    )
    SET(TDM_MINIZIP_LIB "minizip")
endif()
target_link_libraries(TheDarkMod
		z
		${TDM_MINIZIP_LIB}
		vorbis
		ogg
		mbedtls
        pugixml
        png
        curl
        openal
        vorbisfile
        jpeg
)

if (WIN32)
	target_link_libraries(TheDarkMod ws2_32 iphlpapi winmm bcrypt dinput8 dbghelp)
	set_target_properties(TheDarkMod PROPERTIES WIN32_EXECUTABLE TRUE)
elseif (ANDROID)
    target_link_libraries(TheDarkMod z dl android EGL)
    target_include_directories(TheDarkMod PRIVATE "../common")
    #set_target_properties(TheDarkMod PROPERTIES LINK_FLAGS "-static-libstdc++")
elseif (UNIX)
	target_link_libraries(TheDarkMod pthread dl stdc++fs X11 Xext Xxf86vm)

	if(ASAN)
		target_link_libraries(TheDarkMod -fsanitize=address)
	endif()

	# strip debug symbols from executable and put them into a separate symbols file
	add_custom_command(
			TARGET TheDarkMod POST_BUILD
			COMMAND objcopy --only-keep-debug "$<TARGET_FILE:TheDarkMod>" "$<TARGET_FILE:TheDarkMod>.debug"
	)
	add_custom_command(
			TARGET TheDarkMod POST_BUILD
			COMMAND strip --strip-debug --strip-unneeded "$<TARGET_FILE:TheDarkMod>"
	)
	add_custom_command(
			TARGET TheDarkMod POST_BUILD
			COMMAND objcopy "--add-gnu-debuglink=$<TARGET_FILE:TheDarkMod>.debug" "$<TARGET_FILE:TheDarkMod>"
	)
endif ()

if (COPY_EXE)
	# copy produced executable to game directory '../darkmod'
	add_custom_command(
			TARGET TheDarkMod POST_BUILD
			COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:TheDarkMod> "${GAME_DIR}"
			COMMAND ${CMAKE_COMMAND} -E copy_directory "${PROJECT_SOURCE_DIR}/glprogs" "${GAME_DIR}/glprogs"
	)
	if (UNIX)
		add_custom_command(
				TARGET TheDarkMod POST_BUILD
				COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:TheDarkMod>.debug "${GAME_DIR}"
		)
	endif()
endif ()
