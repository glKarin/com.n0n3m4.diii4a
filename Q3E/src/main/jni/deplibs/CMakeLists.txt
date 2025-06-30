cmake_minimum_required(VERSION 3.4.1)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_subdirectory (libogg) # -1.3.5

add_subdirectory (libvorbis) # -1.3.7

add_subdirectory (libjpeg) # 9f of 14-Jan-2024

add_subdirectory (libpng) # 1.6.44.git

add_subdirectory (pugixml) # 1.14

add_subdirectory (mbedtls) # -3.5.2

add_subdirectory (minizip) # -ng-1.2

add_subdirectory (freetype) # 2.13.2

add_subdirectory (flac) # -1.4.3

add_subdirectory (opus) # -1.4

add_subdirectory (mp3lame) # 3.100-2 ???

add_subdirectory (mpg123/ports/cmake) # -1.32.6

add_subdirectory (libsndfile) # -1.2.2

add_subdirectory (fluidsynth)
#add_subdirectory(fluidsynth-lite)

#add_subdirectory (etc2comp/EtcLib)

add_subdirectory (sqlite) # sqlite-amalgamation-3460100

add_subdirectory (cjson) # -1.7.18

add_subdirectory (iconv) # https://github.com/q384264619/android_iconv/tree/master

add_subdirectory (lua) # -5.4.5

add_subdirectory (openssl) # openssl-cmake-1.1.1w-20231130 https://github.com/janbar/openssl-cmake

add_subdirectory (curl) # -8.5.0

add_subdirectory (bzip2) # -1.0.8

# add_subdirectory (glm) # -0.9.9.3

#add_subdirectory (libvpx) # -1.15.0

#add_subdirectory (ktx) # KTX-Software-3.0.1

#add_subdirectory (sdl2) # 2.30.2
