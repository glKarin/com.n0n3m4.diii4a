# Omnibot ET build

add_library(omnibot-et MODULE ${CMAKE_CURRENT_SOURCE_DIR}/ET_BatchBuild.cpp)
target_link_libraries(omnibot-et omni-shared omnibot-common)

if(ANDROID) #//karin: build target name is libomnibot_et.so on Android
    set_target_properties(omnibot-et PROPERTIES
            OUTPUT_NAME "omnibot_et"
            )
else()
set_target_properties(omnibot-et PROPERTIES
        PREFIX ""
        OUTPUT_NAME "omnibot_et${MODULE_SUFFIX}"
        # POSITION_INDEPENDENT_CODE ON
        # MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>"
        )
endif()

if (WIN32)
    set_target_properties(omnibot-et PROPERTIES
            LIBRARY_OUTPUT_DIRECTORY ""
            LIBRARY_OUTPUT_DIRECTORY_DEBUG ""
            LIBRARY_OUTPUT_DIRECTORY_RELEASE ""
    )
    target_sources(omnibot-et PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/../projects/msvc12/Common/Common.rc)
endif ()
