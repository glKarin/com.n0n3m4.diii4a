cmake_minimum_required(VERSION 3.12)

project(openmohaa_curl)

add_library(curldefs INTERFACE)
target_include_directories(curldefs INTERFACE "./")

find_package(CURL)

if (CURL_FOUND)
    target_include_directories(curldefs INTERFACE ${CURL_INCLUDE_DIRS})
    target_compile_definitions(curldefs INTERFACE HAS_LIBCURL=1)
elseif(ANDROID) #k
    set(CURL_INCLUDE_DIRS "${OPENMOHAA_DEPLIBS_DIR}/curl/include")
    target_include_directories(curldefs INTERFACE ${CURL_INCLUDE_DIRS})
    target_compile_definitions(curldefs INTERFACE HAS_LIBCURL=1)
else()
    message(WARNING "CURL is not available, HTTP-based features will not work.")
endif()
