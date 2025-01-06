CMAKE_MINIMUM_REQUIRED(VERSION 3.12)

PROJECT(quake3e)

add_subdirectory(qsandbox_default)
SET(CNAME "quake3e")
SET(DNAME "quake3e.ded")

SET(RENDERER_PREFIX ${CNAME})
SET(RENDERER_DEFAULT opengl) # valid options: opengl, vulkan, opengl2

OPTION(USE_SDL "" OFF) #k ON
OPTION(USE_CURL "" ON)
OPTION(USE_LOCAL_HEADERS "" ON)
OPTION(USE_VULKAN "" OFF) #k ON
OPTION(USE_SYSTEM_JPEG "" OFF)
OPTION(USE_RENDERER_DLOPEN "" OFF) #k ON

OPTION(QS_BUILD_DED "" OFF)

if(NOT USE_VULKAN)
	add_definitions(-DUSE_OPENGL_API)
endif()
# build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()
set(CMAKE_BUILD_TYPE "RelWithDebInfo")

include_directories("../common")
add_definitions(-D_DIII4A) # android
set(cpu ${CMAKE_SYSTEM_PROCESSOR})
if(cpu STREQUAL "powerpc")
	set(cpu "ppc")
elseif(cpu STREQUAL "aarch64")
	# "arm64" is more obvious, and some operating systems (like macOS) use it instead of "aarch64"
	set(cpu "arm64")
elseif(cpu MATCHES "[aA][mM][dD]64" OR cpu MATCHES "[xX].*64")
	set(cpu "x86_64")
elseif(cpu MATCHES "i.86" OR cpu MATCHES "[xX]86")
	set(cpu "i386")
elseif(cpu MATCHES "[aA][rR][mM].*") # some kind of arm..
	# On 32bit Raspbian gcc -dumpmachine returns sth starting with "arm-",
	# while clang -dumpmachine says "arm6k-..." - try to unify that to "arm"
	if(CMAKE_SIZEOF_VOID_P EQUAL 8) # sizeof(void*) == 8 => must be arm64
		set(cpu "arm64")
	else() # should be 32bit arm then (probably "armv7l" "armv6k" or sth like that)
		set(cpu "arm")
	endif()
endif()
set(ARCH "${cpu}")
#add_definitions(-DARCH_STRING="${ARCH}")
add_definitions(-D_SIZEOFPTR=${CMAKE_SIZEOF_VOID_P})
if(CMAKE_CXX_BYTE_ORDER EQUAL BIG_ENDIAN)
	set(big_endian 1)
else()
	set(big_endian 0)
endif()
add_definitions(-D_IS_BIG_ENDIAN=${big_endian})

if(ANDROID)
set(qsclient_libs android log GLESv1_CM EGL)
set(qsclient_libs ${qsclient_libs} oboeaudio)
elseif(WIN32)
set(qsclient_libs winmm wsock32 comctl32 ws2_32)
endif()


set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pipe -fsigned-char -fPIC -DNDEBUG -O0 -g -ggdb -fno-builtin-sin -fno-builtin-cos -ffast-math -fno-strict-aliasing -Wformat=2 -Wno-format-zero-length -Wformat-security -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute -Wdisabled-optimization") # -Werror-implicit-function-declaration -Qunused-arguments
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe -fsigned-char -fPIC -DNDEBUG -O0 -g -ggdb -fno-builtin-sin -fno-builtin-cos -ffast-math -fno-strict-aliasing -Wformat=2 -Wno-format-zero-length -Wformat-security -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute -Wdisabled-optimization") # -Werror-implicit-function-declaration -Qunused-arguments


SET(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake_modules)

AUX_SOURCE_DIRECTORY(code/qcommon QCOMMON_SRCS)
# exclude platform-dependent QVM bytecode compilers
list(FILTER QCOMMON_SRCS EXCLUDE REGEX ".*vm_[alx].*.c")
# add platform-dependent QVM bytecode compilers
IF(CMAKE_SYSTEM_PROCESSOR MATCHES AMD64|x86|i*86)
	IF(CMAKE_SYSTEM_PROCESSOR MATCHES AMD64)
		IF(WIN32)
			SET(BINEXT .x64)
		ELSE()
			SET(BINEXT .x86_64)
		ENDIF()
		SET(RENDEXT _x86_64)
	ELSE()
		#SET(BINEXT .x86)
		SET(RENDEXT _x86)
	ENDIF()
	list(APPEND QCOMMON_SRCS code/qcommon/vm_x86.c)
ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES aarch64|arm64)
	SET(BINEXT .aarch64)
	SET(RENDEXT _aarch64)
	list(APPEND QCOMMON_SRCS code/qcommon/vm_aarch64.c)
ELSEIF(CMAKE_SYSTEM_PROCESSOR MATCHES arm*)
	SET(BINEXT .arm)
	SET(RENDEXT _arm)
	list(APPEND QCOMMON_SRCS code/qcommon/vm_armv7l.c)
ELSE()
	TARGET_COMPILE_DEFINITIONS(qcommon PRIVATE -DNO_VM_COMPILED)
	TARGET_COMPILE_DEFINITIONS(qcommon_ded PRIVATE -DNO_VM_COMPILED)
ENDIF()

# botlib
AUX_SOURCE_DIRECTORY(code/botlib BOTLIB_SRCS)
ADD_LIBRARY(qsbotlib OBJECT ${BOTLIB_SRCS})
TARGET_COMPILE_DEFINITIONS(qsbotlib PRIVATE BOTLIB)

AUX_SOURCE_DIRECTORY(code/server SERVER_SRCS)
list(REMOVE_ITEM SERVER_SRCS code/server/sv_rankings.c)

# universal binary
ADD_LIBRARY(qsqcommon OBJECT ${QCOMMON_SRCS} ${SERVER_SRCS})

# dedicated server
if(QS_BUILD_DED)
ADD_LIBRARY(qcommon_ded OBJECT ${QCOMMON_SRCS} ${SERVER_SRCS})
TARGET_COMPILE_DEFINITIONS(qcommon_ded PUBLIC DEDICATED)
endif()

# client + cURL
AUX_SOURCE_DIRECTORY(code/client CLIENT_SRCS)
IF(NOT USE_CURL)
	list(REMOVE_ITEM CLIENT_SRCS code/client/cl_curl.c)
ENDIF()

#ADD_LIBRARY(client OBJECT ${CLIENT_SRCS})

# jpeg
IF(USE_SYSTEM_JPEG)
	ADD_LIBRARY(qsclient OBJECT ${CLIENT_SRCS})
	find_package(JPEG REQUIRED)
	TARGET_COMPILE_DEFINITIONS(qsclient PRIVATE -DUSE_SYSTEM_JPEG)
	TARGET_LINK_LIBRARIES(qsclient ${JPEG_LIBRARIES})
ELSE()
	AUX_SOURCE_DIRECTORY(code/libjpeg JPEG_SRCS)
	#ADD_LIBRARY(jpeg ${JPEG_SRCS})
	#TARGET_LINK_LIBRARIES(qsclient jpeg)
	ADD_LIBRARY(qsclient OBJECT ${CLIENT_SRCS} ${JPEG_SRCS})
ENDIF()

# renderers
AUX_SOURCE_DIRECTORY(code/renderercommon RENDERER_COMMON_SRCS)
AUX_SOURCE_DIRECTORY(code/renderer RENDERER_GL_SRCS)
AUX_SOURCE_DIRECTORY(code/renderervk RENDERER_VK_SRCS)

IF(USE_RENDERER_DLOPEN)
	SET(AUX_SRCS
		code/qcommon/q_shared.c
		code/qcommon/puff.c
		code/qcommon/q_math.c
	)
	TARGET_COMPILE_DEFINITIONS(qsclient PRIVATE USE_RENDERER_DLOPEN RENDERER_PREFIX="${RENDERER_PREFIX}" RENDERER_DEFAULT="${RENDERER_DEFAULT}")

	ADD_LIBRARY(${RENDERER_PREFIX}_opengl${RENDEXT} SHARED ${RENDERER_GL_SRCS} ${RENDERER_COMMON_SRCS} ${AUX_SRCS})
	TARGET_COMPILE_DEFINITIONS(${RENDERER_PREFIX}_opengl${RENDEXT} PRIVATE USE_RENDERER_DLOPEN)

	ADD_LIBRARY(${RENDERER_PREFIX}_vulkan${RENDEXT} SHARED ${RENDERER_VK_SRCS} ${RENDERER_COMMON_SRCS} ${AUX_SRCS})
	TARGET_COMPILE_DEFINITIONS(${RENDERER_PREFIX}_vulkan${RENDEXT} PRIVATE USE_RENDERER_DLOPEN)
ELSE()
	IF(USE_VULKAN)
		ADD_LIBRARY(${RENDERER_PREFIX}_vulkan OBJECT ${RENDERER_VK_SRCS} ${RENDERER_COMMON_SRCS})
	ELSE()
		ADD_LIBRARY(qs${RENDERER_PREFIX}_opengl OBJECT ${RENDERER_GL_SRCS} ${RENDERER_COMMON_SRCS})
	ENDIF()
ENDIF()

# common function, window system
IF(UNIX)
	if(ANDROID)
		SET (Q3_SRCS
				code/android/android_main.c
				code/android/android_shared.c
				code/android/android_signals.c
				code/android/ifaddrs.c
				)
		SET(Q3_UI_SRCS
				code/android/android_gamma.c
				code/android/android_input.c
				code/android/android_glimp.c
				code/android/android_snd.c
				)
	else(ANDROID)
	SET (Q3_SRCS
		code/unix/unix_main.c
		code/unix/unix_shared.c
		code/unix/linux_signals.c)

	IF(USE_SDL)
		AUX_SOURCE_DIRECTORY(code/sdl Q3_UI_SRCS)
	ELSE()
		SET(Q3_UI_SRCS
			code/unix/linux_glimp.c
			code/unix/linux_qgl.c
			code/unix/linux_snd.c
			code/unix/x11_dga.c
			code/unix/x11_randr.c
			code/unix/x11_vidmode.c)
		IF(USE_VULKAN)
			list(APPEND Q3_UI_SRCS code/unix/linux_qvk.c)
		ENDIF()
	ENDIF()

	IF(APPLE)
		SET(EXE_TYPE MACOSX_BUNDLE)
		list(APPEND Q3_SRCS code/unix/quake3_flat.icns)
		set_source_files_properties(code/unix/quake3_flat.icns PROPERTIES
			MACOSX_PACKAGE_LOCATION Resources)
		SET(MACOSX_BUNDLE_ICON_FILE quake3_flat.icns)
	ENDIF()
	endif(ANDROID)
ELSEIF(WIN32)
	SET(EXE_TYPE WIN32)
	IF(MSVC)
		SET(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} /MANIFEST:NO")
		ADD_COMPILE_DEFINITIONS(_CRT_SECURE_NO_WARNINGS) # v3.12+
	ENDIF()

	SET(Q3_SRCS
		code/win32/win_main.c
		code/win32/win_shared.c
		code/win32/win_syscon.c
		code/win32/win_resource.rc)

	SET(Q3_UI_SRCS
		code/win32/win_gamma.c
		code/win32/win_glimp.c
		code/win32/win_input.c
		code/win32/win_minimize.c
		code/win32/win_qgl.c
		code/win32/win_snd.c
		code/win32/win_wndproc.c)
	IF(USE_VULKAN)
		list(APPEND Q3_UI_SRCS code/win32/win_qvk.c)
	ENDIF()
ENDIF(UNIX)

ADD_LIBRARY(qsq3ui OBJECT ${Q3_UI_SRCS})

IF(NOT MSVC)
	IF(USE_SDL)
		find_package(SDL2 REQUIRED)
		TARGET_LINK_LIBRARIES(qsq3ui SDL2::Main)
	ELSEIF(UNIX)
		if(NOT ANDROID)
		find_package (X11 REQUIRED)
		TARGET_LINK_LIBRARIES(qsq3ui ${X11_LIBRARIES})
		endif(NOT ANDROID)
	ENDIF()
ENDIF()

IF(USE_VULKAN)
	TARGET_COMPILE_DEFINITIONS(qsq3ui PRIVATE USE_VULKAN_API)
	TARGET_COMPILE_DEFINITIONS(qsclient PRIVATE USE_VULKAN_API)
ENDIF()

# client executable

if(ANDROID)
	SET(QSLIBRARY quakesandbox)
	ADD_LIBRARY(${QSLIBRARY} SHARED ${Q3_SRCS})
	TARGET_LINK_LIBRARIES(${QSLIBRARY} qsqcommon qsbotlib qsclient qsq3ui ${qsclient_libs})
else(ANDROID)
	SET(QSLIBRARY ${CNAME}${BINEXT})
ADD_EXECUTABLE(${QSLIBRARY} ${EXE_TYPE} ${Q3_SRCS})
TARGET_LINK_LIBRARIES(${QSLIBRARY} qsqcommon qsbotlib qsclient qsq3ui ${qsclient_libs})
endif(ANDROID)
#TARGET_LINK_OPTIONS(${CNAME} PRIVATE -fno-pie) # v3.13+

IF(NOT USE_RENDERER_DLOPEN)
	IF(USE_VULKAN)
		TARGET_LINK_LIBRARIES(${QSLIBRARY} ${RENDERER_PREFIX}_vulkan)
	ELSE()
		TARGET_LINK_LIBRARIES(${QSLIBRARY} qs${RENDERER_PREFIX}_opengl)
	ENDIF()
ENDIF()

# dedicated server executable

if(QS_BUILD_DED)
if(NOT ANDROID)
ADD_EXECUTABLE(${DNAME}${BINEXT} ${EXE_TYPE} ${Q3_SRCS})
TARGET_LINK_LIBRARIES(${DNAME}${BINEXT} qcommon_ded qsbotlib)
#TARGET_LINK_OPTIONS(${DNAME} PRIVATE -fno-pie) # v3.13+

IF(WIN32)
	TARGET_LINK_LIBRARIES(${CNAME}${BINEXT} winmm comctl32 ws2_32)
	TARGET_LINK_LIBRARIES(${DNAME}${BINEXT} winmm comctl32 ws2_32)
ELSE()
	TARGET_LINK_LIBRARIES(${CNAME}${BINEXT} m ${CMAKE_DL_LIBS})
	TARGET_LINK_LIBRARIES(${DNAME}${BINEXT} m ${CMAKE_DL_LIBS})
ENDIF()
endif(NOT ANDROID)
endif()
