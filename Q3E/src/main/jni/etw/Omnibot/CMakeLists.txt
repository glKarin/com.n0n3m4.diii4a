cmake_minimum_required(VERSION 3.15)
cmake_policy(SET CMP0091 NEW)

project(Omnibot)

set(CMAKE_CXX_STANDARD 17) #//karin: using C++17 instead of boost
set(CMAKE_C_STANDARD 99)
set(CMAKE_CXX_STANDARD_REQUIRED ON) #//karin: using C++17 instead of boost

macro(add_cxx_flag flag)
    message(STATUS "Configure with flag '${flag}'")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}")
endmacro()

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# MSVC static runtime linkage
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(OMNIBOT_ET "Build the Enemy Territory bots module" ON)
option(OMNIBOT_RTCW "Build the Return to Castle Wolfenstein bots module" OFF) #//karin: only build ET
option(OMNIBOT_STATIC_BOOST "Use static linked boost libraries" ON)

string(TOLOWER "${CMAKE_SYSTEM_PROCESSOR}" SYSTEM_PROCESSOR_LOWER)
if (SYSTEM_PROCESSOR_LOWER MATCHES "(x86)|(amd64)" AND NOT APPLE)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        if (WIN32)
            string(APPEND MODULE_SUFFIX "_x64")
        else ()
            string(APPEND MODULE_SUFFIX ".x86_64")
        endif ()
    endif ()
elseif (APPLE)
    string(APPEND MODULE_SUFFIX "_mac")
endif ()

set(DEPENDENCIES_PATH "${CMAKE_CURRENT_SOURCE_DIR}/dependencies") #//karin: is subdirectory on idTech4A++

add_library(omni-shared INTERFACE)

set(USE_CXX17      ON) #//karin: using C++17 instead of boost
if(NOT USE_CXX17)
set(Boost_USE_STATIC_LIBS        ${OMNIBOT_STATIC_BOOST})  # only find static libs
set(Boost_USE_DEBUG_LIBS         OFF)  # ignore debug libs and
set(Boost_USE_RELEASE_LIBS       ON)  # only find release libs
set(Boost_USE_MULTITHREADED      ON)

if (WIN32)
    set(Boost_USE_STATIC_RUNTIME    ON)
else ()
    set(Boost_USE_STATIC_RUNTIME    OFF)
endif ()
find_package(Boost COMPONENTS system filesystem regex date_time REQUIRED)
target_link_libraries(omni-shared INTERFACE Boost::boost Boost::filesystem Boost::regex Boost::date_time)
endif()

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_definitions(-D_SIZEOFPTR=${CMAKE_SIZEOF_VOID_P})
if(CMAKE_CXX_BYTE_ORDER EQUAL BIG_ENDIAN)
    set(big_endian 1)
else()
    set(big_endian 0)
endif()
add_definitions(-D_IS_BIG_ENDIAN=${big_endian})

target_include_directories(omni-shared INTERFACE "${DEPENDENCIES_PATH}/wildmagic") #//karin: wildmagic include path
#target_include_directories(omni-shared INTERFACE "${DEPENDENCIES_PATH}/WildMagic5/SDK/include")

if (NOT WIN32)
    add_compile_options(-Wall -Wno-deprecated -fno-strict-aliasing)
    add_compile_options(-Wno-format-security -Wno-register) #//karin: reduce compilation warnings
    add_compile_options(-Wno-unused-private-field -Wno-unused-value -Wno-unused-local-typedef) #//karin: reduce compilation warnings
endif ()
#add_compile_options("$<$<CONFIG:DEBUG>:-D_DEBUG>") #//karin: build release

add_subdirectory(Common)

if (OMNIBOT_ET)
    add_subdirectory(ET)
    # Set ET as the startup project in Visual Studio
    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT omnibot-et)
endif ()

if (OMNIBOT_RTCW)
    add_subdirectory(RTCW)
    if (NOT OMNIBOT_ET)
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT omnibot-rtcw)
    endif ()
endif ()
