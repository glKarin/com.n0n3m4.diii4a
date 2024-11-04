# Omnibot common build

file(GLOB COMMON_SRC 
	# CONFIGURE_DEPENDS #//karin: not changes
    ${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/gm/*.cpp
    ${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/platform/cmake/*.cpp #//karin: using cmake defines check little/big ndian and 32/64 bits, and stdint.h
    ${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/binds/*.cpp
    ${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/binds/gmSqliteLib.cpp
    ${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/3rdParty/mathlib/*.cpp
    ${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/3rdParty/gmbinder2/*.cpp
    ${DEPENDENCIES_PATH}/wildmagic/*.cpp
    ${DEPENDENCIES_PATH}/physfs/*.c
    ${DEPENDENCIES_PATH}/physfs/archivers/*.c
    ${DEPENDENCIES_PATH}/physfs/archivers/*.c
    ${DEPENDENCIES_PATH}/physfs/lzma/C/*.c
    ${DEPENDENCIES_PATH}/physfs/lzma/C/Archive/7z/*.c
    ${DEPENDENCIES_PATH}/physfs/lzma/C/Compress/Lzma/*.c
    ${DEPENDENCIES_PATH}/physfs/lzma/C/Compress/Branch/*.c
    ${DEPENDENCIES_PATH}/physfs/zlib123/*.c
    ${DEPENDENCIES_PATH}/physfs/platform/*.c

    # ${DEPENDENCIES_PATH}/Recast/Recast/Source/*.cpp
    # ${DEPENDENCIES_PATH}/Recast/Detour/Source/*.cpp
    # ${DEPENDENCIES_PATH}/Recast/DebugUtils/Source/*.cpp

    # ${CMAKE_CURRENT_SOURCE_DIR}/RecastInterfaces.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/BatchBuild.cpp
)

list(FILTER COMMON_SRC EXCLUDE REGEX ".*gmSqliteLib\\.cpp$")

add_library(omnibot-common STATIC ${COMMON_SRC})
set_property(TARGET omnibot-common PROPERTY CXX_STANDARD 17) #//karin: using C++17 instead of boost
set_property(TARGET omnibot-common PROPERTY C_STANDARD 99)

target_include_directories(omnibot-common
    PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"

    PUBLIC "${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/3rdParty"
    # PUBLIC "${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/3rdParty/mathlib"
    PUBLIC "${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/gm"
    PUBLIC "${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/binds"
    PUBLIC "${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/3rdParty/gmbinder2"

    PUBLIC "${DEPENDENCIES_PATH}/wildmagic"
    PUBLIC "${DEPENDENCIES_PATH}/iprof"

    # PUBLIC "${DEPENDENCIES_PATH}/Recast/Recast/Include"
    # PUBLIC "${DEPENDENCIES_PATH}/Recast/DebugUtils/Include"
    # PUBLIC "${DEPENDENCIES_PATH}/Recast/Detour/Include"
    PUBLIC "${DEPENDENCIES_PATH}/physfs"
    PUBLIC "${DEPENDENCIES_PATH}/physfs/lzma/C"
    PUBLIC "${DEPENDENCIES_PATH}/physfs/zlib123"
    PUBLIC "${DEPENDENCIES_PATH}/physfs/lzma/C/Archive/7z"
    PUBLIC "${DEPENDENCIES_PATH}/physfs/lzma/C/Compress/Lzma"
    PUBLIC "${DEPENDENCIES_PATH}/physfs/lzma/C/Compress/Branch"
)

if(ANDROID) #//karin: using cmake defaines check little/big ndian and 32/64 bits, and stdint.h
    target_include_directories(omnibot-common PUBLIC "${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/platform/cmake")
elseif (WIN32)
    if (MSVC)
        target_include_directories(omnibot-common PUBLIC "${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/platform/win32msvc")
    else()
        target_include_directories(omnibot-common PUBLIC "${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/platform/win32gcc")
    endif()
elseif(APPLE)
    target_include_directories(omnibot-common PUBLIC "${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/platform/osx")
else()
    target_include_directories(omnibot-common PUBLIC "${DEPENDENCIES_PATH}/gmscriptex/gmsrc_ex/src/platform/win32gcc")
endif()

target_link_libraries(omnibot-common PUBLIC omni-shared)
target_compile_definitions(omnibot-common PUBLIC PHYSFS_SUPPORTS_ZIP=1 PHYSFS_SUPPORTS_7Z=1 PHYSFS_NO_CDROM_SUPPORT)

if (UNIX)
    set_target_properties(omnibot-common PROPERTIES COMPILE_FLAGS -pthread -ffriend-injection -fno-strict-aliasing)
endif ()

if(APPLE)
    find_library(CARBON_FW Carbon)
    target_link_libraries(omnibot-common PUBLIC ${CARBON_FW})
    find_library(IOKIT_FW IOKit)
    target_link_libraries(omnibot-common PUBLIC ${IOKIT_FW})
endif()
