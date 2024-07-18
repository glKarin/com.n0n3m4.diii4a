cmake_minimum_required(VERSION 3.4.1)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

add_subdirectory (libogg) # -1.3.5

add_subdirectory (libvorbis) # -1.3.7

add_subdirectory (libjpeg) # 9f

add_subdirectory (curl) # -8.5.0

add_subdirectory (libpng) #

add_subdirectory (pugixml) #

add_subdirectory (mbedtls) # -3.5.2

add_subdirectory (minizip) # -ng-1.2

add_subdirectory (freetype) #

add_subdirectory (flac) # -1.4.3

add_subdirectory (opus) # -1.4

add_subdirectory (mp3lame) #

add_subdirectory (mpg123/ports/cmake) # -1.32.6

add_subdirectory (libsndfile) # -1.2.2

add_subdirectory (fluidsynth)
#add_subdirectory(fluidsynth-lite)

add_subdirectory (etc2comp/EtcLib)

#add_subdirectory (ktx) # KTX-Software-3.0.1

#add_subdirectory (sdl2) # 2.30.2
