cmake_minimum_required(VERSION 3.12)

project(libdwarf VERSION 2.3.2 
    DESCRIPTION "Library to access DWARF debugging information" 
    HOMEPAGE_URL "https://github.com/davea42/libdwarf-code.git"
    LANGUAGES C CXX)

# libdwarf
#option(BUILD_NON_SHARED "build archive library libdwarf[p].a" TRUE)
#option(BUILD_SHARED "build shared library libdwarf[p].so and use it" FALSE)
set(BUILD_NON_SHARED TRUE)
set(BUILD_SHARED FALSE)
option(BUILD_MMAP "build library allowing mmap" TRUE)

option(ENABLE_DECOMPRESSION
    "Enables support for compressed debug sections if both libz/libzstd are present"
    TRUE)

#  This adds compiler option -Wall (gcc compiler warnings)
option(WALL "Add -Wall" FALSE)
option(LIBDWARF_STATIC "add -DLIBDWARF_STATIC" FALSE)

option(BUILD_DWARFDUMP "Build dwarfdump (default is YES)" FALSE) # TRUE

option(BUILD_DWARFGEN "Build dwarfgen (default is NO)" FALSE)
message(STATUS "Building dwarfgen    ... ${BUILD_DWARFGEN}")

option(BUILD_DWARFEXAMPLE "Build dwarfexample (default is NO)" FALSE)
message(STATUS "Building dwarfexample... ${BUILD_DWARFEXAMPLE}")

option(DO_TESTING "Do certain api tests (default is NO)" FALSE)
message(STATUS "Building api tests   ... ${DOTESTS}")
### end what was configure.cmake

option(PIC_ALWAYS "Build libdwarf with position-independent code for both static and shared (default is NO)" FALSE)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
include_directories( ${PROJECT_BINARY_DIR} )

# used to compile on MSVC upto 2013 where snprintf is not available
macro(msvc_posix target)
    if(MSVC AND ("${MSVC_VERSION}" LESS 1900))
        # under VS 2015
	target_compile_definitions(${target} PUBLIC
	    snprintf=_snprintf)
    endif()
endmacro()

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
  set(LIBDWARF_CRT "MD" CACHE STRING "Either MT or MD, specifies whether to use the static or dynamic MSVCRT.")

  # Use CMAKE_MSVC_RUNTIME in versions 3.15 and up
  if (CMAKE_VERSION VERSION_GREATER_EQUAL "3.15")
    cmake_policy(SET CMP0091 NEW)
    if (LIBDWARF_CRT STREQUAL "MT")
      message(STATUS "Using MT runtime by CMAKE_MSVC_RUNTIME_LIBRARY")
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    elseif(LIBDWARF_CRT STREQUAL "MD")
      message(STATUS "Using MD runtime by CMAKE_MSVC_RUNTIME_LIBRARY")
      set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    else()
      message(FATAL_ERROR "LIBDWARF_CRT must be MT or MD")
    endif()
  # Use regexes in versions before 3.15
  else()
    if (LIBDWARF_CRT STREQUAL "MT")
      message(STATUS "Using MT runtime by compile flag replacement")
      # taken from the CMake FAQ
      foreach(flag_v
      CMAKE_C_FLAGS CMAKE_C_FLAGS_DEBUG
      CMAKE_C_FLAGS_RELEASE CMAKE_C_FLAGS_MINSIZEREL
      CMAKE_C_FLAGS_RELWITHDEBINFO
      CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG
      CMAKE_CXX_FLAGS_RELEASE CMAKE_CXX_FLAGS_MINSIZEREL
      CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        if (${flag_v} MATCHES "([\\/\\-]M)D")
          string(REGEX REPLACE "([\\/\\-]M)D" "\\1T"
          ${flag_v} "${${flag_v}}")
        endif()
      endforeach()
    elseif(LIBDWARF_CRT STREQUAL "MD")
      message(STATUS "Using MD runtime by cmake default")
    else()
      message(FATAL_ERROR "LIBDWARF_CRT must be MT or MD")
    endif()
  endif()
endif()

# message("CMake flags are:")
# message("  CMAKE_C_FLAGS: ${CMAKE_C_FLAGS}")
# message("  CMAKE_C_FLAGS_DEBUG: ${CMAKE_C_FLAGS_DEBUG}")
# message("  CMAKE_C_FLAGS_RELEASE: ${CMAKE_C_FLAGS_RELEASE}")
# message("  CMAKE_C_FLAGS_MINSIZEREL: ${CMAKE_C_FLAGS_MINSIZEREL}")
# message("  CMAKE_C_FLAGS_RELWITHDEBINFO: ${CMAKE_C_FLAGS_RELWITHDEBINFO}")
# message("  CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")
# message("  CMAKE_CXX_FLAGS_DEBUG: ${CMAKE_CXX_FLAGS_DEBUG}")
# message("  CMAKE_CXX_FLAGS_RELEASE: ${CMAKE_CXX_FLAGS_RELEASE}")
# message("  CMAKE_CXX_FLAGS_MINSIZEREL: ${CMAKE_CXX_FLAGS_MINSIZEREL}")
# message("  CMAKE_CXX_FLAGS_RELWITHDEBINFO: ${CMAKE_CXX_FLAGS_RELWITHDEBINFO}")


# set target folder on IDE
macro(set_folder target folder)
  set_target_properties(${target} PROPERTIES FOLDER ${folder})
endmacro()

# set source groups on IDE
macro(set_source_group list_name group_name)
    set(${list_name} ${ARGN})
    source_group(${group_name} FILES ${ARGN})
endmacro()

# view folders on supported IDEs
set_property(GLOBAL PROPERTY USE_FOLDERS ON)

### begin what was configure.cmake
# cmake macros
include(TestBigEndian)
include(CheckIncludeFile)
include(CheckCSourceCompiles)
include(CheckCSourceRuns)
include(CheckSymbolExists)
### Version also appears in configure.ac
set(PACKAGE_BUGREPORT "https://github.com/davea42/libdwarf-code/issues")
set(PACKAGE_STRING "${PROJECT_NAME} ${PROJECT_VERSION}")
string(REGEX REPLACE "[\"]" "" tarname1 "${PACKAGE_STRING}" )
string(REGEX REPLACE "[^a-zA-Z0-9_]" "-" tarname "${tarname1}" )
set(PACKAGE_TARNAME "\"${tarname}\"" )

test_big_endian(isBigEndian)
if (${isBigEndian})
  set ( WORDS_BIGENDIAN 1 )
endif()

check_include_file( "unistd.h"        HAVE_UNISTD_H   )
check_include_file( "sys/types.h"     HAVE_SYS_TYPES_H )
check_include_file( "malloc.h"        HAVE_MALLOC_H )
check_include_file( "stdint.h"        HAVE_STDINT_H )
check_include_file( "inttypes.h"      HAVE_INTYPES_H )
check_include_file( "stddef.h"        HAVE_STDDEF_H )
check_include_file( "fcntl.h"         HAVE_FCNTL_H   )
check_include_file( "sys/stat.h"      HAVE_SYS_STAT_H )
check_include_file( "stdafx.h"        HAVE_STDAFX_H   )
if (BUILD_MMAP)
check_include_file( "sys/mman.h"      HAVE_SYS_MMAN_H )
endif()

### cmake provides no way to guarantee uint32_t present.
### configure does guarantee that.
if(HAVE_STDINT_H)
  check_c_source_compiles("
  #include <stdint.h>
  int main()
  {
      uintptr_t i; i = 12;
      return (int)i;
  }" HAVE_UINTPTR_T)
  check_c_source_compiles("
  #include <stdint.h>
  int main()
  {
      intptr_t i; i = 12;
      return (int)i;
  }" HAVE_INTPTR_T)
endif()

if(BUILD_MMAP AND HAVE_SYS_MMAN_H )
  check_c_source_compiles("
  #include <unistd.h>
  #include <string.h>
  #include <stdlib.h>
  #include <sys/mman.h>
  int main()
  {
      long pagesize = sysconf(_SC_PAGESIZE);
      int fd = 4;
      mmap(0, 100,PROT_READ|PROT_WRITE,MAP_PRIVATE,
         fd,100);
      munmap((void *)100,100);
      return 0;
  }" HAVE_FULL_MMAP)
endif()

if (HAVE_UINTPTR_T)
  message(STATUS "HAVE_UINTPTR_T 1: uintptr_t defined in stdint.h... YES")
else()
  message(STATUS "uintptr_t defined in stdint.h... NO")
  set(uintptr_t "unsigned long long int" )
  message(STATUS "Setting #define uintptr_t " ${uintptr_t})
endif()
if (uintptr_t)
  message(STATUS "uintptr_t value considered YES ")
else()
  message(STATUS "uintptr_t value considered NO ")
endif()
if (HAVE_INTPTR_T)
  message(STATUS "HAVE_INTPTR_T 1: intptr_t defined in stdint.h... YES")
else()
  message(STATUS "intptr_t defined in stdint.h... NO")
  set(intptr_t "long long int" )
  message(STATUS "Setting #define intptr_t " ${intptr_t})
endif()
if (intptr_t)
  message(STATUS "intptr_t value considered YES ")
else()
  message(STATUS "intptr_t value considered NO ")
endif()

message(STATUS "ENABLE_DECOMPRESSION : " ${ENABLE_DECOMPRESSION})
if (ENABLE_DECOMPRESSION)
  #message(STATUS "In ENABLE_DECOMPRESSION setup: TRUE")
  # Zlib and ZSTD need to be found otherwise disable it
  if(NOT TARGET ZLIB::ZLIB)
    find_package(ZLIB)
  else()
    # Presumably in this case, the target has been found externally but set this flag just in case
    set(ZLIB_FOUND TRUE)
  endif()
  # This feels excessive, however perhaps better safe than sorry
  if(
    NOT (
      TARGET zstd::libzstd_shared OR
      TARGET zstd::libzstd_static OR
      TARGET libzstd_shared OR
      TARGET libzstd_static OR
      TARGET ZSTD::ZSTD
    )
  )
    find_package(zstd)
  else()
    # Presumably in this case, the target has been found externally but set this flag just in case
    set(zstd_FOUND TRUE)
  endif()
  # Unfortunately aliasing ZSTD::ZSTD to zstd::libzstd_shared/static can lead to problems for end-users, a variable
  # is used
  if(TARGET zstd::libzstd_shared)
    set(ZSTD_LIB zstd::libzstd_shared)
  elseif(TARGET zstd::libzstd_static)
    set(ZSTD_LIB zstd::libzstd_static)
  elseif(TARGET libzstd_shared)
    set(ZSTD_LIB libzstd_shared)
  elseif(TARGET libzstd_static)
    set(ZSTD_LIB libzstd_static)
  else()
    set(ZSTD_LIB ZSTD::ZSTD)
  endif()
  if (ZLIB_FOUND AND zstd_FOUND )
    set(HAVE_ZLIB TRUE)
    set(HAVE_ZLIB_H TRUE)
    set(HAVE_ZSTD TRUE)
    set(HAVE_ZSTD_H TRUE)
    set(BUILT_WITH_ZLIB_AND_ZSTD TRUE)
  endif()
  message(STATUS "Found libzstd           : ${zstd_FOUND}")
  message(STATUS "Found zlib              : ${ZLIB_FOUND}")
  message(STATUS "Build with zlib and zstd: ${BUILT_WITH_ZLIB_AND_ZSTD}")
else()
  message(STATUS "Build with zlib and zstd: NO")
endif ()

message(STATUS "CMAKE_SIZEOF_VOID_P ... : ${CMAKE_SIZEOF_VOID_P}")

#  DW_FWALLXX are gnu C++ options.
if (WALL)
  set(DW_FWALLXX -Wall -Wextra -Wno-unused-private-field -Wpointer-arith -Wmissing-declarations -Wcomment -Wformat -Wpedantic -Wuninitialized -Wshadow -Wno-long-long -Werror)
  set(DW_FWALL ${DW_FWALLXX} -Wpointer-arith -Wmissing-declarations -Wmissing-prototypes -Wdeclaration-after-statement -Wextra -Wcomment -Wformat -Wpedantic -Wuninitialized -Wno-long-long -Wshadow -Werror )
  message(STATUS "Compiler warning options... YES ${DW_FWALL}")
else()
  unset(DW_FWALL )
  message(STATUS "Compiler warning options... NO")
endif()
#if (LIBDWARF_STATIC)
#  set(DW_LIBDWARF_STATIC -DLIBDWARF_STATIC)
#else()
#  unset(DW_LIBDWARF_STATIC)
#endif()

configure_file(cmake/config.h.in config.h)

if(BUILD_NON_SHARED)
  set(DW_LIBDWARF_STATIC -DLIBDWARF_STATIC)
  set(BUILD_SHARED_LIBS NO)
else()
  unset(DW_LIBDWARF_STATIC)
  set(BUILD_SHARED_LIBS YES)
endif()

include(GNUInstallDirs)

add_subdirectory(src/lib/libdwarf)
add_subdirectory(doc)

if ( BUILD_DWARFDUMP )
  add_subdirectory(src/bin/dwarfdump)
endif()

if ( BUILD_DWARFEXAMPLE )
	add_subdirectory(src/bin/dwarfexample)
endif()

if ( BUILD_DWARFGEN )
  add_subdirectory(src/lib/libdwarfp)
  add_subdirectory(src/bin/dwarfgen)
endif()

if (DO_TESTING)
  enable_testing()
  add_subdirectory(test)
endif()

message(STATUS "Install prefix ... ${CMAKE_INSTALL_PREFIX}" )
# installation of libdwarf[p] has to be in
# src/lib/libdwarf[p]/CMakeLists.txt
# so that it works properly for cmake before cmake 3.13.
