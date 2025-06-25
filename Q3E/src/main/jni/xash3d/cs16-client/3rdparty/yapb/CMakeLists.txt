#
# YaPB - Counter-Strike Bot based on PODBot by Markus Klinge.
#
# Copyright © 2023 Velaron.
# Copyright © YaPB Project Developers <yapb@jeefo.net>.
#
# SPDX-License-Identifier: MIT
#
cmake_minimum_required(VERSION 3.5)

project(cs16_yapb VERSION 4.5 LANGUAGES CXX)

if(NOT ANDROID)
   set(CMAKE_SHARED_MODULE_PREFIX "")
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(YAPB_SRC
    "src/analyze.cpp"
    "src/botlib.cpp"
    "src/chatlib.cpp"
    "src/combat.cpp"
    "src/config.cpp"
    "src/control.cpp"
    "src/engine.cpp"
    "src/fakeping.cpp"
    "src/graph.cpp"
    "src/hooks.cpp"
    "src/linkage.cpp"
    "src/manager.cpp"
    "src/module.cpp"
    "src/message.cpp"
    "src/navigate.cpp"
    "src/planner.cpp"
    "src/practice.cpp"
    "src/sounds.cpp"
    "src/storage.cpp"
    "src/support.cpp"
    "src/tasks.cpp"
    "src/vision.cpp"
    "src/vistable.cpp"
)

add_library(${PROJECT_NAME} MODULE ${YAPB_SRC})
find_package(Git QUIET)

if(GIT_FOUND)
    execute_process(COMMAND hostname OUTPUT_VARIABLE BUILD_MACHINE OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR} rev-parse --short HEAD OUTPUT_VARIABLE BUILD_HASH OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR} rev-list --count HEAD OUTPUT_VARIABLE BUILD_COUNT OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    execute_process(COMMAND git -C ${CMAKE_CURRENT_SOURCE_DIR} log --pretty="%ae" -1 OUTPUT_VARIABLE BUILD_AUTHOR OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)

    set(BUILD_COMPILER ${CMAKE_CXX_COMPILER_ID}\ ${CMAKE_CXX_COMPILER_VERSION})
    set(BUILD_VERSION ${PROJECT_VERSION})

    string(REPLACE . , BUILD_WINVER ${PROJECT_VERSION})

    configure_file(inc/version.h.in version.build.h @ONLY)
    target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION_GENERATED)
else()
    execute_process(COMMAND hostname OUTPUT_VARIABLE BUILD_MACHINE OUTPUT_STRIP_TRAILING_WHITESPACE ERROR_QUIET)
    set(BUILD_HASH "aeff9f8d4648c3a918b033339ec93c530d43a0de")
    set(BUILD_COUNT "1")
    set(BUILD_AUTHOR "yapb")

    set(BUILD_COMPILER ${CMAKE_CXX_COMPILER_ID}\ ${CMAKE_CXX_COMPILER_VERSION})
    set(BUILD_VERSION ${PROJECT_VERSION})

    string(REPLACE . , BUILD_WINVER ${PROJECT_VERSION})

    configure_file(inc/version.h.in version.build.h @ONLY)
    target_compile_definitions(${PROJECT_NAME} PRIVATE VERSION_GENERATED)
endif()

if(CMAKE_SYSTEM_PROCESSOR MATCHES "^(i386|x86|x86_64|X86|amd64|AMD64)" AND NOT CMAKE_LIBRARY_ARCHITECTURE MATCHES "^(arm|aarch64|ppc)")
    set(BUILD_X86 true)
endif()

if((CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU") AND NOT MSVC)
   if(ANDROID)
    target_compile_options(${PROJECT_NAME} PRIVATE -DCR_NATIVE_BUILD -fno-exceptions -fno-rtti -fno-threadsafe-statics -pthread) #  -flto=auto
   else()
    target_compile_options(${PROJECT_NAME} PRIVATE -DCR_NATIVE_BUILD -flto=auto -fno-exceptions -fno-rtti -fno-threadsafe-statics -pthread)
   endif()

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR MATCHES "arm64")
        target_compile_options(${PROJECT_NAME} PRIVATE -march=armv8-a+fp+simd)
    elseif(BUILD_X86)
        target_compile_options(${PROJECT_NAME} PRIVATE -mmmx -msse -msse2 -msse3 -mfpmath=sse)
    endif()

    if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE -funroll-loops -fomit-frame-pointer -fno-stack-protector -fvisibility=hidden -fvisibility-inlines-hidden -fno-math-errno)

        if(NOT WIN32 AND BUILD_X86)
            target_compile_options(${PROJECT_NAME} PRIVATE -fdata-sections -ffunction-sections -fcf-protection=none -fno-plt)
            target_link_options(${PROJECT_NAME} PRIVATE -Wl,--version-script=${CMAKE_CURRENT_SOURCE_DIR}/ext/ldscripts/version.lds -Wl,--gc-sections)

            if(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
                target_compile_options(${PROJECT_NAME} PRIVATE -fgraphite-identity -floop-nest-optimize)
                target_link_options(${PROJECT_NAME} PRIVATE -fgraphite-identity -floop-nest-optimize)
            endif()
        endif()

        if(NOT WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
            target_link_options(${PROJECT_NAME} PRIVATE -flto-partition=none)
        endif()
    else()
        target_compile_options(${PROJECT_NAME} PRIVATE -g3 -ggdb -DDEBUG -D_FORTIFY_SOURCE=2)
    endif()

    if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "GNU")
        target_link_options(${PROJECT_NAME} PRIVATE -Wl,--kill-at)

        if(CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12")
            target_link_options(${PROJECT_NAME} PRIVATE -Xlinker --script -Xlinker ${CMAKE_CURRENT_SOURCE_DIR}/ext/ldscripts/i386pe.lds)
        endif()
    endif()

    if(WIN32 AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_options(${PROJECT_NAME} PRIVATE -static-libgcc)
    endif()

    if(WIN32 AND CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()

    if(CMAKE_SIZEOF_VOID_P EQUAL 8 OR CMAKE_SYSTEM_PROCESSOR MATCHES "aarch64" OR CMAKE_SYSTEM_PROCESSOR STREQUAL "arm" OR CMAKE_SYSTEM_PROCESSOR MATCHES "^ppc")
        target_compile_options(${PROJECT_NAME} PRIVATE -fPIC)
        target_link_options(${PROJECT_NAME} PRIVATE -fPIC)
    endif()
elseif(WIN32 AND MSVC)
    set_property(TARGET ${PROJECT_NAME} PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

    if(NOT CMAKE_BUILD_TYPE MATCHES "Debug")
        target_compile_options(${PROJECT_NAME} PRIVATE /Zc:threadSafeInit- /GS- /Ob2 /Oy /Oi /Ot /fp:precise /GF /Gw /arch:SSE2 /Zi /guard:ehcont- /guard:cf- /DEBUG)
        target_link_options(${PROJECT_NAME} PRIVATE /OPT:REF,ICF /GUARD:NO /LTCG delayimp.lib /DELAYLOAD:user32.dll /DELAYLOAD:ws2_32.dll)

        if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
           target_compile_options(${PROJECT_NAME} PRIVATE /GL)
        endif()
     endif()
endif()

if(WIN32 OR MINGW)
    target_link_libraries(${PROJECT_NAME} PRIVATE user32 ws2_32)
    target_sources(${PROJECT_NAME} PRIVATE "vc/yapb.rc")
elseif(ANDROID)
    target_link_libraries(${PROJECT_NAME} PRIVATE m dl log)
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE m dl pthread)
endif()

target_include_directories(${PROJECT_NAME} PRIVATE
	${PROJECT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
	"inc"
	"ext"
	"ext/crlib"
	"ext/linkage"
)

if(COMMAND set_target_postfix)
    set_target_postfix(${PROJECT_NAME})
endif()

install(TARGETS ${PROJECT_NAME}
		DESTINATION "${GAME_DIR}/${SERVER_INSTALL_DIR}/"
		PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE
		GROUP_READ GROUP_EXECUTE
		WORLD_READ WORLD_EXECUTE)

if(MSVC)
	install(FILES $<TARGET_PDB_FILE:${PROJECT_NAME}>
		DESTINATION "${GAME_DIR}/${SERVER_INSTALL_DIR}/" OPTIONAL)
endif()
