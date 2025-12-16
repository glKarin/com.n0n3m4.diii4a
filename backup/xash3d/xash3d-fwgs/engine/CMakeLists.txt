cmake_minimum_required(VERSION 3.9)
project (xash3d-fwgs-engine)

include(CheckCXXCompilerFlag)

file(GLOB FWGS_ENGINE_INCLUDES *.h)
file(GLOB FWGS_ENGINE_COMMON_SOURCES common/*.c)
file(GLOB FWGS_ENGINE_COMMON_IMAGELIB_SOURCES common/imagelib/*.c)
file(GLOB FWGS_ENGINE_COMMON_SOUNDLIB_SOURCES common/soundlib/*.c)
file(GLOB FWGS_ENGINE_SERVER_SOURCES server/*.c)
file(GLOB FWGS_ENGINE_CLIENT_SOURCES client/*.c client/*/*.c client/*/*/*.c)
file(GLOB FWGS_ENGINE_PLATFORM_ANDROID_SOURCES platform/android/*.c platform/linux/*.c platform/posix/*.c)
file(GLOB FWGS_ENGINE_PLATFORM_SDL_SOURCES platform/sdl2/*.c)

if(HAVE_CURL)
set(FWGS_ENGINE_COMMON_HTTP_SOURCES common/http/net_http_curl.c)
set(XASH3D_FWGS_ENGINE_CURL_LIB "CURL")
else()
set(FWGS_ENGINE_COMMON_HTTP_SOURCES common/http/net_http_xash.c)
endif()

add_library (xash3d SHARED ${FWGS_ENGINE_COMMON_SOURCES} ${FWGS_ENGINE_COMMON_IMAGELIB_SOURCES} ${FWGS_ENGINE_COMMON_SOUNDLIB_SOURCES} ${FWGS_ENGINE_PLATFORM_SDL_SOURCES} ${FWGS_ENGINE_SERVER_SOURCES} ${FWGS_ENGINE_CLIENT_SOURCES} ${FWGS_ENGINE_PLATFORM_ANDROID_SOURCES} ${FWGS_ENGINE_COMMON_HTTP_SOURCES})
target_include_directories(xash3d PRIVATE . server client client/vgui common/soundlib platform common ../common common/imagelib ../pm_shared ../engine ../public ../filesystem ../3rdparty/bzip2/bzip2 ../3rdparty/MultiEmulator/include ../3rdparty/library_suffix/include)
target_link_libraries(xash3d SDL2 log android fwgs_public fwgs_ogg fwgs_vorbis fwgs_vorbisenc fwgs_vorbisfile fwgs_opus fwgs_opusfile fwgs_bzip2 fwgs_MultiEmulator fwgs_libbacktrace fwgs_library_suffix ${XASH3D_FWGS_ENGINE_CURL_LIB})
set_target_properties(xash3d PROPERTIES COMPILE_DEFINITIONS "XASH_ENABLE_MAIN=1;XASH_SDLMAIN=1;ENGINE_DLL=1;XASH_SDL=2;XASH_REF_NANOGL_ENABLED=1;XASH_REF_GLES3COMPAT_ENABLED=1;XASH_REF_GL4ES_ENABLED=1;XASH_REF_SOFT_ENABLED=1")






