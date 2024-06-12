cmake_minimum_required(VERSION 3.22)

project(DNF)

# build type
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release")
endif()
#set(CMAKE_BUILD_TYPE "Debug")
#set(CMAKE_VERBOSE_MAKEFILE ON)
#link_directories("${PROJECT_SOURCE_DIR}/../../libs")

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
add_definitions(-DARCH_STRING="${ARCH}")
add_definitions(-D_SIZEOFPTR=${CMAKE_SIZEOF_VOID_P})
add_definitions(-DUSE_OPENAL -DUSE_FFMPEG)

#add_compile_options(-std=c++14)
#add_compile_options(-w)
add_compile_options(-fwritable-strings -Wno-format-security -Wno-inconsistent-missing-override)
add_compile_options(-fdeclspec)
#add_definitions(-O0 -g -ggdb)
add_definitions(-O3)
add_link_options("-Wl,-g")

# Output binaries to above directory.
if(NOT ANDROID)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/..)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
endif()

IF( WIN32 )
    SET ( CMAKE_SHARED_LINKER_FLAGS /MANIFEST:NO )
ENDIF( WIN32 )

if(MSVC)
# Allow addressing more memory on a 64-bit host.
add_link_options("/LARGEADDRESSAWARE")

# Project is not compatible with data execution prevention.
add_link_options("/NXCOMPAT:NO")

# Project is not compatible with safe exception handlers. 
add_link_options("/SAFESEH:NO")
endif()

if(NOT ANDROID)
include_directories(external/bink;external/detour/include;external/glew;external/recast/include)
link_directories(external/bink;)
endif()

file(GLOB idLibFiles
     "idlib/*.cpp*"
     "idlib/bv/*.cpp*"
     "idlib/containers/*.cpp*"
     "idlib/geometry/*.cpp*"
     "idlib/hashing/*.cpp*"
     "idlib/math/*.cpp*"  
)

file(GLOB dnGameFiles
     "dukegame/*.cpp*"
     "dukegame/Civilians/*.cpp*"
     "dukegame/ClientGame/*.cpp*"
     "dukegame/DnAI/*.cpp*"     
     "dukegame/Monsters/*.cpp*"
     "dukegame/Components/*.cpp*"
     "dukegame/GameSys/*.cpp*"
     "dukegame/Player/*.cpp*"
     "dukegame/Weapons/*.cpp*"
     "dukegame/Renderer/*.cpp*"
     "dukegame/Items/*.cpp*"
     "dukegame/Editor/*.cpp*"
     "dukegame/Entities/*.cpp*"
     "dukegame/Decls/*.cpp*"
)

file(GLOB gameBaseFiles
     "dukegame/Gamelib/*.cpp*"
     "dukegame/Gamelib/anim/*.cpp*"
     "dukegame/Gamelib/gamesys/*.cpp*"
     "dukegame/Gamelib/player/*.cpp"
     "dukegame/Gamelib/physics/*.cpp"
     "dukegame/Gamelib/script/*.cpp"
) 

file(GLOB typeInfoGenFiles
    "engine/typeinfo/*.cpp*"
)

file(GLOB gameTypeInfoFiles
    "dukegame/Typeinfo/*.cpp*"
)

set (toolSupportSrc 
    engine/framework/CmdSystem.cpp
    engine/framework/CVarSystem.cpp
    engine/framework/File.cpp
    engine/framework/FileSystem.cpp
    engine/framework/Unzip.cpp
)

# idLib project
if(MSVC)
add_definitions(-D_AFXDLL -D_WINDOWS -D_CRT_SECURE_NO_DEPRECATE)
endif()
add_library(duke4_idLib STATIC ${idLibFiles})
target_precompile_headers(duke4_idLib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/idlib/precompiled.h)
if(MSVC)
target_compile_definitions(duke4_idLib PRIVATE NAME=$<TARGET_FILE_NAME:idLib>)
set_target_properties(duke4_idLib PROPERTIES LINK_FLAGS "/PDB:\"idlib.pdb\"")
endif()

add_library(gameTypeInfo STATIC ${gameTypeInfoFiles})
target_compile_definitions(gameTypeInfo PRIVATE GAME_DLL=1 -D_D3XP -DCTF)
if(MSVC)
set_target_properties(gameTypeInfo PROPERTIES LINK_FLAGS "/PDB:\"gameTypeInfo.pdb\"")
endif()


if(NOT ANDROID)
# Type InfoGen Project
add_executable(typeinfogen ${typeInfoGenFiles} ${toolSupportSrc})
target_compile_definitions(typeinfogen PUBLIC -DID_TYPEINFO -D__DOOM_DLL__)
target_link_libraries(typeinfogen duke4_idLib)
target_precompile_headers(typeinfogen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/idlib/precompiled.h)
endif()

# Game Project
add_library(duke4game SHARED  ${dnGameFiles} ${gameBaseFiles} )
target_compile_definitions(duke4game PRIVATE GAME_DLL=1 -D_D3XP -DCTF)
target_link_libraries(duke4game duke4_idLib gameTypeInfo)
target_precompile_headers(duke4game PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/idlib/precompiled.h)
if(MSVC)
set_target_properties(duke4game PROPERTIES OUTPUT_NAME "gamex86" LINK_FLAGS "/PDB:\"gamex86.pdb\"")
endif()

if(NOT ANDROID)
add_custom_target(
    GameCustomTarget
    COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/../typeinfogen.exe
    OUTPUT  ${CMAKE_CURRENT_BINARY_DIR}/__header.h
)

add_dependencies(gameTypeInfo GameCustomTarget)
endif()

# Engine Project
file(GLOB engineSourceFiles
    "engine/cm/*.cpp*"
    "engine/external/detour/Source/*.cpp*"
    "engine/external/recast/Source/*.cpp*"
    "engine/decls/*.cpp*"
    "engine/framework/*.cpp*"
    "engine/framework/async/*.cpp*"
    "engine/models/*.cpp*"
    "engine/models/skeletal/*.cpp*"
    "engine/models/static/*.cpp*"
    "engine/navigation/*.cpp*"
    "engine/renderer/*.cpp*"
    "engine/renderer/ImageTools/*.cpp*"
    "engine/renderer/ImageTools/Color/*.cpp*"
    "engine/renderer/jobs/transcode/image/*.cpp*"
    "engine/renderer/jobs/render/*.cpp*"
    "engine/threads/*.cpp*"
	"engine/sound/*.cpp*"
    "engine/sound/OpenAL/*.cpp*"
    "engine/sys/*.cpp*"
    #"engine/sys/win32/*.cpp*"
    "engine/sys/android/*.cpp*"
    "engine/sys/posix/*.cpp*"
    "engine/ui/*.cpp*"
    "engine/tools/compilers/dmap/*.cpp*"
    "engine/tools/compilers/navmesh/*.cpp*"
    "engine/renderer/*.c"
)

file(GLOB toolSourceFiles
    "engine/tools/*.cpp*"
    "engine/tools/af/*.cpp*"
    "engine/tools/comafx/*.cpp*"
    "engine/tools/radiant/*.cpp*"
    "engine/tools/common/*.cpp*"
    "engine/tools/guis/*.cpp*"
    "engine/tools/common/PropTree/*.cpp*"
    "engine/tools/compilers/dmap/*.cpp*"
    "engine/tools/compilers/navmesh/*.cpp*"
    "engine/tools/guied/*.cpp*"
    "engine/tools/importpipeline/*.cpp*"
    "engine/tools/materialeditor/*.cpp*"
    "engine/tools/particle/*.cpp*"
    "engine/tools/pda/*.cpp*"
    "engine/tools/script/*.cpp*"
    "engine/tools/sound/*.cpp*"
)

file(GLOB engineDependFiles
    #"engine/external/glew/*.c*"
    "engine/external/mikktspace/*.c*"
    "engine/external/oggvorbis/oggsrc/*.c*"
    "engine/external/oggvorbis/vorbissrc/*.c*"
)

set(src_imgui
    "./engine/external/imgui/imconfig.h"
	"./engine/external/imgui/imgui.cpp"
	"./engine/external/imgui/imgui.h"
	"./engine/external/imgui/imgui_draw.cpp"
	"./engine/external/imgui/imgui_internal.h"
	"./engine/external/imgui/imgui_widgets.cpp"
	"./engine/external/imgui/imstb_rectpack.h"
	"./engine/external/imgui/imstb_textedit.h"
	"./engine/external/imgui/imstb_truetype.h"
	#"./engine/external/imgui/examples/imgui_impl_opengl3.cpp"
	#"./engine/external/imgui/examples/imgui_impl_opengl3.h"
	#"./engine/external/imgui/examples/imgui_impl_win32.cpp"
	#"./engine/external/imgui/examples/imgui_impl_win32.h"
)

add_library(engineExternal STATIC ${engineDependFiles} ${src_imgui})
target_include_directories(engineExternal PRIVATE engine/external/oggvorbis/vorbissrc)
if(MSVC)
set_target_properties(engineExternal PROPERTIES LINK_FLAGS "/PDB:\"engineExternal.pdb\"")
endif()

#link_directories("../libs")
# Engine Project
add_library(DukeNukemForever SHARED ${engineSourceFiles})
target_compile_definitions(DukeNukemForever PUBLIC -D__DOOM_DLL__)
target_compile_definitions(DukeNukemForever PRIVATE -DUSE_CORE_PROFILE;-D_NO_PTHREAD_CANCEL)
add_dependencies(DukeNukemForever duke4game)
target_include_directories(DukeNukemForever PUBLIC engine/external/minidx9/include/)
target_link_directories(DukeNukemForever PUBLIC engine/external/bink/)
target_link_libraries(DukeNukemForever duke4_idLib engineExternal)
if(MSVC)
target_link_libraries(DukeNukemForever idLib engineExternal delayimp.lib opengl32.lib binkw32.lib dinput8.lib dxguid.lib winmm.lib wsock32.lib dsound.lib glu32.lib dbghelp.lib iphlpapi.lib)
else()
target_link_libraries(DukeNukemForever dl EGL android openal)
target_include_directories(DukeNukemForever PRIVATE "../../common")
target_include_directories(DukeNukemForever PRIVATE "../../depincs/openal")
target_include_directories(DukeNukemForever PRIVATE "engine/external/ffmpeg/include")
target_link_libraries(DukeNukemForever avcodec avformat swscale avdevice avutil avfilter swresample)
#target_link_libraries(DukeNukemForever ogg vorbis vorbisfile)
endif()
target_precompile_headers(DukeNukemForever PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/idlib/precompiled.h)
if(MSVC)
set_target_properties(DukeNukemForever PROPERTIES OUTPUT_NAME "DukeNukemForever" LINK_FLAGS "/DelayLoad:ComCtl32.dll /STACK:16777216,16777216 /SUBSYSTEM:WINDOWS /PDB:\"DukeNukemForever.pdb\"")
endif()

