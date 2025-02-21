#Note: this file was made primarily to support msvc and its project file incompatibilities nightmare.
#Its also useful for various other IDEs like QtCreator etc.
#It uses system libraries, so it will have dependancy issues with public releases where those dependancies are distro/version-specific.
#Public builds are still built using the (overcomplicated) traditional (g)makefile.

CMAKE_MINIMUM_REQUIRED(VERSION 3.0)
cmake_policy(SET CMP0063 NEW)

PROJECT(FTEQuake)

# build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()
add_definitions(-D_DIII4A) # android
add_definitions(-D_SIZEOFPTR=${CMAKE_SIZEOF_VOID_P})

if(ANDROID)
set(FTEQW_DEPLIBS_NAME deplibs)
set(FTEQW_DEPLIBS ${CMAKE_CURRENT_SOURCE_DIR}/../${FTEQW_DEPLIBS_NAME})
endif()

INCLUDE_DIRECTORIES(
	engine/common
	engine/client
	engine/qclib
	engine/gl
	engine/server
	engine
)

if(NOT ANDROID)
IF (EXISTS ${CMAKE_SOURCE_DIR}/.svn AND "${FTE_REVISON}" STREQUAL "")
	EXECUTE_PROCESS(COMMAND
		"svnversion"
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		OUTPUT_VARIABLE FTE_REVISON
		ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
	)

	IF (NOT "${FTE_REVISON}" STREQUAL "")
		MESSAGE(STATUS "FTE SVN Revision ${FTE_REVISON}")
		IF(FTE_REVISON MATCHES "M")
			MESSAGE(STATUS "--- PRIVATE CHANGES DETECTED ---")
			SET(FTE_REVISON SVNREVISION=${FTE_REVISON})
		ELSE()
			MESSAGE(STATUS "No local changes")
			EXECUTE_PROCESS(COMMAND
				svn info --show-item last-changed-date --no-newline
				WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
				OUTPUT_VARIABLE FTE_DATE
			)

			SET(FTE_REVISON SVNREVISION=${FTE_REVISON} SVNDATE=${FTE_DATE})
		ENDIF()
	ENDIF()
ENDIF()
IF (EXISTS ${CMAKE_SOURCE_DIR}/.git AND "${FTE_REVISON}" STREQUAL "")
	EXECUTE_PROCESS(COMMAND
		git describe --always --long --dirty
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		OUTPUT_VARIABLE FTE_REVISON_GIT
		ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	EXECUTE_PROCESS(COMMAND
		git log -1 --format=%cs
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		OUTPUT_VARIABLE FTE_DATE
		ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	EXECUTE_PROCESS(COMMAND
		git branch --show-current
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		OUTPUT_VARIABLE FTE_BRANCH
		ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	EXECUTE_PROCESS(COMMAND
		git rev-parse --is-shallow-repository
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
		OUTPUT_VARIABLE FTE_GIT_IS_SHALLOW
		ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
	)
	IF(FTE_GIT_IS_SHALLOW STREQUAL true)
		MESSAGE(STATUS "shallow clone prevents calculation of revision number.")
		SET(SVNREVISION "git-${FTE_REVISON_GIT}")	#if its a shallow clone then we can't count commits properly so don't know what revision we actually are.
	ELSE()
		EXECUTE_PROCESS(COMMAND
			git rev-list HEAD --count
			WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
			OUTPUT_VARIABLE SVNREVISION
			ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE
		)
		MATH(EXPR SVNREVISION "${SVNREVISION} + 29") #not all svn commits managed to appear im the git repo, so we have a small bias to keep things consistent.
		IF (FTE_BRANCH STREQUAL "master" OR FTE_BRANCH STREQUAL "")
			SET(SVNREVISION "${SVNREVISION}-git-${FTE_REVISON_GIT}")
		ELSE()
			SET(SVNREVISION "${FTE_BRANCH}-${SVNREVISION}-git-${FTE_REVISON_GIT}") #weird branches get a different form of revision, to reduce confusion.
		ENDIF()
	ENDIF()
	MESSAGE(STATUS "FTE GIT ${FTE_BRANCH} Revision ${SVNREVISION}, ${FTE_DATE}")
	SET(FTE_REVISON SVNREVISION=${SVNREVISION} SVNDATE=${FTE_DATE} FTE_BRANCH=${FTE_BRANCH})
ENDIF()
else()
	SET(FTE_REVISON SVNREVISION=5662b3a23d3c16f5a3b5fe74efb88e77d8fc4da0 SVNDATE=2025-02-02 FTE_BRANCH=master)
endif()

#plugins need visibility hidden in order to avoid conflicts with function names that match the engine.
#this is consistent with how windows works so no great loss.
#plus it means that gcc can inline more (with LTO), including optimising args.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)

IF(${CMAKE_VERSION} VERSION_LESS "3.9.0")
	MESSAGE(STATUS "no LTO - old cmake.")
ELSE()
	cmake_policy(SET CMP0069 NEW)
	IF(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
		#use LTO where possible. reportedly requires cmake 3.9 to actually work
		INCLUDE(CheckIPOSupported)
		check_ipo_supported(RESULT result)
		IF(result)
			SET(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
			MESSAGE(STATUS "Using LTO.")
		ELSE()
			MESSAGE(STATUS "no LTO - not supported.")
		ENDIF()
	ELSE()
		MESSAGE(STATUS "no LTO - debug.")
	ENDIF()
ENDIF()

# Added these to solve some build issues I ran into - Brad
if(ANDROID)
if(CMAKE_CXX_BYTE_ORDER EQUAL BIG_ENDIAN)
	ADD_DEFINITIONS(-DFTE_BIG_ENDIAN)
else()
	ADD_DEFINITIONS(-DFTE_LITTLE_ENDIAN)
endif()
else()
IF(FTE_BIG_ENDIAN)
	ADD_DEFINITIONS(-DFTE_BIG_ENDIAN)
ENDIF()

IF(FTE_LITTLE_ENDIAN)
	ADD_DEFINITIONS(-DFTE_LITTLE_ENDIAN)
ENDIF()
endif()

# libepoll-shim needs to be installed on the BSDs and Mac OSX to get 
# some of the server code to compile and work correctly on those platforms - Brad
IF(CMAKE_SYSTEM_NAME MATCHES "BSD" OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
	INCLUDE(FetchContent)
	FetchContent_Declare(
		epoll-shim
		GIT_REPOSITORY https://github.com/jiixyj/epoll-shim.git
		GIT_TAG master
	)
	SET(epoll-shim BUILD_TESTS OFF CACHE INTERNAL "")
	SET(epoll-shim BUILD_SHARED_LIBS OFF CACHE INTERNAL "")
	FetchContent_MakeAvailable(epoll-shim)

	SET(EPOLL_INC_DIR "${epoll-shim_SOURCE_DIR}/include")
ENDIF()

SET(FTE_BUILD_CONFIG ${CMAKE_CURRENT_SOURCE_DIR}/engine/common/config_fteqw.h CACHE FILEPATH "Which build config file to use to control supported features.")
SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};CONFIG_FILE_NAME=${FTE_BUILD_CONFIG})
SET(FTE_USE_SDL false CACHE BOOL "Force the use of SDL instead of using native builds.")

INCLUDE(GNUInstallDirs)
SET(FTE_INSTALL_BINDIR games CACHE STRING "Binary dir to install to.")
SET(FTE_INSTALL_LIBDIR fteqw CACHE STRING "Binary dir to install to.")

IF(NOT WIN32)
	SET(SYS_LIBS ${SYS_LIBS} m)
ELSE()
	SET(SYS_LIBS ${SYS_LIBS})
ENDIF()

SET(FTE_DEP_ZLIB true CACHE BOOL "Link against zlib.")
IF(FTE_DEP_ZLIB)
	FIND_PACKAGE(ZLIB)
ENDIF()
IF(ZLIB_FOUND)
	SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};ZLIB_STATIC)
	SET(FTE_LIBS ${FTE_LIBS} ${ZLIB_LIBRARIES})
	SET(FTESV_LIBS ${FTESV_LIBS} ${ZLIB_LIBRARIES})
	SET(FTEQCC_LIBS ${FTEQCC_LIBS} ${ZLIB_LIBRARIES})
	SET(FTEQTV_LIBS ${FTEQTV_LIBS} ${ZLIB_LIBRARIES})
ELSE()
	MESSAGE(WARNING "libz library NOT available. compressed pk3, ICE, Q2E, etc etc, yada yada, blah blah will not be available.")
	SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};NO_ZLIB)
ENDIF()

SET(FTE_DEP_BZIP2 true CACHE BOOL "Link against libbzip2.")
IF(FTE_DEP_BZIP2)
if(ANDROID)
set(BZIP2_FOUND TRUE)
	INCLUDE_DIRECTORIES( ${FTEQW_DEPLIBS}/bzip2 )
else()
	FIND_PACKAGE(BZip2)
endif()
ENDIF()
IF(BZIP2_FOUND)
	SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};AVAIL_BZLIB;BZLIB_STATIC)
	SET(FTE_LIBS ${FTE_LIBS} bz2)
	SET(FTESV_LIBS ${FTESV_LIBS} bz2)
#	MESSAGE(STATUS "bzip2 library found. bz2-compressed pk3s will work for the price of extra bloat! yay!")
ELSE()
	MESSAGE(WARNING "bzip2 library NOT available. bz2-compressed pk3s will not be available, as if anyone cares.")
ENDIF()

SET(OpenGL_GL_PREFERENCE LEGACY)
if(ANDROID)
set(OpenGL_FOUND TRUE)
else()
FIND_PACKAGE(OpenGL)
endif()
IF(OpenGL_FOUND)
	SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};GLQUAKE)
ELSE()
	MESSAGE(WARNING "opengl library NOT available. Will depend upon vulkan.")
	SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};NO_OPENGL)
ENDIF()

SET(FTE_DEP_JPEG true CACHE BOOL "Link against libjpeg.")
IF(FTE_DEP_JPEG)
if(ANDROID)
    set(JPEG_FOUND TRUE)
    set(JPEG_INCLUDE_DIRS ${FTEQW_DEPLIBS}/libjpeg/libjpeg)
    set(JPEG_LIBRARIES jpeg)
else()
	FIND_PACKAGE(JPEG)
endif()
ENDIF()
IF(JPEG_FOUND)
	INCLUDE_DIRECTORIES( ${JPEG_INCLUDE_DIRS} )
	SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};LIBJPEG_STATIC)
	SET(FTE_LIBS ${FTE_LIBS} ${JPEG_LIBRARIES})
ELSE()
	MESSAGE(WARNING "libjpeg library NOT available. Who cares?")
	SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};NO_JPEG)
	SET(JPEG_LIBRARIES)
ENDIF()

if(NOT ANDROID)
IF(NOT ${WIN32})
	SET(FTE_DEP_DBUS true CACHE BOOL "Link against libdbus.")
	IF(FTE_DEP_DBUS)
		FIND_PACKAGE(DBus1)
	ENDIF()
	IF(DBUS1_FOUND)
		INCLUDE_DIRECTORIES( ${DBus1_INCLUDE_DIRS} )
		SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};HAVE_DBUS)
		SET(FTE_LIBS ${FTE_LIBS} ${DBus1_LIBRARIES})
	ELSE()
		MESSAGE(WARNING "libdbus-1 library NOT available. Who cares?")
	ENDIF()
ENDIF()
endif()

SET(FTE_DEP_PNG true CACHE BOOL "Link against libpng.")
IF(FTE_DEP_PNG)
if(ANDROID)
    set(PNG_FOUND TRUE)
    set(PNG_INCLUDE_DIRS ${FTEQW_DEPLIBS}/libpng ${CMAKE_BINARY_DIR}/${FTEQW_DEPLIBS_NAME}/libpng)
    set(PNG_LIBRARIES png)
else()
	FIND_PACKAGE(PNG)
endif()
ENDIF()
IF(PNG_FOUND)
	INCLUDE_DIRECTORIES( ${PNG_INCLUDE_DIRS} )
	SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};LIBPNG_STATIC)
	SET(FTE_LIBS ${FTE_LIBS} ${PNG_LIBRARIES})
ELSE()
	MESSAGE(WARNING "libpng library NOT available. Good luck with screenshots.")
	SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};NO_PNG)
	SET(PNG_LIBRARIES)
ENDIF()

SET(FTE_DEP_FREETYPE true CACHE BOOL "Link against libfreetype.")
IF(FTE_DEP_FREETYPE)
if(ANDROID)
    set(FREETYPE_FOUND TRUE)
    set(FREETYPE_INCLUDE_DIRS ${FTEQW_DEPLIBS}/freetype/include)
    set(FREETYPE_LIBRARIES freetype)
else()
	FIND_PACKAGE(Freetype)
endif()
ENDIF()
IF(FREETYPE_FOUND)
	INCLUDE_DIRECTORIES( ${FREETYPE_INCLUDE_DIRS} )
	SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};LIBFREETYPE_STATIC)
	SET(FTE_LIBS ${FTE_LIBS} ${FREETYPE_LIBRARIES})

	FIND_PACKAGE(Fontconfig)
	IF(Fontconfig_FOUND)
		INCLUDE_DIRECTORIES( ${Fontconfig_INCLUDE_DIRS} )
		SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};LIBFONTCONFIG_STATIC)
		SET(FTE_LIBS ${FTE_LIBS} ${Fontconfig_LIBRARIES})
	ELSEIF()
		MESSAGE(WARNING "fontconfig library NOT available. I hope you're not using any system fonts.")
	ENDIF()
ELSE()
	MESSAGE(WARNING "freetype library NOT available. I hope you're okay with ascii.")
	SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};NO_FREETYPE)
ENDIF()

#this is just for headers.
FIND_PATH(VULKAN_INCLUDE_DIR vulkan/vulkan.h)
IF(VULKAN_INCLUDE_DIR)
	INCLUDE_DIRECTORIES( ${VULKAN_INCLUDE_DIR} )
	SET(FTE_DEFINES ${FTE_DEFINES};VKQUAKE) #no libs required, thankfully
ELSE()
	MESSAGE(WARNING "Vulkan headers NOT available.")
ENDIF()

SET(FTE_DEP_VORBISFILE true CACHE BOOL "Link against libvorbisfile.")
IF(FTE_DEP_VORBISFILE)
if(ANDROID)
    set(VORBISFILE_INCLUDE_DIRS ${FTEQW_DEPLIBS}/libvorbis/include)
    set(VORBISFILE_LIBRARY vorbisfile)
	INCLUDE_DIRECTORIES( ${VORBISFILE_INCLUDE_DIRS} )
	SET(FTE_LIBS ${FTE_LIBS} ${VORBISFILE_LIBRARY})

	INCLUDE_DIRECTORIES( ${FTEQW_DEPLIBS}/libogg/include )
	SET(FTE_LIBS ${FTE_LIBS} ogg)
else()
	FIND_LIBRARY(VORBISFILE_LIBRARY NAMES vorbisfile)
endif()
ENDIF()
IF(NOT VORBISFILE_LIBRARY)
	MESSAGE(WARNING "libvorbisfile library NOT available. Who listens to the bgm anyway?")
	SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};NO_OGG)
ENDIF()

IF(CMAKE_BUILD_TYPE MATCHES "Debug")
	SET(FTE_WERROR true CACHE BOOL "Warnings as errors.")
ELSE()
	SET(FTE_WERROR false CACHE BOOL "Warnings as errors.")
ENDIF()
IF(FTE_WERROR)
	SET(FTE_WERROR_ARG "-Werror")
ELSE()
	SET(FTE_WERROR_ARG "")
ENDIF()

IF(CMAKE_C_COMPILER_ID MATCHES "Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-pointer-sign")

	IF(CMAKE_BUILD_TYPE MATCHES "Debug")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ${FTE_WERROR_ARG} -Wno-pointer-sign -Wno-unknown-pragmas -Wno-format-zero-length -Wno-strict-aliasing -Wno-error=cpp")
	ELSE()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 ${FTE_WERROR_ARG}")
	ENDIF()
endif()
IF(CMAKE_C_COMPILER_ID MATCHES "GNU")
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wstrict-prototypes")			#
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wold-style-definition")		#k&r c is weird and can't cope with 64bit types.
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-parameter-type")		#k&r c is weird and can't cope with 64bit types.
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wold-style-declaration")		#
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wpointer-arith")				#void* stuff
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wvla")							#msvc doesn't support vla
	SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wdeclaration-after-statement")	#msvc doesn't allow defs after statements, and they're so very tempting...
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wformat-truncation=1")
	#TODO SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wmissing-prototypes")			#for finding missing statics.
	#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-function")			#
#	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wswitch-enum")					#to warn about omitted enums despite default.

	#might as well do this, public builds use the regular Makefile.
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
	IF(CMAKE_BUILD_TYPE MATCHES "Debug")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall ${FTE_WERROR_} -Wno-pointer-sign -Wno-unknown-pragmas -Wno-format-zero-length -Wno-strict-aliasing -Wno-error=cpp")
	ELSE()
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 ${FTE_WERROR_}")
	ENDIF()
	IF (NOT FTE_USE_SDL)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wl,--warn-common")
	ENDIF()
	#SET(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wc++-compat")	#lul, thousands of errors!
ENDIF()
IF(CMAKE_BUILD_TYPE MATCHES "Debug")
	IF(NOT ${WIN32})
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstack-protector-strong")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -std=gnu89")
	ENDIF()
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_DEBUG")
ENDIF()
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_FILE_OFFSET_BITS=64")

if(NOT ANDROID)
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DFTE_LIBRARY_PATH=${CMAKE_INSTALL_FULL_LIBDIR}/${FTE_INSTALL_LIBDIR} -DFTE_DATA_DIR=${CMAKE_INSTALL_FULL_DATAROOTDIR}")
endif()

FUNCTION(EMBED_PLUGIN_META PLUGNAME PLUGTITLE PLUGDESC)
	SET_TARGET_PROPERTIES(plug_${PLUGNAME} PROPERTIES OUTPUT_NAME "${PLUGNAME}")
	SET_TARGET_PROPERTIES(plug_${PLUGNAME} PROPERTIES PREFIX "fteplug_")
	SET_TARGET_PROPERTIES(plug_${PLUGNAME} PROPERTIES LINK_FLAGS "-Wl,--no-undefined")
	SET(INSTALLTARGS ${INSTALLTARGS} "plug_${PLUGNAME}" PARENT_SCOPE)
	#sadly we need to use a temp zip file, because otherwise zip insists on using zip64 extensions which breaks zip -A (as well as any attempts to read any files).
	if(NOT ANDROID)
	ADD_CUSTOM_COMMAND(
		TARGET plug_${PLUGNAME} POST_BUILD
		COMMAND /bin/echo -e "{\\n	package fteplug_${PLUGNAME}\\n	ver \"${SVNREVISION}\"\\n	category Plugins\\n	title \"${PLUGTITLE}\"\\n	gamedir \"\"\\n	desc \"${PLUGDESC}\"\\n}" | zip -q -9 -fz- $<TARGET_FILE:plug_${PLUGNAME}>.zip -
		COMMAND cmake -E cat $<TARGET_FILE:plug_${PLUGNAME}>.zip >> "$<TARGET_FILE:plug_${PLUGNAME}>"
		COMMAND zip -A "$<TARGET_FILE:plug_${PLUGNAME}>"
		COMMAND cmake -E rm $<TARGET_FILE:plug_${PLUGNAME}>.zip
		VERBATIM)
    endif()
ENDFUNCTION()

SET(FTE_DEP_GNUTLS true CACHE BOOL "Link against gnutls")
IF(FTE_DEP_GNUTLS)
	FIND_PACKAGE(GnuTLS)
	IF(NOT GNUTLS_FOUND)
		MESSAGE(WARNING "gnutls library NOT available. HTTPS/DTLS will not be available.")
		SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};NO_GNUTLS)
	ELSE()
		IF(WIN32)
			SET(GNUTLS_STATIC true CACHE BOOL "Link gnutls statically.") #usually as an .so though. :/
		ELSE()
			SET(GNUTLS_STATIC false CACHE BOOL "Link gnutls statically.") #usually as an .so though. :/
		ENDIF()
		IF(GNUTLS_STATIC)
			SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};GNUTLS_STATIC)
			SET(FTE_LIBS ${FTE_LIBS} ${GNUTLS_LIBRARY})
			SET(FTESV_LIBS ${FTESV_LIBS} ${GNUTLS_LIBRARY})
		ENDIF()
	ENDIF()
ENDIF()
IF(WIN32)
	SET(FTE_DEP_WINSSPI true CACHE BOOL "Link against winsspi(schannel)")
	IF(NOT FTE_DEP_WINSSPI)
		SET(FTE_DEFINES ${FTE_DEFINES};NO_WINSSPI)
	ENDIF()
ENDIF()

IF(${ANDROID})
#	FIND_PACKAGE(Freetype REQUIRED)

#	INCLUDE_DIRECTORIES(	${FREETYPE_INCLUDE_DIRS} )

	SET(FTE_DEFINES ${FTE_DEFINES};ANDROID;VKQUAKE;MULTITHREAD)
	SET(FTE_LIBS ${FTE_LIBS} android log EGL ${SYS_LIBS} ${CMAKE_DL_LIBS})
	SET(FTE_ARCH_FILES
		engine/client/sys_android.c
		engine/common/sys_linux_threads.c
		engine/client/cd_null.c
		engine/gl/gl_vidandroid.c
		engine/client/snd_al.c
	)
	# engine/client/snd_android.c
ELSEIF(WIN32 AND NOT FTE_USE_SDL)
	INCLUDE_DIRECTORIES(engine/libs engine/libs/freetype2/include)
#	LINK_DIRECTORIES(engine/libs/mingw64-libs)
	
	#	engine/server/sv_sys_win.c
	
	SET(FTE_LIBS ${FTE_LIBS} ole32 gdi32 wsock32 winmm dxguid)
	SET(FTE_DEFINES ${FTE_DEFINES};D3D9QUAKE;D3D11QUAKE)
	SET(FTE_ARCH_FILES 
		engine/client/winquake.rc
		engine/common/sys_win_threads.c
		engine/common/net_ssl_winsspi.c
		engine/common/net_ssl_gnutls.c
		engine/common/fs_win32.c
		engine/client/cd_win.c
		engine/client/in_win.c
		engine/client/snd_al.c
		engine/client/snd_directx.c
		engine/client/snd_wasapi.c
		engine/client/snd_win.c
		engine/client/snd_xaudio.c
		engine/client/sys_win.c
		
		engine/gl/gl_vidnt.c

		engine/d3d/d3d_backend.c
		engine/d3d/d3d_image.c
		engine/d3d/d3d_shader.c
		engine/d3d/d3d11_backend.c
		engine/d3d/d3d11_image.c
		engine/d3d/d3d11_shader.c
		engine/d3d/d3d8_backend.c
		engine/d3d/d3d8_image.c
		engine/d3d/vid_d3d.c
		engine/d3d/vid_d3d11.c
		engine/d3d/vid_d3d8.c
	)
	
	SET(FTESV_LIBS ${FTESV_LIBS} wsock32 winmm)
	SET(FTESV_ARCH_FILES 
		engine/client/winquake.rc
		engine/common/sys_win_threads.c
		engine/common/net_ssl_winsspi.c
		engine/common/net_ssl_gnutls.c
		engine/common/fs_win32.c
		engine/server/sv_sys_win.c
	)
ELSEIF(UNIX AND NOT FTE_USE_SDL)	#linux(ish)
				#openbsd will have issues with snd_linux.c

	#linux-only packages
	FIND_PACKAGE(ALSA)
	IF(ALSA_FOUND)
		SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};AUDIO_ALSA;AUDIO_PULSE)
	ELSE()
		MESSAGE(WARNING "asound (alsa) library NOT available.")
	ENDIF()

	FIND_PACKAGE(X11)
	IF(X11_FOUND)
		IF (NOT X11_Xcursor_FOUND)
			SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};NO_X11_CURSOR)
			MESSAGE(WARNING "Xcursor library NOT available.")
		ENDIF()
		IF (NOT X11_Xrandr_FOUND)
			SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};NO_X11_RANDR)
			MESSAGE(WARNING "Xrandr library NOT available.")
		ENDIF()
		IF (NOT X11_Xscreensaver_FOUND)
			SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};NO_X11_XSS)
			MESSAGE(WARNING "Xss library NOT available.")
		ENDIF()
	ELSE()
		MESSAGE(WARNING "x11 library NOT available.")
		SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};NO_X11)
	ENDIF()

	SET(FTE_DEFINES ${FTE_DEFINES};DYNAMIC_SDL;MULTITHREAD)
	SET(FTE_LIBS ${FTE_LIBS} ${SYS_LIBS} ${CMAKE_DL_LIBS} pthread ${SDL2_LIBRARIES})
	SET(FTE_ARCH_FILES ${FTE_ARCH_FILES}
		engine/client/sys_linux.c
		engine/common/sys_linux_threads.c
		engine/common/net_ssl_gnutls.c

		engine/client/snd_al.c
		engine/client/snd_alsa.c
		engine/client/snd_linux.c
		engine/client/snd_pulse.c
		engine/client/snd_sdl.c	#we use SDL audio even without sys_sdl, because of pulseaudio fucking over alsa, alsa fucking over oss3, and oss4 not being used. Either way, openal should be the default anyway.

		engine/client/cd_linux.c
		engine/gl/gl_vidlinuxglx.c
		engine/gl/gl_videgl.c

#		engine/gl/gl_vidrpi.c
#		engine/gl/gl_vidwayland.c
	)

	#openbsd uses a libossaudio library for all the oss stuff, use that to ensure that we still get sound
	FIND_LIBRARY(
		OSSAUDIO_LIBRARY
		NAMES ossaudio
	)
	IF(OSSAUDIO_LIBRARY)
		SET(FTE_LIBS ${FTE_LIBS} ${OSSAUDIO_LIBRARY})
	ENDIF()

	#on linux, use wayland (we normally dynamically link, but we still need headers).
	FIND_LIBRARY(
		WAYLAND_CLIENT_LIBRARY
		NAMES wayland-client libwayland-client
	)
	FIND_LIBRARY(
		WAYLAND_EGL
		NAMES wayland-egl
	)
	FIND_LIBRARY(
		HAVE_XKBCOMMON
		NAMES xkbcommon
	)
	IF(NOT HAVE_XKBCOMMON)
		MESSAGE(WARNING "xkbcommon library not found, needed for wayland to be usable.")
		UNSET(WAYLAND_CLIENT_LIBRARY)
	ENDIF()
	IF(WAYLAND_CLIENT_LIBRARY AND WAYLAND_EGL)
#		SET(FTE_LIBS ${FTE_LIBS} ${WAYLAND_CLIENT_LIBRARY} ${HAVE_XKBCOMMON} ${WAYLAND_EGL})
#		SET(FTE_DEFINES ${FTE_DEFINES};STATIC_WAYLAND)

		SET(FTE_DEFINES ${FTE_DEFINES};WAYLANDQUAKE;USE_EGL)
		SET(FTE_ARCH_FILES ${FTE_ARCH_FILES}
			engine/gl/gl_vidwayland.c
		)
	ELSE()
		MESSAGE(WARNING "Wayland library NOT available. X11 will live forever anyway.")
		IF(NOT X11_FOUND)
			MESSAGE(WARNING "No renderers supported!")
			SET(FTE_NO_RENDERERS 1)
		ENDIF()
	ENDIF()

	IF(CMAKE_SYSTEM_NAME MATCHES "BSD" OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
		FIND_LIBRARY(epoll-shim REQUIRED)
		INCLUDE_DIRECTORIES(${EPOLL_INC_DIR})
		SET(FTESV_LIBS ${FTESV_LIBS} epoll-shim)
		SET(FTESV_DEFINES ${FTESV_DEFINES};HAVE_EPOLL)
	ENDIF()

	SET(FTESV_DEFINES ${FTESV_DEFINES};MULTITHREAD)
	SET(FTESV_ARCH_FILES ${FTESV_ARCH_FILES}
		engine/server/sv_sys_unix.c
		engine/common/sys_linux_threads.c
		engine/common/net_ssl_gnutls.c
	)
	SET(FTESV_LIBS ${FTESV_LIBS} ${SYS_LIBS} ${CMAKE_DL_LIBS} pthread)

ELSEIF(1)	#SDL
#	FIND_PACKAGE(Freetype REQUIRED)
#	INCLUDE_DIRECTORIES(engine/libs engine/libs/freetype2/include)

	FIND_PACKAGE(PkgConfig REQUIRED)
	PKG_SEARCH_MODULE(sdl2 REQUIRED sdl2)

	FIND_PACKAGE(SDL2 REQUIRED)

	INCLUDE_DIRECTORIES(${FREETYPE_INCLUDE_DIRS} ${SDL2_INCLUDE_DIRS})
	
	#SDL2.0.7 supports vulkan, so lets use it.
	SET(FTE_DEFINES ${FTE_DEFINES};FTE_SDL;MULTITHREAD)
	SET(FTE_LIBS ${FTE_LIBS} ${SYS_LIBS} ${CMAKE_DL_LIBS} ${SDL2_LIBRARIES})
	SET(FTE_ARCH_FILES
		engine/client/sys_sdl.c
		engine/client/snd_al.c
		engine/client/snd_sdl.c
		engine/client/in_sdl.c
		engine/client/cd_sdl.c
		engine/gl/gl_vidsdl.c
	)

	SET(FTESV_DEFINES ${FTESV_DEFINES};MULTITHREAD)
	SET(FTESV_LIBS ${FTESV_LIBS} ${SYS_LIBS} ${CMAKE_DL_LIBS} ${SDL2_LIBRARIES})

	IF(WIN32)
		SET(FTE_LIBS ${FTE_LIBS} wsock32 gdi32 ole32)
		SET(FTE_DEFINES ${FTE_DEFINES};NO_DIRECTX)
		SET(FTE_ARCH_FILES ${FTE_ARCH_FILES}
			engine/client/winquake.rc
			engine/common/net_ssl_winsspi.c
		)
		SET(FTESV_ARCH_FILES ${FTESV_ARCH_FILES}
			engine/client/winquake.rc
			engine/common/net_ssl_winsspi.c
			engine/server/sv_sys_win.c
		)
	ELSE()
		SET(FTE_ARCH_FILES ${FTE_ARCH_FILES}
			engine/common/net_ssl_gnutls.c
		)
		SET(FTESV_ARCH_FILES ${FTESV_ARCH_FILES}
			engine/common/net_ssl_gnutls.c
			engine/common/sys_linux_threads.c
			engine/server/sv_sys_unix.c
		)
		SET(FTESV_LIBS ${FTESV_LIBS} pthread)
	ENDIF()
ELSE()
#	engine/common/sys_linux_threads.c
#	engine/common/net_ssl_gnutls.c
#	engine/server/sv_sys_unix.c

#	engine/client/snd_alsa.c
#	engine/client/snd_droid.c
#	engine/client/snd_linux.c
#	engine/client/snd_macos.c
#	engine/client/snd_morphos.c
#	engine/client/snd_sblaster.c
#	engine/client/snd_sdl.c
#	engine/client/snd_sndio.c

#	engine/client/sys_dos.c
#	engine/client/sys_droid.c
#	engine/client/sys_linux.c
#	engine/client/sys_morphos.c
#	engine/client/sys_plugfte.c
#	engine/client/sys_sdl.c
#	engine/client/sys_xdk.c

#	engine/client/cd_linux.c
#	engine/client/cd_null.c
#	engine/client/cd_sdl.c
#	engine/client/in_morphos.c
#	engine/client/in_sdl.c
	
#	engine/gl/gl_viddroid.c
#	engine/gl/gl_videgl.c
#	engine/gl/gl_vidlinuxglx.c
#	engine/gl/gl_vidmacos.c
#	engine/gl/gl_vidmorphos.c
#	engine/gl/gl_vidnull.c
#	engine/gl/gl_vidrpi.c
#	engine/gl/gl_vidsdl.c
#	engine/gl/gl_vidtinyglstubs.c
#	engine/gl/gl_vidwayland.c
ENDIF()

SET(FTE_GL_FILES
#These are GL-specific, but can be left even if no gl is supported.
	engine/gl/gl_backend.c
	engine/gl/gl_bloom.c
	engine/gl/gl_draw.c
	engine/gl/gl_rmain.c
	engine/gl/gl_rmisc.c
	engine/gl/gl_rsurf.c
	engine/gl/gl_screen.c
	engine/gl/gl_vidcommon.c
	engine/gl/glmod_doom.c
)
SET(FTE_VK_FILES
	engine/vk/vk_backend.c
	engine/vk/vk_init.c
)
SET(FTE_QCVM_FILES
	engine/qclib/comprout.c
	engine/qclib/initlib.c
	engine/qclib/pr_edict.c
	engine/qclib/pr_exec.c
	engine/qclib/pr_multi.c
	engine/qclib/qcc_cmdlib.c
	engine/qclib/qcc_pr_comp.c
	engine/qclib/qcc_pr_lex.c
#	engine/qclib/decomp.c
#	engine/qclib/packager.c
#	engine/qclib/pr_x86.c
#	engine/qclib/qccgui.c
#	engine/qclib/qccguistuff.c
#	engine/qclib/qcctui.c
	engine/qclib/qccmain.c
	engine/qclib/qcd_main.c
	engine/qclib/qcdecomp.c
)
SET(FTE_COMMON_FILES
	#these files are common to both server-only and client+server builds.
	engine/common/cmd.c
	engine/common/com_mesh.c
	engine/common/com_bih.c
	engine/common/common.c
	engine/common/json.c
	engine/common/crc.c
	engine/common/cvar.c
	engine/common/fs.c
	engine/common/fs_dzip.c
	engine/common/fs_pak.c
	engine/common/fs_stdio.c
	engine/common/fs_xz.c
	engine/common/fs_zip.c
	engine/common/gl_q2bsp.c
	engine/common/huff.c
	engine/common/log.c
	engine/common/mathlib.c
	engine/common/md4.c
	engine/common/md5.c
	engine/common/net_chan.c
	engine/common/net_ice.c
	engine/common/net_wins.c
	engine/common/plugin.c
	engine/common/pmove.c
	engine/common/pmovetst.c
	engine/common/pr_bgcmd.c
	engine/common/q1bsp.c
	engine/common/q2pmove.c
	engine/common/qvm.c
	engine/common/sha1.c
	engine/common/sha2.c
	engine/common/translate.c
	engine/common/zone.c

	#important headers
	engine/common/bothdefs.h
	engine/common/config_fteqw.h
	engine/common/config_minimal.h
	engine/common/config_nocompat.h
	engine/common/config_wastes.h
	engine/common/config_freecs.h
	engine/common/config_fteqw_noweb.h

	#useless headers that I'll never search for
	engine/client/api_menu.h
	engine/client/cdaudio.h
	engine/client/client.h
	engine/client/cl_ignore.h
	engine/client/cl_master.h
	engine/client/input.h
	engine/client/keys.h
	engine/client/menu.h
	engine/client/merged.h
	engine/client/modelgen.h
	engine/client/quakedef.h
	engine/client/render.h
	engine/client/sbar.h
	engine/client/screen.h
	engine/client/sound.h
	engine/client/spritegn.h
#	engine/client/sys_plugfte.h
	engine/client/vid.h
	engine/client/view.h
	engine/client/wad.h
#	engine/client/winquake.h
	engine/common/bothdefs.h
	engine/common/bspfile.h
	engine/common/cmd.h
	engine/common/com_mesh.h
	engine/common/common.h
	engine/common/console.h
	engine/common/cvar.h
	engine/common/fs.h
	engine/common/mathlib.h
	engine/common/net.h
	engine/common/netinc.h
	engine/common/particles.h
	engine/common/pmove.h
	engine/common/pr_common.h
	engine/common/protocol.h
	engine/common/sys.h
	engine/common/translate.h
	engine/common/ui_public.h
	engine/common/vm.h
	engine/common/world.h
	engine/common/zone.h
	engine/gl/gl_draw.h
	engine/gl/gl_model.h
	engine/gl/glquake.h
	engine/gl/glsupp.h
	engine/gl/gl_terrain.h
	engine/gl/gl_videgl.h
	engine/gl/model_hl.h
	engine/gl/shader.h
	engine/http/iweb.h
	engine/qclib/cmdlib.h
	engine/qclib/execloop.h
	engine/qclib/gui.h
	engine/qclib/hash.h
	engine/qclib/pr_comp.h
	engine/qclib/progsint.h
	engine/qclib/progslib.h
	engine/qclib/progtype.h
	engine/qclib/qcc.h
	engine/qclib/qcd.h
	engine/server/progdefs.h
	engine/server/progs.h
	engine/server/q2game.h
	engine/server/server.h
	#engine/server/svhl_gcapi.h
	engine/server/sv_sql.h
	#engine/sw/sw.h
	#engine/sw/sw_spans.h
	engine/vk/vkrenderer.h
	engine/web/ftejslib.h


	#sigh
	engine/client/pr_skelobj.c
	engine/client/m_download.c
	engine/client/net_master.c
	engine/client/r_d3.c
	
	#these are here because of hitmodel etc
	engine/gl/gl_heightmap.c
	engine/gl/gl_hlmdl.c
	engine/gl/gl_model.c
	
	engine/server/sv_move.c
	engine/server/sv_phys.c
	engine/server/world.c
	
	${FTE_QCVM_FILES}
	engine/qclib/hash.c
	
	engine/http/httpclient.c
)

SET(FTE_SERVER_FILES
	engine/server/net_preparse.c
	engine/server/pr_cmds.c
	engine/server/pr_lua.c
	engine/server/pr_q1qvm.c
	engine/server/savegame.c
	engine/server/sv_ccmds.c
	engine/server/sv_chat.c
	engine/server/sv_cluster.c
	engine/server/sv_demo.c
	engine/server/sv_ents.c
	engine/server/sv_init.c
	engine/server/sv_main.c
	engine/server/sv_master.c
	engine/server/sv_mvd.c
	engine/server/sv_nchan.c
	engine/server/sv_rankin.c
	engine/server/sv_send.c
	engine/server/sv_sql.c
	engine/server/sv_user.c
#	engine/server/svhl_game.c
#	engine/server/svhl_phys.c
#	engine/server/svhl_world.c
	engine/server/svq2_ents.c
	engine/server/svq2_game.c
)

#these files are only in the client
SET(FTE_CLIENT_FILES
	engine/client/cl_cam.c
	engine/client/cl_demo.c
	engine/client/cl_ents.c
	engine/client/cl_ignore.c
	engine/client/cl_input.c
	engine/client/cl_main.c
	engine/client/cl_parse.c
	engine/client/cl_pred.c
	engine/client/cl_screen.c
	engine/client/cl_tent.c
#	engine/client/clhl_game.c
	engine/client/clq2_cin.c
	engine/client/clq2_ents.c
	engine/client/console.c
	engine/client/fragstats.c
	engine/client/image.c
	engine/client/in_generic.c
	engine/client/keys.c
	engine/client/m_items.c
	engine/client/m_master.c
	engine/client/m_mp3.c
	engine/client/m_multi.c
	engine/client/m_options.c
	engine/client/m_script.c
	engine/client/m_native.c
	engine/client/m_single.c
	engine/client/menu.c
	engine/client/p_classic.c
	engine/client/p_null.c
	engine/client/p_script.c
	engine/client/pr_clcmd.c
	engine/client/pr_csqc.c
	engine/client/pr_menu.c
	engine/client/r_2d.c
	engine/client/r_d3.c
	engine/client/r_part.c
	engine/client/r_partset.c
	engine/client/r_surf.c
	engine/client/renderer.c
	engine/client/renderque.c
	engine/client/roq_read.c
	engine/client/sbar.c
	engine/client/skin.c
	engine/client/snd_dma.c
	engine/client/snd_mem.c
	engine/client/snd_mix.c
	engine/client/snd_mp3.c
	engine/client/snd_ov.c
	engine/client/textedit.c
	engine/client/valid.c
	engine/client/view.c
	engine/client/wad.c
	engine/client/zqtp.c


#These are generic renderer files and no longer gl-specific (for the most part)
	engine/gl/gl_alias.c
	engine/gl/gl_font.c
	engine/gl/gl_ngraph.c
	engine/gl/gl_rlight.c
	engine/gl/gl_shader.c
	engine/gl/gl_shadow.c
	engine/gl/gl_warp.c
	engine/gl/ltface.c

#these are renderer-specific
	engine/client/vid_headless.c
	${FTE_GL_FILES}
	${FTE_VK_FILES}
)

SET(FTE_Q3_FILES
	plugins/quake3/botlib/be_aas_bspq3.c
	plugins/quake3/botlib/be_aas_entity.c
	plugins/quake3/botlib/be_aas_move.c
	plugins/quake3/botlib/be_aas_routealt.c
	plugins/quake3/botlib/be_ai_char.c
	plugins/quake3/botlib/be_ai_goal.c
	plugins/quake3/botlib/be_ai_weight.c
	plugins/quake3/botlib/l_crc.c
	plugins/quake3/botlib/l_memory.c
	plugins/quake3/botlib/l_struct.c
	plugins/quake3/botlib/be_aas_cluster.c
	plugins/quake3/botlib/be_aas_file.c
	plugins/quake3/botlib/be_aas_optimize.c
	plugins/quake3/botlib/be_aas_route.c
	plugins/quake3/botlib/be_ai_chat.c
	plugins/quake3/botlib/be_ai_move.c
	plugins/quake3/botlib/be_ea.c
	plugins/quake3/botlib/l_libvar.c
	plugins/quake3/botlib/l_precomp.c
	plugins/quake3/botlib/be_aas_debug.c
	plugins/quake3/botlib/be_aas_main.c
	plugins/quake3/botlib/be_aas_reach.c
	plugins/quake3/botlib/be_aas_sample.c
	plugins/quake3/botlib/be_ai_gen.c
	plugins/quake3/botlib/be_ai_weap.c
	plugins/quake3/botlib/be_interface.c
	plugins/quake3/botlib/l_log.c
	plugins/quake3/botlib/l_script.c
	plugins/quake3/botlib/standalone.c

	plugins/quake3/clq3_cg.c
	plugins/quake3/clq3_ui.c
	plugins/quake3/clq3_parse.c
	plugins/quake3/svq3_game.c
	plugins/quake3/q3common.c
	plugins/quake3/q3common.h

	plugins/quake3/clq3defs.h
	plugins/quake3/q3g_public.h
)

#For annoying compressed gltf2 files.
SET(FTE_DEP_DRACO false CACHE BOOL "Link against libdraco (apache2).")
IF(FTE_DEP_DRACO)
	FIND_LIBRARY(
		DRACO_LIBRARY
		NAMES draco
	)
	IF(DRACO_LIBRARY)
		SET(DRACO_FILES plugins/models/draco.cpp)
		SET(DRACO_CFLAGS HAVE_DRACO)

		SET(FTE_COMMON_FILES ${FTE_COMMON_FILES} ${DRACO_FILES})
		SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES};${DRACO_CFLAGS})
		SET(FTE_LIBS ${FTE_LIBS} ${DRACO_LIBRARY})
		SET(FTESV_LIBS ${FTESV_LIBS} ${DRACO_LIBRARY})
	ELSE()
		MESSAGE(WARNING "draco library not found, needed for GLTF's KHR_draco_mesh_compression to be usable.")
	ENDIF()
ENDIF()


SET(FTE_PLUG_QUAKE3 true CACHE BOOL "Compile Quake3 plugin.")
IF(FTE_PLUG_QUAKE3)
	IF (0)
		SET(FTE_DEFINES ${FTE_DEFINES};${Q3_DEFINES})
		SET(FTE_LIBS ${FTE_LIBS} quake3)

		#define the modules and make sure they're linked (one generic, one for server-only builds.
		ADD_LIBRARY(quake3 STATIC ${FTE_Q3_FILES})
		SET_TARGET_PROPERTIES(quake3 PROPERTIES COMPILE_DEFINITIONS "${FTE_LIB_DEFINES};${FTE_REVISON};BOTLIB;BOTLIB_STATIC;FTEPLUGIN;STATIC_Q3")
		TARGET_LINK_LIBRARIES(quake3 ${SYS_LIBS})

		#ADD_LIBRARY(q3sv STATIC EXCLUDE_FROM_ALL ${FTE_Q3_FILES})
		#SET_TARGET_PROPERTIES(q3sv PROPERTIES COMPILE_DEFINITIONS "${FTE_LIB_DEFINES};${FTE_REVISON};BOTLIB;BOTLIB_STATIC;SERVERONLY")
		#SET_TARGET_PROPERTIES(q3sv PROPERTIES LINK_FLAGS "-Wl,--no-undefined")
		#TARGET_LINK_LIBRARIES(q3sv ${SYS_LIBS})
		#SET(FTESV_LIBS ${FTESV_LIBS} q3sv)
	ELSE()
		#define the modules and make sure they're linked (one generic, one for server-only builds.
		ADD_LIBRARY(plug_quake3 MODULE ${FTE_Q3_FILES} plugins/plugin.c)
		SET_TARGET_PROPERTIES(plug_quake3 PROPERTIES COMPILE_DEFINITIONS "${FTE_LIB_DEFINES};${FTE_REVISON};BOTLIB;BOTLIB_STATIC;FTEPLUGIN")
		TARGET_LINK_LIBRARIES(plug_quake3 ${SYS_LIBS})
		EMBED_PLUGIN_META(quake3 "Quake3 Compat" "Provides compatability with Quake3's gamecode.")
	ENDIF()
ENDIF()

#still a wip, so disabled by default
SET(FTE_PLUG_COD true CACHE BOOL "Compile Call of Duty plugin.")
IF(FTE_PLUG_COD)
	ADD_LIBRARY(plug_cod MODULE
		plugins/cod/codmod.c
		plugins/cod/codbsp.c
		plugins/cod/codmat.c
		plugins/cod/codiwi.c
		#plugins/cod/codff.c
		plugins/plugin.c)
	SET_TARGET_PROPERTIES(plug_cod PROPERTIES COMPILE_DEFINITIONS "${FTE_LIB_DEFINES};${FTE_REVISON};FTEPLUGIN")
	TARGET_LINK_LIBRARIES(plug_cod ${SYS_LIBS}
		#${ZLIB_LIBRARIES}
		)
	EMBED_PLUGIN_META(cod "CoD Formats" "Provides compatability with Call Of Duty's file formats.")
ENDIF()

FILE(STRINGS "${FTE_BUILD_CONFIG}" BULLET_INTERNAL REGEX "^#define[\t ]+USE_INTERNAL_BULLET")
IF(BULLET_INTERNAL)
	#Built-in bullet physics plugin...
	FIND_PACKAGE(Bullet REQUIRED)
	SET(FTE_COMMON_FILES ${FTE_COMMON_FILES} plugins/bullet/bulletplug.cpp)
	INCLUDE_DIRECTORIES( ${BULLET_INCLUDE_DIRS} )
	SET(FTE_LIBS ${FTE_LIBS} ${BULLET_LIBRARIES})
	SET(FTESV_LIBS ${FTESV_LIBS} ${BULLET_LIBRARIES})
ELSE()
	#Bullet Physics library plugin
	SET(FTE_PLUG_BULLET true CACHE BOOL "Compile bullet rigid body physics plugin.")
	IF(FTE_PLUG_BULLET)
		FIND_PACKAGE(Bullet)
		IF (BULLET_FOUND)
			ADD_LIBRARY(plug_bullet MODULE
				plugins/plugin.c
				plugins/bullet/bulletplug.cpp
			)
			TARGET_INCLUDE_DIRECTORIES(plug_bullet PUBLIC ${BULLET_INCLUDE_DIRS})
			SET_TARGET_PROPERTIES(plug_bullet PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES}")
			TARGET_LINK_LIBRARIES(plug_bullet ${SYS_LIBS} ${BULLET_LIBRARIES})

			EMBED_PLUGIN_META(bullet "Bullet Physics Plugin" "Provides Rigid Body Physics.")
		ELSE()
			MESSAGE(WARNING "bullet library not detected, skipping plugin")
		ENDIF()
	ENDIF()
ENDIF()

#ODE Physics library plugin
SET(FTE_PLUG_ODE true CACHE BOOL "Compile ODE rigid body physics plugin.")
SET_PROPERTY(CACHE FTE_PLUG_ODE PROPERTY STRINGS false true static)
IF(FTE_PLUG_ODE)
	FIND_PATH(LIBODE_INCLUDE_DIR ode/ode.h)
	IF (LIBODE_INCLUDE_DIR)
		FIND_LIBRARY(LIBODE_LIBRARY ode)
	ENDIF()
	IF (LIBODE_LIBRARY)
		IF (FTE_PLUG_ODE STREQUAL "static")
			#SET (FTE_COMMON_FILES ${FTE_COMMON_FILES} engine/common/com_phys_ode.c)
			SET(FTE_LIB_DEFINES "${FTE_LIB_DEFINES};USE_INTERNAL_ODE;ODE_STATIC")
			SET(FTE_LIBS ${FTE_LIBS} ${LIBODE_LIBRARY})
			SET(FTESV_LIBS ${FTESV_LIBS} ${LIBODE_LIBRARY})
			SET(FTE_INCLUDES ${FTE_INCLUDES} ${ODE_INCLUDE_DIRS})
		ELSE()
			ADD_LIBRARY(plug_ode MODULE
				plugins/plugin.c
				engine/common/com_phys_ode.c
				engine/common/mathlib.c
			)
			TARGET_INCLUDE_DIRECTORIES(plug_ode PUBLIC ${ODE_INCLUDE_DIRS})
			SET_TARGET_PROPERTIES(plug_ode PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;ODE_STATIC")
			TARGET_LINK_LIBRARIES(plug_ode ${SYS_LIBS} ${LIBODE_LIBRARY})

			EMBED_PLUGIN_META(ode "ODE Physics" "Provides Rigid Body Physics behaviours.")
		ENDIF()
	ELSE()
		MESSAGE(WARNING "ODE library not found, no ode plugin for you")
	ENDIF()
ENDIF()

IF(ANDROID)
	SET(FTE_ENGINE fteqw)
	#android sucks. everything is a library. so we build the engine as a shared library and completely ignore dedicated servers+tools
	SET(FTE_ENGINE_FTEDROID true CACHE BOOL "Compile ftedroid engine shared library.")
	IF(FTE_ENGINE_FTEDROID)
		ADD_LIBRARY(${FTE_ENGINE} MODULE
			${FTE_ARCH_FILES}
			${FTE_COMMON_FILES}
			${FTE_CLIENT_FILES}
			${FTE_SERVER_FILES}
		)
		SET_TARGET_PROPERTIES(${FTE_ENGINE} PROPERTIES COMPILE_DEFINITIONS "${FTE_LIB_DEFINES};${FTE_DEFINES};${FTE_REVISON}")
		TARGET_INCLUDE_DIRECTORIES(${FTE_ENGINE} PUBLIC ${FTE_INCLUDES} ../common)
		set(FTE_LIBS ${FTE_LIBS} oboeaudio)
		TARGET_LINK_LIBRARIES(${FTE_ENGINE} ${FTE_LIBS} )
		SET(INSTALLTARGS ${INSTALLTARGS} ${FTE_ENGINE})
	ENDIF()
ELSE()
	#systems that actually have executables...
	SET(FTE_ENGINE true CACHE BOOL "Compile fteqw engine binary.")
	IF(FTE_ENGINE)
		ADD_EXECUTABLE(fteqw WIN32
			${FTE_ARCH_FILES}
			${FTE_COMMON_FILES}
			${FTE_CLIENT_FILES}
			${FTE_SERVER_FILES}
		)
		IF(CMAKE_SYSTEM_NAME MATCHES "BSD" OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
			FIND_LIBRARY(epoll-shim REQUIRED)
			SET(FTE_INCLUDES ${FTE_INCLUDES} "${EPOLL_INC_DIR}")
			SET(FTE_LIBS ${FTE_LIBS} epoll-shim)
			SET(FTE_DEFINES ${FTE_DEFINES};HAVE_EPOLL)
		ENDIF()
		SET_TARGET_PROPERTIES(fteqw PROPERTIES COMPILE_DEFINITIONS "${FTE_LIB_DEFINES};${FTE_DEFINES};${FTE_REVISON}")
		TARGET_INCLUDE_DIRECTORIES(fteqw PUBLIC ${FTE_INCLUDES})
		TARGET_LINK_LIBRARIES(fteqw ${FTE_LIBS})
		SET(INSTALLTARGS ${INSTALLTARGS} fteqw)

		ADD_CUSTOM_TARGET(fteqw-i18n ALL
			VERBATIM
			WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
			COMMAND xgettext --output=${CMAKE_CURRENT_BINARY_DIR}/fteqw.pot -k -kCon_TPrintf -kCon_SafeTPrintf -kNetchan_OutOfBandTPrintf:4 -kSV_OutOfBandTPrintf:4 -klangtext -kSV_TPrintToClient:3 -kSV_ClientTPrintf:3 -kSV_BroadcastTPrintf:2 -kCVARAFCD:6 -kCVARAFD:5 -kCVARFCD:5 -kCVARAD:4 -kCVARFD:4 -kCVARCD:4 -kCVARD:3 -kCmd_AddCommandD:3 -kCmd_AddCommandAD:4 -kMenu_Prompt:4:5:6 -kHost_EndGame	${FTE_ARCH_FILES} ${FTE_COMMON_FILES} ${FTE_CLIENT_FILES} ${FTE_SERVER_FILES}
			BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/fteqw.pot"
			SOURCES
				${FTE_ARCH_FILES} ${FTE_COMMON_FILES} ${FTE_CLIENT_FILES} ${FTE_SERVER_FILES}
		)
	ENDIF()

	SET(FTE_ENGINE_SERVER_ONLY true CACHE BOOL "Compile fteqw-sv (server only) engine binary.")
	IF(FTE_ENGINE_SERVER_ONLY)
		ADD_EXECUTABLE(fteqw-sv
			${FTESV_ARCH_FILES}
			${FTE_COMMON_FILES}
			${FTE_SERVER_FILES}
		)
		SET_TARGET_PROPERTIES(fteqw-sv PROPERTIES COMPILE_DEFINITIONS "SERVERONLY;${FTE_LIB_DEFINES};${FTESV_DEFINES};${FTE_REVISON}")
		TARGET_INCLUDE_DIRECTORIES(fteqw-sv PUBLIC ${FTE_INCLUDES})
		TARGET_LINK_LIBRARIES(fteqw-sv ${FTESV_LIBS})
		SET(INSTALLTARGS ${INSTALLTARGS} fteqw-sv)
	ENDIF()

	SET(FTE_ENGINE_CLIENT_ONLY false CACHE BOOL "Compile ftedw-cl (client-only) engine binary.")
	IF(FTE_ENGINE_CLIENT_ONLY)
		ADD_EXECUTABLE(fteqw-cl
			${FTE_ARCH_FILES}
			${FTE_COMMON_FILES}
			${FTE_CLIENT_FILES}
		)
		SET_TARGET_PROPERTIES(fteqw-cl PROPERTIES COMPILE_DEFINITIONS "CLIENTONLY;${FTE_LIB_DEFINES};${FTE_DEFINES};${FTE_REVISON}")
		TARGET_INCLUDE_DIRECTORIES(fteqw-cl PUBLIC ${FTE_INCLUDES})
		TARGET_LINK_LIBRARIES(fteqw-cl ${FTE_LIBS})
		SET(INSTALLTARGS ${INSTALLTARGS} fteqw-cl)
	ENDIF()

	IF(FTE_ENGINE OR FTE_ENGINE_CLIENT)
		FIND_PACKAGE(SDL2)
		IF(SDL2_FOUND)
			SET(FTE_SDL2 FTE_SDL)
			SET(FTE_SDL2_INCLUDES ${SDL2_INCLUDE_DIRS})
		ENDIF()
	ENDIF()

	SET(FTE_TOOL_IQM true CACHE BOOL "Compile IQM Tool.")
	IF(FTE_TOOL_IQM)
		ADD_EXECUTABLE(iqmtool
			iqm/iqm.cpp
			plugins/models/gltf.c
			${DRACO_FILES}
			engine/common/json.c
			engine/client/image.c
			imgtool.c
			iqm/iqm.h
		)
		SET_TARGET_PROPERTIES(iqmtool PROPERTIES COMPILE_DEFINITIONS "IQMTOOL;${DRACO_CFLAGS};${FTE_LIB_DEFINES};${FTE_REVISON}")
		TARGET_LINK_LIBRARIES(iqmtool ${CMAKE_DL_LIBS} ${DRACO_LIBRARY} ${JPEG_LIBRARIES} ${PNG_LIBRARIES})
		SET(INSTALLTARGS ${INSTALLTARGS} iqmtool)
	ENDIF()

	SET(FTE_TOOL_IMAGE true CACHE BOOL "Compile Image Tool.")
	IF(FTE_TOOL_IMAGE)
		ADD_EXECUTABLE(imgtool
			engine/client/image.c
			imgtool.c
		)
		TARGET_INCLUDE_DIRECTORIES(imgtool PUBLIC ${FTE_SDL2_INCLUDES})
		SET_TARGET_PROPERTIES(imgtool PROPERTIES COMPILE_DEFINITIONS "IMGTOOL;${FTE_LIB_DEFINES};${FTE_DEFINES};${FTE_REVISON};${FTE_SDL2}")
		TARGET_LINK_LIBRARIES(imgtool ${FTE_LIBS} )
		SET(INSTALLTARGS ${INSTALLTARGS} imgtool)
	ENDIF()

	SET(FTE_TOOL_QTV true CACHE BOOL "Compile qtv server.")
	IF(FTE_TOOL_QTV)
		ADD_EXECUTABLE(qtv
			fteqtv/netchan.c
			fteqtv/parse.c
			fteqtv/msg.c
			fteqtv/qw.c
			fteqtv/source.c
			fteqtv/bsp.c
			fteqtv/rcon.c
			fteqtv/relay.c
			fteqtv/mdfour.c
			engine/common/md5.c
			fteqtv/crc.c
			fteqtv/control.c
			fteqtv/forward.c
			fteqtv/pmove.c
			fteqtv/menu.c
			fteqtv/httpsv.c
			fteqtv/libqtvc/glibc_sucks.c
			engine/common/sha1.c
		)
		SET_TARGET_PROPERTIES(qtv PROPERTIES COMPILE_DEFINITIONS "${FTE_REVISON};${FTE_LIB_DEFINES}")
		IF(WIN32)
			TARGET_LINK_LIBRARIES(qtv ws2_32 winmm ${SYS_LIBS} ${FTEQTV_LIBS})
		ELSEIF(CMAKE_SYSTEM_NAME MATCHES "BSD" OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
			# Add Epoll-shim for the kqueue Unixes here - Brad
			FIND_LIBRARY(epoll-shim REQUIRED)
			TARGET_INCLUDE_DIRECTORIES(qtv PUBLIC "${EPOLL_INC_DIR}")
			TARGET_LINK_LIBRARIES(qtv epoll-shim ${SYS_LIBS} ${FTEQTV_LIBS})
			SET_TARGET_PROPERTIES(qtv PROPERTIES COMPILE_DEFINITIONS "${FTE_REVISON};${FTE_LIB_DEFINES};HAVE_EPOLL")
		ELSE()
			TARGET_LINK_LIBRARIES(qtv ${SYS_LIBS} ${FTEQTV_LIBS})
		ENDIF()
		SET(INSTALLTARGS ${INSTALLTARGS} qtv)
	ENDIF()

	SET(FTE_TOOL_MASTER true CACHE BOOL "Compile master server.")
	IF(FTE_TOOL_MASTER)
		IF(NOT WIN32)
			ADD_EXECUTABLE(ftemaster
				${FTESV_ARCH_FILES}
				engine/server/sv_master.c
				engine/common/net_ice.c		#for the stun responses.
				engine/common/net_wins.c
				engine/common/cvar.c
				engine/common/cmd.c
				engine/common/sha1.c		#for websockets
				engine/common/sha2.c		#for fingerprints
				engine/http/httpclient.c	#for the pipe stuff
				engine/common/log.c
				engine/common/fs.c
				engine/common/fs_stdio.c
				engine/common/common.c
				engine/common/translate.c
				engine/common/zone.c
				engine/qclib/hash.c
			)
			SET_TARGET_PROPERTIES(ftemaster PROPERTIES COMPILE_DEFINITIONS "MASTERONLY;${FTE_LIB_DEFINES};${FTESV_DEFINES};${FTE_REVISON}")
			TARGET_LINK_LIBRARIES(ftemaster ${FTESV_LIBS})
			SET(INSTALLTARGS ${INSTALLTARGS} ftemaster)
		ENDIF()
	ENDIF()

	SET(FTE_TOOL_HTTPSV true CACHE BOOL "Compile small http server.")
	IF(FTE_TOOL_HTTPSV)
		ADD_EXECUTABLE(httpserver
			engine/common/fs_stdio.c
			engine/http/httpserver.c
			engine/http/iwebiface.c
			engine/http/ftpserver.c
		)
		SET_TARGET_PROPERTIES(httpserver PROPERTIES COMPILE_DEFINITIONS "WEBSERVER;WEBSVONLY;${FTE_REVISON}")
		IF(WIN32)
			TARGET_LINK_LIBRARIES(httpserver ws2_32)
		ELSEIF(CMAKE_SYSTEM_NAME MATCHES "BSD" OR CMAKE_SYSTEM_NAME MATCHES "Darwin")
			FIND_LIBRARY(epoll-shim REQUIRED)
			TARGET_INCLUDE_DIRECTORIES(httpserver PUBLIC "${EPOLL_INC_DIR}")
			TARGET_LINK_LIBRARIES(httpserver epoll-shim)
			SET_TARGET_PROPERTIES(httpserver PROPERTIES COMPILE_DEFINITIONS "WEBSERVER;WEBSVONLY;${FTE_REVISON};HAVE_EPOLL")
		ENDIF()
		#SET(INSTALLTARGS ${INSTALLTARGS} httpserver)
	ENDIF()

	SET(FTE_TOOL_QCVM false CACHE BOOL "Compile standalone qcvm.")
	IF(FTE_TOOL_QCVM)
		ADD_EXECUTABLE(qcvm
			engine/qclib/test.c

			engine/qclib/hash.c
			${FTE_QCVM_FILES}
		)
		SET_TARGET_PROPERTIES(qcvm PROPERTIES COMPILE_DEFINITIONS "${FTE_LIB_DEFINES};${FTE_REVISON}")
		TARGET_LINK_LIBRARIES(qcvm ${FTEQCC_LIBS} ${SYS_LIBS})
		SET(INSTALLTARGS ${INSTALLTARGS} qcvm)
	ENDIF()

	SET(FTE_TOOL_QCC true CACHE BOOL "Compile commandline qc compiler.")
	IF(FTE_TOOL_QCC)
		ADD_EXECUTABLE(fteqcc
			engine/qclib/qcctui.c
			engine/qclib/comprout.c
			engine/qclib/hash.c
			engine/qclib/qcc_cmdlib.c
			engine/qclib/qcc_pr_comp.c
			engine/qclib/qcc_pr_lex.c
			engine/qclib/qccmain.c
			engine/qclib/qcd_main.c
			engine/qclib/decomp.c
			engine/qclib/packager.c
		)
		SET_TARGET_PROPERTIES(fteqcc PROPERTIES COMPILE_DEFINITIONS "${FTE_LIB_DEFINES};${FTE_REVISON}")
		TARGET_LINK_LIBRARIES(fteqcc ${FTEQCC_LIBS} ${SYS_LIBS})
		SET(INSTALLTARGS ${INSTALLTARGS} fteqcc)
	ENDIF()

	SET(FTE_TOOL_QCCGUI true CACHE BOOL "Compile gui qc compiler.")
	IF(FTE_TOOL_QCCGUI)
		IF(${WIN32})
			ADD_EXECUTABLE(fteqccgui WIN32
				engine/qclib/qccgui.c
				engine/qclib/qccguistuff.c
				engine/qclib/comprout.c
				engine/qclib/hash.c
				engine/qclib/qcc_cmdlib.c
				engine/qclib/qcc_pr_comp.c
				engine/qclib/qcc_pr_lex.c
				engine/qclib/qccmain.c
				engine/qclib/decomp.c
				engine/qclib/packager.c
				engine/qclib/qcd_main.c
			)
			SET_TARGET_PROPERTIES(fteqccgui PROPERTIES COMPILE_DEFINITIONS "${FTE_LIB_DEFINES};${FTE_REVISON}")
			TARGET_LINK_LIBRARIES(fteqccgui ${FTEQCC_LIBS} shlwapi ole32 comctl32 comdlg32)
			SET(INSTALLTARGS ${INSTALLTARGS} fteqccgui)
		ELSE()
			FIND_PACKAGE(Qt5Widgets)

			FIND_PATH(QSCINTILLA_INCLUDE_DIR
				NAMES Qsci/qsciglobal.h
				PATHS ${Qt5Widgets_INCLUDE_DIRS}
				PATH_SUFFIXES Qsci
			)
			FIND_LIBRARY(QSCINTILLA_LIBRARY
				NAMES qscintilla2_qt5
				PATHS
					${QT_LIBRARY_DIR}
					/usr/local/lib
					/usr/local/lib/qt5
					/usr/lib
			)

			IF (QSCINTILLA_INCLUDE_DIR AND QSCINTILLA_LIBRARY AND Qt5Widgets_FOUND)
				ADD_EXECUTABLE(fteqccgui
					engine/qclib/qccguiqt.cpp
					engine/qclib/qccguistuff.c
					engine/qclib/comprout.c
					engine/qclib/hash.c
					engine/qclib/qcc_cmdlib.c
					engine/qclib/qcc_pr_comp.c
					engine/qclib/qcc_pr_lex.c
					engine/qclib/qccmain.c
					engine/qclib/decomp.c
#					engine/qclib/packager.c
					engine/qclib/qcd_main.c
				)
				TARGET_INCLUDE_DIRECTORIES(fteqccgui PUBLIC ${Qt5Widgets_INCLUDE_DIRS} ${QSCINTILLA_INCLUDE_DIR})
				SET_TARGET_PROPERTIES(fteqccgui PROPERTIES COMPILE_DEFINITIONS "${FTE_LIB_DEFINES};${FTE_REVISON};${Qt5Widgets_COMPILE_DEFINITIONS}")
				SET_PROPERTY(TARGET fteqccgui PROPERTY POSITION_INDEPENDENT_CODE TRUE)
				TARGET_LINK_LIBRARIES(fteqccgui ${FTEQCC_LIBS} ${Qt5Widgets_LIBRARIES} ${QSCINTILLA_LIBRARY})
				SET(INSTALLTARGS ${INSTALLTARGS} fteqccgui)
			ELSE()
				MESSAGE(WARNING "qscintilla/qt5widgets library not detected, no fteqccgui for you")
			ENDIF()
		ENDIF()
	ENDIF()
ENDIF()

IF(0)
	#software renderer plugin
	#not stable enough, and probably won't ever be
	ADD_LIBRARY(sw MODULE
		plugins/plugin.c
		engine/sw/sw_backend.c
		engine/sw/sw_image.c
		engine/sw/sw_rast.c
		#engine/sw/sw_viddos.c
#		engine/sw/sw_vidwin.c
		engine/common/mathlib.c
#		engine/client/in_win.c

		engine/sw/sw.h
		engine/sw/sw_spans.h
	)
	SET_TARGET_PROPERTIES(sw PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES};SWQUAKE")
	TARGET_LINK_LIBRARIES(sw ${SYS_LIBS})

	EMBED_PLUGIN_META(sw "Software Renderer" "Provides software rendering. Slow.")
ENDIF()

#Quake Injector Alike plugin
SET(FTE_PLUG_QI true CACHE BOOL "Compile Quake-Injnector plugin.")
IF(FTE_PLUG_QI)
	ADD_LIBRARY(plug_qi MODULE
		plugins/plugin.c
		plugins/qi/qi.c
		plugins/jabber/xml.c
	)
	SET_TARGET_PROPERTIES(plug_qi PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES}")
	TARGET_LINK_LIBRARIES(plug_qi ${SYS_LIBS})

	EMBED_PLUGIN_META(qi "Quaddicted Map Database" "Provides easy access to the quaddicted map database. Once installed you can use eg 'map qi_dopa:start' to begin playing dopa, or load it via the menus.")
ENDIF()

SET(FTE_PLUG_OPENSSL false CACHE BOOL "Compile OpenSSL.")
IF(FTE_PLUG_OPENSSL)
	#the openssl license is incompatible with the GPL, so while we have code to use it distributing the binaries built with it is not a (legal) option.
	#note that openssl 3.0.0 upwards are apache-2 licensed, which IS gpl-3 compatible (though not gpl-2). debian has not caught up with that yet, however.
	#Crosscompile linux->win64: sudo ln -s ${pwd}/engine/libs-x86_64-w64-mingw32/openssl-openssl-3.0.1/ /usr/x86_64-w64-mingw32/OpenSSL
	SET(OPENSSL_USE_STATIC_LIBS true CACHE BOOL "Link openssl statically.") #usually as an .so though. :/)
	FIND_PACKAGE(OpenSSL)
	IF(OPENSSL_VERSION_MAJOR LESS 3)
		SET(FTE_PRIVATE_USE_ONLY false CACHE BOOL "Ignore license violations.")
	ENDIF()
	IF(NOT OPENSSL_FOUND)
		MESSAGE(WARNING "openssl library NOT available. you'll have to use some other library.")
	ELSEIF(OPENSSL_VERSION_MAJOR LESS 3 AND NOT FTE_PRIVATE_USE_ONLY)
		MESSAGE(WARNING "openssl v3 required for GPL compliance. Enable FTE_PRIVATE_USE_ONLY to compile openssl plugin.")
	ELSE()
		IF(OPENSSL_VERSION_MAJOR LESS 3)
			MESSAGE(WARNING "openssl library version is not 3 or above. You may not distribute plugin binaries due to license conflict.")
		ELSE()
			MESSAGE(WARNING "Using openssl. Resulting plugin must be licensed as GPLv3.")
		ENDIF()
		SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES})
		if (WIN32)
			SET(OPENSSL_LIBRARIES ${OPENSSL_LIBRARIES} ws2_32)
		ENDIF()

		ADD_LIBRARY(plug_openssl MODULE
			plugins/plugin.c
			plugins/net_ssl_openssl.c
		)
		TARGET_INCLUDE_DIRECTORIES(plug_openssl PRIVATE ${OPENSSL_INCLUDE_DIR})
		SET_TARGET_PROPERTIES(plug_openssl PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES}")
		TARGET_LINK_LIBRARIES(plug_openssl ${SYS_LIBS} ${OPENSSL_LIBRARIES})

		EMBED_PLUGIN_META(openssl "OpenSSL" "Provides OpenSSL support for dtls/tls/https support. The crypto library that is actually used is controlled via the tls_provider cvar.")
	ENDIF()
ENDIF()

#SET(FTE_PLUG_GNUTLS true CACHE BOOL "Compile GnuTLS Library.")
#IF(FTE_PLUG_GNUTLS)
#	FIND_PACKAGE(GnuTLS)
#	IF(NOT GNUTLS_FOUND)
#		MESSAGE(WARNING "gnutls library NOT available. you'll have to use some other library.")
#	ELSE()
#		SET(FTE_LIB_DEFINES ${FTE_LIB_DEFINES})
#
#		ADD_LIBRARY(plug_gnutls MODULE
#			plugins/plugin.c
#			engine/common/net_ssl_gnutls.c
#		)
#		SET_TARGET_PROPERTIES(plug_gnutls PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES}")
#		TARGET_LINK_LIBRARIES(plug_gnutls ${SYS_LIBS} ${GNUTLS_LIBRARIES})
#
#		EMBED_PLUGIN_META(gnutls "GnuTLS" "Provides GnuTLS support for dtls/tls/https support. The crypto library that is actually used is controlled via the tls_provider cvar.")
#	ENDIF()
#ENDIF()

#EzQuake Hud port plugin
SET(FTE_PLUG_EZHUD true CACHE BOOL "Compile MoreQuakeWorld Hud plugin .")
IF(FTE_PLUG_EZHUD)
	ADD_LIBRARY(plug_ezhud MODULE
		plugins/plugin.c
		plugins/ezhud/ezquakeisms.c
		plugins/ezhud/hud.c
		plugins/ezhud/hud_common.c
		plugins/ezhud/hud_editor.c
	)
	SET_TARGET_PROPERTIES(plug_ezhud PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES}")
	TARGET_LINK_LIBRARIES(plug_ezhud ${SYS_LIBS})

	EMBED_PLUGIN_META(ezhud "EzHud Plugin" "Provides compat with ezquake's hud scripts.")
ENDIF()

#NameMaker string generation plugin
SET(FTE_PLUG_NAMEMAKER false CACHE BOOL "Compile namemaker plugin.")
IF(FTE_PLUG_NAMEMAKER)
	ADD_LIBRARY(plug_namemaker MODULE
		plugins/plugin.c
		plugins/namemaker/namemaker.c
	)
	SET_TARGET_PROPERTIES(plug_namemaker PROPERTIES COMPILE_DEFINITIONS "${FTE_LIB_DEFINES}")
	TARGET_LINK_LIBRARIES(plug_namemaker ${SYS_LIBS})

	EMBED_PLUGIN_META(namemaker "Name Maker Plugin" "Provides a lame UI for selecting arbitrary non-ascii glyphs as part of your nickname.")
ENDIF()

#Terrain Generation plugin
SET(FTE_PLUG_TERRAINGEN false CACHE BOOL "Compile sample terrain generation plugin.")
IF(FTE_PLUG_TERRAINGEN)
	ADD_LIBRARY(plug_terraingen MODULE
		plugins/plugin.c
		plugins/terrorgen/terragen.c
	)
	SET_TARGET_PROPERTIES(plug_terraingen PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES}")
	TARGET_LINK_LIBRARIES(plug_terraingen ${SYS_LIBS})

	EMBED_PLUGIN_META(terraingen "TerrainGen Plugin" "A lame example plugin for randomised terrain generation.")
ENDIF()

#IRC client plugin
SET(FTE_PLUG_IRC true CACHE BOOL "Compile irc plugin.")
IF(FTE_PLUG_IRC)
	ADD_LIBRARY(plug_irc MODULE
		plugins/plugin.c
		plugins/irc/ircclient.c
	)
	SET_TARGET_PROPERTIES(plug_irc PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_REVISON};${FTE_LIB_DEFINES}")
	TARGET_LINK_LIBRARIES(plug_irc ${SYS_LIBS})

	EMBED_PLUGIN_META(irc "IRC Plugin" "Allows you to chat on IRC without tabbing out.")
ENDIF()

IF(ZLIB_FOUND)
#mpq package format plugin (blizzard games)
SET(FTE_PLUG_MPQ false CACHE BOOL "Compile mpq junk.")
IF(FTE_PLUG_MPQ)
	ADD_LIBRARY(plug_mpq MODULE
		plugins/plugin.c
		plugins/mpq/blast.c
		plugins/mpq/fs_mpq.c
	)
	SET_TARGET_PROPERTIES(plug_mpq PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES}")
	TARGET_LINK_LIBRARIES(plug_mpq ${SYS_LIBS} ${ZLIB_LIBRARIES})

	EMBED_PLUGIN_META(irc "MPQ Archive Plugin" "Adds support for reading .mpq files. Not very useful...")
ENDIF()
ENDIF()

#vpk package format plugin (halflife2)
SET(FTE_PLUG_HL2 true CACHE BOOL "Compile support for hl2 file formats.")
IF(FTE_PLUG_HL2)
	ADD_LIBRARY(plug_hl2 MODULE
		plugins/plugin.c
		plugins/hl2/hl2.c
		plugins/hl2/fs_vpk.c
		plugins/hl2/img_vtf.c
		plugins/hl2/mod_hl2.c
		plugins/hl2/mod_vbsp.c
		plugins/hl2/mat_vmt.c
	)
	SET_TARGET_PROPERTIES(plug_hl2 PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;MULTITHREAD;${FTE_LIB_DEFINES}")
	TARGET_LINK_LIBRARIES(plug_hl2 ${SYS_LIBS})

	EMBED_PLUGIN_META(hl2 "HalfLife2 Formats Plugin" "Adds support for reading various file formats used by HalfLife2. Requires mod support to be useful.")
ENDIF()

#model formats plugin
SET(FTE_PLUG_MODELS true CACHE BOOL "Compile models formats plugin.")
IF(FTE_PLUG_MODELS)
	ADD_LIBRARY(plug_models MODULE
		plugins/plugin.c
		plugins/models/models.c
		plugins/models/gltf.c
		${DRACO_FILES}
		engine/common/json.c
		plugins/models/exportiqm.c
	)
	SET_TARGET_PROPERTIES(plug_models PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${DRACO_CFLAGS};${FTE_LIB_DEFINES}")
	TARGET_LINK_LIBRARIES(plug_models ${SYS_LIBS} ${DRACO_LIBRARY})

	EMBED_PLUGIN_META(models "Models Plugin" "Kinda redundant now that the engine has gltf2 loading.")
ENDIF()

SET(FTE_PLUG_X11SV false CACHE BOOL "Compile x11 server plugin.")
IF(FTE_PLUG_X11SV)
	#x11 server plugin (note: for displaying other programs)
	#not stable enough, and probably won't ever be
	ADD_LIBRARY(plug_x11sv MODULE
		plugins/plugin.c
		plugins/xsv/m_x.c
		plugins/xsv/x_reqs.c
		plugins/xsv/x_res.c
		engine/qclib/hash.c
	)
	SET_TARGET_PROPERTIES(plug_x11sv PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES}")
	TARGET_LINK_LIBRARIES(plug_x11sv ${SYS_LIBS})

	EMBED_PLUGIN_META(x11sv "X11 Server Plugin" "Provides a primitive X11 server in the form of a video decoder plugin.")
ENDIF()

#ffmpeg client plugin. no proper way to detect dependancies right now, so I've gotta try the manual way.
SET(FTE_PLUG_FFMPEG true CACHE BOOL "Compile ffmpeg media plugin.")
IF(FTE_PLUG_FFMPEG)
	FIND_PATH(AVCODEC_INCLUDE_DIR libavcodec/avcodec.h)
	FIND_PATH(AVFORMAT_INCLUDE_DIR libavformat/avformat.h)
	FIND_PATH(AVUTIL_INCLUDE_DIR libavutil/avutil.h)
	FIND_PATH(AVSWSCALE_INCLUDE_DIR libswscale/swscale.h)
	IF((AVFORMAT_INCLUDE_DIR) AND (AVSWSCALE_INCLUDE_DIR))
		FIND_LIBRARY(AVCODEC_LIBRARY avcodec)
		FIND_LIBRARY(AVFORMAT_LIBRARY avformat)
		FIND_LIBRARY(AVUTIL_LIBRARY avutil)
		FIND_LIBRARY(AVSWSCALE_LIBRARY swscale)

		ADD_LIBRARY(plug_ffmpeg MODULE
			plugins/plugin.c
			plugins/avplug/avaudio.c
			plugins/avplug/avdecode.c
			plugins/avplug/avencode.c
		)
		TARGET_INCLUDE_DIRECTORIES(plug_ffmpeg PUBLIC ${AVCODEC_INCLUDE_DIR} ${AVFORMAT_INCLUDE_DIR} ${AVUTIL_INCLUDE_DIR} ${AVSWSCALE_INCLUDE_DIR})
		TARGET_LINK_LIBRARIES(plug_ffmpeg ${SYS_LIBS} ${AVFORMAT_LIBRARY} ${AVCODEC_LIBRARY} ${AVUTIL_LIBRARY} ${AVSWSCALE_LIBRARY})
		SET_TARGET_PROPERTIES(plug_ffmpeg PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES}")

		EMBED_PLUGIN_META(ffmpeg "FFMPEG Video Decoding Plugin" "Provides support for more audio formats, as well as video playback and better capture support.")
	ELSE()
		MESSAGE(WARNING "ffmpeg library NOT available. Quake shouldn't be playing fmv anyway.")
	ENDIF()
ENDIF()

SET(FTE_PLUG_TIMIDITY false CACHE BOOL "Compile timidity audio plugin.")
IF(FTE_PLUG_TIMIDITY)
	#timidity
	FIND_PATH(TIMIDITY_INCLUDE_DIR timidity/timidity.h)
	IF(TIMIDITY_INCLUDE_DIR)
		FIND_LIBRARY(TIMIDITY_LIBRARY timidity)

		ADD_LIBRARY(plug_timidity MODULE
			plugins/plugin.c
			plugins/timidity.c
		)
		TARGET_INCLUDE_DIRECTORIES(plug_timidity PUBLIC ${TIMIDITY_INCLUDE_DIR})
		TARGET_LINK_LIBRARIES(plug_timidity ${SYS_LIBS} ${TIMIDITY_LIBRARY})
		SET_TARGET_PROPERTIES(plug_timidity PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES}")

		EMBED_PLUGIN_META(timidity "Timidity Plugin" "Provides support for playback of midi files.")
	ELSE()
		MESSAGE(WARNING "timidity library NOT available. We'll just stick to fake-cd music for hexen2.")
	ENDIF()
ENDIF()

#openxr plugin
SET(FTE_PLUG_OPENXR true CACHE BOOL "Compile openxr plugin (for vr support).")
IF(FTE_PLUG_OPENXR)
	FIND_PACKAGE(PkgConfig)
	IF (PKGCONFIG_FOUND)
		IF (NOT CMAKE_CROSSCOMPILING)	#its picking up the linux headers then complaining that they're missing in mingw. also almost entirely untested so no great loss.
			PKG_SEARCH_MODULE(OPENXR openxr)
		ENDIF()
		IF (OPENXR_FOUND)
			ADD_LIBRARY(plug_openxr MODULE
				plugins/plugin.c
				plugins/openxr.c
			)
			TARGET_INCLUDE_DIRECTORIES(plug_openxr PRIVATE ${OPENXR_INCLUDE_DIRS} )

			IF (1)	#dynamically link
				SET_TARGET_PROPERTIES(plug_openxr PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES};${FTE_DEFINES};XR_NO_PROTOTYPES")
				TARGET_LINK_LIBRARIES(plug_openxr ${SYS_LIBS})
			ELSE()	#statically link
				SET_TARGET_PROPERTIES(plug_openxr PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES};${FTE_DEFINES}")
				TARGET_LINK_LIBRARIES(plug_openxr ${SYS_LIBS} ${OPENXR_LIBRARIES})
			ENDIF()

			EMBED_PLUGIN_META(openxr "OpenXR Plugin" "Provides support for Virtual Reality headsets and input devices.")
		ELSE()
			MESSAGE(WARNING "openxr library NOT available. Quake is already a reality anyway.")
		ENDIF()
	ENDIF()
ENDIF()

##cef plugin
##libcef itself can be obtained from https://cef-builds.spotifycdn.com/index.html#linux64 (minimal builds, which still ends up with a 1,162,752,744 byte libcef.so - yes, actual size)
##(be sure to manually strip the binary of its debug info)
##to get this cmake stuff to recognise the headers etc:
##	cd $FTEQW-REPO && ln -s $FOO/cef_binary_$FOO+chromium-$FOO_linux64_minimal plugins/cef/cef_linux64
##(note that other systems use other subdir names)
SET(FTE_PLUG_CEF true CACHE BOOL "Compile libcef (webbrowser) plugin.")
SET(CEF_PATH ${CEF_PATH} CACHE PATH "Base location of libcef for target platform.")
IF(FTE_PLUG_CEF)
	IF(CEF_PATH MATCHES "")
		UNSET(CEF_PATH CACHE)
		IF(WIN32)
			IF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "AMD64")
				FIND_PATH (CEF_PATH include/cef_version.h plugins/cef/cef_windows64)
			ELSEIF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86")
				FIND_PATH (CEF_PATH include/cef_version.h plugins/cef/cef_windows32)
			ELSEIF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "ARM64")
				FIND_PATH (CEF_PATH include/cef_version.h plugins/cef/cef_windowsarm64)
			ENDIF()
		ELSEIF("${CMAKE_SYSTEM}" MATCHES "Linux")
			IF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
				FIND_PATH (CEF_PATH include/cef_version.h plugins/cef/cef_linux64)
			ELSEIF(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "i686")
				FIND_PATH (CEF_PATH include/cef_version.h plugins/cef/cef_linux32)
			ENDIF()
		ENDIF()
	ENDIF()
	#FIND_LIBRARY(CEF_LIBRARIES cef ${CEF_PATH}/Release)
	IF (CEF_PATH)
		##statically link only for release builds. debug builds probably don't want to have to wait for ages for the debugger to finish loading debug info unless we're actually using this stuff.
		IF(CMAKE_BUILD_TYPE MATCHES "Release")
			SET(CEF_LIBRARIES "${CMAKE_BINARY_DIR}/libcef.so")
		ENDIF()
		ADD_LIBRARY(plug_cef MODULE
			plugins/plugin.c
			plugins/cef/cef.c
		)
		TARGET_INCLUDE_DIRECTORIES(plug_cef PRIVATE ${CEF_PATH})
		SET_TARGET_PROPERTIES(plug_cef PROPERTIES BUILD_RPATH_USE_ORIGIN true)
		if (CEF_LIBRARIES)
			SET_TARGET_PROPERTIES(plug_cef PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES};${FTE_DEFINES};LIBCEF_STATIC")
			TARGET_LINK_LIBRARIES(plug_cef ${SYS_LIBS} ${CEF_LIBRARIES} ${CMAKE_DL_LIBS})
		ELSE()
			SET_TARGET_PROPERTIES(plug_cef PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES};${FTE_DEFINES};LIBCEF_DYNAMIC")
			TARGET_LINK_LIBRARIES(plug_cef ${SYS_LIBS} ${CMAKE_DL_LIBS})
		ENDIF()

		IF(NOT ${UNIX})
			ADD_CUSTOM_COMMAND(
				TARGET plug_cef PRE_LINK
				COMMAND cp ${CEF_PATH}/Release/* ${CMAKE_BINARY_DIR}
				COMMAND cp ${CEF_PATH}/Resources/* ${CMAKE_BINARY_DIR}
				)
		ELSE()
		    if(NOT ANDROID)
			IF(CMAKE_BUILD_TYPE MATCHES "Release")
				#sigh, cef ain't stripped properly on linux.
				ADD_CUSTOM_COMMAND(
					TARGET plug_cef PRE_LINK
					COMMAND ln -f -s ${CEF_PATH}/Release/* ${CMAKE_BINARY_DIR}
					COMMAND strip ${CMAKE_BINARY_DIR}/libcef.so -o libcef.so
					COMMAND strip ${CMAKE_BINARY_DIR}/libEGL.so -o libEGL.so
					COMMAND strip ${CMAKE_BINARY_DIR}/libGLESv2.so -o libGLESv2.so
					COMMAND strip ${CMAKE_BINARY_DIR}/chrome-sandbox -o chrome-sandbox
					COMMAND ln -f -s ${CEF_PATH}/Resources/* ${CMAKE_BINARY_DIR}
					)
			ELSE()
				ADD_CUSTOM_COMMAND(
					TARGET plug_cef PRE_LINK
					COMMAND ln -f -s ${CEF_PATH}/Release/* ${CMAKE_BINARY_DIR}
					COMMAND ln -f -s ${CEF_PATH}/Resources/* ${CMAKE_BINARY_DIR}
					)
			ENDIF()
			endif()
		ENDIF()

		EMBED_PLUGIN_META(cef "libcef(Browser) Plugin" "This plugin provides support for an in-game web browser.")
	ELSE()
		MESSAGE(WARNING "libcef library NOT available. no web browser support on walls.")
	ENDIF()
ENDIF()

SET(FTE_PLUG_XMPP true CACHE BOOL "Compile xmpp/jabber instant-messenger plugin.")
IF(FTE_PLUG_XMPP)
	#XMPP/jabber client plugin
	ADD_LIBRARY(plug_xmpp MODULE
		plugins/plugin.c
		plugins/jabber/jabberclient.c
		plugins/jabber/xml.c
		plugins/jabber/jingle.c
		plugins/jabber/sift.c
		engine/common/sha1.c
		engine/common/sha2.c
		plugins/emailnot/md5.c
	)
	SET_TARGET_PROPERTIES(plug_xmpp PROPERTIES COMPILE_DEFINITIONS "FTEPLUGIN;${FTE_LIB_DEFINES}")

	#for SRV lookups, so we actually get the right server from account names/etc.
	IF(ANDROID)	#libresolv issues.
	ELSEIF(${WIN32}) #softlinks a dll
	ELSE()
		TARGET_LINK_LIBRARIES(plug_xmpp ${SYS_LIBS} resolv)
	ENDIF()

	EMBED_PLUGIN_META(xmpp "XMPP Plugin" "XMPP/Jabber instant messenger plugin for chatting without tabbing out.")
ENDIF()

INSTALL(TARGETS ${INSTALLTARGS}
	RUNTIME DESTINATION "${CMAKE_INSTALL_PREFIX}/${FTE_INSTALL_BINDIR}"
	LIBRARY DESTINATION "${CMAKE_INSTALL_FULL_LIBDIR}/${FTE_INSTALL_LIBDIR}"
)

INSTALL(FILES
	fteqw.desktop
	DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/applications/")


SET(FTE_MENU_SYS true CACHE BOOL "Compile System Menu.")
IF(FTE_MENU_SYS)
	ADD_CUSTOM_TARGET(menusys ALL
		VERBATIM
		DEPENDS fteqcc
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/quakec/menusys/"
		COMMAND fteqcc -srcfile "menu.src" -o "${CMAKE_CURRENT_BINARY_DIR}/menu.dat" -DREVISION="${SVNREVISION}" -DDATE="${FTE_DATE}" -DBRANCH="${FTE_BRANCH}"
		COMMAND /bin/echo -e "{\\n	package fte_menusys\\n	ver \"${SVNREVISION}\"\\n	category Plugins\\n	title \"Replacement Menus\"\\n	gamedir \"id1\"\\n	desc \"Modern menus to replace the ancient quake ones\"\\n}" | zip -j -q -9 -fz- "${CMAKE_CURRENT_BINARY_DIR}/menusys.pk3" - "${CMAKE_CURRENT_BINARY_DIR}/menu.dat"
		BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/menu.dat" "${CMAKE_CURRENT_BINARY_DIR}/menu.lno" "${CMAKE_CURRENT_BINARY_DIR}/menusys.pk3"
		SOURCES
			quakec/menusys/menu.src
			quakec/menusys/fteextensions.qc
			quakec/menusys/menusys/mitems.qc
			quakec/menusys/menusys/mitems_common.qc
			quakec/menusys/menusys/mitem_frame.qc
			quakec/menusys/menusys/mitem_desktop.qc
			quakec/menusys/menusys/mitem_exmenu.qc
			quakec/menusys/menusys/mitem_edittext.qc
			quakec/menusys/menusys/mitem_tabs.qc
			quakec/menusys/menusys/mitem_colours.qc
			quakec/menusys/menusys/mitem_checkbox.qc
			quakec/menusys/menusys/mitem_slider.qc
			quakec/menusys/menusys/mitem_combo.qc
			quakec/menusys/menusys/mitem_bind.qc
			quakec/menusys/menusys/mitem_spinnymodel.qc
			quakec/menusys/menu/loadsave.qc
			quakec/menusys/menu/newgame.qc
			quakec/menusys/menu/options_basic.qc
			quakec/menusys/menu/options_effects.qc
			quakec/menusys/menu/options_keys.qc
			quakec/menusys/menu/options.qc
			quakec/menusys/menu/presets.qc
			quakec/menusys/menu/servers.qc
			quakec/menusys/menu/main.qc
			quakec/menusys/menu/mods.qc
			quakec/menusys/menu/cvars.qc
			quakec/menusys/menu/updates.qc
			quakec/menusys/menu/options_audio.qc
			quakec/menusys/menu/options_configs.qc
			quakec/menusys/menu/options_hud.qc
			quakec/menusys/menu/options_particles.qc
			quakec/menusys/menu/options_video.qc
			quakec/menusys/menu/quit.qc
	)

	INSTALL(FILES
		${CMAKE_CURRENT_BINARY_DIR}/menusys.pk3
		DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/games/quake/id1/")
ENDIF()

SET(FTE_CSADDON true CACHE BOOL "CS Addon.")
IF(FTE_CSADDON)
	ADD_CUSTOM_TARGET(csaddon ALL
		VERBATIM
		DEPENDS fteqcc
		WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/quakec/csaddon/src/"
		COMMAND fteqcc -srcfile "csaddon.src" -o "${CMAKE_CURRENT_BINARY_DIR}/csaddon.dat"
		COMMAND /bin/echo -e "{\\n	package fte_csaddon\\n	ver \"${SVNREVISION}\"\\n	category Plugins\\n	title \"${PLUGTITLE}\"\\n	gamedir \"id1\"\\n	desc \"${PLUGDESC}\"\\n}" | zip -j -q -9 -fz- "${CMAKE_CURRENT_BINARY_DIR}/csaddon.pk3" - "${CMAKE_CURRENT_BINARY_DIR}/csaddon.dat"
		BYPRODUCTS "${CMAKE_CURRENT_BINARY_DIR}/csaddon.dat" "${CMAKE_CURRENT_BINARY_DIR}/csaddon.lno" "${CMAKE_CURRENT_BINARY_DIR}/csaddon.pk3"
		SOURCES
			quakec/csaddon/src/csaddon.src

			quakec/csaddon/src/csplat.qc
			quakec/csaddon/src/csfixups.qc

			quakec/csaddon/src/editor_lights.qc
			quakec/csaddon/src/editor_terrain.qc
			quakec/csaddon/src/brush_selection.qc
			quakec/csaddon/src/brush_history.qc
			quakec/csaddon/src/brush_manip.qc
			quakec/csaddon/src/brush_draw.qc
			quakec/csaddon/src/brush_vertedit.qc
			quakec/csaddon/src/editor_brushes.qc
			quakec/csaddon/src/editor_ents.qc
			quakec/csaddon/src/textfield.qc
			quakec/csaddon/src/editor_particles.qc
			quakec/csaddon/src/menu.qc
			quakec/csaddon/src/cam.qc
			quakec/csaddon/src/csaddon.qc
	)

	INSTALL(FILES
		${CMAKE_CURRENT_BINARY_DIR}/csaddon.pk3
		DESTINATION "${CMAKE_INSTALL_DATAROOTDIR}/games/quake/id1/")
ENDIF()
