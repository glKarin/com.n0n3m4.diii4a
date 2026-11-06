###Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com>
###This code is public domain and can be freely used or copied.

dnl defines the version name of  the libdwarf.so
m4_define([v_maj], [2])
m4_define([v_min], [3])
m4_define([v_mic], [2])
m4_define([v_ver], [v_maj.v_min.v_mic])

m4_define([v_rel], [])
m4_define([lt_cur], [m4_eval(v_maj + v_min)])
m4_define([lt_rev], [v_mic])
m4_define([lt_age], [v_min])

### Sets the release name.
###m4_define([v_date], [m4_esyscmd_s([date "+%Y%m%d"])])
###Also SET (VERSION ..) in CMakeLists.txt

AC_PREREQ([2.52])
### 2nd arg to AC_INIT is the version 'number'.
AC_INIT([libdwarf], [v_ver], [libdwarf -at- linuxmail -dot- org])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_MACRO_DIRS([m4])
### The following for libdwarf.pc.in
AC_SUBST([PROJECT_VERSION],[v_ver])

### not yet, will be used before long.
###AM_PATH_PYTHON([3.0])

### Additional options to configure

AC_ARG_ENABLE([dwarfgen],
   [AS_HELP_STRING([--enable-dwarfgen],
                   [enable dwarfgen compilation @<:@default=no@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xyes"],
        [enable_dwarfgen="yes" ],
        [enable_dwarfgen="no"])
   ],
   [enable_dwarfgen="no"])
AM_CONDITIONAL([HAVE_DWARFGEN], [test "x${enable_dwarfgen}" = "xyes"])

AC_ARG_ENABLE([dwarfexample],
   [AS_HELP_STRING([--enable-dwarfexample],
                   [enable dwarfexample compilation @<:@default=no@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xyes"],
        [enable_dwarfexample="yes"],
        [enable_dwarfexample="no"])
   ],
   [enable_dwarfexample="no"])
AM_CONDITIONAL([HAVE_DWARFEXAMPLE], [test "x${enable_dwarfexample}" = "xyes"])

AC_ARG_ENABLE([sanitize],
   [AS_HELP_STRING([--enable-sanitize],
                   [enable sanitize compiler option @<:@default=no@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xyes"],
        [enable_sanitize="yes"],
        [enable_sanitize="no"])
   ],
   [enable_sanitize="no"])

AC_ARG_ENABLE([decompression],
   [AS_HELP_STRING([--enable-decompression],
                   [enable decompressing sections  @<:@default=yes@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xno"],
        [enable_decompression="no"],
        [enable_decompression="yes"])
   ],
   [enable_decompression="yes"])


AC_ARG_ENABLE([wall],
   [AS_HELP_STRING([--enable-wall],
                   [enable -Wall and other options @<:@default=no@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xyes"],
        [enable_wall="yes"],
        [enable_wall="no"])
   ],
   [enable_wall="no"])

AC_ARG_ENABLE([doc],
   [AS_HELP_STRING([--enable-doc],
      [build and install Doxygen documentation @<:@default=no@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xyes"],
        [enable_doc="yes"],
        [enable_doc="no"])
   ],
   [enable_doc="no"])

AC_ARG_ENABLE([mmap],
   [AS_HELP_STRING([--enable-mmap],
      [build with mmap support if possible @<:@default=yes@:>@])],
   [
    AS_IF(
        [test "x${enableval}" = "xno"],
        [enable_mmap="no"],
        [enable_mmap="yes"])
   ],
   [enable_mmap="yes"])

### Default options with respect to host

AC_CANONICAL_HOST

have_win32="no"
case "$host_os" in
   mingw*)
      have_win32="yes"
   ;;
esac
### supress debuglink tests if windows.
AM_CONDITIONAL([HAVE_DEBUGLINK], [test "x${have_win32}" = "xno"])

### Checks for programs

AM_PROG_AR

### We don't use dist-xz *.xz output from make dist,
### so don't mention it.
AM_INIT_AUTOMAKE([1.6 dist-xz subdir-objects])
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

AS_IF(
    [test "x${have_win32}" = "xyes"],
    [lt_cv_deplibs_check_method=pass_all])
LT_INIT([win32-dll disable-shared static])

AS_IF(
    [test "x${have_win32}" = "xyes" && test "x${enable_static}" = "xyes"],
    [AC_MSG_ERROR([Static library is not supported on Windows])])

version_info="lt_cur:lt_rev:lt_age"
release_info="v_rel"
AC_SUBST([version_info])
AC_SUBST([release_info])

VMAJ=v_maj
AC_SUBST([VMAJ])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_CC_C_O

PKG_PROG_PKG_CONFIG

AC_ARG_VAR([DOXYGEN], [Doxygen documentation generation program])
AS_IF(
    [test "x${enable_doc}" = "xyes"],
    [
     AC_CHECK_PROGS([DOXYGEN], [doxygen], [no])
     AS_IF(
         [test "x${DOXYGEN}" = "xno"],
         [enable_doc="no" ],
         [enable_doc="yes"])
    ],
    [enable_doc="no"])
AM_CONDITIONAL([HAVE_DOC], [test "x${enable_doc}" = "xyes"])

### Checks for libraries
requirements_libdwarf_pc=
AS_IF(
    [test "x${enable_decompression}" = "xyes"],
    [
        PKG_CHECK_MODULES([ZLIB], [zlib],
            [
             have_zlib="yes"
             have_pc_zlib="yes"
             requirements_libdwarf_pc=zlib
            ],
            [
             have_zlib="no"
             have_pc_zlib="no"
            ])
        PKG_CHECK_MODULES([ZSTD], [libzstd],
            [
             have_zstd="yes"
             have_pc_zstd="yes"
             requirements_libdwarf_pc="${requirements_libdwarf_pc} libzstd"
            ],
            [
             have_zstd="no"
             have_pc_zstd="no"
            ])
    ],
    [
        have_zstd="no"
        have_pc_zstd="no"
        have_zlib="no"
        have_pc_zlib="no"
    ])

AC_SUBST([requirements_libdwarf_pc])

AC_SUBST([DWARFGEN_LIBS])
AC_ARG_VAR([DWARFGEN_LIBS], [extra linker flags when linking dwarfgen])
AC_SUBST([DWARF_LIBS])
AC_ARG_VAR([DWARF_LIBS], [extra linker flags when linking dwarfdump])

### Checks for header files

### MacOS does not have malloc.h
AC_CHECK_HEADERS([unistd.h sys/types.h malloc.h])
### for uintptr_t and open and open argument defines
AC_CHECK_HEADERS([stdint.h inttypes.h stddef.h fcntl.h])
AC_CHECK_HEADERS([sys/stat.h stdafx.h])
###relevant for mmap
have_mmap="no"
AS_IF(  
  [test "x${enable_mmap}" = "xyes"],
  [
    AC_COMPILE_IFELSE(
    [ AC_LANG_PROGRAM(
      [[
      #include <unistd.h>
      #include <string.h>
      #include <stdlib.h>
      #include <sys/mman.h>
      ]],
      [[
        long pagesize = sysconf(_SC_PAGESIZE);
        int fd = 4;
        (void)pagesize;
        mmap(0, 100,PROT_READ|PROT_WRITE,MAP_PRIVATE,
            fd,100);
        munmap((void *)100,100);
      ]]
      )
    ],
    [ have_mmap="yes"
      AC_DEFINE(
        [HAVE_FULL_MMAP], [1],
        [Set to 1 as mmap/munmap is usable])
      AC_DEFINE(
        [HAVE_SYS_MMAP_H], [1],
        [Set to 1 as sys/mmap.h is usable])
    ],
    [ have_mmap="no"]
   )
  ],
  [ have_mmap="no"]
)

AS_IF(
    [test "x${enable_decompression}" = "xyes"],
    [
        AS_IF(
            [test "x${have_zlib}" = "xno"],
            [AC_CHECK_HEADERS([zlib.h],
                [have_zlib="yes"],
                [have_zlib="no"])])
        AS_IF(
            [test "x${have_zstd}" = "xno"],
            [AC_CHECK_HEADERS([zstd.h],
                [have_zstd="yes"],
                [have_zstd="no"])])
    ],
    [
        have_zstd="no"
        have_pc_zstd="no"
        have_zlib="no"
        have_pc_zlib="no"
    ])

### for use in casts to uint to avoid 32bit warnings.
### Also needed by C++ cstdint
AC_TYPE_UINTPTR_T
AC_TYPE_INTPTR_T
AC_TYPE_UINT32_T dnl Needed for hash calculation
### Now we know uintptr_t is either in stdint.h or
### is defined in config.h by configure.

### Checks for compiler characteristics

AC_C_BIGENDIAN
AC_SUBST([DWARF_BIGENDIAN],[${ac_cv_c_bigendian}])
AC_ARG_VAR([DWARF_BIGENDIAN],
    [big endian yes/no needed various places])


# gcc accepts even totally bogus -Wno flags. Other compilers..no
# -Wno-long-long suppresses warnings on 'long long'
# -Wno-pedantic-ms-format (which only exists in mingw)
# suppresses warnings about I64 printf format.
# -Wnostringop_overread is to avoid a build fail with

# some versions of gcc when building with -fsanitize
# (and some other flags). See, in libdwarf/dwarf_init_finish.c
# around line 330.
# one such gcc: Alpine 12.2.1_git20220924-r4) 12.2.1 20220924
# error: 'strcmp' reading 1 or more bytes from a region of
# size 0 [-Werror=stringop-overread]
# !strcmp(postzprefix,targname+DPREFIXLEN)
c_common_flags="-Wall -Wextra -Wpointer-arith -Wmissing-declarations -Wcomment -Wformat -Wpedantic -Wuninitialized -Wshadow -Werror -Wno-long-long"
AS_IF(
    [ test "x$enable_wall" = "xyes" ],
    [
       cxx_compiler_flags="$c_common_flags "

       c_compiler_flags="$c_common_flags -Wmissing-prototypes -Wdeclaration-after-statement -Wmissing-parameter-type -Wnested-externs -Wredundant-decls -Wsign-compare"

#-Wnull-dereference causes bogus warnings.
#-Wbad-function-cast removes useful warnings
#-Wnostringop-overread  removes useful warnings
#-Wbad-function-cast  removes useful warnings
#-Wmissing-parameter-type removes useful warnings

    ]
    )
AS_IF(
    [ test "x$have_win32" = "xyes" ],
    [
       cxx_compiler_flags="$cxx_compiler_flags -Wno-pedantic-ms-format"

       c_compiler_flags="$c_compiler_flags -Wno-pedantic-ms-format"
    ]
    )

DWARF_CHECK_CXX_COMPILER_FLAGS([${cxx_compiler_flags}])
DWARF_CHECK_C_COMPILER_FLAGS([${c_compiler_flags}])

# If we can compile then the runtime can do the
# setlocal itself.
# We do not run this since the C compiler needs
# the C locale to compile correctly.
AC_COMPILE_IFELSE(
  [ AC_LANG_PROGRAM(
    [[
    #include <langinfo.h>
    #include <locale.h>
    #include <string.h>
    #include <stdio.h>
    ]],
    [[
    char * langinfo = 0;
    setlocale(LC_CTYPE, "");
    setlocale(LC_NUMERIC, "");
    langinfo = nl_langinfo(CODESET);
    printf("Lang: %s\n",langinfo);
    if (strcmp(langinfo,"UTF-8") && strcmp(langinfo,"UTF8")) {
        return 1;
    }
    ]]
    )
  ],
  [ have_utf8="yes"
    AC_DEFINE(
      [HAVE_UTF8], [1],
      [Set to 1 as utf8 is fully printable])
  ],
  [ have_utf8="no"
    AC_MSG_WARN([utf8 printed uri-style])
  ]
)

# unused attribute
AC_COMPILE_IFELSE(
    [AC_LANG_PROGRAM(
         [[
static unsigned int foo(unsigned int x, __attribute__ ((unused)) int y){
    unsigned int x2 = x + 1;
    return x2;
}
int goo() {
    unsigned int y = 0;
    y = foo(12, y);
}
         ]],
         [[
         ]])
    ],
    [
     have_unused="yes"
     AC_DEFINE(
         [HAVE_UNUSED_ATTRIBUTE], [1],
         [Set to 1 if __attribute__ ((unused)) is available.])
    ],
    [have_unused="no"])

AC_MSG_CHECKING([whether "unused" attribute is available])
AC_MSG_RESULT([${have_unused}])

# sanitize
AS_IF(
    [test "x${enable_sanitize}" = "xyes"],
    [
     CFLAGS_save=${CFLAGS}
     CFLAGS="${CFLAGS} -fsanitize=address -fsanitize=leak -fsanitize=undefined -fno-omit-frame-pointer"
     AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM([[]], [[]])],
         [
          enable_sanitize="yes"
          DWARF_CFLAGS="$DWARF_CFLAGS -fsanitize=address -fsanitize=leak -fsanitize=undefined -fno-omit-frame-pointer"
          LDFLAGS="$LDFLAGS -fsanitize=address -fsanitize=leak -fsanitize=undefined -fno-omit-frame-pointer"
         ],
         [enable_sanitize="no"])
     CFLAGS="${CFLAGS_save} ${DWARF_CFLAGS}"
    ])

AC_MSG_CHECKING([whether sanitize options are used])
AC_MSG_RESULT([${enable_sanitize}])

# if the builder specifies -std=c99 or various other
# options linking will fail as strdup() is hidden.
# So we do the following.
CPPFLAGS="${CPPFLAGS} -D_GNU_SOURCE"

# -fvisibility
AS_IF(
    [test "x${enable_shared}" = "xyes" && test "x${have_win32}" = "xno"],
    [
     CFLAGS_save=${CFLAGS}
     CFLAGS="${CFLAGS} -fvisibility=hidden -fno-omit-frame-pointer"
     AC_COMPILE_IFELSE(
         [AC_LANG_PROGRAM([[]], [[]])],
         [
          DWARF_CFLAGS="$DWARF_CFLAGS -fvisibility=hidden -fno-omit-frame-pointer"
          LDFLAGS="$LDFLAGS -fvisibility=hidden -fno-omit-frame-pointer"
         ],
         [AC_MSG_NOTICE([Option -fvisibility is ignored. Not supported.])])
     CFLAGS="${CFLAGS_save} ${DWARF_CFLAGS}"
    ])

### Checks for linker characteristics

### Checks for library functions

AC_FUNC_ALLOCA

requirements_libdwarf_libs=
AS_IF(
    [test "x${enable_decompression}" = "xyes"],
    [
        AS_IF(
            echo "dadebug zlib ${have_pc_zlib} ${have_zlib}"
            [test "x${have_pc_zlib}" = "xno"  -a "x${have_zlib}" = "xyes"],
            [
                AC_SEARCH_LIBS(
                [uncompress], [z],
                [
                 ZLIB_LIBS="-lz"
                 requirements_libdwarf_libs="-lz"
                 echo "dadebug req set lz"
                ],
                [have_zlib="no"])
            ])
    ],
    [
        have_zlib="no"
    ])
AS_IF(
    [test "x${enable_decompression}" = "xyes"],
    [
        AS_IF(
            echo "dadebug zstd ${have_pc_zstd} ${have_zstd}"
            [test "x${have_pc_zstd}" = "xno"  -a "x${have_zstd}" = "xyes"],
            [
                AC_SEARCH_LIBS(
                [ZSTD_decompress], [zstd],
                [
                 ZSTD_LIBS="-lzstd"
                 echo "dadebug req set zstd"
                 requirements_libdwarf_libs="${requirements_libdwarf_libs} -lzstd"
                ],
                [have_zstd="no"])
            ])
    ],
    [
        have_zstd="no"
    ])

AS_IF(
    [test "x${have_zlib}" = "xyes"],
    [
     AC_DEFINE([HAVE_ZLIB], [1], [Set to 1 if zlib decompression is available.])
     AC_DEFINE([HAVE_ZLIB_H], [1], [Set to 1 if zlib.h header file is available.])
    ])

AS_IF(
    [test "x${have_zstd}" = "xyes"],
    [
     AC_DEFINE([HAVE_ZSTD], [1], [Set to 1 if zstd decompression is available.])
     AC_DEFINE([HAVE_ZSTD_H], [1], [Set to 1 if zstd.h header file is available.])
    ])
AC_SUBST([requirements_libdwarf_libs])

### Checks for system services

AC_CONFIG_FILES([
Makefile
src/lib/libdwarf/Makefile
src/lib/libdwarfp/Makefile
src/bin/dwarfdump/Makefile
src/bin/dwarfgen/Makefile
src/bin/dwarfexample/Makefile
src/bin/gennames/Makefile
src/bin/tag_attr/Makefile
src/bin/tag_tree/Makefile
src/bin/attr_form/Makefile
src/bin/buildopstab/Makefile
src/bin/checkDW_LANG/Makefile
src/bin/buildDW_LNAME/Makefile
src/bin/builduritable/Makefile
test/Makefile
fuzz/Makefile
doc/Makefile
libdwarf.pc
libdwarfp.pc
])

AC_OUTPUT

echo
echo "$PACKAGE $VERSION"
echo
echo "Configuration Options Summary:"
echo
echo "  BuildOS..............: ${build_os}"
echo "  HostOS...............: ${host_os}"
echo
echo "  shared library.......: ${enable_shared}"
echo "  static library.......: ${enable_static}"
echo
echo "  zlib support.........: ${have_zlib}"
echo "  zstd support.........: ${have_zstd}"
echo "  mmap supported.......: ${have_mmap}"
echo "  sanitize support.....: ${enable_sanitize}"
echo "  documentation........: ${enable_doc}"
echo "  BuildOS-BigEndian....: ${ac_cv_c_bigendian}"
echo
echo "  libdwarf.............: always"
echo "  dwarfdump............: always"
echo "  dwarfdump print utf-8: ${have_utf8}"
echo "  dwarfgen.............: ${enable_dwarfgen}"
echo "  dwarfexample.........: ${enable_dwarfexample}"
echo
echo "Compilation............: make (or gmake)"
echo "  CPPFLAGS.............: $CPPFLAGS"
echo "  CFLAGS...............: $CFLAGS ${c_compiler_flags}"
echo "  LDFLAGS..............: $LDFLAGS"
echo "  LIBS.................: $LIBS"
echo "  ZLIB_LIBS............: $ZLIB_LIBS"
echo "  ZSTD_LIBS............: $ZSTD_LIBS"
echo "  DWARF_CFLAGS.........: $DWARF_CFLAGS"
echo "  DWARF_LIBS...........: $DWARF_LIBS"
echo "  DWARFGEN_LIBS........: $DWARFGEN_LIBS"
echo
echo "Installation...........: make install (as root if needed, with 'su' or 'sudo')"
echo "  prefix...............: $prefix"
echo
