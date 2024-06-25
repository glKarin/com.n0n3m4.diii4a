cmake_minimum_required(VERSION 2.8.7)
project(list_midi_devices)

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/../../cmake")

find_package(ZMusic REQUIRED)

add_executable(list_midi_devices list_midi_devices.cpp)
target_include_directories(list_midi_devices PRIVATE ${ZMUSIC_INCLUDE_DIR})
target_link_libraries(list_midi_devices PRIVATE ${ZMUSIC_LIBRARIES} ${SDL2_LIBRARIES})
