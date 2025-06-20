cmake_minimum_required(VERSION 3.16)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

project(fwgs-OpusFile
  LANGUAGES C
)

option(OP_DISABLE_HTTP "Disable HTTP support" ON) #k OFF
option(OP_DISABLE_FLOAT_API "Disable floating-point API" OFF)
option(OP_FIXED_POINT "Enable fixed-point calculation" OFF)
option(OP_ENABLE_ASSERTIONS "Enable assertions in code" OFF)
option(OP_DISABLE_EXAMPLES "Do not build example applications" OFF)
option(OP_DISABLE_DOCS "Do not build API documentation" OFF)

include(GNUInstallDirs)

include(CMakePushCheckState)
include(CheckSymbolExists)
cmake_push_check_state(RESET)
include(CheckLibraryExists)
check_library_exists(m lrintf "" OP_HAVE_LIBM)
if(OP_HAVE_LIBM)
  list(APPEND CMAKE_REQUIRED_LIBRARIES "m")
endif()
check_symbol_exists(lrintf "math.h" OP_HAVE_LRINTF)
cmake_pop_check_state()

add_library(fwgs_opusfile STATIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include/opusfile.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/info.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/internal.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/internal.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/opusfile.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/stream.c"
)
target_include_directories(fwgs_opusfile
  PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_INSTALL_INCLUDEDIR}/opus
)
target_link_libraries(fwgs_opusfile
  PUBLIC
    fwgs_ogg
    fwgs_opus
    $<$<BOOL:${OP_HAVE_LIBM}>:m>
)
target_compile_options(fwgs_opusfile
  PRIVATE
    $<$<C_COMPILER_ID:MSVC>:/wd4267>
    $<$<C_COMPILER_ID:MSVC>:/wd4244>
    $<$<C_COMPILER_ID:MSVC>:/wd4090>
    $<$<C_COMPILER_ID:Clang,GNU>:-std=c89>
    $<$<C_COMPILER_ID:Clang,GNU>:-pedantic>
    $<$<C_COMPILER_ID:Clang,GNU>:-Wall>
    $<$<C_COMPILER_ID:Clang,GNU>:-Wextra>
    $<$<C_COMPILER_ID:Clang,GNU>:-Wno-parentheses>
    $<$<C_COMPILER_ID:Clang,GNU>:-Wno-long-long>
    $<$<C_COMPILER_ID:Clang,GNU>:-fvisibility=hidden>
)
target_compile_definitions(fwgs_opusfile
  PRIVATE
    $<$<BOOL:${OP_DISABLE_FLOAT_API}>:OP_DISABLE_FLOAT_API>
    $<$<BOOL:${OP_FIXED_POINT}>:OP_FIXED_POINT>
    $<$<BOOL:${OP_ENABLE_ASSERTIONS}>:OP_ENABLE_ASSERTIONS>
    $<$<BOOL:${OP_HAVE_LRINTF}>:OP_HAVE_LRINTF>
)

if(NOT OP_DISABLE_HTTP)
  find_package(OpenSSL REQUIRED)

  include(CheckIncludeFile)
  include(CheckCSourceCompiles)
  cmake_push_check_state(RESET)
  if(NOT WIN32)
    check_include_file("sys/socket.h" OP_HAVE_SYS_SOCKET_H)
    if(NOT OP_HAVE_SYS_SOCKET_H)
      message(FATAL_ERROR "HTTP support requires a POSIX socket library")
    endif()
  endif()
  check_c_source_compiles(
    "#include <time.h>
    int main(void)
    {
      struct timespec ts;
      return clock_gettime(CLOCK_REALTIME, &ts);
    }"
    OP_HAVE_CLOCK_GETTIME
  )
  if(NOT OP_HAVE_CLOCK_GETTIME)
    check_symbol_exists(ftime "sys/timeb.h" OP_HAVE_FTIME)
    if(NOT OP_HAVE_FTIME)
      message(FATAL_ERROR "HTTP support requires either clock_gettime() or ftime()")
    endif()
  endif()
  cmake_pop_check_state()

  add_library(opusurl
    "${CMAKE_CURRENT_SOURCE_DIR}/include/opusfile.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/http.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/internal.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/internal.h"
  )
  add_library(OpusFile::opusurl ALIAS opusurl)
  if(WIN32)
    target_sources(opusurl PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/src/wincerts.c"
      "${CMAKE_CURRENT_SOURCE_DIR}/src/winerrno.h"
    )
  endif()
  set_target_properties(opusurl PROPERTIES
    PUBLIC_HEADER "${CMAKE_CURRENT_SOURCE_DIR}/include/opusfile.h"
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
  )
  target_include_directories(opusurl
    PRIVATE
      "${CMAKE_CURRENT_SOURCE_DIR}/include"
    INTERFACE
      $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
      $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/opus>
  )
  target_compile_definitions(opusurl
    PRIVATE
      $<$<BOOL:${OP_DISABLE_FLOAT_API}>:OP_DISABLE_FLOAT_API>
      $<$<BOOL:${OP_FIXED_POINT}>:OP_FIXED_POINT>
      $<$<BOOL:${OP_ENABLE_ASSERTIONS}>:OP_ENABLE_ASSERTIONS>
      $<$<BOOL:${OP_HAVE_LRINTF}>:OP_HAVE_LRINTF>
      $<$<BOOL:${OP_HAVE_CLOCK_GETTIME}>:OP_HAVE_CLOCK_GETTIME>
      $<$<BOOL:${OP_HAVE_FTIME}>:OP_HAVE_FTIME>
      OP_ENABLE_HTTP
  )
  target_link_libraries(opusurl
    PRIVATE
      opusfile
      OpenSSL::SSL
      $<$<C_COMPILER_ID:MSVC>:ws2_32>
      $<$<C_COMPILER_ID:MSVC>:crypt32>
      $<$<BOOL:${OP_HAVE_LIBM}>:m>
  )
  target_compile_options(opusurl
    PRIVATE
      $<$<C_COMPILER_ID:MSVC>:/wd4267>
      $<$<C_COMPILER_ID:MSVC>:/wd4244>
      $<$<C_COMPILER_ID:MSVC>:/wd4090>
      $<$<C_COMPILER_ID:Clang,GNU>:-std=c89>
      $<$<C_COMPILER_ID:Clang,GNU>:-pedantic>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wall>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wextra>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wno-parentheses>
      $<$<C_COMPILER_ID:Clang,GNU>:-Wno-long-long>
      $<$<C_COMPILER_ID:Clang,GNU>:-fvisibility=hidden>
  )
  install(TARGETS opusurl
    EXPORT OpusFileTargets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    INCLUDES DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/opus"
    PUBLIC_HEADER DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}/opus"
  )
endif()
