# For more information about using CMake with Android Studio, read the
# documentation: https://d.android.com/studio/projects/add-native-code.html

# Sets the minimum version of CMake required to build the native library.

cmake_minimum_required(VERSION 3.4.1)

project(idtech)

add_definitions(-D__ANDROID__)
option(ANDROID "Android build" ON)

# Creates and names a library, sets it as either STATIC
# or SHARED, and provides the relative paths to its source code.
# You can define multiple libraries, and CMake builds them for you.
# Gradle automatically packages shared libraries with your APK.

# Searches for a specified prebuilt library and stores the path as a
# variable. Because CMake includes system libraries in the search path by
# default, you only need to specify the name of the public NDK library
# you want to add. CMake verifies that the library exists before
# completing its build.

#[[find_library( # Sets the name of the path variable.
        log-lib

        # Specifies the name of the NDK library that
        # you want CMake to locate.
        log )]]

# Specifies libraries CMake should link to your target library. You
# can link multiple libraries, such as libraries you define in this
# build script, prebuilt third-party libraries, or system libraries.

set(q3e_src
        q3e.c
        )

add_subdirectory (deplibs/oboe) # 1.8.1

set(q3e_lib
        log
        android
        )

add_library( # Sets the name of the library.
        q3eloader

        # Sets the library as a shared library.
        SHARED

        # Provides a relative path to your source file(s).
        ${q3e_src}
        )

target_link_libraries( # Specifies the target library.
        q3eloader

        # Links the target library to the log library
        # included in the NDK.
        #${log-lib}
        ${q3e_lib}
        )

#add_subdirectory(doom3/neo/externlibs/openal-soft)

add_subdirectory (doom3/neo) # DOOM3 | RoE, Quake4, Prey(2006)

add_subdirectory (deplibs) # common depend libraries

add_subdirectory (common)

add_subdirectory (rtcw) # RTCW

add_subdirectory (quake3) # Quake 3 Arena | Team Arena

add_subdirectory (quake2) # Quake 2

add_subdirectory (quake1) # Quake 1

add_subdirectory (tdm) # The Dark Mod

add_subdirectory (doom3bfg/neo) # RBDOOM3-BFG

#add_subdirectory (duke4) # DNF
