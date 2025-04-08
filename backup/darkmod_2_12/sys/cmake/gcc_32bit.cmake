# To compile for 32bit platform on Linux, call CMake like this:
# cmake path/to/source -DCMAKE_TOOLCHAIN_FILE=path/to/source/sys/cmake/gcc_32bit.cmake

set(CMAKE_CXX_FLAGS_INIT "-m32 -msse2")
set(CMAKE_C_FLAGS_INIT "-m32 -msse2")
