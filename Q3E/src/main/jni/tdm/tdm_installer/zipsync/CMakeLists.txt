cmake_minimum_required (VERSION 3.9.6)
project(zipsync CXX C)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

option(ZIPSYNC_OPTION_BUILD_TESTS "build executable with unit tests" ON)
option(ZIPSYNC_OPTION_BUILD_TOOL "build command-line zipsync tool" ON)

set(lib_sources
    minizip_extra.c
    minizip_extra.h
    minizip_private.h
    StdString.cpp
    StdString.h
    Logging.h
    Logging.cpp
    Ini.h
    Ini.cpp
    Hash.h
    Hash.cpp
    Path.h
    Path.cpp
    Utils.h
    Utils.cpp
    ZipUtils.h
    ZipUtils.cpp
    ChecksummedZip.h
    ChecksummedZip.cpp
    Manifest.h
    Manifest.cpp
    LocalCache.h
    LocalCache.cpp
    ZipSync.h
    ZipSync.cpp
    Downloader.h
    Downloader.cpp
    ZipSync.natvis
)

set(tests_sources
    StdFilesystem.cpp
    StdFilesystem.h
    TestCreator.cpp
    TestCreator.h
    Fuzzer.cpp
    Fuzzer.h
    TestsMain.cpp
    HttpServer.cpp
    HttpServer.h
)

set(extralib_sources
    StdFilesystem.cpp
    StdFilesystem.h
    Wildcards.cpp
    Wildcards.h
    CommandLine.cpp
    CommandLine.h
)

set(zipsynccmd_sources
    CommandLineMain.cpp
)


if(MSVC)
    add_compile_options("/W2")
    add_compile_options("$<$<CONFIG:RelWithDebInfo>:/Ob2>")
    add_compile_options("$<$<CONFIG:Release>:/Zi>")
endif()


find_package(minizip REQUIRED CONFIG)
find_package(CURL REQUIRED CONFIG)
find_package(blake2 REQUIRED CONFIG)

add_library(libzipsync STATIC ${lib_sources})
target_link_libraries(libzipsync PUBLIC minizip::minizip blake2::blake2 PRIVATE CURL::libcurl)

add_library(libzipsyncextra STATIC ${extralib_sources})
target_link_libraries(libzipsyncextra PUBLIC libzipsync)
if(NOT MSVC)
    target_link_libraries(libzipsyncextra PUBLIC stdc++fs)
endif()

if(ZIPSYNC_OPTION_BUILD_TESTS)
    find_package(doctest REQUIRED CONFIG)
    find_package(libmicrohttpd REQUIRED CONFIG)

    add_executable(tests ${tests_sources})
    target_link_libraries(tests libzipsync libzipsyncextra doctest::doctest libmicrohttpd::libmicrohttpd CURL::libcurl minizip::minizip blake2::blake2)
endif()

if(ZIPSYNC_OPTION_BUILD_TOOL)
    find_package(args REQUIRED CONFIG)

    add_executable(zipsync ${zipsynccmd_sources})
    target_link_libraries(zipsync libzipsync libzipsyncextra args::args)
endif()
