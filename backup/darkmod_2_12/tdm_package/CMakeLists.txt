cmake_minimum_required (VERSION 3.9.6)
cmake_policy(SET CMP0091 NEW)   # for MSVC_RUNTIME_LIBRARY
project(tdm_package)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(THIRDPARTY_PLATFORM_OVERRIDE "" CACHE STRING "If you build conan dependencies yourself with custom platform_name, pass platform_name string here")

# TDM: use prebuilt artefacts for third-party libraries
list(PREPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../ThirdParty/cmake_find_package")

set(CMAKE_CONFIGURATION_TYPES "Debug;RelWithDebInfo" CACHE STRING "" FORCE) 


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
    add_compile_options("$<$<CONFIG:RelWithDebInfo>:/Ob2>")
    string(APPEND CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO " /INCREMENTAL:NO /OPT:REF")
elseif(UNIX)
    string(APPEND CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -no-pie")
endif()

find_package(zlib REQUIRED)
find_package(minizip REQUIRED)
find_package(tinyformat REQUIRED)
add_executable(tdm_package ${sources})
target_include_directories(tdm_package PRIVATE ${tinyformat_INCLUDE_DIRS} ${minizip_INCLUDE_DIRS} ${zlib_INCLUDE_DIRS})
target_link_libraries(tdm_package PRIVATE ${tinyformat_LIBRARIES} ${minizip_LIBRARIES} ${zlib_LIBRARIES})

if(UNIX)
    target_link_libraries(tdm_package PRIVATE stdc++fs pthread)
elseif(MSVC)
    set_property(TARGET tdm_package PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    target_compile_definitions(tdm_package PRIVATE _CRT_SECURE_NO_DEPRECATE)
endif()

if(THIRDPARTY_PLATFORM_OVERRIDE)
    if(UNIX)
        set_target_properties(tdm_package PROPERTIES OUTPUT_NAME "tdm_package.custom")
    elseif(MSVC)
        set_target_properties(tdm_package PROPERTIES OUTPUT_NAME "tdm_package_custom.exe")
    endif()
elseif(UNIX)
    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        set_target_properties(tdm_package PROPERTIES OUTPUT_NAME "tdm_package.linux64")
    else()
        set_target_properties(tdm_package PROPERTIES OUTPUT_NAME "tdm_package.linux32")
    endif()
endif()
