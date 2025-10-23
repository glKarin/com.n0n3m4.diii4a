# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

project(idTech4Amm)


message(STATUS "Product flavor: ${DIII4A_PRODUCT_FLAVOR}")
if(DIII4A_PRODUCT_FLAVOR STREQUAL "fdroid")
    set(PRODUCT_FLAVOR_IS_GITHUB False)
    set(PRODUCT_FLAVOR_IS_FDROID True)
else()
    set(PRODUCT_FLAVOR_IS_GITHUB True)
    set(PRODUCT_FLAVOR_IS_FDROID False)
endif()

#[[OPTION(BUILD_D3      "Build DOOM 3"                          OFF)
OPTION(BUILD_Q4         "Build Quake 4"                         OFF)
OPTION(BUILD_PREY       "Build Prey"                            OFF)
OPTION(BUILD_D3_MOD		"Build all DOOM3 mods"                  OFF)
OPTION(BUILD_Q4_MOD	    "Build all Quake4 mods"                 OFF)]]
OPTION(BUILD_BACKPORT   "Build backport version"                OFF)


# Supported
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
OPTION(GAME_FTEQW       "Build FTEQW"                           OFF)
OPTION(GAME_JK          "Build Jedi Knight"                     ON)
OPTION(GAME_SERIOUSSAM  "Build Serious Sam"                     ON)
OPTION(GAME_URT         "Build Urban Terror"                    ON)
OPTION(GAME_OPENMOHAA   "Build Medal of Honor: Allied Assault"  ON)
if(NOT PRODUCT_FLAVOR_IS_FDROID)
OPTION(GAME_XASH3D                  "Build Xash3D"                                  ON)
OPTION(GAME_SOURCE                  "Build Source"                                  ON)
endif()

# Experimental
OPTION(GAME_Q3RALLY     "Build Quake 3 Rally"                   OFF)
OPTION(GAME_QUAKES      "Build Quake SandBox"                   OFF)
OPTION(GAME_DNF         "Build Duke Nukem Forever"              OFF)



# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_definitions(-D__ANDROID__)
option(ANDROID "Android build" ON)
option(BUILD_TESTING "Build tests" OFF)

set(DIII4A_DEPEND_LIBRARY_PATH "${CMAKE_CURRENT_SOURCE_DIR}/deplibs")
set(DIII4A_DEPEND_INCLUDE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/depincs")
set(DIII4A_DEPEND_COMMON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/common")

# backtrace for debug
add_subdirectory ( common/bt )

# using Q3E_SDL2
#if(GAME_XASH3D OR GAME_SOURCE)
set(Q3E_SDL TRUE)
#endif()

# build SDL2
if(Q3E_SDL)
    set(Q3E_SDL2_INCLUDE_PATH "${DIII4A_DEPEND_LIBRARY_PATH}/SDL2/include")
    add_subdirectory(deplibs/SDL2)
    set(SDL2_FOUND True)
    set(SDL2_LIBRARIES "SDL2")
    set(SDL2_INCLUDE_DIRS "${Q3E_SDL2_INCLUDE_PATH}")
    #add_subdirectory(deplibs/SDL2/q3e_test)
endif()

# Q3E jni engine
add_subdirectory( q3e )



if(GAME_QUAKE3 OR GAME_QUAKE2 OR GAME_QUAKE1 OR GAME_RTCW OR GAME_ETW OR GAME_REALRTCW OR GAME_FTEQW OR GAME_QUAKES OR GAME_JK OR GAME_Q3RALLY OR GAME_URT OR GAME_OPENMOHAA OR GAME_SERIOUSSAM)
    set(DIII4A_BUILD_COMMON True)
endif()

if(GAME_DOOM3 OR GAME_DNF OR GAME_TDM OR GAME_DOOM OR DIII4A_BUILD_COMMON)
    set(DIII4A_BUILD_OPENAL True)
endif()

if(DIII4A_BUILD_OPENAL)
    message(STATUS "Build oboe for audio")
    add_subdirectory (deplibs/oboe) # 1.8.1
endif()

if(GAME_DOOM3)
    message(STATUS "Build DOOM 3/Quake 4/Prey(2006)")
    add_subdirectory ( doom3/neo ) # DOOM3 | RoE, Quake4, Prey(2006)
endif()

if(DIII4A_BUILD_COMMON)
    message(STATUS "Build common static libraries")
    add_subdirectory ( common )
endif()

if(NOT GAME_DOOM3 AND DIII4A_BUILD_OPENAL)
    message(STATUS "Build OpenAL alone")
    add_subdirectory(doom3/neo/externlibs/openal-soft)
endif()

if(GAME_QUAKE3 OR GAME_QUAKE2 OR GAME_QUAKE1 OR GAME_DNF OR GAME_TDM OR GAME_RTCW OR GAME_DOOM OR GAME_ETW OR GAME_REALRTCW OR GAME_FTEQW OR GAME_QUAKES OR GAME_JK OR GAME_Q3RALLY OR GAME_URT OR GAME_OPENMOHAA OR GAME_SOURCE)
    message(STATUS "Build extras depend shared libraries")
    add_subdirectory ( deplibs ) # common depend libraries
endif()



if(GAME_RTCW) # 438e7d413b5f7277187c35b032eb0ef9093ae778 | May 27, 2024 | All: Fix compilation on FreeBSD
    message(STATUS "Build Return to Castle Wolfenstein")
    add_subdirectory ( rtcw ) # RTCW
endif()

if(GAME_QUAKE3) # https://github.com/ioquake/ioq3/commit/3fb9006e6461b877b67781cd0f0d94032804a475
    message(STATUS "Build Quake 3")
    add_subdirectory ( quake3 ) # Quake 3 Arena | Team Arena
endif()

if(GAME_QUAKE2) # 8.51 | dc473a9830fd705430d2aef1f78f23d6777216a3 | May 11, 2025 | Bump version number to 8.51.
    message(STATUS "Build Quake 2")
    add_subdirectory ( quake2 ) # Quake 2
endif()

if(GAME_QUAKE1) # https://github.com/DarkPlacesEngine/DarkPlaces/commit/dbebfea96dae5c8c75a2f8680768edc7851801c2
    message(STATUS "Build Quake 1")
    add_subdirectory ( quake1 ) # Quake 1
endif()

if(GAME_TDM) # 2.13 | 2da36cc9e83046160b91bca91c116dc642740b78 | Mar 8, 2025 | Merge one commit from trunk.
    message(STATUS "Build The Dark Mod")
    add_subdirectory ( darkmod ) # The Dark Mod
#[[    if(BUILD_BACKPORT)
        # 2.12 | 5f4793f69bc0b61cbf2f921496455fb9e9296bcd | Feb 24, 2024 | Merged some changes from trunk.
        add_subdirectory ( backports/darkmod_2_12 "darkmod_2_12" )
    endif()]]
endif()

if(GAME_DOOM3BFG) # 1.4.0 | f81a8c1dd9b762774a233ba071837b2c7a374751 | Mar 6, 2022 | Updated release notes
    message(STATUS "Build DOOM 3 BFG")
    add_subdirectory ( doom3bfg ) # RBDOOM3-BFG
endif()

if(GAME_DOOM) # g4.14.0 | ac58779e282f42093c2f7d47298db28203a58c15 | Feb 28, 2025 | - renormalize normals on scaled models
    if(CMAKE_SIZEOF_VOID_P EQUAL 8 #[[ OR TRUE]]) # GZDOOM only support 64bits system
        message(STATUS "Build GZDOOM")
        add_subdirectory ( gzdoom ) # GZDOOM

        # g4.12.2 | 71c40432e5e893c629a1c9c76a523a0ab22bd56a | Apr 28, 2024 | Reworked clientside lerping
        # g4.14.0 | 25ec8a689d5654a7f57869e09c0ce0d0892ba6a4 | Dec 18, 2024 | - language update
    else()
        message(WARNING "GZDOOM not support 32bits build!")
    endif()
endif()

if(GAME_ETW) # v2.83.1 | e8d0ec06800630fd458791ccd29ed488e04648d9 | Nov 12, 2024 | Incrementing version number to 2.83.1
    message(STATUS "Build Wolfenstein: Enemy Territory")
    add_subdirectory ( etw ) # ETW
endif()

if(GAME_REALRTCW) # 5.2 | Jul 26, 2025 | ad529d588e4dec717aff0779f9d1555eddc1e2be | Update ai_cast_survival.c
    message(STATUS "Build RealRTCW")
    add_subdirectory ( realrtcw ) # RealRTCW
#[[    if(BUILD_BACKPORT)
        add_subdirectory ( backports/realrtcw_5_1 "realrtcw_5_1" ) # 5.1 | on Feb 14, 2025 | a6e94b82bd11c41f62ffc969e27b0e63f267ef8e | Version is upped to 5.1
    endif()]]
endif()

if(GAME_FTEQW) # 5662b3a23d3c16f5a3b5fe74efb88e77d8fc4da0 | engine/common/fs.c: Update game definitions
    message(STATUS "Build FTEQW")
    add_subdirectory ( fteqw ) # FTEQW
endif()

if(GAME_JK) # d1cb662f07dfa4c1999edfb5c1a86fd1c6285372 | 2 weeks ago | byBlank can be allocated on autosave and it can still fail to read the levelshot image.
    message(STATUS "Build OpenJK")
    add_subdirectory ( jk ) # Jedi Knight
endif()


if(GAME_SERIOUSSAM) # v1.10.7 | Update README.md | https://github.com/tx00100xt/SeriousSamClassic/commit/871c9ebf1af0a09a976a080cbdc2ab6e7a21ab57
    message(STATUS "Build Serious Sam")
    OPTION(GAME_SAMTFE  "Build Serious Sam Classic The First Encounter"                     ON)
    OPTION(GAME_SAMTSE  "Build Serious Sam Classic The Second Encounter"                    ON)
    add_subdirectory ( serioussam ) # Serious-Engine
endif()


if(GAME_XASH3D) # xash3d-fwgs | https://github.com/FWGS/xash3d-fwgs/commit/74b196d2c362ea195967be6f139faeed18795a53
    message(STATUS "Build Xash3D")
    add_subdirectory(xash3d) # Xash3D
endif()

if(GAME_SOURCE) # 29985681a18508e78dc79ad863952f830be237b6 | Dec 30, 2023 | Merge pull request #312 from AruMoon/add-master-server Add new master server
	message(STATUS "Build Source")
    add_subdirectory(source) # Source-Engine
endif()

if(GAME_Q3RALLY) # v0.5b | Aug 5, 2025 | https://github.com/Q3Rally-Team/q3rally/commit/d495c7518dc17d24357de1df5b308ba251994d1e
    message(STATUS "Build Q3Rally")
    add_subdirectory ( q3rally/engine ) # Quake 3 Rally
endif()

if(GAME_URT) # 4.3.4 | May 26, 2018 | https://github.com/FrozenSand/ioq3-for-UrbanTerror-4/commit/53fdee781c263168965f48cb87cc9187705f5ca3 | Removed dmflags from the serverinfo string, it's an unused leftover from Quake 2
    message(STATUS "Build Urban Terror")
    add_subdirectory ( urt ) # Urban Terror
endif()

if(GAME_OPENMOHAA) # 0.82.1 | Aug 3, 2025 | https://github.com/openmoh/openmohaa/commit/a72bc153db1e6ced6e30431b5217e1a53576712c | Fix configstring replacement causing the model to be referenced twice on base MOHAA clients
    message(STATUS "Build Medal of Honor: Allied Assault")
    add_subdirectory ( openmohaa ) # Medal of Honor: Allied Assault
endif()



if(GAME_DNF)
    message(STATUS "Build Duke Nukem Forever")
    add_subdirectory ( duke4/neo ) # DNF
endif()

if(GAME_QUAKES) # noire-dev/SourceTech/commit/b0a1d46740d5a76f172a2889f285214a95dd3702 noire-dev/QuakeSandbox/commit/f01cab6d123784f6e2aaa839e9ccd3ec0a6f1325
    message(STATUS "Build Quake SandBox")
    add_subdirectory ( quakes ) # Quake SandBox
endif()
