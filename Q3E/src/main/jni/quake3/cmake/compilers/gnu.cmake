# GNU style (GCC/Clang) compiler specific settings

if(NOT CMAKE_C_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_C_COMPILER_ID MATCHES "^(Apple)?Clang$")
    return()
endif()

enable_language(ASM)

set(ASM_SOURCES
    ${SOURCE_DIR}/asm/ftola.c
    ${SOURCE_DIR}/asm/snapvector.c
)

if(DIII4A)
add_compile_options(-Wall -Wimplicit -Wshadow
    -Wstrict-prototypes -Wformat=2  -Wformat-security
    -Wstrict-aliasing=2 -Wmissing-format-attribute
    -Wdisabled-optimization)
else()
add_compile_options(-Wall -Wimplicit -Wshadow
    -Wstrict-prototypes -Wformat=2  -Wformat-security
    -Wstrict-aliasing=2 -Wmissing-format-attribute
    -Wdisabled-optimization -Werror-implicit-function-declaration)
endif()

add_compile_options(-Wno-format-zero-length -Wno-format-nonliteral)

# There are lots of instances of union based aliasing in the code
# that rely on the compiler not optimising them away, so disable it
add_compile_options(-fno-strict-aliasing)

# This is necessary to hide all symbols unless explicitly exported
# via the Q_EXPORT macro
add_compile_options(-fvisibility=hidden)
