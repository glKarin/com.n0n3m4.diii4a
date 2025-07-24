cmake_minimum_required(VERSION 3.1)
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

project(fwgs-Opus LANGUAGES C)

include(OpusFunctions)
include(OpusBuildtype)
include(OpusConfig)
include(OpusSources)
include(GNUInstallDirs)
include(CMakeDependentOption)
include(FeatureSummary)

set(OPUS_BUILD_SHARED_LIBRARY_HELP_STR "build shared library.")
set(OPUS_BUILD_SHARED_LIBRARY False) #k: change option to set, because opus project has exists
if(OPUS_BUILD_SHARED_LIBRARY OR BUILD_SHARED_LIBS OR OPUS_BUILD_FRAMEWORK)
  # Global flag to cause add_library() to create shared libraries if on.
  set(BUILD_SHARED_LIBS ON)
  set(OPUS_BUILD_SHARED_LIBRARY ON)
endif()
add_feature_info(OPUS_BUILD_SHARED_LIBRARY OPUS_BUILD_SHARED_LIBRARY ${OPUS_BUILD_SHARED_LIBRARY_HELP_STR})

set(OPUS_BUILD_TESTING_HELP_STR "build tests.")
option(OPUS_BUILD_TESTING ${OPUS_BUILD_TESTING_HELP_STR} OFF)
if(OPUS_BUILD_TESTING OR BUILD_TESTING)
  set(OPUS_BUILD_TESTING ON)
  set(BUILD_TESTING ON)
endif()
add_feature_info(OPUS_BUILD_TESTING OPUS_BUILD_TESTING ${OPUS_BUILD_TESTING_HELP_STR})

set(OPUS_CUSTOM_MODES_HELP_STR "enable non-Opus modes, e.g. 44.1 kHz & 2^n frames.")
set(OPUS_CUSTOM_MODES True) #k: change option to set, because opus project has exists #k OFF
add_feature_info(OPUS_CUSTOM_MODES OPUS_CUSTOM_MODES ${OPUS_CUSTOM_MODES_HELP_STR})

set(OPUS_BUILD_PROGRAMS_HELP_STR "build programs.")
option(OPUS_BUILD_PROGRAMS ${OPUS_BUILD_PROGRAMS_HELP_STR} OFF)
add_feature_info(OPUS_BUILD_PROGRAMS OPUS_BUILD_PROGRAMS ${OPUS_BUILD_PROGRAMS_HELP_STR})

set(OPUS_DISABLE_INTRINSICS_HELP_STR "disable all intrinsics optimizations.")
option(OPUS_DISABLE_INTRINSICS ${OPUS_DISABLE_INTRINSICS_HELP_STR} OFF)
add_feature_info(OPUS_DISABLE_INTRINSICS OPUS_DISABLE_INTRINSICS ${OPUS_DISABLE_INTRINSICS_HELP_STR})

set(OPUS_FIXED_POINT_HELP_STR "compile as fixed-point (for machines without a fast enough FPU).")
option(OPUS_FIXED_POINT ${OPUS_FIXED_POINT_HELP_STR} OFF)
add_feature_info(OPUS_FIXED_POINT OPUS_FIXED_POINT ${OPUS_FIXED_POINT_HELP_STR})

set(OPUS_ENABLE_FLOAT_API_HELP_STR "compile with the floating point API (for machines with float library).")
option(OPUS_ENABLE_FLOAT_API ${OPUS_ENABLE_FLOAT_API_HELP_STR} ON)
add_feature_info(OPUS_ENABLE_FLOAT_API OPUS_ENABLE_FLOAT_API ${OPUS_ENABLE_FLOAT_API_HELP_STR})

set(OPUS_FLOAT_APPROX_HELP_STR "enable floating point approximations (Ensure your platform supports IEEE 754 before enabling).")
option(OPUS_FLOAT_APPROX ${OPUS_FLOAT_APPROX_HELP_STR} OFF)
add_feature_info(OPUS_FLOAT_APPROX OPUS_FLOAT_APPROX ${OPUS_FLOAT_APPROX_HELP_STR})

set(OPUS_ASSERTIONS_HELP_STR "additional software error checking.")
option(OPUS_ASSERTIONS ${OPUS_ASSERTIONS_HELP_STR} OFF)
add_feature_info(OPUS_ASSERTIONS OPUS_ASSERTIONS ${OPUS_ASSERTIONS_HELP_STR})

set(OPUS_HARDENING_HELP_STR "run-time checks that are cheap and safe for use in production.")
option(OPUS_HARDENING ${OPUS_HARDENING_HELP_STR} ON)
add_feature_info(OPUS_HARDENING OPUS_HARDENING ${OPUS_HARDENING_HELP_STR})

set(OPUS_FUZZING_HELP_STR "causes the encoder to make random decisions (do not use in production).")
option(OPUS_FUZZING ${OPUS_FUZZING_HELP_STR} OFF)
add_feature_info(OPUS_FUZZING OPUS_FUZZING ${OPUS_FUZZING_HELP_STR})

set(OPUS_CHECK_ASM_HELP_STR "enable bit-exactness checks between optimized and c implementations.")
option(OPUS_CHECK_ASM ${OPUS_CHECK_ASM_HELP_STR} OFF)
add_feature_info(OPUS_CHECK_ASM OPUS_CHECK_ASM ${OPUS_CHECK_ASM_HELP_STR})

set(OPUS_FIXED_POINT_DEBUG_HELP_STR "debug fixed-point implementation.")
cmake_dependent_option(OPUS_FIXED_POINT_DEBUG
                      ${OPUS_FIXED_POINT_DEBUG_HELP_STR}
                      ON
                      "OPUS_FIXED_POINT; OPUS_FIXED_POINT_DEBUG"
                      OFF)
add_feature_info(OPUS_FIXED_POINT_DEBUG OPUS_FIXED_POINT_DEBUG ${OPUS_FIXED_POINT_DEBUG_HELP_STR})

set(OPUS_VAR_ARRAYS_HELP_STR "use variable length arrays for stack arrays.")
cmake_dependent_option(OPUS_VAR_ARRAYS
                      ${OPUS_VAR_ARRAYS_HELP_STR}
                      ON
                      "VLA_SUPPORTED; NOT OPUS_USE_ALLOCA; NOT OPUS_NONTHREADSAFE_PSEUDOSTACK"
                      OFF)
add_feature_info(OPUS_VAR_ARRAYS OPUS_VAR_ARRAYS ${OPUS_VAR_ARRAYS_HELP_STR})

set(OPUS_USE_ALLOCA_HELP_STR "use alloca for stack arrays (on non-C99 compilers).")
cmake_dependent_option(OPUS_USE_ALLOCA
                       ${OPUS_USE_ALLOCA_HELP_STR}
                       ON
                       "USE_ALLOCA_SUPPORTED; NOT OPUS_VAR_ARRAYS; NOT OPUS_NONTHREADSAFE_PSEUDOSTACK"
                       OFF)
add_feature_info(OPUS_USE_ALLOCA OPUS_USE_ALLOCA ${OPUS_USE_ALLOCA_HELP_STR})

set(OPUS_NONTHREADSAFE_PSEUDOSTACK_HELP_STR "use a non threadsafe pseudostack when neither variable length arrays or alloca is supported.")
cmake_dependent_option(OPUS_NONTHREADSAFE_PSEUDOSTACK
                       ${OPUS_NONTHREADSAFE_PSEUDOSTACK_HELP_STR}
                       ON
                       "NOT OPUS_VAR_ARRAYS; NOT OPUS_USE_ALLOCA"
                       OFF)
add_feature_info(OPUS_NONTHREADSAFE_PSEUDOSTACK OPUS_NONTHREADSAFE_PSEUDOSTACK ${OPUS_NONTHREADSAFE_PSEUDOSTACK_HELP_STR})

set(OPUS_FAST_MATH_HELP_STR "enable fast math (unsupported and discouraged use, as code is not well tested with this build option).")
cmake_dependent_option(OPUS_FAST_MATH
                       ${OPUS_FAST_MATH_HELP_STR}
                       ON
                       "OPUS_FLOAT_APPROX; OPUS_FAST_MATH; FAST_MATH_SUPPORTED"
                       OFF)
add_feature_info(OPUS_FAST_MATH OPUS_FAST_MATH ${OPUS_FAST_MATH_HELP_STR})

set(OPUS_STACK_PROTECTOR_HELP_STR "use stack protection.")
cmake_dependent_option(OPUS_STACK_PROTECTOR
                       ${OPUS_STACK_PROTECTOR_HELP_STR}
                       ON
                       "STACK_PROTECTOR_SUPPORTED"
                       OFF)
add_feature_info(OPUS_STACK_PROTECTOR OPUS_STACK_PROTECTOR ${OPUS_STACK_PROTECTOR_HELP_STR})

if(NOT MSVC)
  set(OPUS_FORTIFY_SOURCE_HELP_STR "add protection against buffer overflows.")
  cmake_dependent_option(OPUS_FORTIFY_SOURCE
                         ${OPUS_FORTIFY_SOURCE_HELP_STR}
                         ON
                         "FORTIFY_SOURCE_SUPPORTED"
                         OFF)
  add_feature_info(OPUS_FORTIFY_SOURCE OPUS_FORTIFY_SOURCE ${OPUS_FORTIFY_SOURCE_HELP_STR})
endif()

feature_summary(WHAT ALL)

set(Opus_PUBLIC_HEADER
    ${CMAKE_CURRENT_SOURCE_DIR}/include/opus.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/opus_defines.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/opus_multistream.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/opus_projection.h
    ${CMAKE_CURRENT_SOURCE_DIR}/include/opus_types.h)

if(OPUS_CUSTOM_MODES)
  list(APPEND Opus_PUBLIC_HEADER ${CMAKE_CURRENT_SOURCE_DIR}/include/opus_custom.h)
endif()

add_library(fwgs_opus STATIC ${opus_headers} ${opus_sources} ${opus_sources_float} ${Opus_PUBLIC_HEADER})

target_include_directories(
  fwgs_opus
  PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include
  PRIVATE ${CMAKE_CURRENT_BINARY_DIR}
          ${CMAKE_CURRENT_SOURCE_DIR}
          celt
          silk)

target_link_libraries(fwgs_opus PRIVATE ${OPUS_REQUIRED_LIBRARIES})
target_compile_definitions(fwgs_opus PRIVATE OPUS_BUILD)

if(OPUS_FIXED_POINT_DEBUG)
  target_compile_definitions(fwgs_opus PRIVATE FIXED_DEBUG)
endif()

if(OPUS_FORTIFY_SOURCE AND NOT MSVC)
  target_compile_definitions(fwgs_opus PRIVATE
                             $<$<NOT:$<CONFIG:debug>>:_FORTIFY_SOURCE=2>)
endif()

if(OPUS_FLOAT_APPROX)
  target_compile_definitions(fwgs_opus PRIVATE FLOAT_APPROX)
endif()

if(OPUS_ASSERTIONS)
  target_compile_definitions(fwgs_opus PRIVATE ENABLE_ASSERTIONS)
endif()

if(OPUS_HARDENING)
  target_compile_definitions(fwgs_opus PRIVATE ENABLE_HARDENING)
endif()

if(OPUS_FUZZING)
  target_compile_definitions(fwgs_opus PRIVATE FUZZING)
endif()

if(OPUS_CHECK_ASM)
  target_compile_definitions(fwgs_opus PRIVATE OPUS_CHECK_ASM)
endif()

if(OPUS_VAR_ARRAYS)
  target_compile_definitions(fwgs_opus PRIVATE VAR_ARRAYS)
elseif(OPUS_USE_ALLOCA)
  target_compile_definitions(fwgs_opus PRIVATE USE_ALLOCA)
elseif(OPUS_NONTHREADSAFE_PSEUDOSTACK)
  target_compile_definitions(fwgs_opus PRIVATE NONTHREADSAFE_PSEUDOSTACK)
else()
  message(ERROR "Need to set a define for stack allocation")
endif()

if(OPUS_CUSTOM_MODES)
  target_compile_definitions(fwgs_opus PRIVATE CUSTOM_MODES)
endif()

if(OPUS_FAST_MATH)
    target_compile_options(fwgs_opus PRIVATE -ffast-math)
endif()

if(OPUS_STACK_PROTECTOR)
    target_compile_options(fwgs_opus PRIVATE -fstack-protector-strong)
endif()

set_target_properties(fwgs_opus PROPERTIES C_VISIBILITY_PRESET hidden)

add_sources_group(fwgs_opus silk ${silk_headers} ${silk_sources})
add_sources_group(fwgs_opus celt ${celt_headers} ${celt_sources})

if(OPUS_FIXED_POINT)
  add_sources_group(fwgs_opus silk ${silk_sources_fixed})
  target_include_directories(fwgs_opus PRIVATE silk/fixed)
  target_compile_definitions(fwgs_opus PRIVATE FIXED_POINT=1)
else()
  add_sources_group(fwgs_opus silk ${silk_sources_float})
  target_include_directories(fwgs_opus PRIVATE silk/float)
endif()

if(NOT OPUS_ENABLE_FLOAT_API)
  target_compile_definitions(fwgs_opus PRIVATE DISABLE_FLOAT_API)
endif()

target_compile_definitions(fwgs_opus
                           PRIVATE
                           $<$<BOOL:${HAVE_LRINT}>:HAVE_LRINT>
                           $<$<BOOL:${HAVE_LRINTF}>:HAVE_LRINTF>)
