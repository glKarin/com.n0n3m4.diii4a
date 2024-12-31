# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

project(idtech4amm)

add_definitions(-D__ANDROID__)
option(ANDROID "Android build" ON)
option(BUILD_TESTING "Build tests" OFF)

# backtrace for debug
add_subdirectory ( common/bt )

# Q3E jni engine
add_subdirectory( q3e )



OPTION(GAME_DOOM3       "Build DOOM 3/Quake 4/Prey(2006)"       ON)
OPTION(GAME_RTCW        "Build Return to Castle Wolfenstein"    ON)
OPTION(GAME_QUAKE3      "Build Quake 3"                         ON)
OPTION(GAME_QUAKE2      "Build Quake 2"                         ON)
OPTION(GAME_QUAKE1      "Build Quake 1"                         ON)
OPTION(GAME_TDM         "Build The Dark Mod"                    ON)
OPTION(GAME_DOOM3BFG    "Build DOOM 3 BFG"                      ON)
OPTION(GAME_DOOM        "Build GZDOOM"                          ON)
OPTION(GAME_ETW         "Build Wolfenstein: Enemy Territory"    ON)
OPTION(GAME_REALRTCW    "Build RealRTCW"                        ON)

OPTION(GAME_DNF         "Build Duke Nukem Forever"              OFF)

if(GAME_DOOM3)
    message(STATUS "Build DOOM 3/Quake 4/Prey(2006)")
    add_subdirectory ( doom3/neo ) # DOOM3 | RoE, Quake4, Prey(2006)
endif()

if(NOT GAME_DOOM3)
    message(STATUS "Build OpenAL alone")
    add_subdirectory(doom3/neo/externlibs/openal-soft)
endif()

if(GAME_QUAKE3 OR GAME_QUAKE2 OR GAME_QUAKE1 OR GAME_DNF OR GAME_TDM OR GAME_RTCW OR GAME_DOOM OR GAME_ETW OR GAME_REALRTCW)
    message(STATUS "Build extras depend shared libraries")
    add_subdirectory ( deplibs ) # common depend libraries
endif()

if(GAME_QUAKE3 OR GAME_QUAKE2 OR GAME_QUAKE1 OR GAME_RTCW OR GAME_ETW OR GAME_REALRTCW)
    message(STATUS "Build common static libraries")
    add_subdirectory (deplibs/oboe) # 1.8.1
    add_subdirectory ( common )
endif()

if(GAME_RTCW) # 438e7d413b5f7277187c35b032eb0ef9093ae778 | May 27, 2024 | All: Fix compilation on FreeBSD
    message(STATUS "Build Return to Castle Wolfenstein")
    add_subdirectory ( rtcw ) # RTCW
endif()

if(GAME_QUAKE3) # https://github.com/ioquake/ioq3/commit/3fb9006e6461b877b67781cd0f0d94032804a475
    message(STATUS "Build Quake 3")
    add_subdirectory ( quake3 ) # Quake 3 Arena | Team Arena
endif()

if(GAME_QUAKE2) # 8.41 QUAKE2_8_41 https://github.com/yquake2/yquake2/commit/141bf6e7ddf0ee0895783db1de90d96e6a483ab0
    message(STATUS "Build Quake 2")
    add_subdirectory ( quake2 ) # Quake 2
endif()

if(GAME_QUAKE1) # https://github.com/DarkPlacesEngine/DarkPlaces/commit/dbebfea96dae5c8c75a2f8680768edc7851801c2
    message(STATUS "Build Quake 1")
    add_subdirectory ( quake1 ) # Quake 1
endif()

if(GAME_TDM) # 2.12 | 5f4793f69bc0b61cbf2f921496455fb9e9296bcd | Feb 24, 2024 | Merged some changes from trunk.
    message(STATUS "Build The Dark Mod")
    add_subdirectory ( tdm ) # The Dark Mod
endif()

if(GAME_DOOM3BFG) # 1.4.0 | f81a8c1dd9b762774a233ba071837b2c7a374751 | Mar 6, 2022 | Updated release notes
    message(STATUS "Build DOOM 3 BFG")
    add_subdirectory ( doom3bfg/neo ) # RBDOOM3-BFG
endif()

if(GAME_DNF)
    message(STATUS "Build Duke Nukem Forever")
    add_subdirectory ( duke4/neo ) # DNF
endif()

if(GAME_DOOM) # g4.12.2 | 71c40432e5e893c629a1c9c76a523a0ab22bd56a | Apr 28, 2024 | Reworked clientside lerping
    if(CMAKE_SIZEOF_VOID_P EQUAL 8 #[[ OR TRUE]]) # GZDOOM only support 64bits system
        message(STATUS "Build GZDOOM")
        add_subdirectory ( gzdoom ) # GZDOOM
    else()
        message(WARNING "GZDOOM not support 32bits build!")
    endif()
endif()

if(GAME_ETW) # v2.82.1 | 0a24c70f84cf21940277ddabd736ce83535cb794 | Apr 19, 2024 | Incrementing version number to 2.82.1
    message(STATUS "Build Wolfenstein: Enemy Territory")
    add_subdirectory ( etw ) # ETW
endif()

if(GAME_REALRTCW) # 5.0
    message(STATUS "Build RealRTCW")
    add_subdirectory ( realrtcw ) # RealRTCW
endif()
