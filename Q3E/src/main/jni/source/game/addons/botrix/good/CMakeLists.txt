project( source-good )

set(binary_name ${SOURCE_ENGINE_PREFIX}good)

cmake_minimum_required( VERSION 2.9 )

add_definitions(
    -DGOOD_INI_ONE_KV_SEPARATOR
    -DGOOD_INI_ONE_COMMENT_CHAR
    -DGOOD_LOG_FLUSH
    -DGOOD_LOG_USE_ENDL
)

# Get files list.
file( GLOB PROJECT_SOURCES
      "${CMAKE_CURRENT_SOURCE_DIR}/include/good/*.h"
      "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    )

include_directories( "${CMAKE_CURRENT_SOURCE_DIR}/include" )

add_library( ${binary_name} STATIC ${PROJECT_SOURCES} )
if(NOT ANDROID)
target_link_libraries( ${PROJECT_NAME} pthread )
endif()

#add_library ( gcov_good STATIC ${PROJECT_SOURCES} )
#set_target_properties ( gcov_good PROPERTIES
#                        COMPILE_FLAGS "-m32 -march=pentium4-g --coverage -fprofile-arcs -ftest-coverage"
#                        LINK_FLAGS "-m32 -march=pentium4" )
