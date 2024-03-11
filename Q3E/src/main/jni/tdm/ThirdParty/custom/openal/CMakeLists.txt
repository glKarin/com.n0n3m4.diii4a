cmake_minimum_required(VERSION 2.8.12)
project(cmake_wrapper)

include(conanbuildinfo.cmake)
conan_basic_setup()

if(NOT MSVC)
    # stgatilov: revert GCC 10 to old behavior, where global variable can be defined in many TUs
    # already fixed in new OpenAL versions: https://github.com/kcat/openal-soft/issues/519
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fcommon")
endif()

add_subdirectory("source_subfolder")
