# CMake configuration for IJG libjpeg
# To install, copy this file and jconfig.h.in into a libjpeg source directory
# Adapted from LuaDist's CMakeLists
# https://github.com/LuaDist/libjpeg/blob/master/CMakeLists.txt

cmake_minimum_required(VERSION 3.5)

### Setup the project ###
file(READ "configure.ac" ac)
string(REGEX MATCH "AC_INIT\\(\\[libjpeg\\],\ \\[([0-9]*\\.[0-9]*\\.[0-9]*)\\]\\)" _ ${ac})
set(version ${CMAKE_MATCH_1})
project(libjpeg VERSION ${version} LANGUAGES C)
set(C_STANDARD 99)

### Include extra packages ###
include(CMakeDependentOption)
include(GNUInstallDirs)

### Options ###
option(BUILD_SHARED_LIBS "Build shared libraries" ON)
option(BUILD_STATIC_LIBS "Build static libraries" OFF) #k ON
cmake_dependent_option(BUILD_EXECUTABLES "Build JPEG utilities" OFF "BUILD_SHARED_LIBS OR BUILD_STATIC_LIBS" OFF) #k ON
cmake_dependent_option(BUILD_TESTS "Build test executables" OFF "BUILD_SHARED_LIBS OR BUILD_STATIC_LIBS;BUILD_EXECUTABLES" OFF) #k ON
cmake_dependent_option(LINK_STATIC "Link all executables statically" OFF "BUILD_STATIC_LIBS;BUILD_EXECUTABLES" OFF)
cmake_dependent_option(BUILD_ALT_UI "Build alternate interface for command line utilities" OFF "BUILD_EXECUTABLES" OFF)

# Make sure we build at least one library
if(NOT(BUILD_SHARED_LIBS OR BUILD_STATIC_LIBS))
  message(FATAL_ERROR "Both static and shared libraries are disabled. Nothing will be built.")
endif()

### Configure jconfig.h ###
include(ConfigureJConfig.cmake)

### Build the object library ###
set(PUBLIC_HDRS
  jconfig.h
  jerror.h
  jmorecfg.h
  jpeglib.h
)

set(SRCS
  jaricom.c jcapimin.c jcapistd.c jcarith.c jccoefct.c jccolor.c
  jcdctmgr.c jchuff.c jcinit.c jcmainct.c jcmarker.c jcmaster.c jcomapi.c
  jcparam.c jcprepct.c jcsample.c jctrans.c jdapimin.c jdapistd.c jdarith.c
  jdatadst.c jdatasrc.c jdcoefct.c jdcolor.c jddctmgr.c jdhuff.c jdinput.c
  jdmainct.c jdmarker.c jdmaster.c jdmerge.c jdpostct.c jdsample.c jdtrans.c
  jerror.c jfdctflt.c jfdctfst.c jfdctint.c jidctflt.c jidctfst.c jidctint.c
  jmemmgr.c jmemnobs.c jquant1.c jquant2.c jutils.c
)

add_library(jpeg_objs OBJECT ${SRCS})
target_include_directories(jpeg_objs
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:include>
)
set_target_properties(jpeg_objs PROPERTIES POSITION_INDEPENDENT_CODE ON)

### Create static and shared libs ###
if(BUILD_SHARED_LIBS)
  add_library(jpeg SHARED $<TARGET_OBJECTS:jpeg_objs>)
  set_target_properties(jpeg
    PROPERTIES
      VERSION ${PROJECT_VERSION_MAJOR}
      CLEAN_DIRECT_OUTPUT ON
      PUBLIC_HEADER "${PUBLIC_HDRS}"
  )
  install(TARGETS jpeg
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

if(BUILD_STATIC_LIBS)
  add_library(jpeg_static STATIC $<TARGET_OBJECTS:jpeg_objs>)
  set_target_properties(jpeg_static
    PROPERTIES
      OUTPUT_NAME jpeg
      VERSION ${PROJECT_VERSION_MAJOR}
      CLEAN_DIRECT_OUTPUT ON
      PUBLIC_HEADER "${PUBLIC_HDRS}"
  )
  install(TARGETS jpeg_static
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )
endif()

# Configure and install pkg-config and libtool files
if(BUILD_STATIC_LIBS OR BUILD_SHARED_LIBS)
  # Compute the la file's weird version number
  math(EXPR JPEG_CONF_VER_MAJOR "${PROJECT_VERSION_MAJOR} + ${PROJECT_VERSION_MINOR}")
  set(JPEG_LIB_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
  set(JPEG_LIB_VERSION_MINOR ${PROJECT_VERSION_MINOR})

  # Create library file names for libtool file
  if(BUILD_SHARED_LIBS)
    set(DYN_LIB_FILE_VERSIONED ${CMAKE_SHARED_LIBRARY_PREFIX}jpeg.${PROJECT_VERSION_MAJOR}${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(DYN_LIB_FILES "${DYN_LIB_FILE_VERSIONED} ${CMAKE_SHARED_LIBRARY_PREFIX}jpeg${CMAKE_SHARED_LIBRARY_SUFFIX}")
  endif()

  if(BUILD_STATIC_LIBS)
    set(STATIC_LIB_FILE "${CMAKE_STATIC_LIBRARY_PREFIX}jpeg${CMAKE_STATIC_LIBRARY_SUFFIX}")
  endif()

  # Configure and install
  configure_file(libjpeg.pc.cmakein libjpeg.pc @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libjpeg.pc
    DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/pkgconfig
  )

  configure_file(libjpeg.la.in libjpeg.la @ONLY)
  install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/libjpeg.la
    DESTINATION ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/
  )
endif()

### Build executables
if(BUILD_EXECUTABLES)
  # Build alternate UI
  if(BUILD_ALT_UI)
    set(CJPEG_UI cjpegalt.c)
    set(DJPEG_UI djpegalt.c)
  else()
    set(CJPEG_UI cjpeg.c)
    set(DJPEG_UI djpeg.c)
  endif()
  add_executable(cjpeg cdjpeg.c ${CJPEG_UI} rdbmp.c rdgif.c rdppm.c rdrle.c rdtarga.c rdswitch.c)
  add_executable(djpeg cdjpeg.c ${DJPEG_UI} wrbmp.c wrgif.c wrppm.c wrrle.c wrtarga.c rdcolmap.c)
  add_executable(jpegtran jpegtran.c cdjpeg.c rdswitch.c transupp.c)
  add_executable(rdjpgcom rdjpgcom.c)
  add_executable(wrjpgcom wrjpgcom.c)

  # Link appropriately
  if(LINK_STATIC OR (NOT BUILD_SHARED_LIBS))
    message(STATUS "Linking executables statically")
    set(LINKED_JPEG_LIB jpeg_static)
  else()
    message(STATUS "Linking executables dynamically")
    set(LINKED_JPEG_LIB jpeg)
  endif()
  target_link_libraries(cjpeg ${LINKED_JPEG_LIB})
  target_link_libraries(djpeg ${LINKED_JPEG_LIB})
  target_link_libraries(jpegtran ${LINKED_JPEG_LIB})

  install(TARGETS cjpeg djpeg jpegtran rdjpgcom wrjpgcom
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  )
  install(
    FILES cjpeg.1 djpeg.1 jpegtran.1 rdjpgcom.1 wrjpgcom.1
    DESTINATION ${CMAKE_INSTALL_MANDIR}/man1
  )
endif()

### Add tests
if(BUILD_TESTS)
  enable_testing()
  # Copy test files
  file(
    COPY testorig.jpg testimg.ppm testimg.gif testimg.bmp testimg.jpg testprog.jpg testimgp.jpg
    DESTINATION ${CMAKE_CURRENT_BINARY_DIR}
  )
  ## Add tests
  add_test(
    NAME jpeg2ppm
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test-libjpeg.sh jpeg2ppm
  )
  add_test(
    NAME jpeg2gif
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test-libjpeg.sh jpeg2gif
  )
  add_test(
    NAME jpeg2bmp
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test-libjpeg.sh jpeg2bmp
  )
  add_test(
    NAME ppm2jpg
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test-libjpeg.sh ppm2jpg
  )
  add_test(
    NAME progressive2ppm
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test-libjpeg.sh progressive2ppm
  )
  add_test(
    NAME ppm2progressive
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test-libjpeg.sh ppm2progressive
  )
  add_test(
    NAME progressive2baseline
    COMMAND bash ${CMAKE_CURRENT_SOURCE_DIR}/test-libjpeg.sh progressive2baseline
  )
endif()
