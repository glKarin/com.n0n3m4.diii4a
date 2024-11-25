# Omnibot RTCW build

add_library(omnibot-rtcw MODULE ${CMAKE_CURRENT_SOURCE_DIR}/RTCW_BatchBuild.cpp)
target_link_libraries(omnibot-rtcw omni-shared omnibot-common)

set_target_properties(omnibot-rtcw PROPERTIES
    PREFIX ""
    OUTPUT_NAME "omnibot_rtcw${MODULE_SUFFIX}"
)

if (WIN32)
    set_target_properties(omnibot-rtcw PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ""
            LIBRARY_OUTPUT_DIRECTORY_DEBUG ""
            LIBRARY_OUTPUT_DIRECTORY_RELEASE ""
    )
    target_sources(omnibot-rtcw PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../projects/msvc12/Common/Common.rc)
endif ()
