cmake_minimum_required(VERSION 3.9)
project (xash3d-fwgs-filesystem)

include(CheckCXXCompilerFlag)

# filesystem
check_c_source_compiles( "#include <sys/mman.h>
                          int main(int argc, char **argv) { return memfd_create(argv[0], 0); }" MEMFD_CREATE_TEST )
check_c_source_compiles( "#define _GNU_SOURCE
                          #include <dirent.h>
                          int main(int argc, char **argv) { struct dirent entry; entry.d_type = DT_DIR; return 0; }" DIRENT_D_TYPE_TEST )
file(GLOB FWGS_FILESYSTEM_INCLUDES *.h)
file(GLOB FWGS_FILESYSTEM_SOURCES *.c *.cpp)
add_library (xash3d_filesystem_stdio SHARED ${FWGS_FILESYSTEM_SOURCES})
target_include_directories(xash3d_filesystem_stdio PRIVATE . ../common ../pm_shared ../engine ../public)
target_link_libraries(xash3d_filesystem_stdio fwgs_public android)
target_compile_options(xash3d_filesystem_stdio PRIVATE -fno-exceptions)
set(xash_filesystem_stdio_DEFINITIONS "")
if(MEMFD_CREATE_TEST)
set(xash_filesystem_stdio_DEFINITIONS ";HAVE_MEMFD_CREATE")
endif()
if(DIRENT_D_TYPE_TEST)
set(xash_filesystem_stdio_DEFINITIONS ";HAVE_DIRENT_D_TYPE")
endif()
set_target_properties(xash3d_filesystem_stdio PROPERTIES COMPILE_DEFINITIONS ${xash_filesystem_stdio_DEFINITIONS})

set_target_properties(xash3d_filesystem_stdio PROPERTIES OUTPUT_NAME "filesystem_stdio")
