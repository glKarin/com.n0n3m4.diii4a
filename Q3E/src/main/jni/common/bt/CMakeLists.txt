cmake_minimum_required(VERSION 3.4.1)

project(q3e_bt)

set(CMAKE_C_STANDARD 11)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3 -pipe -fPIC")

#find_package(xunwind REQUIRED CONFIG)

set(q3e_bt_src
        q_xunwind.c
        )

set(q3e_bt_lib
        log
        #xunwind::xunwind
        )

add_library(q3e_bt SHARED
        ${q3e_bt_src}
        )

target_link_libraries(q3e_bt
        ${q3e_bt_lib}
        )
