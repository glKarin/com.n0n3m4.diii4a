set(TARGET compat)
add_library(${TARGET} OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/compat/compat.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/compat/compat_str.c")
set_target_properties(${TARGET} PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(${TARGET}_dl OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/compat/compat_dl.c")
set_target_properties(${TARGET}_dl PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(${TARGET}_str OBJECT
    "${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/compat/compat_str.c")
set_target_properties(${TARGET}_str PROPERTIES POSITION_INDEPENDENT_CODE ON)
