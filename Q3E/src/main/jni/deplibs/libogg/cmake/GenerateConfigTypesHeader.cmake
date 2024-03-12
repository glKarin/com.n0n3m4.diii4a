
check_include_files(inttypes.h INCLUDE_INTTYPES_H)
check_include_files(stdint.h INCLUDE_STDINT_H)
check_include_files(sys/types.h INCLUDE_SYS_TYPES_H)

set(SIZE16 int16_t)
set(USIZE16 uint16_t)
set(SIZE32 int32_t)
set(USIZE32 uint32_t)
set(SIZE64 int64_t)
set(USIZE64 uint64_t)

include(${CMAKE_CURRENT_LIST_DIR}/CheckSizes.cmake)

configure_file(${CMAKE_CURRENT_LIST_DIR}/../include/ogg/config_types.h.in ${OGG_CONFIG_TYPES_HEADER_PATH}/ogg/config_types.h @ONLY)