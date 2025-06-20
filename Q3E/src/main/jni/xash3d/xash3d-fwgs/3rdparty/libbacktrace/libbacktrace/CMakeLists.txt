cmake_minimum_required(VERSION 3.9)
project (xash3d-fwgs-libbacktrace)

set(FWGS_LIBBACKTRACE_SOURCES
atomic.c dwarf.c fileline.c posix.c print.c sort.c state.c backtrace.c simple.c mmap.c mmapio.c
)

set(FWGS_LIBBACKTRACE_SOURCES ${FWGS_LIBBACKTRACE_SOURCES} elf.c)

if(CMAKE_SIZEOF_VOID_P EQUAL 8)
set(BACKTRACE_ELF_SIZE 64)
else()
set(BACKTRACE_ELF_SIZE 32)
endif()

set(BACKTRACE_ELF_SIZE ${BACKTRACE_ELF_SIZE})
set(BACKTRACE_XCOFF_SIZE ${BACKTRACE_ELF_SIZE})
set(_ALL_SOURCE 1)
set(_GNU_SOURCE 1)
set(_POSIX_PTHREAD_SEMANTICS 1)
set(_TANDEM_SOURCE 1)
set(__EXTENSIONS__ 1)
set(_DARWIN_USE_64_BIT_INODE 1)
set(_LARGE_FILES 1)
set(HAVE_ZLIB 1)

include(CheckIncludeFiles)

macro(check_include_files2 h v)
	check_include_files(${h} ${v})
	if(NOT ${v})
		set(${v} 0)
	endif()
endmacro()

check_include_files2(dlfcn.h INCLUDE_dlfcn_H)
check_include_files2(inttypes.h INCLUDE_inttypes_H)
check_include_files2(link.h INCLUDE_link_H)
check_include_files2(sys/link.h INCLUDE_sys_link_H)
check_include_files2(mach-o/dyld.h INCLUDE_mach_o_dyld_H)
check_include_files2(memory.h INCLUDE_memory_H)
check_include_files2(stdint.h INCLUDE_stdint_H)
check_include_files2(stdlib.h INCLUDE_stdlib_H)
check_include_files2(strings.h INCLUDE_strings_H)
check_include_files2(string.h INCLUDE_string_H)
check_include_files2(sys/ldr.h INCLUDE_sys_ldr_H)
check_include_files2(sys/mman.h INCLUDE_sys_mman_H)
check_include_files2(sys/stat.h INCLUDE_sys_stat_H)
check_include_files2(sys/types.h INCLUDE_sys_types_H)
check_include_files2(tlhelp32.h INCLUDE_tlhelp32_H)
check_include_files2(unistd.h INCLUDE_unistd_H)
check_include_files2(windows.h INCLUDE_windows_H)

set(BACKTRACE_SUPPORTED 1)
set(BACKTRACE_USES_MALLOC 1)
set(BACKTRACE_SUPPORTS_THREADS 1)
set(BACKTRACE_SUPPORTS_DATA 1)

configure_file(${CMAKE_CURRENT_SOURCE_DIR}/config.h.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/config.h @ONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/backtrace-supported.h.in ${CMAKE_CURRENT_BINARY_DIR}/backtrace-supported.h @ONLY)

add_library (fwgs_libbacktrace STATIC ${FWGS_LIBBACKTRACE_SOURCES})
target_include_directories(fwgs_libbacktrace PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(fwgs_libbacktrace z)
