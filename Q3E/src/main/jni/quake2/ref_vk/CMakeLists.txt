cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

# Enforce "Debug" as standard build type.
if(NOT CMAKE_BUILD_TYPE)
	set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel." FORCE)
endif()

# CMake project configuration.
project(yquake2_ref_vk C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED OFF)

if (MSVC)
	add_compile_options(/MP) # parallel build (use all cores, or as many as configured in VS)

	# ignore some compiler warnings
	add_compile_options(/wd4244 /wd4305) # possible loss of data/truncation (double to float etc; ignore)
	add_compile_options(/wd4018) # signed/unsigned mismatch
	add_compile_options(/wd4996) # 'function': was declared deprecated (like all that secure CRT stuff)
	# don't show me warnings for system headers, why the fuck isn't this default
	add_compile_options(/experimental:external /external:W0)
else() # GCC/clang/mingw
# Enforce compiler flags:
#  -Wall                -> More warnings
#  -fno-strict-aliasing -> Quake 2 is far away from strict aliasing
#  -fwrapv              -> Make signed integer overflows defined
#  -fvisibility=hidden	-> Force defaultsymbol visibility to hidden
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -fno-strict-aliasing -fwrapv -fvisibility=hidden")

# Use -O2 as maximum optimization level. -O3 has it's problems with yquake2.
string(REPLACE "-O3" "-O2" CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE}")
endif() # MSVC'S else-case

# Switch off some annoying warnings
if (${CMAKE_C_COMPILER_ID} STREQUAL "Clang")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-missing-braces")
elseif (${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
	if (CMAKE_C_COMPILER_VERSION GREATER 7.99)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-format-truncation -Wno-format-overflow")
	endif()
endif()

# Set directory locations (allowing us to move directories easily)
set(SOURCE_DIR ${CMAKE_CURRENT_LIST_DIR}/src)
set(BACKENDS_SRC_DIR ${SOURCE_DIR}/backends)
set(COMMON_SRC_DIR ${SOURCE_DIR}/common)
set(CONSTANTS_SRC_DIR ${SOURCE_DIR}/constants)
set(FILES_SRC_DIR ${SOURCE_DIR}/files)
set(VK_SRC_DIR ${SOURCE_DIR}/vk)

# Operating system.
set(YQ2OSTYPE "${CMAKE_SYSTEM_NAME}" CACHE STRING "Override operation system type")
add_definitions(-DYQ2OSTYPE="${YQ2OSTYPE}")

# Architecture string
# work around CMake's useless/broken CMAKE_SYSTEM_PROCESSOR (taken from dhewm3)

set(cpu ${CMAKE_SYSTEM_PROCESSOR})

# Originally, ${CMAKE_SYSTEM_PROCESSOR} was supposed to contain the *target* CPU, according to CMake's documentation.
# As far as I can tell this has always been broken (always returns host CPU) at least on Windows
# (see e.g. https://cmake.org/pipermail/cmake-developers/2014-September/011405.html) and wasn't reliable on
# other systems either, for example on Linux with 32bit userland but 64bit kernel it returned the kernel CPU type
# (e.g. x86_64 instead of i686). Instead of fixing this, CMake eventually updated their documentation in 3.20,
# now it's officially the same as CMAKE_HOST_SYSTEM_PROCESSOR except when cross-compiling (where it's explicitly set)
# So we gotta figure out the actual target CPU type ourselves..
if(NOT (CMAKE_SYSTEM_PROCESSOR STREQUAL CMAKE_HOST_SYSTEM_PROCESSOR))
	# special case: cross-compiling, here CMAKE_SYSTEM_PROCESSOR should be correct, hopefully
	# (just leave cpu at ${CMAKE_SYSTEM_PROCESSOR})
elseif(MSVC)
	# because all this wasn't ugly enough, it turned out that, unlike standalone CMake, Visual Studio's
	# integrated CMake doesn't set CMAKE_GENERATOR_PLATFORM, so I gave up on guessing the CPU arch here
	# and moved the CPU detection to MSVC-specific code in neo/sys/platform.h
else() # not MSVC and not cross-compiling, assume GCC or clang (-compatible), seems to work for MinGW as well
	execute_process(COMMAND ${CMAKE_C_COMPILER} "-dumpmachine"
	                RESULT_VARIABLE cc_dumpmachine_res
	                OUTPUT_VARIABLE cc_dumpmachine_out)
	if(cc_dumpmachine_res EQUAL 0)
		string(STRIP ${cc_dumpmachine_out} cc_dumpmachine_out) # get rid of trailing newline
		message(DEBUG "`${CMAKE_C_COMPILER} -dumpmachine` says: \"${cc_dumpmachine_out}\"")
		# gcc -dumpmachine and clang -dumpmachine seem to print something like "x86_64-linux-gnu" (gcc)
		# or "x64_64-pc-linux-gnu" (clang) or "i686-w64-mingw32" (32bit mingw-w64) i.e. starting with the CPU,
		# then "-" and then OS or whatever - so use everything up to first "-"
		string(REGEX MATCH "^[^-]+" cpu ${cc_dumpmachine_out})
		message(DEBUG "  => CPU architecture extracted from that: \"${cpu}\"")
	else()
		message(WARNING "${CMAKE_C_COMPILER} -dumpmachine failed with error (code) ${cc_dumpmachine_res}")
		message(WARNING "will use the (sometimes incorrect) CMAKE_SYSTEM_PROCESSOR (${cpu}) to determine YQ2ARCH")
	endif()
endif()

if(cpu STREQUAL "powerpc")
	set(cpu "ppc")
elseif(cpu STREQUAL "aarch64")
	# "arm64" is more obvious, and some operating systems (like macOS) use it instead of "aarch64"
	set(cpu "arm64")
elseif(cpu MATCHES "[aA][mM][dD]64" OR cpu MATCHES "[xX].*64")
	set(cpu "x86_64")
elseif(cpu MATCHES "i.86" OR cpu MATCHES "[xX]86")
	set(cpu "i386")
elseif(cpu MATCHES "[aA][rR][mM].*") # some kind of arm..
	# On 32bit Raspbian gcc -dumpmachine returns sth starting with "arm-",
	# while clang -dumpmachine says "arm6k-..." - try to unify that to "arm"
	if(CMAKE_SIZEOF_VOID_P EQUAL 8) # sizeof(void*) == 8 => must be arm64
		set(cpu "arm64")
	else() # should be 32bit arm then (probably "armv7l" "armv6k" or sth like that)
		set(cpu "arm")
	endif()
endif()

if(MSVC)
	# for MSVC YQ2ARCH is set in code (in src/common/header/common.h)
	message(STATUS "Setting YQ2OSTYPE to \"${YQ2OSTYPE}\" - NOT setting YQ2ARCH, because we're targeting MSVC (VisualC++)")
else()
	set(ARCH "${cpu}")
	add_definitions(-DYQ2ARCH="${ARCH}")
	message(STATUS "Setting YQ2OSTYPE to \"${YQ2OSTYPE}\" and YQ2ARCH to \"${ARCH}\".")
endif()
# make sure that ${cpu} isn't used below - if at all use ${ARCH}, but not when compiling with MSVC!
unset(cpu)

# END OF workarounds for CMake's poor choices regarding CPU architecture detection


set(yquake2IncludeDirectories ${CMAKE_CURRENT_SOURCE_DIR}/../../depincs/vulkan ${CMAKE_CURRENT_SOURCE_DIR}/../../common)
set(yquake2ClientLinkerFlags android EGL dl)

# these settings only work for GCC and clang
if(CMAKE_COMPILER_IS_GNUCC OR CMAKE_C_COMPILER_ID STREQUAL "Clang")
	# If we're building with gcc for i386 let's define -ffloat-store.
	# This helps the old and crappy x87 FPU to produce correct values.
	# Would be nice if Clang had something comparable.
	if ("${ARCH}" STREQUAL "i386" AND ${CMAKE_C_COMPILER_ID} STREQUAL "GNU")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffloat-store")
	endif()

	# Force SSE math on x86_64. All sane compilers should do this
	# anyway, just to protect us from broken Linux distros.
	if ("${ARCH}" STREQUAL "x86_64")
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -mfpmath=sse")
	endif()

	if ("${ARCH}" STREQUAL "arm")
		if (CMAKE_SIZEOF_VOID_P EQUAL 4)
			set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=armv6k")
		endif()
	endif()
endif()

if(WIN32)
set(Backend-Source
	${BACKENDS_SRC_DIR}/hunk_windows.c
	)
elseif(ANDROID)
set(Backend-Source
	${BACKENDS_SRC_DIR}/hunk_unix.c
	${BACKENDS_SRC_DIR}/android_vk.c
	)
else()
set(Backend-Source
	${BACKENDS_SRC_DIR}/hunk_unix.c
	)
endif()

set(Constant-Header
	${CONSTANTS_SRC_DIR}/anorms.h
	${CONSTANTS_SRC_DIR}/anormtab.h
	${CONSTANTS_SRC_DIR}/warpsin.h
	)

set(Common-Source
	${COMMON_SRC_DIR}/md4.c
	${COMMON_SRC_DIR}/shared.c
	${COMMON_SRC_DIR}/utils.c
	)

set(Common-Header
	${COMMON_SRC_DIR}/header/common.h
	${COMMON_SRC_DIR}/header/crc.h
	${COMMON_SRC_DIR}/header/files.h
	${COMMON_SRC_DIR}/header/ref_api.h
	${COMMON_SRC_DIR}/header/ref_shared.h
	${COMMON_SRC_DIR}/header/shared.h
	${COMMON_SRC_DIR}/header/shared_safe.h
	${COMMON_SRC_DIR}/header/vid.h
	)

set(File-Source
	${FILES_SRC_DIR}/models.c
	${FILES_SRC_DIR}/pcx.c
	${FILES_SRC_DIR}/pvs.c
	${FILES_SRC_DIR}/stb.c
	${FILES_SRC_DIR}/surf.c
	${FILES_SRC_DIR}/wal.c
	)

set(File-Header
	${FILES_SRC_DIR}/stb_image.h
	${FILES_SRC_DIR}/stb_image_resize.h
	)

set(VK-Source
	${VK_SRC_DIR}/vk_buffer.c
	${VK_SRC_DIR}/vk_cmd.c
	${VK_SRC_DIR}/vk_common.c
	${VK_SRC_DIR}/vk_device.c
	${VK_SRC_DIR}/vk_draw.c
	${VK_SRC_DIR}/vk_image.c
	${VK_SRC_DIR}/vk_light.c
	${VK_SRC_DIR}/vk_mesh.c
	${VK_SRC_DIR}/vk_model.c
	${VK_SRC_DIR}/vk_pipeline.c
	${VK_SRC_DIR}/vk_main.c
	${VK_SRC_DIR}/vk_misc.c
	${VK_SRC_DIR}/vk_surf.c
	${VK_SRC_DIR}/vk_shaders.c
	${VK_SRC_DIR}/vk_swapchain.c
	${VK_SRC_DIR}/vk_validation.c
	${VK_SRC_DIR}/vk_warp.c
	${VK_SRC_DIR}/vk_util.c
	${VK_SRC_DIR}/volk/volk.c
	)

set(VK-Header
	${VK_SRC_DIR}/header/local.h
	${VK_SRC_DIR}/header/model.h
	${VK_SRC_DIR}/header/qvk.h
	${VK_SRC_DIR}/header/shaders.h
	${VK_SRC_DIR}/header/util.h
	${VK_SRC_DIR}/volk/volk.h
	)


# Build the Vulkan dynamic library
add_library(ref_vk SHARED ${VK-Source} ${VK-Header} ${Backend-Source} ${Constant-Header} ${Common-Source} ${Common-Header} ${File-Source} ${File-Header})
target_compile_definitions(ref_vk PRIVATE VK_USE_PLATFORM_ANDROID_KHR)
target_include_directories(ref_vk PRIVATE ${yquake2IncludeDirectories})
target_link_libraries(ref_vk ${yquake2ClientLinkerFlags} )
