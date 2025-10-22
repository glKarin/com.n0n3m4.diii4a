cmake_minimum_required(VERSION 3.22) #k 3.25

if(POLICY CMP0177)
    # Policy CMP0177: install() DESTINATION paths are normalized
    # Introduced in CMake 3.31
    cmake_policy(SET CMP0177 NEW)
endif()

set(DIII4A ON)
if(DIII4A)
add_definitions(-D_DIII4A) # android
add_definitions(-DUSE_OPENGLES) # OpenGLES 1.1
set(IOQ3_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(IOQ3_BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(PRODUCT_VERSION TRUE)
set(IOQ3_PREFIX "q3")
set(IOQ3_CFLAGS "-pipe -fsigned-char -fPIC -DNDEBUG -O3 -fno-builtin-sin -fno-builtin-cos -ffast-math -fno-strict-aliasing -Wformat=2 -Wno-format-zero-length -Wformat-security -Wno-format-nonliteral -Wstrict-aliasing=2 -Wmissing-format-attribute -Wdisabled-optimization -Wno-implicit-function-declaration") # -Qunused-arguments
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${IOQ3_CFLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${IOQ3_CFLAGS}")
list(APPEND CLIENT_INCLUDE_DIRS ${DIII4A_DEPEND_COMMON_PATH} ${CMAKE_CURRENT_SOURCE_DIR}/thirdparty)
list(APPEND CLIENT_LIBRARIES android log GLESv1_CM EGL oboeaudio)
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include(identity)

project(${PROJECT_NAME} VERSION ${PROJECT_VERSION} LANGUAGES C)

set(BUILD_SERVER OFF) # "Build dedicated server" #k ON
set(BUILD_CLIENT ON) # "Build client"
set(BUILD_RENDERER_GL1 ON) # "Build GL1 renderer"
set(BUILD_RENDERER_GL2 OFF) # "Build GL2 renderer" #k ON
set(BUILD_GAME_LIBRARIES ON) # "Build game module libraries"
set(BUILD_GAME_QVMS OFF) # "Build game module qvms" #k ON
set(BUILD_STANDALONE OFF) # "Build binaries for standalone games"

set(USE_RENDERER_DLOPEN OFF) # "Dynamically load the renderer(s)" #k ON
set(USE_OPENAL ON) # "OpenAL audio"
set(USE_OPENAL_DLOPEN ON) # "Dynamically load OpenAL"
set(USE_HTTP ON) # "HTTP download support"
set(USE_CODEC_VORBIS ON) # "Ogg Vorbis support"
set(USE_CODEC_OPUS ON) # "Ogg Opus support"
set(USE_VOIP ON) # "Voice chat"
set(USE_MUMBLE OFF) # "Mumble support" #k ON
set(USE_FREETYPE OFF) # "Freetype font rendering"

set(USE_INTERNAL_LIBS ON) # "Use internally packaged libraries"
set(USE_INTERNAL_SDL ${USE_INTERNAL_LIBS}) # "Use internal SDL binary (if available)"
set(USE_INTERNAL_ZLIB ${USE_INTERNAL_LIBS}) # "Use internal copy of zlib"
set(USE_INTERNAL_JPEG ${USE_INTERNAL_LIBS}) # "Use internal copy of libjpeg"
set(USE_INTERNAL_OGG ${USE_INTERNAL_LIBS}) # "Use internal copy of ogg"
set(USE_INTERNAL_VORBIS ${USE_INTERNAL_LIBS}) # "Use internal copy of vorbis"
set(USE_INTERNAL_OPUS ${USE_INTERNAL_LIBS}) # "Use internal copy of opus"

# Release build by default, set externally if you want something else
if(NOT CMAKE_CONFIGURATION_TYPES AND NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
if(NOT DIII4A)
set(CMAKE_SHARED_LIBRARY_PREFIX "")
endif()

if(NOT PRODUCT_VERSION)
    set(PRODUCT_VERSION "${CMAKE_PROJECT_VERSION}")

    if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
        execute_process(
            COMMAND git describe --tags --exact-match
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
            OUTPUT_VARIABLE GIT_TAG
            OUTPUT_STRIP_TRAILING_WHITESPACE
            ERROR_QUIET)

        if(NOT GIT_TAG STREQUAL "${PRODUCT_VERSION}")
            execute_process(
                COMMAND git show -s --pretty=format:%h
                WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
                OUTPUT_VARIABLE GIT_REV
                OUTPUT_STRIP_TRAILING_WHITESPACE
                ERROR_QUIET)

            if(GIT_REV)
                set(PRODUCT_VERSION "${PRODUCT_VERSION}_g${GIT_REV}")
            endif()
        endif()
    endif()
endif()

add_compile_definitions(PRODUCT_VERSION="${PRODUCT_VERSION}")

# For CI to read from
file(GENERATE OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/version.txt CONTENT ${PRODUCT_VERSION})

if(DEFINED ENV{SOURCE_DATE_EPOCH})
    string(TIMESTAMP PRODUCT_DATE "%b %d %Y" UTC)
    add_compile_definitions(PRODUCT_DATE="${PRODUCT_DATE}")
endif()

set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/code)

include(compilers/all)
include(platforms/all)
include(libraries/all)

include(server)
include(renderer_gl1)
include(renderer_gl2)
include(client)
include(basegame)
include(missionpack)

include(post_configure)
if(NOT DIII4A)
include(installer)
endif()
