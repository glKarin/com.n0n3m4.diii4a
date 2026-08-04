cmake_minimum_required (VERSION 3.9.6)
cmake_policy(SET CMP0091 NEW)   # for MSVC_RUNTIME_LIBRARY
project(tdm_package)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


option(TDM_THIRDPARTY_ARTEFACTS "Use prebuilt artefacts at ThirdParty/artefacts?" ON)

if(TDM_THIRDPARTY_ARTEFACTS)
    include(../ThirdParty/use_artefacts.cmake)
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "" FORCE) 


set(sources
    tdm_package.cpp
    LogWriters.h
    Constants.h
    CRC.h
    File.cpp
    File.h
    PackageInstructions.h
    Pk4Mappings.h
    ProgramOptions.h
    ReleaseManifest.h
    StdFilesystem.cpp
    StdFilesystem.h
    StdFormat.h
    StdString.cpp
    StdString.h
    SvnClient.cpp
    SvnClient.h
    TraceLog.cpp
    TraceLog.h
    ThreadPool.h
    Packager/Packager.cpp
    Packager/Packager.h
    Packager/PackagerOptions.h
    Zip/TdmZip.cpp
    Zip/TdmZip.h
)

if(MSVC)
    add_compile_options("/W2")
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELEASE " /OPT:REF /DEBUG")
elseif(UNIX)
    string(APPEND CMAKE_EXE_LINKER_FLAGS " -no-pie")
endif()

find_package(minizip REQUIRED CONFIG)
find_package(tinyformat REQUIRED CONFIG)
add_executable(tdm_package ${sources})
target_link_libraries(tdm_package PRIVATE tinyformat::tinyformat minizip::minizip)

if(UNIX)
    target_link_libraries(tdm_package PRIVATE stdc++fs pthread)
elseif(MSVC)
    set_property(TARGET tdm_package PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_definitions(tdm_package PRIVATE _CRT_SECURE_NO_DEPRECATE)
endif()

if(UNIX)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set_target_properties(tdm_package PROPERTIES OUTPUT_NAME "tdm_package.linux64")
    else()
        set_target_properties(tdm_package PROPERTIES OUTPUT_NAME "tdm_package.linux32")
    endif()
endif()
