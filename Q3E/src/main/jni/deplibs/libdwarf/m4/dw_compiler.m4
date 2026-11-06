dnl Copyright (C) 2018 Vincent Torri <vincent dot torri at gmail dot com>
dnl This code is public domain and can be freely used or copied.

dnl Macro that check if compiler flags are available


dnl Macro that checks for a C compiler flag availability
dnl
dnl _DWARF_CHECK_C_COMPILER_FLAG(FLAGS)
dnl AC_SUBST : DWARF_CFLAGS_WARN
dnl have_flag: yes or no.
AC_DEFUN([_DWARF_CHECK_C_COMPILER_FLAG],
[dnl

dnl store in options -Wfoo if -Wno-foo is passed
option="m4_bpatsubst([[$1]], [-Wno-], [-W])"
CFLAGS_save="${CFLAGS}"
CFLAGS="${CFLAGS} ${option}"
AC_LANG_PUSH([C])

AC_MSG_CHECKING([whether the C compiler supports $1])
AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM([[]])],
   [have_flag="yes"],
   [have_flag="no"])
AC_MSG_RESULT([${have_flag}])

AC_LANG_POP([C])
CFLAGS="${CFLAGS_save}"

AS_IF(
    [test "x${have_flag}" = "xyes"],
    [DWARF_CFLAGS_WARN="${DWARF_CFLAGS_WARN} [$1]"])

AC_SUBST(DWARF_CFLAGS_WARN)dnl
])

dnl DWARF_CHECK_C_COMPILER_FLAGS(FLAGS)
dnl Checks if FLAGS are supported and add to DWARF_CFLAGS.
dnl
dnl It will first try every flag at once, if one fails we will try
dnl them one by one.
AC_DEFUN([DWARF_CHECK_C_COMPILER_FLAGS],
[dnl
_DWARF_CHECK_C_COMPILER_FLAG([$1])
AS_IF(
    [test "${have_flag}" != "yes"],
    [m4_foreach_w([flag], [$1], [_DWARF_CHECK_C_COMPILER_FLAG(m4_defn([flag]))])])
])


dnl Macro that checks for a C++ compiler flag availability
dnl
dnl _DWARF_CHECK_CXX_COMPILER_FLAG(FLAGS)
dnl AC_SUBST : DWARF_CXXFLAGS_WARN
dnl have_flag: yes or no.
AC_DEFUN([_DWARF_CHECK_CXX_COMPILER_FLAG],
[dnl

dnl store in options -Wfoo if -Wno-foo is passed
option="m4_bpatsubst([[$1]], [-Wno-], [-W])"
CXXFLAGS_save="${CXXFLAGS}"
CXXFLAGS="${CXXFLAGS} ${option}"
AC_LANG_PUSH([C++])

AC_MSG_CHECKING([whether the C++ compiler supports $1])
AC_COMPILE_IFELSE(
   [AC_LANG_PROGRAM([[]])],
   [have_flag="yes"],
   [have_flag="no"])
AC_MSG_RESULT([${have_flag}])

AC_LANG_POP([C++])
CXXFLAGS="${CXXFLAGS_save}"

AS_IF(
    [test "x${have_flag}" = "xyes"],
    [DWARF_CXXFLAGS_WARN="${DWARF_CXXFLAGS_WARN} [$1]"])

AC_SUBST(DWARF_CXXFLAGS_WARN)dnl
])

dnl DWARF_CHECK_CXX_COMPILER_FLAGS(FLAGS)
dnl Checks if FLAGS are supported and add to DWARF_CXXFLAGS.
dnl
dnl It will first try every flag at once, if one fails we will try
dnl them one by one.
AC_DEFUN([DWARF_CHECK_CXX_COMPILER_FLAGS],
[dnl
_DWARF_CHECK_CXX_COMPILER_FLAG([$1])
AS_IF(
    [test "${have_flag}" != "yes"],
    [m4_foreach_w([flag], [$1], [_DWARF_CHECK_CXX_COMPILER_FLAG(m4_defn([flag]))])])
])
