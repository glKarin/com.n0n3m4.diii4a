cmake_minimum_required(VERSION 3.9)
project (VCS_INFO)

execute_process(COMMAND "git" "describe" "--dirty" "--always"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE GIT_VERSION
	OUTPUT_STRIP_TRAILING_WHITESPACE)

execute_process(COMMAND "git" "rev-parse" "--abbrev-ref" "HEAD"
	WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
	OUTPUT_VARIABLE GIT_BRANCH
	OUTPUT_STRIP_TRAILING_WHITESPACE)

if(GIT_VERSION)
	add_definitions(-DXASH_BUILD_COMMIT="${GIT_VERSION}")
endif()

if(GIT_BRANCH)
	add_definitions(-DXASH_BUILD_BRANCH="${GIT_BRANCH}")
endif()


include_directories(.)
add_library(hlsdk_vcs_info STATIC vcs_info.c)
set_property(TARGET hlsdk_vcs_info PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
