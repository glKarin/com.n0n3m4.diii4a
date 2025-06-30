cmake_minimum_required(VERSION 3.6)

project(MY_APP)

add_library(q3e_sdl_test SHARED)

target_sources(q3e_sdl_test PRIVATE YourSourceHere.c)

target_include_directories(q3e_sdl_test PRIVATE "${Q3E_SDL2_INCLUDE_PATH}" "${DIII4A_DEPEND_COMMON_PATH}")

target_link_libraries(q3e_sdl_test SDL2 GLESv1_CM log)

set_target_properties(q3e_sdl_test PROPERTIES
        OUTPUT_NAME "SeriousSamTSE")


