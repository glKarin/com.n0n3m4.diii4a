cmake_minimum_required(VERSION 3.4.1)

project(q3e_jni)

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
        q3eeventqueue.cpp
        q3ebt.c
        q3ethread.c
        q3emisc.c
        q3eutility.c
        )

set(q3e_lib
        log
        android
        q3e_bt
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
target_compile_options(q3eloader PRIVATE -O3 -pipe -fPIC)
target_include_directories(q3eloader PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/.. ${CMAKE_CURRENT_SOURCE_DIR}/../common/bt)
