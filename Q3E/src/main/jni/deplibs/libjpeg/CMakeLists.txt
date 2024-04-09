cmake_minimum_required(VERSION 3.5)
project(jpeg-cmake VERSION 1.3.0)

# Use configure_file insted of file(COPY)
# to ensure files are updated in src directory
configure_file(
  resources/CMakeLists.txt
  ${PROJECT_SOURCE_DIR}/libjpeg/CMakeLists.txt
  COPYONLY
)
configure_file(
  resources/ConfigureJConfig.cmake
  ${PROJECT_SOURCE_DIR}/libjpeg/ConfigureJConfig.cmake
  COPYONLY
)
configure_file(
  resources/jconfig.h.in
  ${PROJECT_SOURCE_DIR}/libjpeg/jconfig.h.in
  COPYONLY
)
configure_file(
  resources/libjpeg.pc.cmakein
  ${PROJECT_SOURCE_DIR}/libjpeg/libjpeg.pc.cmakein
  COPYONLY
)
configure_file(
  resources/libjpeg.la.in
  ${PROJECT_SOURCE_DIR}/libjpeg/libjpeg.la.in
  COPYONLY
)
configure_file(
  resources/test-libjpeg.sh
  ${PROJECT_SOURCE_DIR}/libjpeg/test-libjpeg.sh
  COPYONLY
)

# Enable testing here AND in libjpeg CMakelists.txt
enable_testing()

# Build libjpeg
add_subdirectory(libjpeg)
