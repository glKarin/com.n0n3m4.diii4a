cmake_minimum_required(VERSION 3.4.1)

# build type
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE "Release")
endif()

# OpenGL backend
set(RBDOOM3BFG_BACKEND "")
add_subdirectory (neo)

# Vulkan backend
set(RBDOOM3BFG_BACKEND "Vulkan")
add_subdirectory (neo "vkdoom3bfg")

