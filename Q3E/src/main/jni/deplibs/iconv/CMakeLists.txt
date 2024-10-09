cmake_minimum_required(VERSION 3.4.1)
#字符编码转换
project(iconv)

add_compile_options(
        -Wno-multichar
        -DANDROID
        -D_ANDROID
        -DLIBDIR=\"\"
        -DBUILDING_LIBICONV
        -DIN_LIBRARY
)


include_directories(
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/libcharset
        ${CMAKE_CURRENT_SOURCE_DIR}/lib
        ${CMAKE_CURRENT_SOURCE_DIR}/libcharset/include
        ${CMAKE_CURRENT_SOURCE_DIR}/srclib
        )



add_library( # Sets the name of the library.
        iconv

        # Sets the library as a static library.
        SHARED

        # Provides a relative path to your source file(s).
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/iconv.c
        ${CMAKE_CURRENT_SOURCE_DIR}/libcharset/lib/localcharset.c
        ${CMAKE_CURRENT_SOURCE_DIR}/lib/relocatable.c

        )