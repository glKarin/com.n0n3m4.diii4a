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
endif()


find_package(zlib REQUIRED)
find_package(minizip REQUIRED)
find_package(CURL REQUIRED)
find_package(BLAKE2 REQUIRED)
add_library(libzipsync STATIC ${lib_sources})
target_compile_definitions(libzipsync PUBLIC CURL_STATICLIB)
target_include_directories(libzipsync PUBLIC ${zlib_INCLUDE_DIRS} ${minizip_INCLUDE_DIRS} ${BLAKE2_INCLUDE_DIRS} ${CURL_INCLUDE_DIRS})
target_link_libraries(libzipsync PUBLIC ${zlib_LIBRARIES} ${minizip_LIBRARIES} ${BLAKE2_LIBRARIES} ${CURL_LIBRARIES})

add_library(libzipsyncextra STATIC ${extralib_sources})
target_link_libraries(libzipsyncextra PUBLIC libzipsync)
if(NOT MSVC)
    target_link_libraries(libzipsyncextra PUBLIC stdc++fs)
endif()

if(ZIPSYNC_OPTION_BUILD_TESTS)
    find_package(doctest REQUIRED)
    find_package(libmicrohttpd REQUIRED)
    add_executable(tests ${tests_sources})
    target_include_directories(tests PRIVATE ${doctest_INCLUDE_DIRS} ${libmicrohttpd_INCLUDE_DIRS})
    target_link_libraries(tests PRIVATE libzipsync libzipsyncextra ${doctest_LIBRARIES} ${libmicrohttpd_LIBRARIES})
endif()

if(ZIPSYNC_OPTION_BUILD_TOOL)
    find_package(args REQUIRED)
    add_executable(zipsync ${zipsynccmd_sources})
    target_include_directories(zipsync PRIVATE ${args_INCLUDE_DIRS})
    target_link_libraries(zipsync PRIVATE libzipsync libzipsyncextra ${args_LIBRARIES})
endif()
