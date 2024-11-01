cmake_minimum_required(VERSION 3.4.1)

project(q3e_oboeaudio)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -pipe -fPIC")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -O3 -pipe -fPIC")

set(oboeaudio_src
        snd_oboe.cpp
        )

set(oboeaudio_lib
        oboe
        )

add_library(oboeaudio STATIC
        ${oboeaudio_src}
        )

target_link_libraries(oboeaudio
        ${oboeaudio_lib}
        )
target_include_directories(oboeaudio PRIVATE "../../deplibs/oboe/include")
add_dependencies(oboeaudio oboe)
